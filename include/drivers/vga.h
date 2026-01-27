#pragma once

// ============================================================================
// INCLUDES
// ============================================================================

#include <memory/memory.h>
#include <types.h>

// ============================================================================
// DEFINE AND MACRO
// ============================================================================

// Defines
#define VGA_WIDTH         80
#define VGA_HEIGHT        25
#define VGA_COLOR(bg, fg) ((bg) << 4 | (fg))
#define VGA_DEFAULT_MODE  VGA_COLOR(VGA_COLOR_BLUE, VGA_COLOR_WHITE)
#define VGA_BUFFER        ((vga_entry *)((uintptr_t)0xB8000 + KERNEL_VADDR_BASE))
#define VGA_ENTRY(x, y)   (VGA_BUFFER + ((y) * VGA_WIDTH + (x)))

// Macros

// ============================================================================
// STRUCT
// ============================================================================

// Enums
enum vga_color {
	VGA_COLOR_BLACK         = 0,
	VGA_COLOR_BLUE          = 1,
	VGA_COLOR_GREEN         = 2,
	VGA_COLOR_CYAN          = 3,
	VGA_COLOR_RED           = 4,
	VGA_COLOR_MAGENTA       = 5,
	VGA_COLOR_BROWN         = 6,
	VGA_COLOR_LIGHT_GREY    = 7,
	VGA_COLOR_DARK_GREY     = 8,
	VGA_COLOR_LIGHT_BLUE    = 9,
	VGA_COLOR_LIGHT_GREEN   = 10,
	VGA_COLOR_LIGHT_CYAN    = 11,
	VGA_COLOR_LIGHT_RED     = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_YELLOW        = 14,
	VGA_COLOR_WHITE         = 15,
};

// Structures
typedef struct __attribute__((packed)) {
	uint8_t character;
	uint8_t mode;
} vga_entry;

struct s_cursor {
	uint8_t x;
	uint8_t y;
};

// Typedefs

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// ============================================================================
// EXTERNAL APIs
// ============================================================================

void vga_disable_cursor(void);
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void vga_set_cursor_position(uint8_t x, uint8_t y);
void vga_set_mode(enum vga_color mode);
void vga_setup_default_screen();
void vga_refresh_screen(void);
void vga_printf(const char *fmt, ...);
