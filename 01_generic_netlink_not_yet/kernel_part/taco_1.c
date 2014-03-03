#include <linux/kernel.h>
#include <linux/module.h>
#include <net/genetlink.h>

MODULE_AUTHOR("tacolin");
MODULE_DESCRIPTION("GE-NETLINK KERNEL TEST MODULE");
MODULE_LICENSE("GPL");

typedef unsigned int uint32;

static int _doEcho(struct sk_buff *pSkb, struct genl_info *pInfo);

typedef enum
{
    TACO_ATTRIBUTE_1_INTEGER,
    TACO_ATTRIBUTE_2_STRING,

    TACO_ATTRIBUTE_MAX
}
tTacoAttributes;

static struct genl_family g_kernelFamily = 
{
    .id = GENL_ID_GENERATE,
    .hdrsize = 0,
    .name = "TACO_KERNEL",
    .version = 1,
    .maxattr = TACO_ATTRIBUTE_MAX-1,
};

static struct nla_policy g_kernelAttributeCheckPolicy[TACO_ATTRIBUTE_MAX] = 
{
    [TACO_ATTRIBUTE_1_INTEGER] = { .type = NLA_U32 },
    [TACO_ATTRIBUTE_2_STRING] = { .type = NLA_NUL_STRING },
};

typedef enum
{
    TACO_OPERATION_UNSPEC,
    TACO_OPERATION_ECHO,

    TACO_OPERATION_MAX
}
tTacoOperations;

static struct genl_ops g_kernelOperationEcho = 
{
    .cmd = TACO_OPERATION_ECHO,
    .flags = 0,
    .policy = g_kernelAttributeCheckPolicy,
    .doit = _doEcho,
};


static int _parseMsgFromUseSpace(struct genl_info *pInfo)
{
    struct nlattr *pAttr1 = NULL;
    uint32 data1;
    struct nlattr *pAttr2 = NULL;
    char* pData2;

    pAttr1 = pInfo->attrs[TACO_ATTRIBUTE_1_INTEGER];
    if ( pAttr1 != NULL)
    {
        data1 = *(uint32*)nla_data(pAttr1);
        printk("[KERNEL-PART] data1 = %d\n", data1);
    }

    pAttr2 = pInfo->attrs[TACO_ATTRIBUTE_2_STRING];
    if ( pAttr2 != NULL )
    {
        pData2 = (char*)nla_data(pAttr2);
        printk("[KERNEL-PART] data2 = %s\n", pData2);
    }

    return 0;
}

static int _sendMsgToUserSpace(struct genl_info *pInfo)
{
    struct sk_buff *pSendbackSkb = NULL;
    void *pMsgHead = NULL;
    int nlaputResult;
    int sendbackResult;

    pSendbackSkb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if ( pSendbackSkb == NULL ) 
    {
        printk("[KERNEL-ERROR] sendback skb is null\n");
        return -1;
    }

    pMsgHead = genlmsg_put(pSendbackSkb, 0, pInfo->snd_seq+1, &g_kernelFamily, 0, TACO_OPERATION_ECHO);
    if ( pMsgHead == NULL )
    {
        printk("[KERNEL-ERROR] genl_put failed\n");
        return -1;
    }

    nlaputResult = nla_put_u32(pSendbackSkb, TACO_ATTRIBUTE_1_INTEGER, 9999);
    if ( nlaputResult != 0 )
    {
        printk("[KERNEL-ERROR] nla_put_u32 failed\n");
        return -1;
    }

    nlaputResult = nla_put_string(pSendbackSkb, TACO_ATTRIBUTE_2_STRING, "Hello From Kernel");
    if ( nlaputResult != 0 )
    {
        printk("[KERNEL-ERROR] nla_put_string failed\n");
        return -1;
    }

    genlmsg_end(pSendbackSkb, pMsgHead);

    sendbackResult = genlmsg_unicast(&init_net, pSendbackSkb, pInfo->snd_pid);
    if ( sendbackResult != 0 )
    {
        printk("[KERNEL-ERROR] genl_unicast failed\n");
        return -1;   
    }

    return 0;
}

static int _doEcho(struct sk_buff *pSkb, struct genl_info *pInfo)
{
    if ( pSkb == NULL )
    {
        printk("[KERNEL-ERROR] taco_doEcho : skb is null\n");
        return -1;
    }

    if ( pInfo == NULL )
    {
        printk("[KERNEL-ERROR] taco_doEcho : info is null\n");
        return -1;
    }

    if ( 0 != _parseMsgFromUseSpace(pInfo) )
    {
        printk("[KERNEL-PART] parse msg from user space failed\n");
        return -1;
    }

    if ( 0 != _sendMsgToUserSpace(pInfo) )
    {
        printk("[KERNEL-PART] send msg to user space failed\n");
        return -1;
    }   

    return 0;
}


static int __init kernel_module_init(void)
{
    int registerResult;

    registerResult = genl_register_family(&g_kernelFamily);
    if ( registerResult != 0 )
    {
        printk("[KERNEL-ERROR] Register Family failed\n");
        return -1;
    }

    registerResult = genl_register_ops(&g_kernelFamily, &g_kernelOperationEcho);
    if ( registerResult != 0 )
    {   
        printk("[KERNEL-ERROR] Register Operations failed\n");
        return -1;
    }

    printk("[KERNEL-PART] Module Inserted\n");

    return 0;
}

static void __exit kernel_module_exit(void)
{
    int unregisterResult;

    unregisterResult = genl_unregister_ops(&g_kernelFamily, &g_kernelOperationEcho);
    if ( unregisterResult != 0 )
    {
        printk("[KERNEL-ERROR] Unregister Operation failed\n");
    }

    unregisterResult = genl_unregister_family(&g_kernelFamily);
    if ( unregisterResult != 0 )
    {
        printk("[KERNEL-ERROR] Unregister Family failed\n");
    }

    printk("[KERNEL-PART] Module removed\n");

    return;
}

module_init( kernel_module_init );
module_exit( kernel_module_exit );
