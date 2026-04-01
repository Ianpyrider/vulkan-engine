cmake_minimum_required(VERSION 3.20)
project(N64Engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_executable(VulkanApp VulkanApp/src/main.cpp)

target_include_directories(VulkanApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Lib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanApp/include
)

target_sources(VulkanApp PRIVATE
    # CORE
    VulkanApp/include/core/SwapChainManager.h
    VulkanApp/src/core/SwapChainManager.cpp
    VulkanApp/include/core/VulkanApplication.h
    VulkanApp/src/core/VulkanApplication.cpp
    VulkanApp/include/core/VulkanContext.h
    VulkanApp/src/core/VulkanContext.cpp

    # RENDERER
    "VulkanApp/include/renderer/ImageComputePipeline.h"
    "VulkanApp/src/renderer/ImageComputePipeline.cpp"
    VulkanApp/include/renderer/GraphicsPipeline.h
    VulkanApp/src/renderer/GraphicsPipeline.cpp
    VulkanApp/include/renderer/Vertex.h
    VulkanApp/src/renderer/Vertex.cpp
    VulkanApp/include/renderer/Mesh.h
    VulkanApp/src/renderer/Mesh.cpp

    VulkanApp/include/renderer/Renderer.h
    VulkanApp/src/renderer/Renderer.cpp

    # SHARED
    VulkanApp/include/shared/FileUtils.h
    VulkanApp/src/shared/FileUtils.cpp
    VulkanApp/include/shared/VulkanUtils.h
    VulkanApp/src/shared/VulkanUtils.cpp

    VulkanApp/include/shared/Types.h
    VulkanApp/include/shared/VulkanInclude.h
    VulkanApp/include/shared/EngineConfig.h

    VulkanApp/src/shared/VmaUsage.cpp
    VulkanApp/src/shared/tiny_obj_loader.cpp
    "VulkanApp/include/renderer/ParticleComputePipeline.h" 
    "VulkanApp/src/renderer/ParticleComputePipeline.cpp"
)

target_link_directories(VulkanApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Lib/lib)

target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan
    glfw3
)

file(GLOB_RECURSE SHADER_FILES 
    "VulkanApp/shaders/*.slang" 
)

add_custom_target(CompileShaders ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/VulkanApp/compile.bat $<TARGET_FILE_DIR:VulkanApp>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/VulkanApp
    SOURCES ${SHADER_FILES}
    COMMENT "Compiling Slang shaders directly into the build directory..."
)

add_dependencies(VulkanApp CompileShaders)

add_custom_command(TARGET VulkanApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:VulkanApp>/assets"
    COMMENT "Copying assets directory to build output..."
)