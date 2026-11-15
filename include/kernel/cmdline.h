#pragma once

#include <types.h>

#define CMDLINE_MAX      512
#define CMDLINE_MAX_ARGS 32

/* Store the boot command line. `n` may include the trailing NUL or not. */
void cmdline_set(const char *s, size_t n);

/* The command line as the bootloader gave it. */
const char *cmdline_raw(void);

/* Value of the first `key=value` token, or NULL. `root=UUID=x` gives "UUID=x". */
const char *cmdline_get(const char *key);

/* True if a bare `flag` token (no `=`) is present, e.g. `ro`. */
bool cmdline_has(const char *flag);
