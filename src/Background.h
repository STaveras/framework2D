#pragma once

#include "Image.h"

namespace Background
{
   // I was gonna make this a class but I rather just make a set of utility functions for helping out with the background...
   // Not everything has to be a class, y'know...? (maybe)
//public:
    namespace {
        static enum class Mode
        {
            Still, // A static, sequential set of images
            Repeat, // Repeats >>>>>>>
            Mirror // Repeats, but mirrored ><><><><
        }mode;

        void setMode(Mode m) {
            mode = m;
        }

        Mode getMode(Mode m) {
            return mode;
        }
    };
    
//private:
//   Mode _mode;
//   std::list<Image*> _images; // should this be a factory?

//public:
//   Mode getMode(void) const { return _mode; }
//   void setMode(Mode mode) { _mode = mode; }
//
//   void addImage(Image *image) { _images.push_back(image); }
//   void removeImage(Image *image) { _images.remove(image); }
};
