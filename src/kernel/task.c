#include <onix/task.h>
#include <onix/printk.h>
#include <onix/memory.h>
#include <onix/assert.h>
#include <onix/interrupt.h>
#include <onix/string.h>
#include <onix/bitmap.h>
#include <onix/syscall.h>
#include <onix/list.h>
#include <onix/global.h>
#include <onix/arena.h>
#include <onix/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define NR_TASKS 64
extern u32 volatile jiffies;
extern u32 jiffy;

extern bitmap_t kernel_map;
extern tss_t tss;
extern void task_switch(task_t *next);

static task_t *task_table[NR_TASKS];    // 任务表
static list_t block_list;               // 任务默认阻塞链表
static list_t sleep_list;               // 任务睡眠链表
static task_t *idle_task;

static task_t *get_free_task()
{
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        if (task_table[i] == NULL)
        {
            task_t *task = (task_t *)alloc_kpage(1); // todo free_kpage
            memset(task, 0, PAGE_SIZE);
            task->pid = i;
            task_table[i] = task;
            return task;
        }
    }
    panic("no more tasks");
}

// 获取进程 id
pid_t sys_getpid()
{
    task_t *task = running_task();
    return task->pid;
}

// 获取父进程 id
pid_t sys_getppid()
{
    task_t *task = running_task();
    return task->ppid;
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
    if (task == NULL && state == TASK_READY)
    {
        task = idle_task;
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

void task_sleep(u32 ms)
{
    assert(!get_interrupt_state()); // 不可中断
    u32 ticks = ms / jiffy;
    ticks = ticks > 0 ? ticks : 1;

    task_t *current = running_task();
    current->ticks = jiffies + ticks;
    list_t *list = &sleep_list;
    list_node_t *anchor = &list->tail;

    for (list_node_t *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next)
    {
        task_t *task = element_entry(task_t, node, ptr);
        if (current->ticks < task->ticks)
        {
            anchor = ptr;
            break;
        }
    }
    assert(current->node.next == NULL);
    assert(current->node.prev == NULL);

    list_insert_before(anchor, &current->node);
    current->state = TASK_SLEEPING;
    schedule();
}

void task_wakeup()
{
    assert(!get_interrupt_state()); // 不可中断
    list_t *list = &sleep_list;
    for (list_node_t *ptr = list->head.next; ptr != &list->tail;)
    {
        task_t *task = element_entry(task_t, node, ptr);
        // jiffies随着时间增加
        if (jiffies < task->ticks)
        {
            break;
        }
        // unblock 会将指针清空
        ptr = ptr->next;
        // task->node.next = NULL;
        // task->node.prev = NULL;
        task->ticks = 0;
        task_unblock(task);
    }
}

void task_activate(task_t *task)
{
    assert(task->magic == ONIX_MAGIC);

    if (task->pde != get_cr3())
    {
        set_cr3(task->pde);
    }

    if (task->uid != KERNEL_USER)
    {
        tss.esp0 = (u32)task + PAGE_SIZE;
    }
}

void schedule()
{
    assert(!get_interrupt_state());
    task_t *current = running_task();
    task_t *next = task_search(TASK_READY);
    assert(next != NULL);
    assert(next ->magic == ONIX_MAGIC);
    if (current->state == TASK_RUNNING)
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
    task_activate(next);
    task_switch(next);
}

static task_t *task_create(target_t target, const char *name, u32 priority, u32 uid)
{
    task_t *task = get_free_task();
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
    task->brk = KERNEL_MEMORY_SIZE;
    return task;
}

// 调用该函数的地方不能有任何局部变量
// 调用前栈顶需要准备足够的空间
void task_to_user_mode(target_t target)
{
    task_t *task = running_task();

    // 创建用户进程虚拟内存位图
    task->vmap = kmalloc(sizeof(bitmap_t));
    void *buf = (void *)alloc_kpage(1);
    bitmap_init(task->vmap, buf, PAGE_SIZE, KERNEL_MEMORY_SIZE / PAGE_SIZE);
    
    task->pde = (u32)copy_pde();
    set_cr3(task->pde);

    u32 addr = (u32)task + PAGE_SIZE;

    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)(addr);

    iframe->vector = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 3;
    iframe->esp_dummy = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;

    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = ONIX_MAGIC;

    iframe->eip = (u32)target;
    iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
    iframe->esp = USER_STACK_TOP;

    asm volatile(
        "movl %0, %%esp\n"
        "jmp interrupt_exit\n" ::"m"(iframe));
}

extern void interrupt_exit();
static void task_build_statck(task_t *task)
{
    u32 addr = (u32)task + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)addr;
    iframe->eax = 0;

    addr -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)addr;
    frame->ebp = 0xaa55aa55;
    frame->ebx = 0xaa55aa55;
    frame->edi = 0xaa55aa55;
    frame->esi = 0xaa55aa55;
    frame->eip = interrupt_exit;
    task->stack = (u32 *)frame;
}

pid_t task_fork()
{
    task_t *task = running_task();
    assert(task->node.next == NULL && task->node.prev == NULL && task->state == TASK_RUNNING);
    task_t *child = get_free_task();
    pid_t pid = child->pid;
    memcpy(child, task, PAGE_SIZE);

    child->pid = pid;
    child->ppid = task->pid;
    child->ticks = child->priority;
    child->state = TASK_READY;

    child->vmap = kmalloc(sizeof(bitmap_t));
    memcpy(child->vmap, task->vmap, sizeof(bitmap_t));

    void *buf = (void *)alloc_kpage(1);
    memcpy(buf, task->vmap->bits, PAGE_SIZE);
    child->vmap->bits = buf;

    child->pde = (u32)copy_pde();

    task_build_statck(child);
    return child->pid;
}

void task_exit(int status)
{
    task_t *task = running_task();
    // 当前进程没有阻塞，且正在执行
    assert(task->node.next == NULL && task->node.prev == NULL && task->state == TASK_RUNNING);

    task->state = TASK_DIED;
    task->status = status;

    free_pde();
    free_kpage((u32)task->vmap->bits, 1);
    kfree(task->vmap);
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        task_t *child = task_table[i];
        if (!child)
        {
            continue;
        }
        if (child->ppid != task->pid)
        {
            continue;
        }
        child->ppid = task->ppid;
    }
    LOGK("task 0x%p exit....\n", task);
    schedule();    
}

static void task_setup()
{
    task_t *task = running_task();
    task->magic = ONIX_MAGIC;
    task->ticks = 1;
    memset(task_table, 0, sizeof(task_table));
}

extern void idle_thread();
extern void init_thread();
extern void test_thread();

void task_init()
{
    list_init(&block_list);
    list_init(&sleep_list);
    task_setup();
    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER);
    task_create(init_thread, "init", 5, NORMAL_USER);
    task_create(test_thread, "test", 5, KERNEL_USER);
}
