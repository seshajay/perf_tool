#ifndef __HELPER_H
#define __HELPER_H

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <string>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <vector>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define ERRSTR(str) str " - " AT

typedef std::function<void (void)> func_t;

typedef std::chrono::time_point<std::chrono::system_clock> systime_t;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> hrsystime_t;

static const std::vector<std::string> sizes = { "bps", "kbps", "mbps", "gbps"};
static const std::vector<int> multipliers   = { 0, 10, 20, 30};

inline std::string formatThroughput(uint64_t tput)
{
    int exp = (int) log10(tput);
    int index = exp / 3;

    if (index > 3)
        throw std::runtime_error(ERRSTR("Out of range"));

    double div = pow(10, index * 3);

    std::stringstream tputStr;
    tputStr.precision(4);
    tputStr << tput / div << " " << sizes[index];

    return tputStr.str();
}

inline uint64_t strtou64(const std::string& str)
{
    if (str == "")
        throw std::runtime_error(ERRSTR("Empty string"));

    char *pEnd = NULL;
    double val = ::strtod(str.c_str(), &pEnd);

    if (!val || errno == ERANGE)
        throw std::runtime_error(ERRSTR("Out of range"));

    if (!pEnd[0])
        return (uint64_t) val;

    assert(sizes.size() == multipliers.size());
    int index;
    for (index = 0; index < sizes.size(); index++)
    {
        if (!sizes[index].compare(pEnd))
            return (uint64_t) (val * (1ULL << multipliers[index]));
    }
    throw std::runtime_error(ERRSTR("Invalid multiplier"));
}

#endif /* __HELPER_H */
