#pragma once

#include <list.h>
#include <types.h>

// ============================================================================
// STRUCTS & MACROS
// ============================================================================
struct task;

typedef void (*finish_handler)(struct wq_entry *entry);

struct wq_entry {
	struct task     *task;
	finish_handler   handler;
	struct list_head node;
};

struct wq_head {
	struct list_head head;
};

// ============================================================================
// EXTERNAL APIs
// ============================================================================

static inline void wq_init(struct wq_head *wq) { INIT_SENTINEL(&wq->head); }
static inline void wq_entry_init(struct wq_entry *entry, struct task *task)
{
	entry->task    = task;
	entry->handler = NULL;
	INIT_SENTINEL(&entry->node);
}

void wq_prepare(struct wq_head *wq, struct task *task);
void wq_finish(struct wq_entry *entry);
void wq_wake_one(struct wq_head *wq);
void wq_wake_all(struct wq_head *wq);
