/********************************************************************************
* File Name:	down_read.c
* Description:	第8章实例训练          
* Reference book:《Linux内核API完全参考手册》邱铁，周玉，邓莹莹 ，机械工业出版社.2010.9  
* E_mail:openlinux2100@gmail.com			                
*
********************************************************************************/

#include <linux/rwsem.h>
#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/delay.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL"); 
static int __init down_read_init(void); 
static void __exit down_read_exit(void);

struct rw_semaphore rwsem ;

struct task_struct *rthread, *wthread;

unsigned long number = 0;

int thread_read_func(void * argc)
{
    while (!kthread_should_stop()) {
        down_read( &rwsem );   //写者获取读写信号量
        printk("[%s %d] NO.%ld reading number %lu\n", __func__, __LINE__, rwsem.count, number); 
        msleep(1000);
        up_read( &rwsem );     	//读者释放读写信号量
    }
    printk("[%s %d] read thread now exit\n", __func__, __LINE__); 
    return 0;
}

int thread_write_func(void * argc)
{
    while (!kthread_should_stop()) {
        down_write( &rwsem );   //写者获取读写信号量
        number++;
        printk("[%s %d] NO.%ld write done number %lu\n", __func__, __LINE__, rwsem.count, number); 
        up_write(&rwsem);
        msleep(500);
    }
    printk("[%s %d] write thread now exit\n", __func__, __LINE__); 
    return 0;
}

int __init down_read_init(void) 
{	 
	init_rwsem( &rwsem );   	//读写信号量初始化
	printk("<0>after init_rwsem, count: %ld\n",rwsem.count);

    rthread = kthread_run(thread_read_func, NULL, "readerthread");	
    if (!IS_ERR(rthread))
        printk("[%s %d] Thread Reader create successfully\n", __func__, __LINE__); 

    wthread = kthread_run(thread_write_func, NULL, "writterthread");	
    if (!IS_ERR(wthread))
        printk("[%s %d] Thread Writter create successfully\n", __func__, __LINE__); 

	return 0;
}

void __exit down_read_exit(void) 
{ 
    kthread_stop(wthread);
    kthread_stop(rthread);
	printk("<0>exit!\n");
}

module_init(down_read_init); 
module_exit(down_read_exit);
