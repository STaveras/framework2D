// main.mm
// iOS entry point. UIKit owns the run loop, so instead of main.cpp's loop a
// CADisplayLink steps the engine once per display refresh. The game view is
// backed by a CAMetalLayer for RendererMTL; the touch controls sit on top.
//
// The touch controls are shown whenever no game controller is connected. With
// one connected they hide, but touching the screen brings them back until the
// controller is used again.
//
// Launch arguments use the desktop flags (--debug, --dbg-collision,
// --deterministic, --fixed-dt-ms, --static-bg), e.g.
//   xcrun simctl launch booted <bundle id> --debug --dbg-collision
#if !__has_feature(objc_arc)
#error "main.mm must be compiled with -fobjc-arc"
#endif

#import <GameController/GameController.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include "../Engine2D.h"
#include "../FantasySideScroller/FantasySideScroller.h"
#include "../Renderer.h"
#include "../System.h"
#include "../Window.h"
#include "Input.h"
#import "TouchControls.h"

#include <cmath>
#include <exception>
#include <string>
#include <vector>

@interface FWMetalView : UIView
@end

@implementation FWMetalView

+ (Class)layerClass
{
	return [CAMetalLayer class];
}

- (void)didMoveToWindow
{
	[super didMoveToWindow];
	// Render at the screen's full pixel density.
	if (self.window) {
		self.contentScaleFactor = self.traitCollection.displayScale;
	}
}

@end

@interface FWGameViewController : UIViewController
@end

@implementation FWGameViewController {
	FWMetalView* _gameView;
	FWTouchControlsView* _controls;
	CADisplayLink* _displayLink;
	TouchGamepadState _touchState;
	Window* _window;
	IOSInput* _input;
	IRenderer* _renderer;
	BOOL _started;
	BOOL _touchPreferred; // touched the screen since last using a controller
}

- (void)loadView
{
	UIView* root = [[UIView alloc] initWithFrame:CGRectZero];
	root.backgroundColor = UIColor.blackColor;

	_gameView = [[FWMetalView alloc] initWithFrame:root.bounds];
	_gameView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[root addSubview:_gameView];

	_controls = [[FWTouchControlsView alloc] initWithState:&_touchState];
	_controls.frame = root.bounds;
	_controls.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[root addSubview:_controls];

	self.view = root;
}

- (void)viewDidLoad
{
	[super viewDidLoad];

	NSNotificationCenter* center = NSNotificationCenter.defaultCenter;
	[center addObserver:self selector:@selector(controllersChanged:) name:GCControllerDidConnectNotification object:nil];
	[center addObserver:self selector:@selector(controllersChanged:) name:GCControllerDidDisconnectNotification object:nil];
	[center addObserver:self selector:@selector(willResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
	[center addObserver:self selector:@selector(didBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
	[self updateTouchControls];
}

- (void)dealloc
{
	[_displayLink invalidate];
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];
	// The game's logical width follows the screen, so wait for a real layout.
	if (!_started && _gameView.bounds.size.width > 0.0 && _gameView.bounds.size.height > 0.0) {
		[self startGame];
	}
}

- (void)startGame
{
	_started = YES;

	// Relative paths, such as the AUTO_* recorders' logs, resolve inside the
	// app's container (the working directory is otherwise the read-only "/").
	FileSystem::SetWorkingDirectory(NSHomeDirectory().fileSystemRepresentation);

	NSString* dataPath = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"fantasySideScroller"];
	System::GlobalDataPath(dataPath.fileSystemRepresentation);

	// Launch arguments, for the same System::checkArguments* flags as the desktop.
	std::vector<std::string> arguments;
	for (NSString* argument in NSProcessInfo.processInfo.arguments) {
		arguments.emplace_back(argument.UTF8String);
	}
	std::vector<const char*> argv;
	for (const std::string& argument : arguments) {
		argv.push_back(argument.c_str());
	}
	const int argc = (int)argv.size();

	const bool enableDebug = System::checkArgumentsForDebugMode(argc, argv.data());
	enableDebug ? Debug::Mode.enable() : Debug::Mode.disable();
	if (enableDebug) {
		Debug::dbgCollision = System::checkArgumentsForCollisionDebug(argc, argv.data());
		Debug::dbgObjects = System::checkArgumentsForObjectDebug(argc, argv.data());
		Debug::dbgTiles = System::checkArgumentsForTileDebug(argc, argv.data());
	}
	double fixedDtMs = System::checkArgumentsForFixedDtMs(argc, argv.data(), 0.0);
	if (fixedDtMs <= 0.0) {
		fixedDtMs = 1000.0 / 60.0;
	}

	try {
		const CGSize size = _gameView.bounds.size;
		_window = new Window((int)size.width, (int)size.height, Engine2D::version());
		_window->setNativeView((__bridge void*)_gameView);
		_window->initialize();
		Renderer::mainWindow = _window;

		_input = new IOSInput(&_touchState);
		_renderer = Renderer::createMTLRenderer(_window);
		_renderer->setBackgroundStatic(System::checkArgumentsForStaticBackground(argc, argv.data()));

		Engine2D* engine = Engine2D::getInstance();
		engine->setDeterministicMode(System::checkArgumentsForDeterministic(argc, argv.data()));
		engine->setFixedDeltaSeconds(fixedDtMs / 1000.0);
		engine->setRenderInterpolation(true);
		engine->setInputInterface(_input);
		engine->setRenderer(_renderer);
		engine->setGame(&game);
		engine->initialize();
	}
	catch (const std::exception& e) {
		NSLog(@"framework2D failed to start: %s", e.what());
		return;
	}

	_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(step:)];
	[_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)step:(CADisplayLink*)displayLink
{
	try {
		_window->update();
		Engine2D::getInstance()->update();
	}
	catch (const std::exception& e) {
		NSLog(@"framework2D: %s", e.what());
	}
}

- (void)willResignActive:(NSNotification*)notification
{
	_displayLink.paused = YES;
}

- (void)didBecomeActive:(NSNotification*)notification
{
	// Don't feed the time spent in the background to the simulation.
	Engine2D::getTimer()->reset();
	_displayLink.paused = NO;
}

- (void)controllersChanged:(NSNotification*)notification
{
	[self updateTouchControls];
}

- (void)updateTouchControls
{
	BOOL hasController = NO;
	for (GCController* controller in GCController.controllers) {
		if (controller.extendedGamepad) {
			NSLog(@"framework2D: game controller %@ (%@)", controller.vendorName, controller.productCategory);
			[self watchController:controller];
			hasController = YES;
		}
	}
	[_controls setShown:(!hasController || _touchPreferred) animated:_started];
}

// Hide the touch controls again once a controller is used.
- (void)watchController:(GCController*)controller
{
	__weak FWGameViewController* weakSelf = self;
	controller.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad* gamepad, GCControllerElement* element) {
		BOOL used = NO;
		if ([element isKindOfClass:[GCControllerButtonInput class]]) {
			used = ((GCControllerButtonInput*)element).isPressed;
		}
		else if ([element isKindOfClass:[GCControllerDirectionPad class]]) {
			GCControllerDirectionPad* pad = (GCControllerDirectionPad*)element;
			used = std::fabs(pad.xAxis.value) > 0.5f || std::fabs(pad.yAxis.value) > 0.5f;
		}
		FWGameViewController* strongSelf = weakSelf;
		if (used && strongSelf && strongSelf->_touchPreferred) {
			strongSelf->_touchPreferred = NO;
			[strongSelf updateTouchControls];
		}
	};
}

// Touches reach the view controller only while the touch controls are hidden.
- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	if (!_controls.shown) {
		_touchPreferred = YES;
		[self updateTouchControls];
	}
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
	return YES;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures
{
	// Swipes from the edges go to the touch controls first.
	return UIRectEdgeAll;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	// iPads must allow every orientation (and resizable windows); the renderer
	// letterboxes the game to whatever shape the view has.
	return (self.traitCollection.userInterfaceIdiom == UIUserInterfaceIdiomPad)
		? UIInterfaceOrientationMaskAll
		: UIInterfaceOrientationMaskLandscape;
}

@end

@interface FWSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation FWSceneDelegate

- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)connectionOptions
{
	self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
	self.window.rootViewController = [FWGameViewController new];
	[self.window makeKeyAndVisible];
}

@end

@interface FWAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation FWAppDelegate

- (UISceneConfiguration*)application:(UIApplication*)application
	configurationForConnectingSceneSession:(UISceneSession*)connectingSceneSession
	options:(UISceneConnectionOptions*)options
{
	UISceneConfiguration* configuration = [[UISceneConfiguration alloc] initWithName:@"Default"
	                                                                     sessionRole:connectingSceneSession.role];
	configuration.delegateClass = [FWSceneDelegate class];
	return configuration;
}

@end

int main(int argc, char* argv[])
{
	@autoreleasepool {
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([FWAppDelegate class]));
	}
}
