#include "rgbeReader.h"

#include <string.h> // strcmp
#include <stdlib.h>

//#include "tacLibrary\tacTypes.h"
#include "tacLibrary\tacMath.h"


#include <ctype.h>

/* adapted from rgbe.cpp -- 23 Nov 2007 */
/* minimal header reading.  modify if you want to parse more information */
int RGBE_ReadHeader(FILE *fp, int *width, int *height, Rgbe_header_info *info)
{
  char buf[128];
  int found_format;
  float tempf;
  int i;

  found_format = 0;
  if (info)
  {
    info->valid = 0;
    info->programtype[0] = 0;
    info->gamma = info->exposure = 1.0;
  }
  
  if (fgets(buf,sizeof(buf)/sizeof(buf[0]),fp) == NULL)
  {
    printf("RGBE read error\n");
  return RGBE_RETURN_FAILURE;
  }

  if ((buf[0] != '#')||(buf[1] != '?'))
  {
    /* if you want to require the magic token then uncomment the next line */
    /*return rgbe_error(rgbe_format_error,"bad initial token"); */
  }
  else if (info)
  {
    info->valid |= RGBE_VALID_PROGRAMTYPE;
    
    for( i = 0; i < sizeof(info->programtype)-1; i++ )
    {
      if ((buf[i+2] == 0) || isspace(buf[i+2]))
      {
        break;
      }
      info->programtype[i] = buf[i+2];
    }
    
    info->programtype[i] = 0;
    
    if (fgets(buf, sizeof(buf)/sizeof(buf[0]), fp) == 0)
    {
      printf("RGBE read error\n");
    return RGBE_RETURN_FAILURE;
    }
  }
  for(;;)
  {
    if ((buf[0] == 0))
    {
       printf(" RGBE Error -- no FORMAT specifier found");
       return RGBE_RETURN_FAILURE;
    }
    else if (strcmp(buf, "\n") == 0 )
    {
      break;
    }
    else if (strcmp(buf,"FORMAT=32-bit_rle_rgbe\n") == 0)
      { } // do nothing, just makes sure its valid. previous code: break;       // format found so break out of loop
    else if (info && (sscanf(buf,"GAMMA=%g",&tempf) == 1)) 
    {
      info->gamma = tempf;
      info->valid |= RGBE_VALID_GAMMA;
    }
    else if (info && (sscanf(buf,"EXPOSURE=%g",&tempf) == 1)) 
    {
      info->exposure = tempf;
      info->valid |= RGBE_VALID_EXPOSURE;
    }

    if (fgets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
    {
      printf("RGBE read error\n");
      return RGBE_RETURN_FAILURE;
    }
  }
  if (fgets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
  {
    printf("RGBE read error\n");
    return RGBE_RETURN_FAILURE;
  }
    
  /*
  if (strcmp(buf,"\n") != 0)
    return rgbe_error(rgbe_format_error,
          "missing blank line after FORMAT specifier");
  if (fgets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
    return rgbe_error(rgbe_read_error,NULL);
  */
  if (sscanf(buf,"-Y %d +X %d", height, width) < 2)
  {
    printf("missing image size specifier\n");
    return RGBE_RETURN_FAILURE;
  }
  return RGBE_RETURN_SUCCESS;
}


/* COPY version -- need to switch it to read vec3 data */

/* standard conversion from rgbe to float pixels */
/* note: Ward uses ldexp(col+0.5,exp-(128+8)).  However we wanted pixels */
/*       in the range [0,1] to map back into the range [0,1].            */

static void rgbe2vec3(v3& hdrData, const unsigned char rgbe[4])
{
  float f;

  if (rgbe[3] != 0)
  { 
    /*nonzero pixel*/
    f = (float)ldexp(1.0,rgbe[3]-(int)(128+8));
    hdrData[0] = rgbe[0]*f;
    hdrData[1] = rgbe[1]*f;
    hdrData[2] = rgbe[2]*f;
  }
  else
  {
    Zero( hdrData );    // Don't remove braces!
  }
}

int RGBE_ReadPixels(FILE *fp, v3 *data, int numpixels)
{
  unsigned char rgbe[4];
  int i;

  for (i=0; i<numpixels; i++)
  {
    if (fread(rgbe, sizeof(rgbe), 1, fp) < 1)
    {
      printf("RGBE read error\n");
      return RGBE_RETURN_FAILURE;
    }
    rgbe2vec3(data[i], rgbe);
  }
  return RGBE_RETURN_SUCCESS;

}

int RGBE_ReadPixels_RLE(FILE *fp, v3 *data, int scanline_width, int num_scanlines)
{
  unsigned char rgbe[4], *scanline_buffer, *ptr, *ptr_end;
  int i, count;
  unsigned char buf[2];

  if ((scanline_width < 8)||(scanline_width > 0x7fff))
    /* run length encoding is not allowed so read flat*/
    return RGBE_ReadPixels(fp,data,scanline_width*num_scanlines);
  scanline_buffer = NULL;
  /* read in each successive scanline */
  while(num_scanlines > 0) {
    if (fread(rgbe,sizeof(rgbe),1,fp) < 1) 
    {
      free(scanline_buffer);
      printf("RGBE read error\n");
      return RGBE_RETURN_FAILURE;
    }
    if ((rgbe[0] != 2)||(rgbe[1] != 2)||(rgbe[2] & 0x80)) 
    {
      /* this file is not run length encoded */
      rgbe2vec3(*data, rgbe);
      data++;
      free(scanline_buffer);
      return RGBE_ReadPixels(fp,data,scanline_width*num_scanlines-1);
    }
    if ((((int)rgbe[2])<<8 | rgbe[3]) != scanline_width)
    {
      free(scanline_buffer);
      printf("RGBE format error -- wrong scanline width\n");
      return RGBE_RETURN_FAILURE;
    }
    if (scanline_buffer == NULL)
      scanline_buffer = (unsigned char *)
    malloc(sizeof(unsigned char)*4*scanline_width);
    if (scanline_buffer == NULL)
    {
      printf("RGBE memory error -- unable to allocate buffer space\n");
      return RGBE_RETURN_FAILURE;
    }

    ptr = &scanline_buffer[0];
    /* read each of the four channels for the scanline into the buffer */
    for(i=0;i<4;i++)
    {
      ptr_end = &scanline_buffer[(i+1)*scanline_width];
      while(ptr < ptr_end)
      {
        if (fread(buf,sizeof(buf[0])*2,1,fp) < 1)
        {
          free(scanline_buffer);
          printf("RGBE read error\n");
          return RGBE_RETURN_FAILURE;
        }
        if (buf[0] > 128)
        {
          /* a run of the same value */
          count = buf[0]-128;
          if ((count == 0)||(count > ptr_end - ptr))
          {
            free(scanline_buffer);
            printf("RGBE format error -- bad scanline data\n");
            return RGBE_RETURN_FAILURE;
          }
          while(count-- > 0)
            *ptr++ = buf[1];
        }
        else
        {
          /* a non-run */
          count = buf[0];
          if ((count == 0)||(count > ptr_end - ptr))
          {
            free(scanline_buffer);
            printf("RGBE format error -- bad scanline data\n");
            return RGBE_RETURN_FAILURE;
          }
          *ptr++ = buf[1];
          if (--count > 0)
          {
            if (fread(ptr,sizeof(*ptr)*count,1,fp) < 1)
            {
              free(scanline_buffer);
              printf("RGBE read error\n");
              return RGBE_RETURN_FAILURE;
            }
            ptr += count;
          }
        }
      }  // end while
    }    // end for

    // now convert data from buffer into floats
    for(i=0;i<scanline_width;i++)
    {
      rgbe[0] = scanline_buffer[i];
      rgbe[1] = scanline_buffer[i+scanline_width];
      rgbe[2] = scanline_buffer[i+2*scanline_width];
      rgbe[3] = scanline_buffer[i+3*scanline_width];
      rgbe2vec3(*data, rgbe);
      data++;
    }

    num_scanlines--;
  }  // end while
  free(scanline_buffer);
  return RGBE_RETURN_SUCCESS;
}  // end proc

