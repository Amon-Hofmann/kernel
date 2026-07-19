# Session 08 — Extending `serial_printf`

## Goal

The UART driver and the basic `serial_printf` were built in session 3.1. Two
format specifiers that matter for kernel debugging are still missing: `%d`
(signed decimal integer) and `%c` (single character). In this session you will
add them, and in doing so work through the variadic argument mechanism, signed
integer representation, and the GCC `format` attribute.

---

## Variadic functions and `stdarg.h`

`serial_printf` takes a variable number of arguments after `format`. C provides
access to those extra arguments through the `<stdarg.h>` macros.

```c
#include <stdarg.h>

void example(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);        /* initialise the list after the last named arg */

    int  n = va_arg(args, int); /* consume the next argument as int */
    char c = va_arg(args, int); /* char is promoted to int; read as int */

    va_end(args);               /* mandatory cleanup */
}
```

### What `va_arg` actually does

On i386, extra arguments are pushed onto the stack right-to-left by the caller
before the call instruction. `va_start` computes a pointer to the first
variadic argument by taking the address of the last named parameter and
advancing past it. `va_arg(args, T)` reads a `T`-sized value from that pointer
and advances the pointer by `sizeof(T)` (rounded up to the stack alignment).

One consequence: you must pass the correct type to `va_arg`. If the caller
pushed an `int` and you read it as `unsigned int`, the bit pattern is the same
and the result is well-defined. If you read it as `long long` (8 bytes) when
the caller pushed an `int` (4 bytes), you read past the argument into the
next one — undefined behaviour.

### Argument promotion

C promotes certain narrow types before passing them as variadic arguments:
- `char` → `int`
- `short` → `int`
- `float` → `double`

You must always read a `char` argument with `va_arg(args, int)`, never
`va_arg(args, char)`.

---

## Adding `%c`

`%c` is the simplest specifier: read one `int` (promoted from `char`) and
write it as a character.

```c
case 'c':
    serial_putchar((char)va_arg(args, int));
    break;
```

---

## Adding `%d`

`%d` prints a signed decimal integer. The existing `serial_write_32_u` handles
unsigned values, but passing a negative `int` to it directly would print a
large positive number — negative values have their sign bit set, which looks
like a huge `uint32_t`.

The correct approach:

```c
static void serial_write_32_d(int n) {
    if (n < 0) {
        serial_putchar('-');
        serial_write_32_u((uint32_t)(-(int64_t)n));
    } else {
        serial_write_32_u((uint32_t)n);
    }
}
```

Why the cast through `int64_t`: negating `INT32_MIN` (`-2147483648`) in 32-bit
arithmetic overflows — there is no positive 32-bit representation of
`+2147483648`. Casting to `int64_t` first gives room to negate it safely,
then the result fits in `uint32_t`.

---

## The `format(printf, m, n)` attribute

```c
void serial_printf(const char *format, ...)
    __attribute__((format(printf, 1, 2)));
```

This tells GCC to type-check the format string against the actual arguments,
exactly as it does for the standard `printf`. The numbers `1` and `2` are the
positions of the format string argument and the first variadic argument
(1-indexed).

If you write:

```c
serial_printf("%d", "hello");
```

GCC emits a warning: `format '%d' expects argument of type 'int', but argument
2 has type 'char *'`. With `-Werror` this becomes a compile error. This
attribute catches mistakes at compile time that would otherwise produce silent
garbage at runtime.

---

## What you need to implement

### Modify existing files

**`src/serial.c`** — add two things:

1. A `serial_write_32_d(int n)` helper (static, not declared in the
   header) that handles the sign and calls `serial_write_32_u`.

2. Two new cases in the `switch` inside `serial_printf`:
   - `'c'`: read `va_arg(args, int)`, cast to `char`, call `serial_putchar`
   - `'d'`: read `va_arg(args, int)` (or `va_arg(args, long)` when `long_` is
     set), call `serial_write_32_d`

No header changes are needed — `serial_printf` already accepts any format
string; the new specifiers just work.

### Build and test

```
make clean && make iso && make run
```

Add a test to `kernel_main` before the tick loop:

```c
serial_printf("signed: %d %d\n", 0, -42);
serial_printf("char:   %c%c%c\n", 'O', 'K', '\n');
```

Expected serial output:

```
signed: 0 -42
char:   OK
```

`INT32_MIN` (`-2147483648`) is worth testing separately but cannot be written
as a bare literal in a format call — the compiler parses `2147483648` as
`long` (it exceeds `INT_MAX`), which conflicts with `%d`'s expected `int` and
triggers a `-Wformat` error. Use `(int)(-2147483647 - 1)` if you want to
test that boundary.

---

## Concept check

Answer these before running `/review`:

1. What does `va_arg(args, int)` do at the machine level on i386? What would
   happen if you called `va_arg(args, long long)` when the caller pushed a
   plain `int`?

2. Why can't `%d` just cast the argument to `uint32_t` and call
   `serial_write_32_u` directly? Give the specific value that breaks.

3. The declaration has `__attribute__((format(printf, 1, 2)))`. What do the
   numbers `1` and `2` refer to, and what class of bug does this attribute
   catch that would otherwise go undetected until runtime?

---

## Mutation exercise

**Part A:** Change the `%d` case to read `va_arg(args, unsigned int)` instead
of `va_arg(args, int)` and pass the result directly to `serial_write_32_u`
(skipping the sign check). Call `serial_printf("%d\n", -1)` and observe what
is printed. Explain the output. Restore when done.

**Part B:** Remove `__attribute__((format(printf, 1, 2)))` from the
`serial_printf` declaration in `serial.h`. Write a call that passes a `%d`
specifier with a `char *` argument. Does GCC warn? What does this tell you
about when the attribute is doing its job? Restore when done.

When you are satisfied with the whole session, run `/review`.
