#include "shmalloc.h"

/*
 * Frees an object in shared memory
 * TODO:Figure out mutexes
 */
void shmfree(void *shmptr)
{
    //Get the associated header
    Header *h = shmptr - sizeof(Header);

    //Check that this is a valid header
    if(h->bitseq != BITSEQ)
        return;

    //Bit sequence matches, can proceed
    if(h->is_free)
        return;

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
}
