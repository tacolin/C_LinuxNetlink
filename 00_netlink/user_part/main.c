#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/socket.h>

#define NETLINK_TEST 17
#define MAX_PAYLOAD  512

int main(int argc, char* argv[])
{
    int socketFd;
    socketFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);

    struct sockaddr_nl srcAddr;
    memset( &srcAddr, 0, sizeof(srcAddr) );
    srcAddr.nl_family = AF_NETLINK;
    srcAddr.nl_pid    = getpid();  
    srcAddr.nl_groups = 0;
    bind( socketFd, (struct sockaddr*)&srcAddr, sizeof(srcAddr) );

    struct sockaddr_nl dstAddr;
    memset( &dstAddr, 0, sizeof(dstAddr) );
    dstAddr.nl_family = AF_NETLINK;
    dstAddr.nl_pid    = 0;
    dstAddr.nl_groups = 0;

    struct nlmsghdr *pAllocatedNlhdr;
    pAllocatedNlhdr = (struct nlmsghdr*)malloc( NLMSG_SPACE(MAX_PAYLOAD) );
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
    free(pAllocatedNlhdr); // 用完以後馬上就 free

    char localBufNlhdr[ NLMSG_SPACE(MAX_PAYLOAD) ];
    memset( localBufNlhdr, 0, NLMSG_SPACE(MAX_PAYLOAD) );
    iov.iov_base = (void*)localBufNlhdr; // 把記憶體位置換掉

    recvmsg( socketFd, &msg, 0 );
    printf("[USER-PART] Receive message from kernel : %s\n", (char*)NLMSG_DATA(localBufNlhdr) );

    close(socketFd);

    return 0;
}
