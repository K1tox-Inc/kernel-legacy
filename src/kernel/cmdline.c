#include <kernel/cmdline.h>
#include <libk.h>

static char        raw[CMDLINE_MAX];
static char        args[CMDLINE_MAX];
static const char *argv[CMDLINE_MAX_ARGS];
static size_t      argc;

static bool is_space(char c) { return c == ' ' || c == '\t' || c == '\n'; }

static void tokenize(void)
{
	char *p = args;

	argc = 0;
	while (*p) {
		while (is_space(*p))
			*p++ = '\0';
		if (!*p)
			break;
		if (argc < CMDLINE_MAX_ARGS)
			argv[argc++] = p;
		while (*p && !is_space(*p))
			p++;
	}
}

void cmdline_set(const char *s, size_t n)
{
	if (n > CMDLINE_MAX - 1)
		n = CMDLINE_MAX - 1;
	ft_memcpy(raw, s, n);
	raw[n] = '\0';
	ft_memcpy(args, raw, n + 1);
	tokenize();
}

const char *cmdline_raw(void) { return raw; }

const char *cmdline_get(const char *key)
{
	size_t len = ft_strlen(key);

	for (size_t i = 0; i < argc; i++)
		if (ft_strncmp(argv[i], key, len) == 0 && argv[i][len] == '=')
			return argv[i] + len + 1;
	return NULL;
}

bool cmdline_has(const char *flag)
{
	for (size_t i = 0; i < argc; i++)
		if (ft_strequ(argv[i], flag))
			return true;
	return false;
}
