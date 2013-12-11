#include "shmalloc.h"
#include <stdio.h>

/*
 * Allocates an object in shared memory
 */
void *_shmalloc(int id, size_t *size, void *shmptr, size_t shm_size,
                char *filename, int linenumber)
{
    Header *first, *curr, *best_fit;
    size_t free_size, best_block_size;

    // Verify pointers
    if (shmptr == NULL) {
        fprintf(stderr, "%s, line %d: Shared memory pointer cannot be null.\n",
                        filename, linenumber);
        return NULL;
    }
    if (size == NULL) {
        fprintf(stderr, "%s, line %d: Size pointer cannot be null.\n",
                        filename, linenumber);
        return NULL;
    }
    if (*size == 0) {
        // Like malloc(3), passing in a size of zero returns either NULL or
        // another pointer that can be successfully passed into shmfree()
        fprintf(stderr, "%s, line %d: Warning: allocating a pointer of size "
                        "zero returns NULL.\n", filename, linenumber);
        return NULL;
    }
    if (*size < 0) {
        fprintf(stderr, "%s, line %d: Cannot allocate a negative amount of "
                        "memory in shmalloc().\n", filename, linenumber);
        return NULL;
    }
    if (shm_size < *size + sizeof(Header)) {
        fprintf(stderr, "%s, line %d: Insufficient memory to fulfill the memory"
                        " allocation request.\n", filename, linenumber);
        return NULL;
    }

    // Find the first header
    first = curr = (Header *) shmptr;
    best_fit = NULL;

    // First time calling shmalloc
    if(!first || first->bitseq != BITSEQ)
    {
        initialize_header(first, *size, id, 1);

        //Create the next header if we have enough room
        if((free_size = (2*sizeof(Header) + *size)) < shm_size)
        {
            curr = (Header *)(shmptr + sizeof(Header) + *size);
            initialize_header(curr, free_size, -1, 0);
        }

        return (first + 1);
    }
    else
    {
        //Lock shared memory
        pthread_mutex_lock(&(first->mutex));

        best_block_size = -1;

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
                return (curr + 1);
            }

            //Get size of this block
            if((curr->size < best_block_size || best_block_size == -1) && curr->size >= *size)
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
            fprintf(stderr, "%s, line %d: shmalloc() ran out of available space"
                            " to satisfy the request.\n", filename, linenumber);
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
            curr = (Header *) ((char *) best_fit + best_fit->size);
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
        return (best_fit + 1);
    }
}

/*
 * Frees an object in shared memory
 */
void _shmfree(void *shmptr, size_t shm_size, char *filename, int linenumber)
{
    Header *h, *first;
    if (shmptr == NULL) {
        // Like free(3), shmfree() of a NULL pointer has no effect
        fprintf(stderr, "%s, line %d: free() on a NULL pointer does nothing.\n",
                        filename, linenumber);
        return;
    }

    //Get the associated header
    first = h = ((Header *) shmptr) - 1;

    // More verification checks
    if(h->bitseq != BITSEQ) {
        fprintf(stderr, "%s, line %d: shmfree() detects corruption of internal "
                        "data structures. Check your memory accesses.\n",
                        filename, linenumber);
        return;
    }
    if (h->is_free) {
        fprintf(stderr, "%s, line %d: Attempt to shmfree() a pointer that has "
                        "already been freed.\n", filename, linenumber);
        return;
    }

    //LOCK EVERYTHING
    while(first->prev != NULL)
    {
        first = first->prev;
    }
    pthread_mutex_lock(&(first->mutex));

    //If we are the last reference
    if(--(h->refcount) <= 0)
    {
        //Adjust our size
        if(h->next != NULL)
        {
            h->size = (char *)h->next - (char *)h - sizeof(Header);
        }
        else
        {
            h->size = (char *) shm_size - (char *)h - sizeof(Header);
        }

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
