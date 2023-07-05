#include "AnimationUtils.h"

#include "Animation.h"
#include "AnimationManager.h"

#include "FileSystem.h"

#include <string>

using namespace tinyxml2;

namespace Animations {

   RECT rectFromString(const char *rectDescription)
   {
      RECT output;

      char *tmp = _strdup(rectDescription);

      // tokenize
      const char* seps = "{,}";
      char* first, * second, * value;
      first = strtok_s(tmp, seps, &second);

      while (first != NULL) {

         char *var = strtok_s(first, "=", &value);

         if (!strcmp(var, "X"))
            output.left = atoi(value);
         else if (!strcmp(var, "Y"))
            output.top = atoi(value);
         else if (!strcmp(var, "Width"))
            output.right = atoi(value);
         else if (!strcmp(var, "Height"))
            output.bottom = atoi(value);

         first = strtok_s(second, seps, &second);
      }

      free(tmp);

      return output;
   }

   std::string rectToString(const RECT& rect)
   {
      return std::to_string(rect.left) + "," + std::to_string(rect.top) + "," + std::to_string(rect.right) + "," + std::to_string(rect.bottom);
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

   Animation* fromXMLElement(XMLElement *element, Animation *animation = NULL) {

      animation->setName(element->Attribute("Name"));
      animation->setMode(animationModeFromString(element->Attribute("PlayMode")));
      animation->setIsForward(strcmp("False", element->Attribute("Forward")));
      animation->setSpeed(element->FloatAttribute("Speed"));

      return animation;
   }

   std::vector<Animation*> fromXML(const char *filename, AnimationManager *manager)
   {
      std::vector<Animation*> animations;

      tinyxml2::XMLDocument doc;

      if (XMLError::XML_SUCCESS == doc.LoadFile(filename)) {

         tinyxml2::XMLElement* element = doc.FirstChildElement("AnimationFile");

         if (element) {

            element = element->FirstChildElement("Animation");

            do {
               
               Animation *animation = Animations::fromXMLElement(element, (!manager) ? new Animation() : manager->CreateAnimation(""));

               tinyxml2::XMLElement *frameElement = element->FirstChildElement("Frame");

               do {

                  RECT srcRect = rectFromString(frameElement->FirstChildElement("DisplayRect")->GetText());

                  // TODO: Make key color configurable via a tool
                  // TODO: Collision information
                  // TODO: Add support for triggers (sound, effects, scripts, etc.)

                  const char* frameImagePath = frameElement->FirstChildElement("Filename")->GetText();

                  animation->addFrame(new Frame(new Sprite(frameImagePath, 0xFFFF00FF, srcRect), frameElement->FloatAttribute("Duration")));

               } while (frameElement = frameElement->NextSiblingElement("Frame"));

               animations.push_back(animation);

            } while (element = element->NextSiblingElement("Animation"));
         }
      } 

      return animations;
   }

   std::vector<Animation*> fromJSON(const char* filename, AnimationManager* animationManager)
   {
      std::vector<Animation*> animations;

      simdjson::dom::parser parser;
      simdjson::dom::element document = parser.load(filename);

      for (simdjson::dom::element animationElement : document["AnimationFile"]["Animation"]) {

         Animation* animation = (!animationManager) ? new Animation() : animationManager->CreateAnimation("");

         for (simdjson::dom::element frameElement : animationElement["Frame"]) {

            std::string_view displayRectString = frameElement["DisplayRect"].get_string();

            RECT srcRect = rectFromString(std::string(displayRectString).c_str());

            // TODO: Make key color configurable via a tool
            // TODO: Collision information
            // TODO: Add support for triggers (sound, effects, scripts, etc.)

            std::string_view frameImagePath = frameElement["Filename"].get_string();

            animation->addFrame(new Frame(new Sprite(std::string(frameImagePath).c_str(), 0xFFFF00FF, srcRect), (float)frameElement["Duration"].get_double()));
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

      for (int i = 0; i < animations.size(); ++i) {
         Animation* animation = animations[i];
         file << "      {\n";
         file << "        \"Name\": \"" << animation->getName() << "\",\n";
         file << "        \"Mode\": \"" << animationModeToString(animation->getMode()) << "\",\n";
         file << "        \"Forward\": " << std::boolalpha << animation->isForward() << ",\n";
         file << "        \"Speed\": " << animation->getSpeed() << ",\n";
         file << "        \"Frame\": [\n";

         for (int j = 0; j < animation->getFrameCount(); ++j) {
            Frame* frame = (*animation)[j];
            file << "          {\n";
            file << "            \"DisplayRect\": \"" << rectToString(frame->getSprite()->getSrcRect()) << "\",\n";
            file << "            \"Filename\": \"" << frame->getSprite()->getTexture()->getFilename() << "\",\n";
            file << "            \"Duration\": " << frame->getDuration() << "\n";
            file << "          }";

            if (j != animation->getFrameCount() - 1) {
               file << ",";
            }
            file << "\n";
         }

         file << "        ]\n";
         file << "      }";

         if (i != animations.size() - 1) {
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