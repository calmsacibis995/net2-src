/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)mkboot.c	7.2 (Berkeley) 12/16/90
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)mkboot.c	7.2 (Berkeley) 12/16/90";
#endif /* not lint */

#include "../include/param.h"
#include "volhdr.h"
#include <sys/exec.h>
#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>

int lpflag;
int loadpoint;
struct load ld;
struct lifvol lifv;
struct lifdir lifd[8];
struct exec ex;
char buf[10240];

main(argc, argv)
	char **argv;
{
	int ac;
	char **av;
	int from1, from2, to;
	register int n;
	char *n1, *n2, *lifname();

	ac = --argc;
	av = ++argv;
	if (ac == 0)
		usage();
	if (!strcmp(av[0], "-l")) {
		av++;
		ac--;
		if (ac == 0)
			usage();
		sscanf(av[0], "0x%x", &loadpoint);
		lpflag++;
		av++;
		ac--;
	}
	if (ac == 0)
		usage();
	from1 = open(av[0], O_RDONLY, 0);
	if (from1 < 0) {
		perror("open");
		exit(1);
	}
	n1 = av[0];
	av++;
	ac--;
	if (ac == 0)
		usage();
	if (ac == 2) {
		from2 = open(av[0], O_RDONLY, 0);
		if (from2 < 0) {
			perror("open");
			exit(1);
		}
		n2 = av[0];
		av++;
		ac--;
	} else
		from2 = -1;
	to = open(av[0], O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (to < 0) {
		perror("open");
		exit(1);
	}
	/* clear possibly unused directory entries */
	strncpy(lifd[1].dir_name, "          ", 10);
	lifd[1].dir_type = -1;
	lifd[1].dir_addr = 0;
	lifd[1].dir_length = 0;
	lifd[1].dir_flag = 0xFF;
	lifd[1].dir_exec = 0;
	lifd[7] = lifd[6] = lifd[5] = lifd[4] = lifd[3] = lifd[2] = lifd[1];
	/* record volume info */
	lifv.vol_id = VOL_ID;
	strncpy(lifv.vol_label, "BOOT43", 6);
	lifv.vol_addr = 2;
	lifv.vol_oct = VOL_OCT;
	lifv.vol_dirsize = 1;
	lifv.vol_version = 1;
	/* output bootfile one */
	lseek(to, 3 * SECTSIZE, 0);
	putfile(from1, to);
	n = (ld.count + sizeof(ld) + (SECTSIZE - 1)) / SECTSIZE;
	strcpy(lifd[0].dir_name, lifname(n1));
	lifd[0].dir_type = DIR_TYPE;
	lifd[0].dir_addr = 3;
	lifd[0].dir_length = n;
	lifd[0].dir_flag = DIR_FLAG;
	lifd[0].dir_exec = lpflag? loadpoint + ex.a_entry : ex.a_entry;
	lifv.vol_length = lifd[0].dir_addr + lifd[0].dir_length;
	/* if there is an optional second boot program, output it */
	if (from2 >= 0) {
		lseek(to, (3 + n) * SECTSIZE, 0);
		putfile(from2, to);
		n = (ld.count + sizeof(ld) + (SECTSIZE - 1)) / SECTSIZE;
		strcpy(lifd[1].dir_name, lifname(n2));
		lifd[1].dir_type = DIR_TYPE;
		lifd[1].dir_addr = 3 + lifd[0].dir_length;
		lifd[1].dir_length = n;
		lifd[1].dir_flag = DIR_FLAG;
		lifd[1].dir_exec = lpflag? loadpoint + ex.a_entry : ex.a_entry;
		lifv.vol_length = lifd[1].dir_addr + lifd[1].dir_length;
	}
	/* output volume/directory header info */
	lseek(to, 0 * SECTSIZE, 0);
	write(to, &lifv, sizeof(lifv));
	lseek(to, 2 * SECTSIZE, 0);
	write(to, lifd, sizeof(lifd));
	exit(0);
}

putfile(from, to)
{
	register int n, tcnt, dcnt;

	n = read(from, &ex, sizeof(ex));
	if (n != sizeof(ex)) {
		fprintf(stderr, "error reading file header\n");
		exit(1);
	}
	if (ex.a_magic == OMAGIC) {
		tcnt = ex.a_text;
		dcnt = ex.a_data;
	}
	else if (ex.a_magic == NMAGIC) {
		tcnt = (ex.a_text + PGOFSET) & ~PGOFSET;
		dcnt = ex.a_data;
	}
	else {
		fprintf(stderr, "bad magic number\n");
		exit(1);
	}
	ld.address = lpflag ? loadpoint : ex.a_entry;
	ld.count = tcnt + dcnt;
	write(to, &ld, sizeof(ld));
	while (tcnt) {
		n = sizeof(buf);
		if (n > tcnt)
			n = tcnt;
		n = read(from, buf, n);
		if (n < 0) {
			perror("read");
			exit(1);
		}
		if (n == 0) {
			fprintf(stderr, "short read\n");
			exit(1);
		}
		if (write(to, buf, n) < 0) {
			perror("write");
			exit(1);
		}
		tcnt -= n;
	}
	while (dcnt) {
		n = sizeof(buf);
		if (n > dcnt)
			n = dcnt;
		n = read(from, buf, n);
		if (n < 0) {
			perror("read");
			exit(1);
		}
		if (n == 0) {
			fprintf(stderr, "short read\n");
			exit(1);
		}
		if (write(to, buf, n) < 0) {
			perror("write");
			exit(1);
		}
		dcnt -= n;
	}
}

usage()
{
	fprintf(stderr,
		"usage:  mkboot [-l loadpoint] prog1 [ prog2 ] outfile\n");
	exit(1);
}

char *
lifname(str)
 char *str;
{
	static char lname[10] = "SYS_XXXXX";
	register int i;

	for (i = 4; i < 9; i++) {
		if (islower(*str))
			lname[i] = toupper(*str);
		else if (isalnum(*str) || *str == '_')
			lname[i] = *str;
		else
			break;
		str++;
	}
	for ( ; i < 10; i++)
		lname[i] = '\0';
	return(lname);
}
