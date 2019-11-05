/*
 * endian.h
 *
 * Endianess handling
 *
 * azuepke, 2013-04-03
 */

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include <stdint.h>

/* byte swaps */

#define __bswap64(x) \
	({	uint64_t __x = (x); \
		(((__x & 0xff00000000000000ULL) >> 56) | \
		 ((__x & 0x00ff000000000000ULL) >> 40) | \
		 ((__x & 0x0000ff0000000000ULL) >> 24) | \
		 ((__x & 0x000000ff00000000ULL) >>  8) | \
		 ((__x & 0x00000000ff000000ULL) <<  8) | \
		 ((__x & 0x0000000000ff0000ULL) << 24) | \
		 ((__x & 0x000000000000ff00ULL) << 40) | \
		 ((__x & 0x00000000000000ffULL) << 56) ); \
	})

#define __bswap32(x) \
	({	uint32_t __x = (x); \
		(((__x & 0xff000000) >> 24) | \
		 ((__x & 0x00ff0000) >>  8) | \
		 ((__x & 0x0000ff00) <<  8) | \
		 ((__x & 0x000000ff) << 24) ); \
	})

#define __bswap16(x) \
	({	uint16_t __x = (x); \
		(((__x & 0xff00) >>  8) | \
		 ((__x & 0x00ff) <<  8) ); \
	})



/* endianess detectors */

static inline int __host_is_be(void)
{
	union {
		uint32_t i;
		char c[4];
	} __endianess_union = { .i = 0x01020304 };

	return __endianess_union.c[0] == 1;
}

static inline int __host_is_le(void)
{
	return !__host_is_be();
}


/* conversion macros
 *
 * LE little endian
 * BE big endian
 * H  host endian
 * X  target endian, true for big
 */
#define __htobe16(x)		(__host_is_be() ? ((uint16_t)(x)) : __bswap16(x))
#define __htobe32(x)		(__host_is_be() ? ((uint32_t)(x)) : __bswap32(x))
#define __htobe64(x)		(__host_is_be() ? ((uint64_t)(x)) : __bswap64(x))
#define __htole16(x)		(__host_is_le() ? ((uint16_t)(x)) : __bswap16(x))
#define __htole32(x)		(__host_is_le() ? ((uint32_t)(x)) : __bswap32(x))
#define __htole64(x)		(__host_is_le() ? ((uint64_t)(x)) : __bswap64(x))

#define __betoh16(x)		(__host_is_be() ? ((uint16_t)(x)) : __bswap16(x))
#define __betoh32(x)		(__host_is_be() ? ((uint32_t)(x)) : __bswap32(x))
#define __betoh64(x)		(__host_is_be() ? ((uint64_t)(x)) : __bswap64(x))
#define __letoh16(x)		(__host_is_le() ? ((uint16_t)(x)) : __bswap16(x))
#define __letoh32(x)		(__host_is_le() ? ((uint32_t)(x)) : __bswap32(x))
#define __letoh64(x)		(__host_is_le() ? ((uint64_t)(x)) : __bswap64(x))


#define __htox16(x, big)	((big) ? __htobe16(x) : __htole16(x))
#define __htox32(x, big)	((big) ? __htobe32(x) : __htole32(x))
#define __htox64(x, big)	((big) ? __htobe64(x) : __htole64(x))

#define __xtoh16(x, big)	((big) ? __betoh16(x) : __letoh16(x))
#define __xtoh32(x, big)	((big) ? __betoh32(x) : __letoh32(x))
#define __xtoh64(x, big)	((big) ? __betoh64(x) : __letoh64(x))

#endif
