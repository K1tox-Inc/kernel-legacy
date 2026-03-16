#include <arch/acpi.h>
#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <drivers/vga.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <proc/exec.h>

struct tty ttys[12], *current_tty = ttys;

static void print_help(void);

static inline bool ft_strequ(const char *s1, const char *s2)
{
	size_t len1 = ft_strlen(s1);
	size_t len2 = ft_strlen(s2);

	if (len1 != len2)
		return false;

	return ft_memcmp(s1, s2, len1 + 1) == 0;
}

void tty_framebuffer_set_screen_mode(struct tty *tty, enum vga_color mode)
{
	tty->mode = mode;
	for (unsigned long i = 0; i < VGA_WIDTH * TTY_HIST_SIZE; i++)
		tty->framebuffer[i].mode = mode;
}

static inline void tty_print_prompt(void) { vga_printf("%s", TTY_PROMPT); }

void tty_framebuffer_clear(struct tty *tty)
{
	for (unsigned long i = 0; i < VGA_WIDTH * TTY_HIST_SIZE; i++)
		tty->framebuffer[i] = (struct vga_entry){.character = 0x00, .mode = tty->mode};

	tty->top_line_index = 0;
	tty->cursor         = (struct s_cursor){0, 0};
}

void tty_history_enable(void)
{
	vga_disable_cursor();
	current_tty->history.status        = true;
	current_tty->history.top_line_save = current_tty->top_line_index;
}

void tty_history_disable(void)
{
	vga_enable_cursor(14, 15);
	current_tty->history.status = false;
	current_tty->top_line_index = current_tty->history.top_line_save;
}

void tty_history_scroll_up(void)
{
	if (!current_tty->history.status)
		return;
	uint8_t oldest_line;
	if (current_tty->history.stop_scroll == true)
		oldest_line = 0;
	else
		oldest_line = (uint8_t)current_tty->history.top_line_save + 1;
	if (current_tty->top_line_index == oldest_line)
		return;
	current_tty->top_line_index--;
}

void tty_history_scroll_down(void)
{
	if (!current_tty->history.status)
		return;
	uint8_t real_y = (uint8_t)current_tty->history.top_line_save + (uint8_t)current_tty->cursor.y;
	uint8_t cur_y  = (uint8_t)current_tty->top_line_index + (uint8_t)current_tty->cursor.y;
	if (real_y == cur_y)
		return;
	current_tty->top_line_index++;
}

void tty_framebuffer_scroll_down(void)
{
	current_tty->top_line_index++;
	if (current_tty->top_line_index == 0)
		current_tty->history.stop_scroll = false;
	uint8_t bottom_line = (uint8_t)current_tty->top_line_index + (uint8_t)(VGA_HEIGHT - 1);
	for (size_t x = 0; x < VGA_WIDTH; x++) {
		int offset = (bottom_line * VGA_WIDTH) + x;
		current_tty->framebuffer[offset] =
		    (struct vga_entry){.character = 0x00, .mode = current_tty->mode};
	}
}

void tty_framebuffer_write(struct tty *tty, char c)
{
	if (!tty || !tty->framebuffer)
		return;

	if (tty->history.status)
		tty_history_disable();

	uint8_t real_y           = (uint8_t)tty->top_line_index + (uint8_t)tty->cursor.y;
	int     offset           = (real_y * VGA_WIDTH) + tty->cursor.x;
	tty->framebuffer[offset] = (struct vga_entry){c, tty->mode};
}

static void tty_current_tty_clear(void) { tty_framebuffer_clear(current_tty); }

static void exec_mok_cafe() { exec_mok("cafe"); }

struct shell_command {
	const char *cmd;
	const char *descr;
	void (*func)(void);
};

struct shell_command shell_commands[] = {{"poweroff", "Power off the system.", shutdown},
                                         {"reboot", "Reboot the system.", reboot},
                                         {"halt", "Halt the system.", halt},
                                         {"clear", "Clear the current tty.", tty_current_tty_clear},
                                         {"cafe", "Run the mok process: cafe.", exec_mok_cafe},
                                         {"help", "Print this help message.", print_help}};

#define iter_over_array(p, a)                                                                      \
	for (p = a; (uintptr_t)p - (uintptr_t)a <= sizeof(a) - sizeof(typeof(*a)); p++)

static void print_help(void)
{
	struct shell_command *cmd;
	vga_printf("Usage: [command] [options]\n\nAvailable commands:\n");
	iter_over_array(cmd, shell_commands) { vga_printf("\t%s\t\t%s\n", cmd->cmd, cmd->descr); }
}

void tty_cli_handle_nl(void)
{
	void (*func)(void) = NULL;

	vga_printf("\n");

	if (*current_tty->cli) {
		struct shell_command *cmd;
		iter_over_array(cmd, shell_commands)
		{
			if (ft_strequ(cmd->cmd, current_tty->cli)) {
				func = cmd->func;
				break;
			}
		}

		func ? func() : vga_printf("k1tOS: command not found: %s\n", current_tty->cli);

		ft_bzero(current_tty->cli, 256);
	}

	tty_print_prompt();
}

void tty_init(struct tty *tty)
{
	tty->top_line_index        = 0;
	tty->cursor                = (struct s_cursor){0, 0};
	tty->history.status        = false;
	tty->history.top_line_save = 0;
	tty->history.stop_scroll   = true;
	tty->mode                  = VGA_DEFAULT_MODE;
	tty->framebuffer_size      = (VGA_WIDTH * TTY_HIST_SIZE) * sizeof(struct vga_entry);
	tty->framebuffer           = NULL;

	ft_bzero(tty->cli, 256);
}

void ttys_init(void)
{
	struct tty *tty;
	iter_over_array(tty, ttys) { tty_init(tty); }
	tty_load(ttys);
	vga_enable_cursor(14, 15);
}

static void tty_framebuffer_init(struct tty *tty)
{
	tty->framebuffer = kmalloc(tty->framebuffer_size, (GFP_KERNEL | __GFP_ZERO));
	if (tty->framebuffer == NULL)
		kpanic("No space left to init ttys.");

	tty_framebuffer_set_screen_mode(tty, tty->mode);
	tty_framebuffer_clear(tty);
}

void tty_load(struct tty *tty)
{
	if (tty->framebuffer == NULL) {
		tty_framebuffer_init(tty);
		vga_setup_default_screen();
		tty_print_prompt();
	}

	current_tty = tty;
	vga_refresh_screen();
}
