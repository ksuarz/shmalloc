#include "shmalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 500000
#define SHM_ID 7

/**
 * Prints usage to standard out.
 */
void show_usage(void) {
    printf("Usage: tests <filename>\n");
    printf("\t<filename> must be any readable file descriptor, to be used "
            "in creating a shared memory segment.\n");
}

/**
 * Tests our implementation of `shmalloc`.
 */
int main(int argc, char **argv) {
    // TODO Kyle forgot how to attach to shared memory segments properly
    int shm_id;
    key_t shm_key;
    void *shm_ptr;

    // Lots of checks!
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments. Invoke with --help to "
                        "see proper usage.\n");
        exit(EXIT_FAILURE);
    }
    else if (strcmp(argv[1], "--h") == 0 || strcmp(argv[1], "-h") == 0) {
        show_usage();
        exit(EXIT_SUCCESS);
    }
    else if ((shm_key = ftok(argv[1], SHM_ID)) == -1) {
        fprintf(stderr, "Failed to make a key with file %s.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    else if ((shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT)) == -1) {
        fprintf(stderr, "Failed to get a shared memory segment.\n");
        exit(EXIT_FAILURE);
    }
    else if ((shm_ptr = shmat(shm_id, NULL, 0)) == (void *) -1) {
        fprintf(stderr, "Failed to attach to our shared memory segment.\n");
        exit(EXIT_FAILURE);
    }

    // Begin tests
    int x = 7;
    double *ptr;
    size_t dbl_size = sizeof(double);

    // Regular use of malloc
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    *ptr = 3.1415926535;
    shmfree(ptr);

    // Freeing a pointer not allocated by shmalloc
    shmfree(&x);
    shmfree(ptr + 1);

    // Double-free a pointer
    dbl_size = 2 * sizeof(double);
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    shmfree(ptr);
    shmfree(ptr);

    // Free, alloc, free, alloc should cause no error
    dbl_size = 5 * sizeof(double);
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    shmfree(ptr);
    dbl_size = 7 * sizeof(double);
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    shmfree(ptr);

    exit(EXIT_SUCCESS);
}
