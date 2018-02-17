#pragma once
#include <stdio.h>
#include "tacLibrary\tacMath.h"

struct Rgbe_header_info{
  int valid;            /* indicate which fields are valid */
  char programtype[16]; /* listed at beginning of file to identify it
                         * after "#?".  defaults to "RGBE" */
  float gamma;          /* image has already been gamma corrected with
                         * given gamma.  defaults to 1.0 (no correction) */
  float exposure;       /* a value of 1.0 in an image corresponds to
												 * <exposure> watts/steradian/m^2.
						             * defaults to 1.0 */
} ;

/* flags indicating which fields in an rgbe_header_info are valid */
#define RGBE_VALID_PROGRAMTYPE 0x01
#define RGBE_VALID_GAMMA       0x02
#define RGBE_VALID_EXPOSURE    0x04

#ifdef __cplusplus
/* define if your compiler understands inline commands */
#define INLINE inline
#else
#define INLINE
#endif

/* offsets to red, green, and blue components in a data (float) pixel */
#define RGBE_DATA_RED    0
#define RGBE_DATA_GREEN  1
#define RGBE_DATA_BLUE   2
/* number of floats per pixel */
#define RGBE_DATA_SIZE   3

#define RGBE_RETURN_SUCCESS 0
#define RGBE_RETURN_FAILURE -1
int RGBE_ReadHeader(FILE *fp, int *width, int *height, Rgbe_header_info *info );
int RGBE_ReadPixels_RLE(FILE *fp, v3 *data, int scanline_width, int num_scanlines );