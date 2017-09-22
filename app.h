#ifndef __APP_H
#define __APP_H

#include "tcp.h"
#include "helper.h"
#include "traffic.h"

#include <limits>
#include <string>
#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>

namespace app
{
    struct PerfApp
    {
        PerfApp() {}
        virtual ~PerfApp() {}
    };

    struct ClientApp : public PerfApp
    {
        const uint64_t testDurationSec;
        uint64_t totalBytesSent;
        std::list<TrafficDriver*> drivers;
		const func_t cb;
        const ts::TSDescriptor tsd;

    protected:
        void cleanup();
        void manageDrivers();

    public:
        ClientApp(const ip::sockaddr& laddr, const ip::sockaddr& raddr,
                  const uint64_t testDurationSec, const func_t cb,
                  const ts::TSDescriptor& tsd, const uint16_t numDrivers = 1,
                  const MsgSize msgSize = large, const uint32_t sndBufSize = 0);
        virtual ~ClientApp();
    };

    struct ServerApp : public PerfApp
    {
        tcp::Socket sock;
        int efd;
        uint64_t totalBytesReceived;
        bool shuttingDown;
		bool completedServerVal;

        std::mutex completedServersLock;
        std::condition_variable completedServersCV;
        std::list<TrafficServer*> completedServers;

        std::mutex activeServersLock;
        std::list<TrafficServer*> activeServers;

        std::thread serverThread;
        std::thread listenerThread;

    protected:
        void activeServerCleanup();
        void completedServerCleanup();
        void manageServers();
        void listen();

    public:
        void serverCompleted(TrafficServer* server);

        ServerApp(const ip::sockaddr& addr, const uint64_t rcvBufSize = 0,
                  const uint16_t backlog = 128);
        virtual ~ServerApp();
    };
};
#endif /* __APP_H */
