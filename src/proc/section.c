#include <kernel/io_stream.h>
#include <libk.h>
#include <memory/kmalloc.h>
#include <proc/section.h>

static ssize_t fake_read(io_stream_t *stream, void *dest, size_t size)
{
	section_t *ctx        = (section_t *)stream->ctx;
	size_t     bytes_read = 0;

	if (!ctx || !ctx->data_start || !dest)
		return -1;

	if (stream->pos >= ctx->data_size) {
		ft_bzero(dest, size);
		return 0;
	}

	size_t available = ctx->data_size - stream->pos;
	bytes_read       = (size < available) ? size : available;

	ft_memcpy(dest, (uint8_t *)ctx->data_start + stream->pos, bytes_read);
	stream->pos += bytes_read;

	if (bytes_read < size)
		ft_bzero((uint8_t *)dest + bytes_read, size - bytes_read);

	return bytes_read;
}

io_stream_t *section_create_reader(section_t *sec)
{
	if (!sec || !sec->data_start || sec->data_size == 0)
		return NULL;

	io_stream_t *stream = io_stream_get_new((void *)sec, sec->data_size, 0);
	if (!stream)
		return NULL;

	stream->ops->open  = NULL;
	stream->ops->read  = fake_read;
	stream->ops->write = NULL;
	stream->ops->seek  = NULL;
	stream->ops->close = NULL;

	return stream;
}

bool section_init_from_buffer(section_t *sec, uintptr_t v_addr, const void *start, uint32_t size,
                              uint32_t flags)
{
	if (!sec || !start || size == 0)
		return false;

	sec->v_addr       = v_addr;
	sec->data_size    = size;
	sec->mapping_size = ALIGN(size, PAGE_SIZE);
	sec->flags        = flags;
	sec->data_start   = (uintptr_t)start;

	return true;
}
