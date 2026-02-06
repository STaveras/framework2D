#pragma once

#include <iostream>

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG 1

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
    static bool dbgCollision = false;
}

#ifndef _WIN32
#define OutputDebugString(m) 
#endif

#ifdef DEBUG_MSG
#undef DEBUG_MSG
#endif
#define DEBUG_MSG(msg) do { if (Debug::Mode.isEnabled()) { std::cout << msg; OutputDebugString(msg); } } while(0)

#define DEBUGGING (Debug::Mode.isEnabled())

#include "DebugLogger.hpp"

#endif
