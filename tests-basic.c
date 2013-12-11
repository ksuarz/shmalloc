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
    shmfree(ptr[0], MEM_SIZE);

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
    shmfree(ptr[0], MEM_SIZE);
    printf("Freeing ptr1\n");
    shmfree(ptr[1], MEM_SIZE);

    //Free something twice
    printf("Attempting to free ptr0 again\n");
    shmfree(ptr[0], MEM_SIZE);

    //Free ptr not returned by malloc
    printf("Allocating ptr0\n");
    ptr[0] = shmalloc(1, &size, mem, MEM_SIZE);
    printf("Freeing invalid ptr\n");
    shmfree(((char *) ptr[0] + 1), MEM_SIZE);
    printf("Freeing ptr0\n");
    shmfree(ptr[0], MEM_SIZE);




    return 0;
}
