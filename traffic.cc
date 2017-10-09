#include "traffic.h"
#include "ts.h"

#include <iostream>

#ifdef __linux__
#include <poll.h>
#include <sys/eventfd.h>
#endif

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

#ifdef __APPLE__
    sock->setNoSIGPIPE();
#endif

    ts::TSProvider* tsp = ts::findTSProvider(tsd.name);
    if (!tsp)
        throw std::runtime_error(ERRSTR("Wrong Traffic Shaper"));
    ts = tsp->instantiate(tsd.args);

    driverThread = thread(&app::TrafficDriver::doSetupAndStart, this);
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
            struct iovec iov[2];
            // TODO: Use iov[0] to send the base packet header instead of the
            // size of transfer
            iov[0].iov_base = &avail;
            iov[0].iov_len  = sizeof(avail);
            iov[1].iov_base = buf;
            iov[1].iov_len  = avail;
            size_t iovlen = iov[0].iov_len + iov[1].iov_len;
            sock->writeBlock(iov, 2, iovlen);
            ts->update(iovlen);
            sentBytes += iovlen;
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
#ifdef __linux__
    efd = eventfd(0, 0);
    if (efd == -1)
        throw std::runtime_error(ERRSTR("Error creating event fd"));
#elif __APPLE__
    kq = kqueue();
    if (kq == -1)
        throw std::runtime_error(ERRSTR("Error in kqueue()"));

    ev_pipe(pfd);
    event = (struct kevent *) malloc(sizeof(struct kevent) * 2);
    tevent = (struct kevent *) malloc(sizeof(struct kevent) * 2);
#else
    throw std::runtime_error(ERRSTR("Unsupported Platform"));
#endif

    startTime = chrono::system_clock::now();

    serverThread = thread(&app::TrafficServer::doSetupAndStart, this);
}

app::TrafficServer::~TrafficServer()
{
    shuttingDown = true;

#ifdef __linux__
    eventfd_write(efd, 1);
#elif __APPLE__
    ev_pipe_write(pfd[1]);
#else
    throw std::runtime_error(ERRSTR("Unsupported Platform"));
#endif

    serverThread.join();
    printStats();

#ifdef __APPLE__
    close(pfd[0]);
    close(pfd[1]);
    free(event);
    free(tevent);
#endif
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
#ifdef __linux__
    struct pollfd fds[2];
    fds[0].fd      = sock->fd;
    fds[0].events  = POLLIN;
    fds[0].revents = 0;
    fds[1].fd      = efd;
    fds[1].events  = POLLIN;
    fds[1].revents = 0;
#elif __APPLE__
    EV_SET(event, pfd[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    EV_SET(event + 1, sock->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

    int ret = kevent(kq, event, 2, NULL, 0, NULL);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error while registering kevents"));
    if (event->flags & EV_ERROR || (event+1)->flags & EV_ERROR)
        throw std::runtime_error(ERRSTR("Event Error"));

#else
    throw std::runtime_error(ERRSTR("Unsupported Platform"));
#endif

    while (!shuttingDown)
    {
#ifdef __linux__
        int rc = poll(fds, 2, -1);
#elif __APPLE__
        int rc = kevent(kq, NULL, 0, tevent, 2, NULL);
#else
        throw std::runtime_error(ERRSTR("Unsupported Platform"));
#endif
        if (rc < 0)
        {
            std::cout << "Error\n";
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
