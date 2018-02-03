
#pragma once

#include "logger.h"
#include "ISingleton.h"

namespace Debug
{
    static class DebugLogger : public ISingleton<DebugLogger>, public Logger
    {
        friend ISingleton<DebugLogger>;

    public:
        DebugLogger(void):Logger("./debug.log"){}
        ~DebugLogger(void){}

        void Shutdown(void){}
    }*Log = Log->getInstance();
}