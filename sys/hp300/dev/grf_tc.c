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
 * from: Utah $Hdr: grf_tc.c 1.18 91/04/02$
 *
 *	@(#)grf_tc.c	7.4 (Berkeley) 5/7/91
 */

#include "grf.h"
#if NGRF > 0

/*
 * Graphics routines for TOPCAT frame buffer
 */
#include "sys/param.h"
#include "sys/errno.h"

#include "grfioctl.h"
#include "grfvar.h"
#include "grf_tcreg.h"

#include "../include/cpu.h"

/*
 * Initialize hardware.
 * Must fill in the grfinfo structure in g_softc.
 * Returns 0 if hardware not present, non-zero ow.
 */
tc_init(gp, addr)
	struct grf_softc *gp;
	caddr_t addr;
{
	register struct tcboxfb *tp = (struct tcboxfb *) addr;
	struct grfinfo *gi = &gp->g_display;
	volatile u_char *fbp;
	u_char save;
	int fboff;
	extern caddr_t sctopa(), iomap();

	if (ISIIOVA(addr))
		gi->gd_regaddr = (caddr_t) IIOP(addr);
	else
		gi->gd_regaddr = sctopa(vatosc(addr));
	gi->gd_regsize = 0x10000;
	gi->gd_fbwidth = (tp->fbwmsb << 8) | tp->fbwlsb;
	gi->gd_fbheight = (tp->fbhmsb << 8) | tp->fbhlsb;
	gi->gd_fbsize = gi->gd_fbwidth * gi->gd_fbheight;
	fboff = (tp->fbomsb << 8) | tp->fbolsb;
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
	gi->gd_dwidth = (tp->dwmsb << 8) | tp->dwlsb;
	gi->gd_dheight = (tp->dhmsb << 8) | tp->dhlsb;
	gi->gd_planes = tp->num_planes;
	gi->gd_colors = 1 << gi->gd_planes;
	if (gi->gd_colors == 1) {
		fbp = (u_char *) gp->g_fbkva;
		tp->wen = ~0;
		tp->prr = 0x3;
		tp->fben = ~0;
		save = *fbp;
		*fbp = 0xFF;
		gi->gd_colors = *fbp + 1;
		*fbp = save;
	}
	return(1);
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 * Function may not be needed anymore.
 */
/*ARGSUSED*/
tc_mode(gp, cmd)
	struct grf_softc *gp;
{
	int error = 0;

	switch (cmd) {
	case GM_GRFON:
	case GM_GRFOFF:
		break;
	default:
		error = EINVAL;
		break;
	}
	return(error);
}
#endif
