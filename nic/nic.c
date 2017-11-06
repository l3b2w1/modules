#include <linux/module.h>  /* specifically, a module, needed by all modules */
#include <linux/if_ether.h>
#include <linux/kernel.h>	/* we are doing the kernel work, eg. KERN_INFO */
#include <linux/file.h>
#include <linux/init.h>		/* needed by macro */
#include <asm/uaccess.h>	/* for put_user */
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <net/sock.h>
/* #include <linux/netfilter.h> */
#include <net/netlink.h>
#include <net/netns/generic.h>
#include <net/netlink.h>
#include <linux/if_arp.h>
#include <linux/slab.h>
#include <linux/netfilter_bridge.h>
#include <linux/tty.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/wireless.h>

MODULE_LICENSE("GPL");

static int nic_irq = 16;
struct net_device * nic_netdev;

struct nic_private {
    struct net_device *dev;
    char info[32];
};

static irqreturn_t nic_intr(int data, void *devid)
{
    /* printk("[%s %d] irq handler\n", __func__, __LINE__);  */
    return IRQ_NONE;
}

struct net_device_ops nic_netdevice_ops = {
    .ndo_open = NULL,
    .ndo_stop = NULL,
    .ndo_do_ioctl = NULL,
    .ndo_start_xmit = NULL,
};

unsigned int nic_hook_local_out(const struct nf_hook_ops *ops,
			struct sk_buff *skb,
			const struct nf_hook_state *state)
{
	unsigned char *data;

	data = skb->data;
	printk(KERN_INFO "packet len %d, data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
				skb->len, data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8], data[9], data[10],
				data[11]);
	return NF_ACCEPT;
}


unsigned int nic_hook_pre_routing(const struct nf_hook_ops *ops,
			struct sk_buff *skb,
			const struct nf_hook_state *state)
{
	return NF_ACCEPT;
}

unsigned int nic_hook_post_routing(const struct nf_hook_ops *ops,
			struct sk_buff *skb,
			const struct nf_hook_state *state)
{
	return NF_ACCEPT;
}

static struct nf_hook_ops nic_hooks[] = {
    {
        .hook       =   nic_hook_local_out,
        /* .owner      =   THIS_MODULE, */
        .pf         =   PF_INET,
        .hooknum    =   NF_INET_LOCAL_OUT,
        /* .priority   =   NF_IP_PRI_FIRST, */
    },
    {
        .hook       =   nic_hook_pre_routing,
        /* .owner      =   THIS_MODULE, */
        .pf         =   PF_INET,
        .hooknum    =   NF_INET_PRE_ROUTING,
        /* .priority   =   NF_IP_PRI_FIRST, */
    },
    {
        .hook       =   nic_hook_post_routing,
        /* .owner      =   THIS_MODULE, */
        .pf         =   PF_INET,
        .hooknum    =   NF_INET_POST_ROUTING,
        /* .priority   =   NF_IP_PRI_FIRST, */
    },
};

static int nic_hooks_init(void)
{
    int ret = 0;
    ret = nf_register_hooks(nic_hooks, ARRAY_SIZE(nic_hooks));
    if (ret)
        printk("Failed to register hook\n");
    else
        printk("register hook success\n");

    return ret;
}


int nic_init(void)
{
    struct net_device *dev;
    struct nic_private *pri;
    int i, ret = 0;

    dev = dev_get_by_name(&init_net, "nic");
    if (dev) {
        dev_put(dev);
        return 0;
    }

    dev = alloc_etherdev(sizeof(struct nic_private));
    if (!dev) {
        printk("[%s %d] alloc_etherdev failed\n", __func__, __LINE__); 
        return -ENOMEM;
    }

    pri = (struct nic_private*)netdev_priv(dev);
    pri->dev = dev;
    strncpy(dev->name, "nic", sizeof(dev->name));

    for (i = 0; i < 6; i++)
        dev->dev_addr[i] = i;

    ether_setup(dev);
    dev->netdev_ops = &nic_netdevice_ops;

    ret = register_netdev(dev);

    nic_netdev = dev;

    ret = request_irq(nic_irq, nic_intr, IRQF_SHARED, dev->name, dev);
    if (ret)
        printk("[%s %d] request_irq failed\n", __func__, __LINE__); 

	nic_hooks_init();

    printk("[%s %d] register nic\n", __func__, __LINE__); 
    return 0;
}

void nic_fini(void)
{
    struct net_device *dev;
    dev = nic_netdev;

    if (!dev)
        return;

    unregister_netdev(dev);
    free_irq(nic_irq, dev);
    nic_netdev = NULL;

	nf_unregister_hooks(nic_hooks, ARRAY_SIZE(nic_hooks));
    printk("[%s %d] unregister nic\n", __func__, __LINE__); 

    return;
}

module_init(nic_init);
module_exit(nic_fini);
