#pragma once
#include <cstdint>

namespace EngineConfig {
	inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	inline constexpr uint32_t TIMESTAMPS_PER_FRAME = 2;
	inline constexpr uint32_t NUM_TIMESTAMPS = MAX_FRAMES_IN_FLIGHT * TIMESTAMPS_PER_FRAME;

}