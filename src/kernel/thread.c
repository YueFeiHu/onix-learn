#include <onix/interrupt.h>
#include <onix/syscall.h>
#include <onix/debug.h>
#include <onix/task.h>
#include <onix/mutex.h>
#include <onix/printk.h>
#include <onix/task.h>
#include <onix/stdio.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

mutex_t mutex;

void idle_thread()
{
    set_interrupt_state(true);
    u32 counter = 0;
    while (true)
    {
        // LOGK("idle task.... %d\n", counter++);
        asm volatile(
            "sti\n" // 开中断
            "hlt\n" // 关闭 CPU，进入暂停状态，等待外中断的到来
        );
        yield(); // 放弃执行权，调度执行其他任务
    }
}

extern u32 keyboard_read(char *buf, u32 count);

static void user_init_thread()
{
    u32 counter = 0;
    char ch;
    while (true)
    {
        sleep(100);
        // printf("task is in user mode %d\n", counter++);
    }
}

void init_thread()
{
    // set_interrupt_state(true);
    char temp[100]; // 为栈顶有足够的空间
    task_to_user_mode(user_init_thread);
}

void test_thread()
{
    u32 counter = 0;
    while (true)
    {
        while (true)
        {
            // LOGK("test task %d....\n", counter++);
            void *ptr = kmalloc(1200);
            LOGK("kmalloc 0x%p....\n", ptr);
            kfree(ptr);

            ptr = kmalloc(1024);
            LOGK("kmalloc 0x%p....\n", ptr);
            kfree(ptr);

            ptr = kmalloc(54);
            LOGK("kmalloc 0x%p....\n", ptr);
            kfree(ptr);

            sleep(5000);
        }
    }
}