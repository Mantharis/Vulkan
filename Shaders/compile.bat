cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shader.vert -o vert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" defferedShader1stPass.frag -o defferedShader1stPass.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shader2.frag -o frag2.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" ssShader.vert -o ssShaderVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" ssShader.frag -o ssShaderFrag.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shadowShader.vert -o shadowShaderVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shadowShader.frag -o shadowShaderFrag.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" defferedShader2ndPass.vert -o defferedShader2ndPassVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" defferedShader2ndPass.frag -o defferedShader2ndPassFrag.spv

pause