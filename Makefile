CC = gcc
CFLAGS = -g -fPIC -Wall -pthread
LDFLAGS =

.PHONY: build
build: libscheduler.so

libscheduler.so: scheduler.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	-rm -f *.o libscheduler.so
