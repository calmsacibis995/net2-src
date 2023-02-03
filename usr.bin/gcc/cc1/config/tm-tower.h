/* Definitions of target machine for GNU compiler.
   Copyright (C) 1990 Free Software Foundation, Inc.

   Written by Robert Andersson, International Systems, Oslo, Norway.
   Please send bug reports, questions and improvements to ra@intsys.no.

   For NCR Tower 32/4x0 and 32/6x0 running System V Release 3.
   I don't have access to 200/700/800/850 machines, so I don't know if it
   works on those as well.  It shouldn't be far from it however.
   The hardware floating point support is completely untested, as I do
   not have access to a machine with a 6888x FPU in it.
   It does not work on the System V Release 2 based OS releases.  Making it
   work will not be easy, due to the silly way in which stack expansion is
   implemented in the OS.

   This file is included in both tm-tower-as.h and tm-tower-gc.h, and contains
   definitions common to both of them.  Do *NOT* include this file directly.


This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include "tm-m68k.h"


/* See tm-m68k.h.  5 means 68020 with no 68881.  */

#define TARGET_DEFAULT 5

/* Names to predefine in the preprocessor for this target machine.  */

#ifdef tower32_200
#define CPP_PREDEFINES "-Dunix -Dtower32 -Dtower32_200"
#endif
#ifdef tower32_600
#define CPP_PREDEFINES "-Dunix -Dtower32 -Dtower32_600"
#endif
#ifdef tower32_700
#define CPP_PREDEFINES "-Dunix -Dtower32 -Dtower32_700"
#endif
#ifdef tower32_800
#define CPP_PREDEFINES "-Dunix -Dtower32 -Dtower32_800"
#endif
#ifdef tower32_850
#define CPP_PREDEFINES "-Dunix -Dtower32 -Dtower32_850"
#endif

/* The startfiles and libraries depend on the -p and -m68881 options.
   The Tower does not support the -pg option.  */

#define LINK_SPEC                                              \
"%{p:%{m68881:-L/usr/lib/fp/libp} -L/usr/lib/libp}             \
 %{m68881:-L/usr/lib/fp}"

#define LIB_SPEC "-lc %{m68881:/usr/lib/fp/crtn.o}%{!m68881:/lib/crtn.o}"

/* Use mem* functions, recognize #ident lines.  */

#define TARGET_MEM_FUNCTIONS
#define IDENT_DIRECTIVE

/* This is only used in g++, don't prepend underscore.  */

#define NO_UNDERSCORES

/* Every structure and union's size must be a multiple of two bytes.  */

#define STRUCTURE_SIZE_BOUNDARY 16

/* All register names should have a leading % character.  */

#undef REGISTER_NAMES
#define REGISTER_NAMES \
{"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",                      \
 "%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp",                      \
 "%fp0", "%fp1", "%fp2", "%fp3", "%fp4", "%fp5", "%fp6", "%fp7",              \
 "%fpa0", "%fpa1", "%fpa2", "%fpa3", "%fpa4", "%fpa5", "%fpa6", "%fpa7",      \
 "%fpa8", "%fpa9", "%fpa10", "%fpa11", "%fpa12", "%fpa13", "%fpa14", "%fpa15",\
 "%fpa16","%fpa17", "%fpa18", "%fpa19", "%fpa20", "%fpa21", "%fpa22","%fpa23",\
 "%fpa24","%fpa25", "%fpa26", "%fpa27", "%fpa28", "%fpa29", "%fpa30","%fpa31"}

/* We do not want leading underscores.  */

#undef ASM_OUTPUT_LABELREF
#define ASM_OUTPUT_LABELREF(FILE,NAME)  \
  fprintf (FILE, "%s", NAME)
