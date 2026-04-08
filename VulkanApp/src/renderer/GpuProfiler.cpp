#include "renderer/GpuProfiler.h"

#include <iostream>

GpuProfiler::GpuProfiler(float timestampPeriod) : timestampPeriod(timestampPeriod) {
	timers.emplace_back("Total", 0, 8);
	timers.emplace_back("3D Compute (Snow)", 0, 1);
	timers.emplace_back("Graphics (Meshes)", 2, 3);
	timers.emplace_back("Graphics (Snow)", 4, 5);
	timers.emplace_back("2D Compute (N64)", 6, 7);

	printf("[GPU Profiling] Profiling Enabled\n");
}

void GpuProfiler::update(const std::vector<uint64_t>& timestamps) {
	for (auto& timer : timers) {
		uint64_t deltaTicks = timestamps[timer.endIndex] - timestamps[timer.startIndex];
		timer.deltas[timer.deltaIndex] = (deltaTicks * timestampPeriod) / 1000000.0f;
		timer.deltaIndex++;
		timer.deltaIndex = timer.deltaIndex % numDeltas;
	}
}

void GpuProfiler::printResults() {
	printf("----- [GPU Profiling] Printing results -----\n");

	float trackTotal = 0.0;

	for (auto& timer : timers) {
		float average = 0.0;
		float max = 0.0;

		for (auto frameDelta : timer.deltas) {
			average += frameDelta;

			if (max < frameDelta) {
				max = frameDelta;
			}
		}

		average = average / numDeltas;

		std::cout << "[GPU Profiling] Stage: " << timer.name << ", Average: " << average << "ms, Max: " << max << "ms" << std::endl;

		trackTotal += average;
	}
}