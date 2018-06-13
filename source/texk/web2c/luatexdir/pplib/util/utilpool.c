
#include <string.h> // for memset
#include <stdio.h> // for printf
#include "utilpool.h"

/*
# Pool

Here is a wrapper for malloc() / free() that can be used whenever we need large
number of (rather small) memory chunks of equal size. The basic concept is to
distribute those chunks from larger blocks and eventually reuse chunks that has
been freed. Among of a speed gain (no search for suitable memory
size), pool also provide a storage that allows to free all items at once or
iterate over all being used. The cost is one extra pointer (ghost) at the beginning
of every distributed chunk and possible waste of memory when blocks are scattered
(which is still better then having all mem fragmented).

The pool consists of blocks organized into two bidirectional linked lists --
blocks list and squots list. Squot is a block which has at least one already
released, reusable chunk. Every block knows its space (defined by the user when
initializing the pool), its size (consumed chunks) and a linked list of freed
chunks from that block. Whenever a new chunk is requested, we first try to take
it from the current block. If the entire block is consumed, we take the squot
and reuse a chunk already freed. If there is such chunk, that
is, if there is any squot, we remove the chunk from the squot. If this was
a sole free chunk in this squot, we also remove the block from squots list. If
there are no more reusable items (the pool is fully used), we allocate another
block.

Before providing a new chunk to the user we always save the block to which the
chunk belongs to as the ghost pointer of the chunk. Whenever the user gives
back some chunk, we access its ghost pointer, fetch the containing block and we
insert the released chunk at the beginning of block->free list. The ghost
pointer is then used to store the next reusable chunk pointer forming a linked
list of reusables.

In the first approach, a ghost pointer was used exclusively to store the block
address, while the pointer to the next reusable chunk was stored at the
beginning of the user space of the chunk. Now we don't touch the user space at
all, we always use a ghost pointer instead. So if the chunk is currently used,
the ghost pointer contains a block address. If the chunk has been released by
the user, the pointer stores the next reusable chunk ghost (or NULL). We use
that fact when iterating over items -- if the address of the ghost pointer is
the block address, it means that the chunk is currently in use.

The pool not only grows but generally also shrinks. Among a linked list of
freed chunks, every block also knows how many chunks has been released. If the
number of released chunks is equal to the entire block space, we remove the
block and free it. Note that we free the block only if it has been entirely
consumed and then all its items has been released. Otherwise we
could have a fatal case when the user repeatedly takes just several chunks and
free them all, which would result in pointless allocating large blocks just to
consume a small piece of it and free them just after.

The consequence of freeing only those blocks that has been previously entirely
consumed is that even after releasing all chunks, the pool may still keep (at
most) one partially consumed block. So the pool always needs to be freed when
no longer needed. This also means that if one doesn't care much about memory
occupied by the pool, releasing every single chunk separatelly can be ommited.

Timing shows that using really large blocks doesn't speeds up significantly,
but obviously increases fragmentation risk. Lets take (hopefully not realistic)
example when we first pull out large amount of chunks and then free 99 of every
100 (99 freed, 1 left, 99 freed, 1 left, ...) If we use blocks of 100 chunks or
more, the pool doesn't shrink at all and we waste 99% of space. If we use
blocks of 2 chunks, the pool shrinkage is 98%, but then we aren't noticeably
faster then normal malloc.

Neither receiving nor releasing a chunk require any search so the cost of
pulling out and releasing chunks is constant.

20111113
Added .endblock to pool structure; a pointer to the last of blocks, that is,
first allocated. Keeping this pointer up-to-data is costless, but speeds up
access to first used element when iterating over items.

*/

typedef struct pool_ghost {
  void *pointer;
} pool_ghost;

#define to_pool_ghost(voidp) (((pool_ghost *)(voidp)) - 1)
#define to_pool_ref(voidp) ((int *)(voidp) - sizeof(pool_ghost)/sizeof(int) - 1)
#define ghost_ref(ghost) ((int *)(ghost) - 1)
#define to_voidp(ghost) ((void *)(ghost + 1))

struct pool_block {
  size_t size, space, freed;
  pool_block *next, *prev;
  pool_block *nextsquot, *prevsquot;
  pool_ghost *free;
  pool_byte *data;
  pool *pool;
};

#define pool_unit_space(pool)     (sizeof(pool_ghost) + pool->unitspace)
#define pool_refunit_space(pool)  (pool_unit_space(pool) + sizeof(int))
#define pool_block_space(pool)    (sizeof(pool_block) + pool->blockspace * pool_unit_space(pool))
#define pool_refblock_space(pool) (sizeof(pool_block) + pool->blockspace * pool_refunit_space(pool))

#define ghost_index(pool, block, index)    (block->data + index * pool_unit_space(pool))
#define refghost_index(pool, block, index) (block->data + sizeof(int) + index * pool_refunit_space(pool))

pool * pool_init (pool *p, size_t unitspace, size_t blockspace, void * (*pmalloc)(size_t), void (*pfree)(void *))
{
  p->unitspace = unitspace;
  p->blockspace = blockspace;
  p->malloc = (pmalloc != NULL ? pmalloc : malloc);
  p->free = (pfree != NULL ? pfree : free);
  p->block = p->endblock = p->squot = p->current = NULL;
  p->pointer = p->endpointer = NULL;
  p->flags = POOL_DEFAULTS;
  return p;
}

void pool_free (pool *p)
{
  pool_block *block, *next;
  for (block = p->block; block != NULL; block = next)
  {
    next = block->next;
    p->free(block);
  }
  p->block = p->endblock = p->squot = NULL;
}

/* insert a new block as pool->block */

static pool_block * insert_block (pool *p)
{
  pool_block *block;
  if (p->flags & POOL_REFCOUNT)
    block = (pool_block *)p->malloc(pool_refblock_space(p));
  else
    block = (pool_block *)p->malloc(pool_block_space(p));
  if (block == NULL)
    return NULL;
  block->size = block->freed = 0;
  block->space = p->blockspace; /* constant for now */
  block->free = NULL;
  block->data = (pool_byte *)(block + 1);
  if ((block->next = p->block) != NULL)
    p->block->prev = block;
  else
    p->endblock = block; /* tail block, first for iterators */
  block->prev = NULL;
  block->nextsquot = NULL;
  block->prevsquot = NULL;
  p->block = block;
  block->pool = p;
  return block;
}

/* remove and free a block */

static void remove_block (pool *p, pool_block *squot)
{
  pool_block *next, *prev;
  next = squot->nextsquot, prev = squot->prevsquot;
  if (next)
    next->prevsquot = prev;
  if (prev)
    prev->nextsquot = next;
  else
    p->squot = next;
  next = squot->next, prev = squot->prev;
  if (next)
    next->prev = prev;
  else
    p->endblock = prev;
  if (prev)
    prev->next = next;
  else
    p->block = next;
  p->free(squot);
}

/* pull out an element from pool */

#define new_ghost(pool, block, ghost) \
  (ghost = (pool_ghost *)ghost_index(pool, block, block->size), (ghost->pointer = (void *)block), ++block->size)

#define new_refghost(pool, block, ghost) \
  (ghost = (pool_ghost *)refghost_index(pool, block, block->size), (ghost->pointer = (void *)block), (*(ghost_ref(ghost)) = 0), ++block->size)

void * pool_out (pool *p)
{
  pool_block *block;
  pool_ghost *ghost;
  if ((block = p->block) == NULL) /* only the very first call to pool_out()... */
    if ((block = insert_block(p)) == NULL)
      return NULL;
  if (block->size < block->space)
  {
    new_ghost(p, block, ghost);
    return to_voidp(ghost);
  }
  if ((block = p->squot) != NULL) /* means: block->freed > 0 && block->free != NULL */
  {
    ghost = block->free;
    block->free = (pool_ghost *)ghost->pointer;
    ghost->pointer = (void *)block;
    if (--block->freed == 0)
    {
      if ((p->squot = block->nextsquot) != NULL)
        p->squot->prevsquot = NULL;
      block->nextsquot = NULL;
      block->prevsquot = NULL;
    }
    return to_voidp(ghost);
  }
  if ((block = insert_block(p)) == NULL)
    return NULL;
  new_ghost(p, block, ghost);
  return to_voidp(ghost);
}

void * pool_out0 (pool *p)
{
  void *m;
  if ((m = pool_out(p)) != NULL)
    return memset(m, 0, p->unitspace);
  return NULL;
}

void * pool_ref_out (pool *p)
{
  pool_block *block;
  pool_ghost *ghost;
  if ((block = p->block) == NULL) /* true on first call to pool_out() only */
    if ((block = insert_block(p)) == NULL)
      return NULL;
  if (block->size < block->space)
  {
    new_refghost(p, block, ghost);
    return to_voidp(ghost);
  }
  if ((block = p->squot) != NULL) /* means: block->freed > 0 && block->free != NULL */
  {
    ghost = block->free;
    block->free = (pool_ghost *)ghost->pointer;
    ghost->pointer = (void *)block;
    if (--block->freed == 0)
    {
      if ((p->squot = block->nextsquot) != NULL)
        p->squot->prevsquot = NULL;
      block->nextsquot = NULL;
      block->prevsquot = NULL;
    }
    *(ghost_ref(ghost)) = 0;
    return to_voidp(ghost);
  }
  if ((block = insert_block(p)) == NULL)
    return NULL;
  new_refghost(p, block, ghost);
  return to_voidp(ghost);
}

void * pool_ref_out0 (pool *p)
{
  void *m;
  if ((m = pool_ref_out(p)) != NULL)
    return memset(m, 0, p->unitspace);
  return NULL;
}

int * pool_ref_count (void *pointer)
{
  /*
  pool_ghost *ghost = to_pool_ghost(pointer);
  if (ghost->pool->flags & POOL_REFCOUNT)
    return ghost_ref(ghost);
  return NULL;
  */
  return to_pool_ref(pointer);
}

int pool_ref (void *pointer)
{
  return ++(*to_pool_ref(pointer));
}

int pool_unref (void *pointer)
{
  int *ref = to_pool_ref(pointer);
  //if (--(*ref) <= 0)
  if (--(*ref) == 0)
    pool_ref_in(pointer);
  return *ref;
}

/* put the element back to pool */

void pool_in (void *pointer)
{
  pool_ghost *ghost;
  pool_block *block;

  /* access block from ghost pointer */
  ghost = to_pool_ghost(pointer);
  block = (pool_block *)ghost->pointer;
  ghost->pointer = (void *)block->free,
  block->free = ghost;
  /* if this is the first freed item, add to squots list */
  if (++block->freed == 1)
  {
    pool *p = block->pool;
    block->prevsquot = NULL;
    if ((block->nextsquot = p->squot) != NULL)
      p->squot->prevsquot = block;
    p->squot = block;
  }
  else
  /* if all units from that block has been freed, remove and free the block */
  if (block->freed == block->space)
    remove_block(block->pool, block);
}

/* count used elements */

size_t pool_count (pool *p)
{
  pool_block *block;
  size_t count = 0;
  pool_ghost *ghost;
  for (block = p->block; block != NULL; block = block->next)
  {
    count += block->size;
    if (block->free == NULL)
      continue;
    for (ghost = block->free; ghost != NULL; ghost = (pool_ghost *)ghost->pointer)
      --count;
  }
  return count;
}

/* iterator over pool items in use */

void * pool_first (pool *p)
{
  pool_ghost *ghost;
  for (p->current = p->endblock; p->current != NULL; p->current = p->current->prev)
  {
    p->endpointer = ghost_index(p, p->current, p->current->size);
    for (p->pointer = ghost_index(p, p->current, 0); p->pointer < p->endpointer; )
    {
      ghost = (pool_ghost *)p->pointer;
      p->pointer += pool_unit_space(p);
      if (ghost->pointer == p->current) // means this is used item
        return to_voidp(ghost);
    }
  }
  p->pointer = p->endpointer = NULL;
  return NULL;
}

void * pool_ref_first (pool *p)
{
  pool_ghost *ghost;
  for (p->current = p->endblock; p->current != NULL; p->current = p->current->prev)
  {
    p->endpointer = refghost_index(p, p->current, p->current->size);
    for (p->pointer = refghost_index(p, p->current, 0); p->pointer < p->endpointer; )
    {
      ghost = (pool_ghost *)p->pointer;
      p->pointer += pool_refunit_space(p);
      if (ghost->pointer == p->current)
        return to_voidp(ghost);
    }
  }
  p->pointer = p->endpointer = NULL;
  return NULL;
}

void * pool_next (pool *p)
{
  pool_ghost *ghost;
  while (1)
  {
    if (p->pointer < p->endpointer)
    {
      ghost = (pool_ghost *)p->pointer;
      p->pointer += pool_unit_space(p);
      if (ghost->pointer == p->current)
        return to_voidp(ghost);
      continue;
    }
    if (p->current == NULL || /* to avoid crash on redundant call to pool_next() */
       (p->current = p->current->prev) == NULL)
      break;
    p->pointer = ghost_index(p, p->current, 0);
    p->endpointer = ghost_index(p, p->current, p->current->size);
  }
  p->pointer = p->endpointer = NULL;
  p->current = NULL;
  return NULL;
}

void * pool_ref_next (pool *p)
{
  pool_ghost *ghost;
  while (1)
  {
    if (p->pointer < p->endpointer)
    {
      ghost = (pool_ghost *)p->pointer;
      p->pointer += pool_refunit_space(p);
      if (ghost->pointer == p->current)
        return to_voidp(ghost);
      continue;
    }
    if (p->current == NULL || /* to avoid crash on redundant call to pool_next() */
       (p->current = p->current->prev) == NULL)
      break;
    p->pointer = refghost_index(p, p->current, 0);
    p->endpointer = refghost_index(p, p->current, p->current->size);
  }
  p->pointer = p->endpointer = NULL;
  p->current = NULL;
  return NULL;
}

/* parent pool */

/*
pool * pool_parent (void *pointer)
{
  return (pool_block *)(to_pool_ghost(pointer)->pointer)->pool;
}
*/

/* debuging tools */

void pool_stat (pool *p, pool_info *stat)
{
  pool_block *block;
  stat->blocks.all = stat->blocks.squots = 0;
  stat->units.all = stat->units.used = stat->units.freed = 0;
  for (block = p->block; block != NULL; block = block->next)
  {
    ++stat->blocks.all;
    stat->units.all += block->space;
    stat->units.used += block->size - block->freed;
    stat->units.freed += block->freed;
  }
  stat->blocks.squots = 0;
  for (block = p->squot; block != NULL; block = block->nextsquot)
    ++stat->blocks.squots;
  stat->memory.all = stat->blocks.all * pool_block_space(p);
  stat->memory.used = stat->units.used * pool_unit_space(p);
}

void pool_finalize (pool *p, const char *poolname)
{
  static pool_info poolinfo;
  size_t used;
  pool_stat(p, &poolinfo);
  if ((used = poolinfo.units.used) > 0)
    printf("left %lu %s object%s\n", (unsigned long)used, poolname, (used > 1 ? "s" : ""));
  pool_free(p);
}
