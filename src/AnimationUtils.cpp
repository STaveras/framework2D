#include "AnimationUtils.h"

#include "Animation.h"
#include "AnimationManager.h"

#include "FileSystem.h"
#include "StrUtils.h"
#include "System.h"

#include <string>

using namespace tinyxml2;

namespace Animations {

   RECT rectFromString(const char *rectDescription)
   {
      RECT output{};

      if (!rectDescription || rectDescription[0] == '\0') {
         return output;
      }

      std::string input(rectDescription);
      for (char& c : input) {
         if (c == '{' || c == '}') {
            c = ' ';
         }
      }

      int width = -1;
      int height = -1;
      int numericValues[4]{ 0, 0, 0, 0 };
      int numericCount = 0;
      bool usedKeyValues = false;

      size_t start = 0;
      while (start < input.size()) {
         size_t end = input.find(',', start);
         if (end == std::string::npos) {
            end = input.size();
         }

         std::string token = StrUtils::Trim(std::string_view(input).substr(start, end - start));
         if (!token.empty()) {
            size_t eq = token.find('=');
            if (eq != std::string::npos) {
               usedKeyValues = true;
               std::string key = StrUtils::Trim(std::string_view(token).substr(0, eq));
               std::string value = StrUtils::Trim(std::string_view(token).substr(eq + 1));
               int parsedValue = std::atoi(value.c_str());

               if (key == "X") {
                  output.left = parsedValue;
               }
               else if (key == "Y") {
                  output.top = parsedValue;
               }
               else if (key == "Width") {
                  width = parsedValue;
               }
               else if (key == "Height") {
                  height = parsedValue;
               }
            }
            else if (numericCount < 4) {
               numericValues[numericCount++] = std::atoi(token.c_str());
            }
         }

         start = end + 1;
      }

      if (!usedKeyValues && numericCount >= 4) {
         output.left = numericValues[0];
         output.top = numericValues[1];
         width = numericValues[2];
         height = numericValues[3];
      }

      if (width >= 0) {
         output.right = output.left + width;
      }
      if (height >= 0) {
         output.bottom = output.top + height;
      }

      return output;
   }

   //std::string rectToString(const RECT& rect)
   //{
   //   return std::to_string(rect.left) + "," + std::to_string(rect.top) + "," + std::to_string(rect.right) + "," + std::to_string(rect.bottom);
   //}
   static std::string rectToString(const RECT& r) {
      int w = r.right - r.left;
      int h = r.bottom - r.top;
      return std::to_string(r.left) + "," +
             std::to_string(r.top) + "," +
             std::to_string(w) + "," +
             std::to_string(h);
   }

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
      for (unsigned int i = 0; i < animation->getFrameCount(); i++) {
         Frame *frame = (*animation)[i];
         delete frame->getSprite();
         animation->removeFrame(frame);
         delete frame;
      }
      delete animation;
   }

   std::vector<Animation*> fromJSON(const char* filename, AnimationManager* animationManager)
   {
      std::vector<Animation*> animations;

      simdjson::dom::parser parser;
      simdjson::dom::element document = parser.load(filename);

      for (simdjson::dom::element animationElement : document["AnimationFile"]["Animation"]) {

         std::string name;
         auto nameElement = animationElement["Name"];
         if (nameElement.is_string()) {
            std::string_view nameView = nameElement.get_string().value_unsafe();
            name = std::string(nameView);
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

         auto speedElement = animationElement["Speed"];
         if (speedElement.is_double()) {
            animation->setSpeed((float)speedElement.get_double().value_unsafe());
         }

         for (simdjson::dom::element frameElement : animationElement["Frame"]) {

            std::string_view displayRectView = frameElement["DisplayRect"].get_string().value_unsafe();
            std::string displayRectString(displayRectView);
            RECT srcRect = rectFromString(displayRectString.c_str());

            // TODO: Make key color configurable via a tool
            // TODO: Collision information
            // TODO: Add support for triggers (sound, effects, scripts, etc.)

            std::string_view frameImageView = frameElement["Filename"].get_string().value_unsafe();
            std::string frameImagePath(frameImageView);
            std::string resolvedFramePath = FileSystem::Path::ResolveFromBaseOrParent(frameImagePath, System::GlobalDataPath());

            animation->addFrame(new Frame(new Sprite(resolvedFramePath.c_str(), 0xFFFF00FF, srcRect), (float)frameElement["Duration"].get_double().value_unsafe()));
         }

         animations.push_back(animation);
      }

      return animations;
   }

   void toJSON(const std::vector<Animation*>& animations, const char* filename) {
      std::ofstream file(filename);
      file << "{\n";
      file << "  \"AnimationFile\": {\n";
      file << "    \"Animation\": [\n";

      for (size_t i = 0; i < animations.size(); ++i) {
         Animation* animation = animations[i];
         file << "      {\n";
         file << "        \"Name\": \"" << animation->getName() << "\",\n";
         file << "        \"Mode\": \"" << animationModeToString(animation->getMode()) << "\",\n";
         file << "        \"Forward\": " << std::boolalpha << animation->isForward() << ",\n";
         file << "        \"Speed\": " << animation->getSpeed() << ",\n";
         file << "        \"Frame\": [\n";

         for (size_t j = 0; j < animation->getFrameCount(); ++j) {
            Frame* frame = (*animation)[(unsigned int)j];
            file << "          {\n";
            file << "            \"DisplayRect\": \"" << rectToString(frame->getSprite()->getSrcRect()) << "\",\n";
            file << "            \"Filename\": \"" << FileSystem::Path::MakeRelativeToParent(frame->getSprite()->getTexture()->getFilename(), System::GlobalDataPath()) << "\",\n";
            file << "            \"Duration\": " << frame->getDuration() << "\n";
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
