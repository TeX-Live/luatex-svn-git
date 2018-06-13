#ifndef UTIL_FILTER_H
#define UTIL_FILTER_H

//#define DEBUG_FILTERS

#ifndef NO_IMAGE_FILTER
#  define IMAGE_FILTER
#endif

#include "utiliof.h"
// todo: move those to .c
#include "utilbasexx.h"
#include "utilflate.h"
#include "utillzw.h"
#include "utilfpred.h"
#include "utilcrypt.h"
#ifdef IMAGE_FILTER
#  include "utiljbig.h"
//#  include "utilimage.h"
#endif

/* iof filters */

#define iof_filter_stop(F) ((void)(F->pos = F->end = F->buf, F->flags |= IOF_STOPPED))

iof * iof_filter_file_handle_reader (FILE *file);
iof * iof_filter_file_handle_writer (FILE *file);

iof * iof_filter_iofile_reader (iof_file *iofile, size_t offset);
iof * iof_filter_iofile_writer (iof_file *iofile, size_t offset);

iof * iof_filter_file_reader (const char *filename);
iof * iof_filter_file_writer (const char *filename);

iof * iof_filter_string_reader (const void *s, size_t length);
iof * iof_filter_string_writer (const void *s, size_t length);

iof * iof_filter_stream_reader (FILE *file, size_t offset, size_t length);
iof * iof_filter_stream_coreader (iof_file *iofile, size_t offset, size_t length);
iof * iof_filter_stream_writer (FILE *file);
iof * iof_filter_stream_cowriter (iof_file *iofile, size_t offset);

int iof_filter_basexx_encoder_ln (iof *N, size_t line, size_t maxline);

iof * iof_filter_base16_decoder (iof *N);
iof * iof_filter_base16_encoder (iof *N);

iof * iof_filter_base64_decoder (iof *N);
iof * iof_filter_base64_encoder (iof *N);

iof * iof_filter_base85_decoder (iof *N);
iof * iof_filter_base85_encoder (iof *N);

iof * iof_filter_runlength_decoder (iof *N);
iof * iof_filter_runlength_encoder (iof *N);

iof * iof_filter_eexec_decoder (iof *N);
iof * iof_filter_eexec_encoder (iof *N);

iof * iof_filter_flate_decoder (iof *N);
iof * iof_filter_flate_encoder (iof *N);

iof * iof_filter_lzw_decoder (iof *N, int flags);
iof * iof_filter_lzw_encoder (iof *N, int flags);

iof * iof_filter_predictor_decoder (iof *N, int predictor, int rowsamples, int components, int compbits);
iof * iof_filter_predictor_encoder (iof *N, int predictor, int rowsamples, int components, int compbits);

#ifdef IMAGE_FILTER
iof * iof_filter_img_decoder (iof *N, int hooksource, int nobuffering, int format);
#define iof_filter_dct_decoder(N, hooksource, nobuffering) iof_filter_img_decoder(N, hooksource, nobuffering, IMG_JPG)
#define iof_filter_jpx_decoder(N, hooksource, nobuffering) iof_filter_img_decoder(N, hooksource, nobuffering, IMG_JPX)

iof * iof_filter_img_encoder (iof *N, uint32_t width, uint32_t height, uint8_t channels, int format);
#define iof_filter_dct_encoder(N, width, height, channels) iof_filter_img_encoder(N, width, height, channels, IMG_JPG)
//#define iof_filter_jpx_encoder(N, width, height, channels) iof_filter_img_encoder(N, width, height, channels, IMG_JPX)

iof * iof_filter_jbig_decoder (iof *N, iof *G, int jbigflags);
#endif

iof * iof_filter_rc4_decoder (iof *N, const void *key, size_t keylength);
iof * iof_filter_rc4_encoder (iof *N, const void *key, size_t keylength);

iof * iof_filter_aes_decoder (iof *N, const void *key, size_t keylength);
iof * iof_filter_aes_encoder (iof *N, const void *key, size_t keylength);

void iof_filters_init (void);
void iof_filters_free (void);

#endif