/* Definitions for Intel 386 running SunOS 4.0.
   Copyright (C) 1988 Free Software Foundation, Inc.

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


#include "tm-i386.h"

/* Use the Sun assembler syntax.  */

#include "tm-sun386.h"

/* By default, target has a 80387.  */

#define TARGET_DEFAULT 1

/* Use crt0.o as a startup file.  */

#define STARTFILE_SPEC  \
  "%{pg:gcrt0.o%s}%{!pg:%{p:mcrt0.o%s}%{!p:crt0.o%s}}"

#define LIB_SPEC "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}\
%{sun386:}"
/* That last item is just to prevent a spurious error.  */

/* It is hard to debug with shared libraries,
   so don't use them if going to debug.  */

#define LINK_SPEC "%{g:-Bstatic} %{static:-Bstatic} %{Bstatic}"

/* Extra switches to give the assembler.  */

#define ASM_SPEC "-i386"

/* Specify predefined symbols in preprocessor.  */

#define CPP_PREDEFINES "-Dunix -Di386 -Dsun386 -Dsun"

/* Allow #sccs in preprocessor.  */

#define SCCS_DIRECTIVE

/* Output #ident as a .ident.  */

#define ASM_OUTPUT_IDENT(FILE, NAME) fprintf (FILE, "\t.ident \"%s\"\n", NAME);

/* We don't want to output SDB debugging information.  */

#undef SDB_DEBUGGING_INFO

/* We want to output DBX debugging information.  */

#define DBX_DEBUGGING_INFO

/* Implicit library calls should use memcpy, not bcopy, etc.  */

#define TARGET_MEM_FUNCTIONS

/* Force structure alignment to the type used for a bitfield.  */

#define PCC_BITFIELD_TYPE_MATTERS

/* Define how to find the value returned by a function.
   VALTYPE is the data type of the value (as a tree).
   If the precise function being called is known, FUNC is its FUNCTION_DECL;
   otherwise, FUNC is 0.  */

#define VALUE_REGNO(MODE) \
  (((MODE)==SFmode || (MODE)==DFmode) ? FIRST_FLOAT_REG : 0)

/* 1 if N is a possible register number for a function value. */

#define FUNCTION_VALUE_REGNO_P(N) ((N) == 0 || (N)== FIRST_FLOAT_REG)

/* This is partly guess.  */

#undef DBX_REGISTER_NUMBER
#define DBX_REGISTER_NUMBER(n) \
  ((n) == 0 ? 11 : (n)  == 1 ? 9 : (n) == 2 ? 10 : (n) == 3 ? 8	\
   : (n) == 4 ? 5 : (n) == 5 ? 4 : (n) == 6 ? 6 : (n))

/* Every debugger symbol must be in the text section.
   Otherwise the assembler or the linker screws up.  */

#define DEBUG_SYMS_TEXT

/* This NOP insn makes profiling not fail.  */

#define ASM_IDENTIFY_GCC(FILE) \
fprintf (FILE, (profile_flag ? "gcc_compiled.:\n\tnop\n" : "gcc_compiled.:\n"))
