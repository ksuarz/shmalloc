#include "shmalloc.h"

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
        pthread_mutexattr_init(&(h->attr));
        pthread_mutexattr_setpshared(&(h->attr), PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&(h->mutex), &(h->attr));
    }
}
