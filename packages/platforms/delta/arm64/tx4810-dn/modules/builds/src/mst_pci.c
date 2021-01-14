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


#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
//"linux/uaccess.h" was added in 2.6.18
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif
#include <linux/pci.h>
#include <linux/version.h>

#include "driver_common.h"
#include "mst_pci.h"

#define  INIT   PCI_INIT
#define  STOP   PCI_STOP
#define  PCI_PARAMS_ PCI_PARAMS
#define  CONNECTX_WA PCI_CONNECTX_WA

static char* name="mst_pci";

#if defined(CONFIG_COMPAT) && !HAVE_COMPAT_IOCTL
static unsigned int
ioctl_cmds[] ={
	INIT,
	PCI_PARAMS_,
	CONNECTX_WA
};
#endif

/* Allow minor numbers 0-255 */
#define MAXMINOR 256
#define BUFFER_SIZE 256

MODULE_AUTHOR("Adrian Chiris - Mellanox Technologies LTD");
MODULE_DESCRIPTION("Mellanox configuration registers access driver (pci memory)");

#ifdef RETPOLINE_MLNX
MODULE_INFO(retpoline, "Y");
#endif

struct dev_data {
	struct pci_dev* pci_dev;
	unsigned long long bar;
	volatile ssize_t size;
	struct semaphore sem;
	char buf[BUFFER_SIZE];
	int bufused;
	unsigned char connectx_wa_slots;
};

static struct dev_data devices[MAXMINOR];

static int ioctl (struct inode *inode, struct file *file, unsigned int opcode, unsigned long udata_l)
{
	void* udata=(void*)udata_l;
	int minor=MINOR(inode->i_rdev);
	struct dev_data* dev=&devices[minor];
	int ret=0;

	/* By convention, any user gets read access
	 * and is allowed to use the device.
	 * Commands with no direction are administration
	 * commands, and you need write permission
	 * for this */

	if ( _IOC_DIR(opcode) == _IOC_NONE ) {
		if (! ( file->f_mode & FMODE_WRITE) ) return -EPERM;
	} else {
		if (! ( file->f_mode & FMODE_READ) ) return -EPERM;
	}

	if (down_interruptible(&devices[minor].sem)) {
		return -ERESTARTSYS;
	}


	// printk("<1> --------------------- mst - ioctl (%d)\n", opcode);
	switch (opcode) {

	case INIT:
		{
			struct mst_pci_init_st initd;
			struct pci_bus* bus = NULL;
			/* Now have to init the new device */

			/* printk("<1> --------------------- ioctl INIT: copy_from_user(%p, %p/%lx/, %d)\n", */
			/*        &initd, udata, udata_l, (int)sizeof(initd)); */
			/* printk("<1> --------------------- copy_from_user:%d\n", */
			/*        (int)copy_from_user(&initd, udata, (int)sizeof(initd))); */
			if (copy_from_user(&initd, udata, sizeof(initd))) {
				ret=-EFAULT;
				goto fin;
			}

			if (initd.domain == -1) {
				initd.domain = 0;
			}
			bus = pci_find_bus(initd.domain, initd.bus);

			if (!bus) {
				ret=-ENXIO;
				goto fin;
			}
			dev->pci_dev = NULL;
			dev->pci_dev = pci_get_slot (bus, initd.devfn);

			if (!dev->pci_dev) {
				ret=-ENXIO;
				goto fin;
			}

			if (initd.bar >= DEVICE_COUNT_RESOURCE) {
				ret=-ENXIO;
				goto fin;
			}

			dev->bar  = dev->pci_dev->resource[initd.bar].start;
			dev->size = dev->pci_dev->resource[initd.bar].end + 1 - dev->pci_dev->resource[initd.bar].start;
			if (dev->size == 1) {
				dev->size = 0;
			}

			dev->bufused=0;
			goto fin;
		}

	case STOP:
		pci_dev_put(dev->pci_dev);
		devices[minor].size=0;
		goto fin;

	case CONNECTX_WA:
		{
			struct mst_data* md = (struct mst_data*)file->private_data;
			if (md->connectx_wa_slot_p1) {
			    ret = -EPERM;
			    goto fin;
			}
			/* find first un(set) bit. */
			md->connectx_wa_slot_p1 = ffs(~dev->connectx_wa_slots);
			if (md->connectx_wa_slot_p1 == 0 || md->connectx_wa_slot_p1 > CONNECTX_WA_SIZE) {
				ret = -ENOLCK;
			} else {
				unsigned int slot_mask = 1 << (md->connectx_wa_slot_p1 - 1);
				dev->connectx_wa_slots |= slot_mask;

				// printk("MST_PCI: ioctl CONNECTX_WA: Took slot %u. Current slots: %02x\n", md->connectx_wa_slot_p1 - 1, dev->connectx_wa_slots);
				if (copy_to_user(udata, &md->connectx_wa_slot_p1, sizeof(md->connectx_wa_slot_p1))) {
					ret = -EFAULT;
				}
			}

			goto fin;
		}

	case PCI_PARAMS_:
		{
			struct mst_pci_params_st paramsd;
			paramsd.bar=dev->bar;
			paramsd.size=dev->size;

			if (copy_to_user(udata, &paramsd, sizeof(paramsd))) {
				ret=-EFAULT;
			}
			goto fin;
		}

	default:
		ret= -ENOTTY;
		goto fin;
	}

	fin:
	up(&devices[minor].sem);
	return ret;
}

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 */
static inline int noncached_address(unsigned long addr)
{
#if defined(__i386__)
	/*
	 * On the PPro and successors, the MTRRs are used to set
	 * memory types for physical addresses outside main memory,
	 * so blindly setting PCD or PWT on those pages is wrong.
	 * For Pentiums and earlier, the surround logic should disable
	 * caching for the high addresses through the KEN pin, but
	 * we maintain the tradition of paranoia in this code.
	 */
	return !( test_bit(X86_FEATURE_MTRR, (void *)&boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_K6_MTRR, (void *)&boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_CYRIX_ARR, (void *)&boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_CENTAUR_MCR, (void *)&boot_cpu_data.x86_capability) )
	&& addr >= __pa(high_memory);
#else
	return addr >= __pa(high_memory);
#endif
}

static int mmap(struct file *file, struct vm_area_struct *vma)
{

	int ret=0;
	unsigned long off;
	unsigned long vsize;
	unsigned long long offset;
	struct dev_data* dev=((struct mst_data*)file->private_data)->dev;

	if (!dev) return -ENOTTY; /*Shouldn't happend, as open assigns it
				    but just in case */

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	off = vma->vm_pgoff << PAGE_SHIFT;
	vsize = vma->vm_end - vma->vm_start;

	if ((dev->size <= off) || (dev->size < off+vsize)) {
		ret=-EINVAL;
		goto fin;
	}

	offset = dev->bar+off;
	/*
	 *    * Accessing memory above the top the kernel knows about or
	 *    * through a file pointer that was marked O_SYNC will be
	 *    * done non-cached.
	 *    */
	if (noncached_address(offset) || (file->f_flags & O_SYNC))
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret=io_remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT,
			       vsize, vma->vm_page_prot);

	if (ret) goto fin;

	fin:
	up(&dev->sem);
	return ret;
}


static int mst_pci_release(struct inode* inode, struct file* file) {
	struct mst_data* md = file->private_data;

	if (md->connectx_wa_slot_p1) {
		unsigned int mask = 0;

		if (down_interruptible(&md->dev->sem)) {
			return -ERESTARTSYS;
		}

		mask = ~(1 << (md->connectx_wa_slot_p1 - 1));
		md->dev->connectx_wa_slots &= mask;

		// printk("MST_PCI: released slot %ud. Current slots: %02x\n", md->connectx_wa_slot_p1 - 1 , md->dev->connectx_wa_slots);
		md->connectx_wa_slot_p1 = 0;
		up(&md->dev->sem);
	}

	return 0;
}

#define SUPPORT_MMAP 1
#define MST_PCI_DRIVER
#include "driver_common.c"

