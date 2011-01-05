/*
 * OS interface used by STUN/TURN
 *
 */

#ifndef STUN_OS_H
#define STUN_OS_H

#include <stdbool.h>

#ifndef  __WINDOWS__
#include <pthread.h>
#else
#include <windows.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__WINDOWS__)
    typedef CRITICAL_SECTION STUN_MUTEX;
#else
    /* linux, .. */
    typedef pthread_mutex_t STUN_MUTEX;
#endif

bool Stun_MutexCreate(STUN_MUTEX *m, char *name);
bool Stun_MutexLock  (STUN_MUTEX *m);
bool Stun_MutexUnlock(STUN_MUTEX *m);
bool Stun_MutexDestroy(STUN_MUTEX *m);


#ifdef __cplusplus
}
#endif


#endif /* STUN_OS_H */
