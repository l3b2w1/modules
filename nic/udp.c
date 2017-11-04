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

char payload[] = "i want to send a packet within kernel";

/* blue 44:37:e6:a6:d6:e0 */
/* linux   d4:3d:7e:12:49:06 */
unsigned char dstmac[ETH_ALEN]= {0xd4,0x3d,0x7e, 0x12, 0x49, 0x06};  
unsigned char srcmac[ETH_ALEN] = {0x44,0x37,0xe6, 0xa6, 0xd6, 0xe0};  
unsigned char dstip[4] = {10,0,3,58};  
unsigned char srcip[4] = {10,0,1,199};  
uint32_t sip = 0;
uint32_t dip = 0;

#define SPORT	5320
#define DPORT	5321

uint64_t fakeid = 0;
uint64_t seq = 0;
uint64_t ack_seq = 0;

int create_new_skb(void) 
{
    int ret = -1, len = 0;
    struct ethhdr *eh = 0;  
    struct iphdr *ih = 0;  
    struct udphdr *uh = 0;
    struct net_device *dev = 0;
    struct sk_buff *skb = NULL;
    unsigned char * pdata = 0;  

    dev = dev_get_by_name(&init_net, "eth0");
    if (!dev) {
        printk("[%s %d] get dev eth0  failed\n", __func__, __LINE__); 
        return -1;
    }
    
    len = sizeof(payload) + LL_RESERVED_SPACE(dev) + sizeof(struct iphdr) + sizeof(struct udphdr) + dev->needed_tailroom;

    skb = alloc_skb(len, GFP_ATOMIC);
    if (!skb) {
        printk("[%s %d] skb alloc failed\n", __func__, __LINE__); 
        return -ENOMEM;
    }

    skb->dev = dev;
    skb->pkt_type = PACKET_OTHERHOST;
    skb->protocol = __constant_htons(ETH_P_IP);
    skb->ip_summed = CHECKSUM_NONE;
    skb->priority = 0;

    skb_reserve(skb, LL_RESERVED_SPACE(dev));
    skb_reset_network_header(skb);

    ih = (struct iphdr *)skb_put(skb, sizeof(struct iphdr));
    ih->version = 4;
    ih->ihl = sizeof(struct iphdr) >> 2;
    ih->frag_off = 0;
    ih->protocol = IPPROTO_UDP;
    ih->tos = 0;
    memcpy(&sip, srcip, sizeof(srcip));
    memcpy(&dip, dstip, sizeof(dstip));
    ih->saddr = sip;
    ih->daddr = dip;
    ih->ttl = 0x40;
    ih->tot_len = __constant_htons(sizeof(payload) + sizeof(struct iphdr) + sizeof(struct udphdr));
    ih->check = ip_fast_csum((char*)ih, ih->ihl);

    uh = (struct udphdr *)skb_put(skb, sizeof(struct udphdr));
    uh->source = htons(SPORT);
    uh->dest = htons(DPORT);
    uh->len = htons(sizeof(struct udphdr) + sizeof(payload));
    uh->check = 0;

    skb->csum = skb_checksum (skb, ih->ihl*4, skb->len - ih->ihl * 4, 0);
    uh->check = csum_tcpudp_magic (sip, dip, skb->len - ih->ihl * 4, IPPROTO_UDP, skb->csum);

    pdata = skb_put(skb, sizeof(payload));
    memcpy(pdata, payload, sizeof(payload));

    eh = (struct ethhdr*)skb_push(skb, 14);
    eh->h_proto = __constant_htons(ETH_P_IP);
    memcpy(eh->h_dest, dstmac, ETH_ALEN);
    memcpy(eh->h_source, srcmac, ETH_ALEN);

    if ((ret = dev_queue_xmit(skb)) < 0)
        goto out;
    dev_put(dev);
    return 0;

out:
    if(skb) {
        kfree_skb(skb);
        dev_put(dev);
    }
    return ret;
}

static int __init skbinit(void)
{
    printk("[%s %d]\n", __func__, __LINE__); 
    create_new_skb();
    return 0;
}

static void __exit skbexit(void)
{
    printk("[%s %d]\n", __func__, __LINE__); 
    return;
}

module_init(skbinit);
module_exit(skbexit);
