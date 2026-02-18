#ifndef SIGNAL_H
#define SIGNAL_H

#include <types.h>

struct signal_node {
	int              signo;
	struct list_head queue;
};

struct signal_queue {
	struct list_head queue;
	size_t           size;
};

#endif
