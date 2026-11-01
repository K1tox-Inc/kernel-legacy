#pragma once

#include <utils/compiler.h>

#include "vfs.h"

extern struct dentry *d_alloc(struct dentry *parent, const char *name);
extern void           d_add(struct dentry *d);
extern void           d_free(struct dentry *d);
extern struct dentry *d_lookup(const struct dentry *parent, const char *name);
/* Evicts every descendant of `d`, or returns `-EBUSY` if one of them is still referenced */
extern int            d_prune(struct dentry *d);
extern struct dentry *dget(struct dentry *d);
extern void           dput(struct dentry *d);
