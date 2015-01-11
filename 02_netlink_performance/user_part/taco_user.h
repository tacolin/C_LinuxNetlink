#ifndef _TACO_USER_H_
#define _TACO_USER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/socket.h>

#define NETLINK_TEST 17
#define MAX_PAYLOAD  4096

#define dprint(a, b...) printf("%s(): "a"\n", __func__, ##b)
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
//_TACO_USER_H_
