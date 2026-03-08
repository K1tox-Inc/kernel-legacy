#include <drivers/vga.h>
#include <kernel/io_stream.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/vmm.h>
#include <proc/section.h>
#include <proc/userspace.h>

static inline void init_heap_section(struct section *prev, struct section *heap)
{
	ft_bzero(heap, sizeof(struct section));
	heap->v_addr       = get_next_section_start_after_page_guard(prev->v_addr, prev->mapping_size);
	heap->data_start   = 0;
	heap->data_size    = 0;
	heap->mapping_size = 0;
	heap->flags        = USER_SECTION_RW;
}

static inline void init_stack_section(struct section *stack)
{
	ft_bzero(stack, sizeof(struct section));
	stack->v_addr       = get_prev_section_start(USER_STACK_START, DEFAULT_STACK_SIZE);
	stack->data_start   = 0;
	stack->data_size    = 0;
	stack->mapping_size = ALIGN(DEFAULT_STACK_SIZE, PAGE_SIZE);
	stack->flags        = USER_SECTION_RW;
}

static bool userspace_map_kernel(uint32_t uspace_pd_phy)
{
	uint32_t *uspace_pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(uspace_pd_phy);
	ft_bzero(uspace_pd_virt, PAGE_SIZE);

	uintptr_t kspace_pd_phy = vmm_get_kernel_directory();
	if (!kspace_pd_phy)
		return false;

	uint32_t *kspace_pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(kspace_pd_phy);
	ft_memcpy(&uspace_pd_virt[768], &kspace_pd_virt[768], 224 * sizeof(uint32_t));
	return true;
}

static bool map_section(uintptr_t uspace_pd_phy, struct section *to_map)
{
	if (!to_map || to_map->mapping_size == 0)
		return true;

	void     *kmap_window = NULL;
	uint32_t  pt_count    = to_map->mapping_size / PAGE_SIZE;
	uintptr_t p_pool_addr = (uintptr_t)buddy_alloc_pages(to_map->mapping_size, HIGHMEM_ZONE);
	if (!p_pool_addr)
		return false;

	struct io_stream *stream = NULL;
	if (to_map->data_start && to_map->data_size > 0)
		stream = section_create_reader(to_map);

	if (to_map->data_start && to_map->data_size > 0 && !stream)
		goto bad;
	for (size_t i = 0; i < pt_count; i++) {
		uintptr_t v_addr = to_map->v_addr + (i * PAGE_SIZE);
		uintptr_t p_addr = p_pool_addr + (i * PAGE_SIZE);
		kmap_window      = vmm_kmap(p_addr);
		if (!kmap_window)
			goto bad;
		if (!vmm_map_page(uspace_pd_phy, v_addr, p_addr, to_map->flags))
			goto bad;

		ft_bzero(kmap_window, PAGE_SIZE);
		if (stream) {
			if (io_read(stream, kmap_window, PAGE_SIZE) < 0)
				goto bad;
		}
	}
	to_map->p_addr = p_pool_addr;
	io_close(stream);
	vmm_kunmap();
	return true;

bad:
	buddy_free_block((void *)p_pool_addr);
	io_close(stream);
	if (kmap_window)
		vmm_kunmap();
	return false;
}

bool userspace_create_new(struct task *new_task)
{
	struct section *text = task_text(new_task), *data = task_data(new_task), *last_sec, *stack;

	// Section Text
	if (!text || text->data_size == 0 || text->v_addr >= KERNEL_START)
		return false;

	if (!text->v_addr)
		text->v_addr = USER_TEXT_START;

	text->flags        = USER_SECTION_RO;
	text->mapping_size = ALIGN(text->data_size, PAGE_SIZE);

	// Section Data
	if (data && data->data_size > 0) {
		if (data->v_addr == 0)
			data->v_addr = get_next_section_start(text->v_addr, text->mapping_size);
		else if (data->v_addr >= KERNEL_START)
			return false;

		data->mapping_size = ALIGN(data->data_size, PAGE_SIZE);
		data->flags        = USER_SECTION_RW;
	}

	// Section Heap
	last_sec = (data && data->data_size > 0) ? task_data(new_task) : task_text(new_task);
	init_heap_section(last_sec, task_heap(new_task));

	// Section Stack
	stack = task_stack(new_task);
	init_stack_section(stack);

	// Page Directory
	uintptr_t uspace_pd_phy = (uintptr_t)buddy_alloc_pages(PAGE_SIZE, LOWMEM_ZONE);
	if (!uspace_pd_phy)
		return false;

	// Mapping
	if (!userspace_map_kernel(uspace_pd_phy))
		goto bad;
	else if (!map_section(uspace_pd_phy, task_text(new_task)))
		goto bad;
	else if (!map_section(uspace_pd_phy, task_stack(new_task)))
		goto bad;
	else if ((data && data->data_size > 0) && !map_section(uspace_pd_phy, task_data(new_task)))
		goto bad;

	new_task->cr3 = uspace_pd_phy;

	return true;

bad:
	if (text && text->p_addr)
		buddy_free_block((void *)text->p_addr);
	if (data && data->p_addr)
		buddy_free_block((void *)data->p_addr);
	if (stack && stack->p_addr)
		buddy_free_block((void *)stack->p_addr);

	vmm_destroy_user_pd(uspace_pd_phy);
	return false;
}

// ============================================================================
// DEBUG APIs
// ============================================================================

static void print_section_info(const char *label, const struct section *sec)
{
	if (!sec || (!sec->v_addr && !sec->mapping_size)) {
		vga_printf("  [%s] (not mapped)\n", label);
		return;
	}
	vga_printf("  [%s]\n", label);
	vga_printf("    vaddr       = %p\n", (void *)sec->v_addr);
	vga_printf("    paddr       = %p\n", (void *)sec->p_addr);
	vga_printf("    data_start  = %p\n", (void *)sec->data_start);
	vga_printf("    data_size   = %u bytes\n", sec->data_size);
	vga_printf("    mapping     = %u bytes (%u pages)\n", sec->mapping_size,
	           sec->mapping_size / PAGE_SIZE);
	vga_printf("    flags       = 0x%x (%s)\n", sec->flags,
	           (sec->flags & PTE_RW_BIT) ? "RW" : "RO");
	vga_printf("    range       = [%p - %p)\n", (void *)sec->v_addr,
	           (void *)(sec->v_addr + sec->mapping_size));
}

void userspace_print(const struct task *task)
{
	if (!task) {
		vga_printf("userspace_print: task is NULL\n");
		return;
	}

	vga_printf("========================================\n");
	vga_printf(" Userspace layout: '%s' (PID %d, ring %u)\n", task->name ? task->name : "(null)",
	           task->pid, (unsigned)task->ring);
	vga_printf("========================================\n");

	vga_printf("  cr3 (page dir) = %p\n", (void *)task->cr3);
	vga_printf("  user esp       = %p\n", (void *)task->esp);
	vga_printf("\n");

	print_section_info("TEXT ", task->text_sec);
	print_section_info("DATA ", task->data_sec);
	print_section_info("HEAP ", task->heap_sec);
	print_section_info("STACK", task->stack_sec);

	/* Memory map summary (low -> high) */
	vga_printf("\n  --- Virtual memory map ---\n");
	vga_printf("  0x%08x  USER_TEXT_START\n", USER_TEXT_START);
	if (task->text_sec && task->text_sec->mapping_size)
		vga_printf("  0x%08x  .text end\n", task->text_sec->v_addr + task->text_sec->mapping_size);
	if (task->data_sec && task->data_sec->mapping_size)
		vga_printf("  0x%08x  .data end\n", task->data_sec->v_addr + task->data_sec->mapping_size);
	if (task->heap_sec)
		vga_printf("  0x%08x  heap start (brk)\n", task->heap_sec->v_addr);
	vga_printf("  ...          (free space)\n");
	if (task->stack_sec && task->stack_sec->mapping_size)
		vga_printf("  0x%08x  stack bottom\n", task->stack_sec->v_addr);
	vga_printf("  0x%08x  USER_STACK_START (top)\n", USER_STACK_START);
	vga_printf("  0x%08x  KERNEL_START\n", KERNEL_START);
	vga_printf("========================================\n");
}
