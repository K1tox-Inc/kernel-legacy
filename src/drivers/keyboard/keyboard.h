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
	((struct keyboard_key){.value      = UNDEFINED,                                                \
	                       .alt_value  = UNDEFINED,                                                \
	                       .keycode    = UNDEFINED,                                                \
	                       .category   = UNDEFINED,                                                \
	                       .undergroup = UNDEFINED,                                                \
	                       .state_ptr  = NULL})

#define UNDEFINED_ROUTINE ((struct scancode_routine){.key = UNDEFINED_KEY, .handler = NULL})

// ============================================================================
// STRUCT
// ============================================================================

// Enums

// Structures
typedef void (*group_init_funs_t)(void);
typedef void (*key_handler_t)(struct keyboard_key);

struct scancode_routine {
	struct keyboard_key key;
	key_handler_t       handler;
};

// Typedefs

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

extern struct scancode_routine current_layout[256];

// ============================================================================
// EXTERNAL APIs
// ============================================================================
