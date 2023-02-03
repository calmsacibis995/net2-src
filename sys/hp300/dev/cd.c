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
 * from: Utah $Hdr: cd.c 1.6 90/11/28$
 *
 *	@(#)cd.c	7.4 (Berkeley) 5/7/91
 */

/*
 * "Concatenated" disk driver.
 */
#include "cd.h"
#if NCD > 0

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/errno.h"
#include "sys/dkstat.h"
#include "sys/buf.h"
#include "sys/malloc.h"
#include "sys/conf.h"

#include "cdvar.h"

#ifdef DEBUG
int cddebug = 0x00;
#define CDB_FOLLOW	0x01
#define CDB_INIT	0x02
#define CDB_IO		0x04
#endif

struct	buf cdbuf[NCD];
struct	buf *cdbuffer();
char	*cddevtostr();
int	cdiodone();

#define	cdunit(x)	((minor(x) >> 3) & 0x7)	/* for consistency */

#define	getcbuf()	\
	((struct buf *)malloc(sizeof(struct buf), M_DEVBUF, M_WAITOK))
#define putcbuf(bp)	\
	free((caddr_t)(bp), M_DEVBUF)

struct cd_softc {
	int		 sc_flags;		/* flags */
	size_t		 sc_size;		/* size of cd */
	int		 sc_ileave;		/* interleave */
	int		 sc_ncdisks;		/* number of components */
	struct cdcinfo	 sc_cinfo[NCDISKS];	/* component info */
	struct cdiinfo	 *sc_itable;		/* interleave table */
	int		 sc_usecnt;		/* number of requests active */
	struct buf	 *sc_bp;		/* "current" request */
	int		 sc_dk;			/* disk index */
} cd_softc[NCD];

/* sc_flags */
#define	CDF_ALIVE	0x01
#define CDF_INITED	0x02

cdinit(cd)
	struct cddevice *cd;
{
	register struct cd_softc *cs = &cd_softc[cd->cd_unit];
	register struct cdcinfo *ci;
	register size_t size;
	register int ix;
	size_t minsize;
	dev_t dev;

#ifdef DEBUG
	if (cddebug & (CDB_FOLLOW|CDB_INIT))
		printf("cdinit: unit %d\n", cd->cd_unit);
#endif
	cs->sc_dk = cd->cd_dk;
	cs->sc_size = 0;
	cs->sc_ileave = cd->cd_interleave;
	cs->sc_ncdisks = 0;
	/*
	 * Verify that each component piece exists and record
	 * relevant information about it.
	 */
	minsize = 0;
	for (ix = 0; ix < NCDISKS; ix++) {
		if ((dev = cd->cd_dev[ix]) == NODEV)
			break;
		ci = &cs->sc_cinfo[ix];
		ci->ci_dev = dev;
		/*
		 * Calculate size (truncated to interleave boundary
		 * if necessary.
		 */
		if (bdevsw[major(dev)].d_psize) {
			size = (size_t) (*bdevsw[major(dev)].d_psize)(dev);
			if ((int)size < 0)
				size = 0;
		} else
			size = 0;
		if (cs->sc_ileave > 1)
			size -= size % cs->sc_ileave;
		if (size == 0) {
			printf("cd%d: not configured (component %s missing)\n",
			       cd->cd_unit, cddevtostr(ci->ci_dev));
			return(0);
		}
		if (minsize == 0 || size < minsize)
			minsize = size;
		ci->ci_size = size;
		cs->sc_size += size;
		cs->sc_ncdisks++;
	}
	/*
	 * If uniform interleave is desired set all sizes to that of
	 * the smallest component.
	 */
	if (cd->cd_flags & CDF_UNIFORM) {
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_ncdisks]; ci++)
			ci->ci_size = minsize;
		cs->sc_size = cs->sc_ncdisks * minsize;
	}
	/*
	 * Construct the interleave table
	 */
	if (!cdinterleave(cs))
		return(0);
	if (cd->cd_dk >= 0)
		dk_wpms[cd->cd_dk] = 32 * (60 * DEV_BSIZE / 2);	/* XXX */
	printf("cd%d: %d components ", cd->cd_unit, cs->sc_ncdisks);
	for (ix = 0; ix < cs->sc_ncdisks; ix++)
		printf("%c%s%c",
		       ix == 0 ? '(' : ' ',
		       cddevtostr(cs->sc_cinfo[ix].ci_dev),
		       ix == cs->sc_ncdisks - 1 ? ')' : ',');
	printf(", %d blocks ", cs->sc_size);
	if (cs->sc_ileave)
		printf("interleaved at %d blocks\n", cs->sc_ileave);
	else
		printf("concatenated\n");
	cs->sc_flags = CDF_ALIVE | CDF_INITED;
	return(1);
}

/*
 * XXX not really cd specific.
 */
char *
cddevtostr(dev)
	dev_t dev;
{
	static char dbuf[5];

	dbuf[1] = 'd';
	switch (major(dev)) {
	case 2:
		dbuf[0] = 'r';
		break;
	case 4:
		dbuf[0] = 's';
		break;
	case 5:
		dbuf[0] = 'c';
		break;
	default:
		dbuf[0] = dbuf[1] = '?';
		break;
	}
	dbuf[2] = (minor(dev) >> 3) + '0';
	dbuf[3] = (minor(dev) & 7) + 'a';
	dbuf[4] = '\0';
	return (dbuf);
}

cdinterleave(cs)
	register struct cd_softc *cs;
{
	register struct cdcinfo *ci, *smallci;
	register struct cdiinfo *ii;
	register daddr_t bn, lbn;
	register int ix;
	u_long size;

#ifdef DEBUG
	if (cddebug & CDB_INIT)
		printf("cdinterleave(%x): ileave %d\n", cs, cs->sc_ileave);
#endif
	/*
	 * Allocate an interleave table.
	 * Chances are this is too big, but we don't care.
	 */
	size = (cs->sc_ncdisks + 1) * sizeof(struct cdiinfo);
	cs->sc_itable = (struct cdiinfo *)malloc(size, M_DEVBUF, M_WAITOK);
	bzero((caddr_t)cs->sc_itable, size);
	/*
	 * Trivial case: no interleave (actually interleave of disk size).
	 * Each table entry represent a single component in its entirety.
	 */
	if (cs->sc_ileave == 0) {
		bn = 0;
		ii = cs->sc_itable;
		for (ix = 0; ix < cs->sc_ncdisks; ix++) {
			ii->ii_ndisk = 1;
			ii->ii_startblk = bn;
			ii->ii_startoff = 0;
			ii->ii_index[0] = ix;
			bn += cs->sc_cinfo[ix].ci_size;
			ii++;
		}
		ii->ii_ndisk = 0;
#ifdef DEBUG
		if (cddebug & CDB_INIT)
			printiinfo(cs->sc_itable);
#endif
		return(1);
	}
	/*
	 * The following isn't fast or pretty; it doesn't have to be.
	 */
	size = 0;
	bn = lbn = 0;
	for (ii = cs->sc_itable; ; ii++) {
		/*
		 * Locate the smallest of the remaining components
		 */
		smallci = NULL;
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_ncdisks]; ci++)
			if (ci->ci_size > size &&
			    (smallci == NULL ||
			     ci->ci_size < smallci->ci_size))
				smallci = ci;
		/*
		 * Nobody left, all done
		 */
		if (smallci == NULL) {
			ii->ii_ndisk = 0;
			break;
		}
		/*
		 * Record starting logical block and component offset
		 */
		ii->ii_startblk = bn / cs->sc_ileave;
		ii->ii_startoff = lbn;
		/*
		 * Determine how many disks take part in this interleave
		 * and record their indices.
		 */
		ix = 0;
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_ncdisks]; ci++)
			if (ci->ci_size >= smallci->ci_size)
				ii->ii_index[ix++] = ci - cs->sc_cinfo;
		ii->ii_ndisk = ix;
		bn += ix * (smallci->ci_size - size);
		lbn = smallci->ci_size / cs->sc_ileave;
		size = smallci->ci_size;
	}
#ifdef DEBUG
	if (cddebug & CDB_INIT)
		printiinfo(cs->sc_itable);
#endif
	return(1);
}

#ifdef DEBUG
printiinfo(ii)
	struct cdiinfo *ii;
{
	register int ix, i;

	for (ix = 0; ii->ii_ndisk; ix++, ii++) {
		printf(" itab[%d]: #dk %d sblk %d soff %d",
		       ix, ii->ii_ndisk, ii->ii_startblk, ii->ii_startoff);
		for (i = 0; i < ii->ii_ndisk; i++)
			printf(" %d", ii->ii_index[i]);
		printf("\n");
	}
}
#endif

cdopen(dev, flags)
	dev_t dev;
{
	int unit = cdunit(dev);
	register struct cd_softc *cs = &cd_softc[unit];

#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdopen(%x, %x)\n", dev, flags);
#endif
	if (unit >= NCD || (cs->sc_flags & CDF_ALIVE) == 0)
		return(ENXIO);
	return(0);
}

cdstrategy(bp)
	register struct buf *bp;
{
	register int unit = cdunit(bp->b_dev);
	register struct cd_softc *cs = &cd_softc[unit];
	register daddr_t bn;
	register int sz, s;

#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdstrategy(%x): unit %d\n", bp, unit);
#endif
	if ((cs->sc_flags & CDF_INITED) == 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}
	bn = bp->b_blkno;
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bn < 0 || bn + sz > cs->sc_size) {
		sz = cs->sc_size - bn;
		if (sz == 0) {
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			goto done;
		}
		bp->b_bcount = dbtob(sz);
	}
	bp->b_resid = bp->b_bcount;
	/*
	 * "Start" the unit.
	 * XXX: the use of sc_bp is just to retain the "traditional"
	 * interface to the start routine.
	 */
	s = splbio();
	cs->sc_bp = bp;
	cdstart(unit);
	splx(s);
	return;
done:
	biodone(bp);
}

cdstart(unit)
	int unit;
{
	register struct cd_softc *cs = &cd_softc[unit];
	register struct buf *bp = cs->sc_bp;
	register long bcount, rcount;
	struct buf *cbp;
	caddr_t addr;
	daddr_t bn;

#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdstart(%d)\n", unit);
#endif
	/*
	 * Instumentation (not real meaningful)
	 */
	cs->sc_usecnt++;
	if (cs->sc_dk >= 0) {
		dk_busy |= 1 << cs->sc_dk;
		dk_xfer[cs->sc_dk]++;
		dk_wds[cs->sc_dk] += bp->b_bcount >> 6;
	}
	/*
	 * Allocate component buffers and fire off the requests
	 */
	bn = bp->b_blkno;
	addr = bp->b_un.b_addr;
	for (bcount = bp->b_bcount; bcount > 0; bcount -= rcount) {
		cbp = cdbuffer(cs, bp, bn, addr, bcount);
		rcount = cbp->b_bcount;
		(*bdevsw[major(cbp->b_dev)].d_strategy)(cbp);
		bn += btodb(rcount);
		addr += rcount;
	}
}

/*
 * Build a component buffer header.
 */
struct buf *
cdbuffer(cs, bp, bn, addr, bcount)
	register struct cd_softc *cs;
	struct buf *bp;
	daddr_t bn;
	caddr_t addr;
	long bcount;
{
	register struct cdcinfo *ci;
	register struct buf *cbp;
	register daddr_t cbn, cboff;

#ifdef DEBUG
	if (cddebug & CDB_IO)
		printf("cdbuffer(%x, %x, %d, %x, %d)\n",
		       cs, bp, bn, addr, bcount);
#endif
	/*
	 * Determine which component bn falls in.
	 */
	cbn = bn;
	cboff = 0;
	/*
	 * Serially concatenated
	 */
	if (cs->sc_ileave == 0) {
		register daddr_t sblk;

		sblk = 0;
		for (ci = cs->sc_cinfo; cbn >= sblk + ci->ci_size; ci++)
			sblk += ci->ci_size;
		cbn -= sblk;
	}
	/*
	 * Interleaved
	 */
	else {
		register struct cdiinfo *ii;
		int cdisk, off;

		cboff = cbn % cs->sc_ileave;
		cbn /= cs->sc_ileave;
		for (ii = cs->sc_itable; ii->ii_ndisk; ii++)
			if (ii->ii_startblk > cbn)
				break;
		ii--;
		off = cbn - ii->ii_startblk;
		if (ii->ii_ndisk == 1) {
			cdisk = ii->ii_index[0];
			cbn = ii->ii_startoff + off;
		} else {
			cdisk = ii->ii_index[off % ii->ii_ndisk];
			cbn = ii->ii_startoff + off / ii->ii_ndisk;
		}
		cbn *= cs->sc_ileave;
		ci = &cs->sc_cinfo[cdisk];
	}
	/*
	 * Fill in the component buf structure.
	 */
	cbp = getcbuf();
	cbp->b_flags = bp->b_flags | B_CALL;
	cbp->b_iodone = cdiodone;
	cbp->b_proc = bp->b_proc;
	cbp->b_dev = ci->ci_dev;
	cbp->b_blkno = cbn + cboff;
	cbp->b_un.b_addr = addr;
	cbp->b_vp = 0;
	if (cs->sc_ileave == 0)
		cbp->b_bcount = dbtob(ci->ci_size - cbn);
	else
		cbp->b_bcount = dbtob(cs->sc_ileave - cboff);
	if (cbp->b_bcount > bcount)
		cbp->b_bcount = bcount;
	/*
	 * XXX: context for cdiodone
	 */
	cbp->b_saveaddr = (caddr_t)bp;
	cbp->b_pfcent = ((cs - cd_softc) << 16) | (ci - cs->sc_cinfo);
#ifdef DEBUG
	if (cddebug & CDB_IO)
		printf(" dev %x(u%d): cbp %x bn %d addr %x bcnt %d\n",
		       ci->ci_dev, ci-cs->sc_cinfo, cbp, cbp->b_blkno,
		       cbp->b_un.b_addr, cbp->b_bcount);
#endif
	return(cbp);
}

cdintr(unit)
	int unit;
{
	register struct cd_softc *cs = &cd_softc[unit];
	register struct buf *bp = cs->sc_bp;

#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdintr(%d): bp %x\n", unit, bp);
#endif
	/*
	 * Request is done for better or worse, wakeup the top half.
	 */
	if (--cs->sc_usecnt == 0 && cs->sc_dk >= 0)
		dk_busy &= ~(1 << cs->sc_dk);
	if (bp->b_flags & B_ERROR)
		bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * Called by biodone at interrupt time.
 * Mark the component as done and if all components are done,
 * take a cd interrupt.
 */
cdiodone(cbp)
	register struct buf *cbp;
{
	register struct buf *bp = (struct buf *)cbp->b_saveaddr;/* XXX */
	register int unit = (cbp->b_pfcent >> 16) & 0xFFFF;	/* XXX */
	int count, s;

	s = splbio();
#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdiodone(%x)\n", cbp);
	if (cddebug & CDB_IO) {
		printf("cdiodone: bp %x bcount %d resid %d\n",
		       bp, bp->b_bcount, bp->b_resid);
		printf(" dev %x(u%d), cbp %x bn %d addr %x bcnt %d\n",
		       cbp->b_dev, cbp->b_pfcent & 0xFFFF, cbp,
		       cbp->b_blkno, cbp->b_un.b_addr, cbp->b_bcount);
	}
#endif

	if (cbp->b_flags & B_ERROR) {
		bp->b_flags |= B_ERROR;
		bp->b_error = biowait(cbp);
#ifdef DEBUG
		printf("cd%d: error %d on component %d\n",
		       unit, bp->b_error, cbp->b_pfcent & 0xFFFF);
#endif
	}
	count = cbp->b_bcount;
	putcbuf(cbp);

	/*
	 * If all done, "interrupt".
	 * Again, sc_bp is only used to preserve the traditional interface.
	 */
	bp->b_resid -= count;
	if (bp->b_resid < 0)
		panic("cdiodone: count");
	if (bp->b_resid == 0) {
		cd_softc[unit].sc_bp = bp;
		cdintr(unit);
	}
	splx(s);
}

cdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = cdunit(dev);

#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdread(%x, %x)\n", dev, uio);
#endif
	return(physio(cdstrategy, &cdbuf[unit], dev, B_READ, minphys, uio));
}

cdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = cdunit(dev);

#ifdef DEBUG
	if (cddebug & CDB_FOLLOW)
		printf("cdwrite(%x, %x)\n", dev, uio);
#endif
	return(physio(cdstrategy, &cdbuf[unit], dev, B_WRITE, minphys, uio));
}

cdioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	return(EINVAL);
}

cdsize(dev)
	dev_t dev;
{
	int unit = cdunit(dev);
	register struct cd_softc *cs = &cd_softc[unit];

	if (unit >= NCD || (cs->sc_flags & CDF_INITED) == 0)
		return(-1);
	return(cs->sc_size);
}

cddump(dev)
{
	return(ENXIO);
}
#endif
