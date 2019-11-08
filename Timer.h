#pragma once

#include <chrono>

class Timer
{
	using Clock = std::chrono::high_resolution_clock;
	using Timestamp = Clock::time_point;
	using Duration = std::chrono::duration<float>;

	Timestamp anchor = Clock::now();

public:
	void reset();
	float elapsed() const;
};