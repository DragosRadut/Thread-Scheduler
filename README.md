# Thread Scheduler

## Descriere
Prin implementarea planificatorului se simuleaza un scheduler preemptiv intr-un sistem uniprocesor bazat pe modelul Round Robin cu prioritati. Implementarea este realizata intr-o biblioteca partajata dinamica, ce exporta urmatoarele functii:
* INIT - inițializează structurile interne ale planificatorului.
* FORK - pornește un nou thread.
* EXEC - simulează execuția unei instrucțiuni.
* WAIT - așteaptă un eveniment/operație I/O.
* SIGNAL - semnalează threadurile care așteaptă un eveniment/operație I/O.
* END - distruge planificatorul și eliberează structurile alocate.
