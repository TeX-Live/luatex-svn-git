
#include <stdio.h>
//#include "ppapi.h"
#include "pplib.h"
#include "assert.h"

static int usage (const char *argv0)
{
  printf("pplib " pplib_version ", " pplib_author "\n");
  printf("usage: %s file1.pdf file2.pdf ...\n", argv0);
  return 0;
}

static void print_result_filter (ppstream *stream, int decode)
{
  ppstream_filter info;
  size_t i;

  ppstream_filter_info(stream, &info, decode);
  printf("  when %s: /Filter [", decode ? "uncompressed" : "compressed");
  for (i = 0; i < info.count; ++i)
    printf(" /%s", ppstream_filter_name[info.filters[i]]);
  printf(" ]");
  if (info.params != NULL)
  {
    printf(" /DecodeParms [");
    for (i = 0; i < info.count; ++i)
      printf(" %s", info.params[i] != NULL ? "<<...>>" : "null");
    printf(" ]");
  }
  printf("\n");
}

static void print_stream_info (ppref *ref, ppstream *stream)
{
  size_t length;
  printf("object %lu %lu R\n", (unsigned long)ref->number, (unsigned long)ref->version);
  if (stream->flags & PPSTREAM_FILTER)
    printf("  filtered ");
  else
    printf("  plain ");
  if (stream->flags & PPSTREAM_IMAGE)
    printf("image ");
  else
    printf("stream ");
  if (stream->flags & PPSTREAM_ENCRYPTED)
    printf("encrypted ");
  if (stream->flags & PPSTREAM_NOT_SUPPORTED)
    printf("invalid ");
  if (!ppdict_rget_uint(stream->dict, "Length", &length))
    length = 0;
  assert(stream->length == length);
  printf("length %lu (/Length %lu)\n", (unsigned long)stream->length, (unsigned long)length);
  print_result_filter(stream, 0);
  print_result_filter(stream, 1);
}

static void check_stream_chunks (ppstream *stream)
{
  size_t sum, size;
  uint8_t *data;
  const int decode[2] = {0, 1};
  int d;

  for (d = 0; d < 2; ++d)
  {
    for (sum = 0, data = ppstream_first(stream, &size, decode[d]); data != NULL; data = ppstream_next(stream, &size))
      sum += size;
    ppstream_done(stream);
    ppstream_all(stream, &size, decode[d]);
    ppstream_done(stream);
    assert(sum == size);
    printf("  %s chunks size [%lu]\n", (decode[d] ? "decoded" : "raw"), (unsigned long)size);
  }
}

#define USE_BUFFERS_POOL 1

int main (int argc, const char **argv)
{
  const char *filepath;
  int a;
  ppdoc *pdf;
  ppxref *xref;
  ppxsec *xsec;
  size_t xi;
  ppuint refnum;
  ppref *ref;

  if (argc < 2)
    return usage(argv[0]);
  if (USE_BUFFERS_POOL)
    ppstream_init_buffers();
  for (a = 1; a < argc; ++a)
  {
    filepath = argv[a];
    printf("loading %s... ", filepath);
    pdf = ppdoc_load(filepath);
    if (pdf == NULL)
    {
      printf("failed\n");
      continue;
    }
    printf("done.\n");
    for (xref = ppdoc_xref(pdf); xref != NULL; xref = ppxref_prev(xref))
    {
      for (xi = 0, xsec = xref->sects; xi < xref->size; ++xi, ++xsec)
      {
        for (refnum = xsec->first, ref = xsec->refs; refnum <= xsec->last; ++refnum, ++ref)
        {
          if (ref->object.type != PPSTREAM)
            continue;
          print_stream_info(ref, ref->object.stream);
          check_stream_chunks(ref->object.stream);
        }
      }
    }
    ppdoc_free(pdf);
  }
  if (USE_BUFFERS_POOL)
    ppstream_free_buffers();
  return 0;
}
