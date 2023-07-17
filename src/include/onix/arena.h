#ifndef ONIX_ARENA_H
#define ONIX_ARENA_H

#include <onix/types.h>
#include <onix/list.h>

#define DESC_COUNT 7
typedef list_node_t block_t;

typedef struct arena_descriptor_t
{
  u32 total_block;  // 一页分成了多少块
  u32 block_size;   // 块大小
  list_t free_list;
} arena_descriptor_t;

typedef struct arena_t
{
  arena_descriptor_t *desc;
  u32 count;
  u32 large;
  u32 magic;
} arena_t;

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif