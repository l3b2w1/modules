#include <linux/kernel.h>
#include <linux/module.h>
extern void kernel_speak(char *);
void cleanup_module()
{
    kernel_speak("finalization");
    printk(KERN_INFO "Short is the life of a kernel module");
}
