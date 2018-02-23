#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h> /* register_chrdev, unregister_chrdev */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define NAME "test"
#define MSG_BUFFER_LEN 100

static int major = -1;
static struct cdev mycdev;
static struct class *myclass = NULL;

static char str_info[MSG_BUFFER_LEN] = "author Bogoyavlenskiy Ivan\n";

static int show(struct seq_file *m, void *v)
{
    seq_printf(m, "abcd");
    return 0;
}

static int open(struct inode *inode, struct file *file)
{
    return single_open(file, show, NULL);
}

//   ---   ---   D E V   R E A D   ---   ---
static ssize_t dev_read( struct file * file, char * buf,
						size_t count, loff_t *ppos ) {
	int len = strlen( str_info );
	if( count < len ) return -EINVAL;
	if( *ppos != 0 ) {
		return 0;
	}
	if( copy_to_user( buf, str_info, len ) ) return -EINVAL;
	*ppos = len;
	return len;
}

//   ---   ---   D E V   W R I T E   ---   ---
static ssize_t dev_write( struct file *file, const char *buf, size_t count, loff_t *ppos ) 
{
    printk(KERN_WARNING "test length = %i\n", count);
    return count;
}

//   ---   ---   D E V   R E L E A S E   ---   ---
int dev_release(struct inode *inode, struct file *flip)
{
    return 0;
}

static const struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static void cleanup(int device_created)
{
    if (device_created) {
        device_destroy(myclass, major);
        cdev_del(&mycdev);
    }
    if (myclass)
        class_destroy(myclass);
    if (major != -1)
        unregister_chrdev_region(major, 1);
}

static int myinit(void)
{
    int device_created = 0;

    /* cat /proc/devices */
    if (alloc_chrdev_region(&major, 0, 1, NAME "_proc") < 0)
        goto error;
    /* ls /sys/class */
    if ((myclass = class_create(THIS_MODULE, NAME "_sys")) == NULL)
        goto error;
    /* ls /dev/ */
    if (device_create(myclass, NULL, major, NULL, NAME) == NULL)
        goto error;
    device_created = 1;
    cdev_init(&mycdev, &fops);
    if (cdev_add(&mycdev, major, 1) == -1)
        goto error;
    return 0;
error:
    cleanup(device_created);
    return -1;
}

static void myexit(void)
{
    cleanup(1);
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
