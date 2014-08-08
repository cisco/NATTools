

#include "permissions.h"

static const uint32_t PERMISSIONS_THREAD_CTX = 1;

void sendPermissionsAll(struct turn_info *turnInfo) 
{
    int i;
    const struct sockaddr *perm[MAX_PERMISSIONS];
    
    if(turnInfo->turnAlloc_44.turnPerm.numPermissions > 0){
        for (i=0;i<turnInfo->turnAlloc_44.turnPerm.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnAlloc_44.turnPerm.permissions[i];
        }
        turnInfo->turnAlloc_44.turnPerm.ok = false;

        TurnClient_StartCreatePermissionReq(turnInfo->turnAlloc_44.tInst,
                                            turnInfo->turnAlloc_44.turnPerm.numPermissions,
                                            perm);

    }

    
    if(turnInfo->turnAlloc_46.turnPerm.numPermissions > 0){
        for (i=0;i<turnInfo->turnAlloc_46.turnPerm.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnAlloc_46.turnPerm.permissions[i];
        }
        
        
        turnInfo->turnAlloc_46.turnPerm.ok = false;
        TurnClient_StartCreatePermissionReq(turnInfo->turnAlloc_46.tInst,
                                            turnInfo->turnAlloc_46.turnPerm.numPermissions,
                                            perm);

    }
    
    if(turnInfo->turnAlloc_64.turnPerm.numPermissions > 0){
        
        for (i=0;i<turnInfo->turnAlloc_64.turnPerm.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnAlloc_64.turnPerm.permissions[i];
        }
        turnInfo->turnAlloc_64.turnPerm.ok = false;
        TurnClient_StartCreatePermissionReq(turnInfo->turnAlloc_64.tInst,
                                            turnInfo->turnAlloc_64.turnPerm.numPermissions,
                                            perm);
        
    }
    
    if(turnInfo->turnAlloc_66.turnPerm.numPermissions > 0){
        
        for (i=0;i<turnInfo->turnAlloc_66.turnPerm.numPermissions;i++){
            perm[i] = (struct sockaddr *)&turnInfo->turnAlloc_66.turnPerm.permissions[i];
        }
    
        turnInfo->turnAlloc_66.turnPerm.ok = false;
        TurnClient_StartCreatePermissionReq(turnInfo->turnAlloc_66.tInst,
                                            turnInfo->turnAlloc_66.turnPerm.numPermissions,
                                            perm);
        
    }
      

}
