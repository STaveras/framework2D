#pragma once

#include <iostream>

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG 1

namespace Debug
{
    struct _debugging
    {
        static bool _bDebug;

        bool isEnabled(void) 
        { 
            return _bDebug;
        }

        static void enable(void) { _bDebug = true; }
        static void disable(void) { _bDebug = false; }
        
    };

    extern _debugging Mode;

    extern bool dbgMouse;
    extern bool dbgObjects;
    extern bool dbgTiles;
    extern bool dbgMemory;
    extern bool dbgCollision;
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
