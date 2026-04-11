#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "utils/utils.h"

#include "vm_device.h"
#include "vm_persistence.h"

BANKREF(VM_PERSISTENCE)

#define PERSISTENCE_HANDLE(HANDLE)    (HANDLE) // 0-based.
#define PERSISTENCE_ADDRESS(HANDLE)   (HANDLE) // 0-based.

const FSIGNATURE PERSISTENCE_SIGNATURE = 0x9BA5; // Can be calculated and overwritten by compiler.

UINT16 persistence_file_handles; // 1 bit for each, up to 16 files.

void persistence_init(void) BANKED {
    const UINT8 SRAM_BANKS = persistence_file_max_count();

    ENABLE_RAM;

    persistence_file_handles = 0x0000;
    for (UINT8 i = 0; i != SRAM_BANKS; ++i) {
        SWITCH_RAM(i);
        if (*((FSIGNATURE *)SRAM_OFFSET) != PERSISTENCE_SIGNATURE) {
            memset((POINTER)SRAM_OFFSET, 0, SRAM_BANK_SIZE);
            *((FSIGNATURE *)SRAM_OFFSET) = PERSISTENCE_SIGNATURE;
        }
    }

    DISABLE_RAM;
}

UINT8 persistence_file_max_count(void) BANKED {
    const INT8 sram_type = *(INT8 *)(SRAM_TYPE_ADDRESS);
    UINT8 sram_banks     = 0;
    switch (sram_type) {
    case 0x00: // 0KB.
        // Do nothing.

        break;
    case 0x01: // 2KB.
        sram_banks = 1;

        break;
    case 0x02: // 8KB.
        sram_banks = 1;

        break;
    case 0x03: // 32KB.
        sram_banks = 4;

        break;
    case 0x04: // 128KB.
        sram_banks = 16;

        break;
    }

    return sram_banks;
}

void vm_fopen(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT16 old   = persistence_file_handles;
    const UINT8 handle = PERSISTENCE_HANDLE((UINT8)*(--THIS->stack_ptr));

    if (handle >= persistence_file_max_count()) { // Handle out of bound.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    const UINT16 mask = 1 << handle;
    if ((persistence_file_handles & mask) != 0x0000) { // Already opened.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    persistence_file_handles |= mask;
    *(THIS->stack_ptr++) = TRUE;

    if (old == 0 && persistence_file_handles != 0x0000) {
        if (!FEATURE_RTC_ENABLED)
            ENABLE_RAM;

        SWITCH_RAM(handle);
    }
}

void vm_fclose(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 handle = PERSISTENCE_HANDLE((UINT8)*(--THIS->stack_ptr));

    if (handle >= persistence_file_max_count()) { // Handle out of bound.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    const UINT16 mask = 1 << handle;
    if ((persistence_file_handles & mask) == 0x0000) { // Already closed.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    persistence_file_handles &= ~mask;
    *(THIS->stack_ptr++) = TRUE;

    if (persistence_file_handles == 0x0000) {
        if (!FEATURE_RTC_ENABLED)
            DISABLE_RAM;
    }
}

void vm_fread(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 handle   = PERSISTENCE_HANDLE((UINT8)*(--THIS->stack_ptr));
    const UINT16 address = PERSISTENCE_ADDRESS((UINT16)*(--THIS->stack_ptr));

    if (handle >= persistence_file_max_count()) { // Handle out of bound.
        *(THIS->stack_ptr++) = 0;

        return;
    }

    const UINT16 mask = 1 << handle;
    if ((persistence_file_handles & mask) == 0x0000) { // Already closed.
        *(THIS->stack_ptr++) = 0;

        return;
    }

    if (address * sizeof(FDATA) + sizeof(FSIGNATURE) >= SRAM_BANK_SIZE) { // Address out of bound.
        *(THIS->stack_ptr++) = 0;

        return;
    }

    SWITCH_RAM(handle);
    *(THIS->stack_ptr++) = *((FDATA *)(SRAM_OFFSET + sizeof(FSIGNATURE)) + address);
}

void vm_fwrite(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 handle   = PERSISTENCE_HANDLE((UINT8)*(--THIS->stack_ptr));
    const UINT16 address = PERSISTENCE_ADDRESS((UINT16)*(--THIS->stack_ptr));
    const FDATA value    = (FDATA)*(--THIS->stack_ptr);

    if (handle >= persistence_file_max_count()) { // Handle out of bound.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    const UINT16 mask = 1 << handle;
    if ((persistence_file_handles & mask) == 0x0000) { // Already closed.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    if (address * sizeof(FDATA) + sizeof(FSIGNATURE) >= SRAM_BANK_SIZE) { // Address out of bound.
        *(THIS->stack_ptr++) = FALSE;

        return;
    }

    SWITCH_RAM(handle);
    *((FDATA *)(SRAM_OFFSET + sizeof(FSIGNATURE)) + address) = value;
    *(THIS->stack_ptr++) = TRUE;
}
