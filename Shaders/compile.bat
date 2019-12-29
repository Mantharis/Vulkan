cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shader.vert -o vert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" defferedShader1stPass.frag -o defferedShader1stPass.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shader2.frag -o frag2.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" ssShader.vert -o ssShaderVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" ssShader.frag -o ssShaderFrag.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shadowShader.vert -o shadowShaderVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" shadowShader.frag -o shadowShaderFrag.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" defferedShader2ndPass.vert -o defferedShader2ndPassVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" defferedShader2ndPass.frag -o defferedShader2ndPassFrag.spv


cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" particleShader.vert -o particleShaderVert.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" particleShader.frag -o particleShaderFrag.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" particleShader.geom -o particleShaderGeom.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" particle.comp -o particleCompute.spv

cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" rayTracer.comp -o rayTracerCompute.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" rayTracer.frag -o rayTracerFrag.spv
cmd /C ""C:/SDK programming/VulkanSDK/1.1.121.2/Bin32/glslc.exe"" rayTracer.vert -o rayTracerVert.spv

pause