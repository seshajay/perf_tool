#include "ts.h"

#include <list>

static std::list<ts::TSProvider *> providers;

ts::TrafficShaper::TrafficShaper(const ts::TSDescriptor& tsd) :
    tsd(tsd)
{
}

ts::TrafficShaper::~TrafficShaper()
{
}

ts::TSProvider::TSProvider(const char* name) :
    name(name)
{
    providers.push_back(this);
}

ts::TSProvider::~TSProvider()
{
}

ts::TSProvider* ts::findTSProvider(const std::string& name)
{
    for (auto provider : providers)
    {
        if (provider->name == name)
            return provider;
    }

    return NULL;
}
