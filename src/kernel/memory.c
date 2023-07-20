#include <onix/memory.h>
#include <onix/types.h>
#include <onix/debug.h>
#include <onix/assert.h>
#include <onix/stdlib.h>
#include <onix/string.h>
#include <onix/bitmap.h>
#include <onix/multiboot2.h>
#include <onix/task.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define ZONE_VALID 1    // ards 可用区域
#define ZONE_RESERVED 2 // ards 不可用区域

#define IDX(addr) ((u32)addr >> 12)
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff) // 获得页目录索引
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff) // 获得页目表索引
#define PAGE(idx) ((u32)idx << 12)
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

#define PDE_MASK 0xFFC00000

static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
};
#define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))
#define KERNEL_MAP_BITS 0x4000

bitmap_t kernel_map;

typedef struct ards_t
{
  u64 base;
  u64 size;
  u32 type;
} _packed ards_t;

static u32 memory_base = 0;
static u32 memory_size = 0;
static u32 total_pages = 0;
static u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

void memory_init(u32 magic, u32 addr)
{
  u32 count = 0;
  if (magic == ONIX_MAGIC)
  {
    count = *(u32 *)addr;
    ards_t *ptr = (ards_t *)(addr + 4);
    for (size_t i = 0; i < count; i++, ptr++)
    {
      LOGK("memory base 0x%p, size 0x%p, type %d\n", (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
      if (ptr->type == ZONE_VALID && ptr->size > memory_size)
      {
        memory_base = (u32)ptr->base;
        memory_size = (u32)ptr->size;
      }
    }
  }
  else if (magic == MULTIBOOT2_MAGIC)
  {
    u32 size = *(unsigned int *)addr;
    multi_tag_t *tag = (multi_tag_t *)(addr + 8);

    LOGK("Announced mbi size 0x%x\n", size);
    while (tag->type != MULTIBOOT_TAG_TYPE_END)
    {
      if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
        break;
      // 下一个 tag 对齐到了 8 字节
      tag = (multi_tag_t *)((u32)tag + ((tag->size + 7) & ~7));
    }

    multi_tag_mmap_t *mtag = (multi_tag_mmap_t *)tag;
    multi_mmap_entry_t *entry = mtag->entries;
    while ((u32)entry < (u32)tag + tag->size)
    {
      LOGK("Memory base 0x%p size 0x%p type %d\n",
           (u32)entry->addr, (u32)entry->len, (u32)entry->type);
      count++;
      if (entry->type == ZONE_VALID && entry->len > memory_size)
      {
        memory_base = (u32)entry->addr;
        memory_size = (u32)entry->len;
      }
      entry = (multi_mmap_entry_t *)((u32)entry + mtag->entry_size);
    }
  }
  else
  {
    panic("memory init magic unknown 0x%p\n", magic);
  }

  LOGK("ARDS count %d\n", count);
  LOGK("memory base 0x%p\n", (u32)memory_base);
  LOGK("memory size 0x%p\n", (u32)memory_size);

  assert(memory_base == MEMORY_BASE); // 内存开始的位置为 1M
  assert((memory_size & 0xfff) == 0); // 要求按页对齐

  total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
  free_pages = IDX(memory_size);

  LOGK("Total pages %d\n", total_pages);
  LOGK("Free pages %d\n", free_pages);
}

static u32 start_page = 0;
static u8 *memory_map;
static u32 memory_map_pages;

void memory_map_init()
{
  memory_map = (u8 *)memory_base;
  // 计算物理内存数组需要占用的页数
  memory_map_pages = div_round_up(total_pages, PAGE_SIZE);
  LOGK("memory map page count %d\n", memory_map_pages);
  free_pages -= memory_map_pages;
  memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);
  start_page = IDX(MEMORY_BASE) + memory_map_pages;
  for (size_t i = 0; i < start_page; i++)
  {
    memory_map[i] = 1;
  }
  LOGK("total pages %d free pages %d\n", total_pages, free_pages);

  u32 length = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8;
  bitmap_init(&kernel_map, (u8 *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
  bitmap_scan(&kernel_map, memory_map_pages);
}

static u32 get_page()
{
  for (size_t i = start_page; i < total_pages; i++)
  {
    if (!memory_map[i])
    {
      memory_map[i] = 1;
      free_pages--;
      assert(free_pages >= 0);
      u32 page = PAGE(i);
      LOGK("get page 0x%p\n", page);
      return page;
    }
  }
  panic("OOM");
}

static void put_page(u32 addr)
{
  ASSERT_PAGE(addr);
  u32 idx = IDX(addr);

  assert(idx >= start_page && idx < total_pages);
  assert(memory_map[idx] >= 1);
  memory_map[idx]--;
  if (!memory_map[idx])
  {
    free_pages++;
  }
  assert(free_pages > 0 && free_pages < total_pages);
  LOGK("put pages 0x%p\n", addr);
}

void memory_test()
{
  u32 pages[10];
  for (size_t i = 0; i < 10; i++)
  {
    pages[i] = get_page();
  }
  for (size_t i = 0; i < 10; i++)
  {
    put_page(pages[i]);
  }
}

u32 get_cr3()
{
  asm volatile("movl %cr3, %eax\n");
}

u32 get_cr2()
{
  asm volatile("movl %cr2, %eax\n");
}

void set_cr3(u32 pde)
{
  ASSERT_PAGE(pde);
  asm volatile("movl %%eax, %%cr3\n"
               :
               : "a"(pde));
}

// 将 cr0 寄存器最高位 PE 置为 1，启用分页
static inline void enable_page()
{
  // 0b1000_0000_0000_0000_0000_0000_0000_0000
  // 0x80000000
  asm volatile(
      "movl %cr0, %eax\n"
      "orl $0x80000000, %eax\n"
      "movl %eax, %cr0\n");
}

// 初始化页表项
static void entry_init(page_entry_t *entry, u32 index)
{
  *(u32 *)entry = 0;
  entry->present = 1;
  entry->write = 1;
  entry->user = 1;
  entry->index = index;
}

void mapping_init()
{
  page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
  memset(pde, 0, PAGE_SIZE);
  idx_t index = 0;
  for (idx_t didx = 0; didx < sizeof(KERNEL_PAGE_TABLE) / 4; didx++)
  {
    page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
    memset(pte, 0, PAGE_SIZE);
    page_entry_t *dentry = &pde[didx];
    entry_init(dentry, IDX((u32)pte));
    for (idx_t tidx = 0; tidx < 1024; tidx++, index++)
    {
      if (index == 0)
      {
        continue;
      }
      page_entry_t *tentry = &pte[tidx];
      entry_init(tentry, index);
      memory_map[index] = 1;
    }
  }
  page_entry_t *entry = &pde[1023];
  entry_init(entry, IDX(KERNEL_PAGE_DIR));
  set_cr3((u32)pde);
  enable_page();
}

static page_entry_t *get_pde()
{
  return (page_entry_t *)(0xfffff000);
}

static page_entry_t *get_pte(u32 vaddr, bool create)
{
  page_entry_t *pde = get_pde();
  u32 idx = DIDX(vaddr);
  page_entry_t *entry = &pde[idx];

  assert(create || (!create && entry->present));

  // 0xffc为页目录，0x14为页面索引，其内容为0x5
  page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

  if (!entry->present)
  {
    LOGK("Get and create page table entry for 0x%p\n", vaddr);
    // 为页表申请内存
    u32 page = get_page();
    // 关联页表
    entry_init(entry, IDX(page));
    memset(table, 0, PAGE_SIZE);
  }

  return table;
}

static void flush_tlb(u32 vaddr)
{
  asm volatile("invlpg (%0)"
               :
               : "r"(vaddr)
               : "memory");
}

void mapping_test()
{
  BMB;

  // 将 20 M 0x1400000 内存映射到 64M 0x4000000 的位置

  // 我们还需要一个页表，0x900000

  u32 vaddr = 0x4000000; // 线性地址几乎可以是任意的
  u32 paddr = 0x1400000; // 物理地址必须要确定存在
  u32 table = 0x900000;  // 页表也必须是物理地址

  page_entry_t *pde = get_pde();
  page_entry_t *dentry = &pde[DIDX(vaddr)];
  entry_init(dentry, IDX(table)); // 指向页表的物理地址

  page_entry_t *pte = get_pte(vaddr, true);
  page_entry_t *tentry = &pte[TIDX(vaddr)];
  entry_init(tentry, IDX(paddr));

  BMB;
  char *ptr = (char *)(vaddr);
  ptr[0] = 'a';
  flush_tlb(vaddr);
  BMB;
}

static u32 scan_page(bitmap_t *map, u32 count)
{
  assert(count > 0);
  u32 index = bitmap_scan(map, count);
  if (index == EOF)
  {
    panic("Scan page fail");
  }
  u32 addr = PAGE(index);
  LOGK("Scan page 0x%p count %d\n", addr, count);
  return addr;
}

static void reset_page(bitmap_t *map, u32 addr, u32 count)
{
  ASSERT_PAGE(addr);
  assert(count > 0);

  u32 index = IDX(addr);

  for (size_t i = 0; i < count; i++)
  {
    assert(bitmap_test(map, index + i));
    bitmap_set(map, index + i, 0);
  }
}

u32 alloc_kpage(u32 count)
{
  assert(count > 0);
  u32 vaddr = scan_page(&kernel_map, count);
  LOGK("ALLOC kernel pages 0x%p count %d\n", vaddr, count);
  return vaddr;
}

void free_kpage(u32 vaddr, u32 count)
{
  ASSERT_PAGE(vaddr);
  assert(count > 0);
  reset_page(&kernel_map, vaddr, count);
  LOGK("FREE  kernel pages 0x%p count %d\n", vaddr, count);
}

void memory_alloc_test()
{
  u32 *pages = (u32 *)(0x200000);
  u32 count = 0x6fe;
  for (size_t i = 0; i < count; i++)
  {
    pages[i] = alloc_kpage(1);
    LOGK("0x%x\n", i);
  }

  for (size_t i = 0; i < count; i++)
  {
    free_kpage(pages[i], 1);
  }
}

// 将 vaddr 映射物理内存
void link_page(u32 vaddr)
{
  ASSERT_PAGE(vaddr);

  page_entry_t *pte = get_pte(vaddr, true);
  page_entry_t *entry = &pte[TIDX(vaddr)];

  task_t *task = running_task();
  bitmap_t *map = task->vmap;
  u32 index = IDX(vaddr);

  // 如果页面已存在，则直接返回
  if (entry->present)
  {
    assert(bitmap_test(map, index));
    return;
  }

  assert(!bitmap_test(map, index));
  bitmap_set(map, index, true);

  u32 paddr = get_page();
  entry_init(entry, IDX(paddr));
  flush_tlb(vaddr);

  LOGK("LINK from 0x%p to 0x%p\n", vaddr, paddr);
}

// 去掉 vaddr 对应的物理内存映射
void unlink_page(u32 vaddr)
{
  ASSERT_PAGE(vaddr);

  page_entry_t *pte = get_pte(vaddr, true);
  page_entry_t *entry = &pte[TIDX(vaddr)];

  task_t *task = running_task();
  bitmap_t *map = task->vmap;
  u32 index = IDX(vaddr);

  if (!entry->present)
  {
    assert(!bitmap_test(map, index));
    return;
  }

  assert(entry->present && bitmap_test(map, index));

  entry->present = false;
  bitmap_set(map, index, false);

  u32 paddr = PAGE(entry->index);

  DEBUGK("UNLINK from 0x%p to 0x%p\n", vaddr, paddr);
  put_page(paddr);
  flush_tlb(vaddr);
}

static u32 copy_page(void *page)
{
  u32 paddr = get_page();
  page_entry_t *entry = get_pte(0, false);
  entry_init(entry, IDX(paddr));
  memcpy((void *)0, (void *)page, PAGE_SIZE);
  entry->present = false;
  return paddr;
}

page_entry_t *copy_pde()
{
  task_t *task = running_task();
  page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
  memcpy(pde, (void *)task->pde, PAGE_SIZE);

  page_entry_t *entry = &pde[1023];
  entry_init(entry, IDX(pde));

  page_entry_t *dentry;
  for (size_t didx = 2; didx < 1023; didx++)
  {
    dentry = &pde[didx];
    if (!dentry->present)
    {
      continue;
    }
    page_entry_t *pte = (page_entry_t *)(PDE_MASK | PAGE(didx));
    for (size_t tidx = 0; tidx < 1024; tidx++)
    {
      entry = &pte[tidx];
      if (!entry->present)
      {
        continue;
      }
      assert(memory_map[entry->index] > 0);
      entry->write = false;
      memory_map[entry->index]++;
      assert(memory_map[entry->index] < 255);
    }
    u32 paddr = copy_page(pte);
    dentry->index = IDX(paddr);
  }
  set_cr3(task->pde);
  return pde;
}

int32 sys_brk(void *addr)
{
    LOGK("task brk 0x%p\n", addr);
    u32 brk = (u32)addr;
    ASSERT_PAGE(brk);

    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    assert(KERNEL_MEMORY_SIZE < brk < USER_STACK_BOTTOM);

    u32 old_brk = task->brk;

    if (old_brk > brk)
    {
        for (; brk < old_brk; brk += PAGE_SIZE)
        {
            unlink_page(brk);
        }
    }
    else if (IDX(brk - old_brk) > free_pages)
    {
        // out of memory
        return -1;
    }

    task->brk = brk;
    return 0;
}

typedef struct page_error_code_t
{
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 reserved0 : 1;
    u8 fetch : 1;
    u8 protection : 1;
    u8 shadow : 1;
    u16 reserved1 : 8;
    u8 sgx : 1;
    u16 reserved2;
} _packed page_error_code_t;

void page_fault(
    u32 vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);
    u32 vaddr = get_cr2();
    LOGK("fault address 0x%p\n", vaddr);

    page_error_code_t *code = (page_error_code_t *)&error;
    task_t *task = running_task();

    assert(KERNEL_MEMORY_SIZE <= vaddr < USER_STACK_TOP);

    if (code->present)
    {
      assert(code->write);
      page_entry_t *pte = get_pte(vaddr, false);
      page_entry_t *entry = &pte[TIDX(vaddr)];

      assert(entry->present);
      assert(memory_map[entry->index] > 0);

      if (memory_map[entry->index] == 1)
      {
        entry->write = true;
        LOGK("WRITE page for 0x%p\n", vaddr);
      }
      else
      {
        void *page = (void *)PAGE(IDX(vaddr));
        u32 paddr = copy_page(page);
        memory_map[entry->index]--;
        entry_init(entry, IDX(paddr));
        flush_tlb(vaddr);
        LOGK("COPY page for 0x%p\n",vaddr);
      }
      return;
    }
    

    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM))
    {
        u32 page = PAGE(IDX(vaddr));
        link_page(page);
        // BMB;
        return;
    }

    panic("page fault!!!");
}