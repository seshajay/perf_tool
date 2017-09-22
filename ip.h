#ifndef __IP_H
#define __IP_H

#include <arpa/inet.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

namespace ip
{
#define IP_ADDR_LEN 48

    struct sockaddr
    {
        socklen_t sa_len;
        union
        {
            struct sockaddr_storage storage;
            struct ::sockaddr       sa;
            struct sockaddr_in      ipv4;
            struct sockaddr_in6     ipv6;
        };

		std::string toString() const;

        sockaddr(const std::string addr, const uint16_t port = 0);
        sockaddr() : sa_len(0), storage({}) {}
        ~sockaddr() {}
    };

    struct Socket
    {
        int fd;
        sockaddr addr;

        void     bind();
        void     setReuseAddr();
        void     setNonBlocking();
        void     setBlocking();
        void     setRecvTimeout(uint64_t timeoutUs);
        void     setRecvBufferSize(uint32_t size);
        void     setSendBufferSize(uint32_t size);
        void     setKeepAlive(bool enabled);
        uint32_t getRecvBufferSize();
        uint32_t getSendBufferSize();

        Socket(const int fd, const sockaddr& addr);
        Socket(const sockaddr& addr, const int type = SOCK_STREAM);

        virtual ~Socket();
    };
}

#endif /* __IP_H */
