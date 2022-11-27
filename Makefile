CC = gcc
CFLAGS = -g -fPIC -m32 -Wall -pthread
LDFLAGS = -m32

.PHONY: build
build: libscheduler.so

libscheduler.so: scheduler.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	-rm -f *.o libscheduler.so
