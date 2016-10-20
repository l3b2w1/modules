#include <linux/kernel.h>
void kernel_speak(char *words)
{
    printk(KERN_DEBUG "%s", words);
}
