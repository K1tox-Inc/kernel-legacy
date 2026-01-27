#include "drivers/keyboard.h"
#include <arch/io.h>
#include <drivers/tty.h>
#include <drivers/vga.h>
#include <libk.h>
#include <stdarg.h>
#include <utils/kmacro.h>

#define KERNEL_BANNER                                                                              \
	"  /$$   /$$   /$$   /$$               /$$   /$$\n"                                            \
	" | $$  /$$/ /$$$$  | $$              | $$  / $$\n"                                            \
	" | $$ /$$/ |_  $$ /$$$$$$    /$$$$$$ |  $$/ $$/\n"                                            \
	" | $$$$$/    | $$|_  $$_/   /$$__  $$ \\  $$$$/ \n"                                           \
	" | $$  $$    | $$  | $$    | $$  \\ $$  >$$  $$ \n"                                           \
	" | $$\\  $$   | $$  | $$ /$$| $$  | $$ /$$/\\  $$\n"                                          \
	" | $$ \\  $$ /$$$$$$|  $$$$/|  $$$$$$/| $$  \\ $$\n"                                          \
	" |__/  \\__/|______/ \\___/   \\______/ |__/  |__/\n"

#define KERN_EMERG   "0"
#define KERN_ALERT   "1"
#define KERN_CRIT    "2"
#define KERN_ERR     "3"
#define KERN_WARNING "4"
#define KERN_NOTICE  "5"
#define KERN_INFO    "6"
#define KERN_DEBUG   "7"
#define KERN_DEFAULT ""
#define KERN_CONT    "c"

#define VGA_ENTRY_SIZE (VGA_WIDTH * VGA_HEIGHT) * sizeof(vga_entry)

void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

static int ft_putchar(char c)
{
	switch (c) {
	case '\n':
		current_tty->cursor.y++;
		fallthrough;

	case '\r':
		current_tty->cursor.x = 0;
		break;

	case '\t':
		do {
			ft_putchar(' ');
		} while (current_tty->cursor.x % 4);
		break;

	default:
		tty_framebuffer_write(c);
		current_tty->cursor.x++;
	}

	while (current_tty->cursor.x >= VGA_WIDTH) {
		current_tty->cursor.x -= VGA_WIDTH;
		current_tty->cursor.y++;
	}

	while (current_tty->cursor.y >= VGA_HEIGHT) {
		tty_framebuffer_scroll_down();
		current_tty->cursor.y--;
	}

	return 1;
}

static int ft_putstr(char *str)
{
	int i = -1;

	if (str == NULL)
		return (ft_putstr("(null)"));
	while (str[++i])
		ft_putchar(str[i]);
	return (i);
}

static int ft_putnbr(int nb)
{
	if (nb == -2147483648)
		return (ft_putnbr(-21474) + ft_putnbr(83648));
	if (nb < 0)
		return (ft_putchar('-') + ft_putnbr(nb * -1));
	else if (nb >= 10)
		return (ft_putnbr(nb / 10) + ft_putnbr(nb % 10));
	else
		return (ft_putchar(nb + '0'));
}

static int ft_putunsignednbr(uint32_t nb)
{
	if (nb >= 10)
		return (ft_putnbr(nb / 10) + ft_putnbr(nb % 10));
	else
		return (ft_putchar(nb + '0'));
}

static int ft_puthexa(unsigned long nb, bool upper, bool addr)
{
	char *dict = upper ? "0123456789ABCDEF" : "0123456789abcdef";

	if (addr)
		return (ft_putstr("0x") + ft_puthexa(nb, upper, 0));
	else if (nb < 16)
		return (ft_putchar(dict[nb]));
	else
		return (ft_puthexa(nb / 16, upper, 0) + ft_puthexa(nb % 16, upper, 0));
}

// Internal APIs

// External APIs

void vga_setup_default_screen(void)
{
	tty_framebuffer_clear();
	vga_printf(KERNEL_BANNER "\n" TTY_PROMPT);
}

void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void vga_disable_cursor(void)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void vga_set_cursor_position(uint8_t x, uint8_t y)
{
	uint16_t pos = y * VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, pos);
	outb(0x3D4, 0x0E);
	outb(0x3D5, pos >> 8);
}

void vga_refresh_screen(void)
{
	if (!current_tty || !current_tty->framebuffer)
		return;

	for (int vga_y = 0; vga_y < VGA_HEIGHT; vga_y++) {
		uint8_t    line_buffer = (uint8_t)current_tty->top_line_index + (uint8_t)vga_y;
		vga_entry *src         = &current_tty->framebuffer[line_buffer * VGA_WIDTH];
		vga_entry *dst         = VGA_ENTRY(0, vga_y);
		ft_memcpy(dst, src, VGA_WIDTH * sizeof(vga_entry));
	}
	vga_set_cursor_position(current_tty->cursor.x, current_tty->cursor.y);
}

void vga_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	for (unsigned long i = 0; fmt[i]; i++) {
		if (fmt[i] == '%') {
			switch (fmt[++i]) {
			case 's':
				ft_putstr(va_arg(ap, char *));
				break;

			case 'i':
			case 'd':
				ft_putnbr(va_arg(ap, int));
				break;

			case 'u':
				ft_putunsignednbr(va_arg(ap, uint32_t));
				break;

			case 'x':
				ft_puthexa(va_arg(ap, uint32_t), false, false);
				break;

			case 'X':
				ft_puthexa(va_arg(ap, uint32_t), true, false);
				break;

			case 'p':
				ft_puthexa(va_arg(ap, uint32_t), false, true);
				break;

			case 'c':
				ft_putchar(va_arg(ap, uint32_t));
				break;

			default:
				break;
			}
		} else
			ft_putchar(fmt[i]);
	}
	vga_refresh_screen();
}
