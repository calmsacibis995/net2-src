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
 * from: Utah $Hdr: ite_gb.c 1.18 91/01/21$
 *
 *	@(#)ite_gb.c	7.5 (Berkeley) 5/7/91
 */

#include "ite.h"
#if NITE > 0

#include "param.h"
#include "conf.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "itevar.h"
#include "itereg.h"
#include "grf_gbreg.h"

#include "machine/cpu.h"

/* XXX */
#include "grfioctl.h"
#include "grfvar.h"

#define REGBASE     	((struct gboxfb *)(ip->regbase))
#define WINDOWMOVER 	gatorbox_windowmove

gatorbox_init(ip)
	register struct ite_softc *ip;
{
	/* XXX */
	if (ip->regbase == 0) {
		struct grf_softc *gp = &grf_softc[ip - ite_softc];
		ip->regbase = gp->g_regkva;
		ip->fbbase = gp->g_fbkva;
	}

	REGBASE->write_protect = 0x0;
	REGBASE->interrupt = 0x4;
	REGBASE->rep_rule = RR_COPY;
	REGBASE->blink1 = 0xff;
	REGBASE->blink2 = 0xff;
	gb_microcode(REGADDR);
	REGBASE->sec_interrupt = 0x01;

	/*
	 * Set up the color map entries. We use three entries in the
	 * color map. The first, is for black, the second is for
	 * white, and the very last entry is for the inverted cursor.
	 */
	REGBASE->creg_select = 0x00;
	REGBASE->cmap_red    = 0x00;
	REGBASE->cmap_grn    = 0x00;
	REGBASE->cmap_blu    = 0x00;
	REGBASE->cmap_write  = 0x00;
	gbcm_waitbusy(REGADDR);
	
	REGBASE->creg_select = 0x01;
	REGBASE->cmap_red    = 0xFF;
	REGBASE->cmap_grn    = 0xFF;
	REGBASE->cmap_blu    = 0xFF;
	REGBASE->cmap_write  = 0x01;
	gbcm_waitbusy(REGADDR);

	REGBASE->creg_select = 0xFF;
	REGBASE->cmap_red    = 0xFF;
	REGBASE->cmap_grn    = 0xFF;
	REGBASE->cmap_blu    = 0xFF;
	REGBASE->cmap_write  = 0x01;
	gbcm_waitbusy(REGADDR);

	ite_devinfo(ip);
	ite_fontinit(ip);

	/*
	 * Clear the display. This used to be before the font unpacking
	 * but it crashes. Figure it out later.
	 */
	gatorbox_windowmove(ip, 0, 0, 0, 0, ip->dheight, ip->dwidth, RR_CLEAR);
	tile_mover_waitbusy(REGADDR);

	/*
	 * Stash the inverted cursor.
	 */
	gatorbox_windowmove(ip, charY(ip, ' '), charX(ip, ' '),
			    ip->cblanky, ip->cblankx, ip->ftheight,
			    ip->ftwidth, RR_COPYINVERTED);
}

gatorbox_deinit(ip)
	struct ite_softc *ip;
{
	gatorbox_windowmove(ip, 0, 0, 0, 0,
			    ip->dheight, ip->dwidth, RR_CLEAR);
	tile_mover_waitbusy(REGADDR);

   	ip->flags &= ~ITE_INITED;
}

gatorbox_putc(ip, c, dy, dx, mode)
	register struct ite_softc *ip;
        register int dy, dx;
	int c, mode;
{
        register int wrr = ((mode == ATTR_INV) ? RR_COPYINVERTED : RR_COPY);

	gatorbox_windowmove(ip, charY(ip, c), charX(ip, c),
			    dy * ip->ftheight, dx * ip->ftwidth,
			    ip->ftheight, ip->ftwidth, wrr);
}

gatorbox_cursor(ip, flag)
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

gatorbox_clear(ip, sy, sx, h, w)
	struct ite_softc *ip;
	register int sy, sx, h, w;
{
	gatorbox_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			    sy * ip->ftheight, sx * ip->ftwidth, 
			    h  * ip->ftheight, w  * ip->ftwidth,
			    RR_CLEAR);
}
#define	gatorbox_blockmove(ip, sy, sx, dy, dx, h, w) \
	gatorbox_windowmove((ip), \
			    (sy) * ip->ftheight, \
			    (sx) * ip->ftwidth, \
			    (dy) * ip->ftheight, \
			    (dx) * ip->ftwidth, \
			    (h)  * ip->ftheight, \
			    (w)  * ip->ftwidth, \
			    RR_COPY)

gatorbox_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
	register int height, dy, i;
	
	tile_mover_waitbusy(REGADDR);
	REGBASE->write_protect = 0x0;
	
	gatorbox_cursor(ip, ERASE_CURSOR);

	if (dir == SCROLL_UP) {
		dy = sy - count;
		height = ip->rows - sy;
		for (i = 0; i < height; i++)
			gatorbox_blockmove(ip, sy + i, sx, dy + i, 0,
					   1, ip->cols);
	}
	else if (dir == SCROLL_DOWN) {
		dy = sy + count;
		height = ip->rows - dy;
		for (i = (height - 1); i >= 0; i--)
			gatorbox_blockmove(ip, sy + i, sx, dy + i, 0,
					   1, ip->cols);
	}
	else if (dir == SCROLL_RIGHT) {
		gatorbox_blockmove(ip, sy, sx, sy, sx + count,
				   1, ip->cols - (sx + count));
	}
	else {
		gatorbox_blockmove(ip, sy, sx, sy, sx - count,
				   1, ip->cols - sx);
	}		
}

gatorbox_windowmove(ip, sy, sx, dy, dx, h, w, mask)
     register struct ite_softc *ip;
     int sy, sx, dy, dx, mask;
     register int h, w;
{
	register int src, dest;

	src  = (sy * 1024) + sx;	/* upper left corner in pixels */
	dest = (dy * 1024) + dx;

	tile_mover_waitbusy(REGADDR);
	REGBASE->width = -(w / 4);
	REGBASE->height = -(h / 4);
	if (src < dest)
		REGBASE->rep_rule = MOVE_DOWN_RIGHT|mask;
	else {
		REGBASE->rep_rule = MOVE_UP_LEFT|mask;
		/*
		 * Adjust to top of lower right tile of the block.
		 */
		src = src + ((h - 4) * 1024) + (w - 4);
		dest= dest + ((h - 4) * 1024) + (w - 4);
	}
	FBBASE[dest] = FBBASE[src];
}
#endif
