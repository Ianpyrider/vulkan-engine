cmake_minimum_required(VERSION 3.20)
project(N64Engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES 
    "VulkanApp/*.cpp" 
    "VulkanApp/*.h"
)

add_executable(VulkanApp ${SOURCES})

target_include_directories(VulkanApp PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Lib/include
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanApp/Headers
)

target_link_directories(VulkanApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Lib/lib)

add_custom_target(CompileShaders ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/VulkanApp/compile.bat $<TARGET_FILE_DIR:VulkanApp>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/VulkanApp
    COMMENT "Compiling Slang shaders directly into the build directory..."
)

add_dependencies(VulkanApp CompileShaders)

target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan
    glfw3
)