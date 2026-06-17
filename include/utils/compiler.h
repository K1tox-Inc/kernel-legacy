#pragma once

#ifndef __packed
# define __packed __attribute__((packed))
#endif

#ifndef __always_inline
# define __always_inline inline __attribute__((always_inline))
#endif

#ifndef __always_unused
# define __always_unused __attribute__((__unused__))
#endif

#ifndef __noreturn
# define __noreturn __attribute__((__noreturn__))
#endif

#ifndef unreachable
# define unreachable() __builtin_unreachable()
#endif

#ifndef asmlinkage
# define asmlinkage __attribute__((regparm(0)))
#endif

#ifndef unlikely
# define unlikely(condition) __builtin_expect(!!(condition), 0)
#endif

#ifndef likely
# define likely(condition) __builtin_expect(!!(condition), 1)
#endif
