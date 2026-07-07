#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 110
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_memory.h"

#include "utils/utils.h"

BANKREF(VM_MEMORY)

// Copies arbitrary data from an address in memory into another place.
void vm_memcpy(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    UINT8 * dst        = (UINT8 *)*(--THIS->stack_ptr);
    UINT16 bank        = (UINT16)*(--THIS->stack_ptr);
    UINT8 * ptr        = (UINT8 *)*(--THIS->stack_ptr);
    UINT16 len         = (UINT16)*(--THIS->stack_ptr);
    const INT16 offset = (UINT16)*(--THIS->stack_ptr);

    switch (src) {
    case ASSET_SOURCE_READ:
        len  = MUL2(len); // Words to bytes.
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        current_data_address += len;

        break;
    case ASSET_SOURCE_DATA:
        len  = MUL2(len); // Words to bytes.
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        THIS->PC += len;

        break;
    case ASSET_SOURCE_FAR: // Fall through.
    default:
        // Do nothing.

        break;
    }
    ptr += offset;

    if (bank == 0xFFFF) { // No bank changing is needed.
        vmemcpy(dst, ptr, len);

        return;
    }

    call_v_www((UINT16)dst, (UINT16)ptr, len, (UINT8)bank, (v_www_fn)vmemcpy);
}

// Sets the values of a range of space.
void vm_memset(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UINT8 * dst        = (UINT8 *)*(--THIS->stack_ptr);
    const UINT8 value  = (UINT8)*(--THIS->stack_ptr);
    const UINT16 count = (UINT16)*(--THIS->stack_ptr);

    vmemset(dst, value, count);
}

// Adds the specific value to the values of a range of space.
void vm_memadd(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UINT8 * dst        = (UINT8 *)*(--THIS->stack_ptr);
    const INT8 value   = (INT8)*(--THIS->stack_ptr);
    const UINT16 count = (UINT16)*(--THIS->stack_ptr);

    if (value == 0)
        return;

    const UINT8 * end = dst + count;
    if ((UINT16)dst >= VRAM_BEGIN && (UINT16)dst <= VRAM_END) { // Is VRAM.
        for (; dst != end; ++dst)
            set_vram_byte(dst, get_vram_byte(dst) + value);
    } else {
        for (; dst != end; ++dst)
            *dst += value;
    }
}
