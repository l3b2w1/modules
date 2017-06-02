/*
 * NOTE: This example is works on x86 and powerpc.
 * Here's a sample kernel module showing the use of kprobes to dump a
 * stack trace and selected registers when do_fork() is called.
 *
 * For more information on theory of operation of kprobes, see
 * Documentation/kprobes.txt
 *
 * You will see the trace data in /var/log/messages and on the console
 * whenever do_fork() is invoked to create a new process.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/wireless.h>
#include <net/iw_handler.h>
#include <linux/netdevice.h>

#define JPROBE_DEFINE(func)        \
{                                  \
    .entry = func##_jentry,         \
    .kp = {                        \
        .symbol_name = #func   \
    },                             \
}

static long do_fork_jentry(unsigned long clone_flags, unsigned long stack_start,
	      struct pt_regs *regs, unsigned long stack_size,
	      int __user *parent_tidptr, int __user *child_tidptr)
{
	printk(KERN_INFO "jprobe: clone_flags = 0x%lx, stack_size = 0x%lx,"
			" regs = 0x%p\n",
	       clone_flags, stack_size, regs);

	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	return 0;
}

int ioctl_private_call_jentry(struct net_device *dev, struct iwreq *iwr,
        unsigned int cmd, struct iw_request_info *info,
        iw_handler handler)
{
    int i;

    if (cmd == 0x8be1 || cmd == 0x8be0 || cmd == 0x8be2) {
        printk("[%s %d]cmd 0x%x,  num_private_args %d\n", __func__, __LINE__, cmd, dev->wireless_handlers->num_private_args); 
        for (i = 0; i < dev->wireless_handlers->num_private_args; i++)
            printk("[%s %d] i %d, cmd 0x%x\n", __func__, __LINE__, i, dev->wireless_handlers->private_args[i].cmd); 
    }

    if (cmd == 0x8be1 || cmd == 0x8be0 || cmd == 0x8be2) {
        printk("[%s %d]cmd 0x%x,  num_private_args %d\n", __func__, __LINE__, cmd, dev->wireless_handlers->num_private_args); 
        for (i = 0; i < dev->wireless_handlers->num_private_args; i++) {
            if (cmd == dev->wireless_handlers->private_args[i].cmd) {
                printk("[%s %d] break: i %d, cmd 0x%x\n", __func__, __LINE__, i, dev->wireless_handlers->private_args[i].cmd); 
                break;
            }
        }
    }
	/* Always end with a call to jprobe_return(). */
    jprobe_return();
    return 0;
}

static int wext_handle_ioctl_jentry(struct net *net, struct ifreq *ifr, unsigned int cmd, void __user *arg)
{
    if (cmd == 0x8be1 || cmd == 0x8be0 || cmd == 0x8be2)
        printk("[%s %d] cmd 0x%x\n", __func__, __LINE__, cmd); 

	/* Always end with a call to jprobe_return(). */
    jprobe_return();
    return 0;
}

/* 函数原型定义为static类型，所以register 会失败 */
static int get_priv_descr_and_size_jentry(struct net_device *dev, unsigned int cmd, const struct iw_priv_args **descrp)
{
    printk("[%s %d] cmd 0x%x\n", __func__, __LINE__, cmd); 
    jprobe_return();
    return 0;
}

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp = {
	.symbol_name	= "loopback_xmit",
};

static struct jprobe debug_jprobe[] = { 
    /* JPROBE_DEFINE(do_fork), */
    JPROBE_DEFINE(wext_handle_ioctl),
    JPROBE_DEFINE(ioctl_private_call),
    /* JPROBE_DEFINE(get_priv_descr_and_size), will fail */
};

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	printk(KERN_INFO "X86 pre_handler: p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx\n",
		p->addr, regs->ip, regs->flags);

	/* A dump_stack() here will give a stack backtrace */
	return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
	printk(KERN_INFO "post_handler: p->addr = 0x%p, flags = 0x%lx\n",
		p->addr, regs->flags);

    return;
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_INFO "fault_handler: p->addr = 0x%p, trap #%dn",
		p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}

static int __init jprobe_init(void)
{
    int i = 0, j = 0;
    int ret = 0;

    for (i = 0; i < ARRAY_SIZE(debug_jprobe); i++) {
        ret = register_jprobe(&debug_jprobe[i]);
        if (ret < 0) {
            printk("register_jprobe: func %s failed, returned %d\n", debug_jprobe[i].kp.symbol_name, ret);
            goto bad;
        }
    }

    return 0;

bad:
    for (j = 0; j < i; j++)
        unregister_jprobe(&debug_jprobe[j]);

    return -1;
}

static void __exit jprobe_exit(void)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(debug_jprobe); i++)
        unregister_jprobe(&debug_jprobe[i]);

    return;
}


static int __init kprobe_init(void)
{
#if 0
	int ret;
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "Planted kprobe at %p\n", kp.addr);
#endif

    jprobe_init();
    
	return 0;
}

static void __exit kprobe_exit(void)
{
    /* unregister_kprobe(&kp); */
    jprobe_exit();
	printk(KERN_INFO "kprobe at unregistered\n", kp.addr);
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");
