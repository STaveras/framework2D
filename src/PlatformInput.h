// PlatformInput
// A platform-independent input class
#pragma once

#if !defined(PLATFORM_INPUT_H)
#define PLATFORM_INPUT_H

#include "IInput.h"
#include "Window.h"

class PlatformInput : public IInput
{
    Window *_window;

public:
    PlatformInput(Window *window);
    ~PlatformInput(void);

    void Initialize(void);
    void Update(void);
    void Shutdown(void);
};

#endif