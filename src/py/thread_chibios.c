#include "Python.h"
#include "pythread.h"
#include "ch.h"

/*
 * Thread support.
 */
void
PyThread__init_thread(void)
{
}
 
long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    Thread *thread = chThdCreateFromHeap(NULL, 65536, NORMALPRIO, (tfunc_t)func, arg);
    return thread != NULL;
}

long
PyThread_get_thread_ident(void)
{
    return (long)chThdSelf();
}

void
PyThread_exit_thread(void)
{
    chThdExit(0);
}

/*
 * Lock support. Uses ChibiOS binary semaphores.
 */
PyThread_type_lock
PyThread_allocate_lock(void)
{
    BinarySemaphore *lock;
    lock = (BinarySemaphore *)PyMem_RawMalloc(sizeof(BinarySemaphore));

    if (lock) chBSemInit(lock, 0);

    return (PyThread_type_lock) lock;
}

void
PyThread_free_lock(PyThread_type_lock lock)
{
    PyMem_RawFree((void *)lock);
}

int
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
    return PyThread_acquire_lock_timed(lock, waitflag ? -1 : 0, 0);
}


PyLockStatus
PyThread_acquire_lock_timed(PyThread_type_lock lock, PY_TIMEOUT_T microseconds,
                            int intr_flag)
{
    int success;
    systime_t timeout_value;
    if (microseconds == 0) {
        timeout_value = TIME_IMMEDIATE;
    } else if (microseconds < 0) {
        timeout_value = TIME_INFINITE;
    } else {
        timeout_value = (((long long) microseconds) * CH_FREQUENCY) / microseconds;
    }
    
    msg_t result = chBSemWaitTimeout((BinarySemaphore*) lock, timeout_value);

    return result == RDY_OK;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
    chBSemSignal((BinarySemaphore*) lock);
}


