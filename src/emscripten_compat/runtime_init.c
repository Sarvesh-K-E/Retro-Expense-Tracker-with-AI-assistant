#include <stdio.h>

/*
 * Keep stdout/stderr unbuffered so animation output appears progressively
 * in the browser terminal, similar to a native console.
 */
__attribute__((constructor))
static void emscripten_stdio_init(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}
