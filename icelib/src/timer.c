/*
Copyright 2014 Cisco. All rights reserved. 

Redistribution and use in source and binary forms, with or without modification, are 
permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this list of 
      conditions and the following disclaimer. 

   2. Redistributions in binary form must reproduce the above copyright notice, this list 
      of conditions and the following disclaimer in the documentation and/or other materials 
      provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY CISCO ''AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. 

The views and conclusions contained in the software and documentation are those of the 
authors and should not be interpreted as representing official policies, either expressed 
or implied, of Cisco.
*/


#include <string.h>
#include "icelib.h"
#include "icelib_intern.h"


void ICELIB_timerConstructor(ICELIB_TIMER *timer,
                             unsigned int tickIntervalMS)
{
    memset(timer, 0, sizeof(*timer));
    timer->tickIntervalMS = tickIntervalMS;
    timer->countUpMS      = 0;
    timer->timerState     = ICELIB_timerStopped;
}


void ICELIB_timerStart(ICELIB_TIMER *timer,
                       unsigned int timeoutMS)
{
    timer->timeoutValueMS = timeoutMS;
    timer->countUpMS      = 0;
    timer->timerState     = ICELIB_timerRunning;
}


void ICELIB_timerStop(ICELIB_TIMER *timer)
{
    timer->timerState = ICELIB_timerStopped;
}


void ICELIB_timerTick(ICELIB_TIMER *timer)
{
    if (timer->timerState == ICELIB_timerRunning) {

        timer->countUpMS += timer->tickIntervalMS;

        if (timer->countUpMS >= timer->timeoutValueMS) {
            timer->timerState = ICELIB_timerTimeout;
        }
    }
}


bool ICELIB_timerIsRunning(const ICELIB_TIMER *timer)
{
    return timer->timerState == ICELIB_timerRunning;
}


bool ICELIB_timerIsTimedOut(const ICELIB_TIMER *timer)
{
    return timer->timerState == ICELIB_timerTimeout;
}
