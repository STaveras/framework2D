// Checks animation loading from JSON: frames listed inline in Animations.json or in a
// frameData.json beside the animation's image, an image named once on the animation, and
// frame collision stored as Tiled objectgroups in frame pixels. Also checks that toJSON
// writes what fromJSON reads back.
#include "../src/Animation.h"
#include "../src/AnimationUtils.h"
#include "../src/CollidableGroup.h"
#include "../src/Engine2D.h"
#include "../src/Polygon.h"
#include "../src/Square.h"
#include "../src/System.h"
#include "stb/stb_image.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

class TestTexture : public ITexture {
    unsigned int width = 0, height = 0;
public:
    explicit TestTexture(const char* path) : ITexture(path) {
        int w = 0, h = 0, channels = 0;
        const bool loaded = stbi_info(path, &w, &h, &channels) != 0;
        assert(loaded);
        width = w;
        height = h;
    }
    unsigned int getWidth() const override { return width; }
    unsigned int getHeight() const override { return height; }
};

class TestRenderer : public IRenderer {
public:
    ITexture* createTexture(const char* path, Color = 0) override {
        auto* texture = new TestTexture(path);
        m_Textures.store(texture);
        return texture;
    }
    void initialize() override {}
    void shutdown() override {}
    void render() override {}
};

bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }
bool near(vector2 a, vector2 b) { return near(a.x, b.x) && near(a.y, b.y); }

// collidable was built into the frame's own collision objects (not borrowed from elsewhere).
bool heldBy(Frame* frame, const Collidable* collidable)
{
    Factory<Collidable>& objects = frame->getCollisionObjects();
    return std::find(objects.begin(), objects.end(), collidable) != objects.end() && objects.owns(collidable);
}

bool endsWith(const std::string& text, const std::string& suffix)
{
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Animation* find(const std::vector<Animation*>& animations, const char* name)
{
    for (Animation* animation : animations) {
        if (std::strcmp(animation->getName(), name) == 0) {
            return animation;
        }
    }
    return nullptr;
}

// Every frame has a Square owned by the frame at (x, y) in the animation's space.
void expectSquareOnEveryFrame(Animation* animation, float x, float y, float width, float height)
{
    assert(animation && animation->getFrameCount() > 0);
    for (Frame* frame : animation->getFrames()) {
        Collidable* collidable = frame->getCollidable();
        assert(collidable && collidable->getType() == COL_OBJ_SQUARE);
        assert(heldBy(frame, collidable));
        const Square* square = (const Square*)collidable;
        assert(near(square->getPosition(), vector2(x, y)));
        assert(near(square->getWidth(), width) && near(square->getHeight(), height));
    }
}

void write(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream file(path);
    file << text;
}

std::string read(const std::filesystem::path& path)
{
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

size_t count(const std::string& text, const std::string& needle)
{
    size_t found = 0;
    for (size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + needle.size())) {
        ++found;
    }
    return found;
}

// The non-group colliders under collidable, in order (positions relative to (0, 0) groups).
std::vector<const Collidable*> leaves(const Collidable* collidable)
{
    std::vector<const Collidable*> output;
    if (!collidable) {
        return output;
    }
    if (collidable->getType() != COL_OBJ_GROUP) {
        output.push_back(collidable);
        return output;
    }
    for (const Collidable* member : *(const CollidableGroup*)collidable) {
        for (const Collidable* leaf : leaves(member)) {
            output.push_back(leaf);
        }
    }
    return output;
}

void destroy(std::vector<Animation*>& animations)
{
    for (Animation* animation : animations) {
        Animations::destroyAnimation(animation);
    }
    animations.clear();
}

} // namespace

int main()
{
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);

    // The hero's colliders come from its frameData.json files (plus Attack02's inline
    // frames) and match the hit boxes CharacterStateSetup.cpp falls back to.
    {
        std::vector<Animation*> hero = Animations::fromJSON("bin/fantasySideScroller/Character/Animations.json", nullptr);
        assert(hero.size() == 14);
        for (Animation* animation : hero) {
            assert(animation->getFrameCount() > 0);
        }

        const float foot = 24.0f; // kLocomotionFootLocalY
        expectSquareOnEveryFrame(find(hero, "Idle"), -10, foot - 48, 20, 48);
        expectSquareOnEveryFrame(find(hero, "Rising"), -14, foot - 52, 20, 52);
        expectSquareOnEveryFrame(find(hero, "Jump"), -14, foot - 52, 20, 52);
        expectSquareOnEveryFrame(find(hero, "Landing"), -14, foot - 52, 20, 52);
        expectSquareOnEveryFrame(find(hero, "RunningLeft"), -10, foot - 42, 26, 42);
        expectSquareOnEveryFrame(find(hero, "RunningRight"), -10, foot - 42, 26, 42);
        expectSquareOnEveryFrame(find(hero, "Attack01"), -10, foot - 48, 20, 48);
        expectSquareOnEveryFrame(find(hero, "Attack02"), -10, foot - 48, 20, 48);
        expectSquareOnEveryFrame(find(hero, "CrouchIdle"), -10, foot - 34, 20, 34);
        expectSquareOnEveryFrame(find(hero, "CrouchWalk"), -10, foot - 34, 20, 34);
        expectSquareOnEveryFrame(find(hero, "Roll"), -10, foot - 28, 20, 28);
        expectSquareOnEveryFrame(find(hero, "Slide"), -12, foot - 24, 24, 24);
        expectSquareOnEveryFrame(find(hero, "WallSlide"), -14, foot - 52, 20, 52);

        Animation* dead = find(hero, "Dead");
        assert(near(dead->getOffset(), vector2(8, 8)));
        for (Frame* frame : dead->getFrames()) {
            assert(!frame->getCollidable());
        }
        assert(near(find(hero, "Rising")->getOffset(), vector2(0, -8)));
        assert(near(find(hero, "Idle")->getOffset(), vector2(0, 0)));

        // Frames are centred, and inherit the animation's image.
        Frame* idleFrame = (*find(hero, "Idle"))[0];
        assert(near(idleFrame->getSprite()->getCenter(), vector2(32, 40)));
        assert(endsWith(idleFrame->getSprite()->getTexture()->getFileName(), "Character/Idle/Idle-Sheet.png"));
        assert(endsWith((*find(hero, "Attack02"))[0]->getSprite()->getTexture()->getFileName(), "Character/Attack-01/Attack-01-Sheet.png"));
        assert(near((*find(hero, "Attack02"))[0]->getDuration(), 0.1f));
        assert(near(find(hero, "Landing")->getSpeed(), 2.7f));
        destroy(hero);
    }

    // Fixtures: an inline animation in the old per-frame "Filename" form, one read from its
    // folder's frameData.json, one from the file its "FrameData" names (not the one beside
    // its image), one naming a file that doesn't exist, one whose inline frames win over
    // both, and one with nothing to read.
    const std::filesystem::path dir = std::filesystem::absolute("tmp/animation_frame_data_test");
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir / "Folder");
    std::filesystem::create_directories(dir / "Empty");
    std::filesystem::create_directories(dir / "Other");
    const std::filesystem::path sheet = dir / "Folder" / "Sheet.png";
    std::filesystem::copy_file("bin/fantasySideScroller/Character/Idle/Idle-Sheet.png", sheet); // 256 x 80
    std::filesystem::copy_file("bin/fantasySideScroller/Character/Idle/Idle-Sheet.png", dir / "Empty" / "Sheet.png");

    write(dir / "Folder" / "frameData.json", R"({
  "Frame": [
    { "DisplayRect": "0,0,64,80", "Duration": 1,
      "objectgroup": { "draworder": "index", "objects": [
        { "id": 1, "x": 22, "y": 16, "width": 20, "height": 48, "rotation": 0, "type": "", "visible": true },
        { "id": 2, "x": 0, "y": 0, "width": 0, "height": 0, "polygon": [
          { "x": 0, "y": 0 }, { "x": 30, "y": 0 }, { "x": 30, "y": 10 }, { "x": 10, "y": 10 }, { "x": 10, "y": 30 }, { "x": 0, "y": 30 } ] }
      ], "type": "objectgroup" } },
    { "Duration": 0.25 }
  ]
})");

    write(dir / "Other" / "named.json", R"({ "Frame": [ { "DisplayRect": "192,0,64,80", "Duration": 0.75 } ] })");

    const std::string sheetPath = sheet.generic_string();
    const std::string emptyPath = (dir / "Empty" / "Sheet.png").generic_string();
    const std::string namedPath = (dir / "Other" / "named.json").generic_string();
    const std::string missingPath = (dir / "Other" / "missing.json").generic_string();
    write(dir / "Animations.json", R"({
  "AnimationFile": {
    "Animation": [
      { "Name": "Inline", "Mode": "LOOPING", "Forward": true, "Speed": 2, "Offset": "4,-6", "Frame": [
        { "DisplayRect": "64,0,64,80", "Filename": ")" + sheetPath + R"(", "Duration": 0.5,
          "objectgroup": { "objects": [ { "id": 1, "x": 32, "y": 40, "width": 8, "height": 4 } ] } } ] },
      { "Name": "FromFolder", "Filename": ")" + sheetPath + R"(", "Mode": "ONCE" },
      { "Name": "Named", "Filename": ")" + sheetPath + R"(", "FrameData": ")" + namedPath + R"(" },
      { "Name": "NamedMissing", "Filename": ")" + sheetPath + R"(", "FrameData": ")" + missingPath + R"(" },
      { "Name": "InlineWins", "Filename": ")" + sheetPath + R"(", "FrameData": ")" + namedPath + R"(",
        "Frame": [ { "DisplayRect": "128,0,64,80", "Duration": 0.5 } ] },
      { "Name": "Missing", "Filename": ")" + emptyPath + R"(" }
    ]
  }
})");

    std::vector<Animation*> loaded = Animations::fromJSON((dir / "Animations.json").string().c_str(), nullptr);
    assert(loaded.size() == 4);
    assert(!find(loaded, "Missing"));
    assert(!find(loaded, "NamedMissing"));

    // Inline frame: object pixels (32, 40) are the 64x80 frame's centre, moved by the offset.
    Animation* inlineAnimation = find(loaded, "Inline");
    assert(inlineAnimation->getFrameCount() == 1);
    assert(near(inlineAnimation->getSpeed(), 2.0f));
    assert(inlineAnimation->getMode() == Animation::eLoop);
    assert(near(inlineAnimation->getOffset(), vector2(4, -6)));
    expectSquareOnEveryFrame(inlineAnimation, 4, -6, 8, 4);

    // frameData.json: a rectangle and a concave polygon make a group; the second frame,
    // with no DisplayRect or collision, is the whole image without a collider.
    Animation* fromFolder = find(loaded, "FromFolder");
    assert(fromFolder->getFrameCount() == 2);
    assert(fromFolder->getMode() == Animation::eOnce);
    Frame* first = (*fromFolder)[0];
    assert(near(first->getDuration(), 1.0f));
    assert(first->getCollidable() && first->getCollidable()->getType() == COL_OBJ_GROUP);
    const CollidableGroup* group = (const CollidableGroup*)first->getCollidable();
    assert(group->size() == 2);
    const Collidable* rectangle = group->front();
    const Collidable* polygon = group->back();
    assert(rectangle->getType() == COL_OBJ_SQUARE && near(rectangle->getPosition(), vector2(-10, -24)));
    assert(polygon->getType() == COL_OBJ_GROUP); // the L shape, split into convex pieces
    for (const Collidable* piece : *(const CollidableGroup*)polygon) {
        assert(piece->getType() == COL_OBJ_POLYGON && near(piece->getPosition(), vector2(-32, -40)));
        assert(heldBy(first, piece));
    }
    Frame* second = (*fromFolder)[1];
    assert(!second->getCollidable());
    assert(second->getSprite()->getSrcRect().right == 256 && second->getSprite()->getSrcRect().bottom == 80);
    assert(near(second->getDuration(), 0.25f));

    Animation* named = find(loaded, "Named");
    assert(named->getFrameCount() == 1 && (*named)[0]->getSprite()->getSrcRect().left == 192);
    assert(near((*named)[0]->getDuration(), 0.75f));
    assert(endsWith((*named)[0]->getSprite()->getTexture()->getFileName(), "Folder/Sheet.png"));

    Animation* inlineWins = find(loaded, "InlineWins");
    assert(inlineWins->getFrameCount() == 1 && (*inlineWins)[0]->getSprite()->getSrcRect().left == 128);

    // toJSON names a shared image once, keeps the offset and writes collision back in
    // frame pixels; reading that file gives the same animations.
    const std::filesystem::path roundTripPath = dir / "RoundTrip.json";
    Animations::toJSON(loaded, roundTripPath.string().c_str());
    const std::string written = read(roundTripPath);
    assert(count(written, "\"Filename\"") == 4);
    assert(count(written, "\"Offset\": \"4,-6\"") == 1);

    std::vector<Animation*> reloaded = Animations::fromJSON(roundTripPath.string().c_str(), nullptr);
    assert(reloaded.size() == loaded.size());
    for (size_t i = 0; i < loaded.size(); ++i) {
        Animation* before = loaded[i];
        Animation* after = reloaded[i];
        assert(std::strcmp(before->getName(), after->getName()) == 0);
        assert(before->getMode() == after->getMode() && near(before->getSpeed(), after->getSpeed()));
        assert(near(before->getOffset(), after->getOffset()));
        assert(before->getFrameCount() == after->getFrameCount());
        for (unsigned int j = 0; j < before->getFrameCount(); ++j) {
            Frame* a = (*before)[j];
            Frame* b = (*after)[j];
            assert(near(a->getDuration(), b->getDuration()));
            assert(std::memcmp(&a->getSprite()->getSrcRect(), &b->getSprite()->getSrcRect(), sizeof(RECT)) == 0);
            assert(std::strcmp(a->getSprite()->getTexture()->getFileName(), b->getSprite()->getTexture()->getFileName()) == 0);

            // Groups come back flattened, so compare their members.
            const std::vector<const Collidable*> aLeaves = leaves(a->getCollidable());
            const std::vector<const Collidable*> bLeaves = leaves(b->getCollidable());
            assert(aLeaves.size() == bLeaves.size());
            for (size_t k = 0; k < aLeaves.size(); ++k) {
                assert(aLeaves[k]->getType() == bLeaves[k]->getType());
                assert(near(aLeaves[k]->getPosition(), bLeaves[k]->getPosition()));
                if (aLeaves[k]->getType() == COL_OBJ_SQUARE) {
                    const Square* sa = (const Square*)aLeaves[k];
                    const Square* sb = (const Square*)bLeaves[k];
                    assert(near(sa->getWidth(), sb->getWidth()) && near(sa->getHeight(), sb->getHeight()));
                }
                else {
                    assert(((const PolygonCollider*)aLeaves[k])->getLocalVertices() == ((const PolygonCollider*)bLeaves[k])->getLocalVertices());
                }
            }
        }
    }

    destroy(reloaded);
    destroy(loaded);
    std::filesystem::remove_all(dir);

    std::printf("animation_frame_data_test passed\n");
    return 0;
}
