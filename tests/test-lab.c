#include <assert.h>
#include <stdlib.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include "harness/unity.h"
#include "../src/lab.h"

// really didn't see a need for either of these.
void setUp(void)
{
  //  size_t size = UINT64_C(1) << MIN_K;
  //  buddy_init(&pool, size);
}

void tearDown(void)
{
  //  buddy_destroy(&pool);
}

/**
 * Check the pool to ensure it is full.
 */
void check_buddy_pool_full(struct buddy_pool *pool)
{
  // A full pool should have all values 0-(kval-1) as empty
  for (size_t i = 0; i < pool->kval_m; i++)
  {
    assert(pool->avail[i].next == &pool->avail[i]);
    assert(pool->avail[i].prev == &pool->avail[i]);
    assert(pool->avail[i].tag == BLOCK_UNUSED);
    assert(pool->avail[i].kval == i);
  }

  // The avail array at kval should have the base block
  assert(pool->avail[pool->kval_m].next->tag == BLOCK_AVAIL);
  assert(pool->avail[pool->kval_m].next->next == &pool->avail[pool->kval_m]);
  assert(pool->avail[pool->kval_m].prev->prev == &pool->avail[pool->kval_m]);

  // Check to make sure the base address points to the starting pool
  // If this fails either buddy_init is wrong or we have corrupted the
  // buddy_pool struct.
  assert(pool->avail[pool->kval_m].next == pool->base);
}

/**
 * Check the pool to ensure it is empty.
 */
void check_buddy_pool_empty(struct buddy_pool *pool)
{
  // An empty pool should have all values 0-(kval) as empty
  for (size_t i = 0; i <= pool->kval_m; i++)
  {
    struct avail *head = &pool->avail[i];
    if (head->next != head || head->prev != head)
    {
      fprintf(stderr, "K=%zu list broken: head=%p next=%p prev=%p\n", (size_t)i, (void *)head, (void *)head->next, (void *)head->prev);
    }
    assert(head->next == head);
    assert(head->prev == head);
    assert(head->tag == BLOCK_UNUSED);
    assert(head->kval == i);
  }
}

/**
 * Test allocating 1 byte to make sure we split the blocks all the way down
 * to MIN_K size. Then free the block and ensure we end up with a full
 * memory pool again
 */
void test_buddy_malloc_one_byte(void)
{
  fprintf(stderr, "->Test allocating and freeing 1 byte\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem = buddy_malloc(&pool, 1);
  // Make sure correct kval was allocated
  buddy_free(&pool, mem);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests the allocation of one massive block that should consume the entire memory
 * pool and makes sure that after the pool is empty we correctly fail subsequent calls.
 */
void test_buddy_malloc_one_large(void)
{
  fprintf(stderr, "->Testing size that will consume entire memory pool\n");
  struct buddy_pool pool;
  size_t bytes = UINT64_C(1) << MIN_K;
  buddy_init(&pool, bytes);

  // Ask for an exact K value to be allocated. This test makes assumptions on
  // the internal details of buddy_init.
  size_t ask = bytes - sizeof(struct avail);
  void *mem = buddy_malloc(&pool, ask);
  assert(mem != NULL);

  // Move the pointer back and make sure we got what we expected
  struct avail *tmp = (struct avail *)mem - 1;
  assert(tmp->kval == MIN_K);
  assert(tmp->tag == BLOCK_RESERVED);
  check_buddy_pool_empty(&pool);

  // Verify that a call on an empty tool fails as expected and errno is set to ENOMEM.
  void *fail = buddy_malloc(&pool, 5);
  assert(fail == NULL);
  assert(errno == ENOMEM);

  // Free the memory and then check to make sure everything is OK
  buddy_free(&pool, mem);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests to make sure that the struct buddy_pool is correct and all fields
 * have been properly set kval_m, avail[kval_m], and base pointer after a
 * call to init
 */
void test_buddy_init(void)
{
  fprintf(stderr, "->Testing buddy init\n");
  // Loop through all kval MIN_k-DEFAULT_K and make sure we get the correct amount allocated.
  // We will check all the pointer offsets to ensure the pool is all configured correctly
  for (size_t i = MIN_K; i <= DEFAULT_K; i++)
  {
    size_t size = UINT64_C(1) << i;
    struct buddy_pool pool;
    buddy_init(&pool, size);
    check_buddy_pool_full(&pool);
    buddy_destroy(&pool);
  }
}

/**
 * Test free() and ensure the blocks coalesce
 */
void test_buddy_free(void)
{
  fprintf(stderr, "->Test coalescing of buddy blocks\n");

  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << (MIN_K+1));

  // Allocate
  void *ptr1 = buddy_malloc(&pool, 21);
  void *ptr2 = buddy_malloc(&pool, 21);

  if (ptr1 != NULL || ptr2 != NULL)
  {
    fprintf(stderr, "Memory allocation failed: ptr1=%p, ptr2=%p\n", ptr1, ptr2);
  }
  assert(ptr1 != NULL && ptr2 != NULL); // just in case (it passed upon creating the if() and failure print)

  DBG_PRINT("Before freeing:\n");
  for (size_t i = 0; i <= pool.kval_m; i++)
  {
    struct avail *head = &pool.avail[i];
    DBG_PRINT("KVAL %zu: head=%p, next=%p, prev=%p, tag=%d\n", (size_t)i, (void *)head, (void *)head->next, (void *)head->prev, head->tag);
    struct avail *curr = head->next;
    while (curr != head)
    {
      DBG_PRINT("  Block: %p, tag=%d, kval=%zu\n", (void *)curr, curr->tag, (size_t)curr->kval);
      curr = curr->next;
    }
  }

  // free
  buddy_free(&pool, ptr1);
  buddy_free(&pool, ptr2);

  DBG_PRINT("after freeing:\n");
  for (size_t i = 0; i <= pool.kval_m; i++)
  {
    struct avail *head = &pool.avail[i];
    DBG_PRINT("KVAL %zu: head=%p, next=%p, prev=%p, tag=%d\n", (size_t)i, (void *)head, (void *)head->next, (void *)head->prev, head->tag);
    struct avail *curr = head->next;
    while (curr != head)
    {
      DBG_PRINT("  Block: %p, tag=%d, kval=%zu\n", (void *)curr, curr->tag, (size_t)curr->kval);
      curr = curr->next;
    }
  }

  // Check that the pool is full and blocks have coalesced
  check_buddy_pool_full(&pool);

  buddy_destroy(&pool);
}

/*
 * Tests repeated free() calls
 */
void test_buddy_free_double(void)
{
  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << MIN_K);

  void *ptr = buddy_malloc(&pool, 64);
  buddy_free(&pool, ptr);
  buddy_free(&pool, ptr); // Should handle gracefully without crashing

  buddy_destroy(&pool);
}

/**
 * Tests the allocation of a block that consumes most of the memory pool
 */
void test_buddy_malloc_almost_full_block(void)
{
  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << MIN_K);

  size_t size = (UINT64_C(1) << MIN_K) - sizeof(struct avail) - 1;
  void *ptr = buddy_malloc(&pool, size);
  assert(ptr != NULL);

  buddy_free(&pool, ptr);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests multiple small allocations
 */
void test_buddy_malloc_multiple(void)
{
  fprintf(stderr, "->Test allocating multiple small blocks\n");
  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << (MIN_K+3));

  void *ptrs[4];
  for (int i = 0; i < 4; ++i)
  {
    ptrs[i] = buddy_malloc(&pool, 32);

    //debug printing
    if (ptrs[i] != NULL)
    {
      fprintf(stderr, "Memory allocation failed for block %d\n", i);
      DBG_PRINT("Pool state at failure:\n");
      for (size_t i = 0; i <= pool.kval_m; i++)
      {
        struct avail *head = &pool.avail[i];
        DBG_PRINT("KVAL %zu: head=%p, next=%p, prev=%p, tag=%d\n", (size_t)i, (void *)head, (void *)head->next, (void *)head->prev, head->tag);
        struct avail *curr = head->next;
        while (curr != head)
        {
          DBG_PRINT("  Block: %p, tag=%d, kval=%zu\n", (void *)curr, curr->tag, (size_t)curr->kval);
          curr = curr->next;
        }
      }
    }
    assert(ptrs[i] != NULL);
  }

  for (int i = 0; i < 4; ++i)
    buddy_free(&pool, ptrs[i]);

  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Test erroneous malloc (null pool or zero size)
 */
void test_buddy_malloc_invalid_inputs(void)
{
  fprintf(stderr, "->Test invalid input handling\n");
  void *null_pool_result = buddy_malloc(NULL, 128);
  assert(null_pool_result == NULL);

  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << MIN_K);
  void *zero_size_result = buddy_malloc(&pool, 0);
  assert(zero_size_result == NULL);
  buddy_destroy(&pool);
}

/**
 * Test realloc logic, shrink and grow
 */
void test_buddy_realloc_behavior(void)
{
  fprintf(stderr, "->Test realloc behavior\n");

  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << MIN_K);
  DBG_PRINT("Buddy pool initialized with size: %lu\n", (UINT64_C(1) << MIN_K));

  int *ptr = buddy_malloc(&pool, 64);
  if (ptr == NULL)
  {
    fprintf(stderr, "Failed to allocate initial memory block\n");
  }
  else
  {
    DBG_PRINT("Allocated initial memory block at address: %p, size: 64\n", ptr);
  }
  assert(ptr != NULL);

  // Fill the memory
  for (int i = 0; i < 16; ++i)
  {
    ptr[i] = i;
    DBG_PRINT("Writing %d at ptr[%d]\n", i, i);
  }

  int *bigger = buddy_realloc(&pool, ptr, 256);
  if (bigger == NULL)
  {
    fprintf(stderr, "Failed to realloc memory block %p to size 256\n", bigger);
  }
  else
  {
    DBG_PRINT("Reallocated memory block at address: %p to new size: 256\n", bigger);
  }
  assert(bigger != NULL);

  // Verify memory
  for (int i = 0; i < 16; ++i)
  {
    assert(bigger[i] == i);
    DBG_PRINT("Verifying bigger[%d] = %d\n", i, bigger[i]);
  }

  int *smaller = buddy_realloc(&pool, bigger, 32);
  if (smaller == NULL)
  {
    fprintf(stderr, "Failed to realloc memory block to size 32\n");
  }
  else
  {
    DBG_PRINT("Reallocated memory block at address: %p to new size: 32\n", smaller);
  }
  assert(smaller != NULL);

  // Verify memory
  for (int i = 0; i < 16; ++i)
  {
    assert(smaller[i] == i);
    DBG_PRINT("Verifying smaller[%d] = %d\n", i, smaller[i]);
  }

  buddy_free(&pool, smaller);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

void test_buddy_multiple_realloc(void)
{
  struct buddy_pool pool;
  buddy_init(&pool, UINT64_C(1) << MIN_K);

  int *ptr = buddy_malloc(&pool, 64);
  assert(ptr != NULL);

  // Grow
  ptr = buddy_realloc(&pool, ptr, 128);
  assert(ptr != NULL);

  // Shrink
  ptr = buddy_realloc(&pool, ptr, 32);
  assert(ptr != NULL);

  buddy_free(&pool, ptr);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

int main(void)
{
  printf("Running myMain\n");
  myMain(0, NULL);

  time_t t;
  unsigned seed = (unsigned)time(&t);
  fprintf(stderr, "Random seed:%d\n", seed);
  srand(seed);
  printf("Running memory tests.\n");

  UNITY_BEGIN();
  RUN_TEST(test_buddy_init);
  RUN_TEST(test_buddy_free);
  RUN_TEST(test_buddy_free_double);
  RUN_TEST(test_buddy_malloc_one_byte);
  RUN_TEST(test_buddy_malloc_one_large);
  RUN_TEST(test_buddy_malloc_multiple);
  RUN_TEST(test_buddy_malloc_almost_full_block);
  RUN_TEST(test_buddy_malloc_invalid_inputs);
  //RUN_TEST(test_buddy_realloc_behavior);
  //RUN_TEST(test_buddy_multiple_realloc);
  return UNITY_END();
}
