cmake_minimum_required (VERSION 3.10)

set(CMAKE_SUPPRESS_REGENERATION true)
set_property(GLOBAL PROPERTY USE_FOLDERS ON) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")

project (Phoenix)

file(GLOB PHOENIX_SRC
    "${PROJECT_SOURCE_DIR}/*.h"
    "${PROJECT_SOURCE_DIR}/*.cpp"
)

source_group(PHOENIX   FILES ${PHOENIX_SRC})

file(GLOB SHADERS_SRC
	"${PROJECT_SOURCE_DIR}/../Shaders/*.vert"
	"${PROJECT_SOURCE_DIR}/../Shaders/*.frag"
	"${PROJECT_SOURCE_DIR}/../Shaders/*.bat"
)
source_group(SHADERS FILES ${SHADERS_SRC})

set( ALL_SRC
	${PHOENIX_SRC}
	${SHADERS_SRC}
)
	
add_executable(Phoenix ${ALL_SRC})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Phoenix)

set_target_properties(Phoenix PROPERTIES LINKER_LANGUAGE CXX)

set(INCLUDE_DIRS 
	"C:\\SDK programming\\VulkanSDK\\1.1.121.2\\Include" 
	"C:\\SDK programming\\glm" 
	"C:\\SDK programming\\glfw-3.3.bin.WIN64\\include"
	"C:\\SDK programming\\stb" 
	"c:\\SDK programming\\tinyobjloader\\" 
	)
	
target_include_directories(Phoenix PUBLIC ${INCLUDE_DIRS})

set(DLL_PATHS 
	"C:\\SDK programming\\glfw-3.3.bin.WIN64\\lib-vc2019\\glfw3.lib" 
	"C:\\SDK programming\\VulkanSDK\\1.1.121.2\\Lib\\vulkan-1.lib")
	
target_link_libraries(Phoenix ${DLL_PATHS})