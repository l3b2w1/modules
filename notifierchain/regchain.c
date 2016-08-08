#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");

extern int register_test_notifier(struct notifier_block *);
extern int unregister_test_notifier(struct notifier_block *);

static int test_event1(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("In event 1: Event number is %lu\n", event);
	return 0;
}

static int test_event2(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("In event 2: Event number is %lu\n", event);
	return 0;
}


static int test_event3(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("In event 3: Event number is %lu\n", event);
	return 0;
}

static struct notifier_block test_notifier1 = {
	.notifier_call = test_event1,
};

static struct notifier_block test_notifier2 = {
	.notifier_call = test_event2,
};

static struct notifier_block test_notifier3 = {
	.notifier_call = test_event3,
};

static int __init reg_notifier(void)
{
	int err = 0;
	printk("Begin to register\n");

	err = register_test_notifier(&test_notifier1);
	if (err) {
		printk("register test_notifier1 error\n");
		return -1;
	}
	printk("register test_notifier1 completely\n");

	err = register_test_notifier(&test_notifier2);
	if (err) {
		printk("register test_notifier2 error\n");
		return -1;
	}
	printk("register test_notifier2 completely\n");

	err = register_test_notifier(&test_notifier3);
	if (err) {
		printk("register test_notifier3 error\n");
		return -1;
	}
	printk("register test_notifier3 completely\n");

	return err;
}

static void __exit unregister_notifier(void)
{
	printk("Begin to unregister\n");
	unregister_test_notifier(&test_notifier1);
	unregister_test_notifier(&test_notifier2);
	unregister_test_notifier(&test_notifier3);
	printk("unregister finished\n");
}

module_init(reg_notifier);
module_exit(unregister_notifier);





























