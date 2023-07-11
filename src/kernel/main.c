#include <onix/debug.h>
#include <onix/types.h>
#include <onix/interrupt.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

extern void console_init();
extern void gdt_init();
extern void interrupt_init();
extern void clock_init();
extern void time_init();
extern void rtc_init();
extern void hang();
extern void set_alarm(u32);
extern void memory_map_init();
extern void memory_test();
extern void mapping_init();
extern void mapping_test();
extern void bitmap_tests();
extern void memory_alloc_test();
extern void syscall_init();
void intr_test()
{
    bool intr = interrupt_disable();

    // do something

    set_interrupt_state(intr);
}


void kernel_init()
{
    
    memory_map_init();
    mapping_init();
    interrupt_init();
    clock_init();
    // time_init();
    // rtc_init();
    // task_init();
    // bitmap_tests();
    // memory_alloc_test();
    // mapping_test();
    // asm volatile("sti");
    syscall_init();
    // set_interrupt_state(true);
}
