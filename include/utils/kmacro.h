#pragma once

#if __has_attribute(__fallthrough__)
# define fallthrough __attribute__((__fallthrough__))
#else
# define fallthrough                                                                               \
	 do {                                                                                          \
	 } while (0) /* fallthrough */
#endif // __fallthrough__

// Macro
#define ALIGN(x, a)              __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN_DOWN(x, a)         ((x) & ~((typeof(x))(a) - 1))
#define IS_ALIGNED(addr, a)      (ALIGN((addr), (a)) == (addr))
#define DIV_ROUND_UP(x, y)       (x + y - 1) / y
#define FLAG_IS_SET(flags, flag) (((flags) & (flag)) != 0)
#define FLAG_SET(flags, flag)    ((flags) |= (flag))
#define FLAG_UNSET(flags, flag)  ((flags) &= ~(flag))
#define MIN(a, b)                ((a) < (b) ? (a) : (b))
#define MAX(a, b)                ((a) > (b) ? (a) : (b))
#ifdef offsetof
# undef offsetof
#endif
#define offsetof(type, member) ((size_t)&((type *)0)->member)
#ifdef container_of
# undef container_of
#endif
#define container_of(ptr, type, member)                                                            \
	({                                                                                             \
		const typeof(((type *)0)->member) *__mptr = (ptr);                                         \
		(type *)((char *)__mptr - offsetof(type, member));                                         \
	})
#define BITS_TO_BYTES(nb_bits) DIV_ROUND_UP((nb_bits), 8)

// Define
