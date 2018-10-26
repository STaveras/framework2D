#include "AnimationUtils.h"

#include "Animation.h"
#include "AnimationManager.h"

#include <tinyxml2/tinyxml2.h>
#ifdef _DEBUG
#pragma comment(lib, "tinyxml2_d.lib")
#else
#pragma comment(lib, "tinyxml2.lib")
#endif

#include <string>

using namespace tinyxml2;

namespace AnimationUtils {

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

   std::vector<Animation*> loadAnimationsFromXML(const char *filename, AnimationManager *manager)
   {
      std::vector<Animation*> animations;

      tinyxml2::XMLDocument doc;

      if (XMLError::XML_SUCCESS == doc.LoadFile(filename)) {

         tinyxml2::XMLElement* element = doc.FirstChildElement("AnimationFile");

         if (element) {

            element = element->FirstChildElement("Animation");

            do {

               Animation *animation = new Animation();

               animation->SetName(element->Attribute("Name"));
               animation->SetMode(animationModeFromString(element->Attribute("PlayMode")));
               animation->SetIsForward(strcmp("False", element->Attribute("Forward")));
               animation->SetSpeed(element->FloatAttribute("Speed"));

               tinyxml2::XMLElement *frameElement = element->FirstChildElement("Frame");

               do {

                  float duration = frameElement->FloatAttribute("Duration");

                  RECT srcRect = rectFromString(frameElement->FirstChildElement("DisplayRect")->GetText());

                  // TODO: Make key color configurable via the tool
                  Frame *frame = new Frame(new Sprite(frameElement->FirstChildElement("Filename")->GetText(),
                                                      0xFFFF00FF,
                                                      &srcRect),
                                           duration);

                  // TODO: Collision information
                  // TODO: Add support for triggers (sound, effects, scripts, etc.)

                  animation->AddFrame(frame);

               } while (frameElement = frameElement->NextSiblingElement("Frame"));

               animations.push_back(animation);

            } while (element = element->NextSiblingElement("Animation"));
         }
      }

      return animations;
   }
}