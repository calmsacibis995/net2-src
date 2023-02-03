/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	7.7 (Berkeley) 6/27/91
 */

/*
 * Exported definitions unique to hp300/68k cpu support.
 */

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	COPY_SIGCODE		/* copy sigcode above user stack in exec */

/*
 * function vs. inline configuration;
 * these are defined to get generic functions
 * rather than inline or machine-dependent implementations
 */
#define	NEED_MINMAX		/* need {,i,l,ul}{min,max} functions */
#undef	NEED_FFS		/* don't need ffs function */
#undef	NEED_BCMP		/* don't need bcmp function */
#undef	NEED_STRLEN		/* don't need strlen function */

#define	cpu_exec(p)	/* nothing */
#define	cpu_wait(p)	/* nothing */

/*
 * Arguments to hardclock, softclock and gatherstats
 * encapsulate the previous machine state in an opaque
 * clockframe; for hp300, use just what the hardware
 * leaves on the stack.
 */
typedef struct intrframe {
	int	pc;
	int	ps;
} clockframe;

#define	CLKF_USERMODE(framep)	(((framep)->ps & PSL_S) == 0)
#define	CLKF_BASEPRI(framep)	(((framep)->ps & PSL_IPL7) == 0)
#define	CLKF_PC(framep)		((framep)->pc)


/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched()	{ want_resched++; aston(); }

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On hp300, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(p, framep)	{ (p)->p_flag |= SOWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

#define aston() (astpending++)

int	astpending;		/* need to trap before returning to user mode */
int	want_resched;		/* resched() was called */


/*
 * simulated software interrupt register
 */
extern unsigned char ssir;

#define SIR_NET		0x1
#define SIR_CLOCK	0x2

#define siroff(x)	ssir &= ~(x)
#define setsoftnet()	ssir |= SIR_NET
#define setsoftclock()	ssir |= SIR_CLOCK



/*
 * The rest of this should probably be moved to ../hp300/hp300cpu.h,
 * although some of it could probably be put into generic 68k headers.
 */

/* values for machineid */
#define	HP_320		0	/* 16Mhz 68020+HP MMU+16K external cache */
#define	HP_330		1	/* 16Mhz 68020+68851 MMU */
#define	HP_350		2	/* 25Mhz 68020+HP MMU+32K external cache */
#define	HP_360		3	/* 25Mhz 68030 */
#define	HP_370		4	/* 33Mhz 68030+64K external cache */
#define	HP_340		5	/* 16Mhz 68030 */
#define	HP_375		6	/* 50Mhz 68030+32K external cache */

/* values for mmutype (assigned for quick testing) */
#define	MMU_68030	-1	/* 68030 on-chip subset of 68851 */
#define	MMU_HP		0	/* HP proprietary */
#define	MMU_68851	1	/* Motorola 68851 */

/* values for ectype */
#define	EC_PHYS		-1	/* external physical address cache */
#define	EC_NONE		0	/* no external cache */
#define	EC_VIRT		1	/* external virtual address cache */

/* values for cpuspeed (not really related to clock speed due to caches) */
#define	MHZ_8		1
#define	MHZ_16		2
#define	MHZ_25		3
#define	MHZ_33		4
#define	MHZ_50		6

#ifdef KERNEL
extern	int machineid, mmutype, ectype;
extern	char *intiobase, *intiolimit;

/* what is this supposed to do? i.e. how is it different than startrtclock? */
#define	enablertclock()

#endif

/* physical memory sections */
#define	ROMBASE		(0x00000000)
#define	INTIOBASE	(0x00400000)
#define	INTIOTOP	(0x00600000)
#define	EXTIOBASE	(0x00600000)
#define	EXTIOTOP	(0x20000000)
#define	MAXADDR		(0xFFFFF000)

/*
 * Internal IO space:
 *
 * Ranges from 0x400000 to 0x600000 (IIOMAPSIZE).
 *
 * Internal IO space is mapped in the kernel from ``intiobase'' to
 * ``intiolimit'' (defined in locore.s).  Since it is always mapped,
 * conversion between physical and kernel virtual addresses is easy.
 */
#define	ISIIOVA(va) \
	((char *)(va) >= intiobase && (char *)(va) < intiolimit)
#define	IIOV(pa)	((int)(pa)-INTIOBASE+(int)intiobase)
#define	IIOP(va)	((int)(va)-(int)intiobase+INTIOBASE)
#define	IIOPOFF(pa)	((int)(pa)-INTIOBASE)
#define	IIOMAPSIZE	btoc(INTIOTOP-INTIOBASE)	/* 2mb */

/*
 * External IO space:
 *
 * DIO ranges from select codes 0-63 at physical addresses given by:
 *	0x600000 + (sc - 32) * 0x10000
 * DIO cards are addressed in the range 0-31 [0x600000-0x800000) for
 * their control space and the remaining areas, [0x200000-0x400000) and
 * [0x800000-0x1000000), are for additional space required by a card;
 * e.g. a display framebuffer.
 *
 * DIO-II ranges from select codes 132-255 at physical addresses given by:
 *	0x1000000 + (sc - 132) * 0x400000
 * The address range of DIO-II space is thus [0x1000000-0x20000000).
 *
 * DIO/DIO-II space is too large to map in its entirety, instead devices
 * are mapped into kernel virtual address space allocated from a range
 * of EIOMAPSIZE pages (vmparam.h) starting at ``extiobase''.
 */
#define	DIOBASE		(0x600000)
#define	DIOTOP		(0x1000000)
#define	DIOCSIZE	(0x10000)
#define	DIOIIBASE	(0x01000000)
#define	DIOIITOP	(0x20000000)
#define	DIOIICSIZE	(0x00400000)

/*
 * HP MMU
 */
#define	MMUBASE		IIOPOFF(0x5F4000)
#define	MMUSSTP		0x0
#define	MMUUSTP		0x4
#define	MMUTBINVAL	0x8
#define	MMUSTAT		0xC
#define	MMUCMD		MMUSTAT

#define	MMU_UMEN	0x0001	/* enable user mapping */
#define	MMU_SMEN	0x0002	/* enable supervisor mapping */
#define	MMU_CEN		0x0004	/* enable data cache */
#define	MMU_BERR	0x0008	/* bus error */
#define	MMU_IEN		0x0020	/* enable instruction cache */
#define	MMU_FPE		0x0040	/* enable 68881 FP coprocessor */
#define	MMU_WPF		0x2000	/* write protect fault */
#define	MMU_PF		0x4000	/* page fault */
#define	MMU_PTF		0x8000	/* page table fault */

#define	MMU_FAULT	(MMU_PTF|MMU_PF|MMU_WPF|MMU_BERR)
#define	MMU_ENAB	(MMU_UMEN|MMU_SMEN|MMU_IEN|MMU_FPE)

/*
 * 68851 and 68030 MMU
 */
#define	PMMU_LVLMASK	0x0007
#define	PMMU_INV	0x0400
#define	PMMU_WP		0x0800
#define	PMMU_ALV	0x1000
#define	PMMU_SO		0x2000
#define	PMMU_LV		0x4000
#define	PMMU_BE		0x8000
#define	PMMU_FAULT	(PMMU_WP|PMMU_INV)

/* 680X0 function codes */
#define	FC_USERD	1	/* user data space */
#define	FC_USERP	2	/* user program space */
#define	FC_PURGE	3	/* HPMMU: clear TLB entries */
#define	FC_SUPERD	5	/* supervisor data space */
#define	FC_SUPERP	6	/* supervisor program space */
#define	FC_CPU		7	/* CPU space */

/* fields in the 68020 cache control register */
#define	IC_ENABLE	0x0001	/* enable instruction cache */
#define	IC_FREEZE	0x0002	/* freeze instruction cache */
#define	IC_CE		0x0004	/* clear instruction cache entry */
#define	IC_CLR		0x0008	/* clear entire instruction cache */

/* additional fields in the 68030 cache control register */
#define	IC_BE		0x0010	/* instruction burst enable */
#define	DC_ENABLE	0x0100	/* data cache enable */
#define	DC_FREEZE	0x0200	/* data cache freeze */
#define	DC_CE		0x0400	/* clear data cache entry */
#define	DC_CLR		0x0800	/* clear entire data cache */
#define	DC_BE		0x1000	/* data burst enable */
#define	DC_WA		0x2000	/* write allocate */

#define	CACHE_ON	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	CACHE_OFF	(DC_CLR|IC_CLR)
#define	CACHE_CLR	(CACHE_ON)
#define	IC_CLEAR	(DC_WA|DC_BE|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	DC_CLEAR	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_ENABLE)
