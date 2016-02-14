/*
 * $Id: jpeg.c,v 1.5 2003/06/05 20:33:07 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 */


#include <stdio.h>
#include <errno.h>
#include <jpeglib.h>

#include "../data.h"
#include "../pixmap.h"



void moth_format_save_jpeg (moth_format format, const char *filename,
    moth_pixmap pixmap);


INTERNAL
struct moth_format_definition_s moth_format_definition_jpeg =
{
  1,          /* is pixmap */
  &moth_format_save_jpeg,

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
moth_format_save_jpeg
(
  moth_format format,
  const char  *filename,
  moth_pixmap pixmap
)
{
  FILE            *fp;
  unsigned int    width, height;
  unsigned char   *data;
  unsigned int    rowspan, bpp;

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;

  fp = fopen (filename, "wb");
  if (!fp)
    die ("couldn't open file \"%s\":  %s", filename, strerror(errno));

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);
  jpeg_stdio_dest (&cinfo, fp);

  moth_pixmap_get_geometry (pixmap, &width, &height);
  moth_pixmap_get_data (pixmap, &data, &rowspan, &bpp);
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = bpp;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults (&cinfo);

  jpeg_start_compress (&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height) {
    JSAMPROW row[1];
    row[0] = & data[cinfo.next_scanline * rowspan];
    jpeg_write_scanlines (&cinfo, row, 1);
  }
  jpeg_finish_compress (&cinfo);

  jpeg_destroy_compress (&cinfo);
  fclose (fp);
}



