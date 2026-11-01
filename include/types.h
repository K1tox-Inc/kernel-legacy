#ifndef TYPES_H
#define TYPES_H

#ifndef NULL
# define NULL ((void *)0)
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
# define bool  _Bool
# define true  ((bool)1)
# define false ((bool)0)
#endif

typedef __INT8_TYPE__   int8_t;
typedef __INT16_TYPE__  int16_t;
typedef __INT32_TYPE__  int32_t;
typedef __INT64_TYPE__  int64_t;
typedef __UINT8_TYPE__  uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __INTPTR_TYPE__  intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;

typedef __SIZE_TYPE__    size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef int              ssize_t;

typedef int            pid_t;
typedef unsigned int   uid_t;
typedef unsigned int   gid_t;
typedef unsigned short umode_t;

typedef uint64_t ino64_t;
typedef int64_t  loff_t;
typedef int32_t  off_t;

#include <utils/compiler.h>

#endif
