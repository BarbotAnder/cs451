#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/errno.h>
#else
#include <errno.h>
#endif

#include "lab.h"

#define handle_error_and_die(msg) \
    do                            \
    {                             \
        perror(msg);              \
        raise(SIGKILL);           \
    } while (0)

/**
 * @brief Convert bytes to the correct K value
 *
 * @param bytes the number of bytes
 * @return size_t the K value that will fit bytes
 */
size_t btok(size_t bytes)
{
    size_t total = bytes + sizeof(struct avail);
    size_t k = SMALLEST_K;
    while ((UINT64_C(1) << k) < total && k < MAX_K)
        k++;
    return k;
}

struct avail *buddy_calc(struct buddy_pool *pool, struct avail *block)
{
    uintptr_t offset = (uintptr_t)block - (uintptr_t)pool->base;
    uintptr_t buddy_offset = offset ^ (UINT64_C(1) << block->kval);
    return (struct avail *)((uint8_t *)pool->base + buddy_offset);
}

void *buddy_malloc(struct buddy_pool *pool, size_t size)
{
    if (pool == NULL || size == 0)
        return NULL;

    // get the kval for the requested size with enough room for the tag and kval fields
    size_t k = btok(size);
    size_t i = k;

    // R1 Find a block
    while (i <= pool->kval_m && pool->avail[i].next == &pool->avail[i])
        i++;

    // There was not enough memory to satisfy the request thus we need to set error and return NULL
    if (i > pool->kval_m)
    {
        errno = ENOMEM;
        return NULL;
    }

    // R2 Remove from list;
    struct avail *block = pool->avail[i].next;
    block->prev->next = block->next;
    block->next->prev = block->prev;

    // R3 Split required?
    while (i > k)
    {
        i--;
        uintptr_t buddy_addr = (uintptr_t)block + (UINT64_C(1) << i);
        struct avail *buddy = (struct avail *)buddy_addr;
        buddy->tag = BLOCK_AVAIL;
        buddy->kval = i;
        buddy->next = pool->avail[i].next;
        buddy->prev = &pool->avail[i];
        pool->avail[i].next->prev = buddy;
        pool->avail[i].next = buddy;
        block->kval = i;
    }

    // R4 Split the block
    block->tag = BLOCK_RESERVED;
    return (void *)((uint8_t *)block + sizeof(struct avail));
}

void buddy_free(struct buddy_pool *pool, void *ptr)
{
    if (!pool || !ptr)
        return;

    struct avail *block = (struct avail *)((uint8_t *)ptr - sizeof(struct avail));
    size_t k = block->kval;
    struct avail *buddy;

    while (k < pool->kval_m)
    {
        buddy = buddy_calc(pool, block);

        if (buddy->tag != BLOCK_AVAIL || buddy->kval != k)
            break;

        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;

        if ((void *)buddy < (void *)block)
            block = buddy;

        k++;
        block->kval = k;
    }

    block->tag = BLOCK_AVAIL;
    block->next = pool->avail[k].next;
    block->prev = &pool->avail[k];
    pool->avail[k].next->prev = block;
    pool->avail[k].next = block;
}

/**
 * @brief This is a simple version of realloc.
 *
 * @param poolThe memory pool
 * @param ptr  The user memory
 * @param size the new size requested
 * @return void* pointer to the new user memory
 */
void *buddy_realloc(struct buddy_pool *pool, void *ptr, size_t size)
{
    // not properly tested, may not work
    if (!ptr)
        return buddy_malloc(pool, size);
    if (size == 0)
    {
        buddy_free(pool, ptr);
        return NULL;
    }

    struct avail *old_block = (struct avail *)((uint8_t *)ptr - sizeof(struct avail));
    size_t old_size = UINT64_C(1) << old_block->kval;
    size_t copy_size = old_size - sizeof(struct avail);

    void *new_ptr = buddy_malloc(pool, size);
    if (new_ptr)
        memcpy(new_ptr, ptr, size < copy_size ? size : copy_size);

    buddy_free(pool, ptr);
    return new_ptr;
}

void buddy_init(struct buddy_pool *pool, size_t size)
{
    size_t kval = 0;
    if (size == 0)
        kval = DEFAULT_K;
    else
        kval = btok(size);

    if (kval < MIN_K)
        kval = MIN_K;
    if (kval > MAX_K)
        kval = MAX_K - 1;

    // make sure pool struct is cleared out
    memset(pool, 0, sizeof(struct buddy_pool));
    pool->kval_m = kval;
    pool->numbytes = (UINT64_C(1) << pool->kval_m);
    // Memory map a block of raw memory to manage
    pool->base = mmap(
        NULL,                        /*addr to map to*/
        pool->numbytes,              /*length*/
        PROT_READ | PROT_WRITE,      /*prot*/
        MAP_PRIVATE | MAP_ANONYMOUS, /*flags*/
        -1,                          /*fd -1 when using MAP_ANONYMOUS*/
        0                            /* offset 0 when using MAP_ANONYMOUS*/
    );
    if (MAP_FAILED == pool->base)
    {
        handle_error_and_die("buddy_init avail array mmap failed");
    }

    // Set all blocks to empty. We are using circular lists so the first elements just point
    // to an available block. Thus the tag, and kval feild are unused burning a small bit of
    // memory but making the code more readable. We mark these blocks as UNUSED to aid in debugging.
    for (size_t i = 0; i <= kval; i++)
    {
        pool->avail[i].next = pool->avail[i].prev = &pool->avail[i];
        pool->avail[i].kval = i;
        pool->avail[i].tag = BLOCK_UNUSED;
    }

    // Add in the first block
    pool->avail[kval].next = pool->avail[kval].prev = (struct avail *)pool->base;
    struct avail *m = pool->avail[kval].next;
    m->tag = BLOCK_AVAIL;
    m->kval = kval;
    m->next = m->prev = &pool->avail[kval];
}

void buddy_destroy(struct buddy_pool *pool)
{
    int rval = munmap(pool->base, pool->numbytes);
    if (-1 == rval)
    {
        handle_error_and_die("buddy_destroy avail array");
    }
    // Zero out the array so it can be reused it needed
    memset(pool, 0, sizeof(struct buddy_pool));
}

// brief main for testing
int myMain(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    struct buddy_pool pool;
    buddy_init(&pool, 0);

    void *ptr1 = buddy_malloc(&pool, 1024);
    void *ptr2 = buddy_malloc(&pool, 2048);

    printf("Allocated ptr1: %p\n", ptr1);
    printf("Allocated ptr2: %p\n", ptr2);

    buddy_free(&pool, ptr1);
    buddy_free(&pool, ptr2);

    buddy_destroy(&pool);
    printf("Memory pool destroyed successfully.\n");

    return 0;
}

// idk what this is
#define UNUSED(x) (void)x

/**
 * This function can be useful to visualize the bits in a block. This can
 * help when figuring out the buddy_calc function!
 */
static void printb(unsigned long int b)
{
    size_t bits = sizeof(b) * 8;
    unsigned long int curr = UINT64_C(1) << (bits - 1);
    for (size_t i = 0; i < bits; i++)
    {
        if (b & curr)
        {
            printf("1");
        }
        else
        {
            printf("0");
        }
        curr >>= 1L;
    }
}
