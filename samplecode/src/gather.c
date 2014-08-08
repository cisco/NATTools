#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <pthread.h>


#include "turnclient.h"
#include "sockaddr_util.h"


#include "gather.h"





TURN_INSTANCE_DATA *gather(TURN_INSTANCE_DATA *tInst,
                          struct sockaddr *host_addr,
                          int requestedFamily,
                          char *user,
                          char *pass,
                          struct turn_allocation *turnAlloc){

    int stunCtx;
    //TurnCallBackData_T TurnCbData;

    

    TurnClient_StartAllocateTransaction(&tInst,
                                        50,
                                        turnInfoFunc,
                                        "IceClient",
                                        turnAlloc, // *userCtx
                                        host_addr,
                                        user,
                                        pass,
                                        requestedFamily,
                                        turnSendFunc,
                                        TurnStatusCallBack,
                                        false,
                                        0);
    
        return tInst;
}




void gatherAll(struct turn_info *turnInfo, 
               void(*update_turninfo)(void),
               struct listenConfig *listenConfig,
               void (*data_handler)(unsigned char *))
{
    int idx = 0;

    if( sockaddr_isSet((struct sockaddr *)&turnInfo->localIp4) &&
        sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp4) )
    {
        
        turnInfo->turnAlloc_44.tInst = gather(turnInfo->turnAlloc_44.tInst, 
               (struct sockaddr *)&turnInfo->remoteIp4,
               AF_INET,
               turnInfo->user,
               turnInfo->pass,
               &turnInfo->turnAlloc_44);
        
        turnInfo->turnAlloc_44.update_turninfo = update_turninfo;
        listenConfig->socketConfig[idx].tInst = turnInfo->turnAlloc_44.tInst;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_44.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        listenConfig->socketConfig[idx].realm = turnInfo->realm;
        
        idx++;
         
        turnInfo->turnAlloc_46.tInst = gather(turnInfo->turnAlloc_46.tInst, 
                                              (struct sockaddr *)&turnInfo->remoteIp4,
                                              AF_INET6,
                                              turnInfo->user,
                                              turnInfo->pass,
                                              &turnInfo->turnAlloc_46);
        turnInfo->turnAlloc_46.update_turninfo = update_turninfo;
        listenConfig->socketConfig[idx].tInst = turnInfo->turnAlloc_46.tInst;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_46.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        listenConfig->socketConfig[idx].realm = turnInfo->realm;
        
        
        idx++;
        
    }

    if( sockaddr_isSet((struct sockaddr *)&turnInfo->localIp6) &&
        sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp6) )
    {
        
        turnInfo->turnAlloc_64.tInst = gather(turnInfo->turnAlloc_64.tInst,  
                                              (struct sockaddr *)&turnInfo->remoteIp6,
                                              AF_INET,
                                              turnInfo->user,
                                              turnInfo->pass,
                                              &turnInfo->turnAlloc_64);
        turnInfo->turnAlloc_64.update_turninfo = update_turninfo;
        listenConfig->socketConfig[idx].tInst = turnInfo->turnAlloc_64.tInst;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_64.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        listenConfig->socketConfig[idx].realm = turnInfo->realm;
        idx++;

        turnInfo->turnAlloc_66.tInst = gather(turnInfo->turnAlloc_66.tInst, 
               (struct sockaddr *)&turnInfo->remoteIp6,
               AF_INET6,
               turnInfo->user,
               turnInfo->pass,
               &turnInfo->turnAlloc_66);
        turnInfo->turnAlloc_66.update_turninfo = update_turninfo;
        listenConfig->socketConfig[idx].tInst = turnInfo->turnAlloc_66.tInst;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_66.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        listenConfig->socketConfig[idx].realm = turnInfo->realm;
        idx++;
    }
        
    listenConfig->numSockets = idx;
    listenConfig->data_handler = data_handler;
}



void releaseAll(struct turn_info *turnInfo)
{
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv4) ||
        sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv6) 
    ){

        TurnClient_Deallocate(turnInfo->turnAlloc_44.tInst);
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddrIPv4) ||
        sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddrIPv6) 
    ){
        
        TurnClient_Deallocate(turnInfo->turnAlloc_46.tInst);
    }
    
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddrIPv4) ||
        sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddrIPv6) 
    ){
        
        TurnClient_Deallocate(turnInfo->turnAlloc_64.tInst);
    }
    
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv4) ||
        sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv6) 
    ){
        
        TurnClient_Deallocate(turnInfo->turnAlloc_66.tInst);
    }
    
}
