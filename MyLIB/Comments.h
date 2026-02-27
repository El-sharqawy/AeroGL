#ifndef __COMMENTS_H__
#define __COMMENTS_H__

#include <stdio.h>
#include <stdint.h>

// 8-bit (1 Byte)
int8_t  i8 = -1;          // 0xFF -- to print cast it to unsigned (uint8_t) 0x%02x
uint8_t u8 = 255;         // 0xFF -- to print 0x%02x

// 16-bit (2 Bytes)
int16_t  i16 = -32768;     // 0x8000 -- to print cast it to unsigned (uint16_t) 0x%04x
uint16_t u16 = 65535;      // 0xFFFF -- to print 0x%04x

// 32-bit (4 Bytes)
int32_t  i32 = -2147483648;          // 0x80000000 -- to print cast it to unsigned (uint32_t) 0x%08x
uint32_t u32 = 2166136261U;          // 0x811c9dc5 -- to print 0x%08x

// 64-bit (8 Bytes)
int64_t  i64 = -9223372036854775807LL;	// 0x8000000000000001 -- to print cast it to unsigned (uint64_t) 0x%016x
uint64_t u64 = 18446744073709551615ULL; // 0xFFFFFFFFFFFFFFFF

static inline void print_types()
{
    printf("8-bit Hex: 0x%02x\n", (uint8_t)i8);
    printf("8-bit Hex: 0x%02x\n", u8);
    printf("16-bit Hex: 0x%04x\n", (uint16_t)i16);
    printf("16-bit Hex: 0x%04x\n", u16);
    printf("32-bit Hex: 0x%08x\n", (uint32_t)i32);
    printf("32-bit Hex: 0x%04x\n", u32);
    printf("64-bit Hex: 0x%016llx\n", (uint64_t)i64);
    printf("64-bit Hex: 0x%016llx\n", u64);
}

#endif // 