#pragma once

#include <multiboot2.h>
#include <types.h>

#define TAGS_NEEDED 1

typedef void (*entry_handler_t)(uintptr_t start, uintptr_t end);

struct multiboot_info {
	uint32_t             total_size;
	uint32_t             reserved;
	struct multiboot_tag tags[0];
};

struct multiboot2_header_tag_end {
	uint16_t type;
	uint16_t flags;
	uint32_t size;
} __attribute__((packed, aligned(8)));

struct multiboot2_header_tag_information_request {
	uint16_t type;
	uint16_t flags;
	uint32_t size;
	uint32_t requests[1];
} __attribute__((packed, aligned(8)));

extern const struct multiboot_header                          mb2_header;
extern const struct multiboot2_header_tag_information_request mb2_tag_info_req;
extern const struct multiboot2_header_tag_end                 mb2_tag_end;
extern struct multiboot_info                                 *mb2info;

static inline struct multiboot_mmap_entry *next_entry(struct multiboot_mmap_entry *mmap_entry,
                                                      struct multiboot_tag_mmap   *mmap)
{
	return (struct multiboot_mmap_entry *)((uint8_t *)mmap_entry + mmap->entry_size);
}

void mb2_mmap_iter(struct multiboot_tag_mmap *mmap, uint8_t *mmap_end, entry_handler_t handler,
                   bool free);
void mb2_init(void);
