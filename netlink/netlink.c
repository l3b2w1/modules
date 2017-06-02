#include <linux/module.h>  /* specifically, a module, needed by all modules */
#include <linux/kernel.h>	/* we are doing the kernel work, eg. KERN_INFO */
#include <linux/file.h>
#include <linux/init.h>		/* needed by macro */
#include <asm/uaccess.h>	/* for put_user */
#include <linux/proc_fs.h>	/*	necessary because we use proc fs */
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/freezer.h>
#include <linux/tty.h>
#include <linux/pid_namespace.h>
#include <net/netns/generic.h>
#include <net/netlink.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/spinlock.h>
#include <linux/tty.h>
#include <linux/version.h>
#include <linux/sched.h>

struct ntlkmsg {
	int type; // msg type
	int len; // data len
	unsigned char data[0];
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");
enum {
	CAP_ADD,
	CAP_DEL,
	CAP_PACKET,
	CAP_GET_PACKET
};

static struct sock *cap_sock;

static DEFINE_MUTEX(cap_cmd_mutex);

static int cap_nlk_portid = 0;

static struct nf_hook_ops nfhk;

static struct kmem_cache *capcache = NULL;

struct usrconf{
	int pid;
	int cmd;
};

#define MAX_TAB_SIZE	32
#define MAX_SKB_BUFSIZE  2000 
#define CAP_MAXFREE		1024	
DEFINE_SPINLOCK(usrtab_lock);
static int usrcnt = 0;
struct usrconf usrtab[MAX_TAB_SIZE];

struct cap_buffer {
	struct list_head	 list;
	struct sk_buff		 *skb;	// formatted skb ready to send
	void				*data; // something may  needed in the future

	gfp_t				gfp_mask;
};



/* the cap_freelist is a list of pre-allocated capture buffers (if more
 * than CAPTURE_MAXFREE are in use, the capture buffer is freed instead of
 * being placed on the freelist) */
static DEFINE_SPINLOCK(cap_freelist_lock);
static int		cap_freelist_count;
static LIST_HEAD(cap_freelist);

static struct sk_buff_head cap_skb_queue;
/* queue of skbs to send to captured when/if it comes back */
static struct task_struct *kcapd_task;
static DECLARE_WAIT_QUEUE_HEAD(kcapd_wait);
static DECLARE_WAIT_QUEUE_HEAD(cap_backlog_wait);

/* if cap_rate_limit is non-zero, limit the rate of sending caprured packets
 * to that number per second. This prevents DoS attacks, but results in 
 * captured packets being droped. */
static u32 cap_rate_limit;

/* number of outstanding cap_buffers allowed.
 * when set to zero, this means unlimited. */
static u32 cap_backlog_limit = 64;
static void cap_buffer_free(struct cap_buffer *cb)
{
	unsigned long flags;

	if (!cb)
		kfree_skb(cb->skb);

	spin_lock_irqsave(&cap_freelist_lock, flags);
	if (cap_freelist_count >= CAP_MAXFREE)
		kfree(cb);
	else {
		cap_freelist_count++;
		list_add(&cb->list, &cap_freelist);
	}
	spin_unlock_irqrestore(&cap_freelist_lock, flags);
}

/* type: nlmsg_type */
static struct cap_buffer * cap_buffer_alloc(struct sk_buff *skb, void *data, gfp_t gfp_mask, int type)
{
	unsigned long flags;
	struct cap_buffer *cb = NULL;
	struct nlmsghdr *nlh;
	unsigned char *pdata;

	spin_lock_irqsave(&cap_freelist_lock, flags);
	if (!list_empty(&cap_freelist)) {
		cb = list_entry(cap_freelist.next, struct cap_buffer, list);
		list_del(&cb->list);
		--cap_freelist_count;
	}
	spin_unlock_irqrestore(&cap_freelist_lock, flags);

	if (!cb) {
		cb = kmalloc(sizeof(*cb), gfp_mask);
		if (!cb)
		  goto err;
		memset(cb, 0, sizeof(*cb));
	}

	cb->data = data;
	cb->gfp_mask = gfp_mask;

	cb->skb = nlmsg_new(MAX_SKB_BUFSIZE, gfp_mask);
	if (!cb->skb)
		goto err;

	nlh = nlmsg_put(cb->skb, cap_nlk_portid, 0, type, skb->len, 0);
	if (!nlh)
		goto out_kfree_skb;

	pdata = nlmsg_data(nlh);
	memcpy(pdata, skb->data, skb->len);

	return cb;

out_kfree_skb:
	kfree_skb(cb->skb);
	cb->skb = NULL;

err:
	cap_buffer_free(cb);
	return NULL;

}
		
void usertab_init(void)
{
	int i;
	for (i = 0; i < MAX_TAB_SIZE; ++i) {
		usrtab[i].pid = 0;
		usrtab[i].cmd = 0;
	}
	return;
}


inline void delete_all_user(void)
{
	usertab_init();
}

int user_leave(int pid)
{
	int i;
	
	if (usrcnt <= 0) {
		printk(KERN_INFO "no user");
		return 0;
	}

	for (i = 0; i < MAX_TAB_SIZE; ++i) {
		if (usrtab[i].pid == pid) {
			usrtab[i].pid = 0;
			usrcnt--;
			printk(KERN_INFO "user %d unregistered, user count %d\n", pid, usrcnt);
			return 1;
		}
	}
	return 0;
}

int user_join(int pid)
{
	int i;

	if (usrcnt >= MAX_TAB_SIZE) {
		printk(KERN_INFO "user maximum");
		return 0;
	}

	for (i = 0; i < MAX_TAB_SIZE; ++i) {
		if (usrtab[i].pid == 0) {			
			usrtab[i].pid = pid;
			usrcnt++;
			printk(KERN_INFO "user %d registered, user count %d\n", pid, usrcnt);
			return 1;
		}
	}

	return 0;
}

/* 
 * alloc skb buffer and gather payload into skb 
 * may be we do not need to malloc a new skb and copy data into it every time when we want to send msg to everyone,
 * may be we could use skb_get() (equal to atomic_inc(&skb->users)), just check it sometime;
 */
struct sk_buff *cap_make_reply(__u32 portid, int seq, int type, int done,
				int multi, const void *payload, int size)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	void *data;
	int flags = multi ? NLM_F_MULTI : 0;
	int t = done ? NLMSG_DONE : type;

	skb = nlmsg_new(size, GFP_KERNEL);
	if (!skb) {
		printk(KERN_ERR "nlmsg_new fail");
		return NULL;
	}

	nlh = nlmsg_put(skb, portid, seq, t, size, flags);
	if (!nlh) {
		printk(KERN_ERR "nlmsg_put failed");
		goto out_kfree_skb;
	}

	data = nlmsg_data(nlh);
	memcpy(data, payload, size);

	return skb;	

out_kfree_skb:
	kfree_skb(skb);
	return NULL;
}

static inline int cap_rate_check(void)
{
	static unsigned long last_check = 0;
	static int		packets = 0;
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;
	unsigned long now;
	unsigned long elapsed;
	int		ret = 0;
	if (!cap_rate_limit) return 1;

	spin_lock_irqsave(&lock, flags);
	if (++packets < cap_rate_limit) {
		ret = 1;
	} else {
		now = jiffies;
		elapsed = now - last_check;
		if (elapsed > HZ) {
			last_check = now;
			packets = 0;
			ret = 1;
		}
	}
	spin_unlock_irqrestore(&lock, flags);

	return ret;
}



/**
 * netlink_unicast() cannot be called inside an irq context because it blocks
 * (last arg, flags, it not set to MSG_DONTWAIT), so the capture buffer is
 * placed on a queue and a tasklet is scheduled to remove them from the queue
 * outside the irq context. May be called in any context. *
 */
/* does the hook function run in the irq context ? */
unsigned int hook_func_queue(const struct nf_hook_ops *ops,
			       struct sk_buff *skb,
			       const struct nf_hook_state *state)
{ 
	struct cap_buffer *cb = NULL;
	struct nlmsghdr *nlh = NULL; 

	unsigned char *data;
#if 1 
	data = skb->data;
	printk(KERN_INFO "packet len %d, data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
				skb->len, data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8], data[9], data[10],
				data[11]);
#endif

	if (!cap_nlk_portid) {
		printk(KERN_INFO "NF_ACCEPT");
		return NF_ACCEPT; 
	}

	printk(KERN_INFO "we capture the skb\n");
	cb = cap_buffer_alloc(skb, NULL, GFP_KERNEL, NLMSG_DONE); 
	if (!cb)
		return NF_ACCEPT;
	if (!cap_rate_check()) {
		printk(KERN_INFO "rate limit exceeded");
		return NF_ACCEPT;
	} else {
		nlh = nlmsg_hdr(cb->skb);

		nlh->nlmsg_len = cb->skb->len;

		skb_queue_tail(&cap_skb_queue, cb->skb);
		wake_up_interruptible(&kcapd_wait);
		
		printk(KERN_INFO "hook_func_queue, queuelen %d\n", skb_queue_len(&cap_skb_queue));
		cb->skb = NULL;
	}
	cap_buffer_free(cb);

	return NF_ACCEPT;
}

//int netlink_unicast(struct sock *ssk, struct sk_buff *skb,
//		    u32 portid, int nonblock)
unsigned int hook_func(const struct nf_hook_ops *ops,
			       struct sk_buff *skb,
			       const struct nf_hook_state *state)
{ 
	int i, ret;
	struct sk_buff *tmp = NULL;
	unsigned char *data;
#if 1 
	data = skb->data;
	printk(KERN_INFO "packet len %d, data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
				skb->len, data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8], data[9], data[10],
				data[11]);
#endif

	for (i = 0; i < MAX_TAB_SIZE; ++i) {
		if (usrtab[i].pid != 0) {
			tmp = cap_make_reply(usrtab[i].pid, 0, NLMSG_DONE, 1, 0,  skb->data, skb->len);
			if (tmp) {
				ret = netlink_unicast(cap_sock, tmp, usrtab[i].pid, 0);
				if (ret == -ECONNREFUSED)
					delete_all_user();	
				//kfree_skb(tmp);   // why system down if executed, i already know
			}
		}
	}
	return NF_ACCEPT;
}

static int cap_hook_init(void)
{
/* IP Hooks */
/* After promisc drops, checksum checks. */
#define NF_IP_PRE_ROUTING	0
/* If the packet is destined for this box. */
#define NF_IP_LOCAL_IN		1
/* If the packet is destined for another interface. */
#define NF_IP_FORWARD		2
/* Packets coming from a local process. */
#define NF_IP_LOCAL_OUT		3
/* Packets about to hit the wire. */
#define NF_IP_POST_ROUTING	4
#define NF_IP_NUMHOOKS		5


	nfhk.hook = hook_func_queue;
	nfhk.hooknum = NF_IP_LOCAL_OUT;
	nfhk.pf = PF_INET;
	nfhk.priority = NF_IP_PRI_FIRST;

	nf_register_hook(&nfhk);
	return 0;
}


static int cap_netlink_ok(struct sk_buff *skb, u16 msg_type)
{
	int err = 0;
	printk(KERN_INFO "msg type %d\n", msg_type);
	switch (msg_type) {
	case CAP_ADD:
		printk(KERN_INFO "cap msg type:CAP_ADD");
		break;
	case CAP_DEL:
		printk(KERN_INFO "cap msg type:CAP_DEL");
		break;
	case CAP_GET_PACKET:
		printk(KERN_INFO "cap msg type:CAP_GET_PACKET");
		break;
	default:
		err = -EINVAL;
	}
	return err;
}

static void kcapd_send_reply(struct sk_buff *skb)
{
	int err;

	skb_get(skb);
	/* take a reference in case we can't send it and we want to hold it */
	err = netlink_unicast(cap_sock, skb, cap_nlk_portid, 0);
	printk(KERN_INFO "kcapd_send_reply portid %d", cap_nlk_portid);
	if (err < 0) {
		BUG_ON(err != -ECONNREFUSED); /* Shouldn't happen */
		printk(KERN_INFO "send data fail\n");
		cap_nlk_portid = 0;
	} else {
		/* drop the extra reference if sent ok */
		consume_skb(skb);
		printk(KERN_INFO "cosume skb\n");
	}
}

static int kcapd_thread(void *dummy)
{
	set_freezable();
	while (!kthread_should_stop()) {
		struct sk_buff *skb;

		skb = skb_dequeue(&cap_skb_queue);

		if (skb) {
			printk(KERN_INFO "kcap_thread, queuelen %d, backloglimit %d\n",
						skb_queue_len(&cap_skb_queue), cap_backlog_limit);

			if (skb_queue_len(&cap_skb_queue) <= cap_backlog_limit)
				wake_up(&cap_backlog_wait);
			kcapd_send_reply(skb);
			
			continue;
		}

		wait_event_freezable(kcapd_wait, skb_queue_len(&cap_skb_queue));
	}
	return 0;
}

int cap_receive_msg(struct sk_buff *skb, struct nlmsghdr* nlh)
{
	void *data;
	int err;
	u16 msg_type = nlh->nlmsg_type;
	int pid;

	err = cap_netlink_ok(skb, msg_type);
	if (err)
		return err;
#if 1 
	/* As soon as there's any sign of userspace capd,
	 * start kcapd to talk to it */
	if (!kcapd_task) {
		kcapd_task = kthread_run(kcapd_thread, NULL, "kcapd");
		if (IS_ERR(kcapd_task)) {
			err = PTR_ERR(kcapd_task);
			kcapd_task = NULL;
			return err;
		}
	}

#endif
	data = nlmsg_data(nlh);
	pid = nlh->nlmsg_pid;
	printk(KERN_INFO "receive user pid %d, msgtype %d, client says: %s\n", 
				nlh->nlmsg_pid,
				nlh->nlmsg_type, 
				(char*)data);
	switch (msg_type) {
	case CAP_ADD:
		printk("am i here, cap add !\n");
		cap_nlk_portid = pid;
#if 1
		spin_lock(&usrtab_lock);
		user_join(pid);
		spin_unlock(&usrtab_lock);
#endif
		break;
	case CAP_DEL:
		printk("am i here, cap del !\n");
		cap_nlk_portid = 0;
#if 1
		spin_lock(&usrtab_lock);
		user_leave(pid);
		spin_unlock(&usrtab_lock);
#endif
		break;
	case CAP_GET_PACKET:
		printk("am i here, cap get packet !\n");
		break;
	default:
		err = -EINVAL;
		break;
	}
	return 0;
}

static int cap_receive_skb(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int len, err;

	nlh = nlmsg_hdr(skb);
	len = skb->len;

	printk(KERN_INFO "cap_receive_skb\n");
	while (nlmsg_ok(nlh, len)) {
		err = cap_receive_msg(skb, nlh);
		/* if err or if this message says it wants a reponse */
		if (err || (nlh->nlmsg_flags & NLM_F_ACK))
			netlink_ack(skb, nlh, err);

		nlh = nlmsg_next(nlh,  &len);
	}
	return 0;	
}

/* receive messages from netlink socket. */
static void cap_receive(struct sk_buff *skb)
{
	printk(KERN_INFO "cap_receive\n");
	mutex_lock(&cap_cmd_mutex);
	cap_receive_skb(skb);
	mutex_unlock(&cap_cmd_mutex);
}


//kmem_cache_create(const char *name, size_t size, size_t align,
//		  unsigned long flags, void (*ctor)(void *))
static int cap_kmem_cache_init(void)
{
	capcache = kmem_cache_create("capture", sizeof(struct sk_buff), 0, SLAB_POISON, NULL); 
	if (!capcache) 
		return -1;

	return 0;
}

static int cap_kmem_cache_fini(void)
{
	int err = 0;
	if (capcache)
		kmem_cache_destroy(capcache);

	return err;
}	

static int __init app_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.input = cap_receive,
	};


	cap_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if (!cap_sock)
		return -ENOMEM;

	usertab_init();
	cap_hook_init();
	skb_queue_head_init(&cap_skb_queue);
	//cap_kmem_cache_init();
	cap_nlk_portid = 0;
	printk(KERN_INFO "Netlink module, hello\n");
	return 0;
}
static void __exit app_exit(void)
{
	printk(KERN_INFO "Bye world !\n");
	netlink_kernel_release(cap_sock);
	nf_unregister_hook(&nfhk);
	//cap_kmem_cache_fini();
}

module_init(app_init);
module_exit(app_exit);
