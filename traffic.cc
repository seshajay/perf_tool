#include "traffic.h"
#include "ts.h"

#include <iostream>
#include <poll.h>
#include <sys/eventfd.h>
#include <cstring>

using namespace std;

app::TrafficEnabler::TrafficEnabler(const std::string& name,
                                    tcp::Socket* sock,
                                    const ip::sockaddr& raddr, char* buf) :
    name(name),
    sock(sock),
    raddr(raddr),
    buf(buf)
{
}

app::TrafficEnabler::~TrafficEnabler()
{
    if (buf)
        free(buf);
    delete sock;
}

app::TrafficDriver::TrafficDriver(const string& name, const ip::sockaddr& laddr,
                                  const ip::sockaddr& raddr,
                                  const ts::TSDescriptor& tsd,
                                  const MsgSize msgSize,
                                  const uint32_t sndBufSize) :
    app::TrafficEnabler(name, new tcp::Socket(laddr), raddr, NULL),
    msgSize(msgSize),
    sentBytes(0),
    shuttingDown(false)
{
    try
    {
        uint16_t lport;
        switch (laddr.sa.sa_family)
        {
        case AF_INET:
            lport = laddr.ipv4.sin_port;
            break;
        case AF_INET6:
            lport = laddr.ipv6.sin6_port;
            break;
        default:
            throw std::runtime_error(ERRSTR("Wrong address family"));
        }
        if (lport)
            sock->bind();

        if (sndBufSize)
            sock->setSendBufferSize(sndBufSize);

        ts::TSProvider* tsp = ts::findTSProvider(tsd.name);
        if (!tsp)
            throw std::runtime_error(ERRSTR("Wrong Traffic Shaper"));
        ts = tsp->instantiate(tsd.args);

        driverThread = thread(&app::TrafficDriver::doSetupAndStart, this);
    }
    catch (...)
    {
    }
}

app::TrafficDriver::~TrafficDriver()
{
    stopTraffic();
    delete ts;
}

void
app::TrafficDriver::doSetupAndStart()
{
    sock->connect(raddr);
    cout << "Connected with " << raddr.toString().c_str() << endl;

    sock->setNagle(false);
    buf = (char *) malloc(msgSize * sizeof(char));
    memset(buf, 1, msgSize);

    startTraffic();
}

void
app::TrafficDriver::startTraffic()
{
    while (!shuttingDown)
    {
        if (ts->isReady())
        {
			// TODO: Improve this - split into perftest and reltest
			uint64_t avail = std::min((uint64_t) msgSize, ts->avail());
            uint64_t sent = sock->write(buf, avail);
            ts->update(sent);
            sentBytes += sent;
        }
    }
}

void
app::TrafficDriver::stopTraffic()
{
    if (!shuttingDown)
    {
        shuttingDown = true;
        driverThread.join();
        cout << name.c_str() << " sent " << sentBytes << " bytes\n";
    }
}

app::TrafficServer::TrafficServer(const std::string& name, const int fd,
                                  const ip::sockaddr& laddr,
                                  const ip::sockaddr& raddr,
                                  funcTS_t cb) :
    app::TrafficEnabler(name, new tcp::Socket(fd, laddr), raddr, NULL),
    bytesReceived(0),
    cb(cb),
    shuttingDown(false)
{
    efd = eventfd(0, 0);
    if (efd == -1)
        throw std::runtime_error(ERRSTR("Error creating event fd"));

    startTime = chrono::system_clock::now();

    serverThread = thread(&app::TrafficServer::doSetupAndStart, this);
}

app::TrafficServer::~TrafficServer()
{
    shuttingDown = true;
    eventfd_write(efd, 1);
    serverThread.join();
    printStats();
}

void
app::TrafficServer::doSetupAndStart()
{
    buf = (char *) malloc(large * sizeof(char));
    recvTraffic();
}

void
app::TrafficServer::recvTraffic() try
{
    struct pollfd fds[2];
    fds[0].fd      = sock->fd;
    fds[0].events  = POLLIN;
    fds[0].revents = 0;
    fds[1].fd      = efd;
    fds[1].events  = POLLIN;
    fds[1].revents = 0;

    while (!shuttingDown)
    {
        int rc = poll(fds, 2, -1);
        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            else
                throw std::runtime_error(ERRSTR("Error in poll"));
        }

        if (shuttingDown)
            break;

        ssize_t count = sock->recv(buf, large);
        if (count <= 0)
        {
            if (count < 0 && errno == EINTR)
                continue;
            throw std::runtime_error(ERRSTR("conn closed"));
        }

        bytesReceived += count;
    }
}
catch(...)
{
    cb(this);
}

void
app::TrafficServer::printStats()
{
    systime_t endTime = std::chrono::system_clock::now();
    chrono::duration<double> diff = endTime - startTime;
    uint64_t tput = (bytesReceived / diff.count()) * 8;
    string tputStr = (tput) ? formatThroughput(tput) : "0 bps";

	cout << name.c_str() << " done\n";
    cout << "Bytes Received: " << bytesReceived << endl;
    cout << "Time elapsed: " << diff.count() << " sec\n";
    cout << "Throughput: " << tputStr.c_str() << endl;
}
