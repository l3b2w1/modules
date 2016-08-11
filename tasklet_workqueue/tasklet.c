#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");

char my_tasklet_data[] = "my tasklet function was called";

void my_tasklet_func(unsigned long data)
{
	printk("%s\n", (char *)data);
	return;
}

DECLARE_TASKLET(my_tasklet, my_tasklet_func, (unsigned long)&my_tasklet_data);

int tasklet_init_module(void)
{
	printk("tasklet_schedule(&my_tasklet)\n");
	tasklet_schedule(&my_tasklet);
	return 0;
}

void tasklet_cleanup_module(void)
{
	printk("tasklet_kill(&my_tasklet)\n");
	/* stop the tasklet before we exit */
	tasklet_kill(&my_tasklet);
	return;
}

module_init(tasklet_init_module);
module_exit(tasklet_cleanup_module);
