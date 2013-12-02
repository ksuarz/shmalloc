#define BLOCKSIZE 100000
char big_block[BLOCKSIZE];

typedef struct mementry mementry_t;

void *minimalloc(unsigned int size) {
    static int initialized = 0;
    static mementry_t *root;
    mementry_t *p, *next;

    if (!initialized) {
        // One-time initialization on the first call
        root = big_block;
        root->size = blocksize - sizeof(mementry_t);
        root->prev = NULL;
        root->next = NULL;
        root->is_free = 1;
        initialized = 1;
    }

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
        else if (p->size < (size + sizeof(mementry_t))) {
            // Not enough space to chop up - give them all of it
            p->is_free = 0;
            // Cast to char * to guarantee the proper pointer arithmetic
            return (char *) p + sizeof(mementry_t);
            // Equivalent to `return p + 1`
        }
        else {
            // Found a chunk to allocate and break up
            next = (mementry_t *)((char *) p + size + sizeof(mementry_t));
            // Equivalent to next = (mementry_t *)((char *) (p + 1) + size)
            next->prev = p;
            next->next = p->next;
            next->size = p->size - sizeof(mementry_t) - size;
            next->is_free = 1;
            if (p->next != NULL) {
                p->next->prev = next;
            }
            p->next = next;
            p->size = size;
            p->is_free = 0;
            return (char *) p + sizeof(mementry_t);
        }
    } while (p != NULL);

    // No space available to fulfill the request.
    return NULL;
}

void minifree(void *ptr) {
    mementry_t *p, *prev, *next;

    p = (mementry_t *) ptr - 1;
    if (ptr == NULL) {
        return;
    }
    else if ((prev = p.prev) != NULL && prev->is_free) {
        // Coalesce adjacent free nodes together
        prev->size += sizeof(mementry_t) + p->size;
        prev->next = p.next;
        if (prev->next != NULL) {
            prev->next->prev = prev;
        }
    }
    else {
        p->is_free = 1;
        prev = p;
    }

    if ((next = p.next) != NULL && next->is_free) {
        prev->size += sizeof(mementry_t) + next->size;
        prev->is_free = 1;
    }
    else {
        prev->is_free = 1;
    }
}
