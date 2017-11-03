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
    struct tcphdr *th = 0;
    struct net_device *dev = 0;
    struct sk_buff *skb = NULL;
    unsigned char * pdata = 0;  

    dev = dev_get_by_name(&init_net, "eth0");
    if (!dev) {
        printk("[%s %d] get dev eth0  failed\n", __func__, __LINE__); 
        return -1;
    }
    printk("[%s %d] we get dev eth0 success\n", __func__, __LINE__); 
    
    len = sizeof(payload) + LL_RESERVED_SPACE(dev) + sizeof(struct iphdr) + sizeof(struct tcphdr);

    printk("[%s %d] len %d\n", __func__, __LINE__, len); 

    skb = alloc_skb(len, GFP_ATOMIC);
    if (!skb) {
        printk("[%s %d] skb alloc failed\n", __func__, __LINE__); 
        return -ENOMEM;
    }

    printk("[%s %d] alloc_skb ok\n", __func__, __LINE__); 

    printk("ORI truesize %4d, len %4d, data %p, tail %p, head %p, end %p, mac_header %p\n", skb->truesize, skb->len, skb->data, skb->tail, skb->head, skb->end, skb->mac_header); 

    skb_reserve(skb, LL_RESERVED_SPACE(dev));
    printk("RSV truesize %4d, len %4d, data %p, tail %p, head %p, end %p, mac_header %p\n", skb->truesize, skb->len, skb->data, skb->tail, skb->head, skb->end, skb->mac_header); 

    skb->dev = dev;
    skb->pkt_type = PACKET_OTHERHOST;
    skb->protocol = __constant_htons(ETH_P_IP);
    skb->ip_summed = CHECKSUM_NONE;
    skb->priority = 0;

    ih = (struct iphdr *)skb_put(skb, sizeof(struct iphdr));
    ih->version = 4;
    ih->ihl = sizeof(struct iphdr) >> 2;
    ih->frag_off = 0;
    ih->protocol = IPPROTO_IP;
    ih->tos = 0;
    memcpy(&sip, srcip, sizeof(srcip));
    memcpy(&dip, dstip, sizeof(dstip));
    ih->saddr = sip;
    ih->daddr = dip;
    ih->ttl = 0x40;
    ih->tot_len = __constant_htons(skb->len);
    ih->check = 0;

    th = (struct tcphdr *)skb_put(skb, sizeof(struct tcphdr));
    th->source = SPORT;
    th->dest = DPORT;
    th->seq = seq++;
    th->ack_seq = ack_seq;
    th->psh = 1;
    th->fin = 1;
    th->syn = 1;
    th->doff = 5;
    th->window = __constant_htons(5840);
    th->check = 0;

    skb->csum = skb_checksum (skb, ih->ihl*4, skb->len - ih->ihl * 4, 0);
    th->check = csum_tcpudp_magic (sip, dip, skb->len - ih->ihl * 4, IPPROTO_TCP, skb->csum);

    pdata = skb_put(skb, sizeof(payload));
    memcpy(pdata, payload, sizeof(payload));
    printk("PUT truesize %4d, len %4d, data %p, tail %p, head %p, end %p,  mac_header %p\n", skb->truesize, skb->len, skb->data, skb->tail, skb->head, skb->end, skb->mac_header); 

    eh = skb_push(skb, 14);
    eh->h_proto = __constant_htons(ETH_P_IP);
    memcpy(eh->h_dest, dstmac, ETH_ALEN);
    memcpy(eh->h_source, srcmac, ETH_ALEN);
    printk("PSH truesize %4d, len %4d, data %p, tail %p, head %p, end %p, mac_header %p\n", skb->truesize, skb->len, skb->data, skb->tail, skb->head, skb->end, skb->mac_header); 

    if ((ret = dev_queue_xmit(skb)) < 0)
        goto out;
    printk("[%s %d] ret %d\n", __func__, __LINE__, ret); 
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
