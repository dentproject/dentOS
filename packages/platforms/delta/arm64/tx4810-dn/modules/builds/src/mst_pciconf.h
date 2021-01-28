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


#ifndef H_TEMT_PCICONF_H
#define H_TEMT_PCICONF_H




/* These will be specific for PCI CONF*/
#define PCICONF_MAGIC 0xD2
#define PCICONF_MAX_BUFFER_SIZE 256
#define PCICONF_MAX_MEMACCESS_SIZE 1024
#define PCICONF_CAP_VEC_LEN 16

/* Common Structs*/
struct mst_hdr {
    unsigned short hdr_version;
    unsigned short hdr_len;
    unsigned short payload_version_major;
    unsigned int payload_version_minor;
    unsigned int payload_len;
};


#define PCICONF_INIT _IOC(_IOC_NONE,PCICONF_MAGIC,0,sizeof(struct mst_pciconf_init_st))
struct mst_pciconf_init_st {
        int          domain;
        unsigned int bus;
        unsigned int devfn;
  /* Byte offsets in configuration space */
        unsigned int addr_reg;
        unsigned int data_reg;
};

#define PCICONF_STOP _IOC (_IOC_NONE,PCICONF_MAGIC,1,0)

#define PCICONF_READ4  _IOR (PCICONF_MAGIC,1,struct mst_read4_st)
struct mst_read4_st {
        unsigned int offset;
        unsigned int data; /*OUT*/
};

#define PCICONF_WRITE4 _IOW (PCICONF_MAGIC,2,struct mst_write4_st)
struct mst_write4_st {
        unsigned int offset;
        unsigned int data;
};


#define PCICONF_MODIFY _IOWR(PCICONF_MAGIC,3,struct mst_modify_st)
struct mst_modify_st {
        unsigned int address_space;
        unsigned int offset;
        unsigned int data;
        unsigned int mask;
        unsigned int old_data; /*OUT*/
};

#define PCICONF_READ4_BUFFER  _IOR (PCICONF_MAGIC,4,struct mst_read4_st)
struct mst_read4_buffer_st {
        unsigned int address_space;
        unsigned int offset;
        int size;
        unsigned int data[PCICONF_MAX_BUFFER_SIZE/4]; /*OUT*/
};

#define PCICONF_WRITE4_BUFFER _IOW (PCICONF_MAGIC,5,struct mst_write4_buffer_st)
struct mst_write4_buffer_st {
        unsigned int address_space;
        unsigned int offset;
        int size;
        unsigned int data[PCICONF_MAX_BUFFER_SIZE/4]; /*IN*/
};

#define PCICONF_MST_PARAMS  _IOR (PCICONF_MAGIC,6,struct mst_params_st)
struct mst_params_st {
        unsigned int domain;
        unsigned int bus;
        unsigned int slot;
        unsigned int func;
        unsigned int bar;
        unsigned int device;
        unsigned int vendor;
        unsigned int subsystem_device;
        unsigned int subsystem_vendor;
        unsigned int vendor_specific_cap;
        u_int32_t vsec_cap_mask;
        unsigned int multifunction;
};

#define PCICONF_READ4_NEW  _IOR (PCICONF_MAGIC,7,struct mst_read4_new_st)
struct mst_read4_new_st {
        unsigned int address_space;
        unsigned int offset;
        unsigned int data; /*OUT*/
};

/****************************************************/
/* VPD ACCESS */
#define PCICONF_VPD_READ4 _IOR(PCICONF_MAGIC, 7, struct mst_vpd_read4_st)
struct mst_vpd_read4_st {
    unsigned int offset;    /* IN - must be aligned to DWORD */
    unsigned int data;               /* OUT */

};

#define PCICONF_WRITE4_NEW _IOW (PCICONF_MAGIC,8,struct mst_write4_new_st)
struct mst_write4_new_st {
        unsigned int address_space;
        unsigned int offset;
        unsigned int data;
};


#define PCICONF_VPD_WRITE4 _IOW(PCICONF_MAGIC, 8, struct mst_vpd_write4_st)
struct mst_vpd_write4_st {
    unsigned int offset;    /* IN - must be aligned to DWORD */
    unsigned int data;               /* IN */
};

/*
 * MEM_ACCESS
 */


typedef enum {
    MST_DMA_ICMD,
    MST_DMA_END=32
} mst_dma_type_t;


#define PCICONF_MEM_ACCESS _IOWR(PCICONF_MAGIC, 10, struct mst_mem_access_st)
struct mst_mem_access_st {
    mst_dma_type_t mem_type;
    unsigned int _rw; /* READ: 0, WRITE: 1 */
    unsigned int offset;
    unsigned int size;
    unsigned char data[PCICONF_MAX_MEMACCESS_SIZE];
};

#define PCICONF_DMA_PROPS  _IOR (PCICONF_MAGIC, 11, struct mst_dma_props_st)

struct dma_prop {
    unsigned long long int dma_pa;
    unsigned int mem_size;
};
struct mst_dma_props_st {
    struct dma_prop dma_props[MST_DMA_END];
};


#define PCICONF_MST_META_DATA  _IOR (PCICONF_MAGIC, 12, struct mst_meta_data)

struct mst_meta_data_payload {
    unsigned short api_version_major;
    unsigned int api_version_minor;
    unsigned int cap_vector[PCICONF_CAP_VEC_LEN];
};

struct mst_meta_data {
    struct mst_hdr hdr;
    struct mst_meta_data_payload data;
};
#endif
