#ifndef SHMALLOC_H
#define SHMALLOC_H

#include <pthread.h>

#define BITSEQ 1111115
#define shmalloc(i, s, p, sz) _shmalloc(i, s, p, sz, __FILE__, __LINE__)
#define shmfree(ptr, s) _shmfree(ptr, s,  __FILE__, __LINE__)

/**
 * A header for managing memory.
 */
struct Header {
    int bitseq;
    int id;
    int refcount;
    size_t size;
    struct Header *prev, *next;
    unsigned char has_mutex;
    unsigned char is_free;
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
};

typedef struct Header Header;

/**
 * Initializes values in header
 */
void initialize_header(Header *h, size_t size, int id, unsigned char is_first);

/**
 * Destroys the given header structure.
 */
void destroy_header(Header *);

/**
 * Allocate shared .memory that's already been attached via shmat(3).
 * Returns a pointer to the newly allocated memory.
 */
void *_shmalloc(int id, size_t *size, void *shmptr, size_t shm_size,
                char *filename, int linenumber);

/**
 * Frees a block of shared memory previously allocated with shmalloc().
 */
void _shmfree(void *ptr, size_t shm_size, char *filename, int linenumber);

#endif
