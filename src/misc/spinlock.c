/* Spin Lock
 * Copyright(C) 2013-2014 Cheryl Natsu

 * This file is part of multiple - Multiple Paradigm Language Emulator

 * multiple is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * multiple is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#include <stdio.h>

#ifndef UNIX
#ifndef WINDOWS
#if defined(linux)
#define UNIX
#elif defined(_MSC_VER)
#define WINDOWS
#endif
#endif
#endif

/* Multiple-Threading */
#if defined(UNIX)
#include <pthread.h>
#elif defined(WINDOWS)
#include <windows.h>
#include <process.h>
#endif

#include "spinlock.h"

/* Initialize Mutex */
#if defined(WINDOWS)
int thread_mutex_init(CRITICAL_SECTION *mutex)
{
    int ret = 0;
    if (InitializeCriticalSectionAndSpinCount(mutex, 4000) == FALSE) ret = -1;
    return ret;
}
#elif defined(UNIX)
int thread_mutex_init(mutex_t *mutex)
{
    return pthread_spin_init(mutex, 0);
}
#else
int thread_mutex_init(int *mutex)
{
    *mutex = 0;
    return 0;
}
#endif

/* Uninitialize Mutex */
#if defined(WINDOWS)
int thread_mutex_uninit(CRITICAL_SECTION *mutex)
{
    DeleteCriticalSection(mutex);
    return 0;
}
#elif defined(UNIX)
int thread_mutex_uninit(mutex_t *mutex)
{
    return pthread_spin_destroy(mutex);
}
#else
int thread_mutex_uninit(int *mutex)
{
    (void)mutex;
    return 0;
}
#endif

/* Mutex lock */
#if defined(UNIX)
int thread_mutex_lock(mutex_t *lock)
{
    return pthread_spin_lock(lock);
}
#elif defined(WINDOWS)
int thread_mutex_lock(CRITICAL_SECTION *lock)
{
    EnterCriticalSection(lock);
    return 0;
}
#else
int thread_mutex_lock(int *lock)
{
    while (*lock != 0);
    *lock = 1;
    return 0;
}
#endif

/* Mutex unlock */
#if defined(UNIX)
int thread_mutex_unlock(mutex_t *lock)
{
    return pthread_spin_unlock(lock);
}
#elif defined(WINDOWS)
int thread_mutex_unlock(CRITICAL_SECTION *lock)
{
    LeaveCriticalSection(lock);
    return 0;
}
#else
int thread_mutex_unlock(int *lock)
{
    *lock = 0;
    return 0;
}
#endif


/* Atomic */

void atomic_inc(volatile int *num)
{
#if (defined(__GNUC__)||defined(__x86_64__)||defined(__x86__))
    __asm__ __volatile__ ( "lock incl %0" : "=m" (*num));
#else
    (*num)++;
#endif

}

void atomic_dec(volatile int *num)
{
#if (defined(__GNUC__)||defined(__x86_64__)||defined(__x86__))
    __asm__ __volatile__ ( "lock decl %0" : "=m" (*num));
#else
    (*num)--;
#endif
}

