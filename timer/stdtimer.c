#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");

static int num = 0;
static struct timer_list my_timer;

void my_timer_callback(unsigned long data)
{
	int num = *(int*)data;
	int result = 0;
	printk("my_timer_callback called %d times\n", ++num);
	printk("my_timer_callback called (%ld)\n", jiffies);
	result = mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
	*(int*)data = num;
	if (result) printk("Error in mod_timer\n");
}

int timer_init_module(void)
{
	printk("Timer module installing\n");

	setup_timer(&my_timer, my_timer_callback, (unsigned long)&num);

	printk("Starting timer to fire in 5000ms (%ld)\n", jiffies);

	/* both ok */
#if 1
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
#else
	my_timer.expires = jiffies + msecs_to_jiffies(5000);
	add_timer(&my_timer); 
#endif
	return 0;
}

void timer_exit_module(void)
{
	int ret;
	
	ret = del_timer(&my_timer);

	if (ret) printk("The timer is still in use...\n");

	printk("Timer module uninstalling\n");

	return;
}

module_init(timer_init_module);
module_exit(timer_exit_module);

