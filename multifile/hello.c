#include <linux/kernel.h>
#include <linux/module.h>
void show(void)
{
    printk(KERN_INFO "Hello world\n");
}
