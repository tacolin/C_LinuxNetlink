#include "taco_user.h"

static unsigned char _buffer[NLMSG_SPACE(MAX_PAYLOAD)] = {};

static int _bindNetlinkSocket(int socketFd)
{
    struct sockaddr_nl rxaddr = {
        .nl_family = AF_NETLINK,
        .nl_pid = getpid(),
        .nl_groups = 0,
    };
    return bind( socketFd, (struct sockaddr*)&rxaddr, sizeof(rxaddr) );
}

int main(int argc, char* argv[])
{
    int socketFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    CHECK_IF(0 > socketFd, return -1, "socket failed");

    _bindNetlinkSocket(socketFd);

    struct sockaddr_nl dstAddr;
    memset( &dstAddr, 0, sizeof(dstAddr) );
    dstAddr.nl_family = AF_NETLINK;
    dstAddr.nl_pid    = 0;
    dstAddr.nl_groups = 0;

    struct nlmsghdr *pAllocatedNlhdr;
    pAllocatedNlhdr = (struct nlmsghdr*)_buffer;
    memset( pAllocatedNlhdr, 0, NLMSG_SPACE(MAX_PAYLOAD) );
    pAllocatedNlhdr->nlmsg_len   = NLMSG_SPACE(MAX_PAYLOAD);
    pAllocatedNlhdr->nlmsg_pid   = getpid();
    pAllocatedNlhdr->nlmsg_flags = 0;
    sprintf( NLMSG_DATA(pAllocatedNlhdr), "Hello from User" );

    struct iovec iov;
    memset( &iov, 0, sizeof(struct iovec) );
    iov.iov_base = (void*)pAllocatedNlhdr;
    iov.iov_len  = pAllocatedNlhdr->nlmsg_len;

    struct msghdr msg;
    memset( &msg, 0, sizeof(struct msghdr) );
    msg.msg_name    = (void*)&dstAddr;
    msg.msg_namelen = sizeof(dstAddr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg( socketFd, &msg, 0 );

    char localBufNlhdr[ NLMSG_SPACE(MAX_PAYLOAD) ];
    memset( localBufNlhdr, 0, NLMSG_SPACE(MAX_PAYLOAD) );
    iov.iov_base = (void*)localBufNlhdr; // 把記憶體位置換掉

    recvmsg( socketFd, &msg, 0 );
    dprint("Receive message from kernel : %s", (char*)NLMSG_DATA(localBufNlhdr) );

    close(socketFd);

    return 0;
}
