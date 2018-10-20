#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
 
#define BUFFER_SIZE 1024

static dev_t first;
static struct cdev c_dev;
static struct class *cl;

static char device_buffer[BUFFER_SIZE]; 
static struct file *wf;

static int my_open(struct inode *i, struct file *f)
{
	printk(KERN_INFO "Driver: open()\n");
 	return 0;
}

static int my_close(struct inode *i, struct file *f)
{
	printk(KERN_INFO "Driver: close()\n");
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	int ret;
	mm_segment_t oldfs;
 	
 	printk(KERN_INFO "Driver: read()\n");

 	memset(device_buffer, 0, sizeof device_buffer);

 	oldfs = get_fs();
 	set_fs(get_ds());

 	ret = vfs_read(wf, device_buffer, len, off);
 	
 	set_fs(oldfs);

 	printk(KERN_INFO "Driver: %s\n", device_buffer);

 	return ret;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "Driver: write()\n");
	
	if (!copy_from_user(device_buffer, buf, len))
	{
		return -1;
	}
	
	if (strncmp(device_buffer, "open", 4)) 
	{
		if(!open_file()) {
			printk(KERN_INFO "Driver: can't open file \"%s\"\n", file_name);
			return -1;
		}
	}
	else if(strncmp(device_buffer, "close", 5))
	{
		filp_close(wf, NULL);
	}
	else 
	{
		if(!write_spaces_count()) {
			printk(KERN_INFO "Driver: write error");
			return -1;
		}
	}

	return len;
}

int open_file() 
{
	char file_name[255]; 
	mm_segment_t oldfs;
	
	strncpy(file_name, device_buffer + 5, len - 6);
	file_name[len - 6] = '\0';
	
	oldfs = get_fs();
	set_fs(get_ds());

	wf = filp_open(file_name, O_CREAT|O_RDWR|O_APPEND, 0666);
	
	set_fs(oldfs);

	if(IS_ERR(wf))
	{
		return 1;
	}
	return 0;
}

int write_spaces_count() 
{
	int i = 0;
	int space_count = 0;
	mm_segment_t oldfs;

	for(i = 0; i < len; i++) {
		if(device_buffer[i] == 32) {
			space_count++;
		}
	}

	char result[10];
	sprintf(result, "%d\n", space_count); // int to str

	oldfs = get_fs();
	set_fs(get_ds());

	if(vfs_write(wf, result, strlen(result), off)) {
		set_fs(oldfs);
		return 1;
	}

	set_fs(oldfs);
	return 0;
}

static struct file_operations mychdev_fops =
{
	.owner = THIS_MODULE,
 	.open = my_open,
 	.release = my_close,
 	.read = my_read,
 	.write = my_write
};

static int __init ch_drv_init(void)
{
	printk(KERN_INFO "Hello!\n");
	if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
	{
		return -1;
	}
	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		unregister_chrdev_region(first, 1);
		return -1;
	}
	if (device_create(cl, NULL, first, NULL, "var4") == NULL)
	{
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	cdev_init(&c_dev, &mychdev_fops);
	if (cdev_add(&c_dev, first, 1) == -1)
	{
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	return 0;
}

static void __exit ch_drv_exit(void)
{
	cdev_del(&c_dev);
 	device_destroy(cl, first);
 	class_destroy(cl);
 	unregister_chrdev_region(first, 1);
 	printk(KERN_INFO "Bye!!!\n");
}

module_init(ch_drv_init);
module_exit(ch_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("The first kernel module");
