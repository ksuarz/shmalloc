#include "shmalloc.h"

/**
 * Allocates a chunk of shared memory.
 */
void *shmalloc(int id, size_t *size, void *shmptr)
{
    static int initialized = 0;
    static Header *root;
    Header *p, *next;

    if (!initialized) {
        // One-time initialization on the first call
        // TODO call the header init thing?
    }

    // Lock the mutex
    pthread_mutex_lock(&root->mutex);

    p = root;
    do {
        if(!p->is_free) {
            // Not free.
            p = p->next;
        }
        else if (p->size < size) {
            // Too small to satisfy the request.
            p = p->next;
        }
        else if (p->size < (size + sizeof(Header))) {
            // Not enough space to chop up - give them all of it
            p->is_free = 0;
            return (char *) p + sizeof(Header);
        }
        else {
            // Found a chunk to allocate and break up
            next = (Header *)((char *) p + size + sizeof(Header));
            next->prev = p;
            next->next = p->next;
            next->size = p->size - sizeof(Header) - size;
            next->is_free = 1;
            if (p->next != NULL) {
                p->next->prev = next;
            }
            p->next = next;
            p->size = size;
            p->is_free = 0;
            return (char *) p + sizeof(Header);
        }
    } while (p != NULL);

    // No space available to fulfill the request.
    return NULL;
}

void *_shmalloc(int id, size_t *size, void *shmptr, int line, char *file) {
}

/*
 * Frees an object in shared memory
 */
void shmfree(void *shmptr)
{
    //Get the associated header
    Header *h = shmptr - sizeof(Header);
    Header *first = h;

    //Check that this is a valid header
    if(h->bitseq != BITSEQ)
        return;

    //Bit sequence matches, can proceed
    if(h->is_free)
        return;

    //LOCK EVERYTHING
    while(first->prev != NULL)
    {
        first = first->prev;
    }
    pthread_mutex_lock(&(first->mutex));

    //If we are the last reference
    if(--(h->refcount) <= 0)
    {
        /* If the previous header is also free, we can
         * get rid of this one to open up space*/
        if(h->prev->is_free)
        {
            destroy_header(h);
            return;
        }
        else
        {
            h->is_free = 1;
        }
    }

    pthread_mutex_unlock(&(first->mutex));
}

void initialize_header(Header *h, size_t size, int id, unsigned char is_first)
{
    //Sanity check
    if(h == NULL)
        return;

    h->prev = NULL;
    h->next = NULL;
    h->size = size;
    h->refcount = 0;
    h->id = id;
    h->is_free = 1;
    h->bitseq = BITSEQ;

    if(is_first) {
        h->has_mutex = 1;
        pthread_mutexattr_init(&(h->attr));
        pthread_mutexattr_setpshared(&(h->attr), PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&(h->mutex), &(h->attr));
    }
    else
    {
        h->has_mutex = 0;
    }
}

/*
 * Destroys a header struct
 * Assumes that if a mutex exists, it is locked
 */
void destroy_header(Header *h)
{
    //Sanity check
    if(h == NULL)
        return;

    //Adjust previous and next accordingly
    if(h->prev != NULL)
    {
        h->prev->next = h->next;
    }
    if(h->next != NULL)
    {
        h->next->prev = h->prev;
    }

    //Now the list is good, corrupt bitseq to be safe
    h->bitseq += 1;

    //Unlock and destroy mutex
    if(h->has_mutex)
    {
        pthread_mutex_unlock(&(h->mutex));
        pthread_mutex_destroy(&(h->mutex));
    }

}
