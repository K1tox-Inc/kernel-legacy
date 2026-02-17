#include <kernel/io_stream.h>
#include <kernel/panic.h>
#include <libk.h>
#include <memory/vmm.h>
#include <proc/section.h>
#include <proc/task.h>
#include <proc/userspace.h>

static inline void init_heap_section(section_t *prev, section_t *heap)
{
	ft_bzero(heap, sizeof(section_t));
	heap->v_addr       = get_next_section_start_after_page_guard(prev->v_addr, prev->mapping_size);
	heap->data_start   = 0;
	heap->data_size    = 0;
	heap->mapping_size = 0;
	heap->flags        = USER_SECTION_RW;
}

static inline void init_stack_section(section_t *stack)
{
	ft_bzero(stack, sizeof(section_t));
	stack->v_addr       = get_prev_section_start(USER_STACK_START, USER_STACK_SIZE);
	stack->data_start   = 0;
	stack->data_size    = 0;
	stack->mapping_size = ALIGN(USER_STACK_SIZE, PAGE_SIZE);
	stack->flags        = USER_SECTION_RW;
}

static inline section_t *task_text(struct task *new_task) { return &new_task->code_sec; }
static inline section_t *task_data(struct task *new_task) { return &new_task->data_sec; }
static inline section_t *task_heap(struct task *new_task) { return &new_task->heap_sec; }
static inline section_t *task_stack(struct task *new_task) { return &new_task->stack_sec; }

bool userpsace_map_kernel(uint32_t uspace_pd_phy)
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

bool map_section(uintptr_t uspace_pd_phy, section_t *to_map)
{
	if (!to_map || to_map->mapping_size == 0)
		return true;

	uint32_t  pt_count    = to_map->mapping_size / PAGE_SIZE;
	uintptr_t p_pool_addr = (uintptr_t)buddy_alloc_pages(to_map->mapping_size, HIGHMEM_ZONE);
	if (!p_pool_addr)
		return false;

	io_stream_t *stream = NULL;
	if (to_map->data_start && to_map->data_size > 0) {
		stream = section_create_reader(to_map);
		if (!stream) {
			buddy_free_block((void *)p_pool_addr);
			return false;
		}
	}

	for (size_t i = 0; i < pt_count; i++) {
		uintptr_t v_addr      = to_map->v_addr + (i * PAGE_SIZE);
		uintptr_t p_addr      = p_pool_addr + (i * PAGE_SIZE);
		void     *kmap_window = vmm_kmap(p_addr);
		if (!vmm_map_page(uspace_pd_phy, v_addr, p_addr, to_map->flags)) {
			buddy_free_block((void *)p_pool_addr);
			io_close(stream);
			return false;
		}
		if (stream)
			io_read(stream, kmap_window, PAGE_SIZE);
		else
			ft_bzero(kmap_window, PAGE_SIZE);
	}
	io_close(stream);
	vmm_kunmap();
	return true;
}

bool userspace_create_new(section_t *text, section_t *data, struct task *new_task)
{
	// Section Text
	if (!text || text->data_size == 0)
		return false;
	else if (!text->v_addr)
		text->v_addr = USER_TEXT_START;
	text->flags        = USER_SECTION_RO;
	text->mapping_size = ALIGN(text->data_size, PAGE_SIZE);
	ft_memcpy(task_text(new_task), text, sizeof(section_t));

	if (data && data->data_size > 0) {
		if (data->v_addr == 0)
			data->v_addr = get_next_section_start(text->v_addr, text->mapping_size);
		data->mapping_size = ALIGN(data->data_size, PAGE_SIZE);
		data->flags        = USER_SECTION_RW;
		ft_memcpy(task_data(new_task), data, sizeof(section_t));
	}

	section_t *last_sec = (data && data->data_size > 0) ? task_data(new_task) : task_text(new_task);
	init_heap_section(last_sec, task_heap(new_task));

	section_t *stack = task_stack(new_task);
	init_stack_section(stack);

	uintptr_t uspace_pd_phy = (uintptr_t)buddy_alloc_pages(PAGE_SIZE, LOWMEM_ZONE);
	if (!uspace_pd_phy)
		return false;

	bool map_success = true;
	if (!userpsace_map_kernel(uspace_pd_phy))
		map_success = false;
	else if (!map_section(uspace_pd_phy, task_text(new_task)))
		map_success = false;
	else if (!map_section(uspace_pd_phy, task_stack(new_task)))
		map_success = false;
	else {
		if (data && data->data_size > 0) {
			if (!map_section(uspace_pd_phy, task_data(new_task)))
				map_success = false;
		}
	}

	if (!map_success) {
		buddy_free_block((void *)uspace_pd_phy);
		return false;
	}

	new_task->cr3 = uspace_pd_phy;
	return true;
}
