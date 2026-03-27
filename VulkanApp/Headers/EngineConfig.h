#pragma once
#include <cstdint>

namespace engineConfig {
	inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	inline constexpr uint32_t TIMESTAMPS_PER_FRAME = 2;
	inline constexpr uint32_t NUM_TIMESTAMPS = MAX_FRAMES_IN_FLIGHT * TIMESTAMPS_PER_FRAME;
	inline constexpr bool PRINT_GPU_PROFILING = true;
	inline constexpr const char* SHADER_PATH = "slang.spv";
	inline constexpr const char* COMPUTE_SHADER_PATH = "compute.spv";
}