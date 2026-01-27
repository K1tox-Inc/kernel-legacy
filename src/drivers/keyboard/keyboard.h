#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <arch/acpi.h>
#include <arch/io.h>
#include <arch/x86.h>
#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <kernel/panic.h>
#include <libk.h>
#include <types.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines
#define UNDEFINED           0
#define STOP_WHEN_UNDEFINED 0
#define KEY_MAX             0xff

// Macros
#define UNDEFINED_KEY                                                                              \
	((keyboard_key_t){.value      = UNDEFINED,                                                     \
	                  .alt_value  = UNDEFINED,                                                     \
	                  .keycode    = UNDEFINED,                                                     \
	                  .category   = UNDEFINED,                                                     \
	                  .undergroup = UNDEFINED,                                                     \
	                  .state_ptr  = NULL})

#define UNDEFINED_ROUTINE ((scancode_routine_t){.key = UNDEFINED_KEY, .handler = NULL})

// ============================================================================
// STRUCT
// ============================================================================

// Enums

// Structures
typedef void (*group_init_funs_t)(void);
typedef void (*key_handler_t)(keyboard_key_t);

typedef struct scancode_routine {
	keyboard_key_t key;
	key_handler_t  handler;
} scancode_routine_t;

// Typedefs

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

extern scancode_routine_t current_layout[256];

// ============================================================================
// EXTERNAL APIs
// ============================================================================
