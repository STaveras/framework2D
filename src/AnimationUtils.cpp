#include "AnimationUtils.h"

#include "Animation.h"
#include "AnimationManager.h"

#include "CollidableGroup.h"
#include "Debug.h"
#include "FileSystem.h"
#include "Polygon.h"
#include "Square.h"
#include "Scripting/JSONReader.h"
#include "StrUtils.h"
#include "System.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using Scripting::JSON::readFloat;
using Scripting::JSON::readString;
using StrUtils::rectFromString;
using StrUtils::rectToString;
using StrUtils::vectorFromString;
using StrUtils::vectorToString;

namespace Animations {

   Animation::Mode animationModeFromString(const char *mode) {

      if (!strcmp(mode, "LOOPING"))
         return Animation::Mode::eLoop;
      else if (!strcmp(mode, "OSCILLATE"))
         return Animation::Mode::eOscillate;

      return Animation::Mode::eOnce;
   }

   std::string animationModeToString(Animation::Mode mode) {
      switch (mode) {
      case Animation::eLoop:
         return "LOOPING";
      case Animation::eOscillate:
         return "OSCILLATE";
      case Animation::eOnce:
         return "ONCE";
      }
      return "ONCE";
   }

   void addToRenderList(std::vector<Animation*>& animations, IRenderer::RenderList *renderList)
   {
      std::vector<Animation *>::iterator itr = animations.begin();

      while (itr != animations.end()) {
         renderList->push_back((Renderable*)(*itr)); itr++;
      }
   }

   void destroyAnimation(Animation *animation) {
      // The animation owns and deletes its frames; their sprites are the caller's.
      for (Frame* frame : animation->getFrames()) {
         delete frame->getSprite();
      }
      delete animation;
   }

   // An animation with neither its own "Frame" list nor a "FrameData" file reads its frames
   // from this file, in the folder of the animation's "Filename".
   static const char* kFrameDataFileName = "frameData.json";

   std::vector<Animation*> fromJSON(const char* filename, AnimationManager* animationManager)
   {
      std::vector<Animation*> animations;

      simdjson::dom::parser parser;
      simdjson::dom::element document = parser.load(filename);

      // Holds the frameData.json of the animation being read; reused for the next one.
      simdjson::dom::parser frameDataParser;

      for (simdjson::dom::element animationElement : document["AnimationFile"]["Animation"]) {

         std::string name = readString(animationElement["Name"]);

         // The animation's image, used by every frame that doesn't name its own.
         std::string imagePath = readString(animationElement["Filename"]);
         std::string resolvedImagePath = imagePath.empty() ? "" : FileSystem::Path::ResolveFromBaseOrParent(imagePath, System::GlobalDataPath());

         // Frames: the animation's own "Frame" list, else the file its "FrameData" names, else
         // frameData.json beside its "Filename". A named file that can't be read is not
         // replaced by the one beside the image.
         simdjson::dom::array frameElements;
         if (animationElement["Frame"].is_array()) {
            frameElements = animationElement["Frame"].get_array().value_unsafe();
         }
         else {
            const std::string frameDataName = readString(animationElement["FrameData"]);
            std::string frameDataPath;
            if (!frameDataName.empty()) {
               frameDataPath = FileSystem::Path::ResolveFromBaseOrParent(frameDataName, System::GlobalDataPath());
            }
            else if (!resolvedImagePath.empty()) {
               frameDataPath = FileSystem::File::GetFilePath(resolvedImagePath) + "/" + kFrameDataFileName;
            }

            simdjson::dom::element frameData;
            if (frameDataPath.empty() || frameDataParser.load(frameDataPath).get(frameData) || !frameData["Frame"].is_array()) {
#if _DEBUG
               char buffer[768];
               sprintf_s(buffer, sizeof(buffer), "Animations::fromJSON skipping animation '%s': no \"Frame\" list, and no frames read from '%s'\n",
                  name.c_str(), frameDataPath.empty() ? "(no \"FrameData\" or \"Filename\")" : frameDataPath.c_str());
               DEBUG_MSG(buffer);
#endif
               continue;
            }
            frameElements = frameData["Frame"].get_array().value_unsafe();
         }

         Animation* animation = nullptr;
         if (animationManager) {
            animation = animationManager->CreateAnimation(name.empty() ? "" : name.c_str());
         }
         else {
            animation = name.empty() ? new Animation() : new Animation(name.c_str());
         }

         if (!name.empty()) {
            animation->setName(name.c_str());
         }

         auto modeElement = animationElement["Mode"];
         if (modeElement.is_string()) {
            std::string_view modeView = modeElement.get_string().value_unsafe();
            std::string modeValue(modeView);
            animation->setMode(animationModeFromString(modeValue.c_str()));
         }

         auto forwardElement = animationElement["Forward"];
         if (forwardElement.is_bool()) {
            animation->setIsForward(forwardElement.get_bool().value_unsafe());
         }

         animation->setSpeed(readFloat(animationElement["Speed"], animation->getSpeed()));

         const vector2 offset = vectorFromString(readString(animationElement["Offset"]));
         animation->setOffset(offset);

         unsigned int frameIndex = 0;
         for (simdjson::dom::element frameElement : frameElements) {
            const unsigned int index = frameIndex++;

            // Without a DisplayRect the frame is the whole image.
            const RECT srcRect = rectFromString(readString(frameElement["DisplayRect"]), RECT{ -1, -1, -1, -1 });

            // TODO: Make key color configurable via a tool
            // TODO: Add support for triggers (sound, effects, scripts, etc.)

            const std::string frameImagePath = readString(frameElement["Filename"]);
            const std::string resolvedFramePath = frameImagePath.empty() ? resolvedImagePath : FileSystem::Path::ResolveFromBaseOrParent(frameImagePath, System::GlobalDataPath());
            if (resolvedFramePath.empty()) {
#if _DEBUG
               char buffer[512];
               sprintf_s(buffer, sizeof(buffer), "Animations::fromJSON skipping frame %u of '%s': neither it nor its animation has a \"Filename\"\n",
                  index, name.c_str());
               DEBUG_MSG(buffer);
#endif
               continue;
            }

            Frame* frame = new Frame(new Sprite(resolvedFramePath.c_str(), 0xFFFF00FF, srcRect), readFloat(frameElement["Duration"]));
            frame->getSprite()->center();

            // Frame collision is stored the way Tiled stores a tile's: an "objectgroup" in the
            // frame's pixels. The frame is drawn around its centre, shifted by the animation's
            // offset, so that is where its colliders go too.
            if (frameElement["objectgroup"].is_object()) {
               const std::string source = "animation=" + name + ", frame=" + std::to_string(index);
               frame->setCollidable(Collidable::loadFromObjectGroup(frameElement["objectgroup"].value_unsafe(), frame->getCollisionObjects(),
                  offset - frame->getSprite()->getCenter(), source.c_str()));
            }

            animation->addFrame(frame);
         }

         animations.push_back(animation);
      }

      return animations;
   }

   // Appends Tiled "objectgroup" objects for collidable, moved by -origin into frame pixels.
   static void collisionToObjects(const Collidable* collidable, vector2 origin, std::vector<std::string>& objects)
   {
      if (!collidable) {
         return;
      }

      const std::string indent = "                  ";
      const vector2 position = collidable->getPosition() - origin;
      std::ostringstream object;

      switch (collidable->getType()) {
      case COL_OBJ_SQUARE: {
         const Square* square = (const Square*)collidable;
         object << indent << "{\n"
                << indent << "  \"height\": " << square->getHeight() << ",\n"
                << indent << "  \"id\": " << objects.size() + 1 << ",\n"
                << indent << "  \"name\": \"\",\n"
                << indent << "  \"rotation\": 0,\n"
                << indent << "  \"type\": \"\",\n"
                << indent << "  \"visible\": true,\n"
                << indent << "  \"width\": " << square->getWidth() << ",\n"
                << indent << "  \"x\": " << position.x << ",\n"
                << indent << "  \"y\": " << position.y << "\n"
                << indent << "}";
         break;
      }
      case COL_OBJ_POLYGON: {
         const PolygonCollider* polygon = (const PolygonCollider*)collidable;
         object << indent << "{\n"
                << indent << "  \"height\": 0,\n"
                << indent << "  \"id\": " << objects.size() + 1 << ",\n"
                << indent << "  \"name\": \"\",\n"
                << indent << "  \"polygon\": [";
         const std::vector<vector2>& vertices = polygon->getLocalVertices();
         for (size_t i = 0; i < vertices.size(); ++i) {
            object << (i ? ", " : " ") << "{ \"x\": " << vertices[i].x << ", \"y\": " << vertices[i].y << " }";
         }
         object << " ],\n"
                << indent << "  \"rotation\": 0,\n"
                << indent << "  \"type\": \"\",\n"
                << indent << "  \"visible\": true,\n"
                << indent << "  \"width\": 0,\n"
                << indent << "  \"x\": " << position.x << ",\n"
                << indent << "  \"y\": " << position.y << "\n"
                << indent << "}";
         break;
      }
      case COL_OBJ_GROUP:
         // Members are placed relative to their group.
         for (const Collidable* member : *(const CollidableGroup*)collidable) {
            collisionToObjects(member, origin - collidable->getPosition(), objects);
         }
         return;
      default:
         // Circles and planes have no Tiled object the loader reads back.
         return;
      }

      objects.push_back(object.str());
   }

   void toJSON(const std::vector<Animation*>& animations, const char* filename) {
      std::ofstream file(filename);
      file << "{\n";
      file << "  \"AnimationFile\": {\n";
      file << "    \"Animation\": [\n";

      for (size_t i = 0; i < animations.size(); ++i) {
         Animation* animation = animations[i];

         // One image for the whole animation is written once, on the animation.
         std::string sharedImage;
         for (size_t j = 0; j < animation->getFrameCount(); ++j) {
            std::string image = FileSystem::Path::MakeRelativeToParent((*animation)[(unsigned int)j]->getSprite()->getTexture()->getFileName(), System::GlobalDataPath());
            if (j == 0) {
               sharedImage = image;
            }
            else if (image != sharedImage) {
               sharedImage.clear();
               break;
            }
         }

         file << "      {\n";
         file << "        \"Name\": \"" << animation->getName() << "\",\n";
         if (!sharedImage.empty()) {
            file << "        \"Filename\": \"" << sharedImage << "\",\n";
         }
         file << "        \"Mode\": \"" << animationModeToString(animation->getMode()) << "\",\n";
         file << "        \"Forward\": " << std::boolalpha << animation->isForward() << ",\n";
         file << "        \"Speed\": " << animation->getSpeed() << ",\n";
         if (animation->getOffset() != vector2(0.0f, 0.0f)) {
            file << "        \"Offset\": \"" << vectorToString(animation->getOffset()) << "\",\n";
         }
         file << "        \"Frame\": [\n";

         for (size_t j = 0; j < animation->getFrameCount(); ++j) {
            Frame* frame = (*animation)[(unsigned int)j];

            std::vector<std::string> objects;
            collisionToObjects(frame->getCollidable(), animation->getOffset() - frame->getSprite()->getCenter(), objects);

            file << "          {\n";
            file << "            \"DisplayRect\": \"" << rectToString(frame->getSprite()->getSrcRect()) << "\",\n";
            if (sharedImage.empty()) {
               file << "            \"Filename\": \"" << FileSystem::Path::MakeRelativeToParent(frame->getSprite()->getTexture()->getFileName(), System::GlobalDataPath()) << "\",\n";
            }
            file << "            \"Duration\": " << frame->getDuration() << (objects.empty() ? "\n" : ",\n");
            if (!objects.empty()) {
               file << "            \"objectgroup\": {\n";
               file << "              \"draworder\": \"index\",\n";
               file << "              \"name\": \"\",\n";
               file << "              \"objects\": [\n";
               for (size_t k = 0; k < objects.size(); ++k) {
                  file << objects[k] << (k + 1 != objects.size() ? ",\n" : "\n");
               }
               file << "              ],\n";
               file << "              \"opacity\": 1,\n";
               file << "              \"type\": \"objectgroup\",\n";
               file << "              \"visible\": true,\n";
               file << "              \"x\": 0,\n";
               file << "              \"y\": 0\n";
               file << "            }\n";
            }
            file << "          }";

            if (j + 1 != animation->getFrameCount()) {
               file << ",";
            }
            file << "\n";
         }

         file << "        ]\n";
         file << "      }";

         if (i + 1 != animations.size()) {
            file << ",";
         }
         file << "\n";
      }

      file << "    ]\n";
      file << "  }\n";
      file << "}\n";

      file.close();
   }

   std::vector<Animation*> fromDirectory(const char* directoryPath)
   {
      std::vector<Animation*> animations;

      // FileSystem namespace function test
      // std::vector<std::string> files = FileSystem::GetFiles(directoryPath, "*.xml");
      std::vector<std::string> files = FileSystem::ListFiles(directoryPath);
      // TODO: Go through a directory, and use the structure of the subdirectories to import animations and their associated images

      return animations;
   }

   // Only works when each Frame has the same dimensions
   void createFramesForAnimation(Animation* animation, Texture* spriteSheet, vector2 frameDimensions, Factory<Sprite>& spriteFactory, unsigned int startIndex, unsigned int count)
   {
      if (!animation || !spriteSheet)
         return;

      vector2 frameCounts = { spriteSheet->getWidth() / frameDimensions.x,
                              spriteSheet->getHeight() / frameDimensions.y };

      if (count == 0) {
         count = (unsigned int)(frameCounts.x * frameCounts.y);
      }

      for (int y = 0; y < frameCounts.y; y++) {
         for (int x = 0; x < frameCounts.x; x++) {
            if ((unsigned int)((x + 1) * (y + 1)) < startIndex)
               continue;

            if (count-- == 0)
               break;

            animation->createFrame(spriteFactory.create(Sprite(spriteSheet, { (int)(x * frameDimensions.x), (int)(y * frameDimensions.y),
                                                                              (int)((x + 1) * frameDimensions.x), (int)((y + 1) * frameDimensions.y) })));
         }
      }
   }
}
