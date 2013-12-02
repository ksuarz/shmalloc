shmalloc
========

Like malloc(3), but for shared memory blocks.

Remember
--------
- If someone `shmalloc`s with some ID `id`, we have to tell if we need to create
  it or just attach it (probably a dirwalk).
