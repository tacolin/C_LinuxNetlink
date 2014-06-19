#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>

MODULE_AUTHOR("tacolin");
MODULE_DESCRIPTION("NETLINK KERNEL TEST MODULE");
MODULE_LICENSE("GPL");

#define NETLINK_TEST 17
#define MAX_PAYLOAD  512

static struct sock* g_pSocket = NULL;

static void recv_msg_from_user(struct sk_buff *pSkb, char* pRecvBuf, int *pPid)
{
    struct nlmsghdr *pNlhdr = (struct nlmsghdr*)(pSkb->data);

    memcpy(pRecvBuf, nlmsg_data(pNlhdr), strlen( (char*)nlmsg_data(pNlhdr) ) );

    *pPid = pNlhdr->nlmsg_pid;
}

static void send_msg_to_user(struct sock* pSocket, int pid, char* pSendBuf)
{
    int msgSize = 0;
    int sendResult = -1;
    struct sk_buff *pSkb;
    struct nlmsghdr *pNlhdr;

    msgSize = strlen(pSendBuf);
    pSkb = nlmsg_new( msgSize, 0 );
    pNlhdr = nlmsg_put(pSkb, 0, 0, NLMSG_DONE, msgSize, 0);

    sprintf( nlmsg_data(pNlhdr), pSendBuf, msgSize );
    NETLINK_CB(pSkb).dst_group = 0;

    sendResult =  nlmsg_unicast(pSocket, pSkb, pid);
    if ( sendResult < 0 )
    {
        printk("[KERNEL-PART] unicast a message to user failed\n");
    }
}

static void process_user_msg(struct sk_buff *pSkb) 
{
    int pid = -1;
    char pMsgBuf[MAX_PAYLOAD];

    memset(pMsgBuf, 0, MAX_PAYLOAD);
    recv_msg_from_user(pSkb, pMsgBuf, &pid);

    printk("[KERNEL-PART] received pid=%d's message : %s\n", pid, pMsgBuf);

    memset(pMsgBuf, 0, MAX_PAYLOAD);
    sprintf(pMsgBuf, "Hello from Kernel");
    send_msg_to_user(g_pSocket, pid, pMsgBuf);
}

static struct sock* netlink_create_wrapper(struct net *pNet, int unit, unsigned int groups, void (*input)(struct sk_buff* skb), struct mutex *pCb_mutex, struct module *pModule)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
    return netlink_kernel_create(&init_net, unit, groups, input, pCb_mutex, pModule);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
    struct netlink_kernel_cfg cfg;
    cfg.groups = groups;
    cfg.input = input;
    cfg.cb_mutex = pCb_mutex;
    cfg.bind = NULL;
   
    return netlink_kernel_create(&init_net, unit, pModule, &cfg);
#else
    struct netlink_kernel_cfg cfg;
    cfg.groups = groups;
    cfg.input = input;
    cfg.cb_mutex = pCb_mutex;
    cfg.bind = NULL;
   
    return netlink_kernel_create(&init_net, unit, &cfg);
#endif 
}

static int __init kernel_module_init(void)
{
    g_pSocket = netlink_create_wrapper(&init_net, NETLINK_TEST, 0, process_user_msg, NULL, THIS_MODULE);

    printk("[KERNEL-PART] Module Inserted\n");

    return 0;
}

static void __exit kernel_module_exit(void)
{
    netlink_kernel_release(g_pSocket);

    printk("[KERNEL-PART] Module removed\n");

    return;
}

module_init( kernel_module_init );
module_exit( kernel_module_exit );
