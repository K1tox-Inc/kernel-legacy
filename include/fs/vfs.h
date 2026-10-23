#pragma once

#include <kernel/block.h>
#include <kernel/hashtable.h>
#include <list.h>
#include <types.h>

enum VFS_Inode_Type {
	VFS_Inode_FIFO       = 0x1,
	VFS_Inode_CharDev    = 0x2,
	VFS_Inode_Dir        = 0x4,
	VFS_Inode_BlkDev     = 0x6,
	VFS_Inode_RegFile    = 0x8,
	VFS_Inode_SymLink    = 0xA,
	VFS_Inode_UnixSocket = 0xC
};

#define VFS_InodeTypeCheck(i, t) ((((struct inode *)(i))->i_mode >> 12) == (t))
#define IS_DIR(i)                VFS_InodeTypeCheck(i, VFS_Inode_Dir)

#define NAME_MAX 255

typedef uint16_t umode_t;

enum vfs_types_e {
	FS_EXT2,
};

extern const char *vfs_types[];

struct inode;
struct file;

struct superblock {
	const char                    *s_type;
	struct block_device           *s_bdev;
	const struct inode_operations *s_i_op;
	void                          *s_fs_info;
	struct dentry                 *s_root;
};

struct inode_operations {
	struct dentry *(*lookup)(struct inode *parent, struct dentry *child);
	int (*read_page)(struct inode *ino, off_t off, void *page);
	int (*write_page)(struct inode *ino, off_t off, const void *page);
	int (*permission)(struct inode *ino, int mask);
	void (*destroy)(struct inode *ino);
};

struct dirctx;

struct file_operations {
	int (*open)(struct file *f);
	int (*release)(struct file *f);
	ssize_t (*read)(struct file *f, void *buf, size_t n);
	ssize_t (*write)(struct file *f, const void *buf, size_t n);
	off_t (*lseek)(struct file *f, off_t off, int whence);
	int (*iterate)(struct file *f, struct dirctx *ctx);
};

struct inode {
	/*
	 *  | 15-12 |      11 |      10 |      9 |       8-0 |
	 *  |  Type | set UID | set GID | Sticky | rwxrwxrwx |
	 */
	umode_t i_mode;

	int                            i_count;
	off_t                          i_size;
	const struct inode_operations *i_op;
	const struct file_operations  *i_f_op;
	struct superblock             *i_sb;
	struct list_head               i_dentry;
	struct list_head               i_node;
	void                          *i_prv;
};

struct dentry {
	const char        *d_name;
	struct dentry     *d_parent;
	struct inode      *d_inode;
	struct superblock *d_sb;
	struct list_head   d_child;
	struct list_head   d_sibling;
	struct hlist_node  d_hash;
	int                d_count;
};

enum VFS_Dirent_Type {
	DT_UNKNOWN = 0,
	DT_FIFO    = 1,
	DT_CHR     = 2,
	DT_DIR     = 4,
	DT_BLK     = 6,
	DT_REG     = 8,
	DT_LNK     = 10,
	DT_SOCK    = 12
};

/*
 * `emit` returns 0 to keep going, nonzero when the sink is full. `iterate` returns 0 at the end of
 * the directory, 1 when it stopped because `emit` was full, or a negative errno.
 */
struct dirctx {
	int (*emit)(const char *name, size_t name_len, uint32_t ino, int type);
	off_t pos;
};

struct file {
	off_t                         f_pos;
	int                           f_flags;
	struct dentry                *f_dentry;
	struct inode                 *f_inode;
	const struct file_operations *f_op;
};

extern struct superblock *root_sb;

// extern struct superblock *vfs_mount_root(struct block_device *dev, const char *fstype);

extern struct inode *iget(struct inode *ino);
extern void          iput(struct inode *ino);

/* Returns a counted reference on the dentry, release it with `dput` */
extern struct dentry *vfs_lookup(const char *path, struct dentry *start);

extern int     vfs_open(const char *path, int flags, struct file **out);
extern int     vfs_close(struct file *f);
extern ssize_t vfs_read(struct file *f, void *buf, size_t n);
extern ssize_t vfs_write(struct file *f, const void *buf, size_t n);
extern off_t   vfs_lseek(struct file *f, off_t off, int whence);
extern int     vfs_iterate(struct file *f, struct dirctx *ctx);

// extern int     vfs_create(struct inode *dir, const char *name, umode_t mode);
// extern int     vfs_mkdir(struct inode *dir, const char *name, umode_t mode);
// extern int     vfs_unlink(struct inode *dir, const struct dentry *victim);
// extern int     vfs_rmdir(struct inode *dir, const struct dentry *victim);
// extern int     vfs_rename(struct dentry *old_dir, struct dentry *old, struct dentry *new_dir,
//                           struct dentry *new_dentry);
// extern int     vfs_truncate(struct inode *ino, size_t size);
// extern ssize_t vfs_readlink(struct inode *ino, char *buf, size_t len);
