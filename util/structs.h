#include "so_scheduler.h"

enum state {
	RDY,
	RUN,
	TERM,
	DONE,
	WAIT
};

typedef struct my_thread {
	enum state state;
	pthread_t tid;
	unsigned int priority;
	so_handler *func;
	pthread_cond_t cond;
	unsigned int clk;
	int waiting_io;
} my_thread;

typedef struct node {
	my_thread *data;
	struct node *next;
} node;

typedef struct queue {
	int size;
	node *head;
	node *tail;
} queue;

typedef struct th_list {
	my_thread **threads;
	int no_of_th;
} th_list;

typedef struct {
	pthread_mutex_t mutex;
	unsigned int sched_quantum;
	unsigned int sched_io;
	my_thread *running;
	th_list waiting;
	queue **ready;
	th_list all;
} my_scheduler;