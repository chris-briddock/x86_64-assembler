# AGENTS.md — Senior C Developer Guidelines

This file instructs AI agents on how to write and structure C programs in the style of a senior C developer. All generated code must adhere to these conventions unless the user explicitly overrides them.

---

## Philosophy

- **Clarity over cleverness.** Write code that a tired colleague can understand at 2 AM. If a trick requires a comment to explain it, question whether the trick is worth it.
- **Explicit over implicit.** Never rely on implicit behaviour — zero-initialisation, promotion rules, evaluation order. Make intent visible in code.
- **Own your memory.** Every allocation has a matching free. Every resource has a clear owner. Ownership is documented, not assumed.
- **Fail loudly, fail early.** Validate inputs at boundaries. Return errors up the call stack. Never silently swallow failures.
- **Portable by default.** Write to the C standard (C99 or C11 unless constrained). Avoid compiler extensions unless they are gated behind a feature macro.

---

## Project Structure

```
project/
├── include/          # Public headers (the API surface)
│   └── project/
│       └── module.h
├── src/              # Implementation files
│   └── module.c
├── tests/            # Unit and integration tests
│   └── test_module.c
├── docs/             # Documentation
├── Makefile          # Or CMakeLists.txt
└── README.md
```

- **Public headers** live under `include/project/` and are included as `#include "project/module.h"`. This namespaces includes and avoids collisions.
- **Private headers** (implementation details not part of the API) live alongside their `.c` files in `src/`.
- One `.c` file per logical module. One public header per module. Do not bundle unrelated functionality.
- The `main()` function lives in its own translation unit (e.g. `src/main.c`) and does as little as possible — parse args, call into modules, return.

---

## Header Files

```c
/* include/project/buffer.h */

#ifndef PROJECT_BUFFER_H
#define PROJECT_BUFFER_H

#include <stddef.h>   /* size_t */
#include <stdint.h>   /* uint8_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a growable byte buffer.
 *
 * Callers must not access members directly.
 * Acquire via buffer_create(), release via buffer_destroy().
 */
typedef struct Buffer Buffer;

/**
 * @brief Allocate and initialise a new buffer with the given initial capacity.
 *
 * @param initial_capacity  Hint for pre-allocated bytes (may be 0).
 * @return Pointer to a new Buffer, or NULL on allocation failure.
 */
Buffer *buffer_create(size_t initial_capacity);

/**
 * @brief Free all resources held by buf.
 *
 * @param buf  May be NULL (no-op).
 */
void buffer_destroy(Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* PROJECT_BUFFER_H */
```

Rules:
- Every header has an **include guard** using the pattern `PROJECT_MODULE_H`. Never use `#pragma once` for portable code (it is not in the standard).
- `#include` only what the header itself needs. Move includes that are only needed in the `.c` file into the `.c` file.
- Forward-declare structs in headers and define them in `.c` files to enforce opacity.
- Always wrap headers in `extern "C"` guards so they can be consumed from C++.
- Include `<stddef.h>` and `<stdint.h>` explicitly — never rely on transitive inclusion.

---

## Source Files

```c
/* src/buffer.c */

#include "project/buffer.h"   /* own public header first */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Private constants */
#define BUFFER_GROWTH_FACTOR 2
#define BUFFER_MIN_CAPACITY  16

/* Private struct definition (hidden from callers) */
struct Buffer {
    uint8_t *data;
    size_t   len;
    size_t   cap;
};

/* Forward declarations of private (static) functions */
static int buffer_grow(Buffer *buf, size_t min_cap);

/* --- Public API --- */

Buffer *
buffer_create(size_t initial_capacity)
{
    Buffer *buf = calloc(1, sizeof(*buf));
    if (!buf) {
        return NULL;
    }

    size_t cap = initial_capacity > BUFFER_MIN_CAPACITY
                     ? initial_capacity
                     : BUFFER_MIN_CAPACITY;

    buf->data = malloc(cap);
    if (!buf->data) {
        free(buf);
        return NULL;
    }

    buf->cap = cap;
    return buf;
}

void
buffer_destroy(Buffer *buf)
{
    if (!buf) {
        return;
    }
    free(buf->data);
    free(buf);
}

/* --- Private functions --- */

static int
buffer_grow(Buffer *buf, size_t min_cap)
{
    assert(buf != NULL);

    size_t new_cap = buf->cap * BUFFER_GROWTH_FACTOR;
    if (new_cap < min_cap) {
        new_cap = min_cap;
    }

    uint8_t *new_data = realloc(buf->data, new_cap);
    if (!new_data) {
        return -1;   /* caller preserves original buffer */
    }

    buf->data = new_data;
    buf->cap  = new_cap;
    return 0;
}
```

Rules:
- **Include your own public header first**, then system/stdlib headers, then third-party headers. This catches missing includes in your own header.
- Mark all file-local functions `static`. If a function is not part of the public API, it must not be visible at link time.
- Define private structs and constants in the `.c` file, not the header.
- Use `calloc` for structs that should be zero-initialised. Use `malloc` when you will immediately overwrite.
- On failure paths: free everything allocated so far before returning NULL or an error code. Never leak on the error path.

---

## Naming Conventions

| Entity | Convention | Example |
|---|---|---|
| Types (`typedef struct`) | `PascalCase` | `Buffer`, `HttpRequest` |
| Functions | `module_verb_noun` | `buffer_append_bytes` |
| Macros / constants | `SCREAMING_SNAKE_CASE` | `MAX_RETRIES`, `BUFFER_MIN_CAPACITY` |
| Local variables | `snake_case` | `byte_count`, `is_valid` |
| Private functions | `static` + `snake_case` | `static int buffer_grow(...)` |
| Global variables | Avoid. If needed, prefix with `g_` | `g_log_level` |

- Prefix all public symbols (functions, typedefs, macros) with the module name: `buffer_`, `http_`, `arena_`. This is your namespace.
- Boolean-ish variables use `is_`, `has_`, `can_` prefixes: `is_done`, `has_error`.
- Avoid abbreviations unless they are universally understood (`len`, `buf`, `ptr`, `idx`, `err`).

---

## Error Handling

Senior C treats errors as first-class citizens. There is no single right pattern — choose one per project and apply it consistently.

### Pattern 1 — Return code + out-parameter (preferred for most APIs)

```c
/**
 * @return 0 on success, negative errno value on failure.
 */
int
file_read_all(const char *path, uint8_t **out_data, size_t *out_len)
{
    assert(path    != NULL);
    assert(out_data != NULL);
    assert(out_len  != NULL);

    *out_data = NULL;
    *out_len  = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return -errno;
    }

    /* ... read logic ... */

    fclose(fp);
    return 0;
}
```

### Pattern 2 — NULL sentinel (for allocating constructors)

```c
Buffer *buf = buffer_create(1024);
if (!buf) {
    /* handle allocation failure */
}
```

Rules:
- **Check every return value** — `malloc`, `fopen`, `fread`, `snprintf`, all of them.
- Never `assert()` in place of error handling in production paths. `assert` is for programmer errors (invariants), not runtime errors (bad input, OOM, I/O).
- Propagate errors up; do not print error messages deep in a library. Let the caller decide how to present them.
- Use `goto cleanup` for functions that acquire multiple resources — it keeps a single, linear cleanup path and avoids duplicating free calls.

```c
int
process(const char *path)
{
    int     result = -1;
    FILE   *fp     = NULL;
    char   *buf    = NULL;

    fp = fopen(path, "r");
    if (!fp) { goto cleanup; }

    buf = malloc(BUF_SIZE);
    if (!buf) { goto cleanup; }

    /* ... work ... */
    result = 0;

cleanup:
    free(buf);
    if (fp) { fclose(fp); }
    return result;
}
```

---

## Memory Management

- **Every `malloc` has exactly one `free`.** The owner is documented in the header comment.
- Nullify pointers after freeing them in long-lived contexts: `ptr = NULL;` — this turns use-after-free into a deterministic crash.
- Prefer **arena/pool allocators** for performance-sensitive code with many small allocations of known lifetimes.
- Use `sizeof(*ptr)` instead of `sizeof(TypeName)` — it stays correct if the type changes.
  ```c
  Buffer *buf = malloc(sizeof(*buf));   /* correct */
  Buffer *buf = malloc(sizeof(Buffer)); /* fragile */
  ```
- Cast the result of `malloc` only when compiling as C++ (never required in C99+).
- For string duplication, always use `strndup` or a safe wrapper — never `strcpy` into a fixed buffer without size validation.

---

## Types and Portability

- Use `<stdint.h>` types when width matters: `uint8_t`, `int32_t`, `uint64_t`.
- Use `size_t` for sizes and counts. Use `ptrdiff_t` for pointer differences.
- Never use `int` for sizes or array indices in library code — size overflow is a bug class.
- Do not use `char` for binary data — use `uint8_t`. Reserve `char` for text.
- Do not compare signed and unsigned integers. Enable and heed `-Wsign-compare`.
- Avoid `long` and `short` in interfaces — their width is platform-dependent.

---

## Compiler Flags (Baseline)

Always compile with at minimum:

```makefile
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
         -Wsign-compare -Wmissing-prototypes -Wstrict-prototypes \
         -fstack-protector-strong
```

During development, add:

```makefile
CFLAGS += -g -fsanitize=address,undefined
```

For release:

```makefile
CFLAGS += -O2 -DNDEBUG
```

Rules:
- Treat warnings as errors in CI: `-Werror`.
- Never suppress a warning with a pragma or cast without a comment explaining why.
- Regularly run under **Valgrind** and/or **AddressSanitizer**. A clean sanitiser run is a minimum bar before merging.

---

## Functions

- A function does **one thing**. If you cannot describe it in one sentence without "and", split it.
- Functions should fit on one screen (~50 lines). Beyond that, extract helpers.
- Every parameter is either `const` (input) or a pointer (output) — make the direction of data flow visible in the signature.
- Use `const` aggressively on pointer parameters that are not modified:
  ```c
  size_t buffer_len(const Buffer *buf);
  ```
- Avoid output parameters that can be NULL unless NULLability is explicitly documented and checked.
- Place the return type on its own line for public functions — it aids grep and readability:
  ```c
  int
  buffer_append(Buffer *buf, const uint8_t *data, size_t len)
  {
      /* ... */
  }
  ```

---

## Comments and Documentation

- Use `/** ... */` Doxygen-style comments on every public function and type in the header.
- In `.c` files, use `/* ... */` for block comments and `/* inline comment */` for inline. Do not use `//` in headers (avoid if targeting C89/C90 compatibility).
- Comment the **why**, not the what. The code says what; comments say why.
  ```c
  /* Grow by 2x to amortise reallocation cost over time. */
  new_cap = buf->cap * 2;
  ```
- Mark non-obvious platform/standard assumptions with a comment and a reference if possible.
- Use `TODO(name):` and `FIXME:` tags consistently so they are greppable.

---

## Macros

- Prefer `static inline` functions over function-like macros wherever possible — they are type-safe and debuggable.
- If a macro is unavoidable, wrap the body in `do { ... } while (0)` and parenthesise every argument:

  ```c
  #define SWAP(a, b, T) do { T _tmp = (a); (a) = (b); (b) = _tmp; } while (0)
  ```

- Never use a macro argument more than once — it may be evaluated multiple times.
- Constant macros should be `static const` or `enum` values where possible (they have type and scope).

---

## Testing

- Every module has a corresponding test file in `tests/`.
- Tests are standalone executables that return `0` on pass, non-zero on failure.
- Test the **public API only** — never `#include` a `.c` file to test private functions. If a private function needs testing, it is probably a candidate to become its own module.
- Use `assert()` liberally in tests — failure should be loud and obvious.
- Structure test functions as:
  ```c
  static void
  test_buffer_append_grows_when_full(void)
  {
      Buffer *buf = buffer_create(1);
      assert(buf != NULL);
      /* ... exercise ... */
      /* ... assert post-conditions ... */
      buffer_destroy(buf);
  }
  ```
- Run the full test suite under AddressSanitizer on every CI pass.

---

## Things Senior C Developers Never Do

- Never use `gets()`. It is removed from C11 for good reason.
- Never use `sprintf()` without a size bound — use `snprintf()`.
- Never use `strcpy()` or `strcat()` — use `strncpy()` / `strncat()` or `strlcpy()` / `strlcat()` where available, and always verify the result fits.
- Never ignore the return value of `snprintf` — check that it did not truncate.
- Never cast between function pointer types and data pointer types — it is undefined behaviour.
- Never assume `sizeof(int) == 4`. Never assume pointer size. Use `stdint.h`.
- Never use `realloc(ptr, 0)` as a free — it is implementation-defined.
- Never use global mutable state in library code. It makes the library non-reentrant and untestable.
- Never write `if (flag == TRUE)` — write `if (flag)`. Never write `if (ptr == NULL)` — write `if (!ptr)`.
- Never leave dead code commented out. Use version control. Delete it.