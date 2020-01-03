
#ifndef UTIL_MEM_STOCK_IOF_H
#define UTIL_MEM_STOCK_IOF_H

#include "utilmemstock.h"
#include "utiliof.h"

UTILAPI iof * stock8_buffer_init (stock8 *stock, iof *O);
UTILAPI iof * stock16_buffer_init (stock16 *stock, iof *O);
UTILAPI iof * stock32_buffer_init (stock32 *stock, iof *O);
UTILAPI iof * stock64_buffer_init (stock64 *stock, iof *O);

UTILAPI iof * stock8_buffer_some (stock8 *stock, iof *O, size_t atleast);
UTILAPI iof * stock16_buffer_some (stock16 *stock, iof *O, size_t atleast);
UTILAPI iof * stock32_buffer_some (stock32 *stock, iof *O, size_t atleast);
UTILAPI iof * stock64_buffer_some (stock64 *stock, iof *O, size_t atleast);

#endif