#include <linux/kernel.h>
#include <linux/module.h>

extern void kernel_speak(char *words);
int init_module(void)
{
    kernel_speak("initializaion");
    printk(KERN_INFO "Hello world - this is the kernel speaking\n");
    return 0;
}
