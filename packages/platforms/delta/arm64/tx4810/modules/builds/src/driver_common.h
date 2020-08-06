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


#ifndef DRIVER_COMMON_H
#define DRIVER_COMMON_H

#include <linux/version.h>
#if defined(__aarch64__)
	#include <linux/slab.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    #include <linux/autoconf.h>
#else
    #include <generated/autoconf.h>
#endif

#else
    #include <linux/config.h>

#endif

#include <linux/fs.h>

#ifndef MST_NO_CONFIG_COMPAT
#define MST_NO_CONFIG_COMPAT 0
#endif
#ifndef HAVE_COMPAT_IOCTL
#define HAVE_COMPAT_IOCTL (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
#endif

#if defined(CONFIG_COMPAT) && ! MST_NO_CONFIG_COMPAT && !HAVE_COMPAT_IOCTL

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12) || defined(CONFIG_PPC64)
#include <linux/ioctl32.h>
#else
#include <asm/ioctl32.h>
#endif

#endif


#endif

#define SEMNAME(dev) strrchr(dev, '/')
