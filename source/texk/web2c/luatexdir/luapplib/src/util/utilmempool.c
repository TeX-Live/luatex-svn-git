
#include "utilmempool.h"
#include "utilmemallc.h"

/* alloc */

#define pool8_space(pool)  (sizeof(pile8)  + pool->units * pool->unit)
#define pool16_space(pool) (sizeof(pile16) + pool->units * pool->unit)
#define pool32_space(pool) (sizeof(pile32) + pool->units * pool->unit)
#ifdef BIT32
#  define pool64_space(pool) (sizeof(pile64) + pool->units * pool->unit + 4)
#else
#  define pool64_space(pool) (sizeof(pile64) + pool->units * pool->unit)
#endif

#define pile_alloc8(pool)  ((pile8 *)((pool->flags  & POOL_ZERO) ? util_calloc(1, pool8_space(pool))  : util_malloc(pool8_space(pool))))
#define pile_alloc16(pool) ((pile16 *)((pool->flags & POOL_ZERO) ? util_calloc(1, pool16_space(pool)) : util_malloc(pool16_space(pool))))
#define pile_alloc32(pool) ((pile32 *)((pool->flags & POOL_ZERO) ? util_calloc(1, pool32_space(pool)) : util_malloc(pool32_space(pool))))
#define pile_alloc64(pool) ((pile64 *)((pool->flags & POOL_ZERO) ? util_calloc(1, pool64_space(pool)) : util_malloc(pool64_space(pool))))

/* ghost to block */

#define ghost_pile8(ghost) ghost_block8(ghost, pile8)
#define ghost_pile16(ghost) ghost_block16(ghost, pile16)
#define ghost_pile32(ghost) ghost_block32(ghost, pile32)
#define ghost_pile64(ghost) ghost_block64(ghost, pile64)

/* block reset */

#define reset_pool_head8(pool, pile, used) \
  ((used = block_used8(pile)), (pile->data -= used), ((pool->flags & POOL_ZERO) ? (memset(pile->data, 0, used), 0) : 0), (pile->left = pool->units))
#define reset_pool_head16(pool, pile, used) \
  ((used = block_used16(pile)), (pile->data -= used), ((pool->flags & POOL_ZERO) ? (memset(pile->data, 0, used), 0) : 0), (pile->left = pool->units))
#define reset_pool_head32(pool, pile, used) \
  ((used = block_used32(pile)), (pile->data -= used), ((pool->flags & POOL_ZERO) ? (memset(pile->data, 0, used), 0) : 0), (pile->left = pool->units))
#define reset_pool_head64(pool, pile, used) \
  ((used = block_used64(pile)), (pile->data -= used), ((pool->flags & POOL_ZERO) ? (memset(pile->data, 0, used), 0) : 0), (pile->left = pool->units))

#define remove_pool_head(pool, pile) (pool->head = NULL, pool->tail = NULL, pool->squot = NULL, util_free(pile))

#define pile_reset_free8(pile) (pile->freeoffset = 0xff, pile->prevsquot = NULL, pile->nextsquot = NULL)
#define pile_reset_free16(pile) (pile->freeoffset = 0xffff, pile->prevsquot = NULL, pile->nextsquot = NULL)
#ifdef BIT32
#  define pile_reset_free32(pile) (pile->freeghost = NULL, pile->prevsquot = NULL, pile->nextsquot = NULL)
#else
#  define pile_reset_free32(pile) (pile->freeoffset = 0xffffffff, pile->prevsquot = NULL, pile->nextsquot = NULL)
#endif
#define pile_reset_free64(pile) (pile->freeghost = NULL, pile->prevsquot = NULL, pile->nextsquot = NULL)

#define pile_has_free8(pile) (pile->freeoffset != 0xff)
#define pile_has_free16(pile) (pile->freeoffset != 0xffff)
#ifdef BIT32
#  define pile_has_free32(pile) (pile->freeghost != NULL)
#else
#  define pile_has_free32(pile) (pile->freeoffset != 0xffffffff)
#endif
#define pile_has_free64(pile) (pile->freeghost != NULL)

#define pile_get_free8(pile) ((ghost8 *)void_data(block_top(pile) + pile->freeoffset))
#define pile_get_free16(pile) ((ghost16 *)void_data(block_top(pile) + pile->freeoffset))
#ifdef BIT32
#  define pile_get_free32(pile) (pile->freeghost)
#else
#  define pile_get_free32(pile) ((ghost32 *)void_data(block_top(pile) + pile->freeoffset))
#endif
#define pile_get_free64(pile) (pile->freeghost)

#define pile_use_free8(pile, ghost) ((ghost = pile_get_free8(pile)), (pile->freeoffset = ghost->offset), (ghost->offset = (uint8_t)ghost_offset(pile, ghost)))
#define pile_use_free16(pile, ghost) ((ghost = pile_get_free16(pile)), (pile->freeoffset = ghost->offset), (ghost->offset = (uint16_t)ghost_offset(pile, ghost)))
#ifdef BIT32
#  define pile_use_free32(p1le, ghost) ((ghost = pile_get_free32(p1le)), (p1le->freeghost = ghost->nextfree), (ghost->pile = p1le))
#else
#  define pile_use_free32(pile, ghost) ((ghost = pile_get_free32(pile)), (pile->freeoffset = ghost->offset), (ghost->offset = (uint32_t)ghost_offset(pile, ghost)))
#endif
#define pile_use_free64(p1le, ghost) ((ghost = pile_get_free64(p1le)), (p1le->freeghost = ghost->nextfree), (ghost->pile = p1le))

#define pile_set_free8(pile, ghost) (ghost->offset = pile->freeoffset), (pile->freeoffset = (uint8_t)ghost_offset(pile, ghost))
#define pile_set_free16(pile, ghost) (ghost->offset = pile->freeoffset), (pile->freeoffset = (uint16_t)ghost_offset(pile, ghost))
#ifdef BIT32
#  define pile_set_free32(pile, ghost) ((ghost->nextfree = pile->freeghost), (pile->freeghost = ghost))
#else
#  define pile_set_free32(pile, ghost) (ghost->offset = pile->freeoffset), (pile->freeoffset = (uint32_t)ghost_offset(pile, ghost))
#endif
#define pile_set_free64(pile, ghost) ((ghost->nextfree = pile->freeghost), (pile->freeghost = ghost))

/* ghost in use */

#define ghost_in_use8(ghost, pile) (ghost->offset == (uint8_t)ghost_offset(pile, ghost))
#define ghost_in_use16(ghost, pile) (ghost->offset == (uint16_t)ghost_offset(pile, ghost))
#ifdef BIT32
#  define ghost_in_use32(ghost, p1le) (ghost->pile == p1le)
#else
#  define ghost_in_use32(ghost, pile) (ghost->offset == (uint32_t)ghost_offset(pile, ghost))
#endif
#define ghost_in_use64(ghost, p1le) (ghost->pile == p1le)

/* pool init */

#define unit_size8(unit) ((size_t)unit + sizeof(ghost8))
#define unit_size16(unit) ((size_t)unit + sizeof(ghost16))
#define unit_size32(unit) ((size_t)unit + sizeof(ghost32))
#define unit_size64(unit) ((size_t)unit + sizeof(ghost64))

#define max_offset(units, unitsize) (((size_t)units - 1) * unitsize)

#define pool_units_check8(units, unit)  (units > 0 && max_offset(units, unit) < 0xff)
#define pool_units_check16(units, unit) (units > 0 && max_offset(units, unit) < 0xffff)
#define pool_units_check32(units, unit) (units > 0 && max_offset(units, unit) < 0xffffffff)
#define pool_units_check64(units, unit) (units > 0)

pool8 * pool8_init (pool8 *pool, uint16_t units, size_t unit, uint8_t flags)
{
  unit = unit_size8(unit);
  align_size8(unit);
  if (pool_units_check8(units, unit) == 0) return NULL;
  pool->units = units;
  pool->unit = unit_size8(unit);
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  pool->flags = flags;
  return pool;
}

pool16 * pool16_init (pool16 *pool, uint16_t units, size_t unit, uint8_t flags)
{
  unit = unit_size16(unit);
  align_size16(unit);
  if (pool_units_check16(units, unit) == 0) return NULL;
  pool->units = units;
  pool->unit = unit_size16(unit);
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  pool->flags = flags;
  return pool;
}

pool32 * pool32_init (pool32 *pool, uint16_t units, size_t unit, uint8_t flags)
{
  unit = unit_size32(unit);
  align_size32(unit);
  if (pool_units_check32(units, unit) == 0) return NULL;
  pool->units = units;
  pool->unit = unit_size32(unit);
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  pool->flags = flags;
  return pool;
}

pool64 * pool64_init (pool64 *pool, uint16_t units, size_t unit, uint8_t flags)
{
  unit = unit_size64(unit);
  align_size64(unit);
  if (pool_units_check64(units, unit) == 0) return NULL;
  pool->units = units;
  pool->unit = unit_size64(unit);
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  pool->flags = flags;
  return pool;
}

/* pool free */

void pool8_free (pool8 *pool)
{
  pile8 *pile, *prev;
  pile = pool->head;
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  while (pile != NULL)
  {
    prev = pile->prev;
    util_free(pile);
    pile = prev;
  }
}

void pool16_free (pool16 *pool)
{
  pile16 *pile, *prev;
  pile = pool->head;
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  while (pile != NULL)
  {
    prev = pile->prev;
    util_free(pile);
    pile = prev;
  }
}

void pool32_free (pool32 *pool)
{
  pile32 *pile, *prev;
  pile = pool->head;
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  while (pile != NULL)
  {
    prev = pile->prev;
    util_free(pile);
    pile = prev;
  }
}

void pool64_free (pool64 *pool)
{
  pile64 *pile, *prev;
  pile = pool->head;
  pool->head = NULL;
  pool->tail = NULL;
  pool->squot = NULL;
  while (pile != NULL)
  {
    prev = pile->prev;
    util_free(pile);
    pile = prev;
  }
}

/* pool clear */

void pool8_clear (pool8 *pool)
{
  pile8 *pile, *prev;
  size_t used;
  if ((pile = pool->head) == NULL)
    return;
  pool->tail = pile;
  pool->squot = NULL;
  prev = pile->prev;
  pile->prev = NULL;
  reset_pool_head8(pool, pile, used);
  pile_reset_free8(pile);
  for (; prev != NULL; prev = pile)
  {
    pile = prev->prev;
    util_free(prev);
  }
}

void pool16_clear (pool16 *pool)
{
  pile16 *pile, *prev;
  size_t used;
  if ((pile = pool->head) == NULL)
    return;
  pool->tail = pile;
  pool->squot = NULL;
  prev = pile->prev;
  pile->prev = NULL;
  reset_pool_head16(pool, pile, used);
  pile_reset_free16(pile);
  for (; prev != NULL; prev = pile)
  {
    pile = prev->prev;
    util_free(prev);
  }
}

void pool32_clear (pool32 *pool)
{
  pile32 *pile, *prev;
  size_t used;
  if ((pile = pool->head) == NULL)
    return;
  pool->tail = pile;
  pool->squot = NULL;
  prev = pile->prev;
  pile->prev = NULL;
  reset_pool_head32(pool, pile, used);
  pile_reset_free32(pile);
  for (; prev != NULL; prev = pile)
  {
    pile = prev->prev;
    util_free(prev);
  }
}

void pool64_clear (pool64 *pool)
{
  pile64 *pile, *prev;
  size_t used;
  if ((pile = pool->head) == NULL)
    return;
  pool->tail = pile;
  pool->squot = NULL;
  prev = pile->prev;
  pile->prev = NULL;
  reset_pool_head64(pool, pile, used);
  pile_reset_free64(pile);
  for (; prev != NULL; prev = pile)
  {
    pile = prev->prev;
    util_free(prev);
  }
}

/* pool take */

void pool8_head (pool8 *pool)
{
  pile8 *pile;
  pool->head = pile = pile_alloc8(pool);
  pile->next = NULL, pile->prev = NULL;
  pile_reset_free8(pile);
  pile->data = block_edge8(pile);
  pile->left = pool->units;
  pile->chunks = 0;
}

void pool16_head (pool16 *pool)
{
  pile16 *pile;
  pool->head = pile = pile_alloc16(pool);
  pile->next = NULL, pile->prev = NULL;
  pile_reset_free16(pile);
  pile->data = block_edge16(pile);
  pile->left = pool->units;
  pile->chunks = 0;
}

void pool32_head (pool32 *pool)
{
  pile32 *pile;
  pool->head = pile = pile_alloc32(pool);
  pile->next = NULL, pile->prev = NULL;
  pile_reset_free32(pile);
  pile->data = block_edge32(pile);
  pile->left = pool->units;
  pile->chunks = 0;
}

void pool64_head (pool64 *pool)
{
  pile64 *pile;
  pool->head = pile = pile_alloc64(pool);
  pile->next = NULL, pile->prev = NULL;
  pile_reset_free64(pile);
  pile->data = block_edge64(pile);
  pile->left = pool->units;
  pile->chunks = 0;
}

static pile8 * pool8_new (pool8 *pool)
{
  pile8 *pile, *head;
  pile = pile_alloc8(pool);
  head = pool->head;
  pile->next = NULL, pile->prev = head;
  head->next = pile, pool->head = pile;
  pile_reset_free8(pile);
  pile->data = block_edge8(pile);
  pile->left = pool->units;
  pile->chunks = 0;
  return pile;
}

static pile16 * pool16_new (pool16 *pool)
{
  pile16 *pile, *head;
  pile = pile_alloc16(pool);
  head = pool->head;
  pile->next = NULL, pile->prev = head;
  head->next = pile, pool->head = pile;
  pile_reset_free16(pile);
  pile->data = block_edge16(pile);
  pile->left = pool->units;
  pile->chunks = 0;
  return pile;
}

static pile32 * pool32_new (pool32 *pool)
{
  pile32 *pile, *head;
  pile = pile_alloc32(pool);
  head = pool->head;
  pile->next = NULL, pile->prev = head;
  head->next = pile, pool->head = pile;
  pile_reset_free32(pile);
  pile->data = block_edge32(pile);
  pile->left = pool->units;
  pile->chunks = 0;
  return pile;
}

static pile64 * pool64_new (pool64 *pool)
{
  pile64 *pile, *head;
  pile = pile_alloc64(pool);
  head = pool->head;
  pile->next = NULL, pile->prev = head;
  head->next = pile, pool->head = pile;
  pile_reset_free64(pile);
  pile->data = block_edge64(pile);
  pile->left = pool->units;
  pile->chunks = 0;
  return pile;
}

#define pile_next(pool, pile) (pile->data += pool->unit, --pile->left, ++pile->chunks)

void * _pool8_take (pool8 *pool)
{
  pile8 *pile;
  ghost8 *ghost;
  pile = pool->head;
  if (pile->left > 0)
  {
    ghost_next8(pile, ghost);
    pile_next(pool, pile);
  }
  else if ((pile = pool->squot) != NULL)
  {
    ASSERT8(pile_has_free8(pile));
    pile_use_free8(pile, ghost);
    if (!pile_has_free8(pile))
    {
      pile8 *prev, *next;
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free8(pile);
    }
    if (pool->flags & POOL_ZERO)
      memset(ghost_data(ghost), 0, pool->unit - sizeof(ghost8));
    ++pile->chunks;
  }
  else
  {
    pile = pool8_new(pool);
    ghost_next8(pile, ghost);
    pile_next(pool, pile);
  }
  return ghost_data(ghost);
}

void * _pool16_take (pool16 *pool)
{
  pile16 *pile;
  ghost16 *ghost;
  pile = pool->head;
  if (pile->left > 0)
  {
    ghost_next16(pile, ghost);
    pile_next(pool, pile);
  }
  else if ((pile = pool->squot) != NULL)
  {
    ASSERT16(pile_has_free16(pile));
    pile_use_free16(pile, ghost);
    if (!pile_has_free16(pile))
    {
      pile16 *prev, *next;
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free16(pile);
    }
    if (pool->flags & POOL_ZERO)
      memset(ghost_data(ghost), 0, pool->unit - sizeof(ghost16));
    ++pile->chunks;
  }
  else
  {
    pile = pool16_new(pool);
    ghost_next16(pile, ghost);
    pile_next(pool, pile);
  }
  return ghost_data(ghost);
}

void * _pool32_take (pool32 *pool)
{
  pile32 *pile;
  ghost32 *ghost;
  pile = pool->head;
  if (pile->left > 0)
  {
    ghost_next32(pile, ghost);
    pile_next(pool, pile);
  }
  else if ((pile = pool->squot) != NULL)
  {
    ASSERT32(pile_has_free32(pile));
    pile_use_free32(pile, ghost);
    if (!pile_has_free32(pile))
    {
      pile32 *prev, *next;
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free32(pile);
    }
    if (pool->flags & POOL_ZERO)
      memset(ghost_data(ghost), 0, pool->unit - sizeof(ghost32));
    ++pile->chunks;
  }
  else
  {
    pile = pool32_new(pool);
    ghost_next32(pile, ghost);
    pile_next(pool, pile);
  }
  return ghost_data(ghost);
}

void * _pool64_take (pool64 *pool)
{
  pile64 *pile;
  ghost64 *ghost;
  pile = pool->head;
  if (pile->left > 0)
  {
    ghost_next64(pile, ghost);
    pile_next(pool, pile);
  }
  else if ((pile = pool->squot) != NULL)
  {
    ASSERT64(pile_has_free64(pile));
    pile_use_free64(pile, ghost);
    if (!pile_has_free64(pile))
    {
      pile64 *prev, *next;
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free64(pile);
    }
    if (pool->flags & POOL_ZERO)
      memset(ghost_data(ghost), 0, pool->unit - sizeof(ghost64));
    ++pile->chunks;
  }
  else
  {
    pile = pool64_new(pool);
    ghost_next64(pile, ghost);
    pile_next(pool, pile);
  }
  return ghost_data(ghost);
}

/* pool back */

void pool8_back (pool8 *pool, void *data)
{
  ghost8 *ghost;
  pile8 *pile;
  ghost = data_ghost8(data);
  pile = ghost_pile8(ghost);
  if (--pile->chunks > 0)
  {
    if (!pile_has_free8(pile))
    {
      pile8 *prev;
      if ((prev = pool->squot) != NULL)
      {
        prev->nextsquot = pile;
        pile->prevsquot = prev;
      }
      pool->squot = pile;
    }
    pile_set_free8(pile, ghost);
  }
  else
  {
    pile8 *next, *prev;
    size_t used;
    if (pile_has_free8(pile))
    {
      ASSERT8(pool->squot != NULL);
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free8(pile);
    }
    if ((next = pile->next) != NULL)
    {
      ASSERT8(pile != pool->head);
      prev = pile->prev, next->prev = prev;
      if (prev != NULL) prev->next = next; else pool->tail = next;
      util_free(pile);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT8(next == pool->head && next == pool->tail);
        ASSERT8(!pile_has_free8(next));
        if ((pool->flags & POOL_KEEP) == 0)
          remove_pool_head(pool, next);
      }
    }
    else
    {
      ASSERT8(pile == pool->head);
      if ((prev = pile->prev) != NULL || (pool->flags & POOL_KEEP))
        reset_pool_head8(pool, pile, used);
      else
        remove_pool_head(pool, pile);
    }
  }
}

void pool16_back (pool16 *pool, void *data)
{
  ghost16 *ghost;
  pile16 *pile;
  ghost = data_ghost16(data);
  pile = ghost_pile16(ghost);
  if (--pile->chunks > 0)
  {
    if (!pile_has_free16(pile))
    {
      pile16 *prev;
      if ((prev = pool->squot) != NULL)
      {
        prev->nextsquot = pile;
        pile->prevsquot = prev;
      }
      pool->squot = pile;
    }
    pile_set_free16(pile, ghost);
  }
  else
  {
    pile16 *next, *prev;
    size_t used;
    if (pile_has_free16(pile))
    {
      ASSERT16(pool->squot != NULL);
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free16(pile);
    }
    if ((next = pile->next) != NULL)
    {
      ASSERT16(pile != pool->head);
      prev = pile->prev, next->prev = prev;
      if (prev != NULL) prev->next = next; else pool->tail = next;
      util_free(pile);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT16(next == pool->head && next == pool->tail);
        ASSERT16(!pile_has_free16(next));
        if ((pool->flags & POOL_KEEP) == 0)
          remove_pool_head(pool, next);
      }
    }
    else
    {
      ASSERT16(pile == pool->head);
      if ((prev = pile->prev) != NULL || (pool->flags & POOL_KEEP))
        reset_pool_head16(pool, pile, used);
      else
        remove_pool_head(pool, pile);
    }
  }
}

void pool32_back (pool32 *pool, void *data)
{
  ghost32 *ghost;
  pile32 *pile;
  ghost = data_ghost32(data);
  pile = ghost_pile32(ghost);
  if (--pile->chunks > 0)
  {
    if (!pile_has_free32(pile))
    {
      pile32 *prev;
      if ((prev = pool->squot) != NULL)
      {
        prev->nextsquot = pile;
        pile->prevsquot = prev;
      }
      pool->squot = pile;
    }
    pile_set_free32(pile, ghost);
  }
  else
  {
    pile32 *next, *prev;
    size_t used;
    if (pile_has_free32(pile))
    {
      ASSERT32(pool->squot != NULL);
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free32(pile);
    }
    if ((next = pile->next) != NULL)
    {
      ASSERT32(pile != pool->head);
      prev = pile->prev, next->prev = prev;
      if (prev != NULL) prev->next = next; else pool->tail = next;
      util_free(pile);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT32(next == pool->head && next == pool->tail);
        ASSERT32(!pile_has_free32(next));
        if ((pool->flags & POOL_KEEP) == 0)
          remove_pool_head(pool, next);
      }
    }
    else
    {
      ASSERT32(pile == pool->head);
      if ((prev = pile->prev) != NULL || (pool->flags & POOL_KEEP))
        reset_pool_head32(pool, pile, used);
      else
        remove_pool_head(pool, pile);
    }
  }
}

void pool64_back (pool64 *pool, void *data)
{
  ghost64 *ghost;
  pile64 *pile;
  ghost = data_ghost64(data);
  pile = ghost_pile64(ghost);
  if (--pile->chunks > 0)
  {
    if (!pile_has_free64(pile))
    {
      pile64 *prev;
      if ((prev = pool->squot) != NULL)
      {
        prev->nextsquot = pile;
        pile->prevsquot = prev;
      }
      pool->squot = pile;
    }
    pile_set_free64(pile, ghost);
  }
  else
  {
    pile64 *next, *prev;
    size_t used;
    if (pile_has_free64(pile))
    {
      ASSERT64(pool->squot != NULL);
      next = pile->nextsquot, prev = pile->prevsquot;
      if (next != NULL) next->prevsquot = prev; else pool->squot = prev;
      if (prev != NULL) prev->nextsquot = next;
      pile_reset_free64(pile);
    }
    if ((next = pile->next) != NULL)
    {
      ASSERT64(pile != pool->head);
      prev = pile->prev, next->prev = prev;
      if (prev != NULL) prev->next = next; else pool->tail = next;
      util_free(pile);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT64(next == pool->head && next == pool->tail);
        ASSERT64(!pile_has_free64(next));
        if ((pool->flags & POOL_KEEP) == 0)
          remove_pool_head(pool, next);
      }
    }
    else
    {
      ASSERT64(pile == pool->head);
      if ((prev = pile->prev) != NULL || (pool->flags & POOL_KEEP))
        reset_pool_head64(pool, pile, used);
      else
        remove_pool_head(pool, pile);
    }
  }
}

/* pop the last pool chunk */

#define taken_prev_head(pool, ghost, head) (byte_data(ghost) == head->data - pool->unit)

void pool8_pop (pool8 *pool, void *data)
{
  pile8 *pile;
  ghost8 *ghost;
  ghost = data_ghost8(data);
  pile = pool->head;
  if (taken_prev_head(pool, ghost, pile))
  {
    pile->data -= pool->unit;
    ++pile->left;
  }
}

void pool16_pop (pool16 *pool, void *data)
{
  pile16 *pile;
  ghost16 *ghost;
  ghost = data_ghost16(data);
  pile = pool->head;
  if (taken_prev_head(pool, ghost, pile))
  {
    pile->data -= pool->unit;
    ++pile->left;
  }
}

void pool32_pop (pool32 *pool, void *data)
{
  pile32 *pile;
  ghost32 *ghost;
  ghost = data_ghost32(data);
  pile = pool->head;
  if (taken_prev_head(pool, ghost, pile))
  {
    pile->data -= pool->unit;
    ++pile->left;
  }
}

void pool64_pop (pool64 *pool, void *data)
{
  pile64 *pile;
  ghost64 *ghost;
  ghost = data_ghost64(data);
  pile = pool->head;
  if (taken_prev_head(pool, ghost, pile))
  {
    pile->data -= pool->unit;
    ++pile->left;
  }
}

/* pool stats */

#define pool_unused(pool, pile) (pool->units - pile->left - pile->chunks)

void pool8_stats (pool8 *pool, mem_info *info, int append)
{
  pile8 *pile;
  size_t used, chunks = 0, unused = 0, blocks = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (pile = pool->head; pile != NULL; pile = pile->prev)
  {
    ++blocks;
    chunks += pile->chunks;
    unused += pool_unused(pool, pile);
    used = block_used8(pile);
    info->used += used;
    info->left += pile->left;
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost8);
  info->used -= (chunks + unused) * sizeof(ghost8);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(pile8);
}

void pool16_stats (pool16 *pool, mem_info *info, int append)
{
  pile16 *pile;
  size_t used, chunks = 0, unused = 0, blocks = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (pile = pool->head; pile != NULL; pile = pile->prev)
  {
    ++blocks;
    chunks += pile->chunks;
    unused += pool_unused(pool, pile);
    used = block_used16(pile);
    info->used += used;
    info->left += pile->left;
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost16);
  info->used -= (chunks + unused) * sizeof(ghost16);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(pile16);
}

void pool32_stats (pool32 *pool, mem_info *info, int append)
{
  pile32 *pile;
  size_t used, chunks = 0, unused = 0, blocks = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (pile = pool->head; pile != NULL; pile = pile->prev)
  {
    ++blocks;
    chunks += pile->chunks;
    unused += pool_unused(pool, pile);
    used = block_used32(pile);
    info->used += used;
    info->left += pile->left;
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost32);
  info->used -= (chunks + unused) * sizeof(ghost32);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(pile32);
}

void pool64_stats (pool64 *pool, mem_info *info, int append)
{
  pile64 *pile;
  size_t used, chunks = 0, unused = 0, blocks = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (pile = pool->head; pile != NULL; pile = pile->prev)
  {
    ++blocks;
    chunks += pile->chunks;
    unused += pool_unused(pool, pile);
    used = block_used64(pile);
    info->used += used;
    info->left += pile->left;
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost64);
  info->used -= (chunks + unused) * sizeof(ghost64);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(pile64);
}

/* pool iterator */

#define POOL_FIRST_FUNC(func, N)                                                     \
  void * func (pool##N *pool, pool_iter *iter)                                       \
  {                                                                                  \
    for (iter->p##N = pool->tail; iter->p##N != NULL; iter->p##N = iter->p##N->next) \
    {                                                                                \
      if ((iter->units = pool->units - iter->p##N->left) > 0)                        \
      {                                                                              \
        iter->data = block_edge##N(iter->p##N);                                      \
        do {                                                                         \
          --iter->units;                                                             \
          if (ghost_in_use##N(iter->g##N, iter->p##N))                               \
            return ghost_data(iter->g##N);                                           \
          iter->data += pool->unit;                                                  \
        } while (iter->units > 0);                                                   \
      }                                                                              \
    }                                                                                \
    return NULL;                                                                     \
  }

#define POOL_LAST_FUNC(func, N)                                                      \
  void * func (pool##N *pool, pool_iter *iter)                                       \
  {                                                                                  \
    for (iter->p##N = pool->head; iter->p##N != NULL; iter->p##N = iter->p##N->prev) \
    {                                                                                \
      if ((iter->units = pool->units - iter->p##N->left) > 0)                        \
      {                                                                              \
        iter->data = block_edge##N(iter->p##N) + (iter->units - 1) * pool->unit;     \
        do {                                                                         \
          --iter->units;                                                             \
          if (ghost_in_use##N(iter->g##N, iter->p##N))                               \
            return ghost_data(iter->g##N);                                           \
          iter->data -= pool->unit;                                                  \
        } while (iter->units > 0);                                                   \
      }                                                                              \
    }                                                                                \
    return NULL;                                                                     \
  }

POOL_FIRST_FUNC(pool8_first, 8)
POOL_FIRST_FUNC(pool16_first, 16)
POOL_FIRST_FUNC(pool32_first, 32)
POOL_FIRST_FUNC(pool64_first, 64)

POOL_LAST_FUNC(pool8_last, 8)
POOL_LAST_FUNC(pool16_last, 16)
POOL_LAST_FUNC(pool32_last, 32)
POOL_LAST_FUNC(pool64_last, 64)

#define POOL_NEXT_FUNC(func, N)                               \
  void * func (pool##N *pool, pool_iter *iter)                \
  {                                                           \
    for ( ; iter->units > 0; )                                \
    {                                                         \
      --iter->units;                                          \
      iter->data += pool->unit;                               \
      if (ghost_in_use##N(iter->g##N, iter->p##N))            \
        return ghost_data(iter->g##N);                        \
    }                                                         \
    while ((iter->p##N = iter->p##N->next) != NULL)           \
    {                                                         \
      if ((iter->units = pool->units - iter->p##N->left) > 0) \
      {                                                       \
        iter->data = block_edge##N(iter->p##N);               \
        do {                                                  \
          --iter->units;                                      \
          if (ghost_in_use##N(iter->g##N, iter->p##N))        \
            return ghost_data(iter->g##N);                    \
          iter->data += pool->unit;                           \
        } while (iter->units > 0);                            \
      }                                                       \
    }                                                         \
    return NULL;                                              \
  }

#define POOL_PREV_FUNC(func, N)                                                  \
  void * func (pool##N *pool, pool_iter *iter)                                   \
  {                                                                              \
    for ( ; iter->units > 0; )                                                   \
    {                                                                            \
      --iter->units;                                                             \
      iter->data -= pool->unit;                                                  \
      if (ghost_in_use##N(iter->g##N, iter->p##N))                               \
        return ghost_data(iter->g##N);                                           \
    }                                                                            \
    while ((iter->p##N = iter->p##N->prev) != NULL)                              \
    {                                                                            \
      if ((iter->units = pool->units - iter->p##N->left) > 0)                    \
      {                                                                          \
        iter->data = block_edge##N(iter->p##N) + (iter->units - 1) * pool->unit; \
        do {                                                                     \
          --iter->units;                                                         \
          if (ghost_in_use##N(iter->g##N, iter->p##N))                           \
            return ghost_data(iter->g##N);                                       \
          iter->data -= pool->unit;                                              \
        } while (iter->units > 0);                                               \
      }                                                                          \
    }                                                                            \
    return NULL;                                                                 \
  }

POOL_NEXT_FUNC(pool8_next, 8)
POOL_NEXT_FUNC(pool16_next, 16)
POOL_NEXT_FUNC(pool32_next, 32)
POOL_NEXT_FUNC(pool64_next, 64)

POOL_PREV_FUNC(pool8_prev, 8)
POOL_PREV_FUNC(pool16_prev, 16)
POOL_PREV_FUNC(pool32_prev, 32)
POOL_PREV_FUNC(pool64_prev, 64)
