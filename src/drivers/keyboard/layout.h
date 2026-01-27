#pragma once

#include "keyboard.h"

#define SCROLL_LOCK_KEY 0xC6

/* ---------------------------------------------------------------------------
 * Printable/Alphanumeric Key group
 */
static bool caps_lock   = false;
static bool left_shift  = false;
static bool right_shift = false;
static bool left_ctrl   = false;
static bool left_alt    = false;
static bool num_lock    = false;
static bool scroll_lock = false;

static keyboard_key_t default_key_table[256];

static keyboard_key_t printable_keys[] = {

    // letters
    {'a', 'A', 0x1E, KEY_ALPHANUMERIC, LETTER, NULL},
    {'b', 'B', 0x30, KEY_ALPHANUMERIC, LETTER, NULL},
    {'c', 'C', 0x2E, KEY_ALPHANUMERIC, LETTER, NULL},
    {'d', 'D', 0x20, KEY_ALPHANUMERIC, LETTER, NULL},
    {'e', 'E', 0x12, KEY_ALPHANUMERIC, LETTER, NULL},
    {'f', 'F', 0x21, KEY_ALPHANUMERIC, LETTER, NULL},
    {'g', 'G', 0x22, KEY_ALPHANUMERIC, LETTER, NULL},
    {'h', 'H', 0x23, KEY_ALPHANUMERIC, LETTER, NULL},
    {'i', 'I', 0x17, KEY_ALPHANUMERIC, LETTER, NULL},
    {'j', 'J', 0x24, KEY_ALPHANUMERIC, LETTER, NULL},
    {'k', 'K', 0x25, KEY_ALPHANUMERIC, LETTER, NULL},
    {'l', 'L', 0x26, KEY_ALPHANUMERIC, LETTER, NULL},
    {'m', 'M', 0x32, KEY_ALPHANUMERIC, LETTER, NULL},
    {'n', 'N', 0x31, KEY_ALPHANUMERIC, LETTER, NULL},
    {'o', 'O', 0x18, KEY_ALPHANUMERIC, LETTER, NULL},
    {'p', 'P', 0x19, KEY_ALPHANUMERIC, LETTER, NULL},
    {'q', 'Q', 0x10, KEY_ALPHANUMERIC, LETTER, NULL},
    {'r', 'R', 0x13, KEY_ALPHANUMERIC, LETTER, NULL},
    {'s', 'S', 0x1F, KEY_ALPHANUMERIC, LETTER, NULL},
    {'t', 'T', 0x14, KEY_ALPHANUMERIC, LETTER, NULL},
    {'u', 'U', 0x16, KEY_ALPHANUMERIC, LETTER, NULL},
    {'v', 'V', 0x2F, KEY_ALPHANUMERIC, LETTER, NULL},
    {'w', 'W', 0x11, KEY_ALPHANUMERIC, LETTER, NULL},
    {'x', 'X', 0x2D, KEY_ALPHANUMERIC, LETTER, NULL},
    {'y', 'Y', 0x15, KEY_ALPHANUMERIC, LETTER, NULL},
    {'z', 'Z', 0x2C, KEY_ALPHANUMERIC, LETTER, NULL},
    // top keys
    {'1', '!', 0x02, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'2', '@', 0x03, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'3', '#', 0x04, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'4', '$', 0x05, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'5', '%', 0x06, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'6', '^', 0x07, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'7', '&', 0x08, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'8', '*', 0x09, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'9', '(', 0x0A, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'0', ')', 0x0B, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    // specials
    {'-', '_', 0x0C, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'=', '+', 0x0D, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'[', '{', 0x1A, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {']', '}', 0x1B, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {';', ':', 0x27, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'\'', '"', 0x28, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'`', '~', 0x29, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'\\', '|', 0x2B, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {',', '<', 0x33, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'.', '>', 0x34, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'/', '?', 0x35, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    // spaces
    {' ', ' ', 0x39, KEY_ALPHANUMERIC, SPACE, NULL},
    {'\t', '\t', 0x0F, KEY_ALPHANUMERIC, SPACE, NULL},
    // Num pad other are handled on nav group
    {'*', '*', 0x37, KEY_ALPHANUMERIC, NUM_PAD, NULL},
    {'-', '-', 0x4A, KEY_ALPHANUMERIC, NUM_PAD, NULL},
    {'+', '+', 0x4E, KEY_ALPHANUMERIC, NUM_PAD, NULL},
    UNDEFINED_KEY, // Endof array
};
/*
 * Printable/Alphanumeric Key group
 * ---------------------------------------------------------------------------
 *
 * ---------------------------------------------------------------------------
 * Control Key group
 */
static keyboard_key_t control_keys[] = {
    // Press
    {UNDEFINED, UNDEFINED, 0x1D, KEY_CONTROL, PRESS, &left_ctrl},
    {UNDEFINED, UNDEFINED, 0x2A, KEY_CONTROL, PRESS, &left_shift},
    {UNDEFINED, UNDEFINED, 0x36, KEY_CONTROL, PRESS, &right_shift},
    {UNDEFINED, UNDEFINED, 0x38, KEY_CONTROL, PRESS, &left_alt},
    // Release
    {UNDEFINED, UNDEFINED, 0x9D, KEY_CONTROL, RELEASE, &left_ctrl},
    {UNDEFINED, UNDEFINED, 0xAA, KEY_CONTROL, RELEASE, &left_shift},
    {UNDEFINED, UNDEFINED, 0xB6, KEY_CONTROL, RELEASE, &right_shift},
    {UNDEFINED, UNDEFINED, 0xB8, KEY_CONTROL, RELEASE, &left_alt},
    // Toggle
    /* TODO :
     * Actually Lock key are toggled on release but need to improve the logic the file doc/idea.md
     * contain a good way to start
     */
    {UNDEFINED, UNDEFINED, 0xBA, KEY_CONTROL, TOGGLE, &caps_lock},
    {UNDEFINED, UNDEFINED, 0xC5, KEY_CONTROL, TOGGLE, &num_lock},
    {UNDEFINED, UNDEFINED, 0xC6, KEY_CONTROL, TOGGLE, &scroll_lock},
    UNDEFINED_KEY, // Endof array
};

/*
 * Control Key group
 * ---------------------------------------------------------------------------
 *
 * ---------------------------------------------------------------------------
 * Navigation group
 */

typedef enum {
	COLOR_HOME   = VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_GREEN),       // 0x47
	COLOR_UP     = VGA_COLOR(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_RED),   // 0x48
	COLOR_PGUP   = VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_LIGHT_MAGENTA), // 0x49
	COLOR_LEFT   = VGA_COLOR(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BROWN), // 0x4B
	COLOR_CENTER = VGA_COLOR(VGA_COLOR_YELLOW, VGA_COLOR_GREEN),      // 0x4C
	COLOR_RIGHT  = VGA_COLOR(VGA_COLOR_BLACK, VGA_COLOR_MAGENTA),     // 0x4D
	COLOR_END    = VGA_COLOR(VGA_COLOR_BLUE, VGA_COLOR_LIGHT_CYAN),   // 0x4F
	COLOR_DOWN   = VGA_COLOR(VGA_COLOR_BLUE, VGA_COLOR_BLACK),        // 0x50
	COLOR_PGDN   = VGA_COLOR(VGA_COLOR_MAGENTA, VGA_COLOR_YELLOW),    // 0x51
	COLOR_INS    = VGA_COLOR(VGA_COLOR_BROWN, VGA_COLOR_WHITE),       // 0x52
	COLOR_DEL    = VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_YELLOW),        // 0x53
	COLOR_NONE   = VGA_COLOR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY)
} nav_color_t;

static keyboard_key_t navigation_keys[] = {
    // Arrow keys
    {COLOR_UP, '8', 0x48, KEY_NAVIGATION, NUM_PAD, &num_lock},    // Pavé num 8 = Flèche Haut
    {COLOR_DOWN, '2', 0x50, KEY_NAVIGATION, NUM_PAD, &num_lock},  // Pavé num 2 = Flèche Bas
    {COLOR_LEFT, '4', 0x4B, KEY_NAVIGATION, NUM_PAD, &num_lock},  // Pavé num 4 = Flèche Gauche
    {COLOR_RIGHT, '6', 0x4D, KEY_NAVIGATION, NUM_PAD, &num_lock}, // Pavé num 6 = Flèche Droite

    // Home, End, Page Up, Page Down
    {COLOR_HOME, '7', 0x47, KEY_NAVIGATION, NUM_PAD, &num_lock}, // Pavé num 7 = Home
    {COLOR_END, '1', 0x4F, KEY_NAVIGATION, NUM_PAD, &num_lock},  // Pavé num 1 = End
    {COLOR_PGUP, '9', 0x49, KEY_NAVIGATION, NUM_PAD, &num_lock}, // Pavé num 9 = PgUp
    {COLOR_PGDN, '3', 0x51, KEY_NAVIGATION, NUM_PAD, &num_lock}, // Pavé num 3 = PgDn

    // Insert, Delete
    {COLOR_INS, '0', 0x52, KEY_NAVIGATION, NUM_PAD, &num_lock}, // Pavé num 0 = Insert
    {COLOR_DEL, '.', 0x53, KEY_NAVIGATION, NUM_PAD, &num_lock}, // Pavé num . = Delete

    {COLOR_CENTER, '5', 0x4C, KEY_NAVIGATION, NUM_PAD,
     &num_lock}, // Pavé num 5 = Not a nav key but here for consistency

    UNDEFINED_KEY // End of array
};

/*
 * Navigation group
 * ---------------------------------------------------------------------------
 *
 * ---------------------------------------------------------------------------
 * Function group
 */

static keyboard_key_t function_keys[] = {
    {0, UNDEFINED, 0x3B, KEY_FUNCTION, NONE, NULL},  // f1
    {1, UNDEFINED, 0x3C, KEY_FUNCTION, NONE, NULL},  // f2
    {2, UNDEFINED, 0x3D, KEY_FUNCTION, NONE, NULL},  // f3
    {3, UNDEFINED, 0x3E, KEY_FUNCTION, NONE, NULL},  // f4
    {4, UNDEFINED, 0x3F, KEY_FUNCTION, NONE, NULL},  // f5
    {5, UNDEFINED, 0x40, KEY_FUNCTION, NONE, NULL},  // f6
    {6, UNDEFINED, 0x41, KEY_FUNCTION, NONE, NULL},  // f7
    {7, UNDEFINED, 0x42, KEY_FUNCTION, NONE, NULL},  // f8
    {8, UNDEFINED, 0x43, KEY_FUNCTION, NONE, NULL},  // f9
    {9, UNDEFINED, 0x44, KEY_FUNCTION, NONE, NULL},  // f10
    {10, UNDEFINED, 0x57, KEY_FUNCTION, NONE, NULL}, // f11
    {11, UNDEFINED, 0x58, KEY_FUNCTION, NONE, NULL}, // f12
    UNDEFINED_KEY                                    // End of array
};
/*
 * Function group
 * ---------------------------------------------------------------------------
 *
 * ---------------------------------------------------------------------------
 * Special group
 */

static keyboard_key_t special_keys[] = {
    {UNDEFINED, UNDEFINED, 0x0E, KEY_SPECIAL, BACKSPACE, NULL}, // BACKSPACE
    {UNDEFINED, UNDEFINED, 0x1C, KEY_SPECIAL, ENTER, NULL},     // ENTER
    {UNDEFINED, UNDEFINED, 0x01, KEY_SPECIAL, ESCAPE, NULL},    // ESCAPE
    UNDEFINED_KEY};

/*
 * Special group
 *///-------------------------------------------------------------------------

keyboard_key_t azerty_layout[] = {
    // Top Key
    {'&', '1', 0x02, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'e', '2', 0x03, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'"', '3', 0x04, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'\'', '4', 0x05, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'(', '5', 0x06, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'-', '6', 0x07, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'e', '7', 0x08, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'_', '8', 0x09, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'c', '9', 0x0A, KEY_ALPHANUMERIC, TOP_KEY, NULL},
    {'a', '0', 0x0B, KEY_ALPHANUMERIC, TOP_KEY, NULL},

    // diff
    {'a', 'A', 0x10, KEY_ALPHANUMERIC, LETTER, NULL},
    {'z', 'Z', 0x11, KEY_ALPHANUMERIC, LETTER, NULL},
    {'q', 'Q', 0x1E, KEY_ALPHANUMERIC, LETTER, NULL},
    {'m', 'M', 0x27, KEY_ALPHANUMERIC, LETTER, NULL},
    {'w', 'W', 0x2C, KEY_ALPHANUMERIC, LETTER, NULL},
    {',', '?', 0x32, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {';', '.', 0x33, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {':', '/', 0x34, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    {'!', '!', 0x35, KEY_ALPHANUMERIC, PUNCTUATION, NULL},
    UNDEFINED_KEY};
