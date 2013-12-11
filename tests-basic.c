#include "shmalloc.h"
#include <stdio.h>
#include <string.h>

#define MEM_SIZE 50000

int main(int argc, char *argv[])
{
    char mem[MEM_SIZE];
    size_t size = 10;
    void *ptr[10];

    memset(mem, 0, MEM_SIZE);

    //Basic usage of malloc
    printf("Allocating ptr0\n");
    ptr[0] = shmalloc(1, &size, mem, MEM_SIZE);
    printf("Freeing ptr0\n");
    shmfree(ptr[0], MEM_SIZE, mem);

    //Allocate 2 things
    printf("Allocating ptr0\n");
    ptr[0] = shmalloc(1, &size, mem, MEM_SIZE);

    printf("Allocating ptr1\n");
    ptr[1] = shmalloc(1, &size, mem, MEM_SIZE);
    //Checking that size is right
    if(size != 10)
    {
        fprintf(stderr, "Size returned from shmalloc is %lu, expected 10\n", (unsigned long) size);
        return 1;
    }
    printf("Freeing ptr0\n");
    shmfree(ptr[0], MEM_SIZE, mem);
    printf("Freeing ptr1\n");
    shmfree(ptr[1], MEM_SIZE, mem);

    //Free something twice
    printf("Attempting to free ptr0 again\n");
    shmfree(ptr[0], MEM_SIZE, mem);

    //Free ptr not returned by malloc
    printf("Allocating ptr0\n");
    ptr[0] = shmalloc(1, &size, mem, MEM_SIZE);
    printf("Freeing invalid ptr\n");
    shmfree(((char *) ptr[0] + 1), MEM_SIZE, mem);
    printf("Freeing ptr0\n");
    shmfree(ptr[0], MEM_SIZE, mem);

    //Allocate super big thing
    printf("Allocating more than we have\n");
    size = MEM_SIZE - sizeof(Header) + 10;
    ptr[0] = shmalloc(2, &size, mem, MEM_SIZE);
    if(ptr[0] != NULL)
    {
        fprintf(stderr, "Failed test to allocate more than we have.\n");
        return 1;
    }

    //Two references
    printf("Testing multiple references\n");
    size = sizeof(int);
    ptr[0] = shmalloc(9, &size, mem, MEM_SIZE);
    *((int *)ptr[0]) = 5;

    ptr[1] = shmalloc(9, &size, mem, MEM_SIZE);

    if(*((int *)ptr[1]) != 5)
    {
        fprintf(stderr, "Multiple refernces did not work. Got %d, expecting 5\n", *((int *)ptr[1]));
        return 1;
    }
    shmfree(ptr[0], MEM_SIZE, mem);
    shmfree(ptr[1], MEM_SIZE, mem);



    return 0;
}
