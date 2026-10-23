#pragma once

#include <utils/compiler.h>

#include "vfs.h"

/* The new dentry holds a reference on `parent` and is returned with `d_count == 1` */
extern struct dentry *d_alloc(struct dentry *parent, const char *name);
/* Publish a dentry whose `d_inode` is set, so `d_lookup` can find it */
extern void d_add(struct dentry *d);
/* Drop a dentry that was never added */
extern void           d_free(struct dentry *d);
extern struct dentry *d_lookup(const struct dentry *parent, const char *name);

extern struct dentry *dget(struct dentry *d);
extern void           dput(struct dentry *d);
