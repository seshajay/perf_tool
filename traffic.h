#ifndef __TRAFFIC_H
#define __TRAFFIC_H

#include "tcp.h"
#include "helper.h"
#include "ts.h"

#ifdef __APPLE__
#include <sys/event.h>
#endif

#include <string>
#include <thread>

namespace app
{
    struct TrafficServer;
    typedef std::function<void (TrafficServer *)> funcTS_t;

    enum MsgSize
    {
        small = 32,
        large = 64 * 1024,
    };

	struct TrafficEnabler
	{
        std::string name;
        tcp::Socket* sock;
        ip::sockaddr raddr;
        char* buf;

        virtual void doSetupAndStart() = 0;

        TrafficEnabler(const std::string& name, tcp::Socket* sock,
                       const ip::sockaddr& raddr, char* buf);
        virtual ~TrafficEnabler();
	};

    struct TrafficDriver : public TrafficEnabler
    {
        MsgSize msgSize;
        uint64_t sentBytes;
        ts::TrafficShaper* ts;
        bool shuttingDown;
        std::thread driverThread;

        virtual void doSetupAndStart();
        void startTraffic();
        void stopTraffic();

        TrafficDriver(const std::string& name, const ip::sockaddr& laddr,
                      const ip::sockaddr& raddr, const ts::TSDescriptor& tsd,
                      const MsgSize msgSize = large,
                      const uint32_t sndBufSize = 0);
        virtual ~TrafficDriver();
    };

    struct TrafficServer : public TrafficEnabler
    {
#ifdef __linux__
        int efd;
#elif __APPLE__
        int kq;
        int pfd[2];
        struct kevent *event;
        struct kevent *tevent;
#endif
        uint64_t bytesReceived;
        funcTS_t cb;
        bool shuttingDown;
        std::thread serverThread;
        systime_t startTime;

        virtual void doSetupAndStart();
        void recvTraffic();
        void printStats();

        TrafficServer(const std::string& name, const int fd,
                      const ip::sockaddr& laddr, const ip::sockaddr& raddr,
                      funcTS_t cb);
        virtual ~TrafficServer();
    };
};
#endif /* __TRAFFIC_H */
