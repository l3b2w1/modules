#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");

static struct workqueue_struct *my_wq;

/*
typedef void (*work_func_t)(struct work_struct *work);

struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
};
*/

typedef struct {
	struct work_struct my_work;
	int x;
} my_work_t;

static my_work_t *work, *work2;
static struct work_struct work3;

static void my_wq_func(struct work_struct *work)
{
	my_work_t *mywork = (my_work_t *)work;
	printk("my_work.x %d\n", mywork->x);
	kfree((void *)work);
	return;
}

static void sys_work_handler(struct work_struct *work)
{
	printk("I am here\n");
}

static int __init wq_init(void)
{
	int ret = 0;
	printk("create my workqueue\n");
	my_wq = create_workqueue("my_queuehello");
	if (!my_wq) {
		printk("create workqueue failed\n");
		return -ENOMEM;
	}

	work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);

	if (work) {
		INIT_WORK((struct work_struct *)work, my_wq_func);
		work->x = 1;

		ret = queue_work(my_wq, (struct work_struct *)work);
	}

	work2 = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
	if (work2) {
		INIT_WORK((struct work_struct *)work2, my_wq_func);
		work->x = 2;

		ret = queue_work(my_wq, (struct work_struct *)work2);
	}

	INIT_WORK(&work3, sys_work_handler);
	schedule_work(&work3);

	return 0;
}

static void __exit wq_exit(void)
{
	printk("flush and destroy my_wq\n");
	flush_workqueue(my_wq);
	destroy_workqueue(my_wq);
	return;
}

module_init(wq_init);
module_exit(wq_exit);
