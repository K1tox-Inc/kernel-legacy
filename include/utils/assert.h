#pragma once

#define static_assert _Static_assert

#ifndef NDEBUG

# include <kernel/panic.h>

# define __assert_fail(expr) kpanic("Assertion `%s' failed.", #expr)

# define assert(expr)                                                                              \
	 do {                                                                                          \
		 if (unlikely(!(expr)))                                                                    \
			 __assert_fail(expr);                                                                  \
	 } while (0)

#else

# define assert(expr) ((void)0)

#endif
