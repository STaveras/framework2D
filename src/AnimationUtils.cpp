#include "AnimationUtils.h"

#include "Animation.h"
#include "AnimationManager.h"

#include "FileSystem.h"

#include <string>
#include <tinyxml2.h>

#pragma comment(lib, "tinyxml2.lib")

using namespace tinyxml2;

namespace Animations {

   RECT rectFromString(const char *rectDescription)
   {
      RECT output;

      char *tmp = _strdup(rectDescription);

      // tokenize
      char *first, *second, *value, *seps = "{,}";
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

   Animation::Mode animationModeFromString(const char *mode) {

      if (!strcmp(mode, "LOOPING"))
         return Animation::Mode::eLoop;
      else if (!strcmp(mode, "OSCILLATE"))
         return Animation::Mode::eOscillate;

      return Animation::Mode::eOnce;
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
         delete frame->GetSprite();
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

   std::vector<Animation*> fromDirectory(const char* directoryPath)
   {
      std::vector<Animation*> animations;

      // FileSystem namespace function test
      // std::vector<std::string> files = FileSystem::GetFiles(directoryPath, "*.xml");
      std::vector<std::string> files = FileSystem::ListFiles(directoryPath);
      // TODO: Go through a directory, and use the structure of the subdirectories to import animations and their associated images

      return animations;
   }

   // Only works when each Frame have the same dimensions
   void createFramesForAnimation(Animation* animation, Texture* spriteSheet, vector2 frameDimensions, Factory<Sprite>& spriteFactory)
   {
      vector2 frameCounts = { spriteSheet->getWidth() / frameDimensions.x, spriteSheet->getHeight() / frameDimensions.y };

      for (int x = 0; x < frameCounts.x; x++) {
         for (int y = 0; y < frameCounts.y; y++) {
            animation->createFrame(spriteFactory.Create(Sprite(spriteSheet, { x * (long)frameDimensions.x, y * (long)frameDimensions.y,
                                                                             (x + 1) * (long)frameDimensions.x, (y + 1) * (long)frameDimensions.y })));
         }
      }
   }
}