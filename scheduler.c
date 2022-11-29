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

struct arg{
	so_handler *func;
	unsigned int prio;
};

void *start_thread(void *params) {
	struct arg *args = params;
	so_handler *func = args->func;
	func(args->prio);
	//pthread_exit(NULL);
	return NULL;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
	if(func == NULL  || priority > 5)
		return NULL;
	// new thread
	struct arg *new_thread = malloc(sizeof(struct arg));
	new_thread->func = func;
	new_thread->prio = priority;
	void *aux;
	pthread_t tid;
	//start_thread(new_thread);
	if(pthread_create(&tid, NULL, start_thread, (void*)new_thread) < 0)
		return INVALID_TID;
	pthread_join(tid, NULL); // work in progress
	return tid;
}

