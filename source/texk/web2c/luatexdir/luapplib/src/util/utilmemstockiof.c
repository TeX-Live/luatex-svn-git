
#include "utilmemstockiof.h"

// this is identical to heap iof suite, keep in sync

static size_t stock8_writer (iof *O, iof_mode mode)
{
  stock8 *stock;
  size_t written;
  stock = (stock8 *)O->link;
  switch (mode)
  {
    case IOFFLUSH:
      written = (size_t)iof_size(O);
      stock8_done(stock, O->buf, written);
      O->buf = _stock8_some(stock, 0, &O->space);
      O->pos = O->buf;
      O->end = O->buf + O->space;
      break;
    case IOFWRITE:
      written = (size_t)iof_size(O);
      O->space = written << 1;
      O->buf = stock8_more(stock, O->buf, written, O->space);
      O->pos = O->buf + written;
      O->end = O->buf + O->space;
      return written; // eq (space - written)
    case IOFCLOSE:
    default:
      break;
  }
  return 0;
}

static size_t stock16_writer (iof *O, iof_mode mode)
{
  stock16 *stock;
  size_t written;
  stock = (stock16 *)O->link;
  switch (mode)
  {
    case IOFFLUSH:
      written = (size_t)iof_size(O);
      stock16_done(stock, O->buf, written);
      O->buf = _stock16_some(stock, 0, &O->space);
      O->pos = O->buf;
      O->end = O->buf + O->space;
      break;
    case IOFWRITE:
      written = (size_t)iof_size(O);
      O->space = written << 1;
      O->buf = stock16_more(stock, O->buf, written, O->space);
      O->pos = O->buf + written;
      O->end = O->buf + O->space;
      return written; // eq (space - written)
    case IOFCLOSE:
    default:
      break;
  }
  return 0;
}

static size_t stock32_writer (iof *O, iof_mode mode)
{
  stock32 *stock;
  size_t written;
  stock = (stock32 *)O->link;
  switch (mode)
  {
    case IOFFLUSH:
      written = (size_t)iof_size(O);
      stock32_done(stock, O->buf, written);
      O->buf = _stock32_some(stock, 0, &O->space);
      O->pos = O->buf;
      O->end = O->buf + O->space;
      break;
    case IOFWRITE:
      written = (size_t)iof_size(O);
      O->space = written << 1;
      O->buf = stock32_more(stock, O->buf, written, O->space);
      O->pos = O->buf + written;
      O->end = O->buf + O->space;
      return written; // eq (space - written)
    case IOFCLOSE:
    default:
      break;
  }
  return 0;
}

static size_t stock64_writer (iof *O, iof_mode mode)
{
  stock64 *stock;
  size_t written;
  stock = (stock64 *)O->link;
  switch (mode)
  {
    case IOFFLUSH:
      written = (size_t)iof_size(O);
      stock64_done(stock, O->buf, written);
      O->buf = _stock64_some(stock, 0, &O->space);
      O->pos = O->buf;
      O->end = O->buf + O->space;
      break;
    case IOFWRITE:
      written = (size_t)iof_size(O);
      O->space = written << 1;
      O->buf = stock64_more(stock, O->buf, written, O->space);
      O->pos = O->buf + written;
      O->end = O->buf + O->space;
      return written; // eq (space - written)
    case IOFCLOSE:
    default:
      break;
  }
  return 0;
}

/* buffer init (made once) */

iof * stock8_buffer_init (stock8 *stock, iof *O)
{
  void *data;
  size_t space;
  data = stock8_some(stock, 0, &space);
  if (iof_writer(O, (void *)stock, stock8_writer, data, space) == NULL) // sanity
    return NULL;
  return O;
}

iof * stock16_buffer_init (stock16 *stock, iof *O)
{
  void *data;
  size_t space;
  data = stock16_some(stock, 0, &space);
  if (iof_writer(O, (void *)stock, stock16_writer, data, space) == NULL)
    return NULL;
  return O;
}

iof * stock32_buffer_init (stock32 *stock, iof *O)
{
  void *data;
  size_t space;
  data = stock32_some(stock, 0, &space);
  if (iof_writer(O, (void *)stock, stock32_writer, data, space) == NULL)
    return NULL;
  return O;
}

iof * stock64_buffer_init (stock64 *stock, iof *O)
{
  void *data;
  size_t space;
  data = stock64_some(stock, 0, &space);
  if (iof_writer(O, (void *)stock, stock64_writer, data, space) == NULL)
    return NULL;
  return O;
}

/* buffer for some */

iof * stock8_buffer_some (stock8 *stock, iof *O, size_t atleast)
{
  O->buf = _stock8_some(stock, atleast, &O->space);
  O->pos = O->buf;
  O->end = O->buf + O->space;
  return O;
}

iof * stock16_buffer_some (stock16 *stock, iof *O, size_t atleast)
{
  O->buf = _stock16_some(stock, atleast, &O->space);
  O->pos = O->buf;
  O->end = O->buf + O->space;
  return O;
}

iof * stock32_buffer_some (stock32 *stock, iof *O, size_t atleast)
{
  O->buf = _stock32_some(stock, atleast, &O->space);
  O->pos = O->buf;
  O->end = O->buf + O->space;
  return O;
}

iof * stock64_buffer_some (stock64 *stock, iof *O, size_t atleast)
{
  O->buf = _stock64_some(stock, atleast, &O->space);
  O->pos = O->buf;
  O->end = O->buf + O->space;
  return O;
}
