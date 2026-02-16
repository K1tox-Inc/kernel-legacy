#pragma once
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
	void           *ctx;
	size_t          size;
	size_t          pos;
	const io_ops_t *ops;
} io_stream_t;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
	const void *buffer;
	size_t      size;
} mem_stream_ctx_t;

void io_stream_init_mem(io_stream_t *stream, const void *buffer, size_t size);

static inline bool io_open(io_stream_t *s) { return s->ops->open ? s->ops->open(s) : true; }

static inline ssize_t io_read(io_stream_t *s, void *d, size_t n)
{
	return s->ops->read ? s->ops->read(s, d, n) : -1;
}

static inline ssize_t io_write(io_stream_t *s, const void *d, size_t n)
{
	return s->ops->write ? s->ops->write(s, d, n) : -1;
}

static inline ssize_t io_seek(io_stream_t *s, ssize_t off, int w)
{
	return s->ops->seek ? s->ops->seek(s, off, w) : -1;
}

static inline void io_close(io_stream_t *s)
{
	if (s->ops->close)
		s->ops->close(s);
}
