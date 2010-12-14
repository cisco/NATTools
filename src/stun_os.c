/*
 * OS interface used by STUN/TURN
 *
 * Mutex functions:
 *   CriticalSetions are used for Win32 as this is considerably faster than Win32 Mutex
 *   TTOS_mutex_xxx (=pthread_mutex) are used for linux
 *   TTOS_Semaphore is used for Nucleus+ as this
 *   does not support nested locks e.g. the following fails:  lock(a); lock(b); unlock(b) unlock(a);
 */

#include "stun_os.h"

bool Stun_MutexCreate(STUN_MUTEX *m, char *name)
{
#ifdef __WINDOWS__
    InitializeCriticalSection(m);
    return true; /* TBD */
#elif defined(ENV_PLUS)
    return TTOS_semaphoreCreate(m, name, 1);
#else
    return TTOS_mutexCreate(m, name);
#endif
}

bool Stun_MutexLock(STUN_MUTEX *m)
{
#ifdef __WINDOWS__
    EnterCriticalSection(m);
    return true; /* TBD */
#elif defined(ENV_PLUS)
    return (TTOS_semaphoreObtain(m, TTOS_SUSPEND) >= 0);
#else
    TTOS_mutexLock(m);
    return true;
#endif
}

bool Stun_MutexUnlock(STUN_MUTEX *m)
{
#ifdef __WINDOWS__
    LeaveCriticalSection(m);
    return true; /* TBD */
#elif defined(ENV_PLUS)
    return (TTOS_semaphoreRelease(m));
#else
    TTOS_mutexUnlock(m);
    return true;
#endif
}


