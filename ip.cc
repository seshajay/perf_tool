#include "ip.h"
#include "helper.h"

#include <fcntl.h>
#include <netdb.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>

using namespace std;

int
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

static int
ip_guessIPAddressFamily(const string& addr)
{
    if (addr.find(':') == std::string::npos)
        return AF_INET;

    return AF_INET6;
}

ip::sockaddr::sockaddr(const std::string addr, const uint16_t port)
{
    int family = ip_guessIPAddressFamily(addr);
    int ret;
    storage.ss_family = family;

    if (family == AF_INET)
    {
        ret = inet_pton(AF_INET, addr.c_str(), &ipv4.sin_addr);
        sa_len = sizeof(struct sockaddr_in);
        ipv4.sin_port = htons(port);
    }
    else if (family == AF_INET6)
    {
        ret = inet_pton(AF_INET6, addr.c_str(), &ipv6.sin6_addr);
        sa_len = sizeof(struct sockaddr_in6);
        ipv6.sin6_port = htons(port);
    }
    else
    {
        throw std::runtime_error(ERRSTR("Wrong Address family"));
    }

    if (ret != 1)
        throw std::invalid_argument(ERRSTR("Wrong Address!"));
}

std::string
ip::sockaddr::toString() const
{
	char addrStr[IP_ADDR_LEN];
    int family = sa.sa_family;

	if (family == AF_INET)
        inet_ntop(family, &ipv4.sin_addr, addrStr, IP_ADDR_LEN);
	else if (family == AF_INET6)
		inet_ntop(family, &ipv6.sin6_addr, addrStr, IP_ADDR_LEN);
	else
		return "Unknown address";

	return addrStr;
}

ip::Socket::Socket(const int fd, const ip::sockaddr& addr) :
    fd(fd),
    addr(addr)
{
}

ip::Socket::Socket(const ip::sockaddr& addr, const int type) :
    addr(addr)
{
    int ret;

    fd = ::socket(addr.sa.sa_family, type, 0);
    if (fd == -1)
        throw std::runtime_error(ERRSTR("Error creating a socket"));
}

ip::Socket::~Socket()
{
    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
}

void
ip::Socket::bind()
{
    int ret = ::bind(fd, &addr.sa, addr.sa_len);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error during socket bind"));
}

void
ip::Socket::setReuseAddr()
{
    const int on = 1;
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error during set reuse addr"));
}

void
ip::Socket::setNonBlocking()
{
    int flags = rfcntl(fd, F_GETFL, 0);
    rfcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void
ip::Socket::setBlocking()
{
    int flags = rfcntl(fd, F_GETFL, 0);
    rfcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

void
ip::Socket::setRecvTimeout(uint64_t timeoutUs)
{
    struct timeval tv;
    tv.tv_sec = timeoutUs / 1000000;
    tv.tv_usec = timeoutUs % 1000000;
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting recv timeout"));
}

void
ip::Socket::setRecvBufferSize(uint32_t size)
{
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting recv buffer size"));
}

void
ip::Socket::setSendBufferSize(uint32_t size)
{
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting send buffer size"));
}

void
ip::Socket::setKeepAlive(bool enabled)
{
    int val = enabled;
    int ret = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error setting keepalive"));
}

uint32_t
ip::Socket::getRecvBufferSize()
{
    int val;
    socklen_t len = sizeof(val);
    int ret = ::getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &val, &len);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error getting recv buffer size"));

    return val;
}

uint32_t
ip::Socket::getSendBufferSize()
{
    int val;
    socklen_t len = sizeof(val);
    int ret = ::getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &val, &len);
    if (ret == -1)
        throw std::runtime_error(ERRSTR("Error getting send buffer size"));

    return val;
}
