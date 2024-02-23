#!/bin/bash

# Specify the directory where VulkanSDK is located
vulkan_dir="$HOME/VulkanSDK"

# Check if the user provided a specific version number
if [[ $# -eq 2 && $1 == "--v" ]]; then
    version="$2"
    sdk_dir="$vulkan_dir/$version"
else
    # Find the highest version number in the VulkanSDK directory
    version=$(ls -d $vulkan_dir/* | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | sort -Vr | head -n 1)
    sdk_dir="$vulkan_dir/$version"
fi

# Check if the specified directory exists
if [[ -d "$sdk_dir" ]]; then
    # Run the setup-env.sh script
    source "$sdk_dir/setup-env.sh"
    echo "Vulkan SDK version $version is set up."
else
    echo "Error: Vulkan SDK version $version not found in $vulkan_dir."
    exit 1
fi

export DYLD_FALLBACK_LIBRARY_PATH=/usr/local/lib
