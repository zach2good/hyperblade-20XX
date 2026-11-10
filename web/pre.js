// SDL 3.4's wasm64 heap index divides by 4n, but Emscripten 4 returns malloc's pointer as a Number.
// SDL keeps any helper already defined.
Module['SDL3'] = {
    CPtrToHeap32Index: (ptr) => Number(ptr) / 4,
};
