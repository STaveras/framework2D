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

   ANIMATION_MODE animationModeFromString(const char *mode) {

      if (!strcmp(mode, "LOOPING"))
         return ANIMATION_MODE_LOOP;
      else if (!strcmp(mode, "OSCILLATE"))
         return ANIMATION_MODE_OSCILLATE;

      return ANIMATION_MODE_ONCE;
   }

   void addToRenderList(std::vector<Animation*>& animations, IRenderer::RenderList *renderList)
   {
      std::vector<Animation *>::iterator itr = animations.begin();

      while (itr != animations.end()) {
         renderList->push_back((Renderable*)(*itr)); itr++;
      }
   }

   void destroyAnimation(Animation *animation) {
      for (unsigned int i = 0; i < animation->GetFrameCount(); i++) {
         Frame *frame = (*animation)[i];
         delete frame->GetSprite();
         animation->RemoveFrame(frame);
         delete frame;
      }
      delete animation;
   }

   Animation* fromXMLElement(XMLElement *element, Animation *animation = NULL) {

      animation->SetName(element->Attribute("Name"));
      animation->SetMode(animationModeFromString(element->Attribute("PlayMode")));
      animation->SetIsForward(strcmp("False", element->Attribute("Forward")));
      animation->SetSpeed(element->FloatAttribute("Speed"));

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

                  // TODO: Make key color configurable via the tool
                  // TODO: Collision information
                  // TODO: Add support for triggers (sound, effects, scripts, etc.)

                  const char* frameImagePath = frameElement->FirstChildElement("Filename")->GetText();

                  animation->AddFrame(new Frame(new Sprite(frameImagePath, 0xFFFF00FF, &srcRect), frameElement->FloatAttribute("Duration")));

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
}