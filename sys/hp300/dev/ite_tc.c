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
 * from: Utah $Hdr: ite_tc.c 1.25 91/03/25$
 *
 *	@(#)ite_tc.c	7.4 (Berkeley) 5/7/91
 */

#include "ite.h"
#if NITE > 0

#include "param.h"
#include "conf.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "grf_tcreg.h"
#include "itereg.h"
#include "itevar.h"

#include "machine/cpu.h"

#define REGBASE	    	((struct tcboxfb *)(ip->regbase))
#define WINDOWMOVER 	topcat_windowmove

/* XXX */
#include "grfioctl.h"
#include "grfvar.h"

topcat_init(ip)
	register struct ite_softc *ip;
{
	/* XXX */
	if (ip->regbase == NULL) {
		struct grf_softc *gp = &grf_softc[ip - ite_softc];
		ip->regbase = gp->g_regkva;
		ip->fbbase = gp->g_fbkva;
	}

	/*
	 * Catseye looks a lot like a topcat, but not completely.
	 * So, we set some bits to make it work.
	 */
	if (REGBASE->fbid != GID_TOPCAT) {
		while ((REGBASE->catseye_status & 1))
			;
		REGBASE->catseye_status = 0x0;
		REGBASE->vb_select      = 0x0;
		REGBASE->tcntrl         = 0x0;
		REGBASE->acntrl         = 0x0;
		REGBASE->pncntrl        = 0x0;
		REGBASE->rug_cmdstat    = 0x90;
	}

	/*
	 * Determine the number of planes by writing to the first frame
	 * buffer display location, then reading it back. 
	 */
	REGBASE->wen = ~0;
	REGBASE->fben = ~0;
	REGBASE->prr = RR_COPY;
	*FBBASE = 0xFF;
	ip->planemask = *FBBASE;

	/*
	 * Enable reading/writing of all the planes.
	 */
	REGBASE->fben = ip->planemask;
	REGBASE->wen  = ip->planemask;
	REGBASE->ren  = ip->planemask;
	REGBASE->prr  = RR_COPY;

	ite_devinfo(ip);

	/*
	 * Clear the framebuffer on all planes.
	 */
	topcat_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);
	tc_waitbusy(REGADDR, ip->planemask);

	ite_fontinit(ip);

	/*
	 * Initialize color map for color displays
	 */
	if (ip->planemask != 1) {
	  	tc_waitbusy(REGADDR, ip->planemask);
		REGBASE->nblank = 0x01;

		tccm_waitbusy(REGADDR);
		REGBASE->rdata  = 0x0;
		REGBASE->gdata  = 0x0;
		REGBASE->bdata  = 0x0;
		REGBASE->cindex = 0xFF;
		REGBASE->strobe = 0xFF;

		DELAY(100);
		tccm_waitbusy(REGADDR);
		REGBASE->rdata  = 0x0;
		REGBASE->gdata  = 0x0;
		REGBASE->bdata  = 0x0;
		REGBASE->cindex = 0x0;

		DELAY(100);
		tccm_waitbusy(REGADDR);
		REGBASE->rdata  = 0xFF;
		REGBASE->gdata  = 0xFF;
		REGBASE->bdata  = 0xFF;
		REGBASE->cindex = 0xFE;
		REGBASE->strobe = 0xFF;

		DELAY(100);
		tccm_waitbusy(REGADDR);
		REGBASE->rdata  = 0x0;
		REGBASE->gdata  = 0x0;
		REGBASE->bdata  = 0x0;
		REGBASE->cindex = 0x0;
	}

	/*
	 * Stash the inverted cursor.
	 */
	topcat_windowmove(ip, charY(ip, ' '), charX(ip, ' '),
			  ip->cblanky, ip->cblankx, ip->ftheight,
			  ip->ftwidth, RR_COPYINVERTED);
}

topcat_deinit(ip)
	register struct ite_softc *ip;
{
	topcat_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);
	tc_waitbusy(REGADDR, ip->planemask);

	REGBASE->nblank = ~0;
   	ip->flags &= ~ITE_INITED;
}

topcat_putc(ip, c, dy, dx, mode)
	register struct ite_softc *ip;
	int c, dy, dx, mode;
{
        int wmrr = ((mode == ATTR_INV) ? RR_COPYINVERTED : RR_COPY);
	
	topcat_windowmove(ip, charY(ip, c), charX(ip, c),
			  dy * ip->ftheight, dx * ip->ftwidth,
			  ip->ftheight, ip->ftwidth, wmrr);
}

topcat_cursor(ip, flag)
	register struct ite_softc *ip;
	register int flag;
{
	if (flag == DRAW_CURSOR)
		draw_cursor(ip)
	else if (flag == MOVE_CURSOR) {
		erase_cursor(ip)
		draw_cursor(ip)
	}
	else
		erase_cursor(ip)
}

topcat_clear(ip, sy, sx, h, w)
	register struct ite_softc *ip;
	register int sy, sx, h, w;
{
	topcat_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			  sy * ip->ftheight, sx * ip->ftwidth, 
			  h  * ip->ftheight, w  * ip->ftwidth,
			  RR_CLEAR);
}

topcat_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy, count;
        int dir, sx;
{
	register int dy;
	register int dx = sx;
	register int height = 1;
	register int width = ip->cols;

	topcat_cursor(ip, ERASE_CURSOR);

	if (dir == SCROLL_UP) {
		dy = sy - count;
		height = ip->rows - sy;
	}
	else if (dir == SCROLL_DOWN) {
		dy = sy + count;
		height = ip->rows - dy - 1;
	}
	else if (dir == SCROLL_RIGHT) {
		dy = sy;
		dx = sx + count;
		width = ip->cols - dx;
	}
	else {
		dy = sy;
		dx = sx - count;
		width = ip->cols - sx;
	}		

	topcat_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			  dy * ip->ftheight, dx * ip->ftwidth,
			  height * ip->ftheight,
			  width  * ip->ftwidth, RR_COPY);
}

topcat_windowmove(ip, sy, sx, dy, dx, h, w, func)
	struct ite_softc *ip;
	int sy, sx, dy, dx, h, w, func;
{
  	register struct tcboxfb *rp = REGBASE;
	
	if (h == 0 || w == 0)
		return;
	tc_waitbusy(REGADDR, ip->planemask);
	rp->wmrr     = func;
	rp->source_y = sy;
	rp->source_x = sx;
	rp->dest_y   = dy;
	rp->dest_x   = dx;
	rp->wheight  = h;
	rp->wwidth   = w;
	rp->wmove    = ip->planemask;
}
#endif
