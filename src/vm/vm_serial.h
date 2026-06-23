#ifndef __VM_SERIAL_H__
#define __VM_SERIAL_H__

#include "vm.h"

#ifndef SERIAL_TIMEOUT_ENABLED
#   define SERIAL_TIMEOUT_ENABLED 1
#endif /* SERIAL_TIMEOUT_ENABLED */

BANKREF_EXTERN(VM_SERIAL)

#define SERIAL_ERROR     0xFFFF
#define SERIAL_BUSY      0xFFFE
#define SERIAL_IDLE      0xFFFD
#define SERIAL_TIMEOUT   0xFFFC

void vm_sread(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_swrite(SCRIPT_CTX * THIS) OLDCALL BANKED;

#endif /* __VM_SERIAL_H__ */
