#include "tcp.h"
#include "helper.h"

#include <sys/uio.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <string.h>

tcp::Socket::Socket(const int fd, const ip::sockaddr& addr) :
    ip::Socket(fd, addr)
{
}

tcp::Socket::Socket(const ip::sockaddr& addr) :
    ip::Socket(addr)
{
}

void
tcp::Socket::listen(int backlog)
{
    if (::listen(fd, backlog) == -1)
        throw std::runtime_error(ERRSTR("Error during listen"));
}

int
tcp::Socket::accept(ip::sockaddr& addr)
{
    int ret = ::accept(fd, &addr.sa, &addr.sa_len);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error during accept"));

    return ret;
}

void
tcp::Socket::connect(const ip::sockaddr& addr)
{
    if (::connect(fd, &addr.sa, addr.sa_len) == -1)
    {
        std::cout << "Connect Error " << strerror(errno) << std::endl;
        throw std::runtime_error(ERRSTR("Error during connect"));
    }
}

size_t
tcp::Socket::read(void* buf, size_t nbyte)
{
    return ::read(fd, buf, nbyte);
}

size_t
tcp::Socket::write(const void* buf, size_t nbytes)
{
    for (;;)
    {
#ifdef __linux__
        ssize_t rc = ::send(fd, buf, nbytes, MSG_NOSIGNAL);
#elif __APPLE__
        ssize_t rc = ::send(fd, buf, nbytes, 0);
#else
        throw std::runtime_error(ERRSTR("Unknown Platform"));
#endif

        if (rc >= 0)
            return rc;
        if (errno != EINTR)
            throw std::runtime_error(ERRSTR("Error during send"));
    }
}

void
tcp::Socket::writeBlock(struct iovec *iov, int iovcnt, size_t iovlen)
{
    for (;;)
    {
        ssize_t rc = ::writev(fd, iov, iovcnt);

        if (rc < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            throw std::runtime_error(ERRSTR("Error while sending data"));
        }

        if (iovlen - rc == 0)
            return;
        else if (iovlen < rc)
            throw std::runtime_error(ERRSTR("More than required bytes sent?"));

        while (rc >= iov->iov_len)
        {
            rc -= iov->iov_len;
            iovlen -= iov->iov_len;
            --iovcnt;
            ++iov;
        }

        if (rc)
            iovlen -= rc;
        iov->iov_len -= rc;
        iov->iov_base = (char *) iov->iov_base + rc;
    }
}

ssize_t
tcp::Socket::send(struct iovec *iov, size_t niov)
{
    msghdr mh = {};

    mh.msg_iov = iov;
    mh.msg_iovlen = niov;
#ifdef __linux__
    return ::sendmsg(fd, &mh, MSG_NOSIGNAL);
#elif __APPLE__
    return ::sendmsg(fd, &mh, 0);
#else
    throw std::runtime_error(ERRSTR("Unknown Platform"));
#endif
}

ssize_t
tcp::Socket::recv(void* buf, size_t bufLen)
{
    return ::recv(fd, buf, bufLen, 0);
}

ssize_t
tcp::Socket::recv_nonblocking(void* buf, size_t bufLen)
{
    return ::recv(fd, buf, bufLen, MSG_DONTWAIT);
}

void
tcp::Socket::setNagle(bool enabled)
{
    int noDelay = !enabled;
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &noDelay,
                           sizeof(noDelay));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting nagle"));
}

void
tcp::Socket::setKeepAliveCount(uint32_t size)
{
#ifdef __linux__
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &size,
                           sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive count"));
#else
    throw std::runtime_error(ERRSTR("Not supported"));
#endif
}

void
tcp::Socket::setKeepAliveIdle(uint32_t size)
{
#ifdef __linux__
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &size,
                           sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive idle"));
#else
    throw std::runtime_error(ERRSTR("Not supported"));
#endif
}

void
tcp::Socket::setKeepAliveInterval(uint32_t size)
{
#ifdef __linux__
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &size,
                           sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive interval"));
#else
    throw std::runtime_error(ERRSTR("Not supported"));
#endif
}

void
tcp::Socket::getTCPInfo(struct tcp_info* ti)
{
#ifdef __linux__
    socklen_t len = sizeof(*ti);
    int ret = ::getsockopt(fd, SOL_TCP, TCP_INFO, ti, &len);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error getting tcp info"));
#else
    throw std::runtime_error(ERRSTR("Not supported"));
#endif
}
