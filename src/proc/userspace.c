#include <memory/vmm.h>
#include <proc/userspace.h>
#include <kernel/panic.h>
#include <libk.h>


bool userpsace_map_kernel(uint32_t *uspace_pd_virt)
{
    uintptr_t kspace_pd_phy = vmm_get_kernel_directory();
    if (!kspace_pd_phy)
        return false;
    
    uint32_t *kspace_pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(kspace_pd_phy);
    ft_memcpy(&uspace_pd_virt[768], &kspace_pd_virt[768], 224 * sizeof(uint32_t));
    return true;
}

void    map_section();

bool userspace_create_new(section_t *text, section_t *data, section_t *stack, struct task *new_task)
{   
    // Section Text 
    if (!text || text->size == 0)
        return false;

    uintptr_t uspace_pd_phy =  (uintptr_t)buddy_alloc_pages(PAGE_SIZE, LOWMEM_ZONE);
    if (!uspace_pd_phy)
        return false;

    uint32_t *uspace_pd_virt = (uint32_t *)PHYS_TO_VIRT_LINEAR(uspace_pd_phy);
	ft_bzero(uspace_pd_virt, PAGE_SIZE);

    if (userpsace_map_kernel(uspace_pd_virt)) {
        buddy_free_block(uspace_pd_phy);
        return 0;
    }

    new_task->code_sec = *text;
    map_section(uspace_pd_virt, text);

}


