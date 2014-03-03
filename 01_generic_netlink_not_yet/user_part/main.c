#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/genetlink.h>

#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))

typedef enum
{
    TACO_OPERATION_UNSPEC,
    TACO_OPERATION_ECHO,

    TACO_OPERATION_MAX
}
tTacoOperations;

typedef enum
{
    TACO_ATTRIBUTE_1_INTEGER,
    TACO_ATTRIBUTE_2_STRING,

    TACO_ATTRIBUTE_MAX
}
tTacoAttributes;

typedef struct _tSelfDefinedMsg
{
    struct nlmsghdr nlhdr;
    struct genlmsghdr genlhdr;
    char buf[256];
}
tSelfDefinedMsg;

typedef unsigned int uint32;

static int _createGenericNetlinkSocket(void)
{
    int socketFd;
    socketFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

    struct sockaddr_nl srcAddr;
    memset( &srcAddr, 0, sizeof(srcAddr) );
    srcAddr.nl_family = AF_NETLINK;
    srcAddr.nl_pid    = getpid();  
    srcAddr.nl_groups = 0;
    bind( socketFd, (struct sockaddr*)&srcAddr, sizeof(srcAddr) );

    return socketFd;
}

static int _sendQueryRequest(int socketFd, char* pFamilyName)
{
    tSelfDefinedMsg request;
    struct nlattr *pAttr = NULL;

    memset(&request, 0, sizeof(tSelfDefinedMsg) );
    
    request.nlhdr.nlmsg_type = GENL_ID_CTRL;
    request.nlhdr.nlmsg_flags = NLM_F_REQUEST;
    request.nlhdr.nlmsg_seq = 0;
    request.nlhdr.nlmsg_pid = getpid();
    request.nlhdr.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    
    request.genlhdr.cmd = CTRL_CMD_GETFAMILY;
    request.genlhdr.version = 1;

    pAttr = (struct nlattr*)GENLMSG_DATA(&request);
    pAttr->nla_type = CTRL_ATTR_FAMILY_NAME;
    pAttr->nla_len = strlen( pFamilyName ) + 1 + NLA_HDRLEN;
    strcpy( NLA_DATA(pAttr), pFamilyName );

    request.nlhdr.nlmsg_len += NLMSG_ALIGN( pAttr->nla_len );

    struct sockaddr_nl srcAddr;
    memset( &srcAddr, 0, sizeof(srcAddr) );
    srcAddr.nl_family = AF_NETLINK;

    if ( 0 >= sendto(socketFd, &request, request.nlhdr.nlmsg_len, 0, (struct sockaddr*)&srcAddr, sizeof(srcAddr)) )
    {
        printf("[USER-ERROR] query family id : sendto kerenl failed\n");
        return -1;
    }

    return 0;
}

static int _recvAndParseQueryResponse(int socketFd)
{
    tSelfDefinedMsg response;
    struct nlattr *pRspAttr = NULL;
    int recvLen;
    int familyId = -1;

    recvLen = recv( socketFd, &response, sizeof(response), 0 );
    if ( recvLen <= 0 ) 
    {
        printf("[USER-ERROR] query family id : recv from kernel failed\n");
        return -1;
    }

    if ( !NLMSG_OK( &(response.nlhdr), recvLen  ) )
    {
        printf("[USER-ERROR] query family id : invalid reponse\n");
        return -1;
    }
    
    if ( response.nlhdr.nlmsg_type == NLMSG_ERROR )
    {
        printf("[USER-ERROR] query family id : response type is error\n");
        return -1;
    }

    pRspAttr = (struct nlattr*)GENLMSG_DATA(&response);
    pRspAttr = (struct nlattr*)( (char*)(pRspAttr) + NLA_ALIGN(pRspAttr->nla_len) );
    if ( pRspAttr->nla_type == CTRL_ATTR_FAMILY_ID ) 
    {
        familyId = *(__u16 *)NLA_DATA(pRspAttr);
    }

    return familyId;
}

static int _queryKernelFamilyId(int socketFd, char* pFamilyName)
{
    int familyId;

    _sendQueryRequest(socketFd, pFamilyName);

    familyId = _recvAndParseQueryResponse(socketFd);
    return familyId;    
}

static int _sendDataToTacoKernel(int socketFd, int familyId)
{
    tSelfDefinedMsg req;
    memset(&req, 0, sizeof(tSelfDefinedMsg));
    req.nlhdr.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    req.nlhdr.nlmsg_type = familyId;
    req.nlhdr.nlmsg_flags = NLM_F_REQUEST;
    req.nlhdr.nlmsg_seq = 100;
    req.nlhdr.nlmsg_pid = getpid();

    req.genlhdr.cmd = TACO_OPERATION_ECHO;

    struct nlattr* pAttr;
    pAttr = (struct nlattr*)GENLMSG_DATA(&req);
    pAttr->nla_type = TACO_ATTRIBUTE_1_INTEGER;
    pAttr->nla_len = sizeof(uint32) + NLA_HDRLEN;
    *(uint32*)NLA_DATA(pAttr) = 123;

    req.nlhdr.nlmsg_len += NLMSG_ALIGN(pAttr->nla_len);

    pAttr = (struct nlattr*)( (void*)pAttr + NLMSG_ALIGN(pAttr->nla_len) );
    pAttr->nla_type = TACO_ATTRIBUTE_2_STRING;
    pAttr->nla_len = strlen("Hello from User") + 1 + NLA_HDRLEN;
    strcpy( NLA_DATA(pAttr), "Hello from User");

    req.nlhdr.nlmsg_len += NLMSG_ALIGN(pAttr->nla_len);

    struct sockaddr_nl dstAddr;
    memset( &dstAddr, 0, sizeof(dstAddr) );
    dstAddr.nl_family = AF_NETLINK;

    sendto( socketFd, &req, req.nlhdr.nlmsg_len, 0, (struct sockaddr*)&dstAddr, sizeof(dstAddr) );

    return 0;

}

static int _recvAndParseDataFromTacoKernel(int socketFd)
{
    tSelfDefinedMsg response;
    int recvLen;

    recvLen = recv(socketFd, &response, sizeof(response), 0);
    if ( !NLMSG_OK( &(response.nlhdr), recvLen ) )
    {
        printf("[USER-ERROR] recv invalid data from kernel\n");
        return -1;
    }

    if ( response.nlhdr.nlmsg_type == NLMSG_ERROR )
    {
        printf("[USER-ERROR] received nlmsg type is error\n");
        return -1;
    }

    struct nlattr* pAttr;
    pAttr = (struct nlattr*)GENLMSG_DATA(&response);
    int data1 = *(uint32*)NLA_DATA(pAttr);
    printf("[USER-PART] data1 = %d\n", data1);

    pAttr = (struct nlattr*)( (void*)pAttr + pAttr->nla_len );
    char *pData2 = (char*)NLA_DATA(pAttr);
    printf("[USER-PART] data2 = %s\n", pData2);

    return 0;
}


int main(int argc, char* argv[])
{
    int socketFd = _createGenericNetlinkSocket();
    if ( socketFd < 0 )   
    {
        printf("[USER-ERROR] create generic netlink failed\n");
        return -1;
    }   
    
    int familyId = _queryKernelFamilyId(socketFd, "TACO_KERNEL");
    if ( familyId < 0 )
    {
        printf("[USER-ERROR] query kernel family id failed\n");
        return -1;
    }    

    _sendDataToTacoKernel(socketFd, familyId);
    _recvAndParseDataFromTacoKernel(socketFd);

    close(socketFd);

    return 0;
}
