#pragma once
#include <cstdint>

namespace EngineConfig {
	inline constexpr const char* SHADER_PATH = "slang.spv";
	inline constexpr const char* PARTICLE_SHADER_PATH = "pcl_slang.spv";
	inline constexpr const char* IMAGE_COMPUTE_SHADER_PATH = "img_compute.spv";
	inline constexpr const char* PARTICLE_COMPUTE_SHADER_PATH = "pcl_compute.spv";
	
	inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	inline constexpr uint32_t TIMESTAMPS_PER_FRAME = 2;
	inline constexpr uint32_t NUM_TIMESTAMPS = MAX_FRAMES_IN_FLIGHT * TIMESTAMPS_PER_FRAME;

	inline constexpr bool PRINT_GPU_PROFILING = true;
	inline constexpr bool ENABLE_MAILBOX_PRESENT = false;

	inline constexpr int PARTICLE_COUNT = 1024;
}