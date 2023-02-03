/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: grf_rb.c 1.13 91/04/02$
 *
 *	@(#)grf_rb.c	7.5 (Berkeley) 5/7/91
 */

#include "grf.h"
#if NGRF > 0

/*
 * Graphics routines for the Renaissance, HP98720 Graphics system.
 */
#include "sys/param.h"
#include "sys/errno.h"

#include "grfioctl.h"
#include "grfvar.h"
#include "grf_rbreg.h"

#include "../include/cpu.h"

/*
 * Initialize hardware.
 * Must point g_display at a grfinfo structure describing the hardware.
 * Returns 0 if hardware not present, non-zero ow.
 */
rb_init(gp, addr)
	struct grf_softc *gp;
	caddr_t addr;
{
	register struct rboxfb *rbp;
	struct grfinfo *gi = &gp->g_display;
	int fboff;
	extern caddr_t sctopa(), iomap();

	rbp = (struct rboxfb *) addr;
	if (ISIIOVA(addr))
		gi->gd_regaddr = (caddr_t) IIOP(addr);
	else
		gi->gd_regaddr = sctopa(vatosc(addr));
	gi->gd_regsize = 0x20000;
	gi->gd_fbwidth = (rbp->fbwmsb << 8) | rbp->fbwlsb;
	gi->gd_fbheight = (rbp->fbhmsb << 8) | rbp->fbhlsb;
	gi->gd_fbsize = gi->gd_fbwidth * gi->gd_fbheight;
	fboff = (rbp->fbomsb << 8) | rbp->fbolsb;
	gi->gd_fbaddr = (caddr_t) (*((u_char *)addr + fboff) << 16);
	if (gi->gd_regaddr >= (caddr_t)DIOIIBASE) {
		/*
		 * For DIO II space the fbaddr just computed is the offset
		 * from the select code base (regaddr) of the framebuffer.
		 * Hence it is also implicitly the size of the register set.
		 */
		gi->gd_regsize = (int) gi->gd_fbaddr;
		gi->gd_fbaddr += (int) gi->gd_regaddr;
		gp->g_regkva = addr;
		gp->g_fbkva = addr + gi->gd_regsize;
	} else {
		/*
		 * For DIO space we need to map the seperate framebuffer.
		 */
		gp->g_regkva = addr;
		gp->g_fbkva = iomap(gi->gd_fbaddr, gi->gd_fbsize);
	}
	gi->gd_dwidth = (rbp->dwmsb << 8) | rbp->dwlsb;
	gi->gd_dheight = (rbp->dwmsb << 8) | rbp->dwlsb;
	gi->gd_planes = 0;	/* ?? */
	gi->gd_colors = 256;
	return(1);
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
rb_mode(gp, cmd)
	register struct grf_softc *gp;
{
	register struct rboxfb *rbp;
	int error = 0;

	rbp = (struct rboxfb *) gp->g_regkva;
	switch (cmd) {
	/*
	 * The minimal register info here is from the Renaissance X driver.
	 */
	case GM_GRFON:
	case GM_GRFOFF:
		break;
	case GM_GRFOVON:
		rbp->write_enable = 0;
		rbp->opwen = 0xF;
		rbp->drive = 0x10;
		break;
	case GM_GRFOVOFF:
		rbp->opwen = 0;
		rbp->write_enable = 0xffffffff;
		rbp->drive = 0x01;
		break;
	default:
		error = EINVAL;
		break;
	}
	return(error);
}

#endif
