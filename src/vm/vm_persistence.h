#ifndef __VM_PERSISTENCE_H__
#define __VM_PERSISTENCE_H__

#include "vm.h"

BANKREF_EXTERN(VM_PERSISTENCE)

#define PERSISTENCE_FILE_OPENED   (persistence_file_handles != 0x0000)

typedef UINT16 FSIGNATURE;
typedef UINT8 FDATA;

extern UINT16 persistence_file_handles;

void persistence_init(void) BANKED;
UINT8 persistence_file_max_count(void) BANKED;

void vm_fopen(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_fclose(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_fread(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_fwrite(SCRIPT_CTX * THIS) OLDCALL BANKED;

#endif /* __VM_PERSISTENCE_H__ */
