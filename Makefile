CC = gcc
CFLAGS = -ggdb -Wall -pthread

all : shmalloc.o tests

tests: tests.o shmalloc.o
	$(CC) $(CFLAGS) -o tests shmalloc.o tests.o

shmalloc.o: shmalloc.c shmalloc.h
	$(CC) $(CFLAGS) -c shmalloc.c

tests.o: tests.c
	$(CC) $(CFLAGS) -c tests.c

clean :
	@rm -f *.o *.a
	@rm -f tests
