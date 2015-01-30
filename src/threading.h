/*
 * Copyright (C) 2015 AIRTAME
 *
 * This file is part of airtme-cli.
 *
 *   Airtame-cli is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Airtame-cli is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Airtame-cli.  If not, see <http://www.gnu.org/licenses/>.
 *
*/

#ifndef __H_LOCKING__
#define __H_LOCKING__
/* Cross platform threading and locking abstraction */

#include "error.h"

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif


typedef struct {
    /* Win specific locks */
#ifdef _WIN32
    HANDLE mutex;
    /* *nix specific */
#else
    pthread_mutex_t mutex;
#endif
} Lock_t;

typedef struct {
    Lock_t lock;
#ifdef _WIN32
    HANDLE thread_id;
#else
    pthread_t thread_id;
#endif
} Thread_t;


int threading_create_thread(Thread_t *t, void (*fun)(void *), void *user);
void threading_create_lock(Lock_t *l);
void threading_lock(Lock_t *t);
void threading_unlock(Lock_t *t);
void threading_cleanup_lock(Lock_t *t);
void threading_lock_thread(Thread_t *t);
void threading_unlock_thread(Thread_t *t);
int threading_cleanup_thread(Thread_t *t);


#endif
