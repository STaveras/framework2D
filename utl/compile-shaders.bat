if not exist "%cd%\bin\cache\shader" mkdir "%cd%\bin\cache\shader"
"%VK_SDK_PATH%\Bin\glslc" "%cd%\bin\fantasySideScroller\Shaders\triangle.vert" -o "%cd%\bin\cache\shader\tri.v.spv"
"%VK_SDK_PATH%\Bin\glslc" "%cd%\bin\fantasySideScroller\Shaders\triangle.frag" -o "%cd%\bin\cache\shader\tri.f.spv"
