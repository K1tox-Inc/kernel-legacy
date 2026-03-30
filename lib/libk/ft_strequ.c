#include "libk.h"

bool ft_strequ(const char *s1, const char *s2)
{
	size_t len1 = ft_strlen(s1);
	size_t len2 = ft_strlen(s2);

	if (len1 != len2)
		return false;

	return ft_memcmp(s1, s2, len1 + 1) == 0;
}