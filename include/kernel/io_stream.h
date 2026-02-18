#pragma once
#include <memory/kmalloc.h>
#include <types.h>

struct io_stream;

typedef struct io_ops {
	bool (*open)(struct io_stream *stream);
	ssize_t (*read)(struct io_stream *stream, void *dest, size_t size);
	ssize_t (*write)(struct io_stream *stream, const void *src, size_t size);
	ssize_t (*seek)(struct io_stream *stream, ssize_t offset, int origin);
	void (*close)(struct io_stream *stream);
} io_ops_t;

typedef struct io_stream {
	io_ops_t *ops;
	void     *ctx;
	size_t    size;
	size_t    pos;
} io_stream_t;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
	const void *buffer;
	size_t      size;
} mem_stream_ctx_t;

static inline io_stream_t *io_stream_get_new(void *ctx, size_t size, size_t pos)
{
	io_stream_t *ret = kmalloc(sizeof(io_stream_t) + sizeof(io_ops_t), GFP_KERNEL | __GFP_ZERO);
	if (!ret)
		return NULL;

	ret->ops = (io_ops_t *)(ret + 1);

	ret->ctx  = ctx;
	ret->size = size;
	ret->pos  = pos;
	return ret;
}

static inline bool io_open(io_stream_t *s)
{
	return s && s->ops && s->ops->open ? s->ops->open(s) : true;
}

static inline ssize_t io_read(io_stream_t *s, void *d, size_t n)
{
	return s && s->ops && s->ops->read ? s->ops->read(s, d, n) : -1;
}

static inline ssize_t io_write(io_stream_t *s, const void *d, size_t n)
{
	return s && s->ops && s->ops->write ? s->ops->write(s, d, n) : -1;
}

static inline ssize_t io_seek(io_stream_t *s, ssize_t off, int w)
{
	return s && s->ops && s->ops->seek ? s->ops->seek(s, off, w) : -1;
}

static inline void io_close(io_stream_t *s)
{
	if (!s)
		return;
	if (s->ops && s->ops->close)
		s->ops->close(s);
	kfree(s);
}
