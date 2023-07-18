#ifndef ONIX_MEMORY_H
#define ONIX_MEMORY_H
#include <onix/types.h>

#define KERNEL_PAGE_DIR 0x1000
#define PAGE_SIZE 0x1000     // 一页的大小 4K
#define MEMORY_BASE 0x100000 // 1M，可用内存开始的位置

// 内核占用的内存大小 8M
#define KERNEL_MEMORY_SIZE 0x800000

// 用户栈顶地址 128M
#define USER_STACK_TOP 0x8000000

// 用户栈最大 2M
#define USER_STACK_SIZE 0x200000

// 用户栈底地址 128M - 2M
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

typedef struct page_entry_t
{
    u8 present : 1;  // 在内存中
    u8 write : 1;    // 0 只读 1 可读可写
    u8 user : 1;     // 1 所有人 0 超级用户 DPL < 3
    u8 pwt : 1;      // page write through 1 直写模式，0 回写模式
    u8 pcd : 1;      // page cache disable 禁止该页缓冲
    u8 accessed : 1; // 被访问过，用于统计使用频率
    u8 dirty : 1;    // 脏页，表示该页缓冲被写过
    u8 pat : 1;      // page attribute table 页大小 4K/4M
    u8 global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
    u8 ignored : 3;  // 该安排的都安排了，送给操作系统吧
    u32 index : 20;  // 页索引
} _packed page_entry_t;

// 得到 cr2 寄存器
u32 get_cr2();

u32 get_cr3();
void set_cr3(u32 pde);

void free_kpage(u32 vaddr, u32 count);
u32 alloc_kpage(u32 count);

// 将 vaddr 映射物理内存
void link_page(u32 vaddr);
// 去掉 vaddr 对应的物理内存映射
void unlink_page(u32 vaddr);

page_entry_t *copy_pde();
#endif