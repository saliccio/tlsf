#include "tlsf.h"

/* Check whether the underlying environment is POSIX or freestanding */
#if __STDC_HOSTED__ == 1
    #include <string.h>
    #include <stdlib.h>

    #define GET_POOL_ADDR(size) malloc(size)
    #define RETURN_POOL_ADDR(addr) free(addr)
#else
    #define GET_POOL_ADDR(size) POOL_DEFAULT_ADDR
    #define RETURN_POOL_ADDR(addr)

    /* Memset function for freestanding environments. */
    LOCAL_INLINE void* memset(void* ptr, int value, tlsf_size_t num)
    {
        unsigned char *p = ptr;
        while (num--)
        {
            *p++ = (unsigned char)value;
        }
        return ptr;
    }
#endif

#define MEMSET memset

LOCAL tlsf_pool_t *pool;

/* TODO: Architecture-independent msb-lsb finding */

/* Find the most significant bit of given word. */
LOCAL_INLINE int find_msb(tlsf_word_t word)
{
    int bit = word ? 32 - __builtin_clz(word) : 0;
	return bit - 1;
}

/* Find the least significant bit of given word. */
LOCAL_INLINE int find_lsb(tlsf_word_t word)
{
    return __builtin_ffs(word) - 1;
}

/* Align given size by SIZE_ALIGN_SHIFT.
   If given size is less than MIN_BLOCK_SIZE, sets it to MIN_BLOCK_SIZE. */
LOCAL_INLINE void align_size(tlsf_size_t *size)
{
    *size = ((*size) + (1 << SIZE_ALIGN_SHIFT) - 1) & ~((1 << SIZE_ALIGN_SHIFT) - 1);
    *size = *size < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : *size;
}

/* Since size of header is always aligned by 2 or 3 bits, LSB of size is used for indicating whether the block is free */

/* Check if freeness flag of the block is set. */
LOCAL_INLINE bool is_block_free(tlsf_block_header_t *block)
{
    return CHK_BIT(block->size, 0);
}

/* Set the freeness flag of the block. */
LOCAL_INLINE void set_free_flag(tlsf_block_header_t *block)
{
    SET_BIT(block->size, 0);
}

/* Clear the freeness flag of the block. */
LOCAL_INLINE void clear_free_flag(tlsf_block_header_t *block)
{
    CLR_BIT(block->size, 0);
}

/* Get flag-free difference of given sizes (size1 - size2). */
LOCAL_INLINE bool get_size_difference(tlsf_size_t size1, tlsf_size_t size2)
{
    int diff = (size1 - size2);
    CLR_BIT(diff, 0);  // clear flag
    return diff;
}

/* Check if given block is the last of the pool. */
LOCAL_INLINE bool is_block_last(tlsf_block_header_t *block)
{
    return ((tlsf_addr_t) block + block->size) >= ((tlsf_addr_t) pool + pool->size);
}

/* Does the list of fli and sli include any free blocks? */
LOCAL_INLINE bool is_list_free(int fli, int sli)
{
    return (CHK_BIT(pool->fl_bitmap, fli) && CHK_BIT(pool->sl_bitmap[fli], sli));
}

/* Find the first and second level indexes suitable for given size. */
LOCAL_INLINE void find_indexes(tlsf_size_t size, int *fli, int *sli)
{
    *fli = find_msb(size);
    *sli = (size >> (*fli - SLI_COUNT_LOG2));
    CLR_BIT(*sli, find_msb(*sli));
    *fli -= FLI_OFFSET;
}

/* Fill header fields. */
LOCAL_INLINE void make_header(tlsf_block_header_t *header, tlsf_size_t size, tlsf_block_header_t *prev_phys)
{
    header->size = size;
    header->prev_phys = prev_phys;
    header->next_free = NULL;
    header->prev_free = NULL;
}

/* Insert the block to its corresponding list (as the first element), set its free flag and bitmap bits. */
LOCAL_INLINE void add_block_to_pool(tlsf_block_header_t* block)
{
    int fli, sli;
    find_indexes(block->size, &fli, &sli);

    tlsf_block_header_t *first_block = pool->blocks[fli][sli];
    pool->blocks[fli][sli] = block;
    block->prev_free = NULL;
    block->next_free = first_block;

    if (NULL != first_block)
    {
        first_block->prev_free = block;
    }

    set_free_flag(block);

    SET_BIT(pool->fl_bitmap, fli);
    SET_BIT(pool->sl_bitmap[fli], sli);
}

/* Initialize the pool, insert an initial block stretching the full size. */
bool tlsf_init(tlsf_size_t pool_size)
{
    align_size(&pool_size);

    if (pool_size < MIN_POOL_SIZE || pool_size > MAX_POOL_SIZE)
    {
        return false;
    }

    pool = GET_POOL_ADDR(pool_size);

    MEMSET(pool, 0, pool_size);

    pool->size = pool_size;

    tlsf_size_t actual_size = pool_size - sizeof(tlsf_pool_t);

    int max_size_fli, max_size_sli;
    find_indexes(actual_size, &max_size_fli, &max_size_sli);

    tlsf_block_header_t *initial_block = (tlsf_block_header_t*)((tlsf_addr_t) pool + sizeof(tlsf_pool_t));
    make_header(initial_block, actual_size, NULL);

    add_block_to_pool(initial_block);

    return true;
}

/* Search a free block greater or equal than given size. */
LOCAL_INLINE tlsf_block_header_t* locate_free_block(tlsf_size_t size)
{
    if (!pool->fl_bitmap)
    {
        return NULL;
    }

    int initial_fli, initial_sli;
    find_indexes(size, &initial_fli, &initial_sli);

    if (initial_fli >= FLI_COUNT || initial_sli >= SLI_COUNT)
    {
        return NULL;
    }

    tlsf_block_header_t *candidate_block = pool->blocks[initial_fli][initial_sli];

    if (is_list_free(initial_fli, initial_sli) && get_size_difference(candidate_block->size, size) >= 0)
    {
        return candidate_block;
    }

    int fli = find_lsb(pool->fl_bitmap & ~((1 << initial_fli) - 1));
    int sli = initial_sli;

    if (fli > initial_fli)
    {
        sli = 0;
    }
    else
    {
        sli++;
        if (sli >= SLI_COUNT)
        {
            sli = 0;
            fli++;
        }
    }
    
    while (fli < FLI_COUNT)
    {
        sli = find_lsb(pool->sl_bitmap[fli] & ~((1 << sli) - 1));

        candidate_block = pool->blocks[fli][sli];
        if (is_list_free(fli, sli) && get_size_difference(candidate_block->size, size) >= 0)
        {
            return candidate_block;
        }

        fli++;
    }

    candidate_block = pool->blocks[initial_fli][initial_sli];
    while (NULL != candidate_block)  // worst case
    {
        if (get_size_difference(candidate_block->size, size) >= 0)
        {
            return candidate_block;
        }

        candidate_block = candidate_block->next_free;
    }

    return NULL;
}

/* Delete the block from the corresponding list, clear its free flag and the bitmaps if necessary. */
LOCAL_INLINE void remove_block_from_pool(tlsf_block_header_t* block)
{
    int fli, sli;
    find_indexes(block->size, &fli, &sli);

    if (NULL == block->prev_free)
    {
        pool->blocks[fli][sli] = block->next_free;

        if (NULL != block->next_free)
        {
            block->next_free->prev_free = NULL;
        }
        else  // only element in the list
        {
            CLR_BIT(pool->sl_bitmap[fli], sli);
            if (0 == pool->sl_bitmap[fli])
            {
                CLR_BIT(pool->fl_bitmap, fli);
            }
        }
    }
    else
    {
        block->prev_free->next_free = block->next_free;
        block->next_free->prev_free = block->prev_free;
    }
    
    block->next_free = NULL;
    block->prev_free = NULL;

    clear_free_flag(block);
}

/* Flag-free addition of block sizes. */
LOCAL_INLINE void add_block_sizes(tlsf_size_t* dest, tlsf_size_t* src)
{
    *dest = *dest + (*src & SIZE_ALIGN_MASK);
}

/* Merge given block with its previous and next physical blocks, if they are free. */
LOCAL_INLINE void merge_block(tlsf_block_header_t *block)
{
    if (NULL != block->prev_phys && is_block_free(block->prev_phys))
    {
        add_block_sizes(&block->size, &block->prev_phys->size);
        remove_block_from_pool(block->prev_phys);

        block->prev_phys = block->prev_phys->prev_phys;
    }

    if (!is_block_last(block))
    {
        tlsf_block_header_t *next_phys = (tlsf_addr_t) block + block->size;
        if (is_block_free(next_phys))
        {
            add_block_sizes(&block->size, &next_phys->size);
            remove_block_from_pool(next_phys);
        }
    }
}

/* Split given block and return the part with desired size. */
LOCAL_INLINE tlsf_block_header_t* split_block(tlsf_block_header_t *block, tlsf_size_t size)
{
    remove_block_from_pool(block);

    if (get_size_difference(block->size, size) < MIN_BLOCK_SIZE)
    {
        return block;
    }

    tlsf_size_t residual_size =  get_size_difference(block->size, size);
    int fli, sli;
    find_indexes(residual_size, &fli, &sli);
    block->size = residual_size;

    add_block_to_pool(block);

    tlsf_block_header_t *returned_block = (tlsf_addr_t) block + residual_size;
    make_header(returned_block, size, block);

    return returned_block;
}

/* Allocate memory at least as large as given size.
 * Returns NULL if desired size is greater than pool size,
 * or no suitable block is found. */
tlsf_addr_t tlsf_malloc(tlsf_size_t size)
{
    if (size > MAX_POOL_SIZE)
    {
        return NULL;
    }

    size += sizeof(tlsf_block_header_t);
    align_size(&size);

    tlsf_block_header_t *block = locate_free_block(size);
    if (NULL == block)
    {
        return NULL;
    }

    tlsf_block_header_t *fit_block = split_block(block, size);
    clear_free_flag(fit_block);

    return (tlsf_addr_t) fit_block + sizeof(tlsf_block_header_t);
}

/* Free the allocated memory starting at ptr.
 * Directly returns if ptr is NULL. */
void tlsf_free(tlsf_addr_t ptr)
{
    if (NULL == ptr)
    {
        return;
    }

    tlsf_block_header_t *block = ptr - sizeof(tlsf_block_header_t);
    merge_block(block);
    add_block_to_pool(block);
}

/* Deallocate the resources of the pool. */
void tlsf_teardown(void)
{
    RETURN_POOL_ADDR(pool);
    pool = NULL;
}