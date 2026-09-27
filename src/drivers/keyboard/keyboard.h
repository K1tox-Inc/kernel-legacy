#pragma once

#include <arch/acpi.h>
#include <arch/io.h>
#include <arch/trap_frame.h>
#include <arch/x86.h>
#include <kernel/panic.h>
#include <libk.h>
#include <tty/tty.h>
#include <types.h>

#define UNDEFINED           0
#define STOP_WHEN_UNDEFINED 0
#define KEY_MAX             0xff

#define UNDEFINED_KEY                                                                              \
	((struct keyboard_key){.value      = UNDEFINED,                                                \
	                       .alt_value  = UNDEFINED,                                                \
	                       .keycode    = UNDEFINED,                                                \
	                       .category   = UNDEFINED,                                                \
	                       .undergroup = UNDEFINED,                                                \
	                       .state_ptr  = NULL})

#define UNDEFINED_ROUTINE ((struct scancode_routine){.key = UNDEFINED_KEY, .handler = NULL})

enum layout { QWERTY = 0, AZERTY };

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

typedef void (*group_init_funs_t)(void);
typedef void (*key_handler_t)(struct keyboard_key);

struct scancode_routine {
	struct keyboard_key key;
	key_handler_t       handler;
};

extern struct scancode_routine current_layout[256];

void keyboard_bind_key(key_handler_t handler, struct keyboard_key key);
void keyboard_unbind_key(uint8_t keycode);
void keyboard_handle(struct trap_frame *frame);
void keyboard_init(void);
void keyboard_remap_layout(struct keyboard_key *table, uint32_t size);
void keyboard_switch_layout(enum layout new_layout);
