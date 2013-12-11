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
    Header *root, *first, *last, *opposite_end, *curr, *first_fit;
    size_t free_size;

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
    if (shm_size < *size + 2*sizeof(Header)) {
        fprintf(stderr, "%s, line %d: Insufficient memory to fulfill the memory"
                        " allocation request.\n", filename, linenumber);
        return NULL;
    }

    // Grab the first and the last
    first = (Header *) shmptr;
    last = (Header *) ((char *) shmptr + shm_size - sizeof(Header));

    // Calculate the root and opposite end
    // Sizes greater than THRESHOLD are allocated from the back end
    root = (*size > THRESHOLD) ? last : first;
    opposite_end = (root == first) ? last : first;

    // Check for the first call to shmalloc()
    if(!first || first->bitseq != BITSEQ || !last || last->bitseq != BITSEQ) {
        // TODO Race condition

        // Initialize
        free_size = shm_size - 2*sizeof(Header) - *size;
        initialize_header(root, *size, root == last);
        initialize_header(opposite_end, free_size, root != last);

        // Lock the mutexes, just in case!
        pthread_mutex_lock(&first->mutex);
        pthread_mutex_lock(&last->mutex);

        // Mark that this is now claimed
        root->is_free = 0;
        root->refcount = 1;

        if (free_size > sizeof(Header)) {
            // Make a new header after the allocated one
            curr = (root->is_reversed) ?
                (Header *) (char *) root - *size :
                (Header *) (char *) root + sizeof(Header) + *size;
            initialize_header(curr, free_size - sizeof(Header), root->is_reversed);
            root->next = curr;
            curr->prev = root;
        }

        // Finally, clean up and exit
        pthread_mutex_unlock(&first->mutex);
        pthread_mutex_unlock(&last->mutex);
        return (root->is_reversed) ?
            (char *) root - *size :
            root + 1;
    }
    else {
        // Find where we're looking, as well as the opposite end
        curr = root;
        while (opposite_end->next != NULL)
            opposite_end = opposite_end->next;

        // Find the next header to fit, or one already allocated
        while(curr != NULL) {
            if(curr->id == id && !curr->is_free) {
                // Already have item with this id
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
void initialize_header(Header *h, size_t size, unsigned char is_reversed)
{
    //Sanity check
    if(h == NULL)
        return;

    h->bitseq = BITSEQ;
    h->id = 0;
    h->is_free = 1;
    h->is_reversed = is_reversed;
    h->next = NULL;
    h->prev = NULL;
    h->refcount = 0;
    h->size = size;
    pthread_mutex_init(&(h->mutex), &(h->attr));
    pthread_mutexattr_init(&(h->attr));
    pthread_mutexattr_setpshared(&(h->attr), PTHREAD_PROCESS_SHARED);
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
