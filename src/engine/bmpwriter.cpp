// djpeg's BMP writer, wrbmp.c from IJG 6a, changed to check the output file before writing.
// wrbmp.c is copyright (C) 1994-1996, Thomas G. Lane, and this file stays under the IJG
// licence in third_party/jpeg-6a/README.

#include "bmpwriter.h"

// FUNCTION: HYPERX 0x00494693
djpeg_dest_ptr __cdecl BmpWriter::Create(j_decompress_ptr cinfo, boolean is_os2)
{
    bmp_dest_ptr dest;

    dest = (bmp_dest_ptr)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE, sizeof(struct bmp_dest_struct));

    dest->pub.start_output  = start_output_bmp;
    dest->pub.finish_output = finish_output_bmp;
    dest->is_os2            = is_os2;

    if (cinfo->out_color_space == JCS_GRAYSCALE)
    {
        dest->pub.put_pixel_rows = put_gray_rows;
    }
    else if (cinfo->out_color_space == JCS_RGB)
    {
        if (cinfo->quantize_colors)
        {
            dest->pub.put_pixel_rows = put_gray_rows;
        }
        else
        {
            dest->pub.put_pixel_rows = put_pixel_rows;
        }
    }
    else
    {
        ERREXIT(cinfo, JERR_BMP_COLORSPACE);
    }

    jpeg_calc_output_dimensions(cinfo);

    // A bitmap row is padded out to a four byte boundary.
    JDIMENSION row_width = cinfo->output_width * cinfo->output_components;
    dest->data_width     = row_width;

    while ((row_width & 3) != 0)
    {
        row_width++;
    }

    dest->row_width = row_width;
    dest->pad_bytes = (int)(row_width - dest->data_width);

    dest->whole_image = (*cinfo->mem->request_virt_sarray)(
        (j_common_ptr)cinfo, JPOOL_IMAGE, FALSE, row_width, cinfo->output_height, (JDIMENSION)1);

    dest->cur_output_row = 0;

    if (cinfo->progress != NULL)
    {
        cd_progress_ptr progress = (cd_progress_ptr)cinfo->progress;

        progress->total_extra_passes++;
    }

    dest->pub.buffer        = (*cinfo->mem->alloc_sarray)((j_common_ptr)cinfo, JPOOL_IMAGE, row_width, (JDIMENSION)1);
    dest->pub.buffer_height = 1;

    return (djpeg_dest_ptr)dest;
}

// The original's link folded this onto every other empty hook, so it carries their shared address.
// FUNCTION: HYPERX 0x0049ce32
void __cdecl BmpWriter::start_output_bmp(j_decompress_ptr, djpeg_dest_ptr)
{
}

// FUNCTION: HYPERX 0x00494759
void __cdecl BmpWriter::put_pixel_rows(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo, JDIMENSION)
{
    bmp_dest_ptr dest = (bmp_dest_ptr)dinfo;
    JSAMPARRAY   image_ptr;
    JDIMENSION   col;

    image_ptr = (*cinfo->mem->access_virt_sarray)((j_common_ptr)cinfo, dest->whole_image, dest->cur_output_row, (JDIMENSION)1, TRUE);
    dest->cur_output_row++;

    // A bitmap wants its components in BGR order.
    JSAMPROW inptr  = dest->pub.buffer[0];
    JSAMPROW outptr = image_ptr[0];

    for (col = cinfo->output_width; col > 0; col--)
    {
        outptr[2] = *inptr++;
        outptr[1] = *inptr++;
        outptr[0] = *inptr++;
        outptr += 3;
    }

    int pad = dest->pad_bytes;

    while (--pad >= 0)
    {
        *outptr++ = 0;
    }
}

// FUNCTION: HYPERX 0x004947bd
void __cdecl BmpWriter::put_gray_rows(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo, JDIMENSION)
{
    bmp_dest_ptr dest = (bmp_dest_ptr)dinfo;
    JSAMPARRAY   image_ptr;
    JDIMENSION   col;

    image_ptr = (*cinfo->mem->access_virt_sarray)((j_common_ptr)cinfo, dest->whole_image, dest->cur_output_row, (JDIMENSION)1, TRUE);
    dest->cur_output_row++;

    JSAMPROW inptr  = dest->pub.buffer[0];
    JSAMPROW outptr = image_ptr[0];

    for (col = cinfo->output_width; col > 0; col--)
    {
        *outptr++ = *inptr++;
    }

    int pad = dest->pad_bytes;

    while (--pad >= 0)
    {
        *outptr++ = 0;
    }
}

// FUNCTION: HYPERX 0x00494813
void __cdecl BmpWriter::finish_output_bmp(j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
    bmp_dest_ptr    dest    = (bmp_dest_ptr)dinfo;
    FILE* const     outfile = dest->pub.output_file;
    JSAMPARRAY      image_ptr;
    JDIMENSION      row;
    JDIMENSION      col;
    cd_progress_ptr progress = (cd_progress_ptr)cinfo->progress;

    if (dest->is_os2)
    {
        write_os2_header(cinfo, dest);
    }
    else
    {
        write_bmp_header(cinfo, dest);
    }

    if (outfile != NULL)
    {
        // The rows come back out of the virtual array bottom first.
        for (row = cinfo->output_height; row > 0; row--)
        {
            if (progress != NULL)
            {
                progress->pub.pass_counter = (long)(cinfo->output_height - row);
                progress->pub.pass_limit   = cinfo->output_height;
                (*progress->pub.progress_monitor)((j_common_ptr)cinfo);
            }

            image_ptr = (*cinfo->mem->access_virt_sarray)(
                (j_common_ptr)cinfo, dest->whole_image, row - 1, (JDIMENSION)1, FALSE);
            JSAMPROW data_ptr = image_ptr[0];

            for (col = dest->row_width; col > 0; col--)
            {
                putc(GETJSAMPLE(*data_ptr), outfile);
                data_ptr++;
            }
        }

        if (progress != NULL)
        {
            progress->completed_extra_passes++;
        }

        fflush(outfile);

        if (ferror(outfile))
        {
            ERREXIT(cinfo, JERR_FILE_WRITE);
        }
    }
}

#define PUT_2B(array, offset, value)             \
    (array[offset]     = (char)((value) & 0xFF), \
     array[offset + 1] = (char)(((value) >> 8) & 0xFF))

#define PUT_4B(array, offset, value)                     \
    (array[offset]     = (char)((value) & 0xFF),         \
     array[offset + 1] = (char)(((value) >> 8) & 0xFF),  \
     array[offset + 2] = (char)(((value) >> 16) & 0xFF), \
     array[offset + 3] = (char)(((value) >> 24) & 0xFF))

// FUNCTION: HYPERX 0x004948d1
void BmpWriter::write_bmp_header(j_decompress_ptr cinfo, bmp_dest_ptr dest)
{
    char bmpfileheader[14];
    char bmpinfoheader[40];
    int  bits_per_pixel;
    int  cmap_entries;

    if (cinfo->out_color_space == JCS_RGB)
    {
        if (cinfo->quantize_colors)
        {
            bits_per_pixel = 8;
            cmap_entries   = 256;
        }
        else
        {
            bits_per_pixel = 24;
            cmap_entries   = 0;
        }
    }
    else
    {
        // Grayscale still needs a 256 entry map, faked here.
        bits_per_pixel = 8;
        cmap_entries   = 256;
    }

    INT32 headersize = 14 + 40 + cmap_entries * 4;
    INT32 bfSize     = headersize + (INT32)dest->row_width * (INT32)cinfo->output_height;

    memset(bmpfileheader, 0, sizeof(bmpfileheader));
    memset(bmpinfoheader, 0, sizeof(bmpinfoheader));

    bmpfileheader[0] = 0x42;
    bmpfileheader[1] = 0x4D;
    PUT_4B(bmpfileheader, 2, bfSize);
    PUT_4B(bmpfileheader, 10, headersize);

    PUT_2B(bmpinfoheader, 0, 40);
    PUT_4B(bmpinfoheader, 4, cinfo->output_width);
    PUT_4B(bmpinfoheader, 8, cinfo->output_height);
    PUT_2B(bmpinfoheader, 12, 1);
    PUT_2B(bmpinfoheader, 14, bits_per_pixel);

    if (cinfo->density_unit == 2)
    {
        PUT_4B(bmpinfoheader, 24, (INT32)(cinfo->X_density * 100));
        PUT_4B(bmpinfoheader, 28, (INT32)(cinfo->Y_density * 100));
    }

    PUT_2B(bmpinfoheader, 32, cmap_entries);

    // Not stock: this build checks the file before writing to it.
    if (dest->pub.output_file != NULL)
    {
        if (fwrite(bmpfileheader, 1, 14, dest->pub.output_file) != (size_t)14)
        {
            ERREXIT(cinfo, JERR_FILE_WRITE);
        }

        if (fwrite(bmpinfoheader, 1, 40, dest->pub.output_file) != (size_t)40)
        {
            ERREXIT(cinfo, JERR_FILE_WRITE);
        }
    }

    if (cmap_entries > 0)
    {
        write_colormap(cinfo, dest, cmap_entries, 4);
    }
}

// FUNCTION: HYPERX 0x00494aa9
void BmpWriter::write_os2_header(j_decompress_ptr cinfo, bmp_dest_ptr dest)
{
    char bmpfileheader[14];
    char bmpcoreheader[12];
    int  bits_per_pixel;
    int  cmap_entries;

    if (cinfo->out_color_space == JCS_RGB)
    {
        if (cinfo->quantize_colors)
        {
            bits_per_pixel = 8;
            cmap_entries   = 256;
        }
        else
        {
            bits_per_pixel = 24;
            cmap_entries   = 0;
        }
    }
    else
    {
        bits_per_pixel = 8;
        cmap_entries   = 256;
    }

    INT32 headersize = 14 + 12 + cmap_entries * 3;
    INT32 bfSize     = headersize + (INT32)dest->row_width * (INT32)cinfo->output_height;

    memset(bmpfileheader, 0, sizeof(bmpfileheader));
    memset(bmpcoreheader, 0, sizeof(bmpcoreheader));

    bmpfileheader[0] = 0x42;
    bmpfileheader[1] = 0x4D;
    PUT_4B(bmpfileheader, 2, bfSize);
    PUT_4B(bmpfileheader, 10, headersize);

    PUT_2B(bmpcoreheader, 0, 12);
    PUT_2B(bmpcoreheader, 4, cinfo->output_width);
    PUT_2B(bmpcoreheader, 6, cinfo->output_height);
    PUT_2B(bmpcoreheader, 8, 1);
    PUT_2B(bmpcoreheader, 10, bits_per_pixel);

    if (dest->pub.output_file != NULL)
    {
        if (fwrite(bmpfileheader, 1, 14, dest->pub.output_file) != (size_t)14)
        {
            ERREXIT(cinfo, JERR_FILE_WRITE);
        }

        if (fwrite(bmpcoreheader, 1, 12, dest->pub.output_file) != (size_t)12)
        {
            ERREXIT(cinfo, JERR_FILE_WRITE);
        }
    }

    if (cmap_entries > 0)
    {
        write_colormap(cinfo, dest, cmap_entries, 3);
    }
}

// FUNCTION: HYPERX 0x00494bee
void BmpWriter::write_colormap(j_decompress_ptr cinfo, bmp_dest_ptr dest, int map_colors, int map_entry_size)
{
    JSAMPARRAY  colormap   = cinfo->colormap;
    const int   num_colors = cinfo->actual_number_of_colors;
    FILE* const outfile    = dest->pub.output_file;
    int         i;

    if (colormap != NULL)
    {
        if (cinfo->out_color_components == 3)
        {
            for (i = 0; i < num_colors; i++)
            {
                putc(GETJSAMPLE(colormap[2][i]), outfile);
                putc(GETJSAMPLE(colormap[1][i]), outfile);
                putc(GETJSAMPLE(colormap[0][i]), outfile);

                if (map_entry_size == 4)
                {
                    putc(0, outfile);
                }
            }
        }
        else
        {
            // A grayscale map, which only happens when grayscale output is quantised.
            for (i = 0; i < num_colors; i++)
            {
                putc(GETJSAMPLE(colormap[0][i]), outfile);
                putc(GETJSAMPLE(colormap[0][i]), outfile);
                putc(GETJSAMPLE(colormap[0][i]), outfile);

                if (map_entry_size == 4)
                {
                    putc(0, outfile);
                }
            }
        }
    }
    else
    {
        // No map at all means grayscale data, so a linear one is generated.
        for (i = 0; i < 256; i++)
        {
            putc(i, outfile);
            putc(i, outfile);
            putc(i, outfile);

            if (map_entry_size == 4)
            {
                putc(0, outfile);
            }
        }
    }

    if (i > map_colors)
    {
        ERREXIT1(cinfo, JERR_TOO_MANY_COLORS, i);
    }

    for (; i < map_colors; i++)
    {
        putc(0, outfile);
        putc(0, outfile);
        putc(0, outfile);

        if (map_entry_size == 4)
        {
            putc(0, outfile);
        }
    }
}
