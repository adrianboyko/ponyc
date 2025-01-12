#include <platform.h>

#include <mem/heap.h>
#include <mem/pool.h>
#include <mem/pagemap.h>

#include <gtest/gtest.h>

TEST(Heap, Init)
{
  pony_actor_t* actor = (pony_actor_t*)0xDEADBEEFDEADBEEF;

  heap_t heap;
  ponyint_heap_init(&heap);

  void* p = ponyint_heap_alloc(actor, &heap, 127, TRACK_NO_FINALISERS);
  ASSERT_EQ((size_t)128, heap.used);

  chunk_t* chunk = (chunk_t*)ponyint_pagemap_get(p);
  ASSERT_EQ(actor, ponyint_heap_owner(chunk));

  size_t size = ponyint_heap_size(chunk);
  ASSERT_EQ(size, (size_t)128);

  void* p2 = ponyint_heap_alloc(actor, &heap, 99, TRACK_NO_FINALISERS);
  ASSERT_NE(p, p2);
  ASSERT_EQ((size_t)256, heap.used);

  heap.next_gc = 0;
  ponyint_heap_startgc(&heap);
  ponyint_heap_mark(chunk, p);
  ponyint_heap_endgc(&heap);
  ASSERT_EQ((size_t)128, heap.used);

  void* p3 = ponyint_heap_alloc(actor, &heap, 107, TRACK_NO_FINALISERS);
  ASSERT_EQ(p2, p3);

  ponyint_heap_used(&heap, 1024);
  ASSERT_EQ((size_t)1280, heap.used);

  heap.next_gc = 0;
  ponyint_heap_startgc(&heap);
  ponyint_heap_mark_shallow(chunk, p3);
  ponyint_heap_endgc(&heap);
  ASSERT_EQ((size_t)128, heap.used);

  void* p4 = ponyint_heap_alloc(actor, &heap, 67, TRACK_NO_FINALISERS);
  ASSERT_EQ(p, p4);

  size_t large_size = (1 << 22) - 7;
  void* p5 = ponyint_heap_alloc(actor, &heap, large_size, TRACK_NO_FINALISERS);
  chunk_t* chunk5 = (chunk_t*)ponyint_pagemap_get(p5);
  ASSERT_EQ(actor, ponyint_heap_owner(chunk5));

  size_t adjust_size = ponyint_pool_adjust_size(large_size);
  ASSERT_NE(chunk5, (chunk_t*)NULL);

  char* p5_end = (char*)p5 + adjust_size;
  char* p5_curr = (char*)p5;
  chunk_t* p5_chunk = NULL;

  while(p5_curr < p5_end)
  {
    p5_chunk = (chunk_t*)ponyint_pagemap_get(p5_curr);
    p5_curr += POOL_ALIGN;
    ASSERT_EQ(chunk5, p5_chunk);
  }
  p5_chunk = (chunk_t*)ponyint_pagemap_get(p5_end);
  ASSERT_NE(chunk5, p5_chunk);

  size_t size5 = ponyint_heap_size(chunk5);
  ASSERT_EQ(adjust_size, size5);
  ASSERT_EQ(256 + adjust_size, heap.used);

  heap.next_gc = 0;
  ponyint_heap_startgc(&heap);
  ponyint_heap_mark_shallow(chunk5, p5);
  ponyint_heap_endgc(&heap);
  ASSERT_EQ(adjust_size, heap.used);

  ponyint_heap_destroy(&heap);
}
