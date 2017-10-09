#ifndef __TCP_H
#define __TCP_H

#include "ip.h"
#include <netinet/tcp.h>

namespace tcp
{
    struct Socket : public ip::Socket
    {
        void listen(int backlog);
        int accept(ip::sockaddr& addr);
        void connect(const ip::sockaddr& addr);

        size_t read(void* buf, size_t nbyte);
        size_t write(const void* buf, size_t nbytes);
        void writeBlock(struct iovec *iov, int iovcnt, size_t iovlen);
        ssize_t send(struct iovec *iov, size_t niov);
        ssize_t recv(void* buf, size_t bufLen);
        ssize_t recv_nonblocking(void* buf, size_t bufLen);

        void setNagle(bool enabled);
        void setKeepAliveCount(uint32_t size);
        void setKeepAliveIdle(uint32_t size);
        void setKeepAliveInterval(uint32_t size);
        void getTCPInfo(struct tcp_info* ti);

        Socket(const int fd, const ip::sockaddr& addr);
        Socket(const ip::sockaddr& addr);

        virtual ~Socket() {}
    };
}

#endif /* __SOCKET_H */
