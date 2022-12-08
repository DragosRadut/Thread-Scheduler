#include <stdio.h>
#include <stdlib.h>
#include "util/so_scheduler.h"
#include "util/structs.h"
#include <pthread.h>

static my_scheduler sch;
static int init_protect = 0; 
static int is_first_thread = 1;
queue **ready;
queue *waiting;
my_thread *running = NULL;
static int check = 0;
th_list all;

void *add_rdy(my_thread *new_th, int front) {
	node* new = malloc(sizeof(*new));
	new_th->state = RDY;
	new_th->clk = 0;
	new->data = new_th;
	new->next = NULL;

	int prio = new_th->priority;
	if(ready[prio]->size == 0) {
		ready[prio]->head = ready[prio]->tail = new;
		ready[prio]->size ++;
		return NULL;
	}
	
	if(front) {
		ready[prio]->head->next = new;
		ready[prio]->head = new;
	} else {
		new->next = ready[prio]->tail;
		ready[prio]->tail = new;
	}
	ready[prio]->size ++;
	return NULL;
}

my_thread *get_rdy() { //TODO: remove prio
	// search for highest prio
	int prio = 5;
	while(ready[prio]->size == 0 && prio > 0)
		prio --;

	node *rdy = ready[prio]->head;
	if(rdy == NULL) {
		return NULL;
	}
	my_thread *data = rdy->data;
	free(rdy);
	if(ready[prio]->size == 1) {
		ready[prio]->head = ready[prio]->tail = NULL;
		ready[prio]->size --;
		return data;
	}
	node *aux = ready[prio]->tail;
	while(aux->next != rdy) {
		aux = aux->next;
	}
	ready[prio]->head = aux;
	aux->next = NULL;
	ready[prio]->size --;
	
	return data;
}


void *schedule() {
	my_thread *prev = running;
	my_thread *ready = get_rdy();
	if(ready != NULL && ready != prev) {
		pthread_mutex_lock(&sch.mutex);
		check = 1;
		running = ready;
		pthread_cond_signal(&(ready->cond));
		if(prev != NULL && prev != ready && prev->state != TERM) {
			pthread_cond_wait(&(prev->cond), &sch.mutex);
			// check--;
		}
		pthread_mutex_unlock(&sch.mutex);
	} 
	return NULL;
}

void check_preempt() {
	my_thread *next = get_rdy();
	if(next) {
		if(running->clk == sch.sched_quantum) {
			running->clk = 0;
			running->has_preempt = 1;
			add_rdy(running, 0);
			add_rdy(next, 1);
			schedule();
		} else if(running->priority < next->priority) { //preempt
			running->has_preempt = 1;
			running->clk = 0;
			add_rdy(running, 0);
			add_rdy(next, 1); // add back to front
			
			schedule();
		} 
		else {
			add_rdy(next, 1);
		} 
	}
}

int so_init(unsigned int quantum, unsigned int io) {
	// check params
	if(io > SO_MAX_NUM_EVENTS || quantum <= 0 || init_protect)
		return -1;
	init_protect = 1; // protect double init

	// init mutex
	sch.sched_quantum = quantum;
	sch.sched_io = io;
	waiting = malloc(sizeof(queue *));
	ready = malloc(6 * sizeof(queue*));
	for(int i = 0; i < 6; i++) {
		ready[i] = malloc(sizeof(queue));
		ready[i]->size = 0;
		ready[i]->head = ready[i]->tail = NULL;
	}
	return pthread_mutex_init(&sch.mutex, NULL);
}

void so_end() {
	if(init_protect) { 
		for(int i = all.no_of_th - 1; i >= 0; i--) {
			if(all.threads[i]->state != TERM) 
				pthread_join(all.threads[i]->tid, NULL);
			pthread_cond_destroy(&(all.threads[i]->cond));
			free(all.threads[i]);
		}
		for(int i = 0; i < 6; i++) {
			node *aux = ready[i]->tail;
			while(aux) {
				node *temp = aux;
				aux = aux->next;
				free(temp);
			}
			free(ready[i]);
		}
		free(ready);
		free(waiting);
	}
	
	init_protect = 0;
	pthread_mutex_destroy(&(sch.mutex));

}

int so_wait(unsigned int io) {
	if(io >= sch.sched_io || io < 0)
		return -1;

	node* new = malloc(sizeof(node*));
	running->waiting_io = io;
	new->data = running;
	new->next = NULL;

	if(waiting->size == 0) {
		waiting->head = waiting->tail = new;
	} else {
		new->next = waiting->tail;
		waiting->tail = new;
	} 
	waiting->size ++;

	running = NULL;
	schedule();
	return 0;
}

int so_signal(unsigned int io) {
	if(io >= sch.sched_io || io < 0)
		return -1;
	
	running->clk++;
	
	int cnt = 0;
	node *aux = waiting->tail;
	node *temp;
	for(int i = 0; i < waiting->size; i++) {
		if(aux->data->waiting_io == io) {
			cnt ++;
			temp = aux;
			add_rdy(aux->data, 0);
			aux = aux->next;
			free(temp);
		}
	}
	check_preempt();
	return cnt;
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
	}
	pthread_mutex_unlock(&sch.mutex);

	th_arg->state = RUN;
	running = th_arg;
	
	is_first_thread = 0;

	func(th_arg->priority);

	th_arg->state = TERM;
	running = NULL;
	
	schedule();

	pthread_exit(NULL);
}



tid_t so_fork(so_handler *func, unsigned int priority) {
	if(func == NULL  || priority > 5)
		return INVALID_TID;

	// new thread
	my_thread *new_thread = malloc(sizeof(*new_thread));
	new_thread->func = func;
	new_thread->priority = priority;
	new_thread->clk = 0;
	pthread_cond_init(&(new_thread->cond), NULL);


	// create -> tid = id; NULL = def attr; thread_start func; func params
	if(pthread_create(&(new_thread->tid), NULL, thread_start, (void*)new_thread) < 0)
		return INVALID_TID;
	
	// add to thread list
	if(all.no_of_th == 0) {
		all.threads = malloc(sizeof(my_thread *));
	} else {
		my_thread **temp = realloc(all.threads, sizeof(my_thread *) * (all.no_of_th + 1));
		all.threads = temp;
	}
	all.threads[all.no_of_th] = new_thread;
	all.no_of_th++;

	if(is_first_thread)
		pthread_join(new_thread->tid, NULL);
	else {
		add_rdy(new_thread, 0); 
		running->clk ++;
		check_preempt();
	}
	return new_thread->tid;
}

void so_exec() {
	running->clk ++;
	if(running->clk >= sch.sched_quantum) {
		check_preempt();
	}
	return;
}