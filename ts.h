#ifndef __TS_H
#define __TS_H

#include <string>

namespace ts
{
    struct TSDescriptor
    {
        std::string name;
        std::string args;
    };

    struct TrafficShaper
    {
        const TSDescriptor tsd;

        virtual bool isReady() = 0;
		virtual uint64_t avail() = 0;
        virtual void update(uint64_t size) = 0;

        TrafficShaper(const TSDescriptor& tsd);
        virtual ~TrafficShaper();
    };

    struct TSProvider
    {
        const std::string name;

        virtual TrafficShaper* instantiate(const std::string args) = 0;

        TSProvider(const char* name);
        ~TSProvider();
    };

    TSProvider* findTSProvider(const std::string& name);
}
#endif
