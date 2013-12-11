CC = gcc
CFLAGS = -ggdb -Wall -pthread
LATEX = pdflatex

all : shmalloc.o tests tests-basic

tests: tests.o shmalloc.o
	$(CC) $(CFLAGS) -o tests shmalloc.o tests.o

tests-basic: tests-basic.o shmalloc.o
	$(CC) $(CFLAGS) -o tests-basic shmalloc.o tests-basic.o

shmalloc.o: shmalloc.c shmalloc.h
	$(CC) $(CFLAGS) -c shmalloc.c

tests.o: tests.c
	$(CC) $(CFLAGS) -c tests.c

tests-basic.o: tests-basic.c
	$(CC) $(CFLAGS) -c tests-basic.c

readme: README.ltx
	$(LATEX) README.ltx 2>&1

clean :
	@rm -f *.o *.a
	@rm -f tests tests-basic
	@rm -f README.log README.dvi README.aux
