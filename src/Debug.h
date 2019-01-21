#pragma once

//There needs to a boundary between your work and your personal life.
//Don't put your 'self' in your work.
//Put work in your 'self'.

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
    }Mode;

    static bool dbgMouse = false;
    static bool dbgObjects = false;
}

#define DEBUGGING (Debug::Mode.isEnabled())

#include "debugLogger.hpp"