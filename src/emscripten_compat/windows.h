#ifndef EMSCRIPTEN_WINDOWS_H
#define EMSCRIPTEN_WINDOWS_H

#include <stdint.h>
#include <emscripten.h>

/* Minimal Windows API stubs so legacy code compiles under Emscripten. */
typedef void* HANDLE;
typedef uint32_t DWORD;
typedef int BOOL;

#ifndef STD_OUTPUT_HANDLE
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#endif

static inline HANDLE GetStdHandle(DWORD nStdHandle) {
  (void)nStdHandle;
  return (HANDLE)0;
}

static inline BOOL GetConsoleMode(HANDLE hConsoleHandle, DWORD* lpMode) {
  (void)hConsoleHandle;
  if (lpMode) {
    *lpMode = 0;
  }
  return 1;
}

static inline BOOL SetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode) {
  (void)hConsoleHandle;
  (void)dwMode;
  return 1;
}

static inline void Sleep(unsigned int dwMilliseconds) {
  emscripten_sleep((int)dwMilliseconds);
}

#endif
