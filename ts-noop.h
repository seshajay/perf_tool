#ifndef __TS_NOOP_H
#define __TS_NOOP_H

#include "ts.h"

#include <string>

namespace ts
{
    struct NOOP : public TrafficShaper
    {
        virtual bool isReady();
        virtual uint64_t avail();
        virtual void update(uint64_t size);

        NOOP(const TSDescriptor& tsd);
        virtual ~NOOP();
    };

    struct NOOPProvider : public TSProvider
    {
        const std::string name;

        virtual NOOP* instantiate(const std::string args = "");

        NOOPProvider();
        virtual ~NOOPProvider();
    };

    extern NOOPProvider noopTSProvider;
}
#endif
