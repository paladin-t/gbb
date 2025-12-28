/** @file types.h
    Basic types.  
    Directly include the port specific file.
*/
#ifndef TYPES_INCLUDE
#define TYPES_INCLUDE

#include <asm/types.h>

#ifndef NULL
  #define NULL (void *)0
#endif

/** A 'false' value.
 */
#define FALSE 0
/** A 'true' value.
 */
#define TRUE 1

/** No longer used.
 */
typedef void * POINTER;

#endif
