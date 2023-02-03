/**
 *
 * 	Varargs for PYR/GNU CC
 *
 * WARNING -- WARNING -- DANGER
 *
 * The code in this file implements varargs for gcc on a pyr in
 * a way that is compatible with code compiled by the Pyramid Technology
 * C compiler.
 * As such, it depends strongly on the Pyramid conventions for
 * parameter passing.ct and indepenent implementation. 
 * These (somewhat bizarre) paramter-passing conventions are described
 * in the ``OSx Operating System Porting Guide''.
 * 
 * A quick summary is useful:
 * 12 of the 48 register-windowed regs available for
 * parameter passing.  Parameters of a function call that are eligible
 * to be passed in registers are assigned registers from TR0/PR0 onwards;
 * all other arguments are passed on the stack.
 * Structure and union parameters are *never* passed in registers,
 * even if they are small enough to fit.  They are always passed on
 * the stack.
 *
 * Double-sized parameters cannot be passed in TR11, because
 * TR12 is not used for passing parameters.  If, in the absence of this
 * rule, a double-sized param would have been passed in TR11,
 * that parameter is passed on the stack and no parameters are
 * passed in TR11.
 * 
 * It is only known to work for passing 32-bit integer quantities
 * (ie chars, shorts, ints/enums, longs), doubles, or pointers. 
 * Passing structures on a Pyramid via varargs is a loser.
 * Passing an object larger than 8 bytes on a pyramid via varargs may
 * also be a loser.
 * 
 */


/*
 *  pointer to next stack parameter in __va_buf[0]
 *  pointer to next parameter register in __va_buf[1]
 *  Count of registers seen at __va_buf[2]
 *  saved pr0..pr11 in __va_buf[3..14]
 *  # of calls to va_arg (debugging) at __va_buf[15]
 */

typedef void *__voidptr;
#if 1

typedef struct __va_regs {
      __voidptr __stackp,__regp,__count;
      __voidptr __pr0,__pr1,__pr2,__pr3,__pr4,__pr5,__pr6,__pr7,__pr8,__pr9,__pr10,__pr11;
  } __va_regs;

typedef __va_regs __va_buf;
#else

/* __va_buf[0] = address of next arg passed on the stack
   __va_buf[1] = address of next arg passed in a register
   __va_buf[2] = register-# of next arg passed in a register
*/
typedef __voidptr(*__va_buf);

#endif

#define va_alist \
  __va0,__va1,__va2,__va3,__va4,__va5,__va6,__va7,__va8,__va9,__va10,__va11, \
  __builtin_va_alist

#define va_dcl __voidptr va_alist;

#define va_list __va_buf


/* __asm ("rcsp %0" : "=r" ( _AP [0]));*/

#define va_start(_AP)  \
  _AP =  ((struct __va_regs) {						\
   &(_AP.__pr0), (void*)&__builtin_va_alist, (void*)0,			\
        __va0,__va1,__va2,__va3,__va4,__va5,				\
	__va6,__va7,__va8,__va9,__va10,__va11})
 
  
	 

#define va_arg(_AP, _MODE)	\
({__voidptr *__ap = (__voidptr*)&_AP;					\
  register int __size = sizeof (_MODE);					\
  register int __onstack =						\
	  (__size > 8 || ( (int)(__ap[2]) > 11) ||			\
	    (__size==8 && (int)(__ap[2])==11));				\
  register int* __param_addr =  ((int*)((__ap) [__onstack]));		\
									\
  ((void *)__ap[__onstack])+=__size;					\
    if (__onstack==0 || (int)(__ap[2])==11)				\
      __ap[2]+= (__size >> 2);						\
  *(( _MODE *)__param_addr);						\
})

#define va_end(_X)
