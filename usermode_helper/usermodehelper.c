#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wbl");

/* UHM: user mode helper */
static int uhm_test(void)
{
	int ret = 0;
//	char *argv[] = {"/usr/bin/logger", "help!", NULL};
	char *argv[] = {"/bin/ls", NULL};
	static char *envp[] = {
		"HOME=/",
		"TERM=linux",
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL
	};

	ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	printk("ret %d\n", ret);
	return ret;
}

static int __init mod_entry_func(void)
{
	printk("usermodehelp entry\n");
	return uhm_test();
}

static void __exit mod_exit_func(void)
{
	printk("usermodehelp exit\n");
	return;
}

module_init(mod_entry_func);
module_exit(mod_exit_func);
