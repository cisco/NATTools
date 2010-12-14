/*
 * OS interface used by STUN/TURN
 *
 */

#ifndef STUN_OS_H
#define STUN_OS_H

#ifndef  __WINDOWS__
#if defined(ENV_PLUS)
#include "ttos_semaphore.h"
#else
#include "ttos_mutex.h"
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


#if defined(__WINDOWS__)
    typedef CRITICAL_SECTION STUN_MUTEX;
#elif defined (ENV_PLUS)
    /* nucleus+ .. */
    typedef  TTOS_SEMAPHORE STUN_MUTEX;  /* had to use a semaphore as mutex on necleus+ does not work with > 1 mutex !!! */
#else
    /* linux, .. */
    typedef  TTOS_MUTEX STUN_MUTEX;
#endif



bool Stun_MutexCreate(STUN_MUTEX *m, char *name);
bool Stun_MutexLock  (STUN_MUTEX *m);
bool Stun_MutexUnlock(STUN_MUTEX *m);


#ifdef __cplusplus
}
#endif


#endif /* STUN_OS_H */

