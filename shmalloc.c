#include "shmalloc.h"
#include <stdio.h>

/*
 * Allocates an object in shared memory. This version attempts to handle
 * fragmentation by allocating from both sides of the shared memory block based
 * on a threshold size.
 */
void *_shmalloc(int id, size_t *size, void *shmptr, size_t shm_size,
                char *filename, int linenumber)
{
    Header *first, *last, *curr, *root, *best_fit;
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
        return NULL;
    }
    if (*size < 0) {
        fprintf(stderr, "%s, line %d: Cannot allocate a negative size of memory"
                        " in shmalloc().\n",
                        filename, linenumber);
        return NULL;
    }
    if (shm_size < *size + sizeof(Header)) {
        fprintf(stderr, "%s, line %d: Insufficient memory to fulfill the memory"
                        " allocation request.\n", filename, linenumber);
        return NULL;
    }

    // Grab the first and the last
    first = (Header *) shmptr;
    last = (Header *) ((char *) shmptr + shm_size - sizeof(Header));

    // Check to see if we need to initialize (done twice from both sides of
    // malloc)
    if(!first || first->bitseq != BITSEQ || !last || last->bitseq != BITSEQ)
    {
        initialize_header(first, *size, id);
        initialize_header(last, sIZE, )

        //Create the next header if we have enough room
        if((free_size = ((2*sizeof(Header)) + *size)) < shm_size)
        {
            curr = (Header *)(shmptr + sizeof(Header) + *size);
            initialize_header(curr, free_size, -1);
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
            initialize_header(curr, (free_size - best_fit->size - sizeof(Header)), -1);

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
 * Frees an object in shared memory.
 * TODO Make this work for double-sided list (though it should in theory; check
 * it anyway)
 */
void _shmfree(void *shmptr, char *filename, int linenumber)
{
    Header *h, *prev, *next;

    // Get the associated headers
    h = shmptr - sizeof(Header);

    // Like free(3), shmfree() of a NULL pointer has no effect
    if (shmptr == NULL) {
        fprintf(stderr, "%s, line %d: Cannot free a null pointer.\n",
                        filename, linenumber);
        return;
    }

    // Set pointers needed later for unlocking mutexes
    next = h->next;
    prev = h->prev;

    // Check that this is a valid header - bit sequence must match
    if (h->bitseq != BITSEQ) {
        fprintf(stderr, "%s, line %d: Attempting to free a corrupted pointer, "
                        "or a pointer not allocated by shmalloc().\n",
                        filename, linenumber);
        return;
    }

    // Check for double-free
    if (h->is_free) {
        fprintf(stderr, "%s, line %d: Redundant freeing of a pointer.\n",
                        filename, linenumber);
        return;
    }

    // Canonical locking order: previous, current, next
    if (h->prev != NULL) {
        pthread_mutex_lock(&h->prev->mutex);
    }
    pthread_mutex_lock(&h->mutex);
    if (h->next != NULL) {
        pthread_mutex_lock(&h->next->mutex);
    }

    // If we are the last reference
    if(--(h->refcount) <= 0)
    {
        // Free the one after us, if possible
        if (h->next != NULL && h->next->is_free)
        {
            h->size += h->next->size + sizeof(Header);
            destroy_header(h->next);
            next = NULL;
        }

        // Delete ourself and combine with the previous, if possible
        if (h->prev != NULL && h->prev->is_free)
        {
            h->prev->size += h->size + sizeof(Header);
            destroy_header(h);
            h = NULL;
        }

        // Header is now freed - this also works for the first one
        if (h != NULL)
        {
            h->is_free = 1;
        }
    }

    // Finally, unlock our mutexes in order
    if (prev != NULL) {
        pthread_mutex_unlock(&prev->mutex);
    }
    if (h != NULL) {
        pthread_mutex_unlock(&h->mutex);
    }
    if (next != NULL) {
        pthread_mutex_unlock(&next->mutex);
    }
}

/**
 * Initializes a header with default values.
 */
void initialize_header(Header *h, size_t size, int id)
{
    //Sanity check
    if(h == NULL)
        return;

    h->bitseq = BITSEQ;
    h->id = id;
    h->is_free = 1;
    h->next = NULL;
    h->prev = NULL;
    h->refcount = 0;
    h->size = size;
    pthread_mutexattr_init(&(h->attr));
    pthread_mutexattr_setpshared(&(h->attr), PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(h->mutex), &(h->attr));
}

/*
 * Destroys a header. Does some appropriate locking to obtain only the resources
 * we require.
 */
void destroy_header(Header *h)
{
    //Sanity check
    if(h == NULL)
        return;

    // Lock the mutexes we need
    if (h->prev != NULL)
    {
        pthread_mutex_lock(&h->prev->mutex);
    }
    pthread_mutex_lock(&h->mutex);
    if (h->next != NULL)
    {
        pthread_mutex_lock(&h->next->mutex);
    }

    // Critical section: adjust previous and next accordingly
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

    // Unlock and destroy mutex
    if (h->prev != NULL)
    {
        pthread_mutex_unlock(&h->prev->mutex);
    }
    pthread_mutex_unlock(&h->mutex);
    pthread_mutex_destroy(&h->mutex);
    if (h->next != NULL)
    {
        pthread_mutex_unlock(&h->next->mutex);
    }
}
