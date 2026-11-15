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

} // namespace BmpWriter

#endif
