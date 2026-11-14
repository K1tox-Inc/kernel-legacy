#include <fs/vfs.h>
#include <utils/assert.h>

struct inode *iget(struct inode *ino)
{
	assert(ino);

	ino->i_count++;

	return ino;
}

void iput(struct inode *ino)
{
	assert(ino && ino->i_count > 0);

	if (--ino->i_count == 0 && ino->i_op && ino->i_op->destroy)
		ino->i_op->destroy(ino);
}
