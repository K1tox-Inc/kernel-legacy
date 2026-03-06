#pragma once

#include <drivers/vga.h>

#define MAX_TTY       12
#define TTY_HIST_SIZE 256
#define MAX_CMD_LEN   256
#define TTY_PROMPT    "$> "

struct tty_history {
	bool   status;
	bool   stop_scroll;
	size_t top_line_save;
};

struct tty {
	struct vga_entry  *framebuffer;
	size_t             framebuffer_size;
	uint8_t            top_line_index;
	struct tty_history history;
	struct s_cursor    cursor;
	uint8_t            mode;
	char               cli[256];
};

void ttys_init(void);
void tty_init(struct tty *tty);
void tty_load(struct tty *tty);
void tty_switch(struct tty *tty);
void tty_framebuffer_switch_color(uint8_t mode);
void tty_cli_handle_nl(void);
void tty_framebuffer_set_screen_mode(enum vga_color mode);
void tty_framebuffer_clear(void);
void tty_framebuffer_write(char c);
void tty_framebuffer_scroll_down(void);

void tty_history_enable(void);
void tty_history_disable(void);
void tty_history_scroll_up(void);
void tty_history_scroll_down(void);

extern struct tty *current_tty;
extern struct tty  ttys[MAX_TTY];
