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
    CHECK_IF(0 > socketFd, goto _ERROR, "socket failed");

    int ret = _bindNetlinkSocket(socketFd);
    CHECK_IF(0 > ret, goto _ERROR, "bind failed");

    struct sockaddr_nl txaddr;
    memset( &txaddr, 0, sizeof(txaddr) );
    txaddr.nl_family = AF_NETLINK;
    txaddr.nl_pid    = 0;
    txaddr.nl_groups = 0;

    struct nlmsghdr *nlhdr;
    nlhdr = (struct nlmsghdr*)_buffer;
    nlhdr->nlmsg_len   = NLMSG_SPACE(MAX_PAYLOAD);
    nlhdr->nlmsg_pid   = getpid();
    nlhdr->nlmsg_flags = 0;
    sprintf( NLMSG_DATA(nlhdr), "Hello from User" );

    struct iovec iov;
    memset( &iov, 0, sizeof(struct iovec) );
    iov.iov_base = (void*)nlhdr;
    iov.iov_len  = nlhdr->nlmsg_len;

    struct msghdr msg;
    memset( &msg, 0, sizeof(struct msghdr) );
    msg.msg_name    = (void*)&txaddr;
    msg.msg_namelen = sizeof(txaddr);
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

_ERROR:
    if (socketFd > 0)
    {
        close(socketFd);
    }
    return -1;
}
