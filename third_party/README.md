# Third party source

## stb

`stb/` holds two of Sean Barrett's single-file libraries, unchanged:

- `stb_image.h` v2.30 decodes the JPEG fallback in `GDI::LoadJPEGImage`.
- `stb_image_write.h` v1.16 writes the bring-up snapshots.

`src/graphics/stbimpl.cpp` compiles both implementations, with the image loader limited to JPEG.

| | |
|---|---|
| Origin | https://github.com/nothings/stb |
| Commit | `2c980bb59875b0d32144a71867fbdebb2f77cd20` (2026-08-01) |
| SHA-256 `stb_image.h` | `1f8c1b6b408f26e3b20cbfbbd4758afb3dc9b837ff1e17c258928f406148a87c` |
| SHA-256 `stb_image_write.h` | `17f339e7582ff4d4d56e40b0e87d77eda8d8f8e62806278ddf8fcf0b8b3d9a84` |

Both are public domain, or MIT at your choice. `stb/LICENSE` carries the terms.
