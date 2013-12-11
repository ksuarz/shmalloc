shmalloc
========
Like malloc(3), but for shared memory blocks.

Remember
--------
- If someone `shmalloc`s with some ID `id`, we have to tell if we need to create
  it or just attach it (probably a dirwalk).

Things we need as part of the requirements (i.e. error checking):
- Freeing a pointer not made by malloc
    - You malloc something and store it as ptr, but you free (ptr + 1)
    - Check the bit sequence
    - Freeing pointers that were not allocated
    - Freeing pointers made by `malloc(3)`
- Redundant freeing of the same pointer
    - Check the `is_free` flag
- Allocating all of the memory (oversaturation)
    - return `NULL`
- Fragmentation

IPC stuff:
- Make sure we initialize mutexes for the internal data structures
- Mutexes have special shared memory things (set some flags)
- Don't deadlock

When implementing `shmfree`:
- Keep track of the reference count; when the reference count hits zero we
  actually free and coalesce the memory with adjacent spots.
- Make sure we don't deadlock when we destroy the headers

In `shmfree`:
    
    pthread_mutex_lock(header->mutex);
    if (refcount <= 0) {
        // Destroy the header
    }
    pthread_mutex_unlock(header->mutex);

Have a canonical locking order for the mutexes in `destroy_header`:

    pthread_lock(prev->mutex);
    pthread_lock(next->mutex);
    prev->next = next;
    next->prev = prev;

Or something like that.

For the write-up:
- Don't forget a quick paragraph oh why we didn't do `shmrealloc`




SHMAT GIVES YOU A MEMORY SEGMENT FULL OF ZEROES
Expectation: get malloc in shared memory to work AT ALL
