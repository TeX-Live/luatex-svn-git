
#include "utilmem.h"
#include "utilpool.h"

#include "utilfilter.h"
#include "utilbasexxdef.h"
#include "utilflatedef.h"
#include "utillzwdef.h"
#include "utilfpreddef.h"
#include "utilcryptdef.h"

#ifdef IMAGE_FILTER
#  include "utilimage.h"
//#  include "utilimagedef.h"
#  include "utiljbig.h"
#  include "utiljbigdef.h"
#endif

/* file/stream state */

typedef struct file_state {
  size_t length;
  size_t offset;
} file_state;

#define stream_state file_state

#define file_state_init(state, off, len) ((state)->offset = off, (state)->length = len)
#define stream_state_init(state, off, len) ((state)->offset = off, (state)->length = len)

/* filter state */

typedef struct iof_filter {
  IOF_MEMBERS;
  union {
    file_state filestate;
    stream_state streamstate;
    flate_state flatestate;
    lzw_state lzwstate;
    predictor_state predictorstate;
    base16_state base16state;
    base64_state base64state;
    base85_state base85state;
    eexec_state eexecstate;
    runlength_state runlengthstate;
    rc4_state rc4state;
    aes_state aesstate;
#ifdef IMAGE_FILTER
    img_state imagestate;
    jbig_state jbigstate;
#endif
  };
} iof_filter;

#define filter_to_iof(filter) ((iof *)(filter))
#define iof_to_filter(I) ((iof_filter *)(I))

/* filters pool */

#define IOF_FILTER_BUFFER_COUNT 4
#define IOF_FILTER_BUFFER_SIZE  262144
// 1<<18 = 262144

static pool filter_pool = { POOL_INIT_STRUCT(sizeof(iof_filter), IOF_FILTER_BUFFER_COUNT, util_malloc, util_free, POOL_DEFAULTS) };
static pool buffer_pool = { POOL_INIT_STRUCT(IOF_FILTER_BUFFER_SIZE,  IOF_FILTER_BUFFER_COUNT, util_malloc, util_free, POOL_DEFAULTS) };

#define _filter_new() (iof_filter *)pool_out0(&filter_pool)
#define _filter_free pool_in
#define _buffer_new() (uint8_t *)pool_out(&buffer_pool)
#define _buffer_free pool_in

#ifdef DEBUG_FILTERS

static int BUFFER_COUNT = 0, BUFFER_MAX = 0, FILTER_COUNT = 0, FILTER_MAX = 0;

static uint8_t * buffer_new (void)
{
  if (++BUFFER_COUNT > BUFFER_MAX) BUFFER_MAX = BUFFER_COUNT;
  return _buffer_new();
}

static void buffer_free (void *m)
{
  --BUFFER_COUNT;
  _buffer_free(m);
}

static iof_filter * filter_new (void)
{
  iof_filter *filter = _filter_new();
  if (++FILTER_COUNT > FILTER_MAX) FILTER_MAX = FILTER_COUNT;
  //filter->refcount = 0;
  return filter;
}

static void filter_free (void *m)
{
  --FILTER_COUNT;
  _filter_free(m);
}

#else

static uint8_t * buffer_new (void)
{
  uint8_t *buffer = _buffer_new();
  return buffer;
}

#define buffer_free _buffer_free

static iof_filter * filter_new (void)
{
  iof_filter *filter = _filter_new();
  //filter->refcount = 0;
  return filter;
}

#define filter_free _filter_free

#endif

void iof_filters_init (void)
{
#ifdef DEBUG_FILTERS
#define printfsizeof(state) printf("size of " #state ": %lu\n", (unsigned long)sizeof(state))
  printfsizeof(iof_filter);
  printfsizeof(file_state);
  printfsizeof(stream_state);
  printfsizeof(flate_state);
  printfsizeof(lzw_state);
  printfsizeof(predictor_state);
  printfsizeof(base16_state);
  printfsizeof(base64_state);
  printfsizeof(base85_state);
  printfsizeof(eexec_state);
  printfsizeof(runlength_state);
  printfsizeof(rc4_state);
  printfsizeof(aes_state);
#  ifdef IMAGE_FILTER
  printfsizeof(img_state);
  printfsizeof(img);
#  endif
#endif
}

void iof_filters_free (void)
{
#ifdef DEBUG_FILTERS
  printf("FILTER_MAX %d BUFFER_MAX %d\n", FILTER_MAX, BUFFER_MAX);
#endif
  pool_free(&filter_pool);
  pool_free(&buffer_pool);
}

/* freeing filter */

static void filter_close_next (iof_filter *filter)
{
  iof *N;
  if ((N = filter->next) != NULL)
  {
    filter->next = NULL;
    iof_decref(N);
  }
}

/* when filter creation fails, we should take care to destroy the filter but leave ->next intact */

static void filter_clear_next (iof_filter *filter)
{
  iof *N;
  if ((N = filter->next) != NULL)
  {
    filter->next = NULL;
    iof_unref(N);
  }
}

static void filter_free_buffer (iof_filter *filter)
{
  uint8_t *buf;
  if ((buf = filter->buf) != NULL)
  {
    filter->buf = NULL;
    if (filter->flags & IOF_BUFFER_ISALLOC)
      util_free(buf);
    else
      buffer_free(buf);
  }
}

static void filter_close (iof_filter *filter)
{
  filter_close_next(filter);
  filter_free_buffer(filter);
  filter_free(filter);
}

static void filter_discard (iof_filter *filter)
{
  filter_clear_next(filter);
  filter_free_buffer(filter);
  filter_free(filter);
}

/* resize filter buffer */

static size_t resize_buffer_to (iof *O, size_t space)
{
  uint8_t *buf;
  if (O->flags & IOF_BUFFER_ISALLOC)
  {
    if ((buf = (uint8_t *)util_realloc(O->buf, space)) == NULL)
      return 0;
  }
  else
  {
    if ((buf = (uint8_t *)util_malloc(space)) == NULL)
      return 0;
    O->flags |= IOF_BUFFER_ISALLOC;
    memcpy(buf, O->buf, iof_size(O));
    buffer_free(O->buf);
  }
  O->pos = buf + iof_size(O);
  O->end = buf + space;
  O->buf = buf;
  O->space = space;
  return iof_left(O);
}

static size_t resize_buffer (iof *O)
{
  return resize_buffer_to(O, (O)->space << 1);
}

/* reader / writer */

static void filter_reader (iof_filter *filter, iof_handler handler)
{
  uint8_t *buffer = buffer_new();
  iof_reader_buffer(filter, buffer, IOF_FILTER_BUFFER_SIZE);
  filter->more = handler;
}

#ifdef IMAGE_FILTER // not used except here
static void filter_reader_no_buffer (iof_filter *filter, iof_handler handler)
{
  iof_reader_buffer(filter, NULL, 0);
  filter->more = handler;
}
#endif

static void filter_writer (iof_filter *filter, iof_handler handler)
{
  uint8_t *buffer = buffer_new();
  iof_writer_buffer(filter, buffer, IOF_FILTER_BUFFER_SIZE);
  filter->more = handler;
}

/* file filter */

static size_t file_read (iof *I)
{
  size_t bytes, tail;
  if (I->flags & IOF_STOPPED)
    return 0;
  tail = iof_tail(I);
  if ((bytes = tail + fread(I->buf + tail, sizeof(uint8_t), I->space - tail, I->file)) < I->space)
    I->flags |= IOF_STOPPED;
  I->pos = I->buf;
  I->end = I->buf + bytes;
  return bytes;

}

static size_t iofile_read (iof *I, size_t *poffset)
{
  size_t bytes, tail;
  if (I->flags & IOF_STOPPED)
    return 0;
  iof_file_sync(I->iofile, poffset);
  tail = iof_tail(I);
  if ((bytes = tail + iof_file_read(I->buf + tail, sizeof(uint8_t), I->space - tail, I->iofile)) < I->space)
  {
    I->flags |= IOF_STOPPED;
    iof_file_unsync(I->iofile, poffset);
  }
  I->pos = I->buf;
  I->end = I->buf + bytes;
  return bytes;
}

static size_t file_load (iof *I)
{
  size_t bytes, left, tail;
  if (I->flags & IOF_STOPPED)
    return 0;
  tail = iof_tail(I);
  I->pos = I->buf + tail;
  I->end = I->buf + I->space; /* don't assume its done when initializing the filter */
  left = I->space - tail;
  do {
    bytes = fread(I->pos, sizeof(uint8_t), left, I->file);
    I->pos += bytes;
  } while (bytes == left && (left = resize_buffer(I)) > 0);
  I->flags |= IOF_STOPPED;
  return iof_loaded(I);
}

static size_t iofile_load (iof *I, size_t *poffset)
{
  size_t bytes, left, tail;
  if (I->flags & IOF_STOPPED)
    return 0;
  tail = iof_tail(I);
  I->pos = I->buf + tail;
  I->end = I->buf + I->space; /* don't assume its done when initializing the filter */
  left = I->space - tail;
  iof_file_sync(I->iofile, poffset);
  do {
    bytes = iof_file_read(I->pos, sizeof(uint8_t), left, I->iofile);
    I->pos += bytes;
  } while (bytes == left && (left = resize_buffer(I)) > 0);
  I->flags |= IOF_STOPPED;
  iof_file_unsync(I->iofile, poffset);
  return iof_loaded(I);
}

static size_t file_reader (iof *I, iof_mode mode)
{
  switch (mode)
  {
    case IOFREAD:
      return file_read(I);
    case IOFLOAD:
      return file_load(I);
    case IOFCLOSE:
      iof_close_file(I);
      filter_free_buffer(iof_to_filter(I));
      filter_free(I);
      return 0;
    default:
      return 0;
  }
}

static size_t iofile_reader (iof *I, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(I);
  size_t *poffset = &filter->filestate.offset;
  switch (mode)
  {
    case IOFREAD:
      return iofile_read(I, poffset);
    case IOFLOAD:
      return iofile_load(I, poffset);
    case IOFCLOSE:
      iof_reclose_iofile(I, poffset);
      filter_free_buffer(iof_to_filter(I));
      filter_free(I);
      return 0;
    default:
      return 0;
  }
}

static size_t file_write (iof *O, int flush)
{
  size_t bytes;
  if ((bytes = iof_size(O)) > 0)
    if (bytes != fwrite(O->buf, sizeof(uint8_t), bytes, O->file))
      return 0;
  if (flush)
    fflush(O->file);
  O->end = O->buf + O->space; // remains intact actually
  O->pos = O->buf;
  return O->space;
}

static size_t iofile_write (iof *O, size_t *poffset, int flush)
{
  size_t bytes;
  iof_file_sync(O->iofile, poffset);
  if ((bytes = iof_size(O)) > 0)
  {
    if (bytes != iof_file_write(O->buf, sizeof(uint8_t), bytes, O->iofile))
    {
      iof_file_unsync(O->iofile, poffset);
      return 0;
    }
  }
  if (flush)
    iof_file_flush(O->iofile);
  O->end = O->buf + O->space; // remains intact actually
  O->pos = O->buf;
  return O->space;
}

static size_t file_writer (iof *O, iof_mode mode)
{
  switch (mode)
  {
    case IOFWRITE:
      return file_write(O, 0);
    case IOFFLUSH:
      return file_write(O, 1);
    case IOFCLOSE:
      file_write(O, 1);
      iof_close_file(O);
      filter_free_buffer(iof_to_filter(O));
      filter_free(O);
      return 0;
    default:
      return 0;
  }
}

static size_t iofile_writer (iof *O, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(O);
  size_t *poffset = &filter->filestate.offset;
  switch (mode)
  {
    case IOFWRITE:
      return iofile_write(O, poffset, 0);
    case IOFFLUSH:
      return iofile_write(O, poffset, 1);
    case IOFCLOSE:
      iofile_write(O, poffset, 1);
      iof_close_iofile(O, poffset);
      filter_free_buffer(filter);
      filter_free(O);
      return 0;
    default:
      return 0;
  }
}

/* from FILE* */

iof * iof_filter_file_handle_reader (FILE *file)
{
  iof_filter *filter;
  if (file == NULL || (filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, file_reader);
  iof_setup_file(filter, file);
  file_state_init(&filter->filestate, 0, 0);
  return filter_to_iof(filter);
}

iof * iof_filter_file_handle_writer (FILE *file)
{
  iof_filter *filter;
  if (file == NULL || (filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, file_writer);
  iof_setup_file(filter, file);
  file_state_init(&filter->filestate, 0, 0);
  return filter_to_iof(filter);
}

/* from iof_file * */

iof * iof_filter_iofile_reader (iof_file *iofile, size_t offset)
{
  iof_filter *filter;
  if (!iof_file_reopen(iofile) || (filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, iofile_reader);
  iof_setup_iofile(filter, iofile);
  file_state_init(&filter->filestate, offset, 0);
  return filter_to_iof(filter);
}

iof * iof_filter_iofile_writer (iof_file *iofile, size_t offset)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, iofile_writer);
  iof_setup_iofile(filter, iofile);
  file_state_init(&filter->filestate, offset, 0);
  return filter_to_iof(filter);
}

/* from explicit filename */

iof * iof_filter_file_reader (const char *filename)
{
  iof_filter *filter;
  FILE *file;
  if ((file = fopen(filename, "rb")) == NULL || (filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, file_reader);
  iof_setup_file(filter, file);
  file_state_init(&filter->filestate, 0, 0);
  filter->flags |= IOF_CLOSE_FILE; /* close this FILE* when releasing the filter */
  return filter_to_iof(filter);
}

iof * iof_filter_file_writer (const char *filename)
{
  iof_filter *filter;
  FILE *file;
  if ((file = fopen(filename, "wb")) == NULL || (filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, file_writer);
  iof_setup_file(filter, file);
  file_state_init(&filter->filestate, 0, 0);
  filter->flags |= IOF_CLOSE_FILE; /* close this FILE* when releasing the filter */
  return filter_to_iof(filter);
}

/* from string */

static size_t dummy_handler (iof *I, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(I);
  switch (mode)
  {
    case IOFCLOSE:
      filter_free(filter);
      return 0;
    default:
      return 0;
  }
}

iof * iof_filter_string_reader (const void *s, size_t length)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter->buf = filter->pos = (uint8_t *)s;
  filter->end = (uint8_t *)s + length;
  // filter->space = length;
  filter->more = dummy_handler;
  return filter_to_iof(filter);
}

iof * iof_filter_string_writer (const void *s, size_t length)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter->buf = filter->pos = (uint8_t *)s;
  filter->end = (uint8_t *)s + length;
  // filter->space = length;
  filter->more = dummy_handler;
  return filter_to_iof(filter);
}

/* stream */

static size_t file_stream_read (iof *I, size_t *plength)
{
  size_t bytes, tail;
  if (I->flags & IOF_STOPPED || *plength == 0)
    return 0;
  tail = iof_tail(I);
  if (I->space - tail >= *plength)
  {
    bytes = tail + fread(I->buf + tail, sizeof(uint8_t), *plength, I->file);
    I->flags |= IOF_STOPPED;
    *plength = 0;
  }
  else
  {
    bytes = tail + fread(I->buf + tail, sizeof(uint8_t), I->space - tail, I->file);
    *plength -= bytes - tail;
  }
  I->pos = I->buf;
  I->end = I->buf + bytes;
  return bytes;
}

static size_t iofile_stream_read (iof *I, size_t *plength, size_t *poffset)
{
  size_t bytes, tail;
  if (I->flags & IOF_STOPPED || *plength == 0)
    return 0;
  tail = iof_tail(I);
  iof_file_sync(I->iofile, poffset);
  if (I->space - tail >= *plength)
  {
    bytes = tail + iof_file_read(I->buf + tail, sizeof(uint8_t), *plength, I->iofile);
    iof_file_unsync(I->iofile, poffset);
    I->flags |= IOF_STOPPED;
    *plength = 0;
  }
  else
  {
    bytes = tail + iof_file_read(I->buf + tail, sizeof(uint8_t), I->space - tail, I->iofile);
    *plength -= bytes - tail;
  }
  I->pos = I->buf;
  I->end = I->buf + bytes;
  return bytes;
}

static size_t file_stream_load (iof *I, size_t *plength)
{
  size_t bytes, tail;
  if (I->flags & IOF_STOPPED || *plength == 0)
    return 0;
  tail = iof_tail(I);
  if (I->space - tail < *plength)
    if (resize_buffer_to(I, tail + *plength) == 0)
      return 0;
  bytes = tail + fread(I->buf + tail, sizeof(uint8_t), *plength, I->file);
  I->flags |= IOF_STOPPED;
  *plength = 0;
  I->pos = I->buf;
  I->end = I->buf + bytes;
  return bytes;
}

static size_t iofile_stream_load (iof *I, size_t *plength, size_t *poffset)
{
  size_t bytes, tail;
  if (I->flags & IOF_STOPPED || *plength == 0)
    return 0;
  iof_file_sync(I->iofile, poffset);
  tail = iof_tail(I);
  if (I->space - tail < *plength)
    if (resize_buffer_to(I, tail + *plength) == 0)
      return 0;
  bytes = tail + iof_file_read(I->buf + tail, sizeof(uint8_t), *plength, I->iofile);
  iof_file_unsync(I->iofile, poffset);
  I->flags |= IOF_STOPPED;
  *plength = 0;
  I->pos = I->buf;
  I->end = I->buf + bytes;
  return bytes;
}

static size_t file_stream_reader (iof *I, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(I);
  size_t *plength = &filter->streamstate.length;
  switch(mode)
  {
    case IOFREAD:
      return file_stream_read(I, plength);
    case IOFLOAD:
      return file_stream_load(I, plength);
    case IOFCLOSE:
      iof_close_file(I);
      filter_free_buffer(filter);
      filter_free(filter);
      return 0;
    default:
      return 0;
  }
}

static size_t iofile_stream_reader (iof *I, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(I);
  size_t *plength = &filter->streamstate.length;
  size_t *poffset = &filter->streamstate.offset;
  switch(mode)
  {
    case IOFREAD:
      return iofile_stream_read(I, plength, poffset);
    case IOFLOAD:
      return iofile_stream_load(I, plength, poffset);
    case IOFCLOSE:
      iof_reclose_iofile(I, poffset);
      filter_free_buffer(filter);
      filter_free(filter);
      return 0;
    default:
      return 0;
  }
}

iof * iof_filter_stream_reader (FILE *file, size_t offset, size_t length)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, file_stream_reader);
  iof_setup_file(filter, file);
  stream_state_init(&filter->streamstate, offset, length);
  fseek(file, (long)offset, SEEK_SET); // or perhaps it should be call in file_stream_read(), like iof_file_sync()?
  return filter_to_iof(filter);
}

iof * iof_filter_stream_coreader (iof_file *iofile, size_t offset, size_t length)
{
  iof_filter *filter;
  if (!iof_file_reopen(iofile) || (filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, iofile_stream_reader);
  iof_setup_iofile(filter, iofile);
  stream_state_init(&filter->streamstate, offset, length);
  return filter_to_iof(filter);
}

static size_t file_stream_write (iof *O, size_t *plength, int flush)
{
  size_t bytes;
  if ((bytes = iof_size(O)) > 0)
  {
    if (bytes != fwrite(O->buf, sizeof(uint8_t), bytes, O->file))
    {
      *plength += bytes;
      return 0;
    }
  }
  if (flush)
    fflush(O->file);
  *plength += bytes;
  O->end = O->buf + O->space; // remains intact
  O->pos = O->buf;
  return O->space;
}

static size_t iofile_stream_write (iof *O, size_t *plength, size_t *poffset, int flush)
{
  size_t bytes;
  if ((bytes = iof_size(O)) > 0)
  {
    iof_file_sync(O->iofile, poffset);
    if (bytes != iof_file_write(O->buf, sizeof(uint8_t), bytes, O->iofile))
    {
      *plength += bytes;
      //iof_file_unsync(O->iofile, poffset);
      return 0;
    }
  }
  if (flush)
    iof_file_flush(O->iofile);
  *plength += bytes;
  O->end = O->buf + O->space; // remains intact
  O->pos = O->buf;
  return O->space;
}

static size_t file_stream_writer (iof *O, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(O);
  size_t *plength = &filter->streamstate.length;
  switch (mode)
  {
    case IOFWRITE:
      return file_stream_write(O, plength, 0);
    case IOFFLUSH:
      return file_stream_write(O, plength, 1);
    case IOFCLOSE:
      file_stream_write(O, plength, 1);
      iof_close_file(O);
      filter_free_buffer(filter);
      filter_free(filter);
      return 0;
    default:
      return 0;
  }
}

static size_t iofile_stream_writer (iof *O, iof_mode mode)
{
  iof_filter *filter = iof_to_filter(O);
  size_t *plength = &filter->streamstate.length;
  size_t *poffset = &filter->streamstate.offset;
  switch (mode)
  {
    case IOFWRITE:
      return iofile_stream_write(O, plength, poffset, 0);
    case IOFFLUSH:
      return iofile_stream_write(O, plength, poffset, 1);
    case IOFCLOSE:
      iofile_stream_write(O, plength, poffset, 1);
      iof_close_iofile(O, NULL);
      filter_free_buffer(filter);
      filter_free(filter);
      return 0;
    default:
      return 0;
  }
}

iof * iof_filter_stream_writer (FILE *file)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, file_stream_writer);
  iof_setup_file(filter, file);
  stream_state_init(&filter->streamstate, 0, 0);
  return filter_to_iof(filter);
}

iof * iof_filter_stream_cowriter (iof_file *iofile, size_t offset)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, iofile_stream_writer);
  iof_setup_iofile(filter, iofile);
  stream_state_init(&filter->streamstate, offset, 0);
  return filter_to_iof(filter);
}

/* flate */

static void flate_filter_decoder_close (iof_filter *filter)
{
  flate_decoder_close(&filter->flatestate);
  filter_close(filter);
}

static void flate_filter_encoder_close (iof_filter *filter)
{
  flate_encoder_close(&filter->flatestate);
  filter_close(filter);
}

/* lzw */

static void lzw_filter_decoder_close (iof_filter *filter)
{
  lzw_decoder_close(&filter->lzwstate);
  filter_close(filter);
}

static void lzw_filter_encoder_close (iof_filter *filter)
{
  lzw_encoder_close(&filter->lzwstate);
  filter_close(filter);
}

/* predictor */

static void predictor_filter_decoder_close (iof_filter *filter)
{
  predictor_decoder_close(&filter->predictorstate);
  filter_close(filter);
}

static void predictor_filter_encoder_close (iof_filter *filter)
{
  predictor_encoder_close(&filter->predictorstate);
  filter_close(filter);
}

#ifdef IMAGE_FILTER

/* dct */

static void img_filter_decoder_close (iof_filter *filter)
{ // a specific one; may associate ->file, ->iofile or ->next
  if (filter->flags & IOF_FILE_HANDLE)
    iof_close_file(filter_to_iof(filter));
  else if (filter->flags & IOF_FILE)
    iof_reclose_iofile(filter_to_iof(filter), NULL);
  else
    filter_close_next(filter);
  filter_free_buffer(filter);
  filter_free(filter);
}

#define dct_filter_decoder_close(filter) (dct_decoder_close(&(filter)->imagestate), img_filter_decoder_close(filter))
#define jpx_filter_decoder_close(filter) (jpx_decoder_close(&(filter)->imagestate), img_filter_decoder_close(filter))

static void img_filter_encoder_close (iof_filter *filter)
{
  img_encoder_close(&filter->imagestate);
  filter_close(filter);
}

#define dct_filter_encoder_close(filter) img_filter_encoder_close(filter)
//#define jpx_filter_encoder_close(filter) img_filter_encoder_close(filter)

/* jbig */

static void jbig_filter_decoder_close (iof_filter *filter)
{
  jbig_decoder_close(&filter->jbigstate);
  if (filter->jbigstate.flags & JBIG_DECODER_NOBUFFER)
    filter->buf = filter->pos = filter->end = NULL; // prevent from freeing
  filter_close(filter);
}

#endif

/* crypt */

static void rc4_filter_close (iof_filter *filter)
{
  rc4_state_close(&filter->rc4state);
  filter_close(filter);
}

static void aes_filter_close (iof_filter *filter)
{
  aes_state_close(&filter->aesstate);
  filter_close(filter);
}

/* same scheme for all codecs... */

static size_t decoder_retval (iof *I, const char *type, iof_status status)
{
  switch (status)
  {
    case IOFERR:
    case IOFEMPTY:             // should never happen as we set state.flush = 1 on decoders init
      printf("%s decoder error (%d, %s)\n", type, status, iof_status_kind(status));
      I->flags |= IOF_STOPPED;
      return 0;
    case IOFEOF:               // this is the last chunk,
      I->flags |= IOF_STOPPED; // so stop it and fall
    case IOFFULL:              // prepare pointers to read from I->buf
      I->end = I->pos;
      I->pos = I->buf;
      return I->end - I->buf;
  }
  printf("%s decoder bug, invalid retval %d\n", type, status);
  return 0;
}

static size_t encoder_retval (iof *O, const char *type, iof_status status)
{
  switch (status)
  {
    case IOFERR:
    case IOFFULL:
      printf("%s encoder error (%d, %s)\n", type, status, iof_status_kind(status));
      return 0;
    case IOFEMPTY:
      O->pos = O->buf;
      O->end = O->buf + O->space;
      return O->space;
    case IOFEOF:
      return 0;
  }
  printf("%s encoder bug, invalid retval %d\n", type, status);
  return 0;
}

// base16 decoder function

static size_t base16_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = base16_decode_state(F->next, F, &filter->base16state);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "base16", status);
    case IOFCLOSE:
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// base16 encoder function

static size_t base16_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->base16state.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = base16_encode_state_ln(F, F->next, &filter->base16state);
      return encoder_retval(F, "base16", status);
    case IOFCLOSE:
      if (!filter->base16state.flush)
        base16_encoder(F, IOFFLUSH);
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// base64 decoder function

static size_t base64_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = base64_decode_state(F->next, F, &filter->base64state);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "base64", status);
    case IOFCLOSE:
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// base64 encoder function

static size_t base64_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->base64state.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = base64_encode_state_ln(F, F->next, &filter->base64state);
      return encoder_retval(F, "base64", status);
    case IOFCLOSE:
      if (!filter->base64state.flush)
        base64_encoder(F, IOFFLUSH);
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// base85 decoder function

static size_t base85_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = base85_decode_state(F->next, F, &filter->base85state);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "base85", status);
    case IOFCLOSE:
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// base85 encoder function

static size_t base85_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->base85state.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = base85_encode_state_ln(F, F->next, &filter->base85state);
      return encoder_retval(F, "base85", status);
    case IOFCLOSE:
      if (!filter->base85state.flush)
        base85_encoder(F, IOFFLUSH);
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// runlength decoder function

static size_t runlength_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = runlength_decode_state(F->next, F, &filter->runlengthstate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "runlength", status);
    case IOFCLOSE:
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// runlength encoder function

static size_t runlength_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->runlengthstate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = runlength_encode_state(F, F->next, &filter->runlengthstate);
      return encoder_retval(F, "runlength", status);
    case IOFCLOSE:
      if (!filter->runlengthstate.flush)
        runlength_encoder(F, IOFFLUSH);
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// eexec decoder function

static size_t eexec_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = eexec_decode_state(F->next, F, &filter->eexecstate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "eexec", status);
    case IOFCLOSE:
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// eexec encoder function

static size_t eexec_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->eexecstate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = eexec_encode_state(F, F->next, &filter->eexecstate);
      return encoder_retval(F, "eexec", status);
    case IOFCLOSE:
      if (!filter->eexecstate.flush)
        eexec_encoder(F, IOFFLUSH);
      filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// flate decoder function

static size_t flate_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = flate_decode_state(F->next, F, &filter->flatestate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "flate", status);
    case IOFCLOSE:
      flate_filter_decoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// flate encoder function

static size_t flate_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->flatestate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = flate_encode_state(F, F->next, &filter->flatestate);
      return encoder_retval(F, "flate", status);
    case IOFCLOSE:
      if (!filter->flatestate.flush)
        flate_encoder(F, IOFFLUSH);
      flate_filter_encoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// lzw decoder function

static size_t lzw_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = lzw_decode_state(F->next, F, &filter->lzwstate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "lzw", status);
    case IOFCLOSE:
      lzw_filter_decoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// lzw encoder function

static size_t lzw_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->lzwstate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = lzw_encode_state(F, F->next, &filter->lzwstate);
      return encoder_retval(F, "lzw", status);
    case IOFCLOSE:
      if (!filter->lzwstate.flush)
        lzw_encoder(F, IOFFLUSH);
      lzw_filter_encoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// predictor decoder function

static size_t predictor_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = predictor_decode_state(F->next, F, &filter->predictorstate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "predictor", status);
    case IOFCLOSE:
      predictor_filter_decoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// predictor encoder function

static size_t predictor_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->predictorstate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = predictor_encode_state(F, F->next, &filter->predictorstate);
      return encoder_retval(F, "predictor", status);
    case IOFCLOSE:
      if (!filter->predictorstate.flush)
        predictor_encoder(F, IOFFLUSH);
      predictor_filter_encoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

#ifdef IMAGE_FILTER

// dct decoder function

static size_t dct_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = dct_decode_state(F->next, F, &filter->imagestate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "dct", status);
    case IOFCLOSE:
      dct_filter_decoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// dct encoder function

static size_t dct_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->imagestate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = dct_encode_state(F, F->next, &filter->imagestate);
      return encoder_retval(F, "dct", status);
    case IOFCLOSE:
      if (!filter->imagestate.flush)
        dct_encoder(F, IOFFLUSH);
      dct_filter_encoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// jpx decoder function

static size_t jpx_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = jpx_decode_state(F->next, F, &filter->imagestate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "jpx", status);
    case IOFCLOSE:
      jpx_filter_decoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// jbig decoder function

static size_t jbig_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = jbig_decode_state(F->next, F, &filter->jbigstate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "jbig", status);
    case IOFCLOSE:
      jbig_filter_decoder_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

#endif // IMAGE_FILTER

// rc4 encoder function

static size_t rc4_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->rc4state.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = rc4_encode_state(F, F->next, &filter->rc4state);
      return encoder_retval(F, "rc4", status);
    case IOFCLOSE:
      if (!filter->rc4state.flush)
        rc4_encoder(F, IOFFLUSH);
      rc4_filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// aes encoder function

static size_t aes_encoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  filter = iof_to_filter(F);
  switch (mode)
  {
    case IOFFLUSH:
      filter->aesstate.flush = 1;
    case IOFWRITE:
      F->end = F->pos;
      F->pos = F->buf;
      status = aes_encode_state(F, F->next, &filter->aesstate);
      return encoder_retval(F, "aes", status);
    case IOFCLOSE:
      if (!filter->aesstate.flush)
        aes_encoder(F, IOFFLUSH);
      aes_filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// rc4 decoder function

static size_t rc4_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = rc4_decode_state(F->next, F, &filter->rc4state);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "rc4", status);
    case IOFCLOSE:
      rc4_filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// aes decoder function

static size_t aes_decoder (iof *F, iof_mode mode)
{
  iof_filter *filter;
  iof_status status;
  size_t tail;
  filter = iof_to_filter(F);
  switch(mode)
  {
    case IOFLOAD:
    case IOFREAD:
      if (F->flags & IOF_STOPPED)
        return 0;
      tail = iof_tail(F);
      F->pos = F->buf + tail;
      F->end = F->buf + F->space;
      do {
        status = aes_decode_state(F->next, F, &filter->aesstate);
      } while (mode == IOFLOAD && status == IOFFULL && resize_buffer(F));
      return decoder_retval(F, "aes", status);
    case IOFCLOSE:
      aes_filter_close(filter);
      return 0;
    default:
      break;
  }
  return 0;
}

// end of codecs from template

int iof_filter_basexx_encoder_ln (iof *N, size_t line, size_t maxline)
{ // works for all basexx encoders, as they all refers to the same state struct
  iof_filter *filter;
  if (maxline > 8 && line < maxline)
  {
    filter = iof_to_filter(N);
    filter->base16state.line = line;
    filter->base16state.maxline = maxline;
    return 1;
  }
  return 0;
}

/* base 16 */

iof * iof_filter_base16_decoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, base16_decoder);
  filter->next = iof_ref(N);
  base16_state_init(&filter->base16state);
  filter->base16state.flush = 1; // means N is supposed to be continuous input
  return filter_to_iof(filter);
}

iof * iof_filter_base16_encoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, base16_encoder);
  filter->next = iof_ref(N);
  base16_state_init(&filter->base16state);
  return filter_to_iof(filter);
}

/* base 64 */

iof * iof_filter_base64_decoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, base64_decoder);
  filter->next = iof_ref(N);
  base64_state_init(&filter->base64state);
  filter->base64state.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_base64_encoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, base64_encoder);
  filter->next = iof_ref(N);
  base64_state_init(&filter->base64state);
  return filter_to_iof(filter);
}

/* base 85 */

iof * iof_filter_base85_decoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, base85_decoder);
  filter->next = iof_ref(N);
  base85_state_init(&filter->base85state);
  filter->base85state.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_base85_encoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, base85_encoder);
  filter->next = iof_ref(N);
  base85_state_init(&filter->base85state);
  return filter_to_iof(filter);
}

/* runlength stream filter */

iof * iof_filter_runlength_decoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, runlength_decoder);
  filter->next = iof_ref(N);
  runlength_state_init(&filter->runlengthstate);
  filter->runlengthstate.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_runlength_encoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, runlength_encoder);
  filter->next = iof_ref(N);
  runlength_state_init(&filter->runlengthstate);
  return filter_to_iof(filter);
}

/* eexec stream filter, type1 fonts spec p. 63 */

iof * iof_filter_eexec_decoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, eexec_decoder);
  filter->next = iof_ref(N);
  eexec_state_init(&filter->eexecstate);
  filter->eexecstate.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_eexec_encoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, eexec_encoder);
  filter->next = iof_ref(N);
  eexec_state_init(&filter->eexecstate);
  return filter_to_iof(filter);
}

/* flate stream filter */

iof * iof_filter_flate_decoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, flate_decoder);
  filter->next = iof_ref(N);
  if (flate_decoder_init(&filter->flatestate) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  filter->flatestate.flush = 1;
  return filter_to_iof(filter);;
}

iof * iof_filter_flate_encoder (iof *N)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, flate_encoder);
  filter->next = iof_ref(N);
  if (flate_encoder_init(&filter->flatestate) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  return filter_to_iof(filter);;
}

/* lzw stream filter */

iof * iof_filter_lzw_decoder (iof *N, int flags)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, lzw_decoder);
  filter->next = iof_ref(N);
  if (lzw_decoder_init(&filter->lzwstate, flags) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  filter->lzwstate.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_lzw_encoder (iof *N, int flags)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, lzw_encoder);
  filter->next = iof_ref(N);
  if (lzw_encoder_init(&filter->lzwstate, flags) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  return filter_to_iof(filter);
}

/* predictor stream filter */

iof * iof_filter_predictor_decoder (iof *N, int predictor, int rowsamples, int components, int compbits)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, predictor_decoder);
  filter->next = iof_ref(N);
  if (predictor_decoder_init(&filter->predictorstate, predictor, rowsamples, components, compbits) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  filter->predictorstate.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_predictor_encoder (iof *N, int predictor, int rowsamples, int components, int compbits)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, predictor_encoder);
  filter->next = iof_ref(N);
  if (predictor_encoder_init(&filter->predictorstate, predictor, rowsamples, components, compbits) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  // filter->predictorstate.flush = 1;
  return filter_to_iof(filter);
}

#ifdef IMAGE_FILTER

/*
In our PDF uses, DCTDecode/JPXDecode filters typically drain the source FILE/iofile directly. If so, there is no need
to create a separate filter (with buffer) for source and separate for DCT, as the source won't be used at all
(dct internals operate on FILE * anyway). If there is some intermediate filter between the source FILE/iofile
and DCT, we have to load the input iof in runtime to ensure all JPEG data in one piece, see dct_decode_state().
An initializer tries to catch the case when next iof is a stream reader and modify it accordingly instead of
creating a separate filter chain item.
*/

iof * iof_filter_img_decoder (iof *N, int hooksource, int nobuffering, int format)
{
  iof_filter *filter;
  size_t offset, length;

  if (hooksource)
  { /* try to hook result of iof_filter_stream_(co)reader */
    if (N->more == file_stream_reader)
    { /* result of iof_filter_stream_reader() */
      filter = iof_to_filter(N);
      offset = filter->streamstate.offset;
      length = filter->streamstate.length;
      img_decoder_init_file(&filter->imagestate, filter->file, offset, length, format);
      filter->more = format == IMG_JPG ? dct_decoder : jpx_decoder;
      return N;
    }
    if (N->more == iofile_stream_reader)
    { /* iof_filter_stream_coreader() */
      filter = iof_to_filter(N);
      offset = filter->streamstate.offset;
      length = filter->streamstate.length;
      if (iof_file_reopen(filter->iofile))
      {
        img_decoder_init_iofile(&filter->imagestate, filter->iofile, offset, length, format);
        filter->more = format == IMG_JPG ? dct_decoder : jpx_decoder;
        return N;
      }
    }
  }
  /* if cannot hook, go normal way; a source will be loaded at first call to decoder */
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, format == IMG_JPG ? dct_decoder : jpx_decoder);
  filter->next = iof_ref(N);
  img_decoder_init(&filter->imagestate, nobuffering, format);
  filter->imagestate.flush = 1; // not used, but as all decoders have...
  return filter_to_iof(filter);
}

iof * iof_filter_img_encoder (iof *N, uint32_t width, uint32_t height, uint8_t channels, int format)
{
  iof_filter *filter;
  img_state *state;
  if (width == 0 || height == 0 || (channels != 1 && channels != 3 && channels != 4))
    return NULL;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, dct_encoder);
  filter->next = iof_ref(N);
  if ((state = img_encoder_init(&filter->imagestate, format)) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  img_encoder_params(state, width, height, 8, IMG_NOCOLOR, channels);
  //state->flush = 1;
  return filter_to_iof(filter);
}

/* jbig filters (decoder only) */

/*
jbig2dec library provides decoded data in a single piece. Instead of creating an unecessary buffer,
we let the encoder function jbig_decode_state() to pin the data obtainedm with no redundant copying
from internal buffer to a private space. Since we don't allocate a space for returned iof buffer,
a returned iof->buf/pos/end members are NULL. On attempt to read decoded data, the encoder reads
the entire input, generates the output and sets iof pointers accordingly:
- jbig_decode_state() acts as if the filter iof was an output, so ->buf is set to data, ->pos set to its end
- decoder_retval() sets ->pos to ->buf to let the user read from the beginning
*/

iof * iof_filter_jbig_decoder (iof *N, iof *G, int jbigflags)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  if (jbigflags & JBIG_DECODER_NOBUFFER) // optimized
    filter_reader_no_buffer(filter, jbig_decoder);
  else // go normal way, with a private buffer space
    filter_reader(filter, jbig_decoder);
  filter->next = iof_ref(N);
  if (jbig_decoder_init(&filter->jbigstate, G, jbigflags) == NULL)
  {
    filter_discard(filter); // if filter buffer is NULL, won't be freed anyway
    return NULL;
  }
  filter->jbigstate.flush = 1;
  return filter_to_iof(filter);;
}

#endif // IMAGE_FILTER

/* rc4 crypt filters */

iof * iof_filter_rc4_decoder (iof *N, const void *key, size_t keylength)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, rc4_decoder);
  filter->next = iof_ref(N);
  if (rc4_state_init(&filter->rc4state, key, keylength) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  filter->rc4state.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_rc4_encoder (iof *N, const void *key, size_t keylength)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, rc4_encoder);
  filter->next = iof_ref(N);
  if (rc4_state_init(&filter->rc4state, key, keylength) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  // filter->rc4state.flush = 1;
  return filter_to_iof(filter);
}

/* aes crypt filters */

iof * iof_filter_aes_decoder (iof *N, const void *key, size_t keylength)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_reader(filter, aes_decoder);
  filter->next = iof_ref(N);
  if (aes_decode_init(&filter->aesstate, key, keylength) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  aes_pdf_mode(&filter->aesstate);
  filter->aesstate.flush = 1;
  return filter_to_iof(filter);
}

iof * iof_filter_aes_encoder (iof *N, const void *key, size_t keylength)
{
  iof_filter *filter;
  if ((filter = filter_new()) == NULL)
    return NULL;
  filter_writer(filter, aes_encoder);
  filter->next = iof_ref(N);
  if (aes_encode_init(&filter->aesstate, key, keylength) == NULL)
  {
    filter_discard(filter);
    return NULL;
  }
  aes_pdf_mode(&filter->aesstate);
  // filter->aesstate.flush = 1;
  return filter_to_iof(filter);
}
