#ifndef TLSF_H
#define TLSF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int tlsf_size_t;
typedef unsigned int tlsf_word_t;
typedef void* tlsf_addr_t;

#define bool int
#define true 1
#define false 0

#ifndef NULL
#define NULL ((void*)0)
#endif

#define CHK_BIT(word, bit) ((word) & (1 << (bit)))
#define SET_BIT(word, bit) ((word) |= (1 << (bit)))
#define CLR_BIT(word, bit) ((word) &= ~(1 << (bit)))

#define LOCAL static
#define LOCAL_INLINE static inline __attribute__((always_inline))

#if __SIZE_WIDTH__ == 64
#define SIZE_ALIGN_SHIFT 3
#define FLI_OFFSET 6
#else
#define SIZE_ALIGN_SHIFT 2
#define FLI_OFFSET 5
#endif

#define SIZE_ALIGN_MASK (~((1 << SIZE_ALIGN_SHIFT) - 1))

#define FLI_COUNT 16

#define SLI_COUNT 16
#define SLI_COUNT_LOG2 4

#define MIN_BLOCK_SIZE (1 << FLI_OFFSET)
#define MAX_BLOCK_SIZE (1 << (FLI_COUNT + FLI_OFFSET) - 1)

#define MIN_POOL_SIZE (MIN_BLOCK_SIZE + sizeof(tlsf_pool_t))
#define MAX_POOL_SIZE (MAX_BLOCK_SIZE + sizeof(tlsf_pool_t))

#define BLOCK_SIZE(fli, sli) ((1 << ((fli) + FLI_OFFSET)) + ((sli) << (fli + FLI_OFFSET - 1)))

#define POOL_DEFAULT_ADDR ((tlsf_addr_t)0x10000)  // Pool's starting address for freestanding environments

#define POOL_HEADER_OVERHEAD (sizeof(tlsf_pool_t))

typedef struct tlsf_block_header_t
{
    /* Size of this block (including header) */
    tlsf_size_t size;

    /* Previous physical block */
    struct tlsf_block_header_t* prev_phys;

    /* Previous/next free blocks */
    struct tlsf_block_header_t* prev_free;
    struct tlsf_block_header_t* next_free;
} tlsf_block_header_t;

typedef struct
{
    tlsf_size_t size;

    tlsf_word_t fl_bitmap;
    tlsf_word_t sl_bitmap[FLI_COUNT];

    tlsf_block_header_t* blocks[FLI_COUNT][SLI_COUNT];
} tlsf_pool_t;

bool tlsf_init(tlsf_size_t pool_size);

tlsf_addr_t tlsf_malloc(tlsf_size_t size);

void tlsf_free(tlsf_addr_t ptr);

void tlsf_teardown(void);

#ifdef __cplusplus
}
#endif

#endif