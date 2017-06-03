#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("procfs examples");

/*
 * This proc demo works only with kernel version 2.6.31
 */

#define MODULE_VER "1.0"
#define MODULE_NAME "proctest"

#define FOOBAR_LEN 8

struct fb_data_t {
    char name[FOOBAR_LEN + 1];
    char value[FOOBAR_LEN + 1];
};

static struct proc_dir_entry *proctest_dir, *foo_file,
                             *bar_file, *jiffies_file,
                             *symlink;

struct fb_data_t foo_data, bar_data;

static int proc_read_jiffies(char *page, char **start, 
        off_t off, int count, 
        int *eof, void *data)
{
    int len;
    len = sprintf(page, "jiffies = %ld\n", jiffies);
    return len;
}

static int proc_read_foobar(char *page, char **start, 
        off_t off, int count,
        int *eof, void *data)
{
    int len;
    struct fb_data_t *fb_data = (struct fb_data_t *)data;

    len = sprintf(page, "%s = %s\n", fb_data->name, fb_data->value);
    return len;
}

static int proc_write_foobar(struct file *file, const char *buffer,
        unsigned long count, void *data)
{
    int len;
    struct fb_data_t *fb_data = (struct fb_data_t *)data;

    if (count > FOOBAR_LEN)
        len = FOOBAR_LEN;
    else
        len = count;

    if (copy_from_user(fb_data->value, buffer, len))
        return -EFAULT;

    fb_data->value[len] = '\0';

    return len;
}

static int __init proctest_init(void)
{
    int ret = 0;

    proctest_dir = proc_mkdir(MODULE_NAME, NULL);
    if (proctest_dir == NULL) {
        ret = -ENOMEM;
        goto out;
    }
    /*proctest_dir->owner = THIS_MODULE;*/

    jiffies_file = create_proc_read_entry("jiffies", 0444, proctest_dir, proc_read_jiffies, NULL);
    if (jiffies_file == NULL) {
        ret = -ENOMEM;
        goto no_jiffies;
    }

    /*jiffies_file->owner = THIS_MODULE;*/

    foo_file = create_proc_entry("foo", 0644, proctest_dir);
    if (foo_file == NULL) {
        ret = -ENOMEM;
        goto no_foo;
    }

    strcpy(foo_data.name, "foo");
    strcpy(foo_data.value, "foo");
    foo_file->data = &foo_data;
    foo_file->read_proc = proc_read_foobar;
    foo_file->write_proc = proc_write_foobar;
    /*foo_file->owner = THIS_MODULE;*/

    bar_file = create_proc_entry("bar", 0644, proctest_dir);
    if (bar_file == NULL) {
        ret = -ENOMEM;
        goto no_bar;
    }

    strcpy(bar_data.name, "bar");
    strcpy(bar_data.value, "bar");
    bar_file->data = &bar_data;
    bar_file->read_proc = proc_read_foobar;
    bar_file->write_proc = proc_write_foobar;
    /*bar_file->owner = THIS_MODULE;*/

    symlink = proc_symlink("jiffies_too", proctest_dir, "jiffies");
    if (symlink == NULL) {
        ret = -ENOMEM;
        goto no_symlink;
    }
    /*symlink->owner = THIS_MODULE;*/

    printk(KERN_INFO "[%s %d] %s %s initialised\n", __func__, __LINE__, MODULE_NAME, MODULE_VER); 

    return 0;

no_symlink:
    remove_proc_entry("bar", proctest_dir);

no_bar:
    remove_proc_entry("foo", proctest_dir);

no_foo:
    remove_proc_entry("jiffies", proctest_dir);

no_jiffies:
    remove_proc_entry(MODULE_NAME, NULL);

out:
    return ret;
}


static void __exit proctest_exit(void)
{
    remove_proc_entry("jiffies_too", proctest_dir);
    remove_proc_entry("bar", proctest_dir);
    remove_proc_entry("foo", proctest_dir);
    remove_proc_entry("jiffies", proctest_dir);
    remove_proc_entry(MODULE_NAME, NULL);

    printk("[%s %d] %s %s removed\n", __func__, __LINE__, MODULE_NAME, MODULE_VER); 
}

module_init(proctest_init);
module_exit(proctest_exit);
