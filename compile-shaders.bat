mkdir %cd%\bin\data\cache\shader
%VK_SDK_PATH%\Bin\glslc %cd%\bin\data\shaders\triangle.vert -o %cd%\bin\data\cache\shader\tri.v.spv
%VK_SDK_PATH%\Bin\glslc %cd%\bin\data\shaders\triangle.frag -o %cd%\bin\data\cache\shader\tri.f.spv
pause