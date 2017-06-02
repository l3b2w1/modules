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
#include <net/netlink.h>
#include <net/netns/generic.h>
#include <net/netlink.h>
#include <linux/slab.h>
#include <linux/netfilter_bridge.h>
#include <linux/tty.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/wireless.h>
#include <net/iw_handler.h>


/*
 * These two kernle config macro should be enabled
 * CONFIG_WIRELESS_EXT=y
 * CONFIG_WEXT_PRIV=y
 */

MODULE_LICENSE("GPL");

struct dev_mon_private {
    struct net_device *dev;
    char info[128];
};


enum {
	IOCTL_MON_HELLO = 0x11,
	IOCTL_MON_WORLD,
    IOCTL_MON_SET_MAXSTA,
    IOCTL_MON_GET_MAXSTA,
};

/*
 *
 * iwpriv mon hello helloworld
 * iwpriv mon setmaxsta 123
 * iwpriv mon getmaxsta
 *
 */

#define IOCTL_SETPARA_MON (SIOCIWFIRSTPRIV + 0) /* set command, return value stored in u->data */
#define IOCTL_GETPARA_MON (SIOCIWFIRSTPRIV + 1) /* get command, return value stored in u->data */
#define IOCTL_CHAR128_MON (SIOCIWFIRSTPRIV + 2) /* set command, return value stored in u->data */

int ioctl_char128_mon(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra);

int ioctl_setpara_mon(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra);

int ioctl_getpara_mon(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra);


static const iw_handler iw_handlers_mon[] = {
    (iw_handler) ioctl_setpara_mon,/* SIOCWFIRSTPRIV + 0 */
    (iw_handler) ioctl_getpara_mon,/* SIOCWFIRSTPRIV + 1 */
    (iw_handler) ioctl_char128_mon,/* SIOCWFIRSTPRIV + 2 */
};


/* if change 128 to 4096, iwpriv command not work, maybe 4096 is too large */
/* i do not know why these three sub-ioctl should be included first, or iwpriv won't work */
static const struct iw_priv_args priv_args_mon[] = {
    /* sub-ioctl begin */
    {IOCTL_SETPARA_MON, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, ""},
    {IOCTL_GETPARA_MON, 0, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, ""},
    {IOCTL_CHAR128_MON, IW_PRIV_TYPE_CHAR | 128, IW_HEADER_TYPE_CHAR | 128, ""},
    /* sub-ioctl end */
    {IOCTL_MON_HELLO, IW_PRIV_TYPE_CHAR | 128, IW_HEADER_TYPE_CHAR | 128, "hello"},
    {IOCTL_MON_WORLD, IW_PRIV_TYPE_CHAR | 128, IW_HEADER_TYPE_CHAR | 128, "world"},
    {IOCTL_MON_SET_MAXSTA, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "setmaxsta"},
    {IOCTL_MON_GET_MAXSTA, 0, IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, "getmaxsta"},
};

struct iw_handler_def iw_handler_def_mon = {
#define N(a) (sizeof(a)/sizeof(a[0]))
    /* .standard = (iw_handler *)ioctl_handlers_mon, */
    .private = (iw_handler *)iw_handlers_mon,
    .num_private = N(iw_handlers_mon),
    .private_args = (struct iw_priv_args *) priv_args_mon,
    .num_private_args = N(priv_args_mon),
#undef N
};


void mon_hello_handler(char *msg, int len)
{
    printk("[%s %d] hello: %s\n", __func__, __LINE__, msg); 
    return;
}

void mon_world_handler(char *msg, int len)
{
    printk("[%s %d] world: %s\n", __func__, __LINE__, msg); 
    return;
}

static int g_maxsta = 1;

int ioctl_setpara_mon(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra)
{
    int *i = (int *)extra;

    int param = i[0];
    int value = i[1];

    printk("[%s %d] info->cmd 0x%x, info->flags %d\n", __func__, __LINE__, info->cmd, info->flags); 
    switch (param) {
        case IOCTL_MON_SET_MAXSTA:
            g_maxsta = value; /* maybe we need a mutex lock and validation check */
            printk("[%s %d] set max sta %d\n", __func__, __LINE__, g_maxsta); 
            break;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

int ioctl_getpara_mon(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra)
{
    int *param = (int*)extra;

    printk("[%s %d] info->cmd 0x%x, info->flags %d\n", __func__, __LINE__, info->cmd, info->flags); 
    switch (param[0]) {
        case IOCTL_MON_GET_MAXSTA:
            param[0] = g_maxsta;
            break;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

int ioctl_char128_mon(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra)
{
#define IOCTL_CHAR128 128
    char *s = NULL;
    union iwreq_data *u = wrqu;
    int len = u->data.length;

    s = kzalloc(IOCTL_CHAR128, GFP_KERNEL);

    if (u->data.length > IOCTL_CHAR128)
        u->data.length = IOCTL_CHAR128;

    copy_from_user(s, u->data.pointer, u->data.length);
    s[IOCTL_CHAR128 - 1] = '\0';

    printk("[%s %d] info->cmd 0x%x, info->flags %d\n", __func__, __LINE__, info->cmd, info->flags); 
    printk("[%s %d] flags 0x%x, datalen %d\n", __func__, __LINE__, u->data.flags, len); 

    /* dump_stack(); */
    switch (u->data.flags) {
        case IOCTL_MON_HELLO:
            mon_hello_handler(u->data.pointer, len);
            break;
        case IOCTL_MON_WORLD:
            mon_world_handler(u->data.pointer, len);
            break;
        default:
            printk("[%s %d] unknown command\n", __func__, __LINE__); 
            break;
    }

    kfree(s);
    return 0;
}

static const iw_handler ioctl_handlers_mon[] = {
    (iw_handler) NULL,
};


struct net_device *dev_mon = NULL;
struct net_device_ops mon_ops;

int create_dev_mon(void)
{
	struct net_device *dev;
	struct dev_mon_private *pri;
	int rc;
	int i;

	dev = dev_get_by_name(&init_net, "mon");

	if (dev)
	{
		dev_put(dev);
		return 0;
	}

	dev = alloc_etherdev(sizeof(struct dev_mon_private));

	if (!dev) {
        printk("[%s %d] alloc_etherdev failed\n", __func__, __LINE__); 
		return -1;
    }

	pri = (struct dev_mon_private *) netdev_priv(dev);
	pri->dev = dev;
	strncpy(dev->name, "mon", sizeof(dev->name));

	for (i = 0; i < 6; i++)
		dev->dev_addr[i] = i;

	dev->destructor = free_netdev;
	memset(&mon_ops, 0, sizeof(struct net_device_ops));
	dev->netdev_ops = &mon_ops;
    dev->wireless_handlers = &iw_handler_def_mon;
	rc = register_netdev(dev);
	dev_mon = dev;
    printk("[%s %d] register mon\n", __func__, __LINE__); 
    return 0;
}

void delete_dev_mon(void)
{
    struct net_device *dev;
    dev = dev_mon;

    if (!dev)
        return;

    unregister_netdev(dev);
    dev_mon = NULL;
    printk("[%s %d] unregister mon\n", __func__, __LINE__); 
    return;
}

module_init(create_dev_mon);
module_exit(delete_dev_mon);
