#include "../src/Factory.h"
#include "../src/IRenderer.h"

#include <cassert>
#include <iostream>

namespace {

struct Counted
{
	static int alive;
	Counted(void) { ++alive; }
	Counted(const Counted&) { ++alive; }
	~Counted(void) { --alive; }
};
int Counted::alive = 0;

class TestRenderer final : public IRenderer
{
public:
	ITexture* createTexture(const char*, Color) override { return nullptr; }
	void initialize(void) override {}
	void shutdown(void) override {}
	void render(void) override {}
};

}

int main()
{
	{
		// Created and adopted items are owned; borrowed ones are not deleted.
		Counted borrowed;
		{
			Factory<Counted> factory;
			factory.create();
			factory.store(new Counted());
			factory.store(&borrowed, false);
			assert(Counted::alive == 3);
			assert(factory.owns(factory.at(0)) && factory.owns(factory.at(1)));
			assert(!factory.owns(&borrowed));
			factory.destroy(&borrowed);
			assert(factory.size() == 2 && Counted::alive == 3);
			factory.store(&borrowed, false);
		}
		assert(Counted::alive == 1);
	}
	assert(Counted::alive == 0);

	{
		// erase(index) removes exactly that item.
		Factory<Counted> factory;
		Counted* first = factory.create();
		factory.create();
		Counted* third = factory.create();
		factory.erase(1u);
		assert(factory.size() == 2 && factory.at(0) == first && factory.at(1) == third);
		assert(Counted::alive == 2);
	}
	assert(Counted::alive == 0);

	{
		// Pushed render lists stay the caller's; popping the last one is safe.
		IRenderer::RenderList external;
		TestRenderer renderer;
		renderer.pushRenderList(&external);
		assert(renderer.getRenderListCount() == 2);
		renderer.popRenderList();
		assert(renderer.getRenderListCount() == 1);
		renderer.popRenderList();
		renderer.popRenderList();
		assert(renderer.getRenderListCount() == 0);
		renderer.pushRenderList(&external);
	}

	std::cout << "factory_test passed\n";
	return 0;
}
