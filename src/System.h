// System.h
// Information regarding the underlying system -- e.g. operating system, CPU, GPU, etc.

#include "Types.h"

#pragma once

namespace System
{
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