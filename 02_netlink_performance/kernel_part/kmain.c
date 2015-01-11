#include "taco_kernel.h"

MODULE_AUTHOR("tacolin");
MODULE_DESCRIPTION("NETLINK KERNEL TEST MODULE");
MODULE_LICENSE("GPL");

static struct sock* _socket = NULL;
static unsigned char _rxBuffer[MAX_PAYLOAD] = {};

static void _recvUserMessage(struct sk_buff *skb, char* recvBuf, int *pid)
{
    struct nlmsghdr *nlhdr = NULL;

    CHECK_IF(NULL==skb, return, "skb is null");
    CHECK_IF(NULL==recvBuf, return, "recv buffer is null");
    CHECK_IF(NULL==pid, return, "pid memory is null");

    nlhdr = (struct nlmsghdr*)(skb->data);
    memcpy(recvBuf, nlmsg_data(nlhdr), nlmsg_len(nlhdr) );
    *pid = nlhdr->nlmsg_pid;

    return;
}

static void _sendMessageToUser(struct sock* socket, int pid, char* sendBuf)
{
    int msgSize            = 0;
    int sendResult         = -1;
    struct sk_buff* skb    = NULL;
    struct nlmsghdr* nlhdr = NULL;

    CHECK_IF(NULL==socket, return, "socket is null");
    CHECK_IF(NULL==sendBuf, return, "send buffer is null");

    msgSize = strlen(sendBuf);
    skb     = nlmsg_new(msgSize, 0 );
    nlhdr   = nlmsg_put(skb, 0, 0, NLMSG_DONE, msgSize, 0);

    sprintf( nlmsg_data(nlhdr), sendBuf, msgSize );
    NETLINK_CB(skb).dst_group = 0;

    sendResult =  nlmsg_unicast(socket, skb, pid);
    CHECK_IF(sendResult<0, return, "unicast a message to user failed");

    return;
}

static void _processNetlinkMessage(struct sk_buff *skb)
{
    int pid = -1;

    CHECK_IF(NULL==skb, return, "skb is null");

    memset(_rxBuffer, 0, MAX_PAYLOAD);
    _recvUserMessage(skb, _rxBuffer, &pid);

    dprint("received pid=%d's message : %s", pid, _rxBuffer);

    memset(_rxBuffer, 0, MAX_PAYLOAD);
    sprintf(_rxBuffer, "Hello from Kernel");
    _sendMessageToUser(_socket, pid, _rxBuffer);

    return;
}

static struct sock* _createNetlinkSocket(struct net *net, int unit,
                                         unsigned int groups,
                                         void (*input)(struct sk_buff* skb),
                                         struct mutex *cb_mutext,
                                         struct module *module)
{
    struct netlink_kernel_cfg cfg = {
        .groups   = groups,
        .input    = input,
        .cb_mutex = cb_mutext,
        .bind     = NULL,
    };

    return netlink_kernel_create(net, unit, &cfg);
}

static int __init taco_initModule(void)
{
    _socket = _createNetlinkSocket(&init_net, NETLINK_TEST,
                                   0, _processNetlinkMessage,
                                   NULL, THIS_MODULE);
    CHECK_IF(NULL==_socket, return -1, "Module Insert Failed");

    dprint("ok");
    return 0;
}

static void __exit taco_exitModule(void)
{
    netlink_kernel_release(_socket);
    dprint("ok");
    return;
}

module_init( taco_initModule );
module_exit( taco_exitModule );
