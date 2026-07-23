/*
 * net_rw.h
 *
 * All serialized values are encoded in little-endian format.
 *
 * Lightweight low-level binary read/write helpers.
 *
 * Features:
 *   - Little-endian serialization
 *   - Alignment-safe access using memcpy
 *   - Portable fixed-width integer encoding
 *   - IEEE754 float/double serialization
 *   - No hidden state or allocations
 *   - Header-only implementation
 *
 * Supported types:
 *   - u8/u16/u32/u64
 *   - i8/i16/i32/i64
 *   - f32/f64
 *   - raw byte arrays
 *
 * Design:
 *   This library operates on raw memory pointers only.
 *   Buffer management, offsets, bounds checking, and stream
 *   handling are intentionally left to the caller.
 *
 * Serialized format:
 *   All values are encoded in little-endian format.
 *
 * Example:
 *
 *      uint8_t buffer[256];
 *      size_t offset = 0;
 *
 *      write_u32(buffer + offset, 0x12345678);
 *      offset += 4;
 *
 *      write_f32(buffer + offset, 3.14f);
 *      offset += 4;
 *
 * Usage:
 *   Suitable for:
 *      - network packets
 *      - SDR/radio protocols
 *      - binary file formats
 *      - embedded systems
 *      - IPC/shared memory
 *      - custom serialization
 */

#ifndef NET_RW_H
#define NET_RW_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef _MSC_VER
    #define __ORDER_LITTLE_ENDIAN__ 1234
    #define __ORDER_BIG_ENDIAN__    4321
    #define __BYTE_ORDER__          __ORDER_LITTLE_ENDIAN__
    
    #define __builtin_bswap16 _byteswap_ushort
    #define __builtin_bswap32 _byteswap_ulong
    #define __builtin_bswap64 _byteswap_uint64
#endif

// Check for known endian macros
#if !defined(__BYTE_ORDER__) || !defined(__ORDER_BIG_ENDIAN__) || !defined(__ORDER_LITTLE_ENDIAN__)
    #error "Cannot determine endianness. __BYTE_ORDER__ or __ORDER_BIG_ENDIAN__/__ORDER_LITTLE_ENDIAN__ not defined."
#endif

/* Type size safety */
_Static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
_Static_assert(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
_Static_assert(sizeof(int32_t) == 4, "int32_t must be 4 bytes");
_Static_assert(sizeof(int64_t) == 8, "int64_t must be 8 bytes");
_Static_assert(sizeof(float) == 4, "float must be 4 bytes");
_Static_assert(sizeof(double) == 8, "double must be 8 bytes");

static inline void write_u8(uint8_t *p, uint8_t v)
{
	p[0] = v;
}

static inline void write_u16(uint8_t *p, uint16_t v)
{
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap16(v);
	#endif
	
    memcpy(p, &v, sizeof(v));
}

static inline void write_u32(uint8_t *p, uint32_t v)
{
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap32(v);
	#endif
	
	memcpy(p, &v, sizeof(v));
}

static inline void write_u64(uint8_t *p, uint64_t v)
{
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap64(v);
	#endif
	
	memcpy(p, &v, sizeof(v));
}

static inline void write_i8(uint8_t *p, int8_t v)
{
	p[0] = v;
}

static inline void write_i16(uint8_t *p, int16_t v)
{
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap16(v);
	#endif
	
    memcpy(p, &v, sizeof(v));
}

static inline void write_i32(uint8_t *p, int32_t v)
{
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap32(v);
	#endif
	
	memcpy(p, &v, sizeof(v));
}

static inline void write_i64(uint8_t *p, int64_t v)
{
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap64(v);
	#endif
	
	memcpy(p, &v, sizeof(v));
}

static inline void write_f32(uint8_t *p, float v)
{
	uint32_t u;

	memcpy(&u, &v, sizeof(u));

	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		u = __builtin_bswap32(u);
	#endif

	memcpy(p, &u, sizeof(u));
}

static inline void write_f64(uint8_t *p, double v)
{
	uint64_t u;

	memcpy(&u, &v, sizeof(u));

	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		u = __builtin_bswap64(u);
	#endif

	memcpy(p, &u, sizeof(u));
}

static inline void write_bytes(uint8_t *dst, const void *src, uint32_t len)
{
    memcpy(dst, src, len);
} 

static inline void write_raw(uint8_t *dst, const void *src, uint32_t len)
{
    memcpy(dst, src, len);
}

static inline void write_zero(uint8_t *dst, size_t len) 
{
	memset(dst, 0, len);
}

static inline uint8_t read_u8(const uint8_t *p) {
	return p[0];
}

static inline uint16_t read_u16(const uint8_t *p) {
	uint16_t v;
	
	memcpy(&v, p, sizeof(v));
	
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap16(v);
	#endif
	
	return v;
}

static inline uint32_t read_u32(const uint8_t *p) {
	uint32_t v;
	memcpy(&v, p, sizeof(v));
	
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap32(v);
	#endif
	
	return v;
}

static inline uint64_t read_u64(const uint8_t *p) {
	uint64_t v;
	memcpy(&v, p, sizeof(v));
	
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap64(v);
	#endif
	
	return v;
}

static inline int8_t read_i8(const uint8_t *p) {
	return p[0];
}

static inline int16_t read_i16(const uint8_t *p) {
	int16_t v;
	
	memcpy(&v, p, sizeof(v));
	
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap16(v);
	#endif
	
	return v;
}

static inline int32_t read_i32(const uint8_t *p) {
	int32_t v;
	memcpy(&v, p, sizeof(v));
	
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap32(v);
	#endif
	
	return v;
}

static inline int64_t read_i64(const uint8_t *p) {
	int64_t v;
	memcpy(&v, p, sizeof(v));
	
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		v = __builtin_bswap64(v);
	#endif
	
	return v;
}

static inline float read_f32(const uint8_t *p)
{
	uint32_t u;
	float v;

	memcpy(&u, p, sizeof(u));

	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		u = __builtin_bswap32(u);
	#endif

	memcpy(&v, &u, sizeof(v));

	return v;
}

static inline double read_f64(const uint8_t *p)
{
	uint64_t u;
	double v;

	memcpy(&u, p, sizeof(u));

	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		u = __builtin_bswap64(u);
	#endif

	memcpy(&v, &u, sizeof(v));

	return v;
}

static inline void read_bytes(void *dst, const uint8_t *src, uint32_t len)
{
    memcpy(dst, src, len);
}

static inline void read_raw(void *dst, const uint8_t *src, uint32_t len)
{
    memcpy(dst, src, len);
}

#endif /* NET_RW_H */ 
