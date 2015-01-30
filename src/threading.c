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

#include "threading.h"

int threading_create_thread(Thread_t *t, void (*fun)(void *), void *user) {
#ifdef _WIN32
    t->thread_id = CreateThread(0,0, (LPTHREAD_START_ROUTINE)fun, (LPVOID)user, 0, NULL);
    if(t->thread_id == 0)
        return AIRTAME_ERROR;
#else
    int rc;
    rc = pthread_create(&t->thread_id, NULL, (void* (*)(void*))fun, (void *)user);
    if (rc != 0)
        return AIRTAME_ERROR;
#endif
    threading_create_lock(&t->lock);
    return AIRTAME_OK;
}

void threading_create_lock(Lock_t *l) {
#ifdef _WIN32
    l->mutex = CreateMutex(0,0,0);
#else
    pthread_mutex_init(&l->mutex, NULL);
#endif
}

void threading_lock(Lock_t *l) {
#ifdef _WIN32
    while (WaitForSingleObject(l->mutex, INFINITE) != WAIT_OBJECT_0);
#else
    pthread_mutex_lock(&l->mutex);
#endif
}

void threading_unlock(Lock_t *l) {
#ifdef _WIN32
    ReleaseMutex(l->mutex);
#else
    pthread_mutex_unlock(&l->mutex);
#endif
}

void threading_cleanup_lock(Lock_t *l) {
#ifdef _WIN32
    // FIXME: locking cleanup imp on windows?
#else
    pthread_mutex_destroy(&l->mutex);
#endif
}

void threading_lock_thread(Thread_t *t) {
    threading_lock(&t->lock);
}

void threading_unlock_thread(Thread_t *t) {
    threading_unlock(&t->lock);
}

int threading_cleanup_thread(Thread_t *t) {
#ifdef _WIN32
	DWORD dwWaitResult;
	DWORD ExitCode=0;
	dwWaitResult = WaitForSingleObject(t->thread_id, 3000);
	while(dwWaitResult == WAIT_TIMEOUT)
	{
		dwWaitResult = WaitForSingleObject(t->thread_id, 10);
	}
	if(dwWaitResult == WAIT_FAILED)
		printf("WaitForSingleObject failed with error: %d\n", GetLastError());
	GetExitCodeThread(t->thread_id, &ExitCode);
	if(ExitCode == STILL_ACTIVE)
	{
		printf("ERROR Thread is STILL_ACTIVE!\n");
	}
	CloseHandle(t->thread_id);
	t->thread_id = NULL;
#else
    pthread_cancel(t->thread_id);
    pthread_join(t->thread_id, NULL);
    threading_cleanup_lock(&t->lock);
#endif
    return 0;
}
