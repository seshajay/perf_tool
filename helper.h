#ifndef __HELPER_H
#define __HELPER_H

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <functional>
#include <string>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <unistd.h>

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
    uint8_t index;
    for (index = 0; index < sizes.size(); index++)
    {
        if (!sizes[index].compare(pEnd))
            return (uint64_t) (val * (1ULL << multipliers[index]));
    }
    throw std::runtime_error(ERRSTR("Invalid multiplier"));
}

inline int
rfcntl(int fd, int cmd, long arg)
{
    for (;;)
    {
        int rc = ::fcntl(fd, cmd, arg);
        if (rc != -1)
            return rc;
        if (errno != EINTR)
            std::runtime_error(ERRSTR("Error in fcntl"));
    }
}

inline void
ev_pipe(int pfd[2])
{
    if (pipe(pfd) == -1)
        throw std::runtime_error(ERRSTR("Error in pipe()"));

    int flags = rfcntl(pfd[0], F_GETFL, 0);
    rfcntl(pfd[0], F_SETFL, flags | O_NONBLOCK);

    flags = rfcntl(pfd[1], F_GETFL, 0);
    rfcntl(pfd[1], F_SETFL, flags | O_NONBLOCK);
}

inline int
ev_pipe_write(int pfd)
{
    for (;;)
    {
        int rc = write(pfd, "x", 1);
        if (rc > 0)
            return rc;
        if (errno != EAGAIN)
            std::runtime_error(ERRSTR("Error while writing to pipe"));
    }
}
#endif /* __HELPER_H */
