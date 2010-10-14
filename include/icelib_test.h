#ifndef ICELIB_TEST_H
#define ICELIB_TEST_H

void ICELIB_timerConstructor( ICELIB_TIMER *pTimer, unsigned int tickIntervalMS);
void ICELIB_timerStart(       ICELIB_TIMER *pTimer, unsigned int timeoutMS);
void ICELIB_timerTick(        ICELIB_TIMER *pTimer);
bool ICELIB_timerIsRunning(   ICELIB_TIMER *pTimer);
bool ICELIB_timerIsTimedOut(  ICELIB_TIMER *pTimer);

#endif
