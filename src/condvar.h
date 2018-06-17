/*
 * Portable condition variable support for windows and pthreads.
 * Everything is inline, this header can be included where needed.
 *
 * APIs generally return 0 on success and non-zero on error,
 * and the caller needs to use its platform's error mechanism to
 * discover the error (errno, or GetLastError())
 *
 * Note that some implementations cannot distinguish between a
 * condition variable wait time-out and successful wait. Most often
 * the difference is moot anyway since the wait condition must be
 * re-checked.
 * PyCOND_TIMEDWAIT, in addition to returning negative on error,
 * thus returns 0 on regular success, 1 on timeout
 * or 2 if it can't tell.
 *
 * There are at least two caveats with using these condition variables,
 * due to the fact that they may be emulated with Semaphores on
 * Windows:
 * 1) While PyCOND_SIGNAL() will wake up at least one thread, we
 *    cannot currently guarantee that it will be one of the threads
 *    already waiting in a PyCOND_WAIT() call.  It _could_ cause
 *    the wakeup of a subsequent thread to try a PyCOND_WAIT(),
 *    including the thread doing the PyCOND_SIGNAL() itself.
 *    The same applies to PyCOND_BROADCAST(), if N threads are waiting
 *    then at least N threads will be woken up, but not necessarily
 *    those already waiting.
 *    For this reason, don't make the scheduling assumption that a
 *    specific other thread will get the wakeup signal
 * 2) The _mutex_ must be held when calling PyCOND_SIGNAL() and
 *    PyCOND_BROADCAST().
 *    While e.g. the posix standard strongly recommends that the mutex
 *    associated with the condition variable is held when a
 *    pthread_cond_signal() call is made, this is not a hard requirement,
 *    although scheduling will not be "reliable" if it isn't.  Here
 *    the mutex is used for internal synchronization of the emulated
 *    Condition Variable.
 */

#ifndef _CONDVAR_H_
#define _CONDVAR_H_

#include "ch.h"

#define Py_HAVE_CONDVAR

/* The following functions return 0 on success, nonzero on error 
 *
 * Since the ChibiOS functions return void, use the comma operator to return 0
 */
#define PyMUTEX_T Mutex
#define PyMUTEX_INIT(mut)       (chMtxInit(mut), 0)
#define PyMUTEX_FINI(mut)       (0)
#define PyMUTEX_LOCK(mut)       (chMtxLock(mut), 0)
#define PyMUTEX_UNLOCK(mut)     (chMtxUnlock(), 0)

#define PyCOND_T CondVar
#define PyCOND_INIT(cond)       (chCondInit(cond), 0)
#define PyCOND_FINI(cond)       (0)
#define PyCOND_SIGNAL(cond)     (chCondSignal(cond), 0)
#define PyCOND_BROADCAST(cond)  (chCondBroadcast(cond), 0)
#define PyCOND_WAIT(cond, mut)  (chCondWait(cond), 0)

/* return 0 for success, 1 on timeout, -1 on error */
static inline int
PyCOND_TIMEDWAIT(PyCOND_T *cond, PyMUTEX_T *mut, long long us)
{
    msg_t r = chCondWaitTimeout(cond, (us * CH_FREQUENCY)/1000000LL);

    if (r == RDY_TIMEOUT)
        return 1;
    else if (r==RDY_OK)
        return 0;
    else
        return -1;
}

#endif /* _CONDVAR_H_ */
