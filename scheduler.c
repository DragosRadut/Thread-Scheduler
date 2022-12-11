#include <stdio.h>
#include <stdlib.h>
#include "util/so_scheduler.h"
#include "util/structs.h"
#include <pthread.h>

static my_scheduler sch;
static int init_protect;

void *add_rdy(my_thread *new_th, int front) {
	node *new = malloc(sizeof(*new));

	new_th->state = RDY;
	new_th->clk = 0;
	new->data = new_th;
	new->next = NULL;

	int prio = new_th->priority;
	if (sch.ready[prio]->size == 0) {
		sch.ready[prio]->head = sch.ready[prio]->tail = new;
		sch.ready[prio]->size ++;
		return NULL;
	}
	// front = 1 --> add node to front
	if (front) {
		sch.ready[prio]->head->next = new;
		sch.ready[prio]->head = new;
	} else {
		new->next = sch.ready[prio]->tail;
		sch.ready[prio]->tail = new;
	}
	sch.ready[prio]->size ++;
	return NULL;
}

my_thread *get_rdy() {
	// search for highest prio
	int prio = 5;
	while (sch.ready[prio]->size == 0 && prio > 0)
		prio --;

	node *rdy = sch.ready[prio]->head;
	if (rdy == NULL)
		return NULL;
	my_thread *data = rdy->data;
	free(rdy);
	if (sch.ready[prio]->size == 1) {
		sch.ready[prio]->head = sch.ready[prio]->tail = NULL;
		sch.ready[prio]->size --;
		return data;
	}
	node *aux = sch.ready[prio]->tail;
	while (aux->next != rdy) {
		aux = aux->next;
	}
	sch.ready[prio]->head = aux;
	aux->next = NULL;
	sch.ready[prio]->size --;
	
	return data;
}

void *schedule() {
	pthread_mutex_lock (&sch.mutex);
	my_thread *prev = sch.running;
	my_thread *rdy = get_rdy();

	if (rdy == NULL) {
		pthread_mutex_unlock(&sch.mutex);
		return NULL;
	}

	if (prev == NULL) {
		rdy->state = RUN;
		sch.running = rdy;
		pthread_cond_signal(&(rdy->cond));
		pthread_mutex_unlock(&sch.mutex);
		return NULL;
	}
	
	if (sch.running->clk == sch.sched_quantum) {
		prev->clk = 0;
		if (prev->priority > rdy->priority) {
			add_rdy(rdy, 1);
			pthread_mutex_unlock(&sch.mutex);
			return NULL;
		}
		prev->state = RDY;
		add_rdy(prev, 0);
		sch.running = rdy;
		rdy->state = RUN;
		pthread_cond_signal(&(rdy->cond));
		while (prev->state != RUN)
			pthread_cond_wait(&(prev->cond), &sch.mutex);
		pthread_mutex_unlock(&sch.mutex);
		return NULL;
	}

	if (sch.running->priority < rdy->priority) {
		sch.running->clk = 0;
		prev->state = RDY;
		sch.running = rdy;
		add_rdy(prev, 0);
		rdy->state = RUN;
		pthread_cond_signal(&(rdy->cond));
		while (prev->state != RUN)
			pthread_cond_wait(&(prev->cond), &sch.mutex);
		pthread_mutex_unlock(&sch.mutex);
		return NULL;
	}

	add_rdy(rdy, 1);
	pthread_mutex_unlock(&sch.mutex);
	return NULL;
}

int so_init(unsigned int quantum, unsigned int io) {
	// check params
	if (io > SO_MAX_NUM_EVENTS || quantum <= 0 || init_protect)
		return -1;
	init_protect = 1; // protect double init

	sch.sched_quantum = quantum;
	sch.sched_io = io;
	sch.running = NULL;

	sch.ready = malloc(6 * sizeof(queue*));
	for (int i = 0; i < 6; i++) {
		sch.ready[i] = malloc(sizeof(queue));
		sch.ready[i]->size = 0;
		sch.ready[i]->head = sch.ready[i]->tail = NULL;
	}

	return pthread_mutex_init(&sch.mutex, NULL);
}

void so_end() {
	if (init_protect) {
		for (int i = 0; i < sch.all.no_of_th; i++) {
			if (sch.all.threads[i]->state != TERM)
				pthread_join(sch.all.threads[i]->tid, NULL);
			pthread_cond_destroy(&(sch.all.threads[i]->cond));
			free(sch.all.threads[i]);
		}
		free(sch.all.threads);
	
		for (int i = 0; i < 6; i++) {
			node *aux = sch.ready[i]->tail;
			while (aux) {
				node *temp = aux;
				aux = aux->next;
				free(temp);
			}
			free(sch.ready[i]);
		}
		free(sch.ready);

		for (int i = 0; i < sch.waiting.no_of_th; i++)
			free(sch.waiting.threads[i]);
		free(sch.waiting.threads);
	}
	
	init_protect = 0;
	pthread_mutex_destroy(&(sch.mutex));
}

int so_wait(unsigned int io) {
	if (io >= sch.sched_io || io < 0)
		return -1;

	pthread_mutex_lock(&sch.mutex);
	// add to waiting list
	sch.running->waiting_io = io;
	if (sch.waiting.no_of_th == 0) {
		sch.waiting.threads = malloc(sizeof(my_thread *));
	} else {
		my_thread **temp = realloc(sch.waiting.threads, sizeof(my_thread *) * (sch.waiting.no_of_th + 1));
		sch.waiting.threads = temp;
	}
	sch.waiting.threads[sch.waiting.no_of_th] = sch.running;
	sch.waiting.no_of_th++;

	// switch running thread
	my_thread *prev = sch.running;
	my_thread *rdy = get_rdy();

	sch.running->clk = 0;
	prev->state = WAIT;
	sch.running = rdy;
	rdy->state = RUN;
	pthread_cond_signal(&(rdy->cond));
	while (prev->state != RUN)
		pthread_cond_wait(&(prev->cond), &sch.mutex);
	pthread_mutex_unlock(&sch.mutex);

	return 0;
}

int so_signal(unsigned int io) {
	if (io >= sch.sched_io || io < 0)
		return -1;

	sch.running->clk++;

	int cnt = 0;
	for (int i = 0; i < sch.waiting.no_of_th; i++) {
		if (sch.waiting.threads[i] && sch.waiting.threads[i]->waiting_io == io) {
			cnt ++;
			add_rdy(sch.waiting.threads[i], 0);
			sch.waiting.threads[i] = NULL;
		}
	}
	schedule();
	return cnt;
}

// using pthread_create documentation model
static void *thread_start(void *arg) {
	my_thread *th_arg = arg;
	so_handler *func = th_arg->func;

	// context switch
	pthread_mutex_lock(&sch.mutex);
	while (th_arg->state != RUN)	
		pthread_cond_wait(&(th_arg->cond), &sch.mutex);
	sch.running = th_arg;
	pthread_mutex_unlock(&sch.mutex);

	func(th_arg->priority);

	pthread_mutex_lock(&sch.mutex);
	th_arg->state = TERM;
	sch.running = NULL;
	pthread_mutex_unlock(&sch.mutex);

	schedule();
	return NULL;
}


tid_t so_fork(so_handler *func, unsigned int priority) {
	if (func == NULL  || priority > 5)
		return INVALID_TID;

	// new thread
	my_thread *new_thread = malloc(sizeof(*new_thread));
	new_thread->func = func;
	new_thread->priority = priority;
	new_thread->clk = 0;
	pthread_cond_init(&(new_thread->cond), NULL);

	// create -> tid = id; NULL = def attr; thread_start func; func params
	if (pthread_create(&(new_thread->tid), NULL, thread_start, (void*)new_thread) < 0)
		return INVALID_TID;

	// add to thread list
	pthread_mutex_lock(&sch.mutex);
	if (sch.all.no_of_th == 0) {
		sch.all.threads = malloc(sizeof(my_thread *));
	} else {
		my_thread **temp = realloc(sch.all.threads, sizeof(my_thread *) * (sch.all.no_of_th + 1));
		sch.all.threads = temp;
	}
	sch.all.threads[sch.all.no_of_th] = new_thread;
	sch.all.no_of_th++;

	add_rdy(new_thread, 0); 
	if (sch.running)
		sch.running->clk ++;
	pthread_mutex_unlock(&sch.mutex);
	schedule();

	return new_thread->tid;
}

void so_exec () {
	sch.running->clk ++;
	if (sch.running->clk == sch.sched_quantum)
		schedule();
	return;
}