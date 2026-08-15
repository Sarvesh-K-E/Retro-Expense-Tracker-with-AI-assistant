#ifndef EMSCRIPTEN_CONIO_H
#define EMSCRIPTEN_CONIO_H

#include <stdio.h>
#include <emscripten.h>

/*
 * Input bridge for browser terminal mode.
 * JS pushes chars into a queue; C pulls them with async yielding.
 */
EM_JS(int, wasm_pop_char, (), {
  if (typeof window !== 'undefined' && typeof window.__wasmPopChar === 'function') {
    return window.__wasmPopChar();
  }
  return -1;
});

EM_JS(int, wasm_has_char, (), {
  if (typeof window !== 'undefined' && typeof window.__wasmHasChar === 'function') {
    return window.__wasmHasChar() ? 1 : 0;
  }
  return 0;
});

EM_JS(void, wasm_set_line_mode, (int mode), {
  if (typeof window !== 'undefined' && typeof window.__wasmSetLineMode === 'function') {
    window.__wasmSetLineMode(mode ? true : false);
  }
});

EM_JS(void, wasm_set_waiting_input, (int waiting), {
  if (typeof window !== 'undefined' && typeof window.__wasmSetWaitingInput === 'function') {
    window.__wasmSetWaitingInput(waiting ? true : false);
  }
});

static inline int emscripten_read_char_blocking(int show_wait_cursor) {
  if (show_wait_cursor) {
    wasm_set_waiting_input(1);
  }
  int c = -1;
  while (c < 0) {
    c = wasm_pop_char();
    if (c < 0) {
      emscripten_sleep(16);
    }
  }
  if (show_wait_cursor) {
    wasm_set_waiting_input(0);
  }
  return c;
}

static inline int getch(void) {
  wasm_set_line_mode(0);
  wasm_set_waiting_input(0);
  return emscripten_read_char_blocking(0);
}

static inline int _getch(void) {
  return getch();
}

static inline int kbhit(void) {
  return wasm_has_char();
}

/*
 * Replace fgets() calls in this translation unit so line input also comes
 * from the same in-page terminal queue instead of browser prompt/stdin.
 */
static inline char* emscripten_fgets(char* s, int size, FILE* stream) {
  (void)stream;
  if (!s || size <= 0) {
    return NULL;
  }

  wasm_set_line_mode(1);
  int i = 0;
  while (i < size - 1) {
    int c = emscripten_read_char_blocking(1);
    if (c == '\r' || c == '\n') {
      s[i++] = '\n';
      break;
    }
    if (c == 8 || c == 127) {
      if (i > 0) i--;
      continue;
    }
    s[i++] = (char)c;
  }

  s[i] = '\0';
  wasm_set_line_mode(0);
  return i > 0 ? s : NULL;
}

#define fgets emscripten_fgets

#endif
