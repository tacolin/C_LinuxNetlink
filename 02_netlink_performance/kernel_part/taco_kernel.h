#ifndef _TACO_KERNEL_H_
#define _TACO_KERNEL_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>

#define NETLINK_TEST 17
#define MAX_PAYLOAD  4096

#define dprint(a, b...) printk("%s(): "a"\n", __func__, ##b)
#define derror(a, b...) dprint("[ERROR] "a, ##b)

#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define FN_APPLY_ALL(type, fn, ...) \
{\
    void* _stopPoint = (int[]){0};\
    void** _listForApplyAll = (type[]){__VA_ARGS__, _stopPoint};\
    int i;\
    for (i=0; _listForApplyAll[i] != _stopPoint; i++)\
    {\
        fn(_listForApplyAll[i]);\
    }\
}


#endif
//_TACO_KERNEL_H_
