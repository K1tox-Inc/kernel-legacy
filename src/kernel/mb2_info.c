#include <kernel/mb2_info.h>
#include <kernel/panic.h>
#include <memory/boot_allocator.h>

// Globals

multiboot_info_t *mb2info = NULL;

// Principal Header
const struct multiboot_header mb2_header
    __attribute__((used, section(".multiboot2_header"), aligned(8))) = {
        .magic         = MULTIBOOT2_HEADER_MAGIC,
        .architecture  = 0,
        .header_length = sizeof(struct multiboot_header) +
                         sizeof(struct multiboot2_header_tag_information_request) +
                         sizeof(struct multiboot2_header_tag_end),
        .checksum = 0x100000000 - (MULTIBOOT2_HEADER_MAGIC + 0 +
                                   (sizeof(struct multiboot_header) +
                                    sizeof(struct multiboot2_header_tag_information_request) +
                                    sizeof(struct multiboot2_header_tag_end)))};

// Tag information request
const struct multiboot2_header_tag_information_request mb2_tag_info_req
    __attribute__((used, section(".multiboot2_header"), aligned(8))) = {
        .type     = 1,
        .flags    = 0,
        .size     = sizeof(struct multiboot2_header_tag_information_request),
        .requests = {MULTIBOOT_TAG_TYPE_MMAP} // 6 = memory map
};

// End Tag
const struct multiboot2_header_tag_end mb2_tag_end
    __attribute__((used, section(".multiboot2_header"), aligned(8))) = {
        .type = 0, .flags = 0, .size = sizeof(struct multiboot2_header_tag_end)};

// Functions

void mb2_mmap_iter(multiboot_tag_mmap_t *mmap, uint8_t *mmap_end, entry_handler_t handler,
                   bool free)
{
	for (multiboot_memory_map_t *entry = mmap->entries; (uint8_t *)entry < mmap_end;
	     entry                         = next_entry(entry, mmap)) {

		if (entry->type != 1 && free == true)
			continue;
		else if (entry->type == 1 && free != true)
			continue;

		uintptr_t region_start = (uintptr_t)entry->addr;
		uintptr_t region_end   = (uintptr_t)entry->addr + entry->len;
		handler(region_start, region_end);
	}
}

extern uint32_t mb2_magic;
extern uint32_t mb2_mbi;

void mb2_init(void)
{
	/* Make sure the magic number matches for memory mapping*/
	if (mb2_magic != MULTIBOOT2_BOOTLOADER_MAGIC)
		kpanic("Invalid magic number: expected 0x%x | used 0x%x\n", MULTIBOOT2_BOOTLOADER_MAGIC,
		       mb2_magic);
	int tags_found            = 0;
	mb2info                   = (multiboot_info_t *)mb2_mbi;
	struct multiboot_tag *tag = (struct multiboot_tag *)&mb2info->tags[0];

	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		switch (tag->type) {
		case MULTIBOOT_TAG_TYPE_MMAP:
			multiboot_tag_mmap_t *mmap = (multiboot_tag_mmap_t *)tag;
			boot_allocator_init(mmap, (uint8_t *)mmap + mmap->size);
			tags_found++;
			break;

		default:
			break;
		}
		tag = (struct multiboot_tag *)((uint8_t *)tag + ALIGN(tag->size, 8));
	}
	if (tags_found != TAGS_NEEDED)
		kpanic("Multiboot2: Not whole tags founded\n");
}
