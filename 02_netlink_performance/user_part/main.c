#include "taco_user.h"

static unsigned char _buffer[NLMSG_SPACE(MAX_PAYLOAD)] = {};

static int _running = 1;

static void _sigIntHandler(int signo)
{
    if (signo == SIGINT)
    {
        _running = 0;
    }
}

static int _bindNetlinkSocket(int socketFd)
{
    struct sockaddr_nl rxaddr = {
        .nl_family = AF_NETLINK,
        .nl_pid = getpid(),
        .nl_groups = 0,
    };

    return bind(socketFd, (struct sockaddr*)&rxaddr, sizeof(rxaddr));
}

static int _recvKernelMessage(int socketFd)
{
    struct nlmsghdr *nlhdr;
    nlhdr = (struct nlmsghdr*)_buffer;
    memset(nlhdr, 0, sizeof(struct nlmsghdr));
    nlhdr->nlmsg_len   = NLMSG_SPACE(MAX_PAYLOAD);
    nlhdr->nlmsg_pid   = getpid();
    nlhdr->nlmsg_flags = 0;

    struct iovec iov = {
        .iov_base = (void*)nlhdr,
        .iov_len = nlhdr->nlmsg_len,
    };

    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    return recvmsg(socketFd, &msg, 0);
}

static int _sendMessageToKernel(int socketFd)
{
    struct sockaddr_nl txaddr = {
        .nl_family = AF_NETLINK,
        .nl_pid = 0,
        .nl_groups = 0,
    };

    struct nlmsghdr *nlhdr;
    nlhdr = (struct nlmsghdr*)_buffer;
    memset(nlhdr, 0, sizeof(struct nlmsghdr));
    nlhdr->nlmsg_len   = NLMSG_SPACE(MAX_PAYLOAD);
    nlhdr->nlmsg_pid   = getpid();
    nlhdr->nlmsg_flags = 0;

    sprintf( NLMSG_DATA(nlhdr), "Hello from User" );

    struct iovec iov = {
        .iov_base = (void*)nlhdr,
        .iov_len = nlhdr->nlmsg_len,
    };

    struct msghdr msg = {
        .msg_name = (void*)&txaddr,
        .msg_namelen = sizeof(txaddr),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    return sendmsg(socketFd, &msg, 0);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, _sigIntHandler);

    int socketFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    CHECK_IF(0 > socketFd, goto _ERROR, "socket failed");

    int ret = _bindNetlinkSocket(socketFd);
    CHECK_IF(0 > ret, goto _ERROR, "bind failed");

    int sendLen = _sendMessageToKernel(socketFd);
    CHECK_IF(0 >= sendLen, goto _ERROR, "_sendMessageToKernel failed");

    fd_set readset;
    int selectRet = 0;
    struct timeval timeout;
    int recvLen = 0;

    while (_running)
    {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(socketFd, &readset);

        selectRet = select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
        if (0 > selectRet)
        {
            break;
        }
        else if (0 == selectRet)
        {
            // timeout
            continue;
        }
        else
        {
            if (FD_ISSET(socketFd, &readset))
            {
                recvLen = _recvKernelMessage(socketFd);
                CHECK_IF(0 >= recvLen, goto _ERROR,
                         "_recvKernelMessage failed");

                dprint("recvLen = %d", recvLen);
            }
        }
    }

    close(socketFd);
    return 0;

_ERROR:
    if (socketFd > 0)
    {
        close(socketFd);
    }
    return -1;
}
