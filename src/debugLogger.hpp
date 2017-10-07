
#pragma once

#include "logger.h"
#include "ISingleton.h"

namespace Debug
{
    static class debugLogger : public ISingleton<debugLogger>, public logger
    {
        friend ISingleton<debugLogger>;

    public:
        debugLogger(void):logger("./debug.log"){}
        ~debugLogger(void){}

        void Shutdown(void){}
    }*Log = Log->GetInstance();
}