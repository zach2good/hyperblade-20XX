/* jconfig.h for Microsoft Visual C++ 6.0 on Win32.
 * This file is not part of the IJG release. 6a ships no jconfig.vc; these are the settings of
 * 6b's jconfig.vc, which match the layout HYPERX.EXE was built with (a one-byte boolean).
 * See jconfig.doc in the IJG release for what each symbol means.
 */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

/* A one-byte boolean, as Windows defines it. */
#ifndef __RPCNDR_H__
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED
/* The original's integer IDCTs multiply a 16-bit value by a 32-bit constant (jdct.h). */
#define SHORTxLCONST_32
/* The original's memory manager caps a single allocation at 65520 bytes (jmemsys.h). */
#define MAX_ALLOC_CHUNK 65520L

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED
#define GIF_SUPPORTED
#define PPM_SUPPORTED
#undef RLE_SUPPORTED
#define TARGA_SUPPORTED

#define TWO_FILE_COMMANDLINE
#define USE_SETMODE
/* The game installs cdjpeg.c's signal catcher, which exists only under this symbol. */
#define NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT

#endif /* JPEG_CJPEG_DJPEG */
