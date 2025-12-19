#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdio.h>

#include "utils/linked_list.h"
#include "utils/sleep.h"
#include "utils/text.h"
#include "utils/utils.h"

#include "vm_device.h"
#include "vm_system.h"

BANKREF(VM_SYSTEM)

// Returns the system time onto the VM stack.
void vm_sys_time(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    *A = sys_time;
}

// Sleeps for the specific milliseconds.
void vm_sleep(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    sleep(*A);
}

// Raises a VM exception.
void vm_raise(SCRIPT_CTX * THIS, UINT8 code, UINT8 size) OLDCALL BANKED {
#if VM_EXCEPTION_ENABLED
    VM_THROW(code, THIS->bank, (UINT16)THIS->PC);
#else /* VM_EXCEPTION_ENABLED */
    (void)code;
#endif /* VM_EXCEPTION_ENABLED */
    THIS->PC += size;
}

// Resets the VM.
void vm_reset(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;

    reset();
}

// Outputs debug information.
void vm_dbginfo(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UINT8 * reg = (UINT8 *)TRANSFER_STATUS_REG;
    if (!(device_type & DEVICE_TYPE_GBB)) // Ignore streaming if extension features are not supported.
        return;

    // Check for transfer status only on a device with extension support.
    if (*reg != TRANSFER_STATUS_READY) {
        // Still busy, wait for ready.
        THIS->PC -= INSTRUCTION_SIZE;

        return;
    }

    const BOOLEAN full = (BOOLEAN)*(--THIS->stack_ptr);
    unsigned char display_text[80];
    unsigned char * d = display_text;
    *d++ = '>'; // To the shell.
    *d++ = 'S'; *d++ = ':';
    if (full) {
        for (UINT8 i = 0; i != VM_MAX_CONTEXTS; ++i) {
            SCRIPT_CTX * ctx = &CTXS[i];
            SCRIPT_CTX * first = first_ctx;
            BOOLEAN alive;
            DL_CONTAINS(first, ctx, alive);
            if (alive) {
                const INT16 diff = DIV2((UINT8 *)ctx->stack_ptr - (UINT8 *)ctx->base_addr);
                d += int16_to_str(diff, d);
            } else {
                *d++ = '-';
            }
            ctx = ctx->next;
            if (i != VM_MAX_CONTEXTS - 1)
                *d++ = ',';
        }
    } else {
        d += uint16_to_hex((UINT16)THIS->stack_ptr, d);
    }

    *reg = TRANSFER_STATUS_BUSY;
    const UINT8 len = d - display_text;
    for (UINT8 i = 0; i != len; ++i) {
        unsigned char * c = display_text + i;
        if (i >= TRANSFER_MAX_SIZE)
            break;

        *(UINT8 *)(TRANSFER_ADDRESS + i) = *c; // Output to the shell buffer.
    }
    *reg = TRANSFER_STATUS_FILLED;
}
