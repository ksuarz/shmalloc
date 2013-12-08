#include "shmalloc.h"

/*
 * Frees an object in shared memory
 * TODO:Figure out mutexes
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
