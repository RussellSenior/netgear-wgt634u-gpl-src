/* r128.h -- ATI Rage 128 DRM template customization -*- linux-c -*-
 * Created: Wed Feb 14 16:07:10 2001 by gareth@valinux.com
 *
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 */

#ifndef __R128_H__
#define __R128_H__

/* This remains constant for all DRM template files.
 */
#define DRM(x) r128_##x

/* General customization:
 */
#define __HAVE_AGP		1
#define __MUST_HAVE_AGP		0
#define __HAVE_MTRR		1
#define __HAVE_CTX_BITMAP	1
#define __HAVE_SG		1
#define __HAVE_PCI_DMA		1

/* Driver customization:
 */
#define DRIVER_PRERELEASE() do {					\
	if ( dev->dev_private ) {					\
		drm_r128_private_t *dev_priv = dev->dev_private;	\
		if ( dev_priv->page_flipping ) {			\
			r128_do_cleanup_pageflip( dev );		\
		}							\
	}								\
} while (0)

#define DRIVER_PRETAKEDOWN() do {					\
	if ( dev->dev_private ) r128_do_cleanup_cce( dev );		\
} while (0)

/* DMA customization:
 */
#define __HAVE_DMA		1


/* Buffer customization:
 */
#define DRIVER_BUF_PRIV_T	drm_r128_buf_priv_t

#define DRIVER_AGP_BUFFERS_MAP( dev )					\
	((drm_r128_private_t *)((dev)->dev_private))->buffers

#endif
