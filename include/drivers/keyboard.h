#pragma once

// ============================================================================
// INCLUDES
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

enum layout { QWERTY = 0, AZERTY };

// Structures

// Typedefs

enum key_category { KEY_ALPHANUMERIC = 0, KEY_CONTROL, KEY_NAVIGATION, KEY_FUNCTION, KEY_SPECIAL };

enum key_undergroup {
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
};

struct keyboard_key {
	uint16_t            value;
	uint16_t            alt_value;
	uint16_t            keycode;
	enum key_category   category;
	enum key_undergroup undergroup;
	bool               *state_ptr;
};

typedef void (*key_handler_t)(struct keyboard_key);

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

void keyboard_bind_key(key_handler_t handler, struct keyboard_key key);
void keyboard_unbind_key(uint8_t keycode);
void keyboard_handle(struct trap_frame *frame);
void keyboard_init(void);
void keyboard_remap_layout(struct keyboard_key *table, uint32_t size);
void keyboard_switch_layout(enum layout new_layout);
