
#include "pplib.h"

#define PPHEAP_BUFFER 0xFFFF // to be tuned
#define PPHEAP_WASTE 0x00FF

#define ppheap_head(heap) ((uint8_t *)((heap) + 1))

static ppheap * ppheap_create (size_t size)
{
	ppheap *heap;
	heap = (ppheap *)pp_malloc(sizeof(ppheap) + size * sizeof(uint8_t));
	heap->size = size;
	heap->space = size;
	heap->data = ppheap_head(heap);
	heap->prev = NULL;
	return heap;
}

ppheap * ppheap_new (void)
{
	return ppheap_create(PPHEAP_BUFFER);
}

void ppheap_free (ppheap *heap)
{
  ppheap *prev;
  do
  {
    prev = heap->prev;
    pp_free(heap);
    heap = prev;
  } while (heap != NULL);
}

void ppheap_renew (ppheap *heap)
{ // free all but first
  ppheap *prev;
  if ((prev = heap->prev) != NULL)
  {
    heap->prev = NULL;
    ppheap_free(prev);
  }
  heap->size = heap->space;
  heap->data = ppheap_head(heap);
}

static ppheap * ppheap_insert_top (ppheap **pheap, size_t size)
{
  ppheap *heap;
  heap = ppheap_create(size);
  heap->prev = (*pheap);
  *pheap = heap;
  return heap;
}

static ppheap * ppheap_insert_sub (ppheap **pheap, size_t size)
{
  ppheap *heap;
  heap = ppheap_create(size);
  heap->prev = (*pheap)->prev;
  (*pheap)->prev = heap;
  return heap;
}

void * ppheap_take (ppheap **pheap, size_t size)
{
	ppheap *heap;
	uint8_t *data;
	heap = *pheap;
	if (size <= heap->size)
	  ;
	else if (heap->size <= PPHEAP_WASTE && size <= (PPHEAP_BUFFER >> 1))
	  heap = ppheap_insert_top(pheap, PPHEAP_BUFFER);
	else
    heap = ppheap_insert_sub(pheap, size);
 	data = heap->data;
 	heap->data += size;
 	heap->size -= size;
 	return (void *)data;
}

/* iof buffer tied to a heap */

static ppheap * ppheap_resize  (ppheap **pheap, size_t size)
{
  ppheap *heap;
  heap = ppheap_create(size);
  heap->prev = (*pheap)->prev;
  memcpy(heap->data, (*pheap)->data, (*pheap)->space); // (*pheap)->size is irrelevant
  pp_free(*pheap);
  *pheap = heap;
  return heap;
}

static size_t ppheap_buffer_handler (iof *O, iof_mode mode)
{
  ppheap **pheap, *heap;
  size_t objectsize, buffersize, neededsize;
  uint8_t *copyfrom;
  switch (mode)
  {
    case IOFWRITE:
      /* apparently more space needed then assumed initsize */
      pheap = (ppheap **)O->link;
      heap = *pheap;
      objectsize = (size_t)(O->buf - heap->data);
      buffersize = (size_t)(O->pos - O->buf);
      neededsize = objectsize + (buffersize << 1);
      if (ppheap_head(heap) < heap->data)
      {
        if (heap->size <= PPHEAP_WASTE && neededsize <= (PPHEAP_BUFFER >> 1))
        {
          heap = ppheap_insert_top(pheap, PPHEAP_BUFFER);
          copyfrom = heap->prev->data;
        }
        else
        {
          heap = ppheap_insert_sub(pheap, neededsize);
          copyfrom = (*pheap)->data;
          O->link = (void *)(&(*pheap)->prev);
        }
        memcpy(heap->data, copyfrom, objectsize + buffersize);
      }
      else
      { /* the buffer was (re)initialized from a new empty heap and occupies its entire space */
        // ASSERT(ppheap_head(heap) == heap->data);
        heap = ppheap_resize(pheap, neededsize);
      }
      O->buf = heap->data + objectsize;
      O->pos = O->buf + buffersize;
      O->end = heap->data + heap->size;
      return (size_t)(O->end - O->pos);
    case IOFFLUSH:
      return 0;
    case IOFCLOSE:
      // O->buf = O->pos = O->end = NULL;
      // O->link = NULL;
      return 0;
    default:
      break;
  }
  return 0;
}

iof * ppheap_buffer (ppheap **pheap, size_t objectsize, size_t initsize)
{
  static iof ppheap_buffer_iof = IOF_WRITER_STRUCT(ppheap_buffer_handler, NULL, NULL, 0, 0);
  ppheap *heap;
  size_t size;
  size = objectsize + initsize;
  heap = *pheap;
  if (size <= heap->size)
    ;
  else if (heap->size <= PPHEAP_WASTE && size <= (PPHEAP_BUFFER >> 1))
  {
    heap = ppheap_create(PPHEAP_BUFFER);
    heap->prev = (*pheap);
    *pheap = heap;
  }
  else
  {
    heap = ppheap_create(size);
    heap->prev = (*pheap)->prev;
    (*pheap)->prev = heap;
    pheap = &(*pheap)->prev; // passed to ppheap_buffer_iof.link
  }
  ppheap_buffer_iof.buf = ppheap_buffer_iof.pos = heap->data + objectsize;
  ppheap_buffer_iof.end = heap->data + heap->size;
  ppheap_buffer_iof.link = pheap; // ASSERT(*pheap == heap)
  return &ppheap_buffer_iof;
}

/*
void * ppheap_buffer_data (iof *O, size_t *psize)
{
  ppheap *heap;
  heap = *((ppheap **)(O->link));
  *psize = ppheap_buffer_size(O, heap);
  return heap->data;
}
*/

void * ppheap_flush (iof *O, size_t *psize) // not from explicit ppheap ** !!!
{
  ppheap *heap;
  uint8_t *data;
  heap = *((ppheap **)(O->link));
  *psize = ppheap_buffer_size(O, heap);
  data = heap->data;
  heap->data += *psize;
  heap->size -= *psize;
  // O->buf = O->pos = O->end = NULL;
  // O->link = NULL;
  // iof_close(O);
  return data;
}














