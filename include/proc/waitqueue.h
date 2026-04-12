#pragma once

#include <list.h>
#include <types.h>

// ============================================================================
// STRUCTS & MACROS
// ============================================================================

#define WQ_HEAD_INIT(name)                                                                         \
	{                                                                                              \
		.head = LIST_HEAD_INIT(name.head)                                                          \
	}
#define DECLARE_WQ_HEAD(name) struct wq_head name = WQ_HEAD_INIT(name)

struct task;
struct wq_entry;

typedef void (*finish_handler)(struct wq_entry *entry);

enum wq_state { TASK_INTERRUPTIBLE, TASK_UNINTERRUPTIBLE };

struct wq_entry {
	struct task     *task;
	struct wq_head  *head;
	struct list_head node;
	enum wq_state    state;
	finish_handler   handler;
};

struct wq_head {
	struct list_head head;
};

// ============================================================================
// EXTERNAL APIs
// ============================================================================

static inline void wq_init(struct wq_head *wq) { INIT_SENTINEL(&wq->head); }
static inline void wq_entry_init(struct wq_entry *entry, struct task *task, enum wq_state state)
{
	entry->task    = task;
	entry->head    = NULL;
	entry->handler = NULL;
	entry->state   = state;
	INIT_SENTINEL(&entry->node);
}

void wq_prepare(struct wq_head *wq, struct task *task);
void wq_finish(struct wq_entry *entry);
void wq_wake_specifiq(struct task *task);
void wq_wake_one(struct wq_head *wq);
void wq_wake_all(struct wq_head *wq);
