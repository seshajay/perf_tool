#include "ts-noop.h"

#include <limits>

ts::NOOPProvider noopTSProvider;

ts::NOOP::NOOP(const ts::TSDescriptor& tsd) :
    TrafficShaper(tsd)
{
}

ts::NOOP::~NOOP()
{
}

bool
ts::NOOP::isReady()
{
    return true;
}

uint64_t
ts::NOOP::avail()
{
    return std::numeric_limits<uint64_t>::max();
}

void
ts::NOOP::update(uint64_t size)
{
}

ts::NOOPProvider::NOOPProvider() :
    ts::TSProvider("noop")
{
}

ts::NOOPProvider::~NOOPProvider()
{
}

ts::NOOP*
ts::NOOPProvider::instantiate(const std::string args)
{
    return new ts::NOOP({name, ""});
}
