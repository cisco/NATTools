/*
Copyright 2011 Cisco. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CISCO ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Cisco.
*/

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


