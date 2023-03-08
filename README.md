# Thread Scheduler

## Description
The entire project stands under the copyright of University Politehnica of Bucharest, Operating Systems 2022, being a graded assignment.
The implamantation simulates a preemptive scheduler based on the Round Robin priority model, structured for an uniprocessor system. For a less exhaustive implamantation, priority is not considered like the UNIX standard model, but from one to five.
Implementation is done in a shared dynamic library, exporting the following functions:
* INIT - initializer for internal structs.
* FORK - creates a new thread.
* EXEC - simulates execution of an instruction.
* WAIT - waits for an I/O operation.
* SIGNAL - sends signal to I/O blocked threads.
* END - destroys planner and frees structs.

## Structs
### Thread struct
```
typedef struct my_thread {
	enum state state; 	// thread state : RDY, RUN, TERM, WAIT
	pthread_t tid; 		// thread id
	unsigned int priority;
	so_handler *func;	// thread specific routine
	pthread_cond_t cond; 	// thread specific condition variable
	unsigned int clk;	// time quantum clock
	int waiting_io; 	// waiting event
} my_thread;
```
### Scheduler struct 
```
typedef struct {
	pthread_mutex_t mutex; 	// general mutex
	unsigned int sched_quantum;
	unsigned int sched_io; 
	my_thread *running; 	// pointer to running thread
	th_list waiting; 	// waiting list
	queue **ready; 		// ready queues
	th_list all; 		// all thread list
} my_scheduler;
```

## Scheduler functions
Each function firstly checks validity of parameters.
Fiecare functie verifica intai corectitudinea parametrilor.
` Planning is condition-variable based . `
### SO_INIT
Structure initializer. Alocates memory and initializes mutex. Repeating this step is protected.
### SO_END
Frees memory and waits for all threads to finish execution.
### SO_FORK
Starts a new thread, initializing struct specific members. Pthread_create() is used alongside thread_start() to execute thread specific routine and manage part of the planning. The thread is added in all list and ready queue. Instruction execution is managed (being noted on caller's end -> clk++). A function is called for further planning (schedule()).
### THREAD_START 
Places the new thread waiting on execution signal. After changing its state to RUN, it will recive the signal and execute its routine. After execution, it becomes marked as terminated and a new thread will be planned.
### SCHEDULE
Plans the thread to be executed, guided by the following cases:
* rdy == NULL --> the queue is empty
* prev == NULL --> no thread in execution
* thread reached the maximum number of execution steps allowed --> it is preemted if the thread found in ready queue has higher or equal priority than the one in running; else, clk is reset and execution continues.
* priority of the ready thread is greater than the one of running thread --> preemption
* default --> continue execution
### SO_WAIT 
Thread execution is stoped changing its state to waiting on the signal given as param. Thread is added in waiting list and the next one si planned.
### SO_SIGNAL 
All threads waiting on the signal given as param are awaken and moved to ready queue.
### ADD_RDY, GET_RDY
Auxiliary functions for ready queue. Similar to classic push / pop.
