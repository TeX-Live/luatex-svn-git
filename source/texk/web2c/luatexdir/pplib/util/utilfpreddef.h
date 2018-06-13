#ifndef UTIL_FPRED_H
#define UTIL_FPRED_H

#define predictor_component_t unsigned short
#define predictor_pixel1b_t unsigned int

typedef struct predictor_state {
  int default_predictor;                      /* default predictor indicator */
  int current_predictor;                      /* current predictor, possibly taken from algorithm marker in PNG data */
  int rowsamples;                             /* number of pixels in a scanline (/DecodeParms << /Columns ... >>) */
  int compbits;                               /* number of bits per component (/DecodeParms << /BitsPerComponent ... >>) */
  int components;                             /* number of components (/DecodeParms << /Colors ... >>) */
  uint8_t *buffer;                            /* temporary private buffer area */
  uint8_t *rowin;                             /* an input row buffer position */
  int rowsize;                                /* size of a current scanline in bytes (rounded up) */
  int rowend;                                 /* an input buffer end position */
  int rowindex;                               /* an output buffer position */
  union {
    struct {                                  /* used by PNG predictor codecs */
      uint8_t *rowup, *rowsave;               /* previous scanline buffers */
      int predictorbyte;                      /* flag indicating that algorithm byte is read/written */
      int pixelsize;                          /* number of bytes per pixel (rounded up) */
    };
    struct {                                  /* used by TIFF predictor codecs */
      union {
        predictor_component_t *prevcomp;      /* an array of left pixel components */
        predictor_pixel1b_t *prevpixel;       /* left pixel value stored on a single integer (for 1bit color-depth) */
      };
      int compin, compout;                    /* bit stream buffers */
      int bitsin, bitsout;                    /* bit stream counters */
      int sampleindex;                        /* pixel counter */
      int compindex;                          /* component counter */
      int pixbufsize;                         /* size of pixel buffer in bytes */
    };
  };
  int flush;
  int status;
} predictor_state;

#endif