#include "DEFS.h"

ENTRY(__modsi3, 0)
	clrl	r0
	movl	4(fp),r1
	jgeq	1f
	mnegl	$1,r0
1:	ediv	8(fp),r0,r1,r0
	ret
