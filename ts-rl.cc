#include "ts-rl.h"

#include <stdexcept>
#include <thread>
#include <iostream>

using namespace std;

ts::RLProvider rlTSProvider;

ts::RateLimiter::RateLimiter(const ts::TSDescriptor& tsd) :
    TrafficShaper(tsd)
{
    uint64_t rate = strtou64(tsd.args);
    capacity = rate / (8 * SEC_EPOCH_CONV);
    availCapacity = capacity;
    nextReplenishTime = chrono::system_clock::now();
    timeInterval = chrono::microseconds(TIME_EPOCH_US);
}

ts::RateLimiter::~RateLimiter()
{
}

bool
ts::RateLimiter::isReady()
{
    //An approximation - we replenish only when we don't have capacity
    if (!availCapacity)
    {
        systime_t currTime = chrono::high_resolution_clock::now();
        if (currTime < nextReplenishTime)
        {
            //TODO: Improve this - make this asynchronous instead of blocking
            // This works for now as every Traffic Driver would run in its own
            // thread
            chrono::nanoseconds diff = nextReplenishTime - currTime;
            this_thread::sleep_for(diff);
        }
        availCapacity = capacity;
        nextReplenishTime = chrono::high_resolution_clock::now() + timeInterval;
    }
    return true;
}

uint64_t
ts::RateLimiter::avail()
{
	return availCapacity;
}

void
ts::RateLimiter::update(uint64_t size)
{
    availCapacity = (availCapacity <= size) ? 0 : (availCapacity - size);
}

ts::RLProvider::RLProvider() :
    ts::TSProvider("rate-limit")
{
}

ts::RLProvider::~RLProvider()
{
}

ts::RateLimiter*
ts::RLProvider::instantiate(const std::string args)
{
    return new ts::RateLimiter({name, args});
}
