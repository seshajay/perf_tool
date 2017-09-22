#include "app.h"

#include <algorithm>
#include <iostream>
#include <poll.h>
#include <sys/eventfd.h>
#include <cstring>

using namespace std;

app::ClientApp::ClientApp(const ip::sockaddr& laddr, const ip::sockaddr& raddr,
                          const uint64_t testDurationSec, const func_t cb,
                          const ts::TSDescriptor& tsd,
                          const uint16_t numDrivers, const MsgSize msgSize,
                          const uint32_t sndBufSize) :
    testDurationSec(testDurationSec),
    totalBytesSent(0),
	cb(cb),
    tsd(tsd)
{
    if (!testDurationSec)
        throw std::runtime_error(ERRSTR("Need a non-zero test duration"));
    if (!numDrivers)
        throw std::runtime_error(ERRSTR("Need at least 1 driver"));

    try
    {
        int i;
        for (i = 0; i < numDrivers; i++)
        {
            string name = "Driver-" + to_string(i);
            TrafficDriver* driver = new TrafficDriver(name, laddr, raddr, tsd,
                                                      msgSize, sndBufSize);
            drivers.push_back(driver);
        }

        manageDrivers();
    }
    catch(...)
    {
        cleanup();
        throw;
    }
}

app::ClientApp::~ClientApp()
{
    cleanup();
}

void
app::ClientApp::cleanup()
{
    while (!drivers.empty())
    {
        auto driver = drivers.back();
        drivers.pop_back();
        delete driver;
    }
}

void
app::ClientApp::manageDrivers()
{
    this_thread::sleep_for(chrono::seconds(testDurationSec));
    for (auto driver : drivers)
    {
        driver->stopTraffic();
        totalBytesSent += driver->sentBytes;
    }
	cout << "All Drivers completed\n";
	cb();
}

app::ServerApp::ServerApp(const ip::sockaddr& addr, const uint64_t rcvBufSize,
                          const uint16_t backlog) :
    sock(addr),
    totalBytesReceived(0),
    shuttingDown(false)
{
    efd = eventfd(0, 0);
    if (efd == -1)
        throw std::runtime_error(ERRSTR("Error creating event fd"));

    sock.setNonBlocking();
    sock.setReuseAddr();
    sock.bind();
    sock.listen(backlog);

    if (rcvBufSize)
        sock.setRecvBufferSize(rcvBufSize);

    serverThread = thread(&app::ServerApp::manageServers, this);
    listenerThread = thread(&app::ServerApp::listen, this);
}

app::ServerApp::~ServerApp()
{
    shuttingDown = true;
    eventfd_write(efd, 1);
	completedServersLock.lock();
	completedServerVal = true;
    completedServersCV.notify_one();
	completedServersLock.unlock();
    serverThread.join();
    listenerThread.join();
    activeServerCleanup();
    // No more active servers to complete, so we don't have to take a lock
    completedServerCleanup();
}

void
app::ServerApp::activeServerCleanup()
{
    std::lock_guard<std::mutex> lock(activeServersLock);
    while (!activeServers.empty())
    {
        auto as = activeServers.back();
        activeServers.pop_back();
        totalBytesReceived += as->bytesReceived;
        delete as;
    }
}

void
app::ServerApp::completedServerCleanup()
{
    while (!completedServers.empty())
    {
        auto cs = completedServers.back();
        completedServers.pop_back();
        totalBytesReceived += cs->bytesReceived;
        delete cs;
    }
}

void
app::ServerApp::manageServers()
{
    while (!shuttingDown)
    {
        std::unique_lock<std::mutex> ul(completedServersLock);
        completedServersCV.wait(ul, [this]{return completedServerVal == true;});
        completedServerCleanup();
		completedServerVal = false;
    }
}

void
app::ServerApp::listen()
{
    struct pollfd fds[2];
    fds[0].fd      = sock.fd;
    fds[0].events  = POLLIN;
    fds[0].revents = 0;
    fds[1].fd      = efd;
    fds[1].events  = POLLIN;
    fds[1].revents = 0;

    while (!shuttingDown)
    {
        int ret = poll(fds, 2, -1);
        if (ret == -1)
            throw std::runtime_error(ERRSTR("Error during poll"));
        if (shuttingDown)
            break;

        ip::sockaddr addr;
        addr.sa.sa_family = AF_INET;
        int fd;
        try
        {
            fd = sock.accept(addr);
        }
        catch (...)
        {
            if (errno != EWOULDBLOCK && errno != ECONNABORTED &&
                errno != EPROTO && errno != EINTR)
            {
                throw;
            }
        }

        if (fd > 0)
        {
            uint16_t numServers = activeServers.size();
            string name = "Server-" + to_string(numServers);
            cout << "Received connection from ";
            cout << addr.toString().c_str() << endl;
            funcTS_t cb = std::bind(&app::ServerApp::serverCompleted, this,
                                    std::placeholders::_1);
            app::TrafficServer* server = new app::TrafficServer(name, fd,
                                                                sock.addr, addr,
                                                                cb);

            std::lock_guard<std::mutex> lock(activeServersLock);
            activeServers.push_back(server);
        }
    }
}

void
app::ServerApp::serverCompleted(app::TrafficServer* server)
{
    std::lock_guard<std::mutex> aslock(activeServersLock);
    auto it = std::find(activeServers.begin(),
                        activeServers.end(), server);
    if (it != activeServers.end())
    {
        activeServers.remove(server);
        std::lock_guard<std::mutex> cslock(completedServersLock);
        completedServers.push_back(server);
		completedServerVal = true;
        completedServersCV.notify_one();
    }
}
