#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>  
#include <linux/types.h>  
#include <linux/netdevice.h>  
#include <linux/skbuff.h>  
#include <linux/inet.h>  
#include <linux/netfilter_ipv4.h>  
#include <linux/in.h>  
#include <linux/ip.h>  
#include <linux/tcp.h>  
#include <linux/udp.h>  
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>

/* 
 * i want to send a self-constructed packet within kernel 
 * make a gratuitous arp to send
 * learn from linux kernel inetdev_send_gratuitous_arp() function
 */

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x) ((u8*)(x))[0],((u8*)(x))[1],((u8*)(x))[2],((u8*)(x))[3],((u8*)(x))[4],((u8*)(x))[5]

struct sk_buff * garp_create(__be32 dest_ip,
	      struct net_device *dev, __be32 src_ip,
	      const unsigned char *dest_hw, 
          const unsigned char *src_hw,
	      const unsigned char *target_hw) 
{
    struct sk_buff *skb;
    struct arphdr *arp;
    unsigned char *arp_ptr;

    int hlen = LL_RESERVED_SPACE(dev);
    int tlen = dev->needed_tailroom;


    /* Allocate a buffer */
    skb = alloc_skb(arp_hdr_len(dev) + hlen + tlen, GFP_ATOMIC);
    if (!skb)
        return NULL;

    skb_reserve(skb, hlen);
    skb_reset_network_header(skb);
    arp = (struct arphdr *)skb_put(skb, arp_hdr_len(dev));
    skb->dev = dev;
    skb->protocol = htons(ETH_P_ARP);
    if (!src_hw)
        src_hw = dev->dev_addr;
    if (!dest_hw)
        dest_hw = dev->broadcast;

    /* Fill the device header for the ARP frame */
    if (eth_header(skb, dev, ETH_P_ARP, dest_hw, src_hw, skb->len) < 0)
        goto out;

    /* Fill out the arp protocol part. */
    arp->ar_hrd = htons(dev->type);
    arp->ar_pro = htons(ETH_P_IP);

    arp->ar_hln = dev->addr_len;
    arp->ar_pln = 4;
    arp->ar_op = htons(ARPOP_REQUEST);

    arp_ptr = (unsigned char *)(arp + 1);

    memcpy(arp_ptr, src_hw, dev->addr_len);
    arp_ptr += dev->addr_len;
    memcpy(arp_ptr, &src_ip, 4);
    arp_ptr += 4;

    if (target_hw)
        memcpy(arp_ptr, target_hw, dev->addr_len);
    else
        memset(arp_ptr, 0, dev->addr_len);
    arp_ptr += dev->addr_len;

    memcpy(arp_ptr, &dest_ip, 4);

    return skb;

out:
    kfree_skb(skb);
    return NULL;
}

static int garp_xmit(struct sk_buff *skb)
{
    return dev_queue_xmit(skb);
}

void garp_send(__be32 dest_ip,
	      struct net_device *dev, __be32 src_ip,
	      const unsigned char *dest_hw, const unsigned char *src_hw,
	      const unsigned char *target_hw) 
{
    struct sk_buff *skb;

	skb = garp_create(dest_ip, dev, src_ip,
			 dest_hw, src_hw, target_hw);
    if (!skb)
        return;

	skb_dst_set(skb, dst_clone(NULL));
    if (garp_xmit(skb) < 0)
        kfree_skb(skb);
}

static void garp_send_gratuitous_arp(struct net_device *dev)
{
    struct in_ifaddr *ifa;
    struct in_device *in_dev = __in_dev_get_rtnl(dev);

    for (ifa = in_dev->ifa_list; ifa; ifa = ifa->ifa_next) {
        garp_send(ifa->ifa_local, dev,
                ifa->ifa_local, NULL,
                dev->dev_addr, NULL);
        printk("DEST: " MAC_FMT "\n", MAC_ARG(dev->dev_addr));
    }
}

static int __init garp_init(void)
{
    struct net_device *dev = NULL;
    dev = dev_get_by_name(&init_net, "eth0");
    if (!dev) {
        printk("[%s %d] get dev eth0  failed\n", __func__, __LINE__); 
        return -1;
    }

    printk("[%s %d] enter \n", __func__, __LINE__); 
    garp_send_gratuitous_arp(dev);
    garp_send_gratuitous_arp(dev);
    garp_send_gratuitous_arp(dev);
    dev_put(dev);
    return 0;
}

static void __exit garp_exit(void)
{
    printk("[%s %d] exit \n", __func__, __LINE__); 
}

module_init(garp_init);
module_exit(garp_exit);
