#pragma once
#include <memory/kmalloc.h>
#include <types.h>

struct io_stream;

struct io_ops {
	bool (*open)(struct io_stream *stream);
	ssize_t (*read)(struct io_stream *stream, void *dest, size_t size);
	ssize_t (*write)(struct io_stream *stream, const void *src, size_t size);
	ssize_t (*seek)(struct io_stream *stream, ssize_t offset, int origin);
	void (*close)(struct io_stream *stream);
};

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct mem_stream_ctx {
	const void *buffer;
	size_t      size;
};

struct io_stream {
	struct io_ops *ops;
	void          *ctx;
	size_t         size;
	size_t         pos;
};

static inline struct io_stream *io_stream_get_new(void *ctx, size_t size, size_t pos)
{
	struct io_stream *ret =
	    kmalloc(sizeof(struct io_stream) + sizeof(struct io_ops), GFP_KERNEL | __GFP_ZERO);
	if (!ret)
		return NULL;

	ret->ops = (struct io_ops *)(ret + 1);

	ret->ctx  = ctx;
	ret->size = size;
	ret->pos  = pos;
	return ret;
}

static inline bool io_open(struct io_stream *s)
{
	return s && s->ops && s->ops->open ? s->ops->open(s) : true;
}

static inline ssize_t io_read(struct io_stream *s, void *d, size_t n)
{
	return s && s->ops && s->ops->read ? s->ops->read(s, d, n) : -1;
}

static inline ssize_t io_write(struct io_stream *s, const void *d, size_t n)
{
	return s && s->ops && s->ops->write ? s->ops->write(s, d, n) : -1;
}

static inline ssize_t io_seek(struct io_stream *s, ssize_t off, int w)
{
	return s && s->ops && s->ops->seek ? s->ops->seek(s, off, w) : -1;
}

static inline void io_close(struct io_stream *s)
{
	if (!s)
		return;
	if (s->ops && s->ops->close)
		s->ops->close(s);
	kfree(s);
}
