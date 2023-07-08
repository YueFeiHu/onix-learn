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

void kernel_init()
{
    
    memory_map_init();
    mapping_init();
    interrupt_init();
    // clock_init();
    // time_init();
    // rtc_init();
    // task_init();
    char *ptr = (char *)(0x100000 * 20);
    ptr[0] = 'a';
    asm volatile("sti");
    hang();
}
