@echo off

cd /D "%~dp0"

set "OUT_DIR=%~1"

:: Fallback for no cmake
if "%OUT_DIR%"=="" set "OUT_DIR=."

echo --- Compiling Slang Shaders to: %OUT_DIR% ---

"C:/VulkanSDK/1.4.321.1/bin/slangc.exe" Shaders/shader.slang ^
    -target spirv -profile spirv_1_4 -emit-spirv-directly ^
    -fvk-use-entrypoint-name -entry vertMain -entry fragMain ^
    -o "%OUT_DIR%/slang.spv"

"C:/VulkanSDK/1.4.321.1/bin/slangc.exe" Shaders/n64_compute.slang ^
    -target spirv -profile spirv_1_4 -emit-spirv-directly ^
    -fvk-use-entrypoint-name -entry compMain ^
    -o "%OUT_DIR%/compute.spv"

echo --- Shader Compilation Complete ---