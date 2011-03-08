

#include "permissions.h"

static const uint32_t PERMISSIONS_THREAD_CTX = 1;

void sendPermissionsAll(struct turn_info *turnInfo) 
{
    int i;
    struct sockaddr *perm[MAX_PERMISSIONS];



    if(turnInfo->turnPerm_44.numPermissions > 0){
        for (i=0;i<turnInfo->turnPerm_44.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnPerm_44.permissions[i];
        }
    

        TurnClient_StartCreatePermissionReq(PERMISSIONS_THREAD_CTX,
                                            turnInfo->turnAlloc_44.stunCtx,
                                            turnInfo->turnPerm_44.numPermissions,
                                            perm);

    }


    if(turnInfo->turnPerm_46.numPermissions > 0){
        for (i=0;i<turnInfo->turnPerm_46.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnPerm_46.permissions[i];
        }
    

        TurnClient_StartCreatePermissionReq(PERMISSIONS_THREAD_CTX,
                                            turnInfo->turnAlloc_46.stunCtx,
                                            turnInfo->turnPerm_46.numPermissions,
                                            perm);

    }

    if(turnInfo->turnPerm_64.numPermissions > 0){
        for (i=0;i<turnInfo->turnPerm_64.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnPerm_64.permissions[i];
        }
    

        TurnClient_StartCreatePermissionReq(PERMISSIONS_THREAD_CTX,
                                            turnInfo->turnAlloc_64.stunCtx,
                                            turnInfo->turnPerm_64.numPermissions,
                                            perm);

    }

    if(turnInfo->turnPerm_66.numPermissions > 0){
        for (i=0;i<turnInfo->turnPerm_66.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnPerm_66.permissions[i];
        }
    

        TurnClient_StartCreatePermissionReq(PERMISSIONS_THREAD_CTX,
                                            turnInfo->turnAlloc_66.stunCtx,
                                            turnInfo->turnPerm_66.numPermissions,
                                            perm);

    }
        

}
