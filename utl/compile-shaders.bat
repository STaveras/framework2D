if not exist "%cd%\bin\fantasySideScroller\cache\shader" mkdir "%cd%\bin\fantasySideScroller\cache\shader"
"%VK_SDK_PATH%\Bin\glslc" "%cd%\bin\data\shaders\triangle.vert" -o "%cd%\bin\fantasySideScroller\cache\shader\tri.v.spv"
"%VK_SDK_PATH%\Bin\glslc" "%cd%\bin\data\shaders\triangle.frag" -o "%cd%\bin\fantasySideScroller\cache\shader\tri.f.spv"
