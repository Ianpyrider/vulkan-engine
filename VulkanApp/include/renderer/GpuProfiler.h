#pragma once

#include <vector>
#include <string>

const uint32_t numDeltas = 60;

struct Timer {
	std::string name;
	int startIndex;
	int endIndex;
	std::vector<float> deltas = std::vector<float>(numDeltas);
	int deltaIndex;
};

class GpuProfiler
{
public:
	GpuProfiler(float timestampPeriod);
	~GpuProfiler() = default;

	void addTimer(std::string name, int start, int end);
	void update(const std::vector<uint64_t>& timestamps);
	void printResults();
private:
	std::vector<Timer> timers;
	float timestampPeriod;
};
