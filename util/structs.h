#include "so_scheduler.h"
#define RDY 0
#define RUN 1
#define TERM -1
typedef struct my_thread{
	int state;
	pthread_t tid;
	unsigned int priority;
	so_handler *func;
	pthread_cond_t cond; 
	unsigned int clk;
	struct my_thread **child;
	int waiting_io;
	int has_preempt;
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

typedef struct th_list {
	my_thread **threads;
	int no_of_th;
} th_list;