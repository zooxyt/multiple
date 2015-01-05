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

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

/* Multiple-Threading */
#if defined(UNIX)
#include <pthread.h>
#elif defined(WINDOWS)
#include <windows.h>
#include <process.h>
#endif

#if defined(UNIX)
/*typedef pthread_mutex_t mutex_t;*/
typedef pthread_spinlock_t mutex_t;
#elif defined(WINDOWS)
typedef CRITICAL_SECTION mutex_t;
#else 
typedef int mutex_t;
#endif

/* Spin Lock */
int thread_mutex_init(mutex_t *mutex);
int thread_mutex_lock(mutex_t *lock);
int thread_mutex_unlock(mutex_t *lock);
int thread_mutex_uninit(mutex_t *mutex);

/* Atomic */
void atomic_inc(volatile int *num);
void atomic_dec(volatile int *num);

#endif

