#include "Timer.h"

void Timer::reset()
{
	anchor = Clock::now();
}

float Timer::elapsed() const
{
	Duration duration = std::chrono::duration_cast<Duration>(Clock::now() - anchor);
	return duration.count();
}
