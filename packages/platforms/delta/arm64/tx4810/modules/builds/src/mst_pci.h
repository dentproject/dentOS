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


#ifndef H_MST_PCI_H
#define H_MST_PCI_H


/* These will be specific for PCI */
#define PCI_MAGIC 0xD1

#define PCI_INIT _IOC(_IOC_NONE,PCI_MAGIC,0,sizeof(struct mst_pci_init_st))
struct mst_pci_init_st {
	int          domain;
	unsigned int bus;
	unsigned int devfn;
	int bar;
};

#define PCI_STOP _IOC(_IOC_NONE,PCI_MAGIC,1,0)

#define PCI_PARAMS    _IOR(PCI_MAGIC,2, struct mst_pci_params_st)

struct mst_pci_params_st {
	unsigned long long __attribute__((packed)) bar;
	unsigned long long __attribute__((packed)) size;
};

// CONNECTX ORDERRING WA:

#define CONNECTX_WA_BASE 0xf0384 // SEM BASE ADDR. SEM 0xf0380 is reserved for external tools usage.
#define CONNECTX_WA_SIZE 3       // Size in entries

#define PCI_CONNECTX_WA _IOR(PCI_MAGIC,3, u_int32_t)
struct mst_data {
	struct    dev_data* dev;
	u_int32_t connectx_wa_slot_p1; // Hermon used slot plus 1. Zero means unused
};

#endif
