/*
 * $Id: png.c,v 1.6 2003/06/05 20:33:07 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 */


#include <stdio.h>
#include <errno.h>
#include <png.h>

#include "../data.h"
#include "../pixmap.h"



void moth_format_save_png (moth_format format, const char *filename,
    moth_pixmap pixmap);


INTERNAL
struct moth_format_definition_s moth_format_definition_png =
{
  1,          /* is pixmap */
  &moth_format_save_png,

  /* vector interface */
  NULL,       /* start_image */
  NULL,       /* draw_title */
  NULL,       /* draw_watermark */
  NULL,       /* draw_canvas */
  NULL,       /* draw_axes */
  NULL,       /* draw_areas */
  NULL,       /* draw_grid */
  NULL,       /* draw_line */
  NULL,       /* draw_points */
  NULL,       /* draw_segments */
  NULL        /* finish_image */
};



void
moth_format_save_png
(
  moth_format format,
  const char  *filename,
  moth_pixmap pixmap
)
{
  FILE          *fp;
  png_structp   png_ptr;
  png_infop     info_ptr;
  unsigned char *image_data, **row_pointers;
  unsigned int  width, height, rowspan, row;

  fp = fopen (filename, "wb");
  if (!fp)
    die ("couldn't open file \"%s\":  %s", filename, strerror(errno));

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);

  if (!png_ptr) {
    fclose (fp);
    die ("couldn't find memory for writing PNG image");
  }

  /* allocate/initialize the image information data */
  info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr) {
    fclose (fp);
    png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
    die ("couldn't find memory for writing PNG image");
  }

  /* 
   * png docs say this is required.  I generally try to avoid setjump/longjup,
   * but since we're going to exit pretty quickly, I guess this is OK.
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    fclose (fp);
    png_destroy_write_struct (&png_ptr, &info_ptr);
    die ("error occured while writing to file \"%s\"", filename);
  }

  png_init_io (png_ptr, fp);
  moth_pixmap_get_geometry (pixmap, &width, &height);
  png_set_IHDR (png_ptr, info_ptr, width, height,
      8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  /* FUTURE   add image title to file */

  png_write_info (png_ptr, info_ptr);

  moth_pixmap_get_data (pixmap, &image_data, &rowspan, NULL);
  row_pointers = (unsigned char **) malloc (height*sizeof(png_bytep));
  for (row=0; row<height; ++row)
    row_pointers[row] = &(image_data[row*rowspan]);

  png_set_rows (png_ptr, info_ptr, row_pointers);
  png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  png_destroy_write_struct (&png_ptr, &info_ptr);
  free (row_pointers);
  fclose (fp);
}



