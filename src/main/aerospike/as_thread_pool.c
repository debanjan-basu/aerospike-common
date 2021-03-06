/*
 * Copyright 2008-2015 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include "aerospike/as_thread_pool.h"
#include <citrusleaf/alloc.h>
#include <stdlib.h>
#include <unistd.h>

/******************************************************************************
 * TYPES
 *****************************************************************************/

struct as_thread_pool_task {
	as_task_fn task_fn;
	void* udata;
};

typedef struct as_thread_pool_task as_thread_pool_task;

/******************************************************************************
 * Functions
 *****************************************************************************/

void*
as_thread_worker(void* data)
{
	as_thread_pool* pool = data;
	as_thread_pool_task task;
	
	// Retrieve tasks from queue and execute.
	while (cf_queue_pop(pool->dispatch_queue, &task, CF_QUEUE_FOREVER) == CF_QUEUE_OK) {
		// A null task indicates thread should be shut down.
		if (! task.task_fn) {
			break;
		}
		
		// Run task
		task.task_fn(task.udata);
	}
	
	// Send thread completion event back to caller.
	uint32_t complete = 1;
	cf_queue_push(pool->complete_queue, &complete);
	return 0;
}

static uint32_t
as_thread_pool_create_threads(as_thread_pool* pool, uint32_t count)
{
	// Start detached threads.  There is no need to store thread because a completion queue
	// is used to join back threads on termination.
	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	
	int threads_created = 0;
	pthread_t thread;
	
	for (uint32_t i = 0; i < count; i++) {
		if (pthread_create(&thread, &attrs, as_thread_worker, pool) == 0) {
			threads_created++;
		}
	}
	return threads_created;
}

static void
as_thread_pool_shutdown_threads(as_thread_pool* pool, uint32_t count)
{
	// This tells the worker threads to stop. We do this (instead of using a
	// "running" flag) to allow the workers to "wait forever" on processing the
	// work dispatch queue, which has minimum impact when the queue is empty.
	// This also means all queued requests get processed when shutting down.
	for (uint32_t i = 0; i < count; i++) {
		as_thread_pool_task task;
		task.task_fn = NULL;
		task.udata = NULL;
		cf_queue_push(pool->dispatch_queue, &task);
	}
	
	// Wait till threads finish.
	uint32_t complete;
	for (uint32_t i = 0; i < count; i++) {
		cf_queue_pop(pool->complete_queue, &complete, CF_QUEUE_FOREVER);
	}
}

int
as_thread_pool_init(as_thread_pool* pool, uint32_t thread_size)
{
	if (pthread_mutex_init(&pool->lock, NULL)) {
		return -1;
	}

    if (pthread_mutex_lock(&pool->lock)) {
        return -2;
    }

	// Initialize queues.
	pool->dispatch_queue = cf_queue_create(sizeof(as_thread_pool_task), true);
	pool->complete_queue = cf_queue_create(sizeof(uint32_t), true);
	pool->thread_size = thread_size;
	pool->initialized = 1;
	
	// Start detached threads.
	pool->thread_size = as_thread_pool_create_threads(pool, thread_size);
	int rc = (pool->thread_size == thread_size)? 0 : -3;
	
	pthread_mutex_unlock(&pool->lock);
	return rc;
}

int
as_thread_pool_resize(as_thread_pool* pool, uint32_t thread_size)
{
    if (pthread_mutex_lock(&pool->lock)) {
        return -1;
    }

	if (! pool->initialized) {
		// Pool has already been closed.
		pthread_mutex_unlock(&pool->lock);
		return -2;
	}
	
	int rc = 0;
	
	if (thread_size != pool->thread_size) {
		if (thread_size < pool->thread_size) {
			// Shutdown excess threads.
			as_thread_pool_shutdown_threads(pool, pool->thread_size - thread_size);
			pool->thread_size = thread_size;
		}
		else {
			// Start new threads.
			pool->thread_size += as_thread_pool_create_threads(pool, thread_size - pool->thread_size);
			rc = (pool->thread_size == thread_size)? 0 : -3;
		}
	}
	pthread_mutex_unlock(&pool->lock);
	return rc;
}

int
as_thread_pool_queue_task(as_thread_pool* pool, as_task_fn task_fn, void* udata)
{
	if (pool->thread_size == 0) {
		// No threads are running to process task.
		return -1;
	}
	
	as_thread_pool_task task;
	task.task_fn = task_fn;
	task.udata = udata;
	
	if (cf_queue_push(pool->dispatch_queue, &task) != CF_QUEUE_OK) {
		return -2;
	}
	return 0;
}

int
as_thread_pool_destroy(as_thread_pool* pool)
{
	if (pthread_mutex_lock(&pool->lock)) {
		return -1;
	}

	if (! pool->initialized) {
		// Pool has already been closed.
		pthread_mutex_unlock(&pool->lock);
		return -2;
	}

	as_thread_pool_shutdown_threads(pool, pool->thread_size);
	cf_queue_destroy(pool->dispatch_queue);
	cf_queue_destroy(pool->complete_queue);
	pool->initialized = 0;
	pthread_mutex_unlock(&pool->lock);
	pthread_mutex_destroy(&pool->lock);
	return 0;
}
