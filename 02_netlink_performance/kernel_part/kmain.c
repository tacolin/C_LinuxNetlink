#include "taco_kernel.h"

MODULE_AUTHOR("tacolin");
MODULE_DESCRIPTION("NETLINK KERNEL TEST MODULE");
MODULE_LICENSE("GPL");

static struct sock* _socket = NULL;

static unsigned char _rxBuffer[MAX_PAYLOAD] = {};

static void recv_msg_from_user(struct sk_buff *skb, char* recvBuf, int *pid)
{
    struct nlmsghdr *nlhdr = (struct nlmsghdr*)(skb->data);

    memcpy(recvBuf, nlmsg_data(nlhdr), nlmsg_len(nlhdr) );

    *pid = nlhdr->nlmsg_pid;
}

static void send_msg_to_user(struct sock* socket, int pid, char* sendBuf)
{
    int msgSize = 0;
    int sendResult = -1;
    struct sk_buff *skb;
    struct nlmsghdr *nlhdr;

    msgSize = strlen(sendBuf);
    skb = nlmsg_new( msgSize, 0 );
    nlhdr = nlmsg_put(skb, 0, 0, NLMSG_DONE, msgSize, 0);

    sprintf( nlmsg_data(nlhdr), sendBuf, msgSize );
    NETLINK_CB(skb).dst_group = 0;

    sendResult =  nlmsg_unicast(socket, skb, pid);
    CHECK_IF(sendResult < 0, return, "unicast a message to user failed");
}

static void process_user_msg(struct sk_buff *skb)
{
    int pid = -1;

    memset(_rxBuffer, 0, MAX_PAYLOAD);
    recv_msg_from_user(skb, _rxBuffer, &pid);

    dprint("received pid=%d's message : %s", pid, _rxBuffer);

    memset(_rxBuffer, 0, MAX_PAYLOAD);
    sprintf(_rxBuffer, "Hello from Kernel");
    send_msg_to_user(_socket, pid, _rxBuffer);
}

static struct sock* netlink_create_wrapper(struct net *net, int unit, unsigned int groups, void (*input)(struct sk_buff* skb), struct mutex *cb_mutext, struct module *module)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
    derror("do not support linux kernel version less than 3.5");
    return NULL;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
    return netlink_kernel_create(net, unit, groups, input, cb_mutext, module);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
    struct netlink_kernel_cfg cfg;
    cfg.groups = groups;
    cfg.input = input;
    cfg.cb_mutex = cb_mutext;
    cfg.bind = NULL;

    return netlink_kernel_create(net, unit, module, &cfg);
#else
    struct netlink_kernel_cfg cfg;
    cfg.groups = groups;
    cfg.input = input;
    cfg.cb_mutex = cb_mutext;
    cfg.bind = NULL;

    return netlink_kernel_create(net, unit, &cfg);
#endif
}

static int __init taco_initModule(void)
{
    _socket = netlink_create_wrapper(&init_net, NETLINK_TEST, 0, process_user_msg, NULL, THIS_MODULE);
    CHECK_IF(NULL == _socket, return -1, "Module Insert Failed");

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
