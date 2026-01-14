#include "bitmap.h"

#define BIT_IS_FULL 255

ssize_t bitmap_find_next_zero(size_t nb_bits, uint8_t *data)
{
	return bitmap_find_next_zero_from(nb_bits, data, 0);
}

ssize_t bitmap_find_next_zero_from(size_t nb_bits, uint8_t *data, size_t bit_start)
{
	if (!data || bit_start >= nb_bits)
		return -1;

	size_t byte_start = bit_start / 8;
	size_t bit_offset = bit_start % 8;
	size_t max        = BITS_TO_BYTES(nb_bits);

	for (size_t i = byte_start; i < max; i++) {
		if (data[i] == BIT_IS_FULL)
			continue;
		size_t offset = (i == byte_start) ? bit_offset : 0;
		for (size_t j = offset; j < 8; j++) {
			if (data[i] >> j & 1)
				continue;
			size_t index = i * 8 + j;

			if (index >= nb_bits)
				return -1;

			return (ssize_t)index;
		}
	}
	return -1;
}
