#include <fs/vfs.h>

const char *vfs_types[] = {[FS_EXT2] = "ext2"};

struct superblock *root_sb = NULL;
