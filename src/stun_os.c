/*
 * OS interface used by STUN/TURN
 *
 * Mutex functions:
 *   CriticalSetions are used for Win32 as this is considerably faster than Win32 Mutex
 *   pthread_mutex used for linux
 */

#include "stun_os.h"

bool Stun_MutexCreate(STUN_MUTEX *m, char *name)
{
#ifdef __WINDOWS__
    InitializeCriticalSection(m);
    return true;
#else
    return (pthread_mutex_init(m, NULL) == 0);
#endif
}

bool Stun_MutexLock(STUN_MUTEX *m)
{
#ifdef __WINDOWS__
    EnterCriticalSection(m);
    return true;
#else
    return (pthread_mutex_lock(m) == 0);
#endif
}

bool Stun_MutexUnlock(STUN_MUTEX *m)
{
#ifdef __WINDOWS__
    LeaveCriticalSection(m);
    return true;
#else
    return (pthread_mutex_unlock(m) == 0);
#endif
}

bool Stun_MutexDestroy(STUN_MUTEX *m)
{
#ifdef __WINDOWS__
    DeleteCriticalSection(m);
    return true;
#else
    return (pthread_mutex_destroy(m) == 0);
#endif
}


