
#include "utilmemstock.h"
#include "utilmemallc.h"

/* alloc */

#define ream_alloc8(stock,  space) ((ream8 *)((stock->flags & STOCK_ZERO)  ? util_calloc(1, sizeof(ream8)  + space * sizeof(uint8_t)) : util_malloc(sizeof(ream8)  + space * sizeof(uint8_t))))
#define ream_alloc16(stock, space) ((ream16 *)((stock->flags & STOCK_ZERO) ? util_calloc(1, sizeof(ream16) + space * sizeof(uint8_t)) : util_malloc(sizeof(ream16) + space * sizeof(uint8_t))))
#define ream_alloc32(stock, space) ((ream32 *)((stock->flags & STOCK_ZERO) ? util_calloc(1, sizeof(ream32) + space * sizeof(uint8_t)) : util_malloc(sizeof(ream32) + space * sizeof(uint8_t))))
#define ream_alloc64(stock, space) ((ream64 *)((stock->flags & STOCK_ZERO) ? util_calloc(1, sizeof(ream64) + space * sizeof(uint8_t)) : util_malloc(sizeof(ream64) + space * sizeof(uint8_t))))

#define ream_free(ream) util_free(ream)

/* ghost to block */

#define ghost_ream8(ghost) ghost_block8(ghost, ream8)
#define ghost_ream16(ghost) ghost_block16(ghost, ream16)
#define ghost_ream32(ghost) ghost_block32(ghost, ream32)
#define ghost_ream64(ghost) ghost_block64(ghost, ream64)

/* block reset */

#define reset_stock_head8(stock, ream, used) \
  ((used = block_used8(ream)), (ream->data -= used), ((stock->flags & STOCK_ZERO) ? (memset(ream->data, 0, used), 0) : 0), (ream->left += (uint8_t)used), (ream->unused = 0))
#define reset_stock_head16(stock, ream, used) \
  ((used = block_used16(ream)), (ream->data -= used), ((stock->flags & STOCK_ZERO) ? (memset(ream->data, 0, used), 0) : 0), (ream->left += (uint16_t)used), (ream->unused = 0))
#define reset_stock_head32(stock, ream, used) \
  ((used = block_used32(ream)), (ream->data -= used), ((stock->flags & STOCK_ZERO) ? (memset(ream->data, 0, used), 0) : 0), (ream->left += (uint32_t)used), (ream->unused = 0))
#define reset_stock_head64(stock, ream, used) \
  ((used = block_used64(ream)), (ream->data -= used), ((stock->flags & STOCK_ZERO) ? (memset(ream->data, 0, used), 0) : 0), (ream->left += (uint64_t)used), (ream->unused = 0))

#define remove_stock_head(stock, ream) (stock->head = NULL, ream_free(ream))

/* init stock */

stock8 * stock8_init (stock8 *stock, uint8_t space, uint8_t large, uint8_t flags)
{
  align_space8(space);
  if (large > space) large = space;
  stock->head = NULL;
  stock->space = space;
  stock->large = large;
  stock->flags = flags;
  return stock;
}

stock16 * stock16_init (stock16 *stock, uint16_t space, uint16_t large, uint8_t flags)
{
  stock->head = NULL;
  align_space16(space);
  if (large > space) large = space;
  stock->space = space;
  stock->large = large;
  stock->flags = flags;
  return stock;
}

stock32 * stock32_init (stock32 *stock, uint32_t space, uint32_t large, uint8_t flags)
{
  align_space32(space);
  if (large > space) large = space;
  stock->head = NULL;
  stock->space = space;
  stock->large = large;
  stock->flags = flags;
  return stock;
}

stock64 * stock64_init (stock64 *stock, uint64_t space, uint64_t large, uint8_t flags)
{
  align_space64(space);
  if (large > space) large = space;
  stock->head = NULL;
  stock->space = space;
  stock->large = large;
  stock->flags = flags;
  return stock;
}

/* free stock */

void stock8_free (stock8 *stock)
{
  ream8 *ream, *prev;
  ream = stock->head;
  stock->head = NULL;
  while (ream != NULL)
  {
    prev = ream->prev;
    ream_free(ream);
    ream = prev;
  }
}

void stock16_free (stock16 *stock)
{
  ream16 *ream, *prev;
  ream = stock->head;
  stock->head = NULL;
  while (ream != NULL)
  {
    prev = ream->prev;
    ream_free(ream);
    ream = prev;
  }
}

void stock32_free (stock32 *stock)
{
  ream32 *ream, *prev;
  ream = stock->head;
  stock->head = NULL;
  while (ream != NULL)
  {
    prev = ream->prev;
    ream_free(ream);
    ream = prev;
  }
}

void stock64_free (stock64 *stock)
{
  ream64 *ream, *prev;
  ream = stock->head;
  stock->head = NULL;
  while (ream != NULL)
  {
    prev = ream->prev;
    ream_free(ream);
    ream = prev;
  }
}

/* clear stock */

void stock8_clear (stock8 *stock)
{
  ream8 *ream, *prev;
  size_t used;
  if ((ream = stock->head) == NULL)
    return;
  prev = ream->prev;
  ream->prev = NULL;
  reset_stock_head8(stock, ream, used);
  for (; prev != NULL; prev = ream)
  {
    ream = prev->prev;
    ream_free(prev);
  }
}

void stock16_clear (stock16 *stock)
{
  ream16 *ream, *prev;
  size_t used;
  if ((ream = stock->head) == NULL)
    return;
  prev = ream->prev;
  ream->prev = NULL;
  reset_stock_head16(stock, ream, used);
  for (; prev != NULL; prev = ream)
  {
    ream = prev->prev;
    ream_free(prev);
  }
}

void stock32_clear (stock32 *stock)
{
  ream32 *ream, *prev;
  size_t used;
  if ((ream = stock->head) == NULL)
    return;
  prev = ream->prev;
  ream->prev = NULL;
  reset_stock_head32(stock, ream, used);
  for (; prev != NULL; prev = ream)
  {
    ream = prev->prev;
    ream_free(prev);
  }
}

void stock64_clear (stock64 *stock)
{
  ream64 *ream, *prev;
  size_t used;
  if ((ream = stock->head) == NULL)
    return;
  prev = ream->prev;
  ream->prev = NULL;
  reset_stock_head64(stock, ream, used);
  for (; prev != NULL; prev = ream)
  {
    ream = prev->prev;
    ream_free(prev);
  }
}

/* stock head */

void stock8_head (stock8 *stock)
{
  ream8 *ream;
  stock->head = ream = ream_alloc8(stock, stock->space);
  ream->next = NULL, ream->prev = NULL;                    
  ream->data = block_edge8(ream);
  ream->left = block_left8(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;                      
}

void stock16_head (stock16 *stock)
{
  ream16 *ream;
  stock->head = ream = ream_alloc16(stock, stock->space);
  ream->next = NULL, ream->prev = NULL;                    
  ream->data = block_edge16(ream);
  ream->left = block_left16(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;                      
}

void stock32_head (stock32 *stock)
{
  ream32 *ream;
  stock->head = ream = ream_alloc32(stock, stock->space);
  ream->next = NULL, ream->prev = NULL;                    
  ream->data = block_edge32(ream);
  ream->left = block_left32(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;                      
}

void stock64_head (stock64 *stock)
{
  ream64 *ream;
  stock->head = ream = ream_alloc64(stock, stock->space);
  ream->next = NULL, ream->prev = NULL;                    
  ream->data = block_edge64(ream);
  ream->left = block_left64(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;                      
}

/* new stock head */

static ream8 * stock8_new (stock8 *stock)
{
  ream8 *ream, *head;
  ream = ream_alloc8(stock, stock->space);
  head = stock->head;
  ream->next = NULL, ream->prev = head;
  head->next = ream, stock->head = ream;      
  ream->data = block_edge8(ream);
  ream->left = block_left8(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

static ream16 * stock16_new (stock16 *stock)
{
  ream16 *ream, *head;
  ream = ream_alloc16(stock, stock->space);
  head = stock->head;
  ream->next = NULL, ream->prev = head;
  head->next = ream, stock->head = ream;      
  ream->data = block_edge16(ream);
  ream->left = block_left16(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

static ream32 * stock32_new (stock32 *stock)
{
  ream32 *ream, *head;
  ream = ream_alloc32(stock, stock->space);
  head = stock->head;
  ream->next = NULL, ream->prev = head;
  head->next = ream, stock->head = ream;      
  ream->data = block_edge32(ream);
  ream->left = block_left32(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

static ream64 * stock64_new (stock64 *stock)
{
  ream64 *ream, *head;
  ream = ream_alloc64(stock, stock->space);
  head = stock->head;
  ream->next = NULL, ream->prev = head;
  head->next = ream, stock->head = ream;      
  ream->data = block_edge64(ream);
  ream->left = block_left64(ream, stock->space);
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

/* stock sole ream */

static ream8 * stock8_sole(stock8 *stock, size_t size)
{
  ream8 *ream, *head, *prev;
  ream = ream_alloc8(stock, size);
  head = stock->head;
  if ((prev = head->prev) != NULL)
    ream->prev = prev, prev->next = ream;
  else
    ream->prev = NULL;
  ream->next = head, head->prev = ream;
  ream->data = block_edge8(ream);
  ream->left = 0;
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

static ream16 * stock16_sole(stock16 *stock, size_t size)
{
  ream16 *ream, *head, *prev;
  ream = ream_alloc16(stock, size);
  head = stock->head;
  if ((prev = head->prev) != NULL)
    ream->prev = prev, prev->next = ream;
  else
    ream->prev = NULL;
  ream->next = head, head->prev = ream;
  ream->data = block_edge16(ream);
  ream->left = 0;
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

static ream32 * stock32_sole(stock32 *stock, size_t size)
{
  ream32 *ream, *head, *prev;
  ream = ream_alloc32(stock, size);
  head = stock->head;
  if ((prev = head->prev) != NULL)
    ream->prev = prev, prev->next = ream;
  else
    ream->prev = NULL;
  ream->next = head, head->prev = ream;
  ream->data = block_edge32(ream);
  ream->left = 0;
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

static ream64 * stock64_sole(stock64 *stock, size_t size)
{
  ream64 *ream, *head, *prev;
  ream = ream_alloc64(stock, size);
  head = stock->head;
  if ((prev = head->prev) != NULL)
    ream->prev = prev, prev->next = ream;
  else
    ream->prev = NULL;
  ream->next = head, head->prev = ream;
  ream->data = block_edge64(ream);
  ream->left = 0;
  ream->chunks = 0, ream->unused = 0;
  return ream;
}

/* take from stock */

#define ream_next8(ream, size)  (ream->data += size, ream->left -= (uint8_t)size,  ++ream->chunks)
#define ream_next16(ream, size) (ream->data += size, ream->left -= (uint16_t)size, ++ream->chunks)
#define ream_next32(ream, size) (ream->data += size, ream->left -= (uint32_t)size, ++ream->chunks)
#define ream_next64(ream, size) (ream->data += size, ream->left -= (uint64_t)size, ++ream->chunks)

// for sole blocks, ream->left is permanently zero; we cannot store the actual size_t there
#define ream_last8(ream, size)  (ream->data += size, ream->chunks = 1)
#define ream_last16(ream, size) (ream->data += size, ream->chunks = 1)
#define ream_last32(ream, size) (ream->data += size, ream->chunks = 1)
#define ream_last64(ream, size) (ream->data += size, ream->chunks = 1)

void * _stock8_take (stock8 *stock, size_t size)
{
  ream8 *ream;
  ghost8 *ghost;
  ream = stock->head;
  size += sizeof(ghost8);
  align_size8(size);
  if (size <= ream->left)
  {
    ghost_next8(ream, ghost);
    ream_next8(ream, size);
  }
  else if (take_new_block8(stock, ream8, ream, size))
  {
    ream = stock8_new(stock);
    ghost_next8(ream, ghost);
    ream_next8(ream, size);
  }
  else
  {
    ream = stock8_sole(stock, size);
    ghost_next8(ream, ghost);
    ream_last8(ream, size);
  }
  return ghost_data(ghost);
}

void * _stock16_take (stock16 *stock, size_t size)
{
  ream16 *ream;
  ghost16 *ghost;
  ream = stock->head;
  size += sizeof(ghost16);
  align_size16(size);
  if (size <= ream->left)
  {
    ghost_next16(ream, ghost);
    ream_next16(ream, size);
  }
  else if (take_new_block16(stock, ream16, ream, size))
  {
    ream = stock16_new(stock);
    ghost_next16(ream, ghost);
    ream_next16(ream, size);
  }
  else
  {
    ream = stock16_sole(stock, size);
    ghost_next16(ream, ghost);
    ream_last16(ream, size);
  }
  return ghost_data(ghost);
}

void * _stock32_take (stock32 *stock, size_t size)
{
  ream32 *ream;
  ghost32 *ghost;
  ream = stock->head;
  size += sizeof(ghost32);
  align_size32(size);
  if (size <= ream->left)
  {
    ghost_next32(ream, ghost);
    ream_next32(ream, size);
  }
  else if (take_new_block32(stock, ream32, ream, size))
  {
    ream = stock32_new(stock);
    ghost_next32(ream, ghost);
    ream_next32(ream, size);
  }
  else
  {
    ream = stock32_sole(stock, size);
    ghost_next32(ream, ghost);
    ream_last32(ream, size);
  }
  return ghost_data(ghost);
}

void * _stock64_take (stock64 *stock, size_t size)
{
  ream64 *ream;
  ghost64 *ghost;
  ream = stock->head;
  size += sizeof(ghost64);
  align_size64(size);
  if (size <= ream->left)
  {
    ghost_next64(ream, ghost);
    ream_next64(ream, size);
  }
  else if (take_new_block64(stock, ream64, ream, size))
  {
    ream = stock64_new(stock);
    ghost_next64(ream, ghost);
    ream_next64(ream, size);
  }
  else
  {
    ream = stock64_sole(stock, size);
    ghost_next64(ream, ghost);
    ream_last64(ream, size);
  }
  return ghost_data(ghost);
}

/* put back to stock */

void stock8_back (stock8 *stock, void *data)
{
  ghost8 *ghost;
  ream8 *ream;
  ghost = data_ghost8(data);
  ream = ghost_ream8(ghost);
  ++ream->unused;
  if (--ream->chunks == 0)
  {
    ream8 *next, *prev;
    size_t used;
    if ((next = ream->next) != NULL)
    {
      ASSERT8(ream != stock->head);
      prev = ream->prev, next->prev = prev;
      if (prev != NULL) prev->next = next;
      util_free(ream);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT8(next == stock->head);
        if ((stock->flags & STOCK_KEEP) == 0)
          remove_stock_head(stock, next);
      }
    }
    else
    {
      ASSERT8(ream == stock->head);
      if ((prev = ream->prev) != NULL || (stock->flags & STOCK_KEEP))
        reset_stock_head8(stock, ream, used);
      else
        remove_stock_head(stock, ream);
    }
  }
}

void stock16_back (stock16 *stock, void *data)
{
  ghost16 *ghost;
  ream16 *ream;
  ghost = data_ghost16(data);
  ream = ghost_ream16(ghost);
  ++ream->unused;
  if (--ream->chunks == 0)
  {
    ream16 *next, *prev;
    size_t used;
    if ((next = ream->next) != NULL)
    {
      ASSERT16(ream != stock->head);
      prev = ream->prev, next->prev = prev;
      if (prev != NULL) prev->next = next;
      util_free(ream);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT16(next == stock->head);
        if ((stock->flags & STOCK_KEEP) == 0)
          remove_stock_head(stock, next);
      }
    }
    else
    {
      ASSERT16(ream == stock->head);
      if ((prev = ream->prev) != NULL || (stock->flags & STOCK_KEEP))
        reset_stock_head16(stock, ream, used);
      else
        remove_stock_head(stock, ream);
    }
  }
}

void stock32_back (stock32 *stock, void *data)
{
  ghost32 *ghost;
  ream32 *ream;
  ghost = data_ghost32(data);
  ream = ghost_ream32(ghost);
  ++ream->unused;
  if (--ream->chunks == 0)
  {
    ream32 *next, *prev;
    size_t used;
    if ((next = ream->next) != NULL)
    {
      ASSERT32(ream != stock->head);
      prev = ream->prev, next->prev = prev;
      if (prev != NULL) prev->next = next;
      util_free(ream);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT32(next == stock->head);
        if ((stock->flags & STOCK_KEEP) == 0)
          remove_stock_head(stock, next);
      }
    }
    else
    {
      ASSERT32(ream == stock->head);
      if ((prev = ream->prev) != NULL || (stock->flags & STOCK_KEEP))
        reset_stock_head32(stock, ream, used);
      else
        remove_stock_head(stock, ream);
    }
  }
}

void stock64_back (stock64 *stock, void *data)
{
  ghost64 *ghost;
  ream64 *ream;
  ghost = data_ghost64(data);
  ream = ghost_ream64(ghost);
  ++ream->unused;
  if (--ream->chunks == 0)
  {
    ream64 *next, *prev;
    size_t used;
    if ((next = ream->next) != NULL)
    {
      ASSERT64(ream != stock->head);
      prev = ream->prev, next->prev = prev;
      if (prev != NULL) prev->next = next;
      util_free(ream);
      if (next->next == NULL && prev == NULL && next->chunks == 0)
      {
        ASSERT64(next == stock->head);
        if ((stock->flags & STOCK_KEEP) == 0)
          remove_stock_head(stock, next);
      }
    }
    else
    {
      ASSERT64(ream == stock->head);
      if ((prev = ream->prev) != NULL || (stock->flags & STOCK_KEEP))
        reset_stock_head64(stock, ream, used);
      else
        remove_stock_head(stock, ream);
    }
  }
}

/* pop the last stock item */

#define taken_from_head(takenghost, head) (takenghost == head->dataghost)
#define taken_from_sole(takenghost, head, sole) ((sole = head->prev) != NULL && takenghost == sole->dataghost)

#define taken_prev_head(takenghost, head, size) (takenghost == head->dataghost)
#define taken_prev_sole(takenghost, head, sole, size) ((sole = head->prev) != NULL && takenghost == sole->dataghost)

void stock8_pop (stock8 *stock, void *taken, size_t size)
{
  ream8 *ream, *head, *prev;
  ghost8 *takenghost;
  takenghost = data_ghost8(taken);
  head = stock->head;
  size += sizeof(ghost8);
  align_size8(size);
  if (taken_prev_head(takenghost, head, size))
  {
    head->data -= size;
    head->left += (uint8_t)size;
    --head->chunks;
  }
  else if (taken_prev_sole(takenghost, head, ream, size))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

void stock16_pop (stock16 *stock, void *taken, size_t size)
{
  ream16 *ream, *head, *prev;
  ghost16 *takenghost;
  takenghost = data_ghost16(taken);
  head = stock->head;
  size += sizeof(ghost16);
  align_size16(size);
  if (taken_prev_head(takenghost, head, size))
  {
    head->data -= size;
    head->left += (uint16_t)size;
    --head->chunks;
  }
  else if (taken_prev_sole(takenghost, head, ream, size))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

void stock32_pop (stock32 *stock, void *taken, size_t size)
{
  ream32 *ream, *head, *prev;
  ghost32 *takenghost;
  takenghost = data_ghost32(taken);
  head = stock->head;
  size += sizeof(ghost32);
  align_size32(size);
  if (taken_prev_head(takenghost, head, size))
  {
    head->data -= size;
    head->left += (uint32_t)size;
    --head->chunks;
  }
  else if (taken_prev_sole(takenghost, head, ream, size))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

void stock64_pop (stock64 *stock, void *taken, size_t size)
{
  ream64 *ream, *head, *prev;
  ghost64 *takenghost;
  takenghost = data_ghost64(taken);
  head = stock->head;
  size += sizeof(ghost64);
  align_size64(size);
  if (taken_prev_head(takenghost, head, size))
  {
    head->data -= size;
    head->left += (uint64_t)size;
    --head->chunks;
  }
  else if (taken_prev_sole(takenghost, head, ream, size))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

/* stock buffer */

void * _stock8_some (stock8 *stock, size_t size, size_t *pspace)
{
  ream8 *ream;
  ghost8 *ghost;
  ream = stock->head;
  size += sizeof(ghost8);
  align_size8(size);
  if (size <= ream->left)
  {
    ghost_next8(ream, ghost);
    *pspace = ream->left - sizeof(ghost8);
  }
  else if (take_new_block8(stock, ream8, ream, size))
  {
    ream = stock8_new(stock);
    ghost_next8(ream, ghost);
    *pspace = ream->left - sizeof(ghost8);
  }
  else
  {
    ream = stock8_sole(stock, size);
    ghost_next8(ream, ghost);
    *pspace = size - sizeof(ghost8);
  }
  return ghost_data(ghost);
}

void * _stock16_some (stock16 *stock, size_t size, size_t *pspace)
{
  ream16 *ream;
  ghost16 *ghost;
  ream = stock->head;
  size += sizeof(ghost16);
  align_size16(size);
  if (size <= ream->left)
  {
    ghost_next16(ream, ghost);
    *pspace = ream->left - sizeof(ghost16);
  }
  else if (take_new_block16(stock, ream16, ream, size))
  {
    ream = stock16_new(stock);
    ghost_next16(ream, ghost);
    *pspace = ream->left - sizeof(ghost16);
  }
  else
  {
    ream = stock16_sole(stock, size);
    ghost_next16(ream, ghost);
    *pspace = size - sizeof(ghost16);
  }
  return ghost_data(ghost);
}

void * _stock32_some (stock32 *stock, size_t size, size_t *pspace)
{
  ream32 *ream;
  ghost32 *ghost;
  ream = stock->head;
  size += sizeof(ghost32);
  align_size32(size);
  if (size <= ream->left)
  {
    ghost_next32(ream, ghost);
    *pspace = ream->left - sizeof(ghost32);
  }
  else if (take_new_block32(stock, ream32, ream, size))
  {
    ream = stock32_new(stock);
    ghost_next32(ream, ghost);
    *pspace = ream->left - sizeof(ghost32);
  }
  else
  {
    ream = stock32_sole(stock, size);
    ghost_next32(ream, ghost);
    *pspace = size - sizeof(ghost32);
  }
  return ghost_data(ghost);
}

void * _stock64_some (stock64 *stock, size_t size, size_t *pspace)
{
  ream64 *ream;
  ghost64 *ghost;
  ream = stock->head;
  size += sizeof(ghost64);
  align_size64(size);
  if (size <= ream->left)
  {
    ghost_next64(ream, ghost);
    *pspace = ream->left - sizeof(ghost64);
  }
  else if (take_new_block64(stock, ream64, ream, size))
  {
    ream = stock64_new(stock);
    ghost_next64(ream, ghost);
    *pspace = ream->left - sizeof(ghost64);
  }
  else
  {
    ream = stock64_sole(stock, size);
    ghost_next64(ream, ghost);
    *pspace = size - sizeof(ghost64);
  }
  return ghost_data(ghost);
}

void * stock8_more (stock8 *stock, void *taken, size_t written, size_t size)
{
  ream8 *ream, *prev, *tail;
  ghost8 *ghost, *takenghost;
  void *data;
  takenghost = data_ghost8(taken);
  size += sizeof(ghost8);
  align_size8(size);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    if (size <= ream->left)
    {
      ghost_next8(ream, ghost);
      data = ghost_data(ghost);
    }
    else if (take_new_block8(stock, ream8, ream, size))
    {
      ream = stock8_new(stock);
      ghost_next8(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
    else
    {
      ream = stock8_sole(stock, size);
      ghost_next8(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
  }
  else if (taken_from_sole(takenghost, ream, prev))
  {
    ream = stock8_sole(stock, size); // prev == ream->prev
    if ((tail = prev->prev) != NULL)
      ream->prev = tail, tail->next = ream;
    else
      ream->prev = NULL;
    ghost_next8(ream, ghost);
    data = ghost_data(ghost);
    memcpy(data, taken, written);
    ream_free(prev);
  }
  else
    return NULL; // invalid use
  return data;
}

void * stock16_more (stock16 *stock, void *taken, size_t written, size_t size)
{
  ream16 *ream, *prev, *tail;
  ghost16 *ghost, *takenghost;
  void *data;
  takenghost = data_ghost16(taken);
  size += sizeof(ghost16);
  align_size16(size);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    if (size <= ream->left)
    {
      ghost_next16(ream, ghost);
      data = ghost_data(ghost);
    }
    else if (take_new_block16(stock, ream16, ream, size))
    {
      ream = stock16_new(stock);
      ghost_next16(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
    else
    {
      ream = stock16_sole(stock, size);
      ghost_next16(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
  }
  else if (taken_from_sole(takenghost, ream, prev))
  {
    ream = stock16_sole(stock, size);
    if ((tail = prev->prev) != NULL)
      ream->prev = tail, tail->next = ream;
    else
      ream->prev = NULL;
    ghost_next16(ream, ghost);
    data = ghost_data(ghost);
    memcpy(data, taken, written);
    ream_free(prev);
  }
  else
    return NULL;
  return data;
}

void * stock32_more (stock32 *stock, void *taken, size_t written, size_t size)
{
  ream32 *ream, *prev, *tail;
  ghost32 *ghost, *takenghost;
  void *data;
  takenghost = data_ghost32(taken);
  size += sizeof(ghost32);
  align_size32(size);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    if (size <= ream->left)
    {
      ghost_next32(ream, ghost);
      data = ghost_data(ghost);
    }
    else if (take_new_block32(stock, ream32, ream, size))
    {
      ream = stock32_new(stock);
      ghost_next32(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
    else
    {
      ream = stock32_sole(stock, size);
      ghost_next32(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
  }
  else if (taken_from_sole(takenghost, ream, prev))
  {
    ream = stock32_sole(stock, size);
    if ((tail = prev->prev) != NULL)
      ream->prev = tail, tail->next = ream;
    else
      ream->prev = NULL;
    ghost_next32(ream, ghost);
    data = ghost_data(ghost);
    memcpy(data, taken, written);
    ream_free(prev);
  }
  else
    return NULL;
  return data;
}

void * stock64_more (stock64 *stock, void *taken, size_t written, size_t size)
{
  ream64 *ream, *prev, *tail;
  ghost64 *ghost, *takenghost;
  void *data;
  takenghost = data_ghost64(taken);
  size += sizeof(ghost64);
  align_size64(size);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    if (size <= ream->left)
    {
      ghost_next64(ream, ghost);
      data = ghost_data(ghost);
    }
    else if (take_new_block64(stock, ream64, ream, size))
    {
      ream = stock64_new(stock);
      ghost_next64(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
    else
    {
      ream = stock64_sole(stock, size);
      ghost_next64(ream, ghost);
      data = ghost_data(ghost);
      memcpy(data, taken, written);
    }
  }
  else if (taken_from_sole(takenghost, ream, prev))
  {
    ream = stock64_sole(stock, size);
    if ((tail = prev->prev) != NULL)
      ream->prev = tail, tail->next = ream;
    else
      ream->prev = NULL;
    ghost_next64(ream, ghost);
    data = ghost_data(ghost);
    memcpy(data, taken, written);
    ream_free(prev);
  }
  else
    return NULL;
  return data;
}

void stock8_done (stock8 *stock, void *taken, size_t written)
{
  ream8 *ream;
  ghost8 *takenghost;
  takenghost = data_ghost8(taken);
  written += sizeof(ghost8);
  align_size8(written);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    ream_next8(ream, written);
  }
  else if (taken_from_sole(takenghost, ream, ream))
  {
    ream_last8(ream, written);
  }
}

void stock16_done (stock16 *stock, void *taken, size_t written)
{
  ream16 *ream;
  ghost16 *takenghost;
  takenghost = data_ghost16(taken);
  written += sizeof(ghost16);
  align_size16(written);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    ream_next16(ream, written);
  }
  else if (taken_from_sole(takenghost, ream, ream))
  {
    ream_last16(ream, written);
  }
}

void stock32_done (stock32 *stock, void *taken, size_t written)
{
  ream32 *ream;
  ghost32 *takenghost;
  takenghost = data_ghost32(taken);
  written += sizeof(ghost32);
  align_size32(written);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    ream_next32(ream, written);
  }
  else if (taken_from_sole(takenghost, ream, ream))
  {
    ream_last32(ream, written);
  }
}

void stock64_done (stock64 *stock, void *taken, size_t written)
{
  ream64 *ream;
  ghost64 *takenghost;
  takenghost = data_ghost64(taken);
  written += sizeof(ghost64);
  align_size64(written);
  ream = stock->head;
  if (taken_from_head(takenghost, ream))
  {
    ream_next64(ream, written);
  }
  else if (taken_from_sole(takenghost, ream, ream))
  {
    ream_last64(ream, written);
  }
}

/* giveup */

void stock8_giveup (stock8 *stock, void *taken)
{
  ream8 *head, *ream, *prev;
  ghost8 *takenghost;
  takenghost = data_ghost8(taken);
  head = stock->head;
  if (taken_from_sole(takenghost, head, ream))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

void stock16_giveup (stock16 *stock, void *taken)
{
  ream16 *head, *ream, *prev;
  ghost16 *takenghost;
  takenghost = data_ghost16(taken);
  head = stock->head;
  if (taken_from_sole(takenghost, head, ream))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

void stock32_giveup (stock32 *stock, void *taken)
{
  ream32 *head, *ream, *prev;
  ghost32 *takenghost;
  takenghost = data_ghost32(taken);
  head = stock->head;
  if (taken_from_sole(takenghost, head, ream))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

void stock64_giveup (stock64 *stock, void *taken)
{
  ream64 *head, *ream, *prev;
  ghost64 *takenghost;
  takenghost = data_ghost64(taken);
  head = stock->head;
  if (taken_from_sole(takenghost, head, ream))
  {
    if ((prev = ream->prev) != NULL)
      head->prev = prev, prev->next = head;
    else
      head->prev = NULL;
    ream_free(ream);
  }
}

/* stock empty */

int stock8_empty (stock8 *stock)
{
  ream8 *ream;
  return head_block_empty(stock, ream);
}

int stock16_empty (stock16 *stock)
{
  ream16 *ream;
  return head_block_empty(stock, ream);
}

int stock32_empty (stock32 *stock)
{
  ream32 *ream;
  return head_block_empty(stock, ream);
}

int stock64_empty (stock64 *stock)
{
  ream64 *ream;
  return head_block_empty(stock, ream);
}

/* stock stats */

// stat() is supposed to work incrementaly with various allocators, so we have to record ghosts of various sizes
// here to have a single mem_info struct and single show function

void stock8_stats (stock8 *stock, mem_info *info, int append)
{
  ream8 *ream;
  size_t used, chunks = 0, unused = 0, blocks = 0, singles = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (ream = stock->head; ream != NULL; ream = ream->prev)
  {
    ++blocks;
    chunks += ream->chunks;
    unused += ream->unused;
    used = block_used8(ream);
    info->used += used;
    info->left += ream->left;
    if (ream->chunks == 1 && ream->left == 0)
    {
      ++singles;
      info->singleused += used;
    }
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost8);
  info->used -= (chunks + unused) * sizeof(ghost8);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(ream8);
  info->singles += singles;
  info->singleghosts += singles * sizeof(ream8);
}

void stock16_stats (stock16 *stock, mem_info *info, int append)
{
  ream16 *ream;
  size_t used, chunks = 0, unused = 0, blocks = 0, singles = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (ream = stock->head; ream != NULL; ream = ream->prev)
  {
    ++blocks;
    chunks += ream->chunks;
    unused += ream->unused;
    used = block_used16(ream);
    info->used += used;
    info->left += ream->left;
    if (ream->chunks == 1 && ream->left == 0)
    {
      ++singles;
      info->singleused += used;
    }
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost16);
  info->used -= (chunks + unused) * sizeof(ghost16);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(ream16);
  info->singles += singles;
  info->singleghosts += singles * sizeof(ream16);
}

void stock32_stats (stock32 *stock, mem_info *info, int append)
{
  ream32 *ream;
  size_t used, chunks = 0, unused = 0, blocks = 0, singles = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (ream = stock->head; ream != NULL; ream = ream->prev)
  {
    ++blocks;
    chunks += ream->chunks;
    unused += ream->unused;
    used = block_used32(ream);
    info->used += used;
    info->left += ream->left;
    if (ream->chunks == 1 && ream->left == 0)
    {
      ++singles;
      info->singleused += used;
    }
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost32);
  info->used -= (chunks + unused) * sizeof(ghost32);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(ream32);
  info->singles += singles;
  info->singleghosts += singles * sizeof(ream32);
}

void stock64_stats (stock64 *stock, mem_info *info, int append)
{
  ream64 *ream;
  size_t used, chunks = 0, unused = 0, blocks = 0, singles = 0;
  if (!append)
    memset(info, 0, sizeof(mem_info));
  for (ream = stock->head; ream != NULL; ream = ream->prev)
  {
    ++blocks;
    chunks += ream->chunks;
    unused += ream->unused;
    used = block_used64(ream);
    info->used += used;
    info->left += ream->left;
    if (ream->chunks == 1 && ream->left == 0)
    {
      ++singles;
      info->singleused += used;
    }
  }
  info->chunks += chunks + unused;
  info->unused += unused;
  info->ghosts += (chunks + unused) * sizeof(ghost64);
  info->used -= (chunks + unused) * sizeof(ghost64);
  info->blocks += blocks;
  info->blockghosts += blocks * sizeof(ream64);
  info->singles += singles;
  info->singleghosts += singles * sizeof(ream64);
}

