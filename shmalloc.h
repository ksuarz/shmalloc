#ifndef SHMALLOC_H
#define SHMALLOC_H

#include <pthread.h>

#define BITSEQ 1111111

struct Header {
    struct Header *prev, *next;
    size_t size;
    int refcount;
    int id;
    unsigned char is_free;
    const int bitseq;
    pthread_mutex_t mutex;
};

typedef struct Header Header;

// TODO Have that whole #define __shmalloc for taking in __LINE__ and __FILE__
// business. See the project specs for info

/**
 * Creates a new empty header.
 */
Header *create_header(size_t size, int id);

/**
 * Destroys the given header structure.
 */
void destroy_header(Header *);

/**
 * Allocate shared .memory that's already been attached via shmat(3).
 * Returns a pointer to the newly allocated memory.
 *
 * @param id - An application-specific ID number
 * @param size - A pointer to a size_t. If a block of memory has not yet been
 * allocated with the given ID, a new block is allocated of the given size.
 * Otherwise, the value of size is updated to the actual size of the allocated
 * block.
 * @param shmptr - A pointer to a block of shared memory previously attached to
 * via shmat(3).
 */
void *shmalloc(int id, size_t *size, void *shmptr);

/**
 * Frees a block of shared memory previously allocated with shmalloc().
 *
 * @param shmptr - A pointer to the block to free.
 */
void shmfree(void *shmptr);

#endif
