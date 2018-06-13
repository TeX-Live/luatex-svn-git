/* input/iutput stream */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "utilmem.h"
#include "utiliof.h"

/* commons */

void * iof_copy_data (const void *data, size_t size)
{
  return memcpy(util_malloc(size), data, size);
}


uint8_t * iof_copy_file_data (const char *filename, size_t *psize)
{
  FILE *file;
  size_t size;
  uint8_t *data;
  if ((file = fopen(filename, "rb")) == NULL)
    return NULL;
  fseek(file, 0, SEEK_END);
  size = (size_t)ftell(file);
  data = (uint8_t *)util_malloc(size);
  fseek(file, 0, SEEK_SET);
  if ((*psize = fread(data, 1, size, file)) != size)
  {
    util_free(data);
    data = NULL;
  }
  fclose(file);
  return data;
}

uint8_t * iof_copy_file_handle_data (FILE *file, size_t *psize)
{
  size_t size;
  uint8_t *data;
  //long offset = ftell(file); // keep offset intact?
  fseek(file, 0, SEEK_END);
  size = (size_t)ftell(file);
  data = (uint8_t *)util_malloc(size);
  fseek(file, 0, SEEK_SET);
  if ((*psize = fread(data, 1, size, file)) != size)
  {
    util_free(data);
    data = NULL;
  }
  //fseek(file, offset, SEEK_SET)
  return data;
}

FILE * iof_get_file (iof *F)
{
  if (F->flags & IOF_FILE)
    return iof_file_get_file(F->iofile);
  if (F->flags & IOF_FILE_HANDLE)
    return F->file;
  return NULL;
}

const char * iof_status_kind (iof_status status)
{
  switch (status)
  {
    case IOFEOF:
      return "IOFEOF";
    case IOFERR:
      return "IOFERR";
    case IOFEMPTY:
      return "IOFEMPTY";
    case IOFFULL:
      return "IOFFULL";
    default:
      break;
  }
  return "(unknown)";
}

/* shared pseudofile */

#define IOF_FILE_DEFAULTS 0

iof_file * iof_file_new (FILE *file)
{
  iof_file *iofile = (iof_file *)util_malloc(sizeof(iof_file));
  iof_file_set_fh(iofile, file);
  iofile->offset = NULL;
  iofile->size = 0;
  iofile->name = NULL;
  iofile->refcount = 0;
  iofile->flags = IOF_FILE_DEFAULTS|IOF_ISALLOC;
  return iofile;
}

iof_file * iof_file_init (iof_file *iofile, FILE *file)
{
  iof_file_set_fh(iofile, file);
  iofile->offset = NULL;
  iofile->size = 0;
  iofile->name = NULL;
  iofile->refcount = 0;
  iofile->flags = IOF_FILE_DEFAULTS;
  return iofile;
}

iof_file * iof_file_rdata (const void *data, size_t size)
{
  iof_file *iofile = (iof_file *)util_malloc(sizeof(iof_file));
  iofile->buf = iofile->pos = (uint8_t *)data;
  iofile->end = iofile->buf + size;
  iofile->offset = NULL;
  iofile->size = 0;
  iofile->name = NULL;
  iofile->refcount = 0;
  iofile->flags = IOF_FILE_DEFAULTS|IOF_ISALLOC|IOF_DATA;
  return iofile;
}

iof_file * iof_file_rdata_init (iof_file *iofile, const void *data, size_t size)
{
  iofile->buf = iofile->pos = (uint8_t *)data;
  iofile->end = iofile->buf + size;
  iofile->offset = NULL;
  iofile->size = 0; // letse keep it consequently set to zero (only for user disposal)
  iofile->name = NULL;
  iofile->refcount = 0;
  iofile->flags = IOF_FILE_DEFAULTS|IOF_DATA;
  return iofile;
}

iof_file * iof_file_wdata (void *data, size_t size)
{
  return iof_file_rdata((const void *)data, size);
}

iof_file * iof_file_wdata_init (iof_file *iofile, void *data, size_t size)
{
  return iof_file_rdata_init(iofile, (const void *)data, size);
}

/* typical uses so far */

iof_file * iof_file_reader_from_file_handle (iof_file *iofile, const char *filename, FILE *file, int preload, int closefile)
{
  uint8_t *data;
  size_t size;

  if (preload)
  {
    if ((data = iof_copy_file_handle_data(file, &size)) == NULL)
    {
      if (closefile)
        fclose(file);
      return NULL;
    }
    if (iofile == NULL)
      iofile = iof_file_rdata(data, size);
    else
      iof_file_rdata_init(iofile, data, size);
    iofile->flags |= IOF_BUFFER_ISALLOC;
    if (closefile)
      fclose(file);
  }
  else
  {
    if (iofile == NULL)
      iofile = iof_file_new(file);
    else
      iof_file_init(iofile, file);
    if (closefile)
      iofile->flags |= IOF_CLOSE_FILE;
  }
  if (filename != NULL)
    iof_file_set_name(iofile, filename);
  return iofile;
}

iof_file * iof_file_reader_from_file (iof_file *iofile, const char *filename, int preload)
{
  FILE *file;
  if ((file = fopen(filename, "rb")) == NULL)
    return NULL;
  return iof_file_reader_from_file_handle(iofile, filename, file, preload, 1);
}

iof_file * iof_file_reader_from_data (iof_file *iofile, const void *data, size_t size, int preload, int freedata)
{
  void *newdata;
  if (data == NULL)
    return NULL;
  if (preload)
  {
    newdata = iof_copy_data(data, size);
    if (iofile == NULL)
      iofile = iof_file_rdata(newdata, size);
    else
      iof_file_rdata_init(iofile, newdata, size);
    iofile->flags |= IOF_BUFFER_ISALLOC;
    if (freedata) // hardly makes sense...
      util_free((void *)data);
  }
  else
  {
    if (iofile == NULL)
      iofile = iof_file_rdata(data, size);
    else
      iof_file_rdata_init(iofile, data, size);
    if (freedata)
      iofile->flags |= IOF_BUFFER_ISALLOC;
  }
  return iofile;
}

/*
iof_file * iof_file_writer_from_file (iof_file *iofile, const char *filename)
{
  FILE *file;
  if ((file = fopen(filename, "wb")) == NULL)
    return NULL;
  if (iofile == NULL)
    iofile = iof_file_new(file);
  else
    iof_file_init(iofile, file);
  iofile->flags |= IOF_CLOSE_FILE;
  iof_file_set_name(iofile, filename);
  return iofile;
}
*/

/*
Because of limited number of FILE* handles available, we may need to close contained handle
between accessing it. In applications so far (fonts, images) we typically need the source
to parse the file on creation and to rewrite or reload the data on dump. All iof_file api
functions assume that iofile has FILE* opened. Reopening it on every access (ftell, fseek,
read/write) makes no sense, as we would effectively loose control. If the caller invalidates
iofile by closing and nulling its file handle, it is also responsible to reopen when necessary.
*/

int iof_file_close_input (iof_file *iofile)
{
  FILE *file;
  if (iofile->flags & IOF_DATA)
    return 0;
  if ((file = iof_file_get_fh(iofile)) == NULL)
    return 0;
  fclose(file);
  iof_file_set_fh(iofile, NULL);
  iofile->flags &= ~IOF_RECLOSE_FILE;
  iofile->flags |= IOF_REOPEN_FILE;
  return 1;
}

int iof_file_reopen_input (iof_file *iofile)
{ // returns true if iofile readable
  FILE *file;
  const char *filename;
  if (iofile->flags & IOF_DATA)
    return 1;
  if ((file = iof_file_get_fh(iofile)) != NULL)
    return 1; // if present, assumed readable
  if ((filename = iofile->name) == NULL || (file = fopen(filename, "rb")) == NULL)
    return 0;
  iof_file_set_fh(iofile, file);
  iofile->flags &= ~IOF_REOPEN_FILE;
  iofile->flags |= IOF_RECLOSE_FILE;
  return 1;
}

/**/

void iof_file_free (iof_file *iofile)
{
  FILE *file;
  if (iofile->flags & IOF_DATA)
  {
    if (iofile->flags & IOF_BUFFER_ISALLOC)
    {
      iofile->flags &= ~IOF_BUFFER_ISALLOC;
      if (iofile->buf != NULL)
      {
        util_free(iofile->buf);
        iofile->buf = iofile->pos = iofile->end = NULL;
      }
    }
  }
  else if ((file = iof_file_get_fh(iofile)) != NULL)
  {
    if (iofile->flags & IOF_CLOSE_FILE)
     	fclose(file);
    iof_file_set_fh(iofile, NULL);
  }
  iof_file_set_name(iofile, NULL);
  if (iofile->flags & IOF_ISALLOC)
    util_free(iofile);
}

void iof_file_set_name (iof_file *iofile, const char *name)
{
  if (iofile->name != NULL)
    util_free((void *)iofile->name);
  if (name != NULL)
    iofile->name = iof_copy_data(name, strlen(name) + 1);
  else
    iofile->name = NULL;
}

int iof_file_seek (iof_file *iofile, long offset, int whence)
{
  if (iofile->flags & IOF_DATA)
  {
    switch (whence)
    {
      case SEEK_SET:
        if (offset >= 0 && iofile->buf + offset <= iofile->end)
        {
          iofile->pos = iofile->buf + offset;
          return 0;
        }
        return -1;
      case SEEK_CUR:
        if ((offset >= 0 && iofile->pos + offset <= iofile->end) || (offset < 0 && iofile->pos + offset >= iofile->buf))
        {
          iofile->pos += offset;
          return 0;
        }
        return -1;
      case SEEK_END:
        if (offset <= 0 && iofile->end + offset >= iofile->buf)
        {
          iofile->pos = iofile->end + offset;
          return 0;
        }
        return -1;
    }
    return -1;
  }
  return fseek(iof_file_get_fh(iofile), offset, whence);
}

long iof_file_tell (iof_file *iofile)
{
  return (iofile->flags & IOF_DATA) ? (long)(iofile->pos - iofile->buf) : ftell(iof_file_get_fh(iofile));
}

size_t iof_file_size (iof_file *iofile)
{ 
  long pos, size;
  FILE *file;
  if (iofile->flags & IOF_DATA)
    return (size_t)iof_space(iofile);
  file = iof_file_get_fh(iofile);
  pos = ftell(file);
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, pos, SEEK_SET);
  return size;
}

int iof_file_eof (iof_file *iofile)
{
  if (iofile->flags & IOF_DATA)
    return iofile->pos == iofile->end ? -1 : 0;
  return feof(iof_file_get_fh(iofile));
}

int iof_file_flush (iof_file *iofile)
{
  if (iofile->flags & IOF_DATA)
    return 0;
  return fflush(iof_file_get_fh(iofile));
}

size_t iof_file_read (void *ptr, size_t size, size_t items, iof_file *iofile)
{
  if (iofile->flags & IOF_DATA)
  {
    size_t bytes = size * items;
    if (bytes > (size_t)iof_left(iofile))
      bytes = (size_t)iof_left(iofile);
    memcpy(ptr, iofile->pos, bytes);
    iofile->pos += bytes;
    return bytes / size; // number of elements read
  }
  return fread(ptr, size, items, iof_file_get_fh(iofile));
}

static size_t iof_file_data_resizeto (iof_file *iofile, size_t space)
{
  uint8_t *newbuf;
  size_t size;
  size = iof_size(iofile);
  if (iofile->flags & IOF_BUFFER_ISALLOC)
  {
    newbuf = (uint8_t *)util_realloc(iofile->buf, space);
  }
  else
  {
    newbuf = (uint8_t *)util_malloc(space);
    if (size > 0)
      memcpy(newbuf, iofile->buf, size);
    iofile->flags |= IOF_BUFFER_ISALLOC;
  }
  iofile->buf = newbuf;
  iofile->pos = newbuf + size;
  iofile->end = newbuf + space;
  return space - size;
}

#define iof_file_data_resize(iofile) iof_file_data_resizeto(iofile, iof_space(iofile) << 1)

size_t iof_file_write (const void *ptr, size_t size, size_t items, iof_file *iofile)
{
  if (iofile->flags & IOF_DATA)
  {
    size_t space, sizesofar, bytes;
    bytes = size * items;
    if (bytes > (size_t)iof_left(iofile))
    {      
      if ((space = iof_space(iofile)) == 0) // allow iofile->buf/end initially NULL
        space = BUFSIZ;
      for (sizesofar = iof_size(iofile), space <<= 1; sizesofar + bytes > space; space <<= 1)
        ;
      if (iof_file_data_resizeto(iofile, space) == 0)
        return 0;
    }
    memcpy(iofile->pos, ptr, bytes);
    iofile->pos += bytes;
    return bytes / size;
  }
  return fwrite(ptr, size, items, iof_file_get_fh(iofile));
}

size_t iof_file_ensure (iof_file *iofile, size_t bytes)
{
  if (iofile->flags & IOF_DATA)
  {
    size_t space, sizesofar, left;
    left = (size_t)iof_left(iofile);
    if (bytes > left)
    {      
      if ((space = iof_space(iofile)) == 0) // allow iofile->buf/end initially NULL
        space = BUFSIZ;
      for (sizesofar = iof_size(iofile), space <<= 1; sizesofar + bytes > space; space <<= 1);
      return iof_file_data_resizeto(iofile, space);
    }
    return left;  
  }
  return 0;
}

int iof_file_getc (iof_file *iofile)
{
  if (iofile->flags & IOF_DATA)
    return iofile->pos < iofile->end ? *iofile->pos++ : IOFEOF;
  return fgetc(iof_file_get_fh(iofile));
}

int iof_file_putc (iof_file *iofile, int c)
{
  if (iofile->flags & IOF_DATA)
  {
    if (iofile->pos >= iofile->end)
      if (iof_file_data_resize(iofile) == 0)
        return IOFEOF;
    *iofile->pos++ = (uint8_t)c;
    return c;
  }
  return fputc(c, iof_file_get_fh(iofile));
}

int iof_file_sync (iof_file *iofile, size_t *offset)
{
  if (iofile->offset != offset)
  {
    if (iofile->offset != NULL)
      *iofile->offset = iof_file_tell(iofile);
    iofile->offset = offset;
    if (offset) // let offset be NULL
      return iof_file_seek(iofile, (long)*offset, SEEK_SET);
  }
  return 0;
}

void iof_file_unsync (iof_file *iofile, size_t *offset)
{
  if (iofile->offset && iofile->offset == offset)
    iofile->offset = NULL;
}



/* iof free*/

/*
#define iof_buffer_free_(O, free) \
  ((void)(((O)->flags & IOF_BUFFER_ISALLOC) && (free(O->buf), 0), (O)->flags &= ~IOF_BUFFER_ISALLOC))
#define iof_buffer_free(O) iof_buffer_free_(O, util_free)
#define iof_free_(O, free) ((void)(iof_buffer_free_(O, free), ((O->flags & IOF_ISALLOC) && (free(O), 0))))
#define iof_free(O) iof_free_(O, util_free)
*/

static void iof_free (iof *O)
{
  if (O->flags & IOF_BUFFER_ISALLOC)
  {
    O->flags &= ~IOF_BUFFER_ISALLOC;
    util_free(O->buf);
    O->buf = NULL;
  }
  if (O->flags & IOF_ISALLOC)
    util_free(O);
}

/* iof seek */

#define iof_reader_reset(I) ((I)->pos = (I)->end = (I)->buf)
#define iof_reader_reseek_file(I, offset, whence) (fseek((I)->file, offset, whence) == 0 ? (iof_reader_reset(I), 0) : -1)
#define iof_reader_reseek_iofile(I, offset, whence) (iof_file_seek((I)->iofile, offset, whence) == 0 ? (iof_reader_reset(I), 0) : -1)

#define iof_writer_reset(O) ((O)->pos = (O)->buf)
#define iof_writer_reseek_file(O, offset, whence) (iof_flush(O), (fseek((O)->file, offset, whence) == 0 ? (iof_writer_reset(O), 0) : -1))
#define iof_writer_reseek_iofile(O, offset, whence) (iof_flush(O), (iof_file_seek((O)->iofile, offset, whence) == 0 ? (iof_writer_reset(O), 0) : -1))

static int iof_reader_seek_data (iof *I, long offset, int whence)
{
  switch (whence)
  {
    case SEEK_SET:
      if (offset >= 0 && I->buf + offset <= I->end)
      {
        I->pos = I->buf + offset;
        return 0;
      }
      return -1;
    case SEEK_CUR:
      if ((offset >= 0 && I->pos + offset <= I->end) || (offset < 0 && I->pos + offset >= I->buf))
      {
        I->pos += offset;
        return 0;
      }
      return -1;
    case SEEK_END:
      if (offset <= 0 && I->end + offset >= I->buf)
      {
        I->pos = I->end + offset;
        return 0;
      }
      return -1;
  }
  return -1;
}

static int iof_reader_seek_iofile (iof *I, long offset, int whence)
{
  long fileoffset;
  switch (whence)
  {
    case SEEK_SET:
      fileoffset = iof_file_tell(I->iofile);
      if (offset <= fileoffset && offset >= fileoffset - iof_space(I))
      {
        I->pos = I->end - (fileoffset - offset);
        return 0;
      }
      return iof_reader_reseek_iofile(I, offset, SEEK_SET);
    case SEEK_CUR:
      if ((offset >= 0 && I->pos + offset <= I->end) || (offset < 0 && I->pos + offset >= I->buf))
      {
        I->pos += offset;
        return 0;
      }
      return iof_reader_reseek_iofile(I, offset, SEEK_CUR);
    case SEEK_END:
      return iof_reader_reseek_iofile(I, offset, SEEK_END); // can we do better?
  }
  return -1;
}

static int iof_reader_seek_file (iof *I, long offset, int whence)
{
  long fileoffset;
  switch (whence)
  {
    case SEEK_SET:
      fileoffset = ftell(I->file);
      if (offset <= fileoffset && offset >= fileoffset - iof_space(I))
      {
        I->pos = I->end - (fileoffset - offset);
        return 0;
      }
      return iof_reader_reseek_file(I, offset, SEEK_SET);
    case SEEK_CUR:
      if ((offset >= 0 && I->pos + offset <= I->end) || (offset < 0 && I->pos + offset >= I->buf))
      {
        I->pos += offset;
        return 0;
      }
      return iof_reader_reseek_file(I, offset, SEEK_CUR);
    case SEEK_END:
      return iof_reader_reseek_file(I, offset, SEEK_END); // can we do better?
  }
  return -1;
}

int iof_reader_seek (iof *I, long offset, int whence)
{
  if (I->flags & IOF_FILE)
    return iof_reader_seek_iofile(I, offset, whence);
  if (I->flags & IOF_FILE_HANDLE)
    return iof_reader_seek_file(I, offset, whence);
  if (I->flags & IOF_DATA)
    return iof_reader_seek_data(I, offset, whence);
  return -1;
}

int iof_reader_reseek (iof *I, long offset, int whence)
{
  if (I->flags & IOF_FILE)
    return iof_reader_reseek_iofile(I, offset, whence);
  if (I->flags & IOF_FILE_HANDLE)
    return iof_reader_reseek_file(I, offset, whence);
  if (I->flags & IOF_DATA)
    return iof_reader_seek_data(I, offset, whence);
  return -1;
}

static int iof_writer_seek_data (iof *O, long offset, int whence)
{
  /*
  fseek() allows to seek after the end of file. Seeking does not increase the output file.
  No byte is written before fwirte(). It seems to fill the gap with zeros. Until we really need that,
  no seeking out of bounds for writers.
  */
  return iof_reader_seek_data(O, offset, whence);
}

static int iof_writer_seek_iofile (iof *O, long offset, int whence)
{
  long fileoffset;
  switch (whence)
  {
    case SEEK_SET:
      fileoffset = iof_file_tell(O->iofile);
      if (offset >= fileoffset && offset <= fileoffset + iof_space(O))
      {
        O->pos = O->buf + (offset - fileoffset);
        return 0;
      }
      return iof_writer_reseek_iofile(O, offset, SEEK_SET);
    case SEEK_CUR:
      if ((offset >=0 && O->pos + offset <= O->end) || (offset < 0 && O->pos + offset >= O->buf))
      {
        O->pos += offset;
        return 0;
      }
      return iof_writer_reseek_iofile(O, offset, SEEK_CUR);
    case SEEK_END:
      return iof_writer_reseek_iofile(O, offset, SEEK_END);
  }
  return -1;
}

static int iof_writer_seek_file (iof *O, long offset, int whence)
{
  long fileoffset;
  switch (whence)
  {
    case SEEK_SET:
      fileoffset = ftell(O->file);
      if (offset >= fileoffset && offset <= fileoffset + iof_space(O))
      {
        O->pos = O->buf + (offset - fileoffset);
        return 0;
      }
      return iof_writer_reseek_file(O, offset, SEEK_SET);
    case SEEK_CUR:
      if ((offset >=0 && O->pos + offset <= O->end) || (offset < 0 && O->pos + offset >= O->buf))
      {
        O->pos += offset;
        return 0;
      }
      return iof_writer_reseek_file(O, offset, SEEK_CUR);
    case SEEK_END:
      return iof_writer_reseek_file(O, offset, SEEK_END);
  }
  return -1;
}

int iof_writer_seek (iof *I, long offset, int whence)
{
  if (I->flags & IOF_FILE)
    return iof_writer_seek_iofile(I, offset, whence);
  if (I->flags & IOF_FILE_HANDLE)
    return iof_writer_seek_file(I, offset, whence);
  if (I->flags & IOF_DATA)
    return iof_writer_seek_data(I, offset, whence);
  return -1;
}

int iof_writer_reseek (iof *I, long offset, int whence)
{
  if (I->flags & IOF_FILE)
    return iof_writer_reseek_iofile(I, offset, whence);
  if (I->flags & IOF_FILE_HANDLE)
    return iof_writer_reseek_file(I, offset, whence);
  if (I->flags & IOF_DATA)
    return iof_writer_seek_data(I, offset, whence);
  return -1;
}

int iof_seek (iof *F, long offset, int whence)
{
  return (F->flags & IOF_WRITER) ? iof_writer_seek(F, offset, whence) : iof_reader_seek(F, offset, whence);
}

int iof_reseek (iof *F, long offset, int whence)
{
  return (F->flags & IOF_WRITER) ? iof_writer_reseek(F, offset, whence) : iof_reader_reseek(F, offset, whence);
}

/* tell */

long iof_reader_tell (iof *I)
{
  if (I->flags & IOF_FILE)
    return iof_file_tell(I->iofile) - (long)iof_left(I);
  if (I->flags & IOF_FILE_HANDLE)
    return ftell(I->file) - (long)iof_left(I);
  //if (I->flags & IOF_DATA)
  return (long)iof_size(I);
}

long iof_writer_tell (iof *O)
{
  if (O->flags & IOF_FILE)
    return iof_file_tell(O->iofile) + (long)iof_size(O);
  if (O->flags & IOF_FILE_HANDLE)
    return ftell(O->file) + (long)iof_size(O);
  //if (I->flags & IOF_DATA)
  return (long)iof_size(O);
}

long iof_tell (iof *I)
{
  return (I->flags & IOF_WRITER) ? iof_writer_tell(I) : iof_reader_tell(I);
}

size_t iof_fsize (iof *I)
{
  size_t pos, size;
  if (I->flags & IOF_FILE)
    return iof_file_size(I->iofile);
  if (I->flags & IOF_FILE_HANDLE)
  {
    pos = (size_t)ftell(I->file);
    fseek(I->file, 0, SEEK_END);
    size = (size_t)ftell(I->file);
    fseek(I->file, (long)pos, SEEK_SET);
    return size;
  }
  //if (I->flags & IOF_DATA)
  return (size_t)iof_space(I);
}

/* closing underlying file handle */

void iof_close_file (iof *F)
{
  FILE *file;
  //if (F->flags & IOF_FILE_HANDLE)
  //{
    if ((file = F->file) != NULL)
    {
      if (F->flags & IOF_CLOSE_FILE)
        fclose(F->file);
      F->file = NULL;
    }
  //}
}

void iof_close_iofile (iof *F, size_t *offset)
{
  iof_file *iofile;
  //if (F->flags & IOF_FILE)
  //{
    if ((iofile = F->iofile) != NULL)
    {
      iof_file_unsync(iofile, offset);
      iof_file_decref(iofile);
      F->iofile = NULL;
    }
  //}
}

/* a very special variant for reader filters initiated with iof_file_reopen(). It also calls
   iof_file_reclose(), which takes an effect only if previously reopened, but better to keep
   all this thin ice separated. Used in filters: iofile_reader, iofile_stream_reader, image
   decoders. */

void iof_reclose_iofile (iof *F, size_t *offset)
{
  iof_file *iofile;
  //if (F->flags & IOF_FILE)
  //{
    if ((iofile = F->iofile) != NULL)
    {
      iof_file_unsync(iofile, offset);
      iof_file_reclose(iofile);
      iof_file_decref(iofile);
      F->iofile = NULL;
    }
  //}
}

/* save reader tail */

size_t iof_save_tail (iof *I)
{
  size_t size, left;
  size = iof_size(I);
  left = iof_left(I);
  if (size >= left)
    memcpy(I->buf, I->pos, left);
  else
    memmove(I->buf, I->pos, left);
  return left;
}

/*
uint8_t * iof_tail_data (iof *I, size_t *ptail)
{
  size_t size, left;
  uint8_t *data;
  if ((I->flags & IOF_TAIL) && (*ptail = iof_left(I)) > 0)
  {
    data = (uint8_t *)util_malloc(*ptail);
    memcpy(data, I->pos, *ptail);
    return data;
  }
  *ptail = 0;
  return NULL;
}
*/

size_t iof_input_save_tail (iof *I, size_t back)
{
  size_t size;
  I->flags |= IOF_TAIL;
  I->pos -= back;
  size = iof_input(I);
  I->pos += back;
  I->flags &= ~IOF_TAIL;
  return size; // + back - back
}

/* read from file */

static size_t iof_read_from_file_handle (iof *I, FILE *file)
{
  size_t bytes, tail;
  tail = iof_tail(I);
  bytes = tail + fread(I->buf + tail, sizeof(uint8_t), I->space - tail, file);
  if (bytes > 0)
  {
    I->pos = I->buf;
    I->end = I->pos + bytes;
    return bytes;
  }
  return 0;
}

static size_t file_reader (iof *I, iof_mode mode)
{
  switch(mode)
  {
    case IOFREAD:
      return iof_read_from_file_handle(I, I->file);
    case IOFLOAD: // todo
      return 0;
    case IOFCLOSE:
      if (I->flags & IOF_CLOSE_FILE)
        fclose(I->file);
      iof_free(I);
      return 0;
    default:
      return 0;
  }
}

iof * iof_setup_file_handle_reader (iof *I, void *buffer, size_t space, FILE *f)
{
  if (I == NULL)
    iof_setup_reader(I, buffer, space);
  else
    iof_reader_buffer(I, buffer, space);
  iof_setup_file(I, f);
  I->more = file_reader;
  return I;
}

iof * iof_setup_file_reader (iof *I, void *buffer, size_t space, const char *filename)
{
  FILE *f;
  if ((f = fopen(filename, "rb")) == NULL)
    return NULL;
  if (I == NULL)
    iof_setup_reader(I, buffer, space);
  else
    iof_reader_buffer(I, buffer, space);
  iof_setup_file(I, f);
  I->flags |= IOF_CLOSE_FILE;
  I->more = file_reader;
  return I;
}

/* write to file */

static size_t iof_write_to_file_handle (iof *O, FILE *file)
{
  size_t bytes = O->pos - O->buf;
  if (bytes == fwrite(O->buf, sizeof(uint8_t), bytes, file)) {
    O->pos = O->buf;
    return O->space;
  }
  iof_fwrite_error();
  return 0;
}

static size_t file_writer (iof *O, iof_mode mode)
{
  switch (mode)
  {
    case IOFWRITE:
      return iof_write_to_file_handle(O, O->file);
    case IOFFLUSH:
      iof_write_to_file_handle(O, O->file);
      fflush(O->file);
      return 0;
    case IOFCLOSE:
      iof_write_to_file_handle(O, O->file);
      if (O->flags & IOF_CLOSE_FILE)
        fclose(O->file);
      else
        fflush(O->file); // ?
      iof_free(O);
      return 0;
    default:
      return 0;
  }
}

iof * iof_setup_file_handle_writer (iof *O, void *buffer, size_t space, FILE *f)
{
  if (O == NULL)
    iof_setup_writer(O, buffer, space);
  else
    iof_writer_buffer(O, buffer, space);
  iof_setup_file(O, f);
  O->more = file_writer;
  return O;
}

iof * iof_setup_file_writer (iof *O, void *buffer, size_t space, const char *filename)
{
  FILE *f;
  if ((f = fopen(filename, "wb")) == NULL)
    return NULL;
  if (O == NULL)
    iof_setup_writer(O, buffer, space);
  else
    iof_writer_buffer(O, buffer, space);
  iof_setup_file(O, f);
  O->flags |= IOF_CLOSE_FILE;
  O->more = file_writer;
  return O;
}

/* a dedicated handler for stdout; allows to avoid iof.file := stdout each time we need to use it
  (stdout cannot be used when initializing static structure i guess) */

static size_t stdout_writer (iof *O, iof_mode mode)
{
  switch(mode)
  {
    case IOFWRITE:
    {
      fwrite(O->buf, sizeof(uint8_t), iof_size(O), stdout);
      O->pos = O->buf;
      return O->space;
    }
    case IOFCLOSE:
    case IOFFLUSH:
    {
      fwrite(O->buf, sizeof(uint8_t), iof_size(O), stdout);
      fflush(stdout);
      O->pos = O->buf;
      return 0;
    }
    default:
      break;
  }
  return 0;
}

static size_t stderr_writer (iof *O, iof_mode mode)
{
  switch(mode)
  {
    case IOFWRITE:
    {
      fwrite(O->buf, sizeof(uint8_t), iof_size(O), stderr);
      O->pos = O->buf;
      return O->space;
    }
    case IOFCLOSE:
    case IOFFLUSH:
    {
      fwrite(O->buf, sizeof(uint8_t), iof_size(O), stderr);
      fflush(stderr);
      O->pos = O->buf;
      return 0;
    }
    default:
      break;
  }
  return 0;
}

static uint8_t iof_stdout_buffer[BUFSIZ];
iof iof_stdout = IOF_WRITER_STRUCT(stdout_writer, NULL, iof_stdout_buffer, BUFSIZ, 0);

static uint8_t iof_stderr_buffer[BUFSIZ];
iof iof_stderr = IOF_WRITER_STRUCT(stderr_writer, NULL, iof_stderr_buffer, BUFSIZ, 0);

/* dummy reader, always returns o -> IOFEOF */

static size_t iof_dummy_reader (iof *I, iof_mode mode)
{
  (void)I;
  (void)mode;
  return 0;
}

iof * iof_setup_dummy (iof *I, void *buffer, size_t space)
{
  if (I == NULL)
    iof_setup_reader(I, buffer, space);
  else
    iof_reader_buffer(I, buffer, space);
  I->link = NULL;
  I->more = iof_dummy_reader;
  return I;
}

/* dummy writer, always succeeds */

static size_t iof_dummy_writer (iof *O, iof_mode mode)
{
  switch(mode)
  {
    case IOFWRITE: {
      O->pos = O->buf;
      return O->space;
    }
    case IOFCLOSE:
      iof_free(O);
      return 0;
    default:
      return 0;
  }
}

iof * iof_setup_null (iof *O, void *buffer, size_t space)
{
  if (O == NULL)
    iof_setup_writer(O, buffer, space);
  else
    iof_writer_buffer(O, buffer, space);
  O->link = NULL;
  O->more = iof_dummy_writer;
  return O;
}

/* read from somewhere */

iof * iof_reader (iof *I, void *link, iof_handler reader, const void *m, size_t bytes)
{
  I->space = 0;
  I->link = link;
  I->more = reader;
  I->flags = 0;
  I->refcount = 0;
  //if (m != NULL && bytes > 0)
  if (m != NULL)
  {
    I->buf = I->pos = (uint8_t *)m;
    I->end = (uint8_t *)m + bytes;
    return I;
  }
  // return iof_dummy(I);
  return NULL;
}

iof * iof_string_reader (iof *I, const void *s, size_t bytes)
{
  I->space = 0;
  I->link = NULL;
  I->more = NULL;
  I->flags = 0; // iof_string() sets IOF_DATA
  I->refcount = 0;
  if (s != NULL)
    return iof_string(I, s, bytes);
  return NULL;
}

/* write somewhere */

iof * iof_writer (iof *O, void *link, iof_handler writer, void *m, size_t bytes)
{
  O->space = 0;
  O->link = link;
  O->more = (writer != NULL ? writer : iof_dummy_writer);
  O->flags = 0;
  O->refcount = 0;
  if (m != NULL && bytes > 0)
  {
    O->buf = O->pos = (uint8_t *)m;
    O->end = (uint8_t *)m + bytes;
    return O;
  }
  // return iof_null(O);
  return NULL;
}

/* write to growing bytes buffer */

static size_t iof_mem_handler (iof *O, iof_mode mode)
{
  switch(mode)
  {
    case IOFWRITE:
    {
      size_t bytes = O->pos - O->buf;
      size_t size = 2*bytes;
      uint8_t *b;
      if (O->flags & IOF_BUFFER_ISALLOC)
        b = (uint8_t *)util_realloc(O->buf, size);
      else
      {
        b = (uint8_t *)util_malloc(size);
        memcpy(b, O->buf, bytes);
        O->flags |= IOF_BUFFER_ISALLOC;
      }
      O->buf = b;
      O->pos = b + bytes;
      O->end = b + size;
      return bytes; // == size - bytes;
    }
    case IOFCLOSE:
    {
      iof_free(O);
      return 0;
    }
    default:
      return 0;
  }
}

iof * iof_setup_buffer (iof *O, void *buffer, size_t space)
{
  if (O == NULL)
    iof_setup_writer(O, buffer, space);
  else
    iof_writer_buffer(O, buffer, space);
  O->link = NULL;
  O->flags |= IOF_DATA;
  O->more = iof_mem_handler;
  return O;
}

iof * iof_setup_buffermin (iof *O, void *buffer, size_t space, size_t min)
{
  if ((O = iof_setup_buffer(O, buffer, space)) != NULL && space < min) // just allocate min now to avoid further rewriting
  {
    O->buf = O->pos = (uint8_t *)util_malloc(min);
    O->flags |= IOF_BUFFER_ISALLOC;
    O->end = O->buf + min;
  }
  return O;
}

iof * iof_buffer_create (size_t space)
{
  uint8_t *buffer;
  iof *O;
  space += sizeof(iof);
  buffer = util_malloc(space);
  if ((O = iof_setup_buffer(NULL, buffer, space)) != NULL)
    O->flags |= IOF_ISALLOC;
  return O;
}

/* set/get */

int iof_getc (iof *I)
{
  if (iof_readable(I))
    return *I->pos++;
  return IOFEOF;
}

int iof_hgetc (iof *I)
{
  if (iof_hreadable(I))
    return *I->hpos++;
  return IOFEOF;
}

int iof_igetc (iof *I)
{
  if (iof_ireadable(I))
    return *I->ipos++;
  return IOFEOF;
}

int iof_putc (iof *O, int u)
{
  if (iof_writable(O))
  {
    iof_set(O, u);
    return (uint8_t)u;
  }
  return IOFFULL;
}

int iof_hputc (iof *O, int u)
{
  if (iof_hwritable(O))
  {
    iof_hset(O, u);
    return (uint16_t)u;
  }
  return IOFFULL;
}

int iof_iputc (iof *O, int u)
{
  if (iof_iwritable(O))
  {
    iof_iset(O, u);
    return (uint32_t)u;
  }
  return IOFFULL;
}

size_t iof_skip (iof *I, size_t bytes)
{
  while (bytes)
  {
    if (iof_readable(I))
      ++I->pos;
    else
      break;
    --bytes;
  }
  return bytes;
}

size_t iof_hskip (iof *I, size_t bytes)
{
  while (bytes)
  {
    if (iof_hreadable(I))
      ++I->hpos;
    else
      break;
    --bytes;
  }
  return bytes;
}

size_t iof_iskip (iof *I, size_t bytes)
{
  while (bytes)
  {
    if (iof_ireadable(I))
      ++I->ipos;
    else
      break;
    --bytes;
  }
  return bytes;
}

/* from iof to iof */

iof_status iof_pass (iof *I, iof *O)
{
  size_t leftin, leftout;
  if ((leftin = iof_left(I)) == 0)
    leftin = iof_input(I);
  while (leftin)
  {
    if ((leftout = iof_left(O)) == 0)
      if ((leftout = iof_output(O)) == 0)
        return IOFFULL;
    while (leftin > leftout)
    {
      memcpy(O->pos, I->pos, leftout);
      I->pos += leftout;
      O->pos = O->end; /* eq. += leftout */
      leftin -= leftout;
      if ((leftout = iof_output(O)) == 0)
        return IOFFULL;
    }
    if (leftin)
    {
      memcpy(O->pos, I->pos, leftin);
      I->pos = I->end; /* eq. += leftin */
      O->pos += leftin;
    }
    leftin = iof_input(I);
  }
  return IOFEOF;
}

/* read n-bytes */

size_t iof_read (iof *I, void *to, size_t size)
{
  size_t leftin, done = 0;
  char *s = (char *)to;
  
  if ((leftin = iof_left(I)) == 0)
    if ((leftin = iof_input(I)) == 0)
      return done;
  while (size > leftin)
  {
    memcpy(s, I->pos, leftin * sizeof(uint8_t));
    size -= leftin;
    done += leftin;
    s += leftin;
    I->pos = I->end;
    if ((leftin = iof_input(I)) == 0)
      return done;
  }
  if (size)
  {
    memcpy(s, I->pos, size * sizeof(uint8_t));
    I->pos += size;
    done += size;
  }
  return done;
}

size_t iof_hread (iof *I, void *to, size_t size)
{
  size_t leftin, done = 0;
  short *s = (short *)to;
  
  if ((leftin = iof_hleft(I)) == 0)
    if ((leftin = iof_input(I)) == 0)
      return done;
  while (size > leftin)
  {
    memcpy(s, I->hpos, leftin * sizeof(uint16_t));
    size -= leftin;
    done += leftin;
    s += leftin;
    I->hpos = I->hend;
    if ((leftin = iof_input(I)) == 0)
      return done;
  }
  if (size)
  {
    memcpy(s, I->hpos, size * sizeof(uint16_t));
    I->hpos += size;
    done += size;
  }
  return done;
}

size_t iof_iread (iof *I, void *to, size_t size)
{
  size_t leftin, done = 0;
  int *s = (int *)to;
  
  if ((leftin = iof_ileft(I)) == 0)
    if ((leftin = iof_input(I)) == 0)
      return done;
  while (size > leftin)
  {
    memcpy(s, I->hpos, leftin * sizeof(uint32_t));
    size -= leftin;
    done += leftin;
    s += leftin;
    I->ipos = I->iend;
    if ((leftin = iof_input(I)) == 0)
      return done;
  }
  if (size)
  {
    memcpy(s, I->ipos, size * sizeof(uint32_t));
    I->ipos += size;
    done += size;
  }
  return done;
}

/* rewrite FILE content (use fseek if needed) */

size_t iof_write_file_handle (iof *O, FILE *file)
{
  size_t leftout, size, readout;
  if ((leftout = iof_left(O)) == 0)
    if ((leftout = iof_output(O)) == 0)
      return 0;
  size = 0;
  do {
    readout = fread(O->pos, 1, leftout, file);    
    O->pos += readout;
    size += readout;
  } while(readout == leftout && (leftout = iof_output(O)) > 0);
  return size;
}

size_t iof_write_file (iof *O, const char *filename)
{
  FILE *file;
  size_t size;
  if ((file = fopen(filename, "rb")) == NULL)
    return 0;
  size = iof_write_file_handle(O, file);
  fclose(file);
  return size;
}

size_t iof_write_iofile (iof *O, iof_file *iofile, int savepos)
{
  long offset;
  size_t size;
  FILE *file;
  if (iofile->flags & IOF_DATA)
    return iof_write(O, iofile->pos, (size_t)(iofile->end - iofile->pos));
  file = iof_file_get_fh(iofile);
  if (savepos)
  {
    offset = ftell(file);  
    size = iof_write_file_handle(O, file);
    fseek(file, offset, SEEK_SET);
    return size;
  }
  return iof_write_file_handle(O, file);
}

/* write n-bytes */

size_t iof_write (iof *O, const void *data, size_t size)
{
  size_t leftout, done = 0;
  const char *s = (const char *)data;
  if ((leftout = iof_left(O)) == 0)
    if ((leftout = iof_output(O)) == 0)
      return done;
  while (size > leftout)
  {
    memcpy(O->pos, s, leftout * sizeof(uint8_t));
    size -= leftout;
    done += leftout;
    s += leftout;
    O->pos = O->end;
    if ((leftout = iof_output(O)) == 0)
      return done;
  }
  if (size)
  {
    memcpy(O->pos, s, size * sizeof(uint8_t));
    O->pos += size;
    done += size;
  }
  return done;
}

size_t iof_hwrite (iof *O, const void *data, size_t size)
{
  size_t leftout, done = 0;
  const short *s = (const short *)data;
  if ((leftout = iof_hleft(O)) == 0)
    if ((leftout = iof_output(O)) == 0)
      return done;
  while (size > leftout)
  {
    memcpy(O->hpos, s, leftout * sizeof(uint16_t));
    size -= leftout;
    done += leftout;
    s += leftout;
    O->hpos = O->hend;
    if ((leftout = iof_output(O)) == 0)
      return done;
  }
  if (size)
  {
    memcpy(O->hpos, s, size * sizeof(uint16_t));
    O->hpos += size;
    done += size;
  }
  return done;
}

size_t iof_iwrite (iof *O, const void *data, size_t size)
{
  size_t leftout, done = 0;
  const int *s = (const int *)data;
  if ((leftout = iof_ileft(O)) == 0)
    if ((leftout = iof_output(O)) == 0)
      return done;
  while (size > leftout)
  {
    memcpy(O->ipos, s, leftout * sizeof(uint32_t));
    size -= leftout;
    done += leftout;
    s += leftout;
    O->ipos = O->iend;
    if ((leftout = iof_output(O)) == 0)
      return done;
  }
  if (size)
  {
    memcpy(O->ipos, s, size * sizeof(uint32_t));
    O->ipos += size;
    done += size;
  }
  return done;
}

/* write '\0'-terminated string */

iof_status iof_puts (iof *O, const void *data)
{
  const char *s = (const char *)data;
  while (*s)
  {
    if (iof_writable(O))
      iof_set(O, *s++);
    else
      return IOFFULL;
  }
  return IOFEOF; // ?
}

size_t iof_put_string (iof *O, const void *data)
{
  const char *p, *s = (const char *)data;
  for (p = s; *p != '\0' && iof_writable(O); iof_set(O, *p++));
  return p - s;
}

/* write byte n-times */

/*
iof_status iof_repc (iof *O, char c, size_t bytes)
{
  while (bytes)
  {
    if (iof_writable(O))
      iof_set(O, c);
    else
      return IOFFULL;
    --bytes;
  }
  return IOFEOF; // ?
}
*/

size_t iof_repc (iof *O, char c, size_t bytes)
{
  size_t leftout, todo = bytes;
  if ((leftout = iof_left(O)) == 0)
    if ((leftout = iof_output(O)) == 0)
      return 0;
  while (bytes > leftout)
  {
    memset(O->pos, c, leftout);
    bytes -= leftout;
    O->pos = O->end;
    if ((leftout = iof_output(O)) == 0)
      return todo - bytes;
  }
  if (bytes)
  {
    memset(O->pos, c, bytes);
    O->pos += bytes;
  }
  return todo;
}

/* putfs */

#define IOF_FMT_SIZE 1024

size_t iof_putfs (iof *O, const char *format, ...)
{
  static char buffer[IOF_FMT_SIZE];
  va_list args;
  va_start(args, format);
  if (vsnprintf(buffer, IOF_FMT_SIZE, format, args) > 0)
  {
    va_end(args);
    return iof_put_string(O, buffer);
  }
  else
  {
    va_end(args);
    return iof_write(O, buffer, IOF_FMT_SIZE);
  }
}

/* integer from iof; return 1 on success, 0 otherwise */

int iof_get_int32 (iof *I, int32_t *number)
{
  int sign, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  if (!base10_digit(c)) return 0;
  iof_read_integer(I, c, *number);
  if (sign) *number = -*number;
  return 1;
}

int iof_get_intlw (iof *I, intlw_t *number)
{
  int sign, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  if (!base10_digit(c)) return 0;
  iof_read_integer(I, c, *number);
  if (sign) *number = -*number;
  return 1;
}

int iof_get_int64 (iof *I, int64_t *number)
{
  int sign, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  if (!base10_digit(c)) return 0;
  iof_read_integer(I, c, *number);
  if (sign) *number = -*number;
  return 1;
}

int iof_get_uint32 (iof *I, uint32_t *number)
{
  int c = iof_char(I);
  if (!base10_digit(c)) return 0;
  iof_read_integer(I, c, *number);
  return 1;
}

int iof_get_uintlw (iof *I, uintlw_t *number)
{
  int c = iof_char(I);
  if (!base10_digit(c)) return 0;
  iof_read_integer(I, c, *number);
  return 1;
}

int iof_get_uint64 (iof *I, uint64_t *number)
{
  int c = iof_char(I);
  if (!base10_digit(c)) return 0;
  iof_read_integer(I, c, *number);
  return 1;
}

int iof_get_int32_radix (iof *I, int32_t *number, int radix)
{
  int sign, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  if (!base10_digit(c)) return 0;
  iof_read_radix(I, c, *number, radix);
  if (sign) *number = -*number;
  return 1;

}

int iof_get_intlw_radix (iof *I, intlw_t *number, int radix)
{
  int sign, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  if (!base10_digit(c)) return 0;
  iof_read_radix(I, c, *number, radix);
  if (sign) *number = -*number;
  return 1;
}

int iof_get_int64_radix (iof *I, int64_t *number, int radix)
{
  int sign, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  if (!base10_digit(c)) return 0;
  iof_read_radix(I, c, *number, radix);
  if (sign) *number = -*number;
  return 1;
}

int iof_get_uint32_radix (iof *I, uint32_t *number, int radix)
{
  int c = iof_char(I);
  if (!base10_digit(c)) return 0;
  iof_read_radix(I, c, *number, radix);
  return 1;
}

int iof_get_uintlw_radix (iof *I, uintlw_t *number, int radix)
{
  int c = iof_char(I);
  if (!base10_digit(c)) return 0;
  iof_read_radix(I, c, *number, radix);
  return 1;
}

int iof_get_uint64_radix (iof *I, uint64_t *number, int radix)
{
  int c = iof_char(I);
  if (!base10_digit(c)) return 0;
  iof_read_radix(I, c, *number, radix);
  return 1;
}

/* get roman to uint16_t, cf. roman_to_uint16() from utilnumber.c*/

/* todo: some trick in place of this macro horror? */

#define roman1000(c) (c == 'M' || c == 'm')
#define roman500(c)  (c == 'D' || c == 'd')
#define roman100(c)  (c == 'C' || c == 'c')
#define roman50(c)   (c == 'L' || c == 'l')
#define roman10(c)   (c == 'X' || c == 'x')
#define roman5(c)    (c == 'V' || c == 'v')
#define roman1(c)    (c == 'I' || c == 'i')

#define roman100s(I, c) \
  (roman100(c) ? (100 + ((c = iof_next(I), roman100(c)) ? (100 + ((c = iof_next(I), roman100(c)) ? (c = iof_next(I), 100) : 0)) : 0)) : 0)
#define roman10s(I, c) \
  (roman10(c) ? (10 + ((c = iof_next(I), roman10(c)) ? (10 + ((c = iof_next(I), roman10(c)) ? (c = iof_next(I), 10) : 0)) : 0)) : 0)
#define roman1s(I, c) \
  (roman1(c) ? (1 + ((c = iof_next(I), roman1(c)) ? (1 + ((c = iof_next(I), roman1(c)) ? (c = iof_next(I), 1) : 0)) : 0)) : 0)

int iof_get_roman (iof *I, uint16_t *number)
{
  int c;
  /* M */
  for (*number = 0, c = iof_char(I); roman1000(c); *number += 1000, c = iof_next(I));
  /* D C */
  if (roman500(c))
  {
    c = iof_next(I);
    *number += 500 + roman100s(I, c);
  }
  else if (roman100(c))
  {
    c = iof_next(I);
    if (roman1000(c))
    {
      c = iof_next(I);
      *number += 900;
    }
    else if (roman500(c))
    {
      c = iof_next(I);
      *number += 400;
    }
    else
      *number += 100 + roman100s(I, c);
  }
  /* L X */
  if (roman50(c))
  {
    c = iof_next(I);
    *number += 50 + roman10s(I, c);
  }
  else if (roman10(c))
  {
    c = iof_next(I);
    if (roman100(c))
    {
      c = iof_next(I);
      *number += 90;
    }
    else if (roman50(c))
    {
      c = iof_next(I);
      *number += 40;
    }
    else
      *number += 10 + roman10s(I, c);
  }
  /* V I */
  if (roman5(c))
  {
    c = iof_next(I);
    *number += 5 + roman1s(I, c);
  }
  else if (roman1(c))
  {
    c = iof_next(I);
    if (roman10(c))
    {
      c = iof_next(I);
      *number += 9;
    }
    else if (roman5(c))
    {
      c = iof_next(I);
      *number += 4;
    }
    else
      *number += 1 + roman1s(I, c);
  }
  return 1;
}

/* double from iof; return 1 on success */

int iof_get_double (iof *I, double *number) // cf. string_to_double()
{
  int sign, exponent10, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  iof_scan_decimal(I, c, *number);
  if (c == '.')
  {
    c = iof_next(I);
    iof_scan_fraction(I, c, *number, exponent10);
  }
  else
    exponent10 = 0;
  if (c == 'e' || c == 'E')
  {
    c = iof_next(I);
    iof_scan_exponent10(I, c, exponent10);
  }
  double_exp10(*number, exponent10);
  if (sign) *number = -*number;
  return 1;
}

int iof_get_float (iof *I, float *number) // cf. string_to_float() in utilnumber.c
{
  int sign, exponent10, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  iof_scan_decimal(I, c, *number);
  if (c == '.')
  {
    c = iof_next(I);
    iof_scan_fraction(I, c, *number, exponent10);
  }
  else
    exponent10 = 0;
  if (c == 'e' || c == 'E')
  {
    c = iof_next(I);
    iof_scan_exponent10(I, c, exponent10);
  }
  float_exp10(*number, exponent10);
  if (sign) *number = -*number;
  return 1;
}

int iof_conv_double (iof *I, double *number) // cf. convert_to_double() in utilnumber.c
{
  int sign, exponent10, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  iof_scan_decimal(I, c, *number);
  if (c == '.' || c == ',')
  {
    c = iof_next(I);
    iof_scan_fraction(I, c, *number, exponent10);
    if (exponent10 < 0)
      double_negative_exp10(*number, exponent10);
  }
  if (sign) *number = -*number;
  return 1;
}

int iof_conv_float (iof *I, float *number) // cf. convert_to_float()
{
  int sign, exponent10, c = iof_char(I);
  iof_scan_sign(I, c, sign);
  iof_scan_decimal(I, c, *number);
  if (c == '.' || c == ',')
  {
    c = iof_next(I);
    iof_scan_fraction(I, c, *number, exponent10);
    if (exponent10 < 0)
      float_negative_exp10(*number, exponent10);
  }
  if (sign) *number = -*number;
  return 1;
}

/* integer to iof; return a number of written bytes */

#define iof_copy_number_buffer(O, s, p) for (p = s; *p && iof_writable(O); iof_set(O, *p), ++p)

size_t iof_put_int32 (iof *O, int32_t number)
{
  const char *s, *p;
  s = int32_to_string(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_intlw (iof *O, intlw_t number)
{
  const char *s, *p;
  s = intlw_to_string(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_int64 (iof *O, int64_t number)
{
  const char *s, *p;
  s = int64_to_string(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_uint32 (iof *O, uint32_t number)
{
  const char *s, *p;
  s = uint32_to_string(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_uintlw (iof *O, uintlw_t number)
{
  const char *s, *p;
  s = uintlw_to_string(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_uint64 (iof *O, uint64_t number)
{
  const char *s, *p;
  s = uint64_to_string(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_int32_radix (iof *O, int32_t number, int radix)
{
  const char *s, *p;
  s = int32_to_radix(number, radix);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_intlw_radix (iof *O, intlw_t number, int radix)
{
  const char *s, *p;
  s = intlw_to_radix(number, radix);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_int64_radix (iof *O, int64_t number, int radix)
{
  const char *s, *p;
  s = int64_to_radix(number, radix);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_uint32_radix (iof *O, uint32_t number, int radix)
{
  const char *s, *p;
  s = uint32_to_radix(number, radix);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_uintlw_radix (iof *O, uintlw_t number, int radix)
{
  const char *s, *p;
  s = uintlw_to_radix(number, radix);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_uint64_radix (iof *O, uint64_t number, int radix)
{
  const char *s, *p;
  s = uint64_to_radix(number, radix);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

/* roman numerals */

size_t iof_put_roman_uc (iof *O, uint16_t number)
{
  const char *s, *p;
  s = uint16_to_roman_uc(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_roman_lc (iof *O, uint16_t number)
{
  const char *s, *p;
  s = uint16_to_roman_lc(number);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

/* double/float to iof; return the number of written bytes */

size_t iof_put_double (iof *O, double number, int digits)
{
  const char *s, *p;
  s = double_to_string(number, digits);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

size_t iof_put_float (iof *O, float number, int digits)
{
  const char *s, *p;
  s = float_to_string(number, digits);
  iof_copy_number_buffer(O, s, p);
  return p - s;
}

/* iof to binary integer; pretty common */

int iof_get_be_uint2 (iof *I, uint32_t *pnumber)
{
  int c1, c2;
  if ((c1 = iof_get(I)) < 0 || (c2 = iof_get(I)) < 0)
    return 0;
  *pnumber = (c1<<8)|c2;
  return 1;
}

int iof_get_be_uint3 (iof *I, uint32_t *pnumber)
{
  int c1, c2, c3;
  if ((c1 = iof_get(I)) < 0 || (c2 = iof_get(I)) < 0 || (c3 = iof_get(I)) < 0)
    return 0;
  *pnumber = (c1<<16)|(c2<<8)|c3;
  return 1;
}

int iof_get_be_uint4 (iof *I, uint32_t *pnumber)
{
  int c1, c2, c3, c4;
  if ((c1 = iof_get(I)) < 0 || (c2 = iof_get(I)) < 0 || (c3 = iof_get(I)) < 0 || (c4 = iof_get(I)) < 0)
    return 0;
  *pnumber = (c1<<24)|(c2<<16)|(c3<<8)|c4;
  return 1;
}

int iof_get_le_uint2 (iof *I, uint32_t *pnumber)
{
  int c1, c2;
  if ((c1 = iof_get(I)) < 0 || (c2 = iof_get(I)) < 0)
    return 0;
  *pnumber = (c2<<8)|c1;
  return 1;
}

int iof_get_le_uint3 (iof *I, uint32_t *pnumber)
{
  int c1, c2, c3;
  if ((c1 = iof_get(I)) < 0 || (c2 = iof_get(I)) < 0 || (c3 = iof_get(I)) < 0)
    return 0;
  *pnumber = (c3<<16)|(c2<<8)|c1;
  return 1;
}

int iof_get_le_uint4 (iof *I, uint32_t *pnumber)
{
  int c1, c2, c3, c4;
  if ((c1 = iof_get(I)) < 0 || (c2 = iof_get(I)) < 0 || (c3 = iof_get(I)) < 0 || (c4 = iof_get(I)) < 0)
    return 0;
  *pnumber = (c4<<24)|(c3<<16)|(c2<<8)|c1;
  return 1;
}

/* iof input data */

uint8_t * iof_file_input_data (iof_file *iofile, size_t *psize, int *isnew)
{
  uint8_t *data;
  if (iofile->flags & IOF_DATA)
  {
    data = iofile->buf;
    *psize = iofile->end - iofile->buf;
    *isnew = 0;
    return data;
  }
  if (iof_file_reopen(iofile))
  {
    data = iof_copy_file_handle_data(iof_file_get_fh(iofile), psize);
    *isnew = 1;
    iof_file_reclose(iofile);
    return data;
  }
  return NULL;
}

/*
uint8_t * iof_file_reader_data (iof_file *iofile, size_t *size)
{
  uint8_t *data;
  if (!(iofile->flags & IOF_DATA) || iofile->pos == NULL || (*size = (size_t)iof_left(iofile)) == 0)
    return NULL;  
  if (iofile->flags & IOF_BUFFER_ISALLOC)
  {
    data = iofile->buf; // iofile->pos; // returned must be freeable, makes sense when ->buf == ->pos
    iofile->flags &= ~IOF_BUFFER_ISALLOC;
    iofile->buf = iofile->pos = iofile->end = NULL;
    return data;
  }
  data = (uint8_t *)util_malloc(*size);
  memcpy(data, iofile->buf, *size);
  return data;
}

uint8_t * iof_file_writer_data (iof_file *iofile, size_t *size)
{
  uint8_t *data;
  if (!(iofile->flags & IOF_DATA) || iofile->buf == NULL || (*size = (size_t)iof_size(iofile)) == 0)
    return NULL;  
  if (iofile->flags & IOF_BUFFER_ISALLOC)
  {
    iofile->flags &= ~IOF_BUFFER_ISALLOC;
    data = iofile->buf;
    iofile->buf = iofile->pos = iofile->end = NULL;
    return data;
  }
  data = (uint8_t *)util_malloc(*size);
  memcpy(data, iofile->buf, *size);
  return data;
}
*/

uint8_t * iof_reader_data (iof *I, size_t *psize)
{
  uint8_t *data;
  *psize = (size_t)iof_left(I);
  if (I->flags & IOF_BUFFER_ISALLOC)
  {
    data = I->buf; // actually I->pos, but we have to return something freeable
    I->flags &= ~IOF_BUFFER_ISALLOC;
    I->buf = NULL;
  }
  else
  {
    data = util_malloc(*psize);
    memcpy(data, I->pos, *psize);
  }
  iof_close(I);
  return data;
}


uint8_t * iof_writer_data (iof *O, size_t *psize)
{
  uint8_t *data;
  *psize = (size_t)iof_size(O);
  if (O->flags & IOF_BUFFER_ISALLOC)
  {
    data = O->buf;
    O->flags &= ~IOF_BUFFER_ISALLOC;
    O->buf = NULL;
  }
  else
  {
    data = util_malloc(*psize);
    memcpy(data, O->buf, *psize);
  }
  iof_close(O);
  return data;
}

size_t iof_reader_to_file_handle (iof *I, FILE *file)
{
  size_t size;
  for (size = 0; iof_readable(I); I->pos = I->end)
    size += fwrite(I->buf, sizeof(uint8_t), iof_left(I), file);
  return size;
}

size_t iof_reader_to_file (iof *I, const char *filename)
{
  FILE *file;
  size_t size;
  if ((file = fopen(filename, "wb")) == NULL)
    return 0;
  for (size = 0; iof_readable(I); I->pos = I->end)
    size += fwrite(I->buf, sizeof(uint8_t), iof_left(I), file);
  fclose(file);
  return size;
}

/* debug */

size_t iof_data_to_file (const void *data, size_t size, const char *filename)
{
  FILE *fh;
  if ((fh = fopen(filename, "wb")) == NULL)
    return 0;
  // size = fwrite(data, size, sizeof(uint8_t), fh); // WRONG, this always returns 1, as fwrite returns the number of elements successfully written out
  size = fwrite(data, sizeof(uint8_t), size, fh);
  fclose(fh);
  return size;
}

size_t iof_result_to_file_handle (iof *F, FILE *file)
{
  const void *data;
  size_t size;
  data = iof_result(F, size);
	return iof_data_to_file_handle(data, size, file);
}

size_t iof_result_to_file (iof *F, const char *filename)
{
  const void *data;
  size_t size;
  data = iof_result(F, size);
  return iof_data_to_file(data, size, filename);
}

void iof_debug (iof *I, const char *filename)
{
  FILE *file = fopen(filename, "wb");
  if (file != NULL)
  {
    fprintf(file, ">>> buf %p <<<\n", I->buf);
    fwrite(I->buf, sizeof(uint8_t), iof_size(I), file);
    fprintf(file, "\n>>> pos %p (%ld) <<<\n", I->pos, (long)iof_size(I));
    fwrite(I->pos, sizeof(uint8_t), iof_left(I), file);
    fprintf(file, "\n>>> end %p (%ld) <<<\n", I->end, (long)iof_left(I));
    fwrite(I->end, sizeof(uint8_t), I->space - iof_space(I), file);
    fprintf(file, "\n>>> end of buffer %p (%ld) <<<\n", I->buf + I->space, (long)(I->buf + I->space - I->end));
    fclose(file);
  }
}
