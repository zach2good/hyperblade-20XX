# Notes for AI tools

This repository is a modern port of the HyperBlade decompilation. Nothing here has to match the
original's bytes.

- Build with `cmake --build build` from an x86 developer environment, and keep it free of errors.
- Run `clang-format -i` (version 22) on every source file you touch.
- Write C++23. The decompiled source is C++98, so modernise the code you touch, and leave the rest.
- Keep `src/` changes small enough to merge with the decomp's `upstream/main`.
- Commit nothing from the original: no binary, no game data, no disassembly.
- Never add `Co-Authored-By` or any trailer that credits an AI tool.
