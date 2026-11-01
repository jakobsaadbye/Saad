//////////////////////////////////////////////////////////////////////////////////////////
// Linux test harness: runs Win64-ABI code produced by the Saad backend on Linux.
//
// The program is written as an ELF object by 'saad build --elf' (or build/out.asm is assembled with
// 'nasm -felf64 --prefix ms_'), with every global/extern symbol prefixed with "ms_". This file provides those ms_ symbols as
// __attribute__((ms_abi)) wrappers around the real C functions, so gcc takes care of the
// Win64 calling convention at the boundary (shadow space, register usage, varargs).
//
// If a program uses a C function that is not wrapped here, linking fails with an undefined
// reference to ms_<name>. Add a wrapper below.
//////////////////////////////////////////////////////////////////////////////////////////
#define MS __attribute__((ms_abi))
#include <stdarg.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ---- printf family: walk the format and fetch each argument from the ms va_list ----
static int ms_vformat(char *out, size_t out_cap, FILE *f, const char *fmt, __builtin_ms_va_list ap) {
    char buf[4096]; size_t pos = 0; int total = 0;
    #define EMIT(s, n) do { if (f) fwrite(s, 1, n, f); else if (out) { size_t k = (n); if (pos + k >= out_cap) k = out_cap - pos - 1; memcpy(out + pos, s, k); pos += k; } total += (n); } while (0)
    for (const char *p = fmt; *p; ) {
        if (*p != '%') { const char *q = p; while (*q && *q != '%') q++; EMIT(p, (size_t)(q - p)); p = q; continue; }
        if (p[1] == '%') { EMIT("%", 1); p += 2; continue; }
        char spec[64]; int n = 0; spec[n++] = *p++;
        while (*p && strchr("-+ #0123456789.*", *p) && n < 40) {
            if (*p == '*') { int w = (int) __builtin_va_arg(ap, long long); n += snprintf(spec + n, 12, "%d", w); p++; }
            else spec[n++] = *p++;
        }
        while (*p && strchr("hlLqjzt", *p)) p++;          // drop length modifiers, we re-add our own
        char conv = *p ? *p++ : 0; int len;
        if (strchr("fFeEgGaA", conv)) { spec[n++] = conv; spec[n] = 0; double d = __builtin_va_arg(ap, double); len = snprintf(buf, sizeof buf, spec, d); }
        else if (strchr("diuxXo", conv)) { spec[n++] = 'l'; spec[n++] = 'l'; spec[n++] = conv; spec[n] = 0; long long v = __builtin_va_arg(ap, long long); len = snprintf(buf, sizeof buf, spec, v); }
        else if (conv == 'c') { spec[n++] = 'c'; spec[n] = 0; long long v = __builtin_va_arg(ap, long long); len = snprintf(buf, sizeof buf, spec, (int)v); }
        else if (conv == 's') { spec[n++] = 's'; spec[n] = 0; char *s = __builtin_va_arg(ap, char *); len = snprintf(buf, sizeof buf, spec, s); }
        else if (conv == 'p') { void *v = __builtin_va_arg(ap, void *); len = snprintf(buf, sizeof buf, "%016llX", (unsigned long long)(uintptr_t) v); } // Like msvcrt
        else { len = 0; }
        EMIT(buf, (size_t)len);
    }
    if (out) out[pos] = 0;
    return total;
}
MS int ms_printf(const char *fmt, ...) { __builtin_ms_va_list ap; __builtin_ms_va_start(ap, fmt); int r = ms_vformat(NULL, 0, stdout, fmt, ap); __builtin_ms_va_end(ap); fflush(stdout); return r; }
MS int ms_sprintf(char *out, const char *fmt, ...) { __builtin_ms_va_list ap; __builtin_ms_va_start(ap, fmt); int r = ms_vformat(out, 1 << 30, NULL, fmt, ap); __builtin_ms_va_end(ap); return r; }

// ---- libc ----
MS void *ms_malloc(int64_t n) { return malloc(n); }
MS void *ms_calloc(int64_t a, int64_t b) { return calloc(a, b); }
MS void *ms_realloc(void *p, int64_t n) { return realloc(p, n); }
MS void  ms_free(void *p) { free(p); }
MS void *ms_memset(void *d, int v, int64_t n) { return memset(d, v, n); }
MS void *ms_memcpy(void *d, const void *s, int64_t n) { return memcpy(d, s, n); }
MS int64_t ms_strlen(const char *s) { return (int64_t) strlen(s); }
MS void  ms_exit(int c) { fflush(stdout); exit(c); }
MS void  ms_ExitProcess(unsigned c) { fflush(stdout); exit((int)c); }
MS int   ms_abs(int x) { return abs(x); }

// ---- stdio, stdlib and string.h used by std/c ----
MS int     ms_puts(const char *s) { int r = puts(s); fflush(stdout); return r; }
MS FILE   *ms_fopen(const char *p, const char *m) { return fopen(p, m); }
MS int     ms_fclose(FILE *f) { return fclose(f); }
MS int64_t ms_fread(void *b, int64_t s, int64_t c, FILE *f) { return (int64_t) fread(b, s, c, f); }
MS int64_t ms_fwrite(const void *b, int64_t s, int64_t c, FILE *f) { return (int64_t) fwrite(b, s, c, f); }
MS int     ms_fflush(FILE *f) { return fflush(f); }
MS int     ms_fseek(FILE *f, int32_t o, int w) { return fseek(f, o, w); }
MS int32_t ms_ftell(FILE *f) { return (int32_t) ftell(f); }
MS int     ms_remove(const char *p) { return remove(p); }
MS int     ms_rename(const char *a, const char *b) { return rename(a, b); }
MS char   *ms_getenv(const char *n) { return getenv(n); }
MS double  ms_strtod(const char *s, char **e) { return strtod(s, e); }
MS int     ms_system(const char *c) { fflush(stdout); return system(c); }
MS int     ms_memcmp(const void *a, const void *b, int64_t n) { return memcmp(a, b, n); }

// ---- kernel32 functions used by std/os/windows.sd and std/time.sd ----
#include <time.h>
#include <unistd.h>
MS void ms_Sleep(uint32_t ms) { usleep((useconds_t) ms * 1000); }
MS int  ms_QueryPerformanceCounter(int64_t *c) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); *c = (int64_t) t.tv_sec * 1000000000 + t.tv_nsec; return 1; }
MS int  ms_QueryPerformanceFrequency(int64_t *f) { *f = 1000000000; return 1; }
MS void ms_GetSystemTimeAsFileTime(uint64_t *ft) { struct timespec t; clock_gettime(CLOCK_REALTIME, &t); *ft = ((uint64_t) t.tv_sec + 11644473600ULL) * 10000000ULL + (uint64_t) t.tv_nsec / 100; }
#define F1(name) MS double ms_##name(double x) { return name(x); } MS float ms_##name##f(float x) { return name##f(x); }
F1(ceil) F1(floor) F1(fabs) F1(round) F1(trunc) F1(rint) F1(nearbyint)
F1(sin) F1(cos) F1(tan) F1(asin) F1(acos) F1(atan) F1(sinh) F1(cosh) F1(tanh) F1(sqrt) F1(exp) F1(log) F1(log10)
MS double ms_atan2(double y, double x) { return atan2(y, x); } MS float ms_atan2f(float y, float x) { return atan2f(y, x); }
MS double ms_pow(double x, double y) { return pow(x, y); }     MS float ms_powf(float x, float y) { return powf(x, y); }
MS long ms_lround(double x) { return lround(x); }
MS long long ms_llround(double x) { return llround(x); }

// Standard handles are the file descriptor + 1, so stdin isn't NULL
#include <sys/ioctl.h>
#include <sys/wait.h>
static int fd_of(void *h) { return (int)(intptr_t) h - 1; }
MS void *ms_GetStdHandle(uint32_t n) { int fd = n == (uint32_t)-10 ? 0 : n == (uint32_t)-11 ? 1 : 2; return (void *)(intptr_t)(fd + 1); }
MS int ms_ReadFile(void *h, void *b, uint32_t n, uint32_t *got, void *o) { ssize_t r = read(fd_of(h), b, n); *got = r > 0 ? (uint32_t) r : 0; return r >= 0; }
MS int ms_WriteFile(void *h, const void *b, uint32_t n, uint32_t *put, void *o) { fflush(stdout); ssize_t r = write(fd_of(h), b, n); *put = r > 0 ? (uint32_t) r : 0; return r >= 0; }
MS int ms_PeekNamedPipe(void *h, void *b, uint32_t size, uint32_t *got, uint32_t *avail, uint32_t *left) {
    int n = 0; if (ioctl(fd_of(h), FIONREAD, &n) < 0) return 0; if (avail) *avail = (uint32_t) n; return 1;
}
// cmd.exe /c removes the outer quotes of a command that starts with a quote, which std/os does on purpose
MS FILE *ms__popen(const char *command, const char *mode) {
    size_t n = strlen(command); char *c = strdup(command);
    if (n >= 2 && c[0] == '"' && c[n - 1] == '"') { memmove(c, c + 1, n - 2); c[n - 2] = 0; }
    fflush(stdout); FILE *f = popen(c, mode[0] == 'w' ? "w" : "r"); free(c); return f;
}
MS int ms__pclose(FILE *f) { int s = pclose(f); return WIFEXITED(s) ? WEXITSTATUS(s) : -1; }
static int saved_argc; static char **saved_argv;
extern char **environ;
MS int ms___getmainargs(int *argc, char ***argv, char ***env, int wild, void *info) { *argc = saved_argc; *argv = saved_argv; *env = environ; return 0; }

// ---- entry ----
MS int ms_main(void);
int main(int argc, char **argv) { saved_argc = argc; saved_argv = argv; int r = ms_main(); fflush(stdout); return r; }
