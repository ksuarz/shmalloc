CC = gcc
CFLAGS = -ggdb -Wall -pthread
LATEX = pdflatex

all : shmalloc.o tests

tests: tests.o shmalloc.o
	$(CC) $(CFLAGS) -o tests shmalloc.o tests.o

shmalloc.o: shmalloc.c shmalloc.h
	$(CC) $(CFLAGS) -c shmalloc.c

tests.o: tests.c
	$(CC) $(CFLAGS) -c tests.c

readme: README.ltx
	$(LATEX) README.ltx 2>&1

clean :
	@rm -f *.o *.a
	@rm -f tests
	@rm -f README.log README.dvi README.aux
