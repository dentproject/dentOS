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
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/dmapool.h>

#include "driver_common.h"
#include "mst_pciconf.h"
#define  INIT               PCICONF_INIT
#define  STOP               PCICONF_STOP
#define  READ4              PCICONF_READ4
#define  READ4_NEW          PCICONF_READ4_NEW
#define  WRITE4             PCICONF_WRITE4
#define  WRITE4_NEW         PCICONF_WRITE4_NEW
#define  MODIFY             PCICONF_MODIFY
#define  READ4_BUFFER       PCICONF_READ4_BUFFER
#define  READ4_BUFFER_EX    PCICONF_READ4_BUFFER_EX
#define  WRITE4_BUFFER      PCICONF_WRITE4_BUFFER
#define  MST_PARAMS         PCICONF_MST_PARAMS
#define  MST_META_DATA      PCICONF_MST_META_DATA

/* Versions */
#define MST_HDR_VERSION 1

/* Command: MST_META_DATA versions*/
#define MST_META_DATA_VERSION_MAJOR 1
#define MST_META_DATA_VERSION_MINOR 0
#define MST_API_VERSION_MAJOR 1
#define MST_API_VERSION_MINOR 0

static char* name="mst_pciconf";


#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

#if defined(CONFIG_COMPAT) && CONFIG_COMPAT && !(defined(HAVE_COMPAT_IOCTL) && HAVE_COMPAT_IOCTL)
static unsigned int
ioctl_cmds[] ={
    INIT,
    READ4,
    WRITE4,
    MODIFY,
    READ4_OLD,
    WRITE4_OLD,
    MODIFY_OLD
};
#endif

/* Allow minor numbers 0-255 */
#define MAXMINOR 256
#define BUFFER_SIZE 256
#define MLNX_VENDOR_SPECIFIC_CAP_ID 0x9
#define CRSPACE_DOMAIN 0x2
#define AS_ICMD      0x3
#define AS_CR_SPACE  0x2
#define AS_SEMAPHORE 0xa

#define DMA_MBOX_SIZE (1 << PAGE_SHIFT)

MODULE_AUTHOR("Adrian Chiris - Mellanox Technologies LTD");
MODULE_DESCRIPTION("Mellanox configuration registers access driver (pci conf)");

struct dma_props_internal {
    void *mem_pa;
    dma_addr_t dma_map;
    unsigned int mem_size;
};

#ifdef RETPOLINE_MLNX
MODULE_INFO(retpoline, "Y");
#endif

struct dev_data {
    struct pci_dev* pci_dev;
    int addr_reg;
    int data_reg;
    int wo_addr;
    volatile int valid; // dotanb-cr: use pci_dev as valid
    struct semaphore sem;
    char buf[BUFFER_SIZE];
    int bufused;
    /* Vendor specific capability address */
    int vendor_specific_cap;
    /* status mask on VSEC supported capabilities*/
    u_int32_t vsec_cap_mask;
    /* multipurpose DMA properity */
    struct dma_props_internal dma_props[MST_DMA_END];
};

static struct dev_data devices[MAXMINOR]; // dotanb-cr: use dynamic alloc

typedef enum {
    S_ICMD = 0x1,
    S_CRSPACE = 0x2,
    S_ALL_ICMD = 0x3,
    S_NODNIC_INIT_SEG = 0x4,
    S_EXPANSION_ROM = 0x5,
    S_ND_CRSPACE = 0x6,
    S_SCAN_CRSPACE = 0x7,
    S_GLOBAL_SEMAPHORE = 0xa,
    S_MAC = 0xf,
    S_END
} Space;

typedef enum {
    VC_INITIALIZED = 0x0,
    VC_ICMD_SPACE_SUPPORTED = 0x1,
    VC_CRSPACE_SPACE_SUPPORTED = 0x2,
    VC_ALL_ICMD_SPACE_SUPPORTED = 0x3,
    VC_NODNIC_INIT_SEG_SPACE_SUPPORTED = 0x4,
    VC_EXPANSION_ROM_SPACE_SUPPORTED = 0x5,
    VC_ND_CRSPACE_SPACE_SUPPORTED = 0x6,
    VC_SCAN_CRSPACE_SPACE_SUPPORTED = 0x7,
    VC_GLOBAL_SEMAPHORE_SPACE_SUPPORTED = 0x8,
    VC_MAC_SPACE_SUPPORTED = 0x9,
} VSCCap;

/*
 * Read/Write CR-Space (Old Cap)
 */
static int write4_old(struct dev_data* dev, unsigned int offset, unsigned int data)
{
    int ret;

    if (dev->wo_addr) {
        /*
         * Write operation with new WO GW
         * 1. Write data
         * 2. Write address
         */

        //printk("WRITE DATA: %#x, TO %#x\n", data, dev->data_reg);
        ret=pci_write_config_dword(dev->pci_dev, dev->data_reg, data);
        if (ret) return ret;
        //printk("WRITE ADDR: %#x, to %#x\n", offset, dev->addr_reg);
        ret=pci_write_config_dword(dev->pci_dev, dev->addr_reg, offset);
    } else {
        ret=pci_write_config_dword(dev->pci_dev, dev->addr_reg, offset);
        if (ret) return ret;

        ret=pci_write_config_dword(dev->pci_dev, dev->data_reg, data);
    }
    return ret;
}

static int read4_old(struct dev_data* dev, unsigned int offset, unsigned int* data)
{
    int ret;
    unsigned int newOffset = offset;
    if (dev->wo_addr) {
        /*
         * Read operation, Address LSB should be 1
         */
        newOffset = offset | 0x1;
    }
    //printk("WRITE ADDR: %#x, to %#x\n", newOffset, dev->addr_reg);
    ret=pci_write_config_dword(dev->pci_dev, dev->addr_reg, newOffset);
    if (ret) return ret;

    ret=pci_read_config_dword(dev->pci_dev, dev->data_reg, data);
    //printk("READ DATA: %#x, from %#x\n", *data, dev->data_reg);
    return ret;
}

/*
 * Read/Write Address-Domain (New Cap)
 */

// BIT Slicing macros
#define ONES32(size)                    ((size)?(0xffffffff>>(32-(size))):0)
#define MASK32(offset,size)             (ONES32(size)<<(offset))

#define EXTRACT_C(source,offset,size)   ((((unsigned)(source))>>(offset)) & ONES32(size))
#define EXTRACT(src,start,len)          (((len)==32)?(src):EXTRACT_C(src,start,len))

#define MERGE_C(rsrc1,rsrc2,start,len)  ((((rsrc2)<<(start)) & (MASK32((start),(len)))) | ((rsrc1) & (~MASK32((start),(len)))))
#define MERGE(rsrc1,rsrc2,start,len)    (((len)==32)?(rsrc2):MERGE_C(rsrc1,rsrc2,start,len))


/* PCI address space related enum*/
enum {
    PCI_CAP_PTR = 0x34,
    PCI_HDR_SIZE = 0x40,
    PCI_EXT_SPACE_ADDR = 0xff,

    PCI_CTRL_OFFSET = 0x4, // for space / semaphore / auto-increment bit
    PCI_COUNTER_OFFSET = 0x8,
    PCI_SEMAPHORE_OFFSET = 0xc,
    PCI_ADDR_OFFSET = 0x10,
    PCI_DATA_OFFSET = 0x14,

    PCI_FLAG_BIT_OFFS = 31,

    PCI_SPACE_BIT_OFFS = 0,
    PCI_SPACE_BIT_LEN = 16,

    PCI_STATUS_BIT_OFFS = 29,
    PCI_STATUS_BIT_LEN = 3,
};

/* Mellanox vendor specific enum */
enum {
    CAP_ID = 0x9,
    IFC_MAX_RETRIES = 0x10000,
    SEM_MAX_RETRIES = 0x1000
};

/* PCI operation enum(read or write)*/
enum {
    READ_OP = 0,
    WRITE_OP = 1,
};

#define VSEC_MIN_SUPPORT_KERN(dev) (((dev)->vsec_cap_mask & (1 << VC_INITIALIZED)) && \
                                    ((dev)->vsec_cap_mask & (1 << VC_CRSPACE_SPACE_SUPPORTED)) && \
                                    ((dev)->vsec_cap_mask & (1 << VC_ALL_ICMD_SPACE_SUPPORTED)) && \
                                    ((dev)->vsec_cap_mask & (1 << VC_GLOBAL_SEMAPHORE_SPACE_SUPPORTED)))

// VSEC supported macro
#define VSEC_FULLY_SUPPORTED(dev) (((dev)->vendor_specific_cap) && (VSEC_MIN_SUPPORT_KERN(dev)))

#define VSEC_ASSUMED_SUPPORTED(dev) (((dev)->vendor_specific_cap) && \
                             (VSEC_MIN_SUPPORT_KERN(dev) || (!((dev)->vsec_cap_mask & (1 << VC_INITIALIZED)))))

static int _vendor_specific_sem(struct dev_data* dev, int state)
{
    u32 lock_val;
    u32 counter = 0;
    int retries = 0;
    int ret;
    if (!state) {// unlock
        ret = pci_write_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_SEMAPHORE_OFFSET, 0);
        if (ret) return ret;
    } else { // lock
        do {
            if (retries > SEM_MAX_RETRIES) {
                return -1;
            }
            // read semaphore untill 0x0
            ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_SEMAPHORE_OFFSET, &lock_val);
            if (ret) return ret;

            if (lock_val) { //semaphore is taken
                retries++;
                msleep(1); // wait for current op to end
                continue;
            }
            //read ticket
            ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_COUNTER_OFFSET, &counter);
            if (ret) return ret;
            //write ticket to semaphore dword
            ret = pci_write_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_SEMAPHORE_OFFSET, counter);
            if (ret) return ret;
            // read back semaphore make sure ticket == semaphore else repeat
            ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_SEMAPHORE_OFFSET, &lock_val);
            if (ret) return ret;
            retries++;
        } while (counter != lock_val);
    }
    return 0;
}

static int _wait_on_flag(struct dev_data* dev, u8 expected_val)
{
    int retries = 0;
    int ret;
    u32 flag;
    do {
         if (retries > IFC_MAX_RETRIES) {
             return -1;
         }

         ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_ADDR_OFFSET, &flag);
         if (ret) return ret;

         flag = EXTRACT(flag, PCI_FLAG_BIT_OFFS, 1);
         retries++;
         if ((retries & 0xf) == 0) {// dont sleep always
             //usleep_range(1,5);
         }
     } while (flag != expected_val);
    return 0;
}

static int _space_to_cap_offset(u16 space)
{
    switch (space) {
    case S_ICMD:
        return VC_ICMD_SPACE_SUPPORTED;
    case S_CRSPACE:
        return VC_CRSPACE_SPACE_SUPPORTED;
    case S_ALL_ICMD:
        return VC_ALL_ICMD_SPACE_SUPPORTED;
    case S_NODNIC_INIT_SEG:
        return VC_NODNIC_INIT_SEG_SPACE_SUPPORTED;
    case S_EXPANSION_ROM:
        return VC_EXPANSION_ROM_SPACE_SUPPORTED;
    case S_ND_CRSPACE:
        return VC_ND_CRSPACE_SPACE_SUPPORTED;
    case S_SCAN_CRSPACE:
        return VC_SCAN_CRSPACE_SPACE_SUPPORTED;
    case S_GLOBAL_SEMAPHORE:
        return VC_GLOBAL_SEMAPHORE_SPACE_SUPPORTED;
    case S_MAC:
        return VC_MAC_SPACE_SUPPORTED;
    default:
        return 0;
    }
}

static int _set_addr_space(struct dev_data* dev, u16 space)
{
    // read modify write
    u32 val;
    int ret;
    ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_CTRL_OFFSET, &val);
    if (ret) return ret;
    val = MERGE(val, space, PCI_SPACE_BIT_OFFS, PCI_SPACE_BIT_LEN);
    ret = pci_write_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_CTRL_OFFSET, val);
    if (ret) return ret;
    // read status and make sure space is supported
    ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_CTRL_OFFSET, &val);
    if (ret) return ret;

    if (EXTRACT(val, PCI_STATUS_BIT_OFFS, PCI_STATUS_BIT_LEN) == 0) {
        //printk("[MST]: CRSPACE %d is not supported !\n", space);
        return -1;
    }
    //printk("[MST]: CRSPACE %d is supported !\n", space);
    return 0;
}

static int _pciconf_rw(struct dev_data* dev, unsigned int offset, u32* data, int rw)
{
    int ret = 0;
    u32 address = offset;

    //last 2 bits must be zero as we only allow 30 bits addresses
    if (EXTRACT(address, 30, 2)) {
        return -1;
    }

    address = MERGE(address,(rw ? 1 : 0), PCI_FLAG_BIT_OFFS, 1);
    if (rw == WRITE_OP) {
        // write data
        ret = pci_write_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_DATA_OFFSET, *data);
        if (ret) return ret;
        // write address
        ret = pci_write_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_ADDR_OFFSET, address);
        if (ret) return ret;
        // wait on flag
        ret = _wait_on_flag(dev, 0);
    } else {
        // write address
        ret = pci_write_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_ADDR_OFFSET, address);
        if (ret) return ret;
        // wait on flag
        ret = _wait_on_flag(dev, 1);
        // read data
        ret = pci_read_config_dword(dev->pci_dev, dev->vendor_specific_cap + PCI_DATA_OFFSET, data);
        if (ret) return ret;
    }
    return ret;
}

static int _send_pci_cmd_int(struct dev_data* dev, int space, unsigned int offset, u32* data, int rw)
{
    int ret = 0;

    // take semaphore
    ret = _vendor_specific_sem(dev, 1);
    if (ret) {
        return ret;
    }
    // set address space
    ret = _set_addr_space(dev, space);
    if (ret) {
        goto cleanup;
    }
    // read/write the data
    ret = _pciconf_rw(dev, offset, data, rw);
cleanup:
    // clear semaphore
    _vendor_specific_sem(dev, 0);
    return ret;
}

static int _block_op(struct dev_data* dev, int space, unsigned int offset, int size, u32* data, int rw)
{
    int i;
    int ret = 0;
    int wrote_or_read = size;
    if (size % 4) {
        return -1;
    }
    // lock semaphore and set address space
    ret = _vendor_specific_sem(dev, 1);
    if (ret) {
         return -1;
    }
    // set address space
    ret = _set_addr_space(dev, space);
    if (ret) {
        wrote_or_read = -1;
        goto cleanup;
    }

    for (i = 0; i < size ; i += 4) {
        if (_pciconf_rw(dev, offset + i, &(data[(i >> 2)]), rw)) {
            wrote_or_read = i;
            goto cleanup;
        }
    }
cleanup:
    _vendor_specific_sem(dev, 0);
    return wrote_or_read;
}

static int write4_new(struct dev_data* dev, int addresss_domain, unsigned int offset, unsigned int data)
{
    int ret;

    ret = _send_pci_cmd_int(dev, addresss_domain, offset, &data, WRITE_OP);
    if (ret) {
        return -1;
    }
    return 0;
}

static int read4_new(struct dev_data* dev, int address_space, unsigned int offset, unsigned int* data)
{
    int ret;

    ret = _send_pci_cmd_int(dev, address_space, offset, data, READ_OP);
    if (ret) {
        return -1;
    }
    return 0;
}

static int write4_block_new(struct dev_data* dev, int address_space, unsigned int offset, int size, u32* data)
{
    return _block_op(dev, address_space, offset, size, data, WRITE_OP);
}

static int read4_block_new(struct dev_data* dev, int address_space, unsigned int offset, int size, u32* data)
{
    return _block_op(dev, address_space, offset, size, data, READ_OP);
}

// Turning on the space capability bit in vsec_cap_mask iff
// space capability supported
static int get_space_support_status(struct dev_data* dev, u_int16_t space)
{
    int status = _set_addr_space(dev, space) == 0 ? 1 : 0;
    dev->vsec_cap_mask |= (status << _space_to_cap_offset(space));
    return status;
}

static int get_vsec_cap_mask(struct dev_data* dev)
{
    int ret;
    //printk("[MST] Checking if the Vendor CAP %d supports the SPACES in devices\n", vend_cap);
    if (!dev->vendor_specific_cap) {
        return 0;
    }
    if (dev->vsec_cap_mask & (1 << VC_INITIALIZED)) {
        return 0;
    }
    // take semaphore
    ret = _vendor_specific_sem(dev, 1);
    if (ret) {
        //printk("[MST] Failed to lock semaphore\n");
        return 1;
    }

    get_space_support_status(dev, S_ICMD);
    get_space_support_status(dev, S_CRSPACE);
    get_space_support_status(dev, S_ALL_ICMD);
    get_space_support_status(dev, S_NODNIC_INIT_SEG);
    get_space_support_status(dev, S_EXPANSION_ROM);
    get_space_support_status(dev, S_ND_CRSPACE);
    get_space_support_status(dev, S_SCAN_CRSPACE);
    get_space_support_status(dev, S_GLOBAL_SEMAPHORE);
    get_space_support_status(dev, S_MAC);
    dev->vsec_cap_mask |= (1 << VC_INITIALIZED);
    // clear semaphore
    _vendor_specific_sem(dev, 0);
    return 0;
}

/*
 * End of Capabilities section
 */
#define WO_REG_ADDR_DATA 0xbadacce5
#define DEVID_OFFSET     0xf0014
int is_wo_gw(struct pci_dev* pcidev, unsigned addr_reg)
{
    int ret;
    unsigned int data = 0;
    ret = pci_write_config_dword(pcidev, addr_reg, DEVID_OFFSET);
    if (ret) {
        return 0;
    }
    ret = pci_read_config_dword(pcidev, addr_reg, &data);
    if (ret) {
        return 0;
    }
    if ( data == WO_REG_ADDR_DATA ) {
        return 1;
    }
    return 0;
}

static void init_dma(struct dev_data* dev)
{
/*
    if((pci_set_dma_mask (dev->pci_dev, DMA_BIT_MASK(64)) >= 0) ){

        dev->dma_props[MST_DMA_ICMD].mem_pa = kmalloc (DMA_MBOX_SIZE, GFP_DMA | GFP_KERNEL);
        if (!dev->dma_props[MST_DMA_ICMD].mem_pa) {
            return;
        }
        dev->dma_props[MST_DMA_ICMD].mem_size = DMA_MBOX_SIZE;
        memset(dev->dma_props[MST_DMA_ICMD].mem_pa, 0, DMA_MBOX_SIZE);
        dev->dma_props[MST_DMA_ICMD].dma_map = pci_map_single(dev->pci_dev, dev->dma_props[MST_DMA_ICMD].mem_pa, DMA_MBOX_SIZE, DMA_BIDIRECTIONAL);
        if (!dev->dma_props[MST_DMA_ICMD].dma_map) {
            kfree(dev->dma_props[MST_DMA_ICMD].mem_pa);
            dev->dma_props[MST_DMA_ICMD].mem_pa = 0;
        }

    } else {
    }
*/
}

static void close_dma(struct dev_data* dev)
{
    int i = 0;
    for (i = 0; i < MST_DMA_END; i++) {
        if (dev->dma_props[i].mem_pa) {
            pci_unmap_single(dev->pci_dev, dev->dma_props[i].dma_map, DMA_MBOX_SIZE, DMA_BIDIRECTIONAL);
            kfree(dev->dma_props[i].mem_pa);
            dev->dma_props[i].mem_pa = NULL;
        }
    }
}

static int ioctl (struct inode *inode, struct file *file, unsigned int opcode, unsigned long udata_l)
{
    void* udata=(void*)udata_l;
    int minor=MINOR(inode->i_rdev);
    struct dev_data* dev=&devices[minor];
    int ret=0;
    unsigned int d;

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

    switch (opcode) {

    case INIT:
        {
            struct mst_pciconf_init_st initd;
            struct pci_bus *bus = NULL;
            /* Now have to init the new device */

            if (copy_from_user(&initd, udata, sizeof(initd))) {
                ret=-EFAULT;
                goto fin;
            }

            dev->pci_dev = NULL;
            if (initd.domain == -1) {
                initd.domain = 0;
            }
            bus = pci_find_bus(initd.domain, initd.bus);

            if (!bus) {
                ret=-ENXIO;
                goto fin;
            }

            dev->pci_dev = pci_get_slot (bus, initd.devfn);

            if (!dev->pci_dev) {
                ret=-ENXIO;
                goto fin;
            }
            dev->valid=1;
            dev->bufused=0;
            /* Old Cap for CR-Access*/
            dev->addr_reg=initd.addr_reg;
            dev->data_reg=initd.data_reg;
            /*
             * Check LiveFish GW mode
             */
            dev->wo_addr = is_wo_gw(dev->pci_dev, initd.addr_reg);
//          printk("%04x:%02x:%02x.%0x - ADDR REG WO: %d\n", pci_domain_nr(dev->pci_dev->bus),
//                  dev->pci_dev->bus->number, PCI_SLOT(dev->pci_dev->devfn),
//                  PCI_FUNC(dev->pci_dev->devfn), dev->wo_addr);
            /* New Cap for CR-Access*/
            dev->vendor_specific_cap = pci_find_capability(dev->pci_dev, MLNX_VENDOR_SPECIFIC_CAP_ID);
            dev->vsec_cap_mask = 0;
            init_dma(dev);
            goto fin;
        }

    case STOP:
        close_dma(dev);
        if (dev->valid) {
            pci_dev_put(dev->pci_dev);
        }
        dev->valid=0;
        goto fin;

    case WRITE4_NEW:
    {
        struct mst_write4_new_st write4d;
        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (copy_from_user(&write4d, udata, sizeof(write4d))) {
            ret=-EFAULT;
            goto fin;
        }
        if (get_vsec_cap_mask(dev)) {
            ret=-EBUSY;
            goto fin;
        }
        if (VSEC_FULLY_SUPPORTED(dev)) {
            ret = write4_new(dev, write4d.address_space, write4d.offset, write4d.data);
        } else {
            ret = write4_old(dev, write4d.offset, write4d.data);
        }
        goto fin;
    }
    case WRITE4:
        {
            struct mst_write4_st write4d;
            if (!dev->valid) {
                ret=-ENOTTY;
                goto fin;
            }

            if (copy_from_user(&write4d, udata, sizeof(write4d))) {
                ret=-EFAULT;
                goto fin;
            }

            ret = write4_old(dev, write4d.offset, write4d.data);

            goto fin;
        }

    case READ4_NEW:
    {
        struct mst_read4_new_st read4d;
        struct mst_read4_new_st* r_udata=(struct mst_read4_new_st*)udata;

        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (copy_from_user(&read4d, udata, sizeof(read4d))) {
            ret=-EFAULT;
            goto fin;
        }

        if (get_vsec_cap_mask(dev)) {
            ret=-EBUSY;
            goto fin;
        }
        if ( VSEC_FULLY_SUPPORTED(dev)) {
            ret = read4_new(dev, read4d.address_space, read4d.offset, &d);
        } else {
            ret = read4_old(dev, read4d.offset, &d);
        }
        if (ret) goto fin;
        ret=put_user(d,&(r_udata->data))?-EFAULT:0;
        goto fin;
    }
    case READ4:
    {
        struct mst_read4_st read4d;
        struct mst_read4_st* r_udata=(struct mst_read4_st*)udata;

        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (copy_from_user(&read4d, udata, sizeof(read4d))) {
            ret=-EFAULT;
            goto fin;
        }

        ret = read4_old(dev, read4d.offset, &d);
        if (ret) goto fin;
        ret=put_user(d,&(r_udata->data))?-EFAULT:0;
        goto fin;
    }
    case MODIFY:
        {
            struct mst_modify_st modifyd;
            struct mst_modify_st* m_udata=(struct mst_modify_st*)udata;

            if (!dev->valid) {
                ret=-ENOTTY;
                goto fin;
            }
            if (copy_from_user(&modifyd, udata, sizeof(modifyd))) {
                ret=-EFAULT;
                goto fin;
            }

            if (get_vsec_cap_mask(dev)) {
                ret=-EBUSY;
                goto fin;
            }

            // Read
            if (VSEC_FULLY_SUPPORTED(dev)) {
                ret = read4_new(dev, modifyd.address_space, modifyd.offset, &d);
            } else {
                ret = read4_old(dev, modifyd.offset, &d);
            }
            if (ret) goto fin;
            // Modify
            d= (d & ~modifyd.mask) | (modifyd.data & modifyd.mask);
            // Write
            if (VSEC_FULLY_SUPPORTED(dev)) {
                ret = write4_new(dev, modifyd.address_space, modifyd.offset, d);
            } else {
                ret = write4_old(dev, modifyd.offset, d);
            }
            if (ret) goto fin;

            ret=put_user(d,&(m_udata->old_data))?-EFAULT:0;
            goto fin;
        }
    case READ4_BUFFER_EX:
    case READ4_BUFFER:
    {
        struct mst_read4_buffer_st read4_buf;
        struct mst_read4_buffer_st* rb_udata=(struct mst_read4_buffer_st*)udata;

        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (get_vsec_cap_mask(dev)) {
            ret=-EBUSY;
            goto fin;
        }

        if (!VSEC_MIN_SUPPORT_KERN(dev)) {
            ret = -ENOSYS;
            goto fin;
        }

        if (copy_from_user(&read4_buf, udata, sizeof(read4_buf))) {
            ret=-EFAULT;
            goto fin;
        }

        ret = read4_block_new(dev, read4_buf.address_space, read4_buf.offset, read4_buf.size, read4_buf.data);
        if (ret != read4_buf.size) goto fin;

        ret = copy_to_user(rb_udata, &read4_buf, sizeof(read4_buf)) ? -EFAULT : read4_buf.size;
        goto fin;
    }
    case WRITE4_BUFFER:
    {
        struct mst_write4_buffer_st write4_buf;
        struct mst_write4_buffer_st* wb_udata=(struct mst_write4_buffer_st*)udata;

        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (get_vsec_cap_mask(dev)) {
            ret=-EBUSY;
            goto fin;
        }

        if (!VSEC_MIN_SUPPORT_KERN(dev)) {
            ret = -ENOSYS;
            goto fin;
        }

        if (copy_from_user(&write4_buf, udata, sizeof(write4_buf))) {
            ret=-EFAULT;
            goto fin;
        }

        ret = write4_block_new(dev, write4_buf.address_space, write4_buf.offset, write4_buf.size, write4_buf.data);
        if (ret != write4_buf.size) goto fin;

        ret = copy_to_user(wb_udata, &write4_buf, sizeof(write4_buf)) ? -EFAULT : write4_buf.size;
        goto fin;
    }
    case MST_META_DATA:
    {
        struct mst_meta_data meta_data;
        struct mst_hdr hdr;
        memset(&meta_data, 0, sizeof(meta_data));
        // check hdr
        if (copy_from_user(&hdr, udata, sizeof(struct mst_hdr))) {
            ret=-EFAULT;
            goto fin;
        }

        if (hdr.payload_version_major != MST_META_DATA_VERSION_MAJOR ||
                hdr.payload_len < sizeof(meta_data.data)) {
            ret = -EINVAL;
            goto fin;
        }
        // fill meta_data hdr
        meta_data.hdr.hdr_version = MST_HDR_VERSION;
        meta_data.hdr.hdr_len = sizeof(meta_data.hdr);
        meta_data.hdr.payload_len = sizeof(meta_data.data);
        meta_data.hdr.payload_version_major = MST_META_DATA_VERSION_MAJOR;
        meta_data.hdr.payload_version_minor = MST_META_DATA_VERSION_MINOR;
        // fill payload
        meta_data.data.api_version_major = MST_API_VERSION_MAJOR;
        meta_data.data.api_version_minor = MST_API_VERSION_MINOR;

        ret = copy_to_user(udata, &meta_data, sizeof(meta_data));
        goto fin;
    }
    case MST_PARAMS:
    {
        struct mst_params_st params;
        struct mst_params_st* params_udata = (struct mst_params_st*)udata;
        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }
        // best effort : try to get space spport status if we fail assume we got vsec support.
        get_vsec_cap_mask(dev);

        params.bus = dev->pci_dev->bus->number;
        params.bar = 0;
        params.domain = pci_domain_nr(dev->pci_dev->bus);
        params.func = PCI_FUNC(dev->pci_dev->devfn);
        params.slot = PCI_SLOT(dev->pci_dev->devfn);
        params.device = dev->pci_dev->device;
        params.vendor = dev->pci_dev->vendor;
        params.subsystem_device = dev->pci_dev->subsystem_device;
        params.subsystem_vendor = dev->pci_dev->subsystem_vendor;
        if (VSEC_ASSUMED_SUPPORTED(dev)) { // assume supported if uninitialized (since semaphore is locked)
            params.vendor_specific_cap = dev->vendor_specific_cap;
            params.vsec_cap_mask = dev->vsec_cap_mask;
        } else {
            params.vendor_specific_cap = 0;
        }
        params.multifunction = dev->pci_dev->multifunction;
        ret = copy_to_user(params_udata, &params, sizeof(params));
        goto fin;
    }
    case PCICONF_VPD_READ4:
#if (LINUX_VERSION_CODE >= 0x02061D)
    {
        struct mst_vpd_read4_st vpd_read4;
        struct mst_vpd_read4_st* r_udata = (struct mst_vpd_read4_st*)udata;
        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (copy_from_user(&vpd_read4, udata, sizeof(vpd_read4))) {
            ret=-EFAULT;
            goto fin;
        }

        ret = pci_read_vpd(dev->pci_dev, vpd_read4.offset, 4, &d);
        if (ret < 1) {
            ret=-EFAULT;
            goto fin;
        }
        ret = put_user(d,&(r_udata->data))?-EFAULT:0;
        goto fin;
    }
#endif
    case PCICONF_VPD_WRITE4:
#if (LINUX_VERSION_CODE >= 0x02061D)
    {
        struct mst_vpd_write4_st vpd_write4;

        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        if (copy_from_user(&vpd_write4, udata, sizeof(vpd_write4))) {
            ret=-EFAULT;
            goto fin;
        }

        ret = pci_write_vpd(dev->pci_dev, vpd_write4.offset, 4, &vpd_write4.data);
        goto fin;
    }
#else
    {
        ret= -ENOTTY;
        goto fin;
    }
#endif
    case PCICONF_DMA_PROPS:
    {
        struct mst_dma_props_st dma_props;
        struct mst_dma_props_st* dma_props_udata = (struct mst_dma_props_st*)udata;
        int i;
        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }

        for (i = 0; i < MST_DMA_END; i++) {
            dma_props.dma_props[i].dma_pa = (unsigned long long int)dev->dma_props[i].dma_map;
            dma_props.dma_props[i].mem_size = dev->dma_props[i].mem_size;
        }
        ret = copy_to_user(dma_props_udata, &dma_props, sizeof(dma_props));
        goto fin;
    }
    case PCICONF_MEM_ACCESS:
    {
        struct mst_mem_access_st mem_access;
        struct mst_mem_access_st* mm_udata=(struct mst_mem_access_st*)udata;


        if (!dev->valid) {
            ret=-ENOTTY;
            goto fin;
        }
        if (copy_from_user(&mem_access, udata, sizeof(mem_access))) {
            ret=-EFAULT;
            goto fin;
        }
        if (mem_access.mem_type >= MST_DMA_END) {
            ret=-EFAULT;
            goto fin;
        }
        if (!dev->dma_props[mem_access.mem_type].mem_pa) {
            ret=-EINVAL;
            goto fin;
        }
        if (mem_access.size + mem_access.offset > dev->dma_props[mem_access.mem_type].mem_size) {
            ret=-EINVAL;
            goto fin;
        }
        dma_sync_single_for_cpu(&dev->pci_dev->dev, dev->dma_props[mem_access.mem_type].dma_map, dev->dma_props[mem_access.mem_type].mem_size, DMA_BIDIRECTIONAL);
        if (mem_access._rw == 0) {
            /* READ */
            memcpy(mem_access.data, dev->dma_props[mem_access.mem_type].mem_pa + mem_access.offset, mem_access.size);
        } else {
            memcpy(dev->dma_props[mem_access.mem_type].mem_pa + mem_access.offset, mem_access.data, mem_access.size);
        }
        dma_sync_single_for_device(&dev->pci_dev->dev, dev->dma_props[mem_access.mem_type].dma_map, dev->dma_props[mem_access.mem_type].mem_size, DMA_BIDIRECTIONAL);
        ret = copy_to_user(mm_udata, &mem_access, sizeof(mem_access));
        break;
    }
    default:
        ret= -ENOTTY;
        goto fin;
    }

    fin:
    up(&dev->sem);
    return ret;
}

#include "driver_common.c"


