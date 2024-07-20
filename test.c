#include <stdio.h>
#include <assert.h>
#include "tlsf.h"

#define DEFAULT_USABLE_POOL_SIZE (1 << 10)
#define DEFAULT_POOL_SIZE (DEFAULT_USABLE_POOL_SIZE + POOL_HEADER_OVERHEAD)

void test_initialization_1()
{
    int result = tlsf_init(1);
    assert(result == 0);
    printf("Initialization with size 1B passed.\n");
    tlsf_teardown();
}

void test_initialization_128()
{
    int result = tlsf_init(128);
    assert(result == 0);
    printf("Initialization with size 128B passed.\n");
    tlsf_teardown();
}

void test_initialization_65536()
{
    int result = tlsf_init(65536);
    assert(result == 1);
    printf("Initialization with size 65536B passed.\n");
    tlsf_teardown();
}

void test_initialization_1G()
{
    int result = tlsf_init(1 << 30);
    assert(result == 0);
    printf("Initialization with size 1GB passed.\n");
    tlsf_teardown();
}

void test_basic_allocation()
{
    tlsf_init(DEFAULT_POOL_SIZE);
    void *ptr = tlsf_malloc(64);
    assert(ptr != NULL);
    printf("Basic allocation test passed.\n");
    tlsf_teardown();
}

void test_full_pool_at_once()
{
    tlsf_init(DEFAULT_POOL_SIZE + sizeof(tlsf_block_header_t));
    void *ptr = tlsf_malloc(DEFAULT_USABLE_POOL_SIZE);
    assert(ptr != NULL);
    printf("Full pool allocation at once test passed.\n");
    tlsf_teardown();
}

void test_full_pool_chunked()
{
    tlsf_init(960 + POOL_HEADER_OVERHEAD + 20 * sizeof(tlsf_block_header_t));
    for (int i = 0; i < 20; i++)
    {
        assert(tlsf_malloc(48) != NULL);
    }
    printf("Chunked full pool allocation test passed.\n");
    tlsf_teardown();
}

void test_allocation_and_deallocation()
{
    tlsf_init(DEFAULT_POOL_SIZE);
    void *ptr = tlsf_malloc(128);
    assert(ptr != NULL);
    tlsf_free(ptr);
    void *ptr2 = tlsf_malloc(128);
    assert(ptr2 != NULL);
    printf("Allocation and deallocation test passed.\n");
    tlsf_teardown();
}

void test_multiple_allocations()
{
    tlsf_init(DEFAULT_POOL_SIZE);
    void *ptr1 = tlsf_malloc(32);
    void *ptr2 = tlsf_malloc(64);
    void *ptr3 = tlsf_malloc(128);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr3 != NULL);
    printf("Multiple allocations test passed.\n");
    tlsf_teardown();
}

void test_worst_case()
{
    /* Worst case happens when all free blocks must be iterated in a list.
     * If no lists have a free block beyond the fli and sli of the desired size,
       blocks[initial_fli][initial_sli] must be iterated as a last resort.
    */

    tlsf_init(150 + POOL_HEADER_OVERHEAD + 2 * sizeof(tlsf_block_header_t));
    void *ptr1 = tlsf_malloc(80);
    void *ptr2 = tlsf_malloc(70);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    tlsf_free(ptr1);
    tlsf_free(ptr2);  // 70 bytes will be the head of the list
    void *ptr3 = tlsf_malloc(80);
    assert(ptr3 != NULL);
    printf("Worst case test passed.\n");
    tlsf_teardown();
}

void test_merging()
{
    tlsf_init(192 + POOL_HEADER_OVERHEAD + 2 * sizeof(tlsf_block_header_t));
    void *ptr1 = tlsf_malloc(64);
    void *ptr2 = tlsf_malloc(128);
    tlsf_free(ptr1);
    tlsf_free(ptr2);
    void *ptr3 = tlsf_malloc(192);
    assert(ptr3 != NULL);
    printf("Merging test passed.\n");
    tlsf_teardown();
}

void test_minimum_allocation()
{
    tlsf_init(DEFAULT_POOL_SIZE);
    void *ptr = tlsf_malloc(1);
    assert(ptr != NULL);
    printf("Minimum allocation test passed.\n");
    tlsf_teardown();
}

void test_free_null_pointer()
{
    tlsf_init(DEFAULT_POOL_SIZE);
    tlsf_free(NULL);
    printf("Free NULL pointer test passed.\n");
    tlsf_teardown();
}

void test_double_free()
{
    tlsf_init(DEFAULT_POOL_SIZE);
    void *ptr = tlsf_malloc(64);
    assert(ptr != NULL);
    tlsf_free(ptr);
    tlsf_free(ptr); // Should not cause issues
    printf("Double free test passed.\n");
    tlsf_teardown();
}

int main()
{
    test_initialization_1();
    test_initialization_128();
    test_initialization_65536();
    test_initialization_1G();
    test_basic_allocation();
    test_full_pool_at_once();
    test_full_pool_chunked();
    test_allocation_and_deallocation();
    test_multiple_allocations();
    test_worst_case();
    test_merging();
    test_minimum_allocation();
    test_free_null_pointer();
    test_double_free();
    printf("All tests passed.\n");
    return 0;
}