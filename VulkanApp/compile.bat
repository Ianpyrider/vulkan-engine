cd /D "%~dp0"

C:/VulkanSDK/1.4.321.1/bin/slangc.exe Shaders/shader.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o slang.spv
C:/VulkanSDK/1.4.321.1/bin/slangc.exe Shaders/n64_compute.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry compMain -o compute.spv