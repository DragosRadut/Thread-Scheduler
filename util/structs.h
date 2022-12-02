#include "so_scheduler.h"

typedef struct {
	pthread_t tid;
	unsigned int priority;
	so_handler *func;
	pthread_cond_t cond; 
	unsigned int clk;
} my_thread;

typedef struct node {
	my_thread* data;
	struct node* next;
} node;

typedef struct queue {
	int size;
	node* head;
	node* tail;
} queue;

typedef struct {
	pthread_mutex_t mutex;
	unsigned int sched_quantum; //cuanta de timp
	unsigned int sched_io;
	my_thread *running;
	queue waiting;
} my_scheduler;