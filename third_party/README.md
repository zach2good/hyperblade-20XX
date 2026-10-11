# Third party source

## jpeg-6a

HYPERX.EXE embeds the Independent JPEG Group's library, compiled `/O1` while the rest of the
game is `/O2`. It is version 6a, not the more widely mirrored 6b: the virtual array manager
has no `max_alloc_chunk`, `my_post_controller` orders `whole_image` before `buffer`, and
`IFAST_MULT_TYPE` is `INT32`. All three read out of the image and all three are stock 6a.

`jpeg-6a/` holds the release files the game uses: the decompression modules, the common files,
djpeg's `cdjpeg.c` for its signal catcher, the headers they include, and `README`, which carries
the IJG licence. `CMakeLists.txt` builds them into the static library `jpeg6a` and links it. None
of these files is changed.

`jpeg-6a/jconfig.h` is not an IJG file. 6a ships no configuration for Visual C++, so it holds the
settings of 6b's `jconfig.vc` and two the image shows the original used: `SHORTxLCONST_32` for
the integer inverse DCTs, and a `MAX_ALLOC_CHUNK` of 65520.

The game's BMP writer, `src/engine/bmpwriter.cpp`, is djpeg's `wrbmp.c` as the game changed it:
it checks the output file before each write. It is game code and lives under `src/`, but it is
built into the same library, because the original built it there and its code sits among the
library's.

`Copyright (C) 1996, Thomas G. Lane` and `Wrong JPEG library version` are the only library
banners in HYPERX.EXE: there is no libpng, no zlib, and no audio or video middleware.
Everything else that looks external is either a system import (DirectDraw, Direct3D, Win32) or
the C runtime, which this tree links. Ghidra's `PNG__` and `BMP__` prefixes across 0x494000 are
wrong; every one of those functions is IJG code.

| | |
|---|---|
| Origin | https://www.ijg.org/files/jpegsrc.v6a.tar.gz |
| Retrieved | 2026-08-30 |
| SHA-256 | `0e963fdd29d5049f61f3dc137bdc9437473a0ad505dbcfc8925f0fcad46c5929` |

To check that the files are stock, fetch the origin, check it against the hash above, unpack it,
and compare each file here except `jconfig.h` with its namesake.

The IJG licence permits use, modification and redistribution, including commercially, and asks
for an acknowledgement in the documentation of anything that ships it. `jpeg-6a/README` carries
the full terms.
