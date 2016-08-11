#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");

#define MS_TO_NS(x)	(x * 1E6L)

static struct hrtimer hr_timer;

enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
	printk("my_hrtimer_callback called (%ld).\n", jiffies);
	return HRTIMER_NORESTART;
}

int hrtimer_init_module(void)
{
	ktime_t ktime;

	unsigned long delay_in_ms = 200L;

	printk("HR timer module installing\n");

	ktime = ktime_set(0, MS_TO_NS(delay_in_ms));

	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	hr_timer.function = &my_hrtimer_callback;

	printk("Starting timer to fire in %ldms (%ld)\n", delay_in_ms, jiffies);

	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

	return 0;
}


void hrtimer_exit_module(void)
{
	int ret;

	ret = hrtimer_cancel(&hr_timer);
	if (ret) printk("The timer was still in use...\n");

	printk("HR timer module uninstalling\n");

	return;
}

module_init(hrtimer_init_module);
module_exit(hrtimer_exit_module);
