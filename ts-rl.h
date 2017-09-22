#ifndef __TS_RL_H
#define __TS_RL_H

#include "ts.h"
#include "helper.h"

#include <string>

namespace ts
{
#define TIME_EPOCH_US 500
#define SEC_EPOCH_CONV (1000000 / TIME_EPOCH_US)

    struct RateLimiter : public TrafficShaper
    {
        size_t capacity;
        size_t availCapacity;
        systime_t nextReplenishTime;
        std::chrono::nanoseconds timeInterval;

        virtual bool isReady();
		virtual uint64_t avail();
        virtual void update(uint64_t size);

        RateLimiter(const TSDescriptor& tsd);
        virtual ~RateLimiter();
    };

    struct RLProvider : public TSProvider
    {
        const std::string name;

        virtual RateLimiter* instantiate(const std::string args);

        RLProvider();
        virtual ~RLProvider();
    };

    extern RLProvider rlTSProvider;
}
#endif
