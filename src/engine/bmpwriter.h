// The loader reads the collected image straight out of the writer's private object.

#ifndef HYPERBLADE_BMPWRITER_H
#define HYPERBLADE_BMPWRITER_H

#include "engine/ijgjpeg.h"

// The private version of the output object, as wrbmp.c declares it.
struct bmp_dest_struct
{
    struct djpeg_dest_struct pub;

    boolean is_os2;

    jvirt_sarray_ptr whole_image;
    JDIMENSION       data_width;
    JDIMENSION       row_width;
    int              pad_bytes;
    JDIMENSION       cur_output_row;
};

typedef struct bmp_dest_struct* bmp_dest_ptr;

namespace BmpWriter
{

djpeg_dest_ptr __cdecl Create(j_decompress_ptr cinfo, boolean is_os2);
void __cdecl           start_output_bmp(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo);
void __cdecl           put_pixel_rows(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo, JDIMENSION rows_supplied);
void __cdecl           put_gray_rows(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo, JDIMENSION rows_supplied);
void __cdecl           finish_output_bmp(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo);
void                   write_bmp_header(j_decompress_ptr cinfo, bmp_dest_ptr dest);
void                   write_os2_header(j_decompress_ptr cinfo, bmp_dest_ptr dest);
void                   write_colormap(j_decompress_ptr cinfo, bmp_dest_ptr dest, int map_colors, int map_entry_size);

} // namespace BmpWriter

#endif
