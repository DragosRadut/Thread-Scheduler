#include <stdio.h>
#include <stdlib.h>
#include "util/so_scheduler.h"
#include "util/structs.h"
#include <pthread.h>

static my_scheduler sch;
static int init_protect = 0; 
static int is_first_thread = 1;
queue **ready;
my_thread *running = NULL;
static int check = 0;

int so_init(unsigned int quantum, unsigned int io) {
	// check params
	if(io > SO_MAX_NUM_EVENTS || quantum <= 0 || init_protect)
		return -1;
	init_protect = 1; // protect double init

	// init mutex
	sch.sched_quantum = quantum;
	sch.sched_io = io;
	ready = malloc(6 * sizeof(queue*));
	for(int i = 0; i < 6; i++) {
		ready[i] = malloc(sizeof(queue));
		ready[i]->size = 0;
		ready[i]->head = ready[i]->tail = NULL;
	}
	return pthread_mutex_init(&sch.mutex, NULL);
}

void so_end() {
	init_protect = 0;
	pthread_mutex_destroy(&(sch.mutex));

}

int so_wait(unsigned int io) {

}

int so_signal(unsigned int io) {

}

void *add_rdy(my_thread *new_th) {
	node* new = malloc(sizeof(node*));
	new_th->clk = 0;
	new->data = new_th;
	
	int prio = new_th->priority;
	if(ready[prio]->size == 0) {
		ready[prio]->head = ready[prio]->tail = new;
		ready[prio]->size ++;
		return NULL;
	}
	
	new->next = ready[prio]->tail;
	ready[prio]->tail = new;
	ready[prio]->size ++;
	return NULL;
}

my_thread *get_rdy(int prio) { //TODO: remove prio
	node *rdy = ready[prio]->head;
	if(rdy == NULL) {
		return NULL;
	}

	if(ready[prio]->size == 1) {
		ready[prio]->head = ready[prio]->tail = NULL;
		ready[prio]->size --;
		return rdy->data;
	}
	node *aux = ready[prio]->tail;
	while(aux->next != rdy) {
		aux = aux->next;
	}
	ready[prio]->head = aux;
	aux->next = NULL;
	ready[prio]->size --;
	return rdy->data;
}


void *schedule() {
	my_thread *prev = running;
	my_thread *ready = get_rdy(0);
	if(ready != NULL && ready != prev) {
		pthread_mutex_lock(&sch.mutex);
		check ++;
		running = ready;
		pthread_cond_signal(&(ready->cond));
		if(prev != NULL && prev != ready) {
			pthread_cond_wait(&(prev->cond), &sch.mutex);
			
		}
		pthread_mutex_unlock(&sch.mutex);
	} 
	return NULL;
}

// using pthread_create documentation model
static void *thread_start(void *arg) {
	my_thread *th_arg = arg;
	so_handler *func = th_arg->func;

	// context switch
	pthread_mutex_lock(&sch.mutex);
	if(!is_first_thread) {
		while(check == 0)
			pthread_cond_wait(&(th_arg->cond), &sch.mutex);
		check --;
	} else is_first_thread = 0;
	pthread_mutex_unlock(&sch.mutex);

	running = th_arg;
	func(th_arg->priority);
	running = NULL;

	schedule();

	return NULL;
}



tid_t so_fork(so_handler *func, unsigned int priority) {
	if(func == NULL  || priority > 5)
		return NULL;

	pthread_t tid;

	// new thread
	my_thread *new_thread = malloc(sizeof(*new_thread));
	new_thread->tid = tid;
	new_thread->func = func;
	new_thread->priority = priority;
	new_thread->clk = 0;
	pthread_cond_init(&(new_thread->cond), NULL);

	// create -> tid = id; NULL = def attr; thread_start func; func params
	if(pthread_create(&tid, NULL, thread_start, (void*)new_thread) < 0)
		return INVALID_TID;
	
	if(is_first_thread) {
		pthread_join(tid, NULL); // w8 for created threads
	} else {
		add_rdy(new_thread);
		
	}
	
	//pthread_cond_destroy(&(new_thread->cond));
	//free(new_thread);
	return tid;
}

void so_exec() {
	running->clk ++;
	if(running->clk == sch.sched_quantum) {
		running->clk = 0;
		add_rdy(running);
		schedule();
	}
}