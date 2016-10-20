#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/signal.h>

MODULE_LICENSE("GPL");
static struct task_struct *thread;

static int thread_func(void *unused)
{
	allow_signal(SIGKILL);
	while (!kthread_should_stop()) {
		printk(KERN_INFO "func thread running\n");
		ssleep(3);
	}
	printk(KERN_INFO "func Thread stopping\n");
	do_exit(0);

	return 0;
}

static int __init minit_thread(void)
{
	printk(KERN_INFO "Creating thread\n");


	thread = kthread_run(thread_func, NULL, "mythead");
	if (thread)
		printk(KERN_INFO "thread created successfully\n");
	else
		printk(KERN_INFO "thread created failed\n");

	return 0;
}

static void __exit mexit_thread(void)
{
	printk(KERN_INFO "cleaning up\n");
	if (thread) {
		kthread_stop(thread);
		printk(KERN_INFO "exit thead stopped\n");
	}
}

module_init(minit_thread);
module_exit(mexit_thread);
