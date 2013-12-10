#include "shmalloc.h"

/*
 * Allocates an object in shared memory
 */
void *shmalloc(int id, size_t *size, void *shmptr, size_t shm_size)
{
    Header *first = (Header *) shmptr;
    Header *curr = first;
    Header *best_fit = NULL;
    size_t free_size;
    size_t best_block_size;

    //First time calling shmalloc
    if(!first || first->bitseq != BITSEQ)
    {
        initialize_header(first, *size, id, 1);

        //Create the next header if we have enough room
        if((free_size = ((2*sizeof(Header)) + *size)) < shm_size)
        {
            curr = (Header *)(shmptr + sizeof(Header) + *size);
            initialize_header(curr, free_size, -1, 0);
        }

        return (first + sizeof(Header));
    }
    else
    {
        //Lock shared memory
        pthread_mutex_lock(&(first->mutex));

        //Loop through all headers to see if id already exists
        //Also record best spot to put this new item if it does not exist
        while(curr != NULL)
        {
            if(curr->id == id && !curr->is_free)
            {
                //Already have item with this id
                curr->refcount++;
                *size = curr->size;

                //Can unlock mutex and return here
                pthread_mutex_unlock(&(first->mutex));
                return (curr + sizeof(Header));
            }

            //Get size of this block
            if(curr->size < best_block_size && curr->size > *size)
            {
                best_block_size = curr->size;
                best_fit = curr;
            }

            curr = curr->next;
        }

        //Did not find existing entry

        if(best_fit == NULL)
        {
            //Did not find a viable chunk, failure
            pthread_mutex_unlock(&(first->mutex));
            return NULL;
        }

        //Found a viable chunk - use it
        free_size = best_fit->size; //Total size of chunk before next header

        best_fit->size = *size;
        best_fit->refcount = 1;
        best_fit->id = id;
        best_fit->is_free = 0;

        //Check if there is enough room to make another header
        if((free_size - best_fit->size) > 0)
        {
            curr = (Header *) (best_fit + best_fit->size);
            initialize_header(curr, (free_size - best_fit->size - sizeof(Header)), -1, 0);

            //Adjust pointers
            curr->prev = best_fit;
            curr->next = best_fit->next;
            best_fit->next = curr;
            if(best_fit->next != NULL)
            {
                best_fit->next->prev = curr;
            }
        }

        pthread_mutex_unlock(&(first->mutex));
        return (best_fit + sizeof(Header));
    }
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
        //Don't delete the first entry
        if(h != first) {

            /*Check if we can delete ourselves or our next to free up space*/
            if(h->next != NULL && h->next->is_free)
            {
                h->size += h->next->size + sizeof(Header);
                destroy_header(h->next);
            }
            if(h->prev != NULL && h->prev->is_free)
            {
                h->prev->size += h->size + sizeof(Header);
                destroy_header(h);
                h = NULL;
            }
        }

        //Need to set h to freed
        if(h != NULL || h == first)
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
