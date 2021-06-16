/*
 * Copyright (C) Mar 2012 Mellanox Technologies Ltd. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *  Version: $Id: $
 *
 */

#include <linux/fs.h>

#include <linux/netdevice.h>

#include <linux/init.h>
#include <linux/moduleparam.h>

static int major=-1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	#include <linux/semaphore.h>
#else
	#include <asm/semaphore.h>
#endif

#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

/* TODO:
 * implement READ4/WRITE4 IOCTLS
 * (+more sizes?)
 * implement lseek?
 * http://www.tldp.org/LDP/khg/HyperNews/get/devices/addrxlate.html
 */

#if !defined(PPC_MAGIC) && !defined(MST_PCI_DRIVER)
//#ifndef MST_PCI_DRIVER
struct mst_data {
	struct dev_data* dev;
};
#endif

static int open (struct inode *inode, struct file *file)
{
	int minor=MINOR(inode->i_rdev);

	struct mst_data* md;
	md = kzalloc(sizeof(struct mst_data), GFP_KERNEL);

	if (!md) {
		return -ERESTARTSYS;
	}

	if (down_interruptible(&devices[minor].sem)) {
		return -ERESTARTSYS;
	}

	md->dev = &devices[minor];
	file->private_data=md;

	/* Truncate to 0 length on open for writing */
	if (file->f_flags & O_APPEND) {
		file->f_pos=devices[minor].bufused;
	} else if ( (file->f_flags & O_TRUNC) || (file->f_flags & O_WRONLY ) ) {
		devices[minor].bufused=0;
	}

	/*fin:*/
	up(&devices[minor].sem);
	return 0;
}

static  int release (struct inode *inode, struct file *file)
{
	int rc = 0;
#ifdef MST_PCI_DRIVER
	rc = mst_pci_release(inode, file);
#endif

	// dotanb-cr: clean all active devices.

	kfree(file->private_data);
	return rc;
}

static ssize_t read (struct file * file, char *buf, size_t count, loff_t *f_pos)
{
	struct dev_data* dev=((struct mst_data*)file->private_data)->dev;
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	// dotanb-cr: check device is valid.

	if (*f_pos >= dev->bufused) {
		count=0;
		goto fin;
	}
	if (*f_pos + count > dev->bufused)
		count = dev->bufused - *f_pos;

	if (copy_to_user(buf,dev->buf+*f_pos, count)) {
		count=-EFAULT;
		goto fin;
	}
	*f_pos+=count;

	fin:
	up(&dev->sem);
	return count;
}

static ssize_t write (struct file *file, const char * buf, size_t count, loff_t *f_pos)
{
	struct dev_data* dev=((struct mst_data*)file->private_data)->dev;
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (*f_pos >= BUFFER_SIZE) {
		count=0;
		goto fin;
	}
	if (*f_pos + count > BUFFER_SIZE)
		count = BUFFER_SIZE - *f_pos;

	if (copy_from_user(dev->buf+*f_pos, buf, count)) {
		count=-EFAULT;
		goto fin;
	}
	*f_pos+=count;
	if (dev->bufused < *f_pos) dev->bufused=*f_pos;

	fin:
	up(&dev->sem);
	return count;
}

static long new_ioctl (struct file *f, unsigned int o, unsigned long d)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,18,0)
    struct inode *n = f->f_dentry->d_inode;
#else
    struct inode *n = f->f_path.dentry->d_inode;
#endif

	return ioctl(n, f, o, d);
}

static struct file_operations fops = {
	/*llseek:  scull_llseek,*/
	read:  read,
	write:  write,

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
	ioctl:  ioctl,
#else
	unlocked_ioctl: new_ioctl,
#endif

	open:  open,
	release: release,
#ifdef SUPPORT_MMAP
	mmap: mmap,
#endif
	owner: THIS_MODULE,
#if HAVE_COMPAT_IOCTL
	compat_ioctl: new_ioctl
#endif
};

static int mst_init_module(void)
{
	int i;
	major=register_chrdev(0, name, &fops);
	if (major<=0) {
		printk("Unable to register character device %s\n",name);
		return -1;
	}

#if defined(CONFIG_COMPAT) && ! MST_NO_CONFIG_COMPAT && !HAVE_COMPAT_IOCTL
	{
		int j;
		int rc=0;
		for (i=0;i<sizeof(ioctl_cmds)/sizeof(ioctl_cmds[0]);++i) {
			rc=register_ioctl32_conversion(ioctl_cmds[i], NULL);
			if (rc) {
				printk("Unable to register compatibility ioctl %d[%d] for device %s\n",
				       i, ioctl_cmds[i], name);
				for (j=0;j<i;++j) {
					unregister_ioctl32_conversion(ioctl_cmds[j]);
				}
				unregister_chrdev(major, name);
				return rc;
			}
		}
	}
#endif

	for (i=0;i<MAXMINOR;++i) {
		sema_init(&devices[i].sem,1);

#ifdef MST_PCI_DRIVER
		devices[i].connectx_wa_slots = 0;
#endif
#ifdef MST_PPC_DRIVER
		devices[i].cs=(unsigned int)(-1);
#endif
	}
#if 0
	printk("MST Driver initialised\n");
	printk("Device name:%s\n",name);
	printk("Major number:%d\n",major);
	printk("Minor numbers:%d\n",MAXMINOR);
	printk("IOCTL numbers:\n");
	printk("INIT:0x%x\n", INIT);
	printk("STOP:0x%x\n", STOP);
#ifdef WRITE4
	printk("WRITE4:0x%x\n", WRITE4);
	printk("READ4:0x%x\n",  READ4);
	printk("MODIFY:0x%x\n", MODIFY);
#endif
#ifdef MST_PCI_DRIVER
	printk("PARAMS:0x%x\n", PARAMS);
	printk("CONNECTX_WA:0x%x\n", CONNECTX_WA);


#endif
#endif


	return 0;
}

static void mst_cleanup_module(void)
{
	/* don't have to destroy semaphores in linux */

#if defined(CONFIG_COMPAT) && ! MST_NO_CONFIG_COMPAT && !HAVE_COMPAT_IOCTL
	int i;
	for (i=0;i<sizeof(ioctl_cmds)/sizeof(ioctl_cmds[0]);++i) {
		unregister_ioctl32_conversion(ioctl_cmds[i]);
	}
#endif
	unregister_chrdev(major, name);
}

module_init(mst_init_module);
module_exit(mst_cleanup_module);
