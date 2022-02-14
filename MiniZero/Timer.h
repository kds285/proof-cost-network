#ifndef H_STOP_TIMER
#define H_STOP_TIMER

#include <ctime>
#include <ratio>
#include <chrono>
using namespace std::chrono;

#define ull unsigned long long

class StopTimer
{
public:
	inline void reset() { 
		start_time_point = {}; 
		end_time_point = {};
		accumulated_time_duration = {}; 
		start_count = 0 ;
		stop_count = 0 ;
	}
	inline void start() { 
		start_time_point = high_resolution_clock::now(); 
		++start_count;
	}
	inline void stop()  {
		end_time_point = high_resolution_clock::now(); 
		++stop_count;
	}
	inline duration<double> getElapsedTime() const { return duration_cast<duration<double>>(end_time_point - start_time_point); }
	inline void stopAndAddAccumulatedTime() {
		stop();
		addAccumulatedElapsedTime();
	}
	inline void addAccumulatedElapsedTime() { accumulated_time_duration += getElapsedTime(); }
	inline duration<double> getAccumulatedElapsedTime() const { return accumulated_time_duration; }
	inline ull getStartCount() { return start_count; }
	inline ull getStopCount() { return stop_count; }

private:
	high_resolution_clock::time_point start_time_point, end_time_point;
	duration<double> accumulated_time_duration;
	ull start_count ;
	ull stop_count ;
};

#endif