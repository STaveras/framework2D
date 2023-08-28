// System.h
// Information regarding the underlying system -- e.g. operating system, CPU, GPU, etc.

#pragma once

#include "Types.h"

namespace System
{
   // When we implement a in-game developer console (for issuing commands), \
   // the command line arguments for the executable should eventually use the same parser as the console, \
   // to directly get and set the same variables
    static const char* checkArgumentsForDataPath(int argc, char** argv) 
    {
        if (argc > 2) {

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--dataPath") || !strcmp(argv[i], "-d")) {
                    return argv[i + 1]; // What if it's empty?
                }
            }
        }
        return DEFAULT_DATA_PATH;
    }

    static bool checkArgumentsForDebugMode(int argc, char** argv) 
    {
        if (argc > 1) {

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--debug") || !strcmp(argv[i], "-D")) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool checkArgumentsForFullscreen(int argc, char** argv) 
    {
        if (argc > 1) {

           // TODO: Check if the second argument is true or false as to force on or off

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-F")) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool checkArgumentsForFPSCounter(int argc, char** argv)
    {
       if (argc > 1) {

          for (int i = 0; i < argc; i++) {
             if (!strcmp(argv[i], "--fps") || !strcmp(argv[i], "-f")) {
                return true;
             }
          }
       }
       return false;
    }

    static bool checkArgumentsForWindowed(int argc, char** argv) 
    {
        if (argc > 1) {

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--windowed") || !strcmp(argv[i], "-w")) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool checkArgumentsForVSync(int argc, char** argv) // also check for a true or false
    {
        if (argc > 1) {

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--vsync") || !strcmp(argv[i], "-v")) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool checkArgumentsForVulkan(int argc, char** argv) 
    {
        if (argc > 1) {

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--vulkan") || !strcmp(argv[i], "-vk") || !strcmp(argv[i], "-V")) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool checkArgumentsForOpenGL(int argc, char** argv) 
    {
        if (argc > 1) {

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--opengl") || !strcmp(argv[i], "-gl")) {
                    return true;
                }
            }
        }
        return false;
    }

    // Get the current operating system.
    const char* GetOperatingSystem();
    
    // Get the current CPU architecture.
    const char* GetCPUArchitecture();
    
    // Get the current GPU architecture.
    const char* GetGPUArchitecture();
    
    // Get the current CPU vendor.
    const char* GetCPUVendor();
    
    // Get the current CPU model.
    const char* GetCPUModel();
    
    // Get the current CPU clock speed.
    float GetCPUClockSpeed();
    
    // Get the current CPU cache size.
    uint32_t GetCPUCacheSize();
    
    // Get the current CPU number of cores.
    uint32_t GetCPUNumberOfCores();
    
    // Get the current CPU number of threads.
    uint32_t GetCPUNumberOfThreads();
    
    // Get the current CPU number of logical processors.
    uint32_t GetCPUNumberOfLogicalProcessors();
    
    // Get the current CPU number of physical processors.
    uint32_t GetCPUNumberOfPhysicalProcessors();
    
    // Get the current GPU vendor.
    const char* GetGPUVendor();
    
    // Get the current GPU model.
    const char* GetGPUModel();
    
    // Get the current GPU clock speed.
    float GetGPUClockSpeed();
    
    // Get the current GPU memory size.
    uint32_t GetGPUMemorySize();
    
    // Get the current GPU number of cores.
    uint32_t GetGPUNumberOfCores();
    
    // Get the current GPU number of threads.
    uint32_t GetGPUNumberOfThreads();
    
    // Get the current GPU number of logical processors.
    uint32_t GetGPUNumberOfLogicalProcessors();
    
    // Get the current GPU number of physical processors.
    uint32_t GetGPUNumberOfPhysicalProcessors();
    
    // Get the current GPU number of video memory.
    uint32_t GetGPUNumberOfVideoMemory();

    // Additionally we might want to store the environment information like the current working directory, the current user, etc.
    static const char* GlobalDataPath(const char *dataPath = NULL)
    {
        static std::string path = (dataPath && strcmp(dataPath, "")) ? dataPath : DEFAULT_DATA_PATH;
        return path.c_str();
    }
}