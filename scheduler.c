#include <stdio.h>
#include <stdlib.h>
#include "util/so_scheduler.h"
#include "util/structs.h"
#include <pthread.h>

static my_scheduler sch;
static init_protect = 0; 

int so_init(unsigned int quantum, unsigned int io) {
	if(io > SO_MAX_NUM_EVENTS || quantum <= 0 || init_protect)
		return -1;
	init_protect = 1;
	sch.sched_quantum = quantum;
	sch.sched_io = io;
	return pthread_mutex_init(&sch.mutex, NULL);
}

void so_end() {
	init_protect = 0;
}

void so_exec() {

}

int so_wait(unsigned int io) {

}

int so_signal(unsigned int io) {

}

typedef struct {
	so_handler *func;
	unsigned int prio;
} thread_info;

// using pthread_create documentation model
static void *thread_start(void *arg) {
	thread_info *th_arg = arg;
	so_handler *func = th_arg->func;
	func(th_arg->prio);
	free(th_arg);
	return NULL;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
	if(func == NULL  || priority > 5)
		return NULL;
	
	// create and pass thread info
	thread_info *new_info = malloc(sizeof(*new_info));
	pthread_t tid;
	new_info->func = func;
	new_info->prio = priority;
	// create -> tid = id; NULL = def attr; thread_start func; func params
	if(pthread_create(&tid, NULL, thread_start, (void*)new_info) < 0)
		return INVALID_TID;
	
	// new thread
	my_thread *new_thread = malloc(sizeof(*new_thread));
	new_thread->tid = tid;
	new_thread->func = func;
	new_thread->priority = priority;
	
	if(sch.running == NULL)
		pthread_join(tid, NULL); // w8 for thread tid to terminate
	free(new_thread);
	return tid;
}

