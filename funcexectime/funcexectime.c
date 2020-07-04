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
#include <linux/kallsyms.h>
#include <linux/limits.h>
#include <linux/sched.h>

static char funcname[NAME_MAX] = "module_address_lookup";
module_param_string(func, funcname, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "calculate function's execution time");

/* per-instance private data */
struct insstamp {
    ktime_t entry_stamp;
};

static unsigned long count = 0;

/*
 * teturn-probe handler: Log the return value and duration. Duration may turn
 * out to be zero consistently, depending upon the granularity of time
 * accounting on the platform.
 */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    int retval = regs_return_value(regs);
    struct insstamp *data = (struct insstamp *)ri->data;
    s64 delta;
    ktime_t now;

    now = ktime_get();
    delta = ktime_to_ns(ktime_sub(now, data->entry_stamp));

    trace_printk("[%lu]%s ret %d took %lld ns\n", count++,
            funcname, retval, (long long)delta);
    return 0;
}


static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct insstamp *data;

    if (!current->mm)
        return 1;       /* Skip kernel threads */

    data = (struct insstamp *)ri->data;
    data->entry_stamp = ktime_get();
    return 0;
}

static struct kretprobe modukretp = {
        .handler                = ret_handler,
        .entry_handler          = entry_handler,
        .data_size              = sizeof(struct insstamp),
        /* Probe up to 20 instances concurrently. */
        .maxactive              = 20,
};

static int __init fet_init(void)
{
    int ret;

    modukretp.kp.symbol_name = funcname;
    ret = register_kretprobe(&modukretp);
    if (ret < 0) {
        printk(KERN_INFO "register_kretprobe failed, returned %d\n",
                ret);
        return -1;
    }
    printk(KERN_INFO "Planted return probe at %s: %p\n",
            modukretp.kp.symbol_name, modukretp.kp.addr);
    return 0;
}

static void __exit fet_exit(void)
{
    unregister_kretprobe(&modukretp);
    printk(KERN_INFO "kretprobe at %p unregistered\n",
            modukretp.kp.addr);

    /* nmissed > 0 suggests that maxactive was set too low. */
    printk(KERN_INFO "Missed probing %d instances of %s\n",
            modukretp.nmissed, modukretp.kp.symbol_name);
}

module_init(fet_init)
module_exit(fet_exit)
MODULE_LICENSE("GPL");
