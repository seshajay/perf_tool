#include "tcp.h"
#include "helper.h"

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
        ssize_t rc = ::send(fd, buf, nbytes, MSG_NOSIGNAL);
        if (rc >= 0)
            return rc;
        if (errno != EINTR)
            throw std::runtime_error(ERRSTR("Error during send"));
    }
}

ssize_t
tcp::Socket::send(struct iovec *iov, size_t niov)
{
    msghdr mh = {};

    mh.msg_iov = iov;
    mh.msg_iovlen = niov;
    return ::sendmsg(fd, &mh, MSG_NOSIGNAL);
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
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &size,
                           sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive count"));
}

void
tcp::Socket::setKeepAliveIdle(uint32_t size)
{
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &size,
                           sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive idle"));
}

void
tcp::Socket::setKeepAliveInterval(uint32_t size)
{
    int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &size,
                           sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive interval"));
}

void
tcp::Socket::getTCPInfo(struct tcp_info* ti)
{
    socklen_t len = sizeof(*ti);
    int ret = ::getsockopt(fd, SOL_TCP, TCP_INFO, ti, &len);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error getting tcp info"));
}
