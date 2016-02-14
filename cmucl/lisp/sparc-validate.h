/*

 $Header: sparc-validate.h,v 1.8 94/10/27 17:13:54 ram Exp $

 This code was written as part of the CMU Common Lisp project at
 Carnegie Mellon University, and has been placed in the public domain.

*/


#define READ_ONLY_SPACE_START	(0x01000000)
#define READ_ONLY_SPACE_SIZE	(0x03ff8000)
  
#define STATIC_SPACE_START  	(0x05000000)
#define STATIC_SPACE_SIZE   	(0x01ff8000)

#define DYNAMIC_0_SPACE_START	(0x07000000)
#define DYNAMIC_1_SPACE_START	(0x0b000000)
#define DYNAMIC_SPACE_SIZE  	(0x03ff8000)

#define CONTROL_STACK_START 	(0x00900000)
#define CONTROL_STACK_SIZE  	(0x00078000)

#define BINDING_STACK_START 	(0x00980000)
#define BINDING_STACK_SIZE  	(0x00078000)

#define HOLES {0x04ff8000, 0x06ff8000, 0x0aff8000, 0x1fff8000}
#define HOLE_SIZE 0x2000
