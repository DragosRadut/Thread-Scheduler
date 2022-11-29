#include "so_scheduler.h"

typedef struct {
	pthread_t tid;
	unsigned int priority;
	so_handler *func;
} my_thread;

typedef struct {
	struct node *next;
	my_thread *val;
} node;

typedef struct {
	node *head;
	node *tail;
} queue;

typedef struct {
	pthread_mutex_t mutex;
	unsigned int sched_quantum; //cuanta de timp
	unsigned int sched_io;
	my_thread *running;
	queue waiting;
	queue ready[6];
} my_scheduler;