#ifndef TURNHELPER_H
#define TURNHELPER_H

#include <turnclient.h>

typedef enum {IDLE, RUNNING, SUCCESS, ERROR} state_t;

struct turnData{
    
    int sockfd;
    char *turn_uri;
    char *turn_port;
    char *user;
    char *password;
    
    
    
    state_t turn_state;
    struct sockaddr_storage active_turn_server;
    struct sockaddr_storage rflx;
    struct sockaddr_storage relay;
    TURN_INSTANCE_DATA *instData;

};


struct mediaStream{
    struct sockaddr_storage localIPv4;
    struct sockaddr_storage localIPv6;
    struct turnData *tData_44;
    struct turnData *tData_46;
    struct turnData *tData_64;
    struct turnData *tData_66;
};


void turnCbFunc(void *userCtx, TurnCallBackData_T *turnCbData);

static void *handleTurnResponse(void *ptr);

int gatherCandidates(struct turnData *tData);

static void turnSendFunc(const uint8_t         *buffer,
                         size_t                bufLen,
                         const struct sockaddr *dstAddr,
                         void                  *userCtx);


static void *tickTurn( void *ptr);


static void turnInfoFunc(void *userCtx, TurnInfoCategory_T category, char *ErrStr);


#endif
