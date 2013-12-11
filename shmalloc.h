#ifndef SHMALLOC_H
#define SHMALLOC_H

#include <pthread.h>

// Useful constants
#define BITSEQ 1111111
#define MUTEXSEQ 1010101
#define THRESHOLD 1024

// Macros for allocation, freeing, etc.
#define shmalloc(i, s, p, sz) _shmalloc(i, s, p, sz, __FILE__, __LINE__)
#define shmfree(shmptr, ptr) _shmfree(shmptr, ptr, __FILE__, __LINE__)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

/**
 * Header for managing allocated memory.
 */
struct Header {
    int bitseq;
    int id;
    int refcount;
    size_t size;
    struct Header *prev, *next;
    unsigned char is_free;
};

typedef struct Header Header;

/**
 * A mutex for shared memory.
 */
struct SharedMutex {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
    int bitseq;
}

typedef struct SharedMutex SharedMutex;

/**
 * The default overhead for allocating one chunk of memory via shmalloc.
 */
size_t SHMALLOC_MIN_OVERHEAD = 2*sizeof(Header) + sizeof(SharedMutex);

/**
 * Initializes values in header.
 */
void initialize_header(Header *h, size_t size);

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
void _shmfree(void *shmptr, void *ptr, char *filename, int linenumber);

#endif
