#pragma once

#include <kernel/block.h>
#include <list.h>
#include <types.h>
#include <utils/assert.h>

#define EXT2_INITIAL_BLOCK_SIZE 1024
#define EXT2_MAGIC              0xEF53
#define EXT2_MAX_BLOCK_SIZE     (EXT2_INITIAL_BLOCK_SIZE << 4)
#define EXT2_SUPERBLOCK_SIZE    1024
#define EXT2_ROOT_INODE         2
#define EXT2_VALID_FS           0x0001
#define EXT2_ERROR_FS           0x0002
#define EXT2_ERRORS_CONTINUE    0x0001
#define EXT2_ERRORS_REMOUNT     0x0002
#define EXT2_ERRORS_PANIC       0x0003

#define EXT2_OS_LINUX    0
#define EXT2_OS_GNU_HURD 1
#define EXT2_OS_MASIX    2
#define EXT2_OS_FREEBSD  3
#define EXT2_OS_LITES    4

#define EXT2_FEATURE_COMPAT_DIR_PREALLOC    0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES   0x0002
#define EXT2_FEATURE_COMPAT_HAS_JOURNAL     0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR        0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE    0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX       0x0020
#define EXT2_FEATURE_INCOMPAT_COMPRESSION   0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE      0x0002
#define EXT2_FEATURE_INCOMPAT_RECOVER       0x0004
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004

#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7
#define EXT2_FT_WHITEOUT 8

struct ext2_super_block {
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_r_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;
	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	uint32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;
	uint32_t s_inodes_per_group;
	uint32_t s_mtime;
	uint32_t s_wtime;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;
	uint16_t s_errors;
	uint16_t s_minor_rev_level;
	uint32_t s_lastcheck;
	uint32_t s_checkinterval;
	uint32_t s_creator_os;
	uint32_t s_rev_level;
	uint16_t s_def_resuid;
	uint16_t s_def_resgid;
	uint32_t s_first_ino;
	uint16_t s_inode_size;
	uint16_t s_block_group_nr;
	uint32_t s_feature_compat;
	uint32_t s_feature_incompat;
	uint32_t s_feature_ro_compat;
	uint32_t s_uuid[4];
	char     s_volume_name[16];
	char     s_last_mounted[64];
	uint32_t s_algorithm_usage;
	uint8_t  s_prealloc_lo;
	uint8_t  s_prealloc_hi;
	uint16_t : 16;
	uint32_t s_journal_id[4];
	uint32_t s_journal_ino;
	uint32_t s_journal_dev;
	uint32_t s_orph_ino_list_head;
} __packed;

static_assert(sizeof(struct ext2_super_block) == 236, "Misformatted `ext2_super_block' structure.");

struct ext2_group_desc {
	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint32_t bg_reserved[3];
} __packed;

struct ext2_inode {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_ctime;
	uint32_t i_mtime;
	uint32_t i_dtime;
	uint16_t i_gid;
	uint16_t i_links_count;
	uint32_t i_blocks;
	uint32_t i_flags;
	uint32_t i_osd1;
	uint32_t i_block[15];
	uint32_t i_generation;
	uint32_t i_file_acl;
	uint32_t i_dir_acl;
	uint32_t i_faddr;
	uint32_t i_osd2[3];
} __packed;

struct ext2_dir_entry {
	uint32_t inode;
	uint16_t rec_len;
	uint16_t name_len;
	uint8_t  file_type;
	char     name[];
} __packed;

struct ext2_fs_info {
	struct block_device *dev;
	size_t               block_size;
	size_t               sector_size;
	size_t               frag_size;
	uint32_t             inodes_count;
	uint32_t             blocks_count;
	uint32_t             r_blocks_count;
	uint32_t             free_blocks_count;
	uint32_t             free_inodes_count;
	uint32_t             blocks_per_group;
	uint32_t             frags_per_group;
	uint32_t             inodes_per_group;
	uint32_t             first_data_block;
	uint16_t             inode_size;
	uint16_t             magic;
	bool                 valid;
};

extern int ext2_parse_superblock(struct block_device *);
