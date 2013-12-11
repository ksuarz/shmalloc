#include "shmalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 500000
#define SHM_ID 7

/**
 * Prints usage to standard out.
 */
void show_usage(void) {
    printf("Usage: tests <filename>\n");
    printf("<filename> must be any readable file descriptor, to be used "
            "in creating a shared memory segment.\n");
}

/**
 * Tests our implementation of `shmalloc`.
 */
int main(int argc, char **argv) {
    int shm_id;
    key_t shm_key;
    void *shm_ptr;

    // Lots of checks!
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments. Invoke with --help to "
                        "see proper usage.\n");
        exit(EXIT_FAILURE);
    }
    else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        show_usage();
        exit(EXIT_SUCCESS);
    }
    else if ((shm_key = ftok(argv[1], SHM_ID)) == -1) {
        fprintf(stderr, "Failed to make a key with file %s.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    else if ((shm_id = shmget(shm_key, SHM_SIZE, 0777 | IPC_CREAT)) == -1) {
        fprintf(stderr, "Failed to get a shared memory segment.\n");
        exit(EXIT_FAILURE);
    }
    else if ((shm_ptr = shmat(shm_id, NULL, 0)) == (void *) -1) {
        fprintf(stderr, "Failed to attach to our shared memory segment.\n");
        exit(EXIT_FAILURE);
    }

    // Begin tests - only one process
    int x = 7;
    double *ptr;
    size_t dbl_size = sizeof(double);

    // Regular use of malloc
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    *ptr = 3.1415926535;
    shmfree(ptr, SHM_SIZE);

    // Freeing a pointer not allocated by shmalloc
    shmfree(&x, SHM_SIZE);
    shmfree(ptr + 1, SHM_SIZE);

    // Double-free a pointer
    dbl_size = 2 * sizeof(double);
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    shmfree(ptr, SHM_SIZE);
    shmfree(ptr, SHM_SIZE);

    // Free, alloc, free, alloc should cause no error
    dbl_size = 5 * sizeof(double);
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    shmfree(ptr, SHM_SIZE);
    dbl_size = 7 * sizeof(double);
    ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
    shmfree(ptr, SHM_SIZE);

    // Tests - multiple processes
//    int pid;
//    pid = fork();
//    if (pid == 0) {
//        // Child process
//        // Allocate the same ID (one create, another attaches)
//         ptr = (double *) shmalloc(shm_id, &dbl_size, shm_ptr, SHM_SIZE);
//    }
//    else {
//        // Parent process
//    }

    exit(EXIT_SUCCESS);
}
