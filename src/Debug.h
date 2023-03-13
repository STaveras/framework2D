#pragma once

#include <iostream>

#define DEBUG

namespace Debug
{
    static struct _debugging
    {
        static bool _bDebug;

        bool isEnabled(void) 
        { 
            return _bDebug;
        }

        static void enable(void) { _bDebug = true; }
        static void disable(void) { _bDebug = false; }
        
    } Mode;

    static bool dbgMouse = false;
    static bool dbgObjects = false;
    static bool dbgTiles = false;
    static bool dbgMemory = false;
    static bool dbgCollision = true;
}

#ifndef _WIN32
#define OutputDebugString(m) 
#endif

#define DEBUG_MSG(msg) std::cout << msg; OutputDebugString(msg);

#define DEBUGGING (Debug::Mode.isEnabled())

#include "debugLogger.hpp"