#pragma once

// ============================================================================
// IMCLUDES
// ============================================================================

#include <arch/trap_frame.h>
#include <types.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines

// Macros

// ============================================================================
// STRUCT
// ============================================================================

// Enums

typedef enum { QWERTY = 0, AZERTY } Layout;

// Structures

// Typedefs

typedef enum {
	KEY_ALPHANUMERIC = 0,
	KEY_CONTROL,
	KEY_NAVIGATION,
	KEY_FUNCTION,
	KEY_SPECIAL
} KeyCategory;

typedef enum {
	NONE = 0,
	LETTER,
	TOP_KEY,
	PUNCTUATION,
	SPACE,
	NUM_PAD,
	PRESS,
	RELEASE,
	TOGGLE,
	BACKSPACE,
	ENTER,
	ESCAPE
} KeyUndergroup;

typedef struct keyboard_key {
	uint16_t      value;
	uint16_t      alt_value;
	uint16_t      keycode;
	KeyCategory   category;
	KeyUndergroup undergroup;
	bool         *state_ptr;
} keyboard_key_t;

typedef void (*key_handler_t)(keyboard_key_t);

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

void keyboard_bind_key(key_handler_t handler, keyboard_key_t key);
void keyboard_unbind_key(uint8_t keycode);
void keyboard_handle(trap_frame_t *frame);
void keyboard_init(void);
void keyboard_remap_layout(keyboard_key_t *table, uint32_t size);
void keyboard_switch_layout(Layout new_layout);
