# Thread Scheduler

## Descriere
Prin implementarea planificatorului se simuleaza un scheduler preemptiv intr-un sistem uniprocesor bazat pe modelul Round Robin cu prioritati. Implementarea este realizata intr-o biblioteca partajata dinamica, ce exporta urmatoarele functii:
* INIT - inițializează structurile interne ale planificatorului.
* FORK - pornește un nou thread.
* EXEC - simulează execuția unei instrucțiuni.
* WAIT - așteaptă un eveniment/operație I/O.
* SIGNAL - semnalează threadurile care așteaptă un eveniment/operație I/O.
* END - distruge planificatorul și eliberează structurile alocate.

## Structs
### Thread struct
```
typedef struct my_thread {
	enum state state; // thread state : RDY, RUN, TERM, WAIT
	pthread_t tid; // thread id
	unsigned int priority;
	so_handler *func; // thread specific routine
	pthread_cond_t cond; // thread specific condition variable
	unsigned int clk; // time quantum clock
	int waiting_io; // waiting event
} my_thread;
```
### Scheduler struct 
```
typedef struct {
	pthread_mutex_t mutex; // general mutex
	unsigned int sched_quantum;
	unsigned int sched_io; 
	my_thread *running; // pointer to running thread
	th_list waiting; // waiting list
	queue **ready; // ready queues
	th_list all; // all thread list
} my_scheduler;
```

## Scheduler functions
Fiecare functie verifica intai corectitudinea parametrilor.
` Planificarea are la baza utilizarea variabilelor de conditie. `
### SO_INIT
Initializeaza membrii structurii de scheduler, aloca memoria necesara si initializeaza mutex-ul. Se evita initializarea multipla.
### SO_END
Elibereaza memoria alocata anterior si asteapta terminarea tuturor thread-urilor.
### SO_FORK
Porneste un nou thread, initializand membrii specifici structurii. Se foloseste pthread_create() impruna cu functia thread_start() care va executa rutina specifica fiecarui thread si va asigura planificarea. Thread-ul se adauga in lista all si in coada ready. Se noteaza executarea unei intructiuni (clk++ -> modificare adusa caller-ului functiei) si se apeleaza functia responsabila cu planificarea (schedule()).
### THREAD_START 
Trece thread-ul in asteptarea semnalui de executie. Dupa ce va trece in starea RUN, va primi semnalul si va executa rutina. In urma executiei, va fi marcat ca si terminat si se va planifica un nou thread.
### SCHEDULE
Planifica urmatorul thread ce trebuie executat, tratand urmatoarele cazuri:
* rdy == NULL --> coada e goala
* prev == NULL --> nu exista niciun thread in executie
* thread-ul a atins numarul maxim de executii permise --> se preempteaza in cazul in care thread-ul scos din ready are prioritatea mai mare sau egala decat cel din running; altfel, se reseteaza timpul si se continua executia
* prioritatea thread-ului din ready este mai mare decat cea a thread-ului din running --> preeempt
* altfel --> se continua executia





