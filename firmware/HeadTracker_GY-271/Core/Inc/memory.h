#include <stdint.h>
#include <string.h>

// Usage:
// uint16_t value = read_u16_le(buffer + 3);

#define __builtin_bswap8(x) (x) // "hack"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define int_little_to_native(_Value, _Size) (_Value)
#define int_big_to_native(_Value, _Size) __builtin_bswap##_Size(_Value)
#else
#define int_little_to_native(_Value, _Size) __builtin_bswap##_Size(_Value)
#define int_big_to_native(_Value, _Size) (_Value)
#endif

#define DECLARE_READ_VALUE(_Type, _Name, _FixByteOrder, _Size)  \
    static inline _Type _Name (void * const mem) {              \
        _Type value;                                            \
        memcpy(&value, mem, sizeof value);                      \
        _FixByteOrder(value, _Size);                            \
        return value;                                           \
    }

DECLARE_READ_VALUE(int8_t,   read_i8_le,  int_little_to_native, 8)
DECLARE_READ_VALUE(int16_t,  read_i16_le, int_little_to_native, 16)
DECLARE_READ_VALUE(int32_t,  read_i32_le, int_little_to_native, 32)
DECLARE_READ_VALUE(int64_t,  read_i64_le, int_little_to_native, 64)
DECLARE_READ_VALUE(uint8_t,  read_u8_le,  int_little_to_native, 8)
DECLARE_READ_VALUE(uint16_t, read_u16_le, int_little_to_native, 16)
DECLARE_READ_VALUE(uint32_t, read_u32_le, int_little_to_native, 32)
DECLARE_READ_VALUE(uint64_t, read_u64_le, int_little_to_native, 64)

DECLARE_READ_VALUE(int8_t,   read_i8_be,  int_big_to_native, 8)
DECLARE_READ_VALUE(int16_t,  read_i16_be, int_big_to_native, 16)
DECLARE_READ_VALUE(int32_t,  read_i32_be, int_big_to_native, 32)
DECLARE_READ_VALUE(int64_t,  read_i64_be, int_big_to_native, 64)
DECLARE_READ_VALUE(uint8_t,  read_u8_be,  int_big_to_native, 8)
DECLARE_READ_VALUE(uint16_t, read_u16_be, int_big_to_native, 16)
DECLARE_READ_VALUE(uint32_t, read_u32_be, int_big_to_native, 32)
DECLARE_READ_VALUE(uint64_t, read_u64_be, int_big_to_native, 64)

#undef DECLARE_READ_VALUE
#undef __builtin_bswap8
