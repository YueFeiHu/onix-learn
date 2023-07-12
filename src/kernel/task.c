#include <onix/task.h>
#include <onix/printk.h>
#include <onix/memory.h>
#include <onix/assert.h>
#include <onix/interrupt.h>
#include <onix/string.h>
#include <onix/bitmap.h>
#include <onix/syscall.h>
#include <onix/list.h>

extern bitmap_t kernel_map;
extern void task_switch(task_t *next);

#define NR_TASKS 64
static task_t *task_table[NR_TASKS];    // 任务表
static list_t block_list;               // 任务默认阻塞链表

static task_t *get_free_task()
{
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        if (task_table[i] == NULL)
        {
            task_table[i] = (task_t *)alloc_kpage(1);
            return task_table[i];
        }
    }
    panic("no more tasks");
}

static task_t *task_search(task_state_t state)
{
    assert(!get_interrupt_state());
    task_t *task = NULL;
    task_t *current = running_task();
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        task_t *ptr = task_table[i];
        if (ptr == NULL || ptr->state != state || current == ptr)
        {
            continue;
        }
        if (task == NULL || task->ticks < ptr->ticks || ptr->jiffies < task->jiffies)
            task = ptr;
    }
    return task;
}

void task_yield()
{
    schedule();
}

task_t *running_task()
{
    asm volatile(
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n");
}

void task_block(task_t *task, list_t *blist, task_state_t state)
{
    assert(!get_interrupt_state());
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);
    if (blist == NULL)
    {
        blist = &block_list;
    }
    list_push(blist, &task->node);
    assert(state != TASK_READY && state != TASK_RUNNING);
    task->state = state;
    task_t *current = running_task();
    if (current == task)
    {
        schedule();
    }
}

void task_unblock(task_t *task)
{
    assert(!get_interrupt_state());
    list_remove(&task->node);
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);
    task->state = TASK_READY;
}

void schedule()
{
    assert(!get_interrupt_state());
    task_t *current = running_task();
    task_t *next = task_search(TASK_READY);
    assert(next != NULL);
    assert(next ->magic == ONIX_MAGIC);
    if (current->state = TASK_RUNNING)
    {
        current->state = TASK_READY;
    }
    if (!current->ticks)
    {
        current->ticks = current->priority;
    }
    next->state = TASK_RUNNING;
    if (next == current)
    {
        return;
    }
    task_switch(next);
}



static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid)
{
    task_t *task = get_free_task();
    memset(task, 0, PAGE_SIZE);
    u32 stack = (u32)task + PAGE_SIZE;

    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;

    strcpy((char *)task->name, name);
    task->stack = (u32 *)stack;
    task->priority = priority;
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernel_map;
    task->pde = KERNEL_PAGE_DIR;
    task->magic = ONIX_MAGIC;

    return task;
}
static void task_setup()
{
    task_t *task = running_task();
    task->magic = ONIX_MAGIC;
    task->ticks = 1;
    memset(task_table, 0, sizeof(task_table));
}

u32  thread_a()
{
    set_interrupt_state(true);
    while (true)
    {
        printk("A");
        test();
    }
}

u32  thread_b()
{
    set_interrupt_state(true);
    while (true)
    {
        printk("B");
        test();
    }
}

u32  thread_c()
{
    set_interrupt_state(true);
    while (true)
    {
        printk("C");
        test();
    }
}

void task_init()
{
    list_init(&block_list);
    task_setup();
    task_create(thread_a, "a", 5, KERNEL_USER);
    task_create(thread_b, "b", 5, KERNEL_USER);
    // task_create(thread_c, "c", 5, KERNEL_USER);
}
