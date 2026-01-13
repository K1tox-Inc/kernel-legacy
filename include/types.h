#ifndef TYPES_H
#define TYPES_H

#ifndef NULL
# define NULL (void *)0
#endif

#define bool  _Bool
#define true  1
#define false 0

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned int   uintptr_t;

typedef unsigned int size_t;
typedef signed int   ssize_t;

typedef int pid_t;
typedef int uid_t;
typedef int gid_t;

struct list_head {
	struct list_head *next, *prev;
};

#endif
