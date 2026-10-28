//////////////////////////////////////////////////////////////////////////////////////////
// Linux test harness: runs Win64-ABI code produced by the Saad backend on Linux.
//
// The program's asm (build/out.asm) is assembled as ELF with every global/extern symbol
// prefixed with "ms_" (nasm --prefix ms_). This file provides those ms_ symbols as
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
#include "runtime_support.c"   // compiled normally (SysV), wrapped below

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
        else if (conv == 'p') { spec[n++] = 'p'; spec[n] = 0; void *v = __builtin_va_arg(ap, void *); len = snprintf(buf, sizeof buf, spec, v); }
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
#define F1(name) MS double ms_##name(double x) { return name(x); } MS float ms_##name##f(float x) { return name##f(x); }
F1(ceil) F1(floor) F1(fabs) F1(round) F1(trunc) F1(rint) F1(nearbyint)
MS long ms_lround(double x) { return lround(x); }
MS long long ms_llround(double x) { return llround(x); }

// ---- runtime_support.c ----
MS void ms_runtime_builtin_assert(bool c, RawString f, uint64_t l) { fflush(stdout); runtime_builtin_assert(c, f, l); }
MS void ms_runtime_builtin_append(RawDynamicArray *da, void *item, bool by_value) { runtime_builtin_append(da, item, by_value); }
MS int  ms_runtime_compare_strings(RawString *a, RawString *b) { return runtime_compare_strings(a, b); }
MS TypePointer *ms_runtime_get_type_pointer(RawString n, Type *p) { return runtime_get_type_pointer(n, p); }
MS TypeArray *ms_runtime_get_type_array(RawString n, int k, Type *e, int64_t c) { return runtime_get_type_array(n, k, e, c); }
MS TypeEnum *ms_runtime_get_type_enum(RawString n, RawString *v, Type *b, int lo, int hi) { return runtime_get_type_enum(n, v, b, lo, hi); }
MS TypeStruct *ms_runtime_get_type_struct(RawString n, StructMember *m, int c, int s, int a) { return runtime_get_type_struct(n, m, c, s, a); }
MS TypeAny *ms_runtime_get_type_any(void) { return runtime_get_type_any(); }

// ---- data symbols: plain aliases ----
#define ALIAS(sym) __asm__(".globl ms_" #sym "\n.set ms_" #sym ", " #sym);
ALIAS(Type_uint) ALIAS(Type_u8) ALIAS(Type_u16) ALIAS(Type_u32) ALIAS(Type_u64) ALIAS(Type_int) ALIAS(Type_s8)
ALIAS(Type_s16) ALIAS(Type_s32) ALIAS(Type_s64) ALIAS(Type_float) ALIAS(Type_f32) ALIAS(Type_f64) ALIAS(Type_char)
ALIAS(Type_bool) ALIAS(Type_void) ALIAS(Type_untyped_int) ALIAS(Type_untyped_float) ALIAS(Type_string)

// ---- entry ----
MS int ms_main(void);
int main(void) { int r = ms_main(); fflush(stdout); return r; }
