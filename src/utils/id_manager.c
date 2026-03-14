#include <memory/kmalloc.h>
#include <memory/memory.h>
#include <utils/id_manager.h>

struct id_manager *id_manager_create(size_t max_id)
{
	size_t nb_of_bytes = BITS_TO_BYTES(max_id);

	struct id_manager *ret = kmalloc(sizeof(struct id_manager) + nb_of_bytes, GFP_KERNEL);
	if (!ret)
		return NULL;

	ret->bitmap.data    = (uint8_t *)(ret + 1);
	ret->bitmap.nb_bits = max_id;
	ret->next_free      = 0;

	bitmap_clear(&ret->bitmap);

	return ret;
}

void id_manager_destroy(struct id_manager *mgr)
{
	if (mgr)
		kfree(mgr);
}

ssize_t id_manager_alloc(struct id_manager *mgr)
{
	if (!mgr || mgr->next_free == -1)
		return -1;
	else {
		ssize_t allocated = mgr->next_free;
		bitmap_set(&mgr->bitmap, allocated);
		mgr->next_free =
		    bitmap_find_next_zero_from(mgr->bitmap.nb_bits, mgr->bitmap.data, allocated + 1);
		if (mgr->next_free == -1)
			mgr->next_free = bitmap_find_next_zero(mgr->bitmap.nb_bits, mgr->bitmap.data);
		return allocated;
	}
}

void id_manager_free(struct id_manager *mgr, size_t id)
{
	if (mgr) {
		bitmap_unset(&mgr->bitmap, id);
		if (mgr->next_free == -1 || (ssize_t)id < mgr->next_free)
			mgr->next_free = id;
	}
}

bool id_manager_reserve_id(struct id_manager *mgr, size_t id)
{
	if (!mgr || bitmap_check(&mgr->bitmap, id) || id >= mgr->bitmap.nb_bits)
		return false;
	else {
		bitmap_set(&mgr->bitmap, id);
		if ((ssize_t)id == mgr->next_free) {
			mgr->next_free =
			    bitmap_find_next_zero_from(mgr->bitmap.nb_bits, mgr->bitmap.data, id + 1);
			if (mgr->next_free == -1)
				mgr->next_free = bitmap_find_next_zero(mgr->bitmap.nb_bits, mgr->bitmap.data);
		}
		return true;
	}
}
