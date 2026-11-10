// SDL 3.4's wasm64 heap index divides by 4n, but Emscripten 4 returns malloc's pointer as a Number.
// SDL keeps any helper already defined.
Module['SDL3'] = {
    CPtrToHeap32Index: (ptr) => Number(ptr) / 4,
};

// The game sizes its canvas for a desktop window. Scale it to fit the page; SDL maps the mouse
// through the canvas's on-screen size, so clicks still land.
if (typeof document !== 'undefined') {
    const style = document.createElement('style');
    style.textContent = '#canvas { width: auto !important; height: auto !important; max-width: 100vw; max-height: 100vh; }';
    document.head.append(style);
}

// Emscripten 4 calls SDL's page-close handler as returning an int, but it returns a pointer under
// wasm64 and throws. Registered first, this stops the event before SDL sees it; saves need none.
if (typeof window !== 'undefined') {
    window.addEventListener('beforeunload', (event) => event.stopImmediatePropagation(), true);
}
