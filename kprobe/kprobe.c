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

#define KPROBE_DEFINE_ADDR(ADDR, pre, post, fault)        \
{                                  \
    .addr = ADDR,   \
    .pre_handler = pre,\
    .post_handler = post,\
    .fault_handler = fault,\
}

#define KPROBE_DEFINE_NAME(func, pre, post, fault)        \
{                                  \
    .symbol_name = #func,   \
    .pre_handler = pre,\
    .post_handler = post,\
    .fault_handler = fault,\
}

#ifdef KCONFIG_WEXT_ENABLED
static int ioctl_private_call_jentry(struct net_device *dev, struct iwreq *iwr,
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

#endif

void handle_fasteoi_irq_jentry(unsigned int irq, struct irq_desc *desc)
{
    if (irq == 16)
        printk("[%s %d] irq desc: name %s, nr_actions %d\n", __func__, __LINE__, desc->name, desc->nr_actions); 
    jprobe_return();
    return 0;
}

static int ip_rcv_jentry(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
    printk("[%s %d] dev->name %s\n", __func__, __LINE__, dev->name); 
    jprobe_return();
    return 0;
}

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	printk(KERN_INFO "pre_handler: p->symbol_name %s,  p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx\n", p->symbol_name,	p->addr, regs->ip, regs->flags);

	/* A dump_stack() here will give a stack backtrace */
	return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
	printk(KERN_INFO "post_handler: p->symbol_name %s, p->addr = 0x%p, flags = 0x%lx\n",
		p->symbol_name,	p->addr, regs->flags);

    return;
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_INFO "fault_handler: p->symbol_name %s, p->addr = 0x%p, trap #%dn",
		p->symbol_name,	p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp = {
    /* .symbol_name	= "kernel_thread", */
	.pre_handler = handler_pre,
	.post_handler = handler_post,
	.fault_handler = handler_fault,
};

static struct kprobe kp0 = {
    .symbol_name	= "loopback_xmit",
	.pre_handler = NULL,
	.post_handler = NULL,
	.fault_handler = NULL,
};

static struct kprobe debug_kprobe[] = {
    KPROBE_DEFINE_NAME(netif_rx, handler_pre, handler_post, handler_fault),
    KPROBE_DEFINE_ADDR(0xc161f240, NULL, NULL, NULL),
};

static int __init kprobe_init(void)
{
	int ret;
    int i, j;

    /* type 1 */
    kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name("kernel_thread");
    /* kp.addr = (kprobe_opcode_t *)0xc10628b0 */
    if (kp.addr == NULL)
        printk("[%s %d] not found symbol address\n", __func__, __LINE__); 
	ret = register_kprobe(&kp);
	if (ret < 0) {
		printk(KERN_INFO "[%s %d]register_kprobe failed, returned %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	printk(KERN_INFO "Planted kprobe %s at %p\n", kp.symbol_name, kp.addr);

    /* type 2 */
	ret = register_kprobe(&kp0);
	if (ret < 0) {
		printk(KERN_INFO "[%s %d]register_kprobe failed, returned %d\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	printk(KERN_INFO "Planted kprobe %s at %p\n", kp0.symbol_name, kp0.addr);

    /* type 3 */
    for (i = 0; i < ARRAY_SIZE(debug_kprobe); i++) {
        ret = register_kprobe(&debug_kprobe[i]);
        if (ret < 0) {
            printk("register_jprobe: func %s failed, returned %d\n", debug_kprobe[i].symbol_name, ret);
            goto bad;
        }
        printk("[%s %d] plant %s ok %p\n", __func__, __LINE__, debug_kprobe[i].symbol_name, debug_kprobe[i].addr); 
    }
    return 0;

bad:
    for (j = 0; j < i; j++)
        unregister_kprobe(&debug_kprobe[j]);

    return ret;
}


static void __exit kprobe_exit(void)
{
    int i;
    unregister_kprobe(&kp);
    unregister_kprobe(&kp0);

    for (i = 0; i < ARRAY_SIZE(debug_kprobe); i++)
        unregister_kprobe(&debug_kprobe[i]);

    printk("[%s %d] Bye, kprobe\n", __func__, __LINE__); 
}

static struct jprobe debug_jprobe[] = { 
    JPROBE_DEFINE(ip_rcv),
#ifdef KCONFIG_WEXT_ENABLED
    JPROBE_DEFINE(wext_handle_ioctl),
    JPROBE_DEFINE(ioctl_private_call),
#endif
    JPROBE_DEFINE(handle_fasteoi_irq),
    /* JPROBE_DEFINE(get_priv_descr_and_size), will fail, because its static type */
};


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

    printk("[%s %d] register success\n", __func__, __LINE__); 
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

    printk("[%s %d] unregister success\n", __func__, __LINE__); 
    return;
}


static int __init probe_init(void)
{
    kprobe_init();
    jprobe_init();
	return 0;
}

static void __exit probe_exit(void)
{
    kprobe_exit();
    jprobe_exit();
}

module_init(probe_init)
module_exit(probe_exit)
MODULE_LICENSE("GPL");
