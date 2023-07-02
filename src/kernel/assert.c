#include <onix/stdarg.h>
#include <onix/types.h>
#include <onix/stdio.h>
#include <onix/printk.h>
#include <onix/assert.h>

static u8 buf[1024];
static void spin(char *name)
{
  printk("spinning in %s ...\n", name);
  while(true);
}

void assertion_failure(char *expr, char *file, char *base, int line)
{
  printk(
    "\n-->assert(%s) failed!!!\n"
    "--> file: %s\n"
    "--> base: %s\n"
    "--> line: %d\n",
    expr, file, base, line);
    spin("assertion_failure()");
    asm volatile("ud2");
}

void panic(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int i = vsprintf(buf, fmt, args);
  va_end(args);

  printk("!!! panic !!!\n --> %s \n", buf);
  spin("panic()");
  asm volatile("ud2");
}