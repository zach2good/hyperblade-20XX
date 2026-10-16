# Third party source

## stb

`stb/` holds `stb_image.h` v2.30, one of Sean Barrett's single-file libraries, unchanged. It decodes
the JPEG fallback in `GDI::LoadJPEGImage`. `src/graphics/stbimpl.cpp` compiles it, limited to JPEG.

| | |
|---|---|
| Origin | https://github.com/nothings/stb |
| Commit | `2c980bb59875b0d32144a71867fbdebb2f77cd20` (2026-08-01) |
| SHA-256 `stb_image.h` | `1f8c1b6b408f26e3b20cbfbbd4758afb3dc9b837ff1e17c258928f406148a87c` |

It is public domain, or MIT at your choice. `stb/LICENSE` carries the terms.
