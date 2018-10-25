#include "AnimationLoader.h"

#include "Animation.h"
#include "AnimationManager.h"

#include <tinyxml2/tinyxml2.h>
#ifdef _DEBUG
#pragma comment(lib, "tinyxml2_d.lib")
#else
#pragma comment(lib, "tinyxml2.lib")
#endif

#include <iostream>

using namespace tinyxml2;

AnimationLoader::AnimationLoader()
{
}

AnimationLoader::~AnimationLoader()
{

}

std::map<std::string, Animation*> AnimationLoader::loadAnimationsFromXML(const char *filename, AnimationManager *manager)
{
   std::map<std::string, Animation*> animations;

   tinyxml2::XMLDocument doc;

   if (XMLError::XML_SUCCESS == doc.LoadFile(filename)) {

      tinyxml2::XMLElement* element = doc.FirstChildElement("AnimationFile");

      if (element) {

         element = element->FirstChildElement("Animation");

         do {

            std::string Name = element->Attribute("Name");
            std::string PlayMode = element->Attribute("PlayMode"); // Map to enum? It's ONCE, LOOPING, or OSCILLATE
            std::string Direction = element->Attribute("Forward");
            std::string Speed = element->Attribute("Speed"); // Convert to number (decimal, double, float)

            tinyxml2::XMLElement *frame = element->FirstChildElement("Frame");

            do {

               std::cout << "Frame";

            } while (frame = frame->NextSiblingElement("Frame"));

         } while (element = element->NextSiblingElement("Animation"));
      }

      std::cout << "Done\n";
   }

   return animations;
}

