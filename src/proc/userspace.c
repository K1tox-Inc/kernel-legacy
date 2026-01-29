#include <memory/vmm.h>
#include <proc/userspace.h>
#include <kernel/panic.h>


// bool vmm_map_page(uintptr_t page_dir_phys, uintptr_t v_addr, uintptr_t p_addr, uint32_t flags)

uintptr_t userspace_create_new(section_t *text, section_t *data, section_t *stack) {
    uintptr_t kernel_space = vmm_get_kernel_directory();
    if (!kernel_space)
        return 0;
    
    // Section Text
    if (!text || text->size == 0)
        return 0;

    if (!data) {

    }
}
