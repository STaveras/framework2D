#include "../src/Engine2D.h"
#include "../src/Animation.h"
#include "../src/Camera.h"
#include "../src/IRenderer.h"
#include "../src/Sprite.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& message)
{
	std::cerr << "FAIL: " << message << '\n';
	std::exit(1);
}

void require(bool condition, const std::string& message)
{
	if (!condition) fail(message);
}

// Records what reaches the backend, and what drawing every list item one by one
// (no culling chunks) would have handed it.
class RecordingRenderer final : public IRenderer
{
public:
	std::vector<const Sprite*> drawn;

	RecordingRenderer(void) : IRenderer(RENDERER_TYPE_NULL, 640, 480) {}

	ITexture* createTexture(const char*, Color) override { return nullptr; }
	void initialize(void) override {}
	void shutdown(void) override {}
	void render(void) override
	{
		drawn.clear();
		_drawRenderLists(false);
	}

	std::vector<const Sprite*> expected(void)
	{
		std::vector<const Sprite*> result;
		for (size_t i = 0; i < getRenderListCount(); ++i) {
			RenderList* list = getRenderList(i);
			if (!list || list->screenSpace) continue;
			vector2 viewMin, viewMax;
			_viewBounds(_parallaxCameraPosition(*list), viewMin, viewMax);
			for (Renderable* renderable : *list) {
				if (!renderable || !renderable->isVisible()) continue;
				const Sprite* sprite = nullptr;
				vector2 offset = renderable->getOffset();
				if (renderable->getRenderableType() == RENDERABLE_TYPE_SPRITE) {
					sprite = static_cast<const Sprite*>(renderable);
				}
				else if (renderable->getRenderableType() == RENDERABLE_TYPE_ANIMATION) {
					Animation* animation = static_cast<Animation*>(renderable);
					Frame* frame = animation->getFrameCount() ? animation->getCurrentFrame() : nullptr;
					sprite = frame ? frame->getSprite() : nullptr;
				}
				if (sprite && quadOverlaps(sprite, offset, viewMin, viewMax)) {
					result.push_back(sprite);
				}
			}
		}
		return result;
	}

protected:
	void _renderSprite(Sprite* sprite, Color, const vector2&, const RenderList&) override { drawn.push_back(sprite); }

private:
	// The quad every backend draws, transformed corner by corner.
	static bool quadOverlaps(const Sprite* sprite, vector2 offset, vector2 viewMin, vector2 viewMax)
	{
		const RECT& rect = sprite->getSrcRect();
		const vector2 position = sprite->getPosition() + offset;
		const vector2 center = sprite->getCenter();
		const vector2 scale = sprite->getScale();
		const float c = std::cos(sprite->getRotation()), sn = std::sin(sprite->getRotation());
		const float xs[4] = { -center.x, (rect.right - rect.left) - center.x, (rect.right - rect.left) - center.x, -center.x };
		const float ys[4] = { -center.y, -center.y, (rect.bottom - rect.top) - center.y, (rect.bottom - rect.top) - center.y };
		vector2 lo(INFINITY, INFINITY), hi(-INFINITY, -INFINITY);
		for (int i = 0; i < 4; ++i) {
			const float x = xs[i] * scale.x, y = ys[i] * scale.y;
			const float px = position.x + c * x - sn * y, py = position.y + sn * x + c * y;
			lo.x = std::min(lo.x, px); lo.y = std::min(lo.y, py);
			hi.x = std::max(hi.x, px); hi.y = std::max(hi.y, py);
		}
		return !(hi.x < viewMin.x || lo.x > viewMax.x || hi.y < viewMin.y || lo.y > viewMax.y);
	}
};

std::unique_ptr<Sprite> makeTile(float x, float y)
{
	auto sprite = std::make_unique<Sprite>(nullptr, RECT{ 0, 0, 16, 16 });
	sprite->setPosition(vector2(x, y));
	return sprite;
}

void requireMatches(RecordingRenderer& renderer, const std::string& when)
{
	renderer.render();
	const std::vector<const Sprite*> expected = renderer.expected();
	require(renderer.drawn == expected, when + ": drew " + std::to_string(renderer.drawn.size()) +
		" sprites, expected " + std::to_string(expected.size()) + " (or a different order)");
}

// A tile layer's worth of sprites in row-major order, plus a moving camera, moving
// tiles, and every kind of transform and membership change.
void testTileLayer()
{
	RecordingRenderer renderer;
	Camera camera;
	renderer.setCamera(&camera);
	camera.setPosition(vector2(320.0f, 240.0f));

	IRenderer::RenderList* layer = renderer.createRenderList();
	IRenderer::RenderList* parallax = renderer.createRenderList();
	parallax->parallaxX = 0.4f;
	parallax->parallaxY = 0.4f;
	std::vector<std::unique_ptr<Sprite>> tiles;
	for (int row = 0; row < 26; ++row) {
		for (int column = 0; column < 142; ++column) {
			tiles.push_back(makeTile(column * 16.0f, row * 16.0f));
			((row + column) % 7 ? layer : parallax)->push_back(tiles.back().get());
		}
	}
	requireMatches(renderer, "first frame");

	for (float x = -600.0f; x < 2800.0f; x += 37.0f) {
		camera.setPosition(vector2(x, 200.0f + std::sin(x) * 150.0f));
		requireMatches(renderer, "camera at x=" + std::to_string(x));
	}

	camera.setPosition(vector2(320.0f, 240.0f));
	Sprite* far = tiles[100].get();
	far->setPosition(vector2(300.0f, 200.0f));
	requireMatches(renderer, "tile moved into view");
	tiles[5]->setPosition(vector2(5000.0f, 5000.0f));
	requireMatches(renderer, "tile moved out of view");

	Sprite* edited = tiles[130].get(); // off screen to the right
	edited->setScale(vector2(-40.0f, 1.0f));
	requireMatches(renderer, "setScale");
	edited->setScale(vector2(1.0f, 1.0f));
	edited->setRotation(3.0f);
	edited->setCenter(vector2(400.0f, 0.0f));
	requireMatches(renderer, "setRotation/setCenter");
	edited->setRotation(0.0f);
	edited->setCenter(vector2(0.0f, 0.0f));
	edited->setOffset(vector2(-1500.0f, 0.0f));
	requireMatches(renderer, "setOffset");
	edited->setOffset(vector2(0.0f, 0.0f));
	edited->setSrcRect(RECT{ 0, 0, 16, 16 });
	edited->setCenter(vector2(-20.0f, 0.0f));
	edited->mirror(true, false);
	requireMatches(renderer, "mirror");
	tiles[131]->setSrcRect(RECT{ 0, 0, 2000, 16 });
	tiles[131]->setPosition(vector2(-1500.0f, 32.0f));
	requireMatches(renderer, "setSrcRect");

	tiles[200]->setVisibility(false);
	requireMatches(renderer, "hidden sprite");

	// Membership: removed, moved to another list, added twice, sitting in two lists.
	layer->remove(tiles[1].get());
	parallax->push_back(tiles[1].get());
	requireMatches(renderer, "sprite moved between lists");
	layer->push_back(tiles[2].get());
	parallax->push_front(tiles[3].get());
	requireMatches(renderer, "sprite in two lists and twice in one");
	tiles[2]->setPosition(vector2(310.0f, 210.0f));
	tiles[3]->setPosition(vector2(330.0f, 220.0f));
	requireMatches(renderer, "shared sprites moved");
	layer->erase(layer->begin());
	layer->insert(std::next(layer->begin(), 10), tiles[4].get());
	requireMatches(renderer, "erase/insert");

	// A copy reports to no chunk; editing it must not disturb the original's list.
	Sprite copy(*tiles[6]);
	copy.setPosition(vector2(100000.0f, 0.0f));
	requireMatches(renderer, "copied sprite moved");
	layer->push_back(&copy);
	copy.setPosition(vector2(320.0f, 240.0f));
	requireMatches(renderer, "copied sprite added");
	layer->remove(&copy);

	// Animations sit between chunks and keep list order.
	Animation animation;
	Sprite frameA(nullptr, RECT{ 0, 0, 16, 16 }), frameB(nullptr, RECT{ 0, 0, 16, 16 });
	animation.createFrame(&frameA, 0.1f);
	animation.createFrame(&frameB, 0.1f);
	animation.setPosition(vector2(330.0f, 230.0f));
	layer->insert(std::next(layer->begin(), 50), &animation);
	requireMatches(renderer, "animation between chunks");
	animation.play();
	animation.update(0.15f);
	requireMatches(renderer, "animation advanced a frame");

	// Random churn: moves big and small, over many frames.
	std::mt19937 random(1234);
	std::uniform_int_distribution<size_t> pick(0, tiles.size() - 1);
	std::uniform_real_distribution<float> coordinate(-800.0f, 3000.0f);
	std::uniform_real_distribution<float> nudge(-3.0f, 3.0f);
	for (int frame = 0; frame < 300; ++frame) {
		for (int change = 0; change < 20; ++change) {
			Sprite* tile = tiles[pick(random)].get();
			if (change % 4 == 0) tile->setPosition(vector2(coordinate(random), coordinate(random) * 0.2f));
			else tile->setPosition(tile->getPosition() + vector2(nudge(random), nudge(random)));
		}
		camera.setPosition(vector2(coordinate(random), 240.0f));
		requireMatches(renderer, "random churn frame " + std::to_string(frame));
	}

	renderer.setCamera(nullptr);
}

// Lists and sprites torn down in either order leave no dangling chunk links.
void testTeardown()
{
	RecordingRenderer renderer;
	Camera camera;
	renderer.setCamera(&camera);
	camera.setPosition(vector2(320.0f, 240.0f));

	std::vector<std::unique_ptr<Sprite>> survivors;
	{
		IRenderer::RenderList* doomed = renderer.createRenderList();
		std::vector<std::unique_ptr<Sprite>> deleted;
		for (int i = 0; i < 200; ++i) {
			survivors.push_back(makeTile(i * 16.0f, 0.0f));
			deleted.push_back(makeTile(i * 16.0f, 16.0f));
			doomed->push_back(survivors.back().get());
			doomed->push_back(deleted.back().get());
		}
		requireMatches(renderer, "list before teardown");
		doomed->clear();
		deleted.clear();
		requireMatches(renderer, "list emptied, then its sprites deleted");
		for (auto& sprite : survivors) doomed->push_back(sprite.get());
		requireMatches(renderer, "list refilled");
		renderer.destroyRenderList(doomed);
	}
	// Sprites keep their old chunk ids; moving them, or filing them in a new list
	// that reuses those ids, must still be safe and correct.
	for (auto& sprite : survivors) sprite->setPosition(sprite->getPosition() + vector2(1.0f, 1.0f));
	IRenderer::RenderList* fresh = renderer.createRenderList();
	std::vector<std::unique_ptr<Sprite>> others;
	for (int i = 0; i < 100; ++i) {
		others.push_back(makeTile(i * 16.0f, 64.0f));
		fresh->push_back(others.back().get());
	}
	requireMatches(renderer, "new list reusing freed chunk ids");
	for (int i = 0; i < 100; i += 2) fresh->push_back(survivors[i].get());
	survivors[0]->setPosition(vector2(320.0f, 240.0f));
	requireMatches(renderer, "survivors filed again");
	renderer.setCamera(nullptr);
}

} // namespace

int main()
{
	Engine2D::getEventSystem()->initialize(INFINITE);
	testTileLayer();
	testTeardown();
	std::cout << "PASS: chunk-culled render lists draw exactly what per-sprite culling draws, in order\n";
	return 0;
}
