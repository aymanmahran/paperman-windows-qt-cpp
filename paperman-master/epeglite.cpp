/*
License: GPL-2
  An electronic filing cabinet: scan, print, stack, arrange
 Copyright (C) 2009 Simon Glass, chch-kiwi@users.sourceforge.net
 .
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

X-Comment: On Debian GNU/Linux systems, the complete text of the GNU General
 Public License can be found in the /usr/share/common-licenses/GPL file.
*/

#include "err.h"
#include "epeglite.h"


//#define _GNU_SOURCE
#include <stdio.h>


#include <jerror.h>

#define HAVE_FMEMOPEN
#define NOWARNINGS

// predeclarations
static Epeg_Image *_epeg_open_header(Epeg_Image *im);
static int _epeg_encode(Epeg_Image *im);
static int _epeg_decode(Epeg_Image *im);
static int _epeg_scale(Epeg_Image *im);
FILE *
_epeg_memfile_read_open(void *data, size_t size);

static void			_epeg_fatal_error_handler(j_common_ptr cinfo);
METHODDEF(void)		_jpeg_init_source(j_decompress_ptr cinfo);
METHODDEF(boolean)	_jpeg_fill_input_buffer(j_decompress_ptr cinfo);
METHODDEF(void)		_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes);
METHODDEF(void)		_jpeg_term_source(j_decompress_ptr cinfo);
METHODDEF(void)		_jpeg_init_destination(j_compress_ptr cinfo);
METHODDEF(boolean)	_jpeg_empty_output_buffer(j_compress_ptr cinfo);
METHODDEF(void)		_jpeg_term_destination(j_compress_ptr cinfo);

static const JOCTET fake_EOI[2] = { 0xFF, JPEG_EOI };

/**
* Open a JPEG image by filename.
* @param file The file path to open.
* @return A handle to the opened JPEG file, with the header decoded.
*
* This function opens the file indicated by the @p file parameter, and
* attempts to decode it as a jpeg file. If this failes, NULL is returned.
* Otherwise a valid handle to an open JPEG file is returned that can be used
* by other Epeg calls.
*
* The @p file must be a pointer to a valid C string, NUL (0 byte) terminated
* thats is a relative or absolute file path. If not results are not
* determined.
*
* See also: epeg_memory_open(), epeg_close()
*/
Epeg_Image *
epeg_file_open(const char *file)
{
	Epeg_Image *im;

	im = (Epeg_Image *)calloc(1, sizeof(Epeg_Image));
	if (!im) return NULL;

	im->in.file = strdup(file);
	if (!im->in.file)
	{
		free(im);
		return NULL;
	}

	im->in.f = fopen(im->in.file, "rb");
	if (!im->in.f)
	{
		epeg_close(im);
		return NULL;
	}
	//	fstat(fileno(im->in.f), &(im->stat_info));
	im->out.quality = 75;
	im->out.smoothing = 0;
	return _epeg_open_header(im);
}

/**
 * Open a JPEG image stored in memory.
 * @param data A pointer to the memory containing the JPEG data.
 * @param size The size of the memory segment containing the JPEG.
 * @return  A handle to the opened JPEG, with the header decoded.
 *
 * This function opens a JPEG file that is stored in memory pointed to by
 * @p data, and that is @p size bytes in size. If successful a valid handle
 * is returned, or on failure NULL is returned.
 *
 * See also: epeg_file_open(), epeg_close()
 */
Epeg_Image *
epeg_memory_open(unsigned char *data, int size)
{
   Epeg_Image *im;

   im = (Epeg_Image *)calloc(1, sizeof(Epeg_Image));
   im->in.f = _epeg_memfile_read_open(data, size);
   if (!im->in.f)
     {
    epeg_close(im);
    return NULL;
     }
   im->out.quality = 75;
   return _epeg_open_header(im);
}


/**
* Close an image handle.
* @param im A handle to an opened Epeg image.
*
* This closes an opened image handle and frees all memory associated with it.
* It does not free encoded data generated by epeg_memory_output_set() followed
* by epeg_encode() nor does it guarantee to free any data recieved by
* epeg_pixels_get(). Once an image handle is closed consider it invalid.
*/
void
epeg_close(Epeg_Image *im)
{
	if (!im) return;
	if (im->pixels)                   free(im->pixels);
	if (im->lines)                    free(im->lines);
	if (im->in.file)                  free(im->in.file);
//	if (!im->in.file)                 free(im->in.jinfo.src);
	if (im->in.f || im->in.mem.data)  jpeg_destroy_decompress(&(im->in.jinfo));
	if (im->in.f)                     fclose(im->in.f);
	if (im->out.file)                 free(im->out.file);
	if (!im->out.file)                free(im->out.jinfo.dest);
	if (im->out.f || im->in.mem.data) jpeg_destroy_compress(&(im->out.jinfo));
	if (im->out.f)                    fclose(im->out.f);
	free(im);
}


/*
 * Decide whether to emit a trace or warning message.
 * msg_level is one of:
 *   -1: recoverable corrupt-data warning, may want to abort.
 *    0: important advisory messages (always display to user).
 *    1: first level of tracing detail.
 *    2,3,...: successively more detailed tracing messages.
 * An application might override this method if it wanted to abort on warnings
 * or change the policy about which messages to display.
 */

METHODDEF(void)
_emit_message (j_common_ptr cinfo, int msg_level)
{
  struct jpeg_decompress_struct *decomp = (struct jpeg_decompress_struct *)cinfo;
  struct _epeg_error_mgr * err = (struct _epeg_error_mgr *)cinfo->err;

  if (msg_level < 0) {
    /* It's a warning message.  Since corrupt files may generate many warnings,
     * the policy implemented here is to show only the first warning,
     * unless trace_level >= 3.
     */
    if (err->pub.num_warnings == 0 || err->pub.trace_level >= 3)
      (*err->pub.output_message) (cinfo);
    /* Always count warnings in num_warnings. */
    err->pub.num_warnings++;
    err->last_valid_row = decomp->output_scanline * decomp->scale_denom / decomp->scale_num;
printf ("valid row = %d\n", err->last_valid_row);
  } else {
    /* It's a trace message.  Show it if trace_level >= msg_level. */
    if (err->pub.trace_level >= msg_level)
      (*err->pub.output_message) (cinfo);
  }
}

static Epeg_Image *
_epeg_open_header(Epeg_Image *im)
{
	struct jpeg_source_mgr *src_mgr = NULL;

	im->in.jinfo.err = jpeg_std_error(&(im->jerr.pub));
	im->jerr.pub.error_exit = _epeg_fatal_error_handler;
#ifdef NOWARNINGS
	im->jerr.pub.emit_message = _emit_message;
//	im->jerr.pub.output_message = _output_message;
//	im->jerr.pub.format_message = _format_message;
#endif

	if (setjmp(im->jerr.setjmp_buffer))
	{
error:
		epeg_close(im);
		im = NULL;
		return NULL;
	}

	jpeg_create_decompress(&(im->in.jinfo));
	jpeg_save_markers(&(im->in.jinfo), JPEG_APP0 + 7, 1024);
	jpeg_save_markers(&(im->in.jinfo), JPEG_COM,      65535);
	if (im->in.f != NULL)
	{
		jpeg_stdio_src(&(im->in.jinfo), im->in.f);
	}
	else
	{
		/* Setup RAM source manager. */
		src_mgr = (jpeg_source_mgr *)calloc(1, sizeof(struct jpeg_source_mgr));
		if (!src_mgr) goto error;
		src_mgr->init_source = _jpeg_init_source;
		src_mgr->fill_input_buffer = _jpeg_fill_input_buffer;
		src_mgr->skip_input_data = _jpeg_skip_input_data;
		src_mgr->resync_to_restart = jpeg_resync_to_restart;
		src_mgr->term_source = _jpeg_term_source;
		src_mgr->bytes_in_buffer = im->in.mem.size;
		src_mgr->next_input_byte = (JOCTET *) im->in.mem.data;
		im->in.jinfo.src = (struct jpeg_source_mgr *) src_mgr;
	}

	jpeg_read_header(&(im->in.jinfo), true);
	im->in.w = im->in.jinfo.image_width;
	im->in.h = im->in.jinfo.image_height;
	if (im->in.w < 1) goto error;
	if (im->in.h < 1) goto error;

	im->out.w = im->in.w;
	im->out.h = im->in.h;

	im->color_space = ((im->in.color_space = im->in.jinfo.out_color_space) == JCS_GRAYSCALE) ? EPEG_GRAY8 : EPEG_RGB8;
	if (im->in.color_space == JCS_CMYK) im->color_space = EPEG_CMYK;

	return im;
}


FILE *
_epeg_memfile_read_open(void *data, size_t size)
{
//#ifdef HAVE_FMEMOPEN
//   return (FILE *)fmemopen(data, size, "r");
//#else
   FILE *f;

   f = tmpfile();
   if (!f) return NULL;
   fwrite(data, size, 1, f);
   rewind(f);
   return f;
//#endif
}

void
_epeg_memfile_read_close(FILE *f)
{
#ifdef HAVE_FMEMOPEN
   fclose(f);
#else
   fclose(f);
#endif
}


typedef struct _Eet_Memfile_Write_Info Eet_Memfile_Write_Info;
struct _Eet_Memfile_Write_Info
{
   FILE *f;
   void **data;
   size_t *size;
};

static int                     _epeg_memfile_info_alloc_num = 0;
static int                     _epeg_memfile_info_num       = 0;
static Eet_Memfile_Write_Info *_epeg_memfile_info           = NULL;

FILE *
_epeg_memfile_write_open(void **data, size_t *size)
{
#ifdef HAVE_OPEN_MEMSTREAM
   return open_memstream((char **)data, size);
#else
   FILE *f;

   _epeg_memfile_info_num++;
   if (_epeg_memfile_info_num > _epeg_memfile_info_alloc_num)
     {
    Eet_Memfile_Write_Info *tmp;

    _epeg_memfile_info_alloc_num += 16;
    tmp = (Eet_Memfile_Write_Info *)realloc(_epeg_memfile_info,
              _epeg_memfile_info_alloc_num *
              sizeof(Eet_Memfile_Write_Info));
    if (!tmp)
      {
         _epeg_memfile_info_alloc_num -= 16;
         _epeg_memfile_info_num--;
         return NULL;
      }
    _epeg_memfile_info = tmp;
     }
   f = tmpfile();
   if (!f)
     {
    _epeg_memfile_info_num--;
    return NULL;
     }
   _epeg_memfile_info[_epeg_memfile_info_num - 1].f = f;
   _epeg_memfile_info[_epeg_memfile_info_num - 1].data = data;
   _epeg_memfile_info[_epeg_memfile_info_num - 1].size = size;
   return f;
#endif
}

void
_epeg_memfile_write_close(FILE *f)
{
#ifdef HAVE_OPEN_MEMSTREAM
   fclose(f);
#else
   int i;

   for (i = 0; i < _epeg_memfile_info_num; i++)
     {
    if (_epeg_memfile_info[i].f == f)
      {
         int j;

         fseek(f, 0, SEEK_END);
         (*(_epeg_memfile_info[i].size)) = ftell(f);
         rewind(f);
         (*(_epeg_memfile_info[i].data)) = malloc(*(_epeg_memfile_info[i].size));
         if (!(*(_epeg_memfile_info[i].data)))
           {
          fclose(f);
          (*(_epeg_memfile_info[i].size)) = 0;
          return;
           }
         fread((*(_epeg_memfile_info[i].data)), (*(_epeg_memfile_info[i].size)), 1, f);
         for (j = i + 1; j < _epeg_memfile_info_num; j++)
           _epeg_memfile_info[j - 1] = _epeg_memfile_info[j];
         _epeg_memfile_info_num--;
         fclose(f);
         return;
      }
     }
   fclose(f);
#endif
}


void epeg_size_get(Epeg_Image *im, int *w, int *h)
{
	if (w) *w = im->in.w;
	if (h) *h = im->in.h;
}

/**
* Set the encoding quality of the saved image.
* @param im A handle to an opened Epeg image.
* @param quality The quality of encoding from 0 to 100.
*
* Set the quality of the output encoded image. Values from 0 to 100
* inclusive are valid, with 100 being the maximum quality, and 0 being the
* minimum. If the quality is set equal to or above 90%, the output U and V
* color planes are encoded at 1:1 with the Y plane.
*
* The default quality is 75.
*
*/
void epeg_quality_set(Epeg_Image *im, int quality, int smoothing)
{
	if      (quality < 0)   quality = 0;
	else if (quality > 100) quality = 100;
	im->out.quality = quality;

	if (smoothing == 0)
		im->out.smoothing = 0;
	else
		im->out.smoothing = 1;

}

/**
* This saves the image to its specified destination.
* @param im A handle to an opened Epeg image.
*
* This saves the image @p im to its destination specified by
* epeg_file_output_set() or epeg_memory_output_set(). The image will be
* encoded at the deoded pixel size, using the quality, comment and thumbnail
* comment settings set on the image.
*
* retval 1 - error scale
*        2 - error encode
*        3 - error decode
*        4 - error decode ( setjmp )
*/
int epeg_encode(Epeg_Image *im)
{
	int ret;
	if ((ret = _epeg_decode(im)) != 0)
		return (ret == 2 ? 4 : 3);
	if (_epeg_scale(im) != 0)
		return 1;
	if (_epeg_encode(im) != 0)
		return 2;
	return 0;
}

/* just decode the image - don't convert to JPEG at the end. stride is the required byte width
of each line - the image is copied accordingly */

int epeg_raw(Epeg_Image *im, int stride)
{
    int ret, size;
    char *p;

    if ((ret = _epeg_decode(im)) != 0)
        return (ret == 2 ? 4 : 3);
    if (_epeg_scale(im) != 0)
        return 1;

    // work out stride
    size = stride * im->out.h;
//printf ("size = %d, %d, stride = %d, bytes = %d, in_stride = %d\n",im->out.w, im->out.h ,stride, size, in_stride);
    *im->out.mem.size = size;

    p = (char *)malloc(size);
    if (!p)
        return 2;

    *im->out.mem.data = (unsigned char *)p;
    return 0;
}

/* copy the preview data out, using the given height in case it has shrunk */

int epeg_copy(Epeg_Image *im, int width, int height, int stride)
{
    char *p, *s, *d, *end;
    int y, nc, in_stride;

    nc = im->in.jinfo.output_components;

    p = (char *)*im->out.mem.data;
    in_stride = im->in.jinfo.output_width * nc;

//    printf ("epeg_copy size = %d\n", height *

    // copy each line, reversing red and blue, and inverting
    for (y = 0; y < height; y++)
       if (nc == 3) for (s = (char *)im->pixels + in_stride * (height - y - 1), d = p + stride * y, end = d + 3 * width; d < end; s += 3, d += 3)
        {
        d [0] = s [2];
        d [1] = s [1];
        d [2] = s [0];
        }
       else
           memcpy (p + stride * y, (char *)im->pixels + in_stride * (height - 1 - y), stride);
   return 0;
}

/**
* Set the output file path for the image when saved.
* @param im A handle to an opened Epeg image.
* @param file The path to the output file.
*
* This sets the output file path name (either a full or relative path name)
* to where the file will be written when saved. @p file must be a NUL
* terminated C string conatining the path to the file to be saved to. If it is
* NULL, the image will not be saved to a file when calling epeg_encode().
*/
void epeg_file_output_set(Epeg_Image *im, const char *file)
{
	if (im->out.file) free(im->out.file);
	if (!file) im->out.file = NULL;
	else im->out.file = strdup(file);
}

/**
 * Set the output file to be a block of allocated memory.
 * @param im A handle to an opened Epeg image.
 * @param data A pointer to a pointer to a memory block.
 * @param size A pointer to a counter of the size of the memory block.
 *
 * This sets the output encoding of the image when saved to be allocated
 * memory. After epeg_close() is called the pointer pointed to by @p data
 * and the integer pointed to by @p size will contain the pointer to the
 * memory block and its size in bytes, respecitvely. The memory block can be
 * freed with the free() function call. If the save fails the pointer to the
 * memory block will be unaffected, as will the size.
 *
 */
void
epeg_memory_output_set(Epeg_Image *im, unsigned char **data, int *size)
{
   im->out.mem.data = data;
   im->out.mem.size = size;
}


struct epeg_destination_mgr
{
	struct jpeg_destination_mgr dst_mgr;
	Epeg_Image *im;
	unsigned char *buf;
};

static int _epeg_encode(Epeg_Image *im)
{
	struct epeg_destination_mgr *dst_mgr = NULL;
	int ok = 0;

	if ((im->out.w < 1) || (im->out.h < 1)) return 1;
	if (im->out.f) return 1;

	if (im->out.file)
	{
		im->out.f = fopen(im->out.file, "wb");
		if (!im->out.f)
		{
			im->error = 1;
			return 1;
		}
	}
	else
		im->out.f = NULL;

	im->out.jinfo.err = jpeg_std_error(&(im->jerr.pub));
	im->jerr.pub.error_exit = _epeg_fatal_error_handler;
#ifdef NOWARNINGS
//	im->jerr.pub.emit_message = _emit_message;
//	im->jerr.pub.output_message = _output_message;
//	im->jerr.pub.format_message = _format_message;
#endif

	if (setjmp(im->jerr.setjmp_buffer))
	{
		ok = 1;
		im->error = 1;
		goto done;
	}

	jpeg_create_compress(&(im->out.jinfo));
	if (im->out.f)
		jpeg_stdio_dest(&(im->out.jinfo), im->out.f);
	else
	{
		*(im->out.mem.data) = NULL;
		*(im->out.mem.size) = 0;
		/* Setup RAM destination manager */
		dst_mgr = (epeg_destination_mgr *)calloc(1, sizeof(struct epeg_destination_mgr));
		if (!dst_mgr) return 1;
		dst_mgr->dst_mgr.init_destination = _jpeg_init_destination;
		dst_mgr->dst_mgr.empty_output_buffer = _jpeg_empty_output_buffer;
		dst_mgr->dst_mgr.term_destination = _jpeg_term_destination;
		dst_mgr->im = im;
		dst_mgr->buf = (unsigned char *)malloc(65536);
		if (!dst_mgr->buf)
		{
			ok = 1;
			im->error = 1;
			goto done;
		}
		im->out.jinfo.dest = (struct jpeg_destination_mgr *)dst_mgr;
	}
	im->out.jinfo.image_width      = im->out.w;
	im->out.jinfo.image_height     = im->out.h;
	im->out.jinfo.input_components = im->in.jinfo.output_components;
//printf ("encoding %d x %d\n", im->out.w, im->out.h);
	im->out.jinfo.in_color_space   = im->in.jinfo.out_color_space;
	im->out.jinfo.dct_method	  = im->in.jinfo.dct_method;
	jpeg_set_defaults(&(im->out.jinfo));
	jpeg_set_quality(&(im->out.jinfo), im->out.quality, true);

	if (im->out.quality >= 90)
	{
		im->out.jinfo.comp_info[0].h_samp_factor = 1;
		im->out.jinfo.comp_info[0].v_samp_factor = 1;
		im->out.jinfo.comp_info[1].h_samp_factor = 1;
		im->out.jinfo.comp_info[1].v_samp_factor = 1;
		im->out.jinfo.comp_info[2].h_samp_factor = 1;
		im->out.jinfo.comp_info[2].v_samp_factor = 1;
	}
	jpeg_start_compress(&(im->out.jinfo), true);

	while ((int)im->out.jinfo.next_scanline < im->out.h)
		jpeg_write_scanlines(&(im->out.jinfo), &(im->lines[im->out.jinfo.next_scanline]), 1);
	jpeg_finish_compress(&(im->out.jinfo));

done:
	if ((im->in.f) || (im->in.mem.data != NULL)) jpeg_destroy_decompress(&(im->in.jinfo));
	if ((im->in.f) && (im->in.file)) fclose(im->in.f);
	if (dst_mgr)
	{
		if (dst_mgr->buf) free(dst_mgr->buf);
		free(dst_mgr);
		im->out.jinfo.dest = NULL;
	}
	jpeg_destroy_compress(&(im->out.jinfo));
	if ((im->out.f) && (im->out.file)) fclose(im->out.f);
	im->in.f = NULL;
	im->out.f = NULL;

	return ok;
}

void epeg_decode_size_set(Epeg_Image *im, int w, int h)
{
	if      (im->pixels) return;
	if      (w < 1)        w = 1;
	else if (w > im->in.w) w = im->in.w;
	if      (h < 1)        h = 1;
	else if (h > im->in.h) h = im->in.h;
	im->out.w = w;
	im->out.h = h;
	im->in.x = 0;
	im->in.y = 0;
	im->in.xw = im->in.w;
	im->in.xh = im->in.h;

}

void epeg_decode_bounds_set(Epeg_Image *im, int x, int y, int xw, int xh, int w, int h)
{
	if      (im->pixels) return;
	if      (w < 1)        w = 1;
	else if (w > im->in.w) w = im->in.w;
	if      (h < 1)        h = 1;
	else if (h > im->in.h) h = im->in.h;
	im->out.w = w;
	im->out.h = h;
	if      (x < 0)        x = 0;
	if      (y < 0)        y = 0;
	if      (x >= im->in.w) x = im->in.w - 1;
	if      (y >= im->in.h) y = im->in.h - 1;
	if      (xw <= 0)	xw = 1;
	if      (xh <= 0)	xh = 1;
	if	   ((x + xw) > im->in.w) xw = im->in.w - x;
	if	   ((y + xh) > im->in.h) xh = im->in.h - y;

	im->in.x = x;
	im->in.y = y;
	im->in.xw = xw;
	im->in.xh = xh;
}

static int _epeg_decode(Epeg_Image *im)
{
	int scale, scalew, scaleh, y;
	int ow, oh;

	JDIMENSION old_output_scanline = 1;

	if (im->pixels) return 1;
	if ((im->out.w < 1) || (im->out.h < 1)) return 1;

	ow = im->out.w;
	oh = im->out.h;

	if (im->out.smoothing)
	{
		ow *= 2; oh *=2;
		ow += 1; oh +=1;
	}

	scalew = im->in.xw / ow;
	scaleh = im->in.xh / oh;

	scale = scalew;
	if (scaleh < scalew) scale = scaleh;

	if      (scale > 8) scale = 8;
	else if (scale < 1) scale = 1;

//	printf("Jpeg scale : 1/%d\n", scale);

	im->in.jinfo.scale_num           = 1;
	im->in.jinfo.scale_denom         = scale;
	im->in.jinfo.do_fancy_upsampling = false;
	im->in.jinfo.do_block_smoothing  = false;
	if (scale == 1)
		im->in.jinfo.dct_method          = JDCT_ISLOW;
	else
		im->in.jinfo.dct_method          = JDCT_IFAST;

	switch (im->color_space)
	{
	case EPEG_GRAY8:
		im->in.jinfo.out_color_space = JCS_GRAYSCALE;
		im->in.jinfo.output_components = 1;
		break;

	case EPEG_YUV8:
		im->in.jinfo.out_color_space = JCS_YCbCr;
		break;

	case EPEG_RGB8:
	case EPEG_BGR8:
	case EPEG_RGBA8:
	case EPEG_BGRA8:
	case EPEG_ARGB32:
		im->in.jinfo.out_color_space = JCS_RGB;
		break;

	case EPEG_CMYK:
		im->in.jinfo.out_color_space = JCS_CMYK;
		im->in.jinfo.output_components = 4;
		break;

	default:
		break;
	}

	im->out.jinfo.err			= jpeg_std_error(&(im->jerr.pub));
	im->jerr.pub.error_exit		= _epeg_fatal_error_handler;
#ifdef NOWARNINGS
	im->jerr.pub.emit_message = _emit_message;
//	im->jerr.pub.output_message = _output_message;
//	im->jerr.pub.format_message = _format_message;
#endif

	if (setjmp(im->jerr.setjmp_buffer))
		return 2;

	jpeg_calc_output_dimensions(&(im->in.jinfo));
//printf ("width = %d, height = %d, components = %d\n", im->in.jinfo.output_width, im->in.jinfo.output_height, im->in.jinfo.output_components);

	im->pixels = (unsigned char *)malloc(im->in.jinfo.output_width * im->in.jinfo.output_height * im->in.jinfo.output_components);
	if (!im->pixels) return 1;

	im->lines = (unsigned char **)malloc(im->in.jinfo.output_height * sizeof(char *));
	if (!im->lines)
	{
		free(im->pixels);
		im->pixels = NULL;
		return 1;
	}

	jpeg_start_decompress(&(im->in.jinfo));

    im->jerr.last_valid_row = -1;
	for (y = 0; y < (int)im->in.jinfo.output_height; y++)
		im->lines[y] = im->pixels + (y * im->in.jinfo.output_components * im->in.jinfo.output_width);
	while (im->in.jinfo.output_scanline < im->in.jinfo.output_height)
	{
		if (old_output_scanline == im->in.jinfo.output_scanline)
		{
			jpeg_abort_decompress(&(im->in.jinfo));
			return 1;
		}
		old_output_scanline = im->in.jinfo.output_scanline;
		jpeg_read_scanlines(&(im->in.jinfo),
			&(im->lines[im->in.jinfo.output_scanline]),
			im->in.jinfo.rec_outbuf_height);
	}

	jpeg_finish_decompress(&(im->in.jinfo));

    // if we didn't get enough data, record the last valid row
    im->last_valid_row = im->jerr.last_valid_row == -1 ? -1 : im->jerr.last_valid_row;
	return 0;
}

static int _epeg_scale(Epeg_Image *im)
{
	unsigned char *dst, *row, *rowp, *rows;
	unsigned char *src, *src2, *src4, *src6, *src8;
	int            x, y, w, h, i;
	int				sx, sy, sxw, sxh, sw, sh;	// ratios of the input square on the scaled decoded img
	int				nc; // number of components in the img

	if ((im->in.w == im->out.w) && (im->in.h == im->out.h)) return 0;
	if (im->scaled) return 0;

	if ((im->out.w < 1) || (im->out.h < 1)) return 0;

	im->scaled = 1;
	nc = im->in.jinfo.output_components;

	w = im->out.w;
	h = im->out.h;

	sw = im->in.jinfo.output_width;
	sh = im->in.jinfo.output_height;

	sx = (im->in.x * sw) / im->in.w;
	sy = (im->in.y * sh) / im->in.h;
	sxw = (im->in.xw * sw) / im->in.w;
	sxh = (im->in.xh * sh) / im->in.h;

	if (im->out.smoothing)
	{
		int hh, ww;

		ww = 2*w+1;
		hh = 2*h+1;

		for (y = 0; y < h; y++)
		{
			int yy = y*2+1;

			row =  im->pixels + (((yy * sxh) / hh + sy) * nc * sw);
			rowp = im->pixels + ((((yy-1) * sxh) / hh + sy) * nc * sw);
			rows = im->pixels + ((((yy+1) * sxh) / hh + sy) * nc * sw);

			dst = im->pixels + (y * nc * sw);

			for (x = 0; x < w; x++)
			{
				int xx = x*2+1;

				src  = row  + ((((xx  ) * sxw) / ww + sx) * nc);
				src8 = row  + ((((xx-1) * sxw) / ww + sx) * nc);
				src4 = row  + ((((xx+1) * sxw) / ww + sx) * nc);
				src2 = rowp + ((((xx  ) * sxw) / ww + sx) * nc);
				src6 = rows + ((((xx  ) * sxw) / ww + sx) * nc);
				for (i = 0; i < nc; i++)
					dst[i] = (2*src[i] + src2[i] + src4[i] + src6[i] + src8[i])/6;
				dst += nc;
			}
		}
	}
	else
	{
		for (y = 0; y < h; y++)
		{
			row = im->pixels + (((y * sxh) / h + sy) * nc * sw);
			dst = im->pixels + (y * nc * sw);

			for (x = 0; x < w; x++)
			{
				src = row + (((x * sxw) / w + sx) * nc);
				for (i = 0; i < nc; i++)
					dst[i] = src[i];
				dst += nc;
			}
		}
	}

	return 0;
}


// -------------------------------- JPG callbacks -------------------------
METHODDEF(void)	_jpeg_init_source(j_decompress_ptr cinfo)
{
    UNUSED(cinfo);
}

METHODDEF(boolean) _jpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
	WARNMS(cinfo, JWRN_JPEG_EOF);

	/* Insert a fake EOI marker */
	cinfo->src->next_input_byte = fake_EOI;
	cinfo->src->bytes_in_buffer = sizeof(fake_EOI);
	return true;
}


METHODDEF(void)	_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes > (long)(cinfo)->src->bytes_in_buffer)
		ERREXIT(cinfo, 0);

	(cinfo)->src->next_input_byte += num_bytes;
	(cinfo)->src->bytes_in_buffer -= num_bytes;
}

METHODDEF(void)	_jpeg_term_source(j_decompress_ptr cinfo)
{
    UNUSED(cinfo);
}

static void _epeg_fatal_error_handler(j_common_ptr cinfo)
{
	emptr errmgr;

	errmgr = (emptr)cinfo->err;
	longjmp(errmgr->setjmp_buffer, 1);
	return;
}

METHODDEF(void) _jpeg_init_destination(j_compress_ptr cinfo)
{
	struct epeg_destination_mgr *dst_mgr;

	dst_mgr = (struct epeg_destination_mgr *)cinfo->dest;
	dst_mgr->dst_mgr.free_in_buffer = 65536;
	dst_mgr->dst_mgr.next_output_byte = (JOCTET *)dst_mgr->buf;
}

METHODDEF(boolean) _jpeg_empty_output_buffer(j_compress_ptr cinfo)
{
	struct epeg_destination_mgr *dst_mgr;
	unsigned char *p;
	int psize;

	dst_mgr = (struct epeg_destination_mgr *)cinfo->dest;
	psize = *(dst_mgr->im->out.mem.size);
	*(dst_mgr->im->out.mem.size) += 65536;

	p = (unsigned char *)realloc(*(dst_mgr->im->out.mem.data), *(dst_mgr->im->out.mem.size));
	if (p)
	{
		memcpy(p + psize, dst_mgr->buf, 65536);
		dst_mgr->dst_mgr.free_in_buffer = 65536;
		dst_mgr->dst_mgr.next_output_byte = (JOCTET *)dst_mgr->buf;
	}
	else
		return false;
	return true;
}

METHODDEF(void) _jpeg_term_destination(j_compress_ptr cinfo)
{
	struct epeg_destination_mgr *dst_mgr;
	unsigned char *p;
	int psize;

	dst_mgr = (struct epeg_destination_mgr *)cinfo->dest;
	psize = *(dst_mgr->im->out.mem.size);
	*(dst_mgr->im->out.mem.size) += 65536 - dst_mgr->dst_mgr.free_in_buffer;
	p = (unsigned char *)realloc(*(dst_mgr->im->out.mem.data), *(dst_mgr->im->out.mem.size));
	if (p)
	{
		*(dst_mgr->im->out.mem.data) = p;
		memcpy(p + psize, dst_mgr->buf, 65536 - dst_mgr->dst_mgr.free_in_buffer);
	}
}
