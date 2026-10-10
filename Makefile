# Build settings
CXX ?= c++
STD := -std=c++17
WARN := -Wall -Wextra
SUPPRESS_UNUSED ?= 1
ifeq ($(SUPPRESS_UNUSED),1)
  WARN += -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
endif
DIAG := -fdiagnostics-color=always
INCLUDES := -I./ext -isystem ./ext/inc -isystem ./ext/inc/metal-cpp
LIBS := -lglfw -lvulkan -ltinyxml2 -lsimdjson

# Debug (make DEBUG=1)
ifeq ($(DEBUG),1)
  DEBUG_FLAGS := -D_DEBUG -g -O0
else
  DEBUG_FLAGS := -O2 -DNDEBUG
endif

UNAME_S := $(shell uname -s)

# Platform-specific flags
ifeq ($(UNAME_S),Darwin)
  CXX := clang++
  PLATFORM_COMPILE_FLAGS := -stdlib=libc++ -DGL_SILENCE_DEPRECATION -x objective-c++
  PLATFORM_LINK_FLAGS := -stdlib=libc++ -framework Metal -framework QuartzCore -framework OpenGL -framework Cocoa
  # Ensure runtime can find Vulkan dylib on common macOS prefixes
  PLATFORM_RPATH := -Wl,-rpath,/usr/local/lib -Wl,-rpath,/opt/homebrew/lib
  PLATFORM_SRCS := src/RendererMTL.mm src/TextureMTL.mm
else
  PLATFORM_COMPILE_FLAGS :=
  PLATFORM_LINK_FLAGS :=
  PLATFORM_RPATH :=
  PLATFORM_SRCS :=
endif

COMMON_SRCS := \
  src/main.cpp src/Engine2D.cpp src/RenderInterpolation.cpp src/EventSystem.cpp src/ProgramStack.cpp src/InputManager.cpp src/StateMachine.cpp \
  src/ObjectManager.cpp src/CollisionSystem.cpp src/Game.cpp src/GameObject.cpp src/GameState.cpp src/Frame.cpp src/Animation.cpp \
  src/AnimationManager.cpp src/AnimationUtils.cpp src/Camera.cpp src/InputMap.cpp src/Timer.cpp \
  src/Window.cpp src/Player.cpp src/IRenderer.cpp src/Renderer.cpp src/RendererVK.cpp src/RendererGL.cpp \
  src/InputTapeRecorder.cpp src/PlatformMouse.cpp src/PlatformGamepad.cpp src/Font.cpp src/Cursor.cpp \
  src/Physical.cpp src/Kinematics2D.cpp src/Telemetry2D.cpp src/StrUtils.cpp \
  src/TextureVK.cpp src/TextureGL.cpp src/InputEvent.cpp src/IInput.cpp src/Trigger.cpp \
  src/UpdateBackgroundOperator.cpp src/SDSParser.cpp src/PlatformInput.cpp src/PlatformKeyboard.cpp src/System.cpp \
  src/ImageLoaders.cpp src/Sprite.cpp src/TileSet.cpp src/Debug.cpp \
  src/CollidableGroup.cpp src/Plane.cpp src/Square.cpp src/Circle.cpp src/Polygon.cpp src/PolygonDecomposition.cpp \
  src/FantasySideScroller/FantasySideScroller.cpp src/FantasySideScroller/LevelManager.cpp src/FantasySideScroller/PlayState.cpp \
  src/FantasySideScroller/PauseState.cpp src/FantasySideScroller/GameOverState.cpp src/FantasySideScroller/Character.cpp src/FantasySideScroller/Boar.cpp \
  src/FantasySideScroller/CharacterMovement.cpp src/FantasySideScroller/CharacterStateSetup.cpp src/FantasySideScroller/CharacterUpdate.cpp \
  src/FantasySideScroller/TraversalMechanics.cpp src/FantasySideScroller/LevelProps.cpp src/BlinkFlash.cpp \
  src/Actor.cpp src/PlayerController.cpp src/Widgets.cpp

SRCS := $(COMMON_SRCS) $(PLATFORM_SRCS)

TARGET_BASE := $(notdir $(CURDIR))
ifeq ($(DEBUG),1)
  TARGET := bin/$(TARGET_BASE)_d
  OBJDIR := build/obj_d
else
  TARGET := bin/$(TARGET_BASE)
  OBJDIR := build/obj_release
endif
OBJS := $(SRCS:%.cpp=$(OBJDIR)/%.o)
OBJS := $(OBJS:%.mm=$(OBJDIR)/%.o)

CPPFLAGS := $(INCLUDES)
CXXFLAGS := $(STD) $(WARN) $(DIAG) $(DEBUG_FLAGS) $(PLATFORM_COMPILE_FLAGS)
LDFLAGS := $(LIBS) $(PLATFORM_LINK_FLAGS) $(PLATFORM_RPATH)

.PHONY: all clean shaders

all: $(TARGET) shaders

# Vulkan shaders, compiled to where RendererVK loads them (as utl/compile-shaders.sh does;
# the CMake and Visual Studio builds run that script after linking).
GLSLC ?= glslc
SHADER_SRC_DIR := bin/fantasySideScroller/Shaders
SHADER_OUT_DIR := bin/cache/shader
SHADERS := $(SHADER_OUT_DIR)/tri.v.spv $(SHADER_OUT_DIR)/tri.f.spv

ifeq ($(shell command -v $(GLSLC) 2>/dev/null),)
shaders:
	@echo "warning: $(GLSLC) not found; Vulkan shaders in $(SHADER_OUT_DIR) were not rebuilt"
else
shaders: $(SHADERS)
endif

$(SHADER_OUT_DIR)/tri.v.spv: $(SHADER_SRC_DIR)/triangle.vert
	@mkdir -p $(dir $@)
	$(GLSLC) $< -o $@

$(SHADER_OUT_DIR)/tri.f.spv: $(SHADER_SRC_DIR)/triangle.frag
	@mkdir -p $(dir $@)
	$(GLSLC) $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o $@ $(LDFLAGS)

# C++ sources
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Objective-C++ sources (macOS)
$(OBJDIR)/%.o: %.mm
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# The Metal backend uses ARC; everything else is manually reference counted.
$(OBJDIR)/src/RendererMTL.o $(OBJDIR)/src/TextureMTL.o: CXXFLAGS += -fobjc-arc

-include $(OBJS:.o=.d)
# Test objects under tools/ need their header dependencies too, or they go stale.
-include $(wildcard $(OBJDIR)/tools/*.d)

clean:
	rm -rf build/obj build/obj_release build/obj_d bin/$(TARGET_BASE) bin/$(TARGET_BASE)_d

.PHONY: test-boar
# Keep test assertions active even when linking optimized engine objects.
$(OBJDIR)/tools/boar_smoke_test.o: CXXFLAGS += -UNDEBUG
test-boar: $(filter-out $(OBJDIR)/src/main.o,$(OBJS)) $(OBJDIR)/tools/boar_smoke_test.o
	$(CXX) $^ -o $(OBJDIR)/boar_smoke_test $(LDFLAGS)
	./$(OBJDIR)/boar_smoke_test

.PHONY: test-tile-flip
$(OBJDIR)/tools/tile_flip_test.o: CXXFLAGS += -UNDEBUG
test-tile-flip: $(filter-out $(OBJDIR)/src/main.o,$(OBJS)) $(OBJDIR)/tools/tile_flip_test.o
	$(CXX) $^ -o $(OBJDIR)/tile_flip_test $(LDFLAGS)
	./$(OBJDIR)/tile_flip_test

.PHONY: test-level-props
$(OBJDIR)/tools/level_props_test.o: CXXFLAGS += -UNDEBUG
test-level-props: $(filter-out $(OBJDIR)/src/main.o,$(OBJS)) $(OBJDIR)/tools/level_props_test.o
	$(CXX) $^ -o $(OBJDIR)/level_props_test $(LDFLAGS)
	./$(OBJDIR)/level_props_test

.PHONY: test-render-culling
$(OBJDIR)/tools/render_culling_test.o: CXXFLAGS += -UNDEBUG
test-render-culling: $(filter-out $(OBJDIR)/src/main.o,$(OBJS)) $(OBJDIR)/tools/render_culling_test.o
	$(CXX) $^ -o $(OBJDIR)/render_culling_test $(LDFLAGS)
	./$(OBJDIR)/render_culling_test

.PHONY: test-section-transition
$(OBJDIR)/tools/section_transition_test.o: CXXFLAGS += -UNDEBUG
test-section-transition: $(filter-out $(OBJDIR)/src/main.o,$(OBJS)) $(OBJDIR)/tools/section_transition_test.o
	$(CXX) $^ -o $(OBJDIR)/section_transition_test $(LDFLAGS)
	./$(OBJDIR)/section_transition_test

# iOS: an Xcode project generated from the root CMakeLists (Metal renderer,
# touch controls). `make ios` builds it for the simulator; `make ios-run` also boots
# IOS_SIMULATOR, installs the app and launches it.
IOS_BUILD := build/ios
IOS_CONFIG ?= Debug
IOS_SIMULATOR ?= iPhone 17 Pro
IOS_APP = $(IOS_BUILD)/$(IOS_CONFIG)-iphonesimulator/framework2D.app

.PHONY: ios-project ios ios-run
ios-project:
	cmake -S . -B $(IOS_BUILD) -G Xcode -DFRAMEWORK_IOS=ON

ios: ios-project
	xcodebuild -project $(IOS_BUILD)/framework2D.xcodeproj -scheme framework2D \
	  -configuration $(IOS_CONFIG) -sdk iphonesimulator \
	  -destination 'platform=iOS Simulator,name=$(IOS_SIMULATOR)' build

ios-run: ios
	xcrun simctl boot '$(IOS_SIMULATOR)' 2>/dev/null || true
	open -a Simulator
	xcrun simctl install '$(IOS_SIMULATOR)' '$(IOS_APP)'
	xcrun simctl launch --terminate-running-process '$(IOS_SIMULATOR)' \
	  "$$(/usr/libexec/PlistBuddy -c 'Print CFBundleIdentifier' '$(IOS_APP)/Info.plist')"
