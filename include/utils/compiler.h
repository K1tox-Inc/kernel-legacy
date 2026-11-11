#pragma once

/*
 * The kernel relies on GNU C extensions (attributes, builtins, inline asm).
 * GCC and Clang both define __GNUC__, anything else is unsupported.
 */
#if !defined(__GNUC__)
# error "Unsupported compiler: a GNU C compatible compiler (GCC or Clang) is required"
#endif

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#ifndef BTF_TYPE_TAG
# if defined(CONFIG_DEBUG_INFO_BTF) && __has_attribute(btf_type_tag)
#  define BTF_TYPE_TAG(value) __attribute__((btf_type_tag(#value)))
# else
#  define BTF_TYPE_TAG(value)
# endif
#endif

#ifdef __CHECKER__
# define __kernel                  __attribute__((address_space(0)))
# define __user                    __attribute__((noderef, address_space(__user)))
# define __iomem                   __attribute__((noderef, address_space(__iomem)))
# define __force                   __attribute__((force))
# define __nocast                  __attribute__((nocast))
# define __safe                    __attribute__((safe))
# define __private                 __attribute__((noderef))
# define ACCESS_PRIVATE(p, member) (*((typeof((p)->member) __force *)&(p)->member))
#else
# define __kernel
# ifdef STRUCTLEAK_PLUGIN
#  define __user __attribute__((user))
# else
#  define __user BTF_TYPE_TAG(user)
# endif
# define __iomem
# define __force
# define __nocast
# define __safe
# define __private
# define ACCESS_PRIVATE(p, member)  ((p)->member)
# define __builtin_warning(x, y...) (1)
#endif

#ifndef __nullable
# if defined(__clang__) && __has_feature(nullability)
#  define __nullable _Nullable
# else
#  define __nullable
# endif
#endif

#ifndef __packed
# if __has_attribute(__packed__)
#  define __packed __attribute__((__packed__))
# else
#  error "Compiler does not support __attribute__((packed))"
# endif
#endif

#ifndef __always_inline
# if __has_attribute(__always_inline__)
#  define __always_inline __inline__ __attribute__((__always_inline__))
# else
#  define __always_inline __inline__
# endif
#endif

#ifndef __always_unused
# if __has_attribute(__unused__)
#  define __always_unused __attribute__((__unused__))
# else
#  define __always_unused
# endif
#endif

#ifndef __noreturn
# if __has_attribute(__noreturn__)
#  define __noreturn __attribute__((__noreturn__))
# else
#  define __noreturn _Noreturn
# endif
#endif

/* __builtin_unreachable and __builtin_expect are guaranteed by __GNUC__. */
#ifndef unreachable
# define unreachable() __builtin_unreachable()
#endif

/* regparm only exists on i386; elsewhere GCC warns it is ignored (fatal with -Werror). */
#ifndef asmlinkage
# if defined(__i386__) && __has_attribute(__regparm__)
#  define asmlinkage __attribute__((__regparm__(0)))
# else
#  define asmlinkage
# endif
#endif

#ifndef likely
# define likely(condition) __builtin_expect(!!(condition), 1)
#endif

#ifndef unlikely
# define unlikely(condition) __builtin_expect(!!(condition), 0)
#endif
