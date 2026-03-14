#pragma once

#include <bitmap/bitmap.h>
#include <types.h>

struct id_manager {
	t_bitmap bitmap;
	ssize_t  next_free;
};

struct id_manager *id_manager_create(size_t max_id);
void               id_manager_destroy(struct id_manager *mgr);
ssize_t            id_manager_alloc(struct id_manager *mgr);
void               id_manager_free(struct id_manager *mgr, size_t id);
bool               id_manager_reserve_id(struct id_manager *mgr, size_t id);
