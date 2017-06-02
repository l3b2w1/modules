#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>

#define MAX_PAYLOAD 1600  /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

/* define yourself info struct to communicate with user if needed */
struct ntlkmsg {
	int type; // msg type
	int len; // data len
	unsigned char data[0];
};


enum {
	CAP_ADD,
	CAP_DEL,
	CAP_GET_PACKET
};


int netlink_init()
{
	int ret = 0;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if (sock_fd < 0) {
		perror("socket");
		exit(-1);
	}

	memset(&msg, 0, sizeof(msg));
	memset(&src_addr, 0, sizeof(src_addr));

	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 0; // not in mcast groups

	ret = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if (ret < 0) {
		perror("bind");
		exit(-1);
	}
}


int recv_from_kernel()
{
	int len = 0;
	unsigned char *pdata = NULL;
	
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));	
	len = recvmsg(sock_fd, &msg, 0);
	pdata = NLMSG_DATA(nlh);
	printf("message len %d, payload: \

		%02x %02x %02x %02x %02x %02x 	%02x %02x %02x %02x %02x %02x\n", len - sizeof(struct nlmsghdr), 
		pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5],
		pdata[6], pdata[7], pdata[8], pdata[9], pdata[10], pdata[11]);
	return 0;
}


int recv_from_kernel_ntlkmsg()
{
	int len = 0;
	struct ntlkmsg *pmsg = NULL;
	int times = 10;

	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));	
	len = recvmsg(sock_fd, &msg, 0);
	pmsg = NLMSG_DATA(nlh);
	printf("data type %d, data len %d\n", pmsg->type, pmsg->len);
	printf("Received message len %d, payload: %s\n", 
				len, (char*)NLMSG_DATA(nlh) + sizeof(struct ntlkmsg));
	return 0;
}



int send_to_kernel(void *data, int len, int type)
{
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; // for linux kernel
	dest_addr.nl_groups = 0;	// unicast


	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (!nlh) {
		printf("malloc nlmsg buffer failure\n");
		exit(-1);
	}

	nlh->nlmsg_type = type;
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	// fill in the netlink message payload
	strncpy(NLMSG_DATA(nlh), data, len);

	iov.iov_base = (void*)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void*)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(sock_fd, &msg, 0);
	printf("send to kernel success! \n");
}

int main(int argc, char **argv)
{
	int len;
	int size;
	int type;
	char *hello = "hello, netlink world";
	char *bye = "Bye, netlink world";
	unsigned char *pmsg = NULL;
	struct ntlkmsg *pnlmsg = NULL;
	int time = 10;

	time = atoi(argv[1]);
	len = strlen(hello) + 1;
	netlink_init();
	send_to_kernel(hello, len, CAP_ADD);
	for (;;) {
		time--;
		recv_from_kernel();
		if (time == 0)
		  break;
	}
	send_to_kernel(bye, len, CAP_DEL); 
}


