#pragma once

#include <types.h>
#include <utils/compiler.h>

struct dentry;
struct dirctx;
struct file;
struct inode;
struct vm_area;

struct super_operations {
	void (*__nullable destroy)(struct inode *);
};

struct file_operations {
	off_t (*__nullable llseek)(struct file *, off_t offset, int whence);
	ssize_t (*__nullable read)(struct file *, void *buf, size_t len, off_t *pos);
	ssize_t (*__nullable write)(struct file *, const void *buf, size_t len, off_t *pos);
	int (*__nullable iterate)(struct file *, struct dirctx *);
	// unsigned int (*__nullable poll)(struct file *, struct poll_table *);
	int (*__nullable ioctl)(struct file *, unsigned int cmd, unsigned long arg);
	int (*__nullable mmap)(struct file *, struct vm_area *);
	int (*__nullable open)(struct inode *, struct file *);
	int (*__nullable flush)(struct file *);
	int (*__nullable release)(struct inode *, struct file *);
	int (*__nullable fsync)(struct file *, off_t start, off_t end, int datasync);
	// int (*__nullable lock)(struct file *, int cmd, struct file_lock *);
};

struct inode_operations {
	struct dentry *(*__nullable lookup)(struct inode *dir, struct dentry *);
	// const char *(*__nullable get_link)(struct dentry *, struct inode *, struct delayed_call *);
	int (*__nullable permission)(struct inode *, int mask);
	ssize_t (*__nullable readlink)(struct inode *, char *buf, size_t len);
	int (*__nullable create)(struct inode *dir, struct dentry *, umode_t mode);
	int (*__nullable link)(struct dentry *old, struct inode *dir, struct dentry *new);
	int (*__nullable unlink)(struct inode *dir, struct dentry *);
	int (*__nullable symlink)(struct inode *dir, struct dentry *, const char *target);
	int (*__nullable mkdir)(struct inode *dir, struct dentry *, umode_t mode);
	int (*__nullable rmdir)(struct inode *dir, struct dentry *);
	// int (*__nullable mknod)(struct inode *dir, struct dentry *, umode_t mode, dev_t dev);
	int (*__nullable rename)(struct inode *old_dir, struct dentry *old, struct inode *new_dir,
	                         struct dentry *new);
	// int (*__nullable setattr)(struct dentry *, struct iattr *);
};

struct dentry_operations {
	int (*__nullable d_delete)(const struct dentry *);
	void (*__nullable d_release)(struct dentry *);
	void (*__nullable d_iput)(struct dentry *, struct inode *);
};
