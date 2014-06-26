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


#define fifoIncrementToNext(index) (index = (index + 1) % ICELIB_MAX_FIFO_ELEMENTS)


void ICELIB_fifoClear(ICELIB_TRIGGERED_FIFO *fifo)
{
    memset(fifo, 0, sizeof(*fifo));
}


unsigned int ICELIB_fifoCount(const ICELIB_TRIGGERED_FIFO *fifo)
{
    if (fifo->isFull)
        return ICELIB_MAX_FIFO_ELEMENTS;

    if (fifo->inIndex >= fifo->outIndex) {
        return fifo->inIndex - fifo->outIndex;
    } else {
        return ICELIB_MAX_FIFO_ELEMENTS - (fifo->outIndex - fifo->inIndex);
    }
}


bool ICELIB_fifoIsEmpty(const ICELIB_TRIGGERED_FIFO *fifo)
{
    return (fifo->inIndex == fifo->outIndex) && !fifo->isFull;
}


bool ICELIB_fifoIsFull(const ICELIB_TRIGGERED_FIFO *fifo)
{
    return fifo->isFull;
}


//
//----- Return: true  - no more room, fifo is full!!
//              false - element was inserted into fifo
//
bool ICELIB_fifoPut(ICELIB_TRIGGERED_FIFO *fifo, ICELIB_FIFO_ELEMENT element)
{
    if (ICELIB_fifoIsFull(fifo))
        return true;

    fifo->elements[ fifo->inIndex] = element;
    fifoIncrementToNext(fifo->inIndex);
    if (fifo->inIndex == fifo->outIndex)
        fifo->isFull = true;

    return false;
}


//
//----- Return: element               - fifo was not empty
//              ICELIB_FIFO_IS_EMPTY  - fifo is empty!!
//
ICELIB_FIFO_ELEMENT ICELIB_fifoGet(ICELIB_TRIGGERED_FIFO *fifo)
{
    unsigned int outPreIndex;

    if (ICELIB_fifoIsEmpty(fifo))
        return ICELIB_FIFO_IS_EMPTY;

    fifo->isFull = false;
    outPreIndex   = fifo->outIndex;
    fifoIncrementToNext(fifo->outIndex);

    return fifo->elements[ outPreIndex];
}


void ICELIB_fifoIteratorConstructor(ICELIB_TRIGGERED_FIFO_ITERATOR *iterator,
                                    ICELIB_TRIGGERED_FIFO          *fifo)
{
    iterator->fifo = fifo;
    iterator->index = fifo->outIndex;
    iterator->atEnd = false;
}


ICELIB_FIFO_ELEMENT *pICELIB_fifoIteratorNext(ICELIB_TRIGGERED_FIFO_ITERATOR *iterator)
{
    ICELIB_FIFO_ELEMENT *element = NULL;

    if (ICELIB_fifoIsEmpty(iterator->fifo))
        return NULL;

    if (iterator->atEnd)
        return NULL;

    if (ICELIB_fifoIsFull(iterator->fifo)) {
        element = &iterator->fifo->elements[ iterator->index];
        fifoIncrementToNext(iterator->index);
        if (iterator->index == iterator->fifo->inIndex) {
            iterator->atEnd = true;
        }
    } else {
        if (iterator->index == iterator->fifo->inIndex) {
            iterator->atEnd = true;
            return NULL;
        }

        element = &iterator->fifo->elements[ iterator->index];
        fifoIncrementToNext(iterator->index);
    }

    return element;
}


void ICELIB_triggeredFifoClear(ICELIB_TRIGGERED_FIFO *fifo)
{
    ICELIB_fifoClear(fifo);
}


unsigned int ICELIB_triggeredFifoCount(const ICELIB_TRIGGERED_FIFO *fifo)
{
    return ICELIB_fifoCount(fifo);
}


bool ICELIB_triggeredFifoIsEmpty(const ICELIB_TRIGGERED_FIFO *fifo)
{
    return ICELIB_fifoIsEmpty(fifo);
}


bool ICELIB_triggeredFifoIsFull(const ICELIB_TRIGGERED_FIFO *fifo)
{
    return ICELIB_fifoIsFull(fifo);
}


//
//----- Return: true  - no more room, fifo is full!!
//              false - element was inserted into fifo
//
bool ICELIB_triggeredFifoPut(ICELIB_TRIGGERED_FIFO *fifo,
                             ICELIB_LIST_PAIR      *pair)
{
    return ICELIB_fifoPut(fifo, pair->pairId);
}


bool ICELIB_isTriggeredFifoPairPresent(ICELIB_TRIGGERED_FIFO *fifo,
                                       ICELIB_LIST_PAIR      *pair,
                                       ICELIB_CALLBACK_LOG   *callbackLog)
{
    uint32_t                        *pairId;
    ICELIB_TRIGGERED_FIFO_ITERATOR  tfIterator;

    ICELIB_fifoIteratorConstructor(&tfIterator, fifo);

    while ((pairId = pICELIB_fifoIteratorNext(&tfIterator)) != NULL) {
        if (*pairId == pair->pairId) {
            ICELIB_log(callbackLog, ICELIB_logDebug, "pair already present in FIFO");
            return true;
        }
    }
    return false;
}


//
//----- Return: true  - no more room, fifo is full!!
//              false - element was inserted into fifo
//
bool ICELIB_triggeredFifoPutIfNotPresent(ICELIB_TRIGGERED_FIFO *fifo,
                                         ICELIB_LIST_PAIR      *pair,
                                         ICELIB_CALLBACK_LOG   *callbackLog)
{
    if (ICELIB_isTriggeredFifoPairPresent(fifo, pair, callbackLog)) {
        return ICELIB_fifoPut(fifo, pair->pairId);
    }
    return false;
}


//
//----- Get a pair from the triggered checks queue
//
//      Get a pointer to the pair.
//      Since the fifo holds pairId's, the address of the pair is found by
//      looking up the pair in the check list by its pairId.
//
//      Return: pointer to pair   - fifo was not empty
//              NULL              - fifo is empty!!
//
ICELIB_LIST_PAIR *pICELIB_triggeredFifoGet(ICELIB_CHECKLIST      *checkList,
                                           ICELIB_CALLBACK_LOG   *callbackLog,
                                           ICELIB_TRIGGERED_FIFO *fifo)
{
    uint32_t          pairId;
    ICELIB_LIST_PAIR *pair;

    do {
        if (ICELIB_fifoIsEmpty(fifo)) {
            ICELIB_log(callbackLog, ICELIB_logDebug, "Triggered Check FIFO is empty!");
            return NULL;
        }
        pairId = ICELIB_fifoGet(fifo);
    } while (pairId == ICELIB_FIFO_ELEMENT_REMOVED);

    pair  = ICELIB_getPairById(checkList, pairId);

    if (pair == NULL) {
        ICELIB_log1(callbackLog, ICELIB_logDebug, "Could not find pair by Id: %u", pairId);
    }

    return pair;
}


void ICELIB_triggeredFifoRemove(ICELIB_TRIGGERED_FIFO *fifo,
                                ICELIB_LIST_PAIR      *pair)
{
    uint32_t                        *pairId;
    ICELIB_TRIGGERED_FIFO_ITERATOR  tfIterator;

    ICELIB_fifoIteratorConstructor(&tfIterator, fifo);

    while ((pairId = pICELIB_fifoIteratorNext(&tfIterator)) != NULL) {
        if (*pairId == pair->pairId) {
            *pairId = ICELIB_FIFO_ELEMENT_REMOVED;
        }
    }
}


void ICELIB_triggeredfifoIteratorConstructor(ICELIB_TRIGGERED_FIFO_ITERATOR *iterator,
                                             ICELIB_TRIGGERED_FIFO          *fifo)
{
    ICELIB_fifoIteratorConstructor(iterator, fifo);
}


ICELIB_LIST_PAIR *pICELIB_triggeredfifoIteratorNext(ICELIB_CHECKLIST               *checkList,
                                                    ICELIB_CALLBACK_LOG            *callbackLog,
                                                    ICELIB_TRIGGERED_FIFO_ITERATOR *iterator)
{
    uint32_t         *pairId;
    ICELIB_LIST_PAIR *pair = NULL;

    pairId = pICELIB_fifoIteratorNext(iterator);

    if (pairId != NULL) {

        pair = ICELIB_getPairById(checkList, *pairId);

        if (pair == NULL) {
            ICELIB_log1(callbackLog, ICELIB_logDebug, "Could not find pair by Id: %u", *pairId);
        }
    }

    return pair;
}
