#pragma bank 1

#if defined __SDCC
#   include <gbdk/console.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <rand.h>

#include "utils/font.h"
#include "utils/text.h"

#include "vm.h"
#include "vm_actor.h"
#include "vm_audio.h"
#include "vm_device.h"
#include "vm_effects.h"
#include "vm_emote.h"
#include "vm_game.h"
#include "vm_graphics.h"
#include "vm_gui.h"
#include "vm_input.h"
#include "vm_native.h"
#include "vm_persistence.h"
#include "vm_projectile.h"
#include "vm_scene.h"
#include "vm_trigger.h"

BANKREF(VM_MAIN)

BANKREF_EXTERN(BOOTSTRAP)

// Define all the VM instructions: their handlers, bank and parameter lengths in
// bytes. This array must be NONBANKED as well as `VM_STEP(...)`.
extern const SCRIPT_CMD script_cmds[];

// We need BANKED functions here to have two extra words before arguments. We
// will put VM stuff there, plus we get an ability to call them from wherever we
// want in native code. You can manipulate context (THIS) within VM functions.
// If VM function has no parameters and does not manipulate context, then you
// may declare it without params at all bacause caller clears stack - that is
// safe.

// Does nothing.
void vm_nop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;

    // Do nothing.
}

// Reserves a number of slots on the VM stack.
void vm_reserve(SCRIPT_CTX * THIS, INT8 ofs) OLDCALL BANKED {
    THIS->stack_ptr += ofs;
}

// Pushes the immediate value onto the VM stack. You can also invent calling
// convention and pass parameters to scripts on the VM stack, make a library of
// scripts and so on.
void vm_push(SCRIPT_CTX * THIS, UINT16 value) OLDCALL BANKED {
    *(THIS->stack_ptr++) = value;
}

// Pushes the value on the VM stack or global onto the VM stack.
void vm_push_value(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED {
    *(THIS->stack_ptr) = *(UINT16 *)VM_REF_TO_PTR(idx);
    ++THIS->stack_ptr;
}

// Removes `n` values from the VM stack and returns the last one.
UINT16 vm_pop(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED {
    if (n) THIS->stack_ptr -= n;

    return *(THIS->stack_ptr);
}

// Ignores one value and removes `n` values from the VM stack and returns the last one.
UINT16 vm_pop_1(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED {
    UINT16 top = (UINT16)*(THIS->stack_ptr - 1);
    if (n) THIS->stack_ptr -= n;
    *(THIS->stack_ptr - 1) = top;

    return top;
}

// Calls by the near address.
void vm_call(SCRIPT_CTX * THIS, UINT8 * pc) OLDCALL BANKED {
    *(THIS->stack_ptr++) = (UINT16)THIS->PC;
    THIS->PC = pc;
}

// Returns from a near call.
void vm_ret(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED {
    // Pop VM PC from VM stack.
    THIS->PC = (const UINT8 *)*(--THIS->stack_ptr);
    if (n) THIS->stack_ptr -= n;
}

// Calls by the far address.
void vm_call_far(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED {
    *(THIS->stack_ptr++) = (UINT16)THIS->PC;
    *(THIS->stack_ptr++) = THIS->bank;
    THIS->PC = pc;
    THIS->bank = bank;
}

// Returns from a far call.
void vm_ret_far(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED {
    THIS->bank = (UINT8)(*(--THIS->stack_ptr));
    THIS->PC = (const UINT8 *)*(--THIS->stack_ptr);
    if (n) THIS->stack_ptr -= n;
}

// Jumps to the near address.
void vm_jump(SCRIPT_CTX * THIS, UINT8 * pc) OLDCALL BANKED {
    THIS->PC = pc;
}

// Jumps to the far address.
void vm_jump_far(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED {
    THIS->PC = pc;
    THIS->bank = bank;
}

// Steps to the next bank.
void vm_next_bank(SCRIPT_CTX * THIS) OLDCALL BANKED {
    THIS->PC = (UINT8 *)BROM_ADDRESS; // Start address of a ROM bank.
    ++THIS->bank;
}

// Compares variable with a set of values, if it equals to one of them then jump
// to the specified label.
void vm_switch(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, INT16 idx, UINT8 size, UINT8 n, BOOLEAN call) OLDCALL NONBANKED {
    (void)dummy0;
    (void)dummy1;

    INT16 value;
    INT16 * table;

    value = *(INT16 *)VM_REF_TO_PTR(idx);
    if (n) THIS->stack_ptr -= n;            // Dispose values on VM stack if required.

    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(THIS->bank);

    table = (INT16 *)(THIS->PC);
    while (size) {
        if (value == *table++) {            // Condition met.
            if (call) {
                *(THIS->stack_ptr++) = (UINT16)THIS->PC;
                *(THIS->stack_ptr++) = THIS->bank;
            }
            THIS->PC = (UINT8 *)(*table);
            ++table;
            THIS->bank = *(UINT8 *)table;   // Perform a jump.
            SWITCH_ROM_BANK(_save);

            return;
        } else {
            table = (INT16 *)((UINT8 *)table + 3);
        }
        --size;
    }

    SWITCH_ROM_BANK(_save);
    THIS->PC = (UINT8 *)table;              // Make the PC points to the next instruction.
}

// Compares the value on the VM stack or global, with another one.
void vm_if(SCRIPT_CTX * THIS, UINT8 condition, INT16 idxA, INT16 idxB, UINT8 * pc, UINT8 n) OLDCALL BANKED {
    INT16 A, B;
    A = *(INT16 *)VM_REF_TO_PTR(idxA);
    B = *(INT16 *)VM_REF_TO_PTR(idxB);
    UINT8 res = FALSE;
    switch (condition) {
    case VM_OP_EQ: res = (A == B); break;
    case VM_OP_LT: res = (A <  B); break;
    case VM_OP_LE: res = (A <= B); break;
    case VM_OP_GT: res = (A >  B); break;
    case VM_OP_GE: res = (A >= B); break;
    case VM_OP_NE: res = (A != B); break;
    }
    if (res) THIS->PC = pc;
    if (n) THIS->stack_ptr -= n;
}

// Compares the value on the VM stack or global, with the immediate value.
void vm_if_const(SCRIPT_CTX * THIS, UINT8 condition, INT16 idxA, INT16 B, UINT8 * pc, UINT8 n) OLDCALL BANKED {
    INT16 A;
    A = *(INT16 *)VM_REF_TO_PTR(idxA);
    UINT8 res = FALSE;
    switch (condition) {
    case VM_OP_EQ: res = (A == B); break;
    case VM_OP_LT: res = (A <  B); break;
    case VM_OP_LE: res = (A <= B); break;
    case VM_OP_GT: res = (A >  B); break;
    case VM_OP_GE: res = (A >= B); break;
    case VM_OP_NE: res = (A != B); break;
    }
    if (res) THIS->PC = pc;
    if (n) THIS->stack_ptr -= n;
}

// Check the condition on the VM stack or global, returns either the first or second value.
void vm_iif(SCRIPT_CTX * THIS, INT16 idxR, INT16 idxC, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    INT16 * R;
    R = VM_REF_TO_PTR(idxR);
    INT16 C;
    C = *(INT16 *)VM_REF_TO_PTR(idxC);
    if (C) {
        INT16 * A;
        A = VM_REF_TO_PTR(idxA);
        *R = *A;
    } else {
        INT16 * B;
        B = VM_REF_TO_PTR(idxB);
        *R = *B;
    }
}

// Loops by the near address within the range of constant values.
void vm_loop(SCRIPT_CTX * THIS, INT16 idxA, INT16 B, INT16 C, UINT8 * pc, UINT8 n) OLDCALL BANKED {
    INT16 * A;
    A = VM_REF_TO_PTR(idxA);
    if ((C > 0 && *A < B) || (C < 0 && *A > B)) {
        THIS->PC = pc; // Continue loop.
        *A += C; // Increase one step.
    }
    if (n) THIS->stack_ptr -= n;
}

// Loops by the near address within the range.
void vm_for(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, INT16 idxC, UINT8 * pc, UINT8 n) OLDCALL BANKED {
    INT16 A;
    A = *(INT16 *)VM_REF_TO_PTR(idxA);
    INT16 B;
    B = *(INT16 *)VM_REF_TO_PTR(idxB);
    INT16 C;
    C = *(INT16 *)VM_REF_TO_PTR(idxC);
    if ((C > 0 && A > B) || (C < 0 && A < B)) {
        THIS->PC = pc; // Break from loop.
    }
    if (n) THIS->stack_ptr -= n;
}

// Sets the value on the VM stack or global, with the value on the VM stack or
// global.
void vm_set(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_REF_TO_PTR(idxA);
    B = VM_REF_TO_PTR(idxB);
    *A = *B;
}

// Sets the value on the VM stack or global, with the immediate value.
void vm_set_const(SCRIPT_CTX * THIS, INT16 idx, INT16 value) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    *A = value;
}

// Sets the value on the VM stack or global indirectly, with the value on the VM
// stack or global.
void vm_set_indirect(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    // Prepare.
    INT16 * A, * B;

    // Get the target address indirectly.
    A = VM_REF_TO_PTR(idxA);
    A = VM_REF_TO_PTR(*A);

    // Get the source address.
    B = VM_REF_TO_PTR(idxB);

    // Assign.
    *A = *B;
}

// Gets the value on the VM stack or global indirectly.
void vm_get_indirect(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    // Prepare.
    INT16 * A, * B;

    // Get the target address.
    A = VM_REF_TO_PTR(idxA);

    // Get the source address indirectly.
    B = VM_REF_TO_PTR(idxB);
    B = VM_REF_TO_PTR(*B);

    // Assign.
    *A = *B;
}

// Accumulates the value on the VM stack or global, with another value.
void vm_acc(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, UINT8 n) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_REF_TO_PTR(idxA);
    B = VM_REF_TO_PTR(idxB);
    *A += *B;
    if (n) THIS->stack_ptr -= n;
}

// Accumulates the value on the VM stack or global, with the immediate value.
void vm_acc_const(SCRIPT_CTX * THIS, INT16 idxA, INT16 value) OLDCALL BANKED {
    INT16 * A;
    A = VM_REF_TO_PTR(idxA);
    *A += value;
}

// Calculation helpers.
STATIC INT16 sign_of(INT16 val) {
    return SIGN(val);
}

// Calculates and returns the result on the VM stack. Must be NONBANKED because
// we access the VM bytecode. The dummy parameters are needed to make NONBANKED
// function to be compatible with banked call.
void vm_rpn(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS) OLDCALL NONBANKED {
    (void)dummy0;
    (void)dummy1;

    INT16 * A, * B, * ARGS;
    INT16 idx;

    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(THIS->bank);

    ARGS = THIS->stack_ptr;
    while (TRUE) {
        INT8 op = *(THIS->PC++);
        if (op < 0) {
            switch (op) {
            // Reference.
            case VM_OP_R_REF:
                idx = *((INT16 *)(THIS->PC));
                if (idx < 0) A = ARGS + idx; else A = script_memory + idx;
                *(THIS->stack_ptr) = *A;
                THIS->PC += 2;

                break;

            // Int16.
            case VM_OP_R_INT16:
                *(THIS->stack_ptr) = *((UINT16 *)(THIS->PC));
                THIS->PC += 2;

                break;

            // Int8.
            case VM_OP_R_INT8:
                op = *(THIS->PC++);
                *(THIS->stack_ptr) = op;

                break;

            default:
                SWITCH_ROM_BANK(_save);

                return;
            }

            ++THIS->stack_ptr;
        } else {
            A = THIS->stack_ptr - 2; B = A + 1;
            switch (op) {
            // Logical.
            case VM_OP_EQ:    *A = (*A == *B);                break;
            case VM_OP_LT:    *A = (*A <  *B);                break;
            case VM_OP_LE:    *A = (*A <= *B);                break;
            case VM_OP_GT:    *A = (*A >  *B);                break;
            case VM_OP_GE:    *A = (*A >= *B);                break;
            case VM_OP_NE:    *A = (*A != *B);                break;
            case VM_OP_AND:   *A = ((bool)*A && (bool)*B);    break;
            case VM_OP_OR:    *A = ((bool)*A || (bool)*B);    break;
            case VM_OP_NOT:   *B = !(*B);                     continue;

            // Arithmetics.
            case VM_OP_ADD:   *A = *A + *B;                   break;
            case VM_OP_SUB:   *A = *A - *B;                   break;
            case VM_OP_MUL:   *A = *A * *B;                   break;
            case VM_OP_DIV:   *A = *A / *B;                   break;
            case VM_OP_MOD:   *A = *A % *B;                   break;

            // Bitwise.
            case VM_OP_BAND:  *A = *A &  *B;                  break;
            case VM_OP_BOR:   *A = *A |  *B;                  break;
            case VM_OP_BXOR:  *A = *A ^  *B;                  break;
            case VM_OP_SHL:   *A = *A << *B;                  break;
            case VM_OP_SHR:   *A = *A >> *B;                  break;

            // Unary.
            case VM_OP_BNOT:  *B = ~(*B);                     continue;
            case VM_OP_NEG:   *B = -(*B);                     continue;
            case VM_OP_SGN:   *B = sign_of(*B);               continue;
            case VM_OP_ABS:   *B = abs(*B);                   continue;
            case VM_OP_SQR:   *B = *B * *B;                   continue;
            case VM_OP_SQRT:  *B = sqrt_uint16((UINT16)*B);   continue;
            case VM_OP_SIN:   *B = SIN(*B);                   continue;
            case VM_OP_COS:   *B = COS(*B);                   continue;

            // Functions.
            case VM_OP_ATAN2: *A = atan2(*A, *B);             break;
            case VM_OP_POW:   *A = pow_int16(*A, *B);         break;
            case VM_OP_MIN:   *A = (*A < *B) ? *A : *B;       break;
            case VM_OP_MAX:   *A = (*A > *B) ? *A : *B;       break;

            // Terminator.
            default:
                SWITCH_ROM_BANK(_save);

                return;
            }

            --THIS->stack_ptr;
        }
    }
}

// Gets the thread local variable; non-negative index of the second argument
// points to a thread local variable (arguments passed into thread).
void vm_get_tlocal(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_REF_TO_PTR(idxA);
    B = VM_STK_TO_PTR(idxB);
    *A = *B;
}

// Sets the thread local variable; non-negative index of the first argument
// points to a thread local variable (arguments passed into thread).
void vm_set_tlocal(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_STK_TO_PTR(idxA);
    B = VM_REF_TO_PTR(idxB);
    *A = *B;
}

// Packs a number of parts into an `INT16` in the VM RAM.
void vm_pack(SCRIPT_CTX * THIS, INT16 idx, INT16 idx0, INT16 idx1, INT16 idx2, INT16 idx3, BOOLEAN n) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    if (n == 0) { // Pack 2 bytes.
        UINT8 * p0, * p1;
        p0 = VM_REF_TO_PTR(idx0);
        p1 = VM_REF_TO_PTR(idx1);
        *A = (*p1 << 8) | (*p0);
    } else { // Pack 4 nibbles.
        UINT8 * p0, * p1, * p2, * p3;
        p0 = VM_REF_TO_PTR(idx0);
        p1 = VM_REF_TO_PTR(idx1);
        p2 = VM_REF_TO_PTR(idx2);
        p3 = VM_REF_TO_PTR(idx3);
        *A = (*p3 << 12) | (*p2 << 8) | (*p1 << 4) | (*p0);
    }
}

// Unpacks a number of parts from an `INT16` in the VM RAM.
void vm_unpack(SCRIPT_CTX * THIS, INT16 idx, INT16 idx0, INT16 idx1, INT16 idx2, INT16 idx3, BOOLEAN n) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    if (n == 0) { // Unpack 2 bytes.
        UINT16 * p0, * p1;
        p0 = VM_REF_TO_PTR(idx0);
        p1 = VM_REF_TO_PTR(idx1);
        *p0 = *A & 0xFF;
        *p1 = (*A >> 8) & 0xFF;
    } else { // Unpack 4 nibbles.
        UINT16 * p0, * p1, * p2, * p3;
        p0 = VM_REF_TO_PTR(idx0);
        p1 = VM_REF_TO_PTR(idx1);
        p2 = VM_REF_TO_PTR(idx2);
        p3 = VM_REF_TO_PTR(idx3);
        *p0 = *A & 0x0F;
        *p1 = (*A >> 4) & 0x0F;
        *p2 = (*A >> 8) & 0x0F;
        *p3 = (*A >> 12) & 0x0F;
    }
}

// Swaps two variables.
void vm_swap(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_REF_TO_PTR(idxA);
    B = VM_REF_TO_PTR(idxB);
    INT16 tmp = *A;
    *A = *B;
    *B = tmp;
}

// Cursor of the data reading stream.
#define DATA_OFFSET   4
UINT8 current_data_bank = BANK(BOOTSTRAP);
UINT16 current_data_address = BROM_ADDRESS + DATA_OFFSET /* skip the head `VM_JUMP_FAR` instruction */;

// Sets the cursor of the data reading stream. Returns the bank if `for_bank` is `true`, otherwise returns the address.
void vm_data_ptr(SCRIPT_CTX * THIS, INT16 idx, BOOLEAN for_bank) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    if (for_bank)
        *A = current_data_bank;
    else
        *A = current_data_address;
}

// Reads an `UINT8` or `INT16` from the current data reading stream.
void vm_read(SCRIPT_CTX * THIS, INT16 idx, UINT16 size, BOOLEAN copy) OLDCALL BANKED {
    if (!copy) {
        current_data_address += size;

        return;
    }

    INT16 * A;
    A = VM_REF_TO_PTR(idx);
    if (size == 1) {
        *A = get_uint8(current_data_bank, (UINT8 *)current_data_address++);
    } else if (size == 2) {
        *A = get_int16(current_data_bank, (UINT8 *)current_data_address); current_data_address += 2;
    } else {
        call_v_www((UINT16)A, current_data_address, size, current_data_bank, (v_www_fn)vmemcpy);
        current_data_address += size;
    }
}

// Restores the cursor of the data reading stream.
void vm_restore(SCRIPT_CTX * THIS, UINT8 bank, INT16 idx) OLDCALL BANKED {
    INT16 * A;
    A = VM_REF_TO_PTR(idx);
    current_data_bank = bank;
    current_data_address = *A; // 0-based.
}

// Spawns the thread in a separate context.
void vm_begin_thread(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc, BOOLEAN put, INT16 idx, UINT8 nargs) OLDCALL BANKED {
    UINT16 * A;
    if (put) {
        A = VM_REF_TO_PTR(idx);
    } else {
        A = NULL; // It is a temp variable.
    }

    SCRIPT_CTX * ctx = script_execute(bank, pc, A, 0);
    if (!nargs)
        return;
    if (!ctx)
        return;

    for (UINT8 i = nargs; i != 0; --i) { // Initialize thread locals if any.
        INT16 B = get_int16_be(THIS->bank, (UINT8 *)THIS->PC);
        THIS->PC += 2;
        B = *(INT16 *)VM_REF_TO_PTR(B);
        *(ctx->stack_ptr++) = (UINT16)B;
    }
}

// Joins the thread.
void vm_join(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    if (!VM_IS_TERMINATED(*A)) {
        THIS->PC -= INSTRUCTION_SIZE + sizeof(idx);
        THIS->waitable = TRUE;
    }
}

// Terminates the thread.
void vm_terminate(SCRIPT_CTX * THIS, INT16 idx, BOOLEAN all) OLDCALL BANKED {
    if (all) {
        SCRIPT_CTX * ctx = first_ctx;
        while (ctx) {
            if (ctx != THIS) {
                if (ctx->hthread) {
                    *(ctx->hthread) |= SCRIPT_TERMINATED;
                    ctx->hthread = NULL;
                }
                ctx->terminated = TRUE;
            }
            ctx = ctx->next;
        }

        return;
    }

    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    script_terminate((UINT8)(*A), TRUE);
}

// Signals the runner that the context will be in a waitable state.
void vm_wait(SCRIPT_CTX * THIS) OLDCALL BANKED {
    THIS->waitable = TRUE;
}

// Signals the runner that the context will be in a waitable state for the specific frames.
void vm_wait_n(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED {
    // Prepare.
    const UINT8 bank = BANK(VM_NATIVE);
    UINT8 * fn = (UINT8 *)wait_for;
    const UINT8 nparams = 1;
    UINT16 * stack_frame = VM_REF_TO_PTR(idx);

    // Update the function pointer.
    const BOOLEAN start = ((THIS->update_fn != fn) || (THIS->update_fn_bank != bank)) ? THIS->update_fn = fn, THIS->update_fn_bank = bank, (BOOLEAN)TRUE : (BOOLEAN)FALSE;

    // Call the handler.
    if (FAR_CALL_EX(fn, bank, SCRIPT_UPDATE_FN, THIS, start, stack_frame)) {
        THIS->stack_ptr -= nparams;
        THIS->update_fn = NULL, THIS->update_fn_bank = 0;

        return;
    }

    // Call the handler again, wait if the condition is not met.
    THIS->PC -= INSTRUCTION_SIZE + sizeof(idx);
}

// Sets the lock flag for the current context.
void vm_lock(SCRIPT_CTX * THIS) OLDCALL BANKED {
    ++THIS->lock_count;
    ++vm_lock_state;
}

// Resets the lock flag for the current context.
void vm_unlock(SCRIPT_CTX * THIS) OLDCALL BANKED {
    if (THIS->lock_count == 0)
        return;

    --THIS->lock_count;
    --vm_lock_state;
}

// Invokes the C function at `<bank>:<fn>` until it returns true. Callee
// cleanups the stack.
void vm_invoke_fn(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * fn, UINT8 nparams, INT16 idx) OLDCALL BANKED {
    // Prepare.
    UINT16 * stack_frame = VM_REF_TO_PTR(idx);

    // Update the function pointer.
    const BOOLEAN start = ((THIS->update_fn != fn) || (THIS->update_fn_bank != bank)) ? THIS->update_fn = fn, THIS->update_fn_bank = bank, (BOOLEAN)TRUE : (BOOLEAN)FALSE;

    // Call the handler.
    if (FAR_CALL_EX(fn, bank, SCRIPT_UPDATE_FN, THIS, start, stack_frame)) {
        if (nparams) THIS->stack_ptr -= nparams;
        THIS->update_fn = NULL, THIS->update_fn_bank = 0;

        return;
    }

    // Call the handler again, wait if the condition is not met.
    THIS->PC -= INSTRUCTION_SIZE + sizeof(bank) + sizeof(fn) + sizeof(nparams) + sizeof(idx);
}

// Sets the cursor to the specific location.
void vm_locate(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED {
    INT16 A, B;
    A = *(INT16 *)VM_REF_TO_PTR(idxA);
    B = *(INT16 *)VM_REF_TO_PTR(idxB);
    gotoxy((UINT8)A, (UINT8)B);
}

// Prints something.
void vm_print(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 nargs) OLDCALL BANKED {
    print(THIS, new_line, nargs, TEXT_PRINT_MODE_TEXT);
}

// Sets the random seed with the value.
void vm_srand(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED {
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    initrand(*A);
}

// Gets a random value between `min` and `max`.
void vm_rand(SCRIPT_CTX * THIS, INT16 idx, INT16 idxMin, INT16 idxMax) OLDCALL BANKED {
    UINT16 min, max;
    min = *(UINT16 *)VM_REF_TO_PTR(idxMin);
    max = *(UINT16 *)VM_REF_TO_PTR(idxMax);
    UINT16 value = randw();
    value %= (max - min + 1);
    value += min;
    UINT16 * A;
    A = VM_REF_TO_PTR(idx);
    *A = value;
}

// Gets the `UINT8` from the system RAM.
void vm_peek(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, BOOLEAN word) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_REF_TO_PTR(idxA);
    B = VM_REF_TO_PTR(idxB);
    if (word == FALSE) {
        const UINT8 * addr = (UINT8 *)(*B);
        *A = *addr;
    } else {
        const UINT16 * addr = (UINT16 *)(*B);
        *A = *addr;
    }
}

// Sets the value on the system RAM, with the value on the VM stack or global.
void vm_poke(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, BOOLEAN word) OLDCALL BANKED {
    INT16 * A, * B;
    A = VM_REF_TO_PTR(idxA);
    B = VM_REF_TO_PTR(idxB);
    if (word == FALSE) {
        UINT8 * addr = (UINT8 *)(*A);
        *addr = (UINT8)(*B);
    } else {
        UINT16 * addr = (UINT16 *)(*A);
        *addr = (UINT16)(*B);
    }
}

// Sets the values of a range of space.
void vm_fill(SCRIPT_CTX * THIS, INT16 idx, INT16 value, INT16 count) OLDCALL BANKED {
    for (INT16 i = 0, * v = VM_REF_TO_PTR(idx); i != count; ++i) *v++ = value;
}

// Executes one step in the passed context.
static SFR current_fn_bank;
static SFR current_fn_nargs;
static UINT16 current_sp;
BOOLEAN VM_STEP(SCRIPT_CTX * CTX) NAKED NONBANKED STEP_FUNC_ATTR {
    (void)CTX;

#if defined __SDCC && defined NINTENDO
__asm
        ld b, d
        ld c, e                     ; BC = THIS.

        ld a, (de)
        ld l, a
        inc de
        ld a, (de)
        ld h, a                     ; HL offset of the script.
        inc de

        ldh a, (__current_bank)
        push af

        ld a, (de)                  ; Bank of the script.
        ldh (__current_bank), a
        ld (_rROMB0), a             ; Switch bank with VM code.

        ld a, (hl+)                 ; Load current command and return if terminator.
        ld e, a
        or a
        jr z, 3$

        ld (_current_sp), sp

        push bc                     ; Store BC == THIS.
        push hl

        ld h, #0
        ld l, a
        add hl, hl
        add hl, hl                  ; HL = instruction * sizeof(SCRIPT_CMD).
        dec hl
        ld de, #_script_cmds
        add hl, de                  ; HL = &script_cmds[instruction].args_len.

        ld a, (hl-)
        ldh (_current_fn_nargs), a
        ld a, (hl-)
        ldh (_current_fn_bank), a
        ld a, (hl-)
        ld b, a
        ld c, (hl)                  ; BC = fn.

        pop hl                      ; HL points to the next VM instruction or a first byte of the args.
        ldh a, (_current_fn_nargs)
        srl a
        jr nc, 4$                   ; D is even?
        ld d, (hl)                  ; Copy one arg onto stack.
        inc hl
        push de
        inc sp
4$:
        jr z, 1$                    ; Only one arg?
2$:
        ld d, (hl)
        inc hl
        ld e, (hl)
        inc hl
        push de
        dec a
        jr nz, 2$                   ; Loop through remaining args, copy 2 bytes at a time.
1$:
        ld d, h
        ld e, l                     ; DE points to the next VM instruction.

        ld hl, #_current_sp
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        dec hl

        ld a, (hl-)
        ld l, (hl)
        ld h, a                     ; HL = THIS.

        push hl                     ; Pushing THIS.

        ld a, e
        ld (hl+), a
        ld (hl), d                  ; PC = PC + sizeof(instruction) + args_len.

        ld hl, #_current_sp
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        push hl                     ; Not used.
        push hl                     ; SP to restore.

        ldh a, (_current_fn_bank)   ; A = script_bank.
        ldh (__current_bank), a
        ld (_rROMB0), a             ; Switch bank with functions.

        ld h, b                     ; Restore function pointer.
        ld l, c
        rst 0x20                    ; Call HL.

        pop hl
        ld sp, hl

        pop af
        ldh (__current_bank), a
        ld (_rROMB0), a             ; Restore bank.

        ld a, #1                    ; Instruction executed.
        ret
3$:
        pop af
        ldh (__current_bank), a
        ld (_rROMB0), a             ; Restore bank.

        xor a                       ; VM_STOP encountered.
        ret
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */
}

// The shared context memory.
UINT16 script_memory[VM_HEAP_SIZE + (VM_MAX_CONTEXTS * VM_CONTEXT_STACK_SIZE)];

// The contexts for executing scripts.
SCRIPT_CTX CTXS[VM_MAX_CONTEXTS];
SCRIPT_CTX * first_ctx, * free_ctxs;

// The lock state.
UINT8 vm_lock_state;
// The exception flag and parameters.
#if VM_EXCEPTION_ENABLED
UINT8 vm_exception_code;   // Exception type.
UINT8 vm_exception_source; // Exception source or parameters bank.
UINT16 vm_exception_data;  // Exception data or parameters address.
#endif /* VM_EXCEPTION_ENABLED */

void VBL_isr(void) NONBANKED {
    if (FEATURE_MAP_MOVEMENT_FLAG) {
        FEATURE_MAP_MOVEMENT_CLEAR;
        graphics_put_map();
    }
}

void LCD_isr(void) NONBANKED {
    if (FEATURE_EFFECT_PARALLAX_ENABLED) {
        effects_parallax_sync();
    } else if (effects_wobble) {
        effects_wobble_sync();
    }
}

STATIC void reset_contexts(BOOLEAN reset) {
    if (reset) {
        memset(script_memory, 0, sizeof(script_memory));
        memset(CTXS, 0, sizeof(CTXS));
    }
    UINT16 * base_addr = script_memory + VM_HEAP_SIZE;
    free_ctxs = CTXS, first_ctx = NULL;
    SCRIPT_CTX * nxt = NULL;
    SCRIPT_CTX * tmp = CTXS + (VM_MAX_CONTEXTS - 1);
    for (UINT8 i = VM_MAX_CONTEXTS; i != 0; --i) {
        tmp->next = nxt;
        tmp->base_addr = base_addr;
        tmp->stack_ptr = base_addr;
        tmp->ID = i;
        base_addr += VM_CONTEXT_STACK_SIZE;
        nxt = tmp--;
    }
    vm_lock_state = 0;
}

// Initializes the script runner contexts.
void script_runner_init(BOOLEAN reset) BANKED {
    // Setup the sub modules.
    device_init(); // Initialize the device module first.
                   // The following modules' initialization routines should not depend on each other.
    actor_init();
    audio_init();
    effects_init();
    emote_init();
    game_init();
    graphics_init();
    gui_init();
    input_init();
    persistence_init();
    projectile_init();
    scene_init();
    trigger_init();

    // Setup the interrupts.
    CRITICAL {
        TMA_REG = (device_type & DEVICE_TYPE_CGB) ? 0x80 : 0xC0;
        TAC_REG = TACF_START | TACF_16KHZ;
        add_VBL(VBL_isr);
        LYC_REG = 0;
        STAT_REG |= STATF_LYC;
        add_LCD(LCD_isr);
    }
    set_interrupts(DEVICE_ISR_DEFAULT);

    // Setup the font module.
    font_init();

    // Setup the script contexts.
    reset_contexts(reset);

    // Setup the random number generator.
    initrand(DIV_REG);
}

// Executes a script in the new allocated context.
SCRIPT_CTX * script_execute(UINT8 bank, UINT8 * pc, UINT16 * handle, UINT8 nargs, ...) BANKED {
    // Prepare.
    if (!free_ctxs)
        return NULL;
#if defined SAFE_SCRIPT_EXECUTE
    if (!pc)
        return NULL;
#endif /* SAFE_SCRIPT_EXECUTE */

    SCRIPT_CTX * tmp = free_ctxs;

    // Remove the context from the free list.
    free_ctxs = free_ctxs->next;

    // Initialize the context.
    tmp->PC = pc, tmp->bank = bank, tmp->stack_ptr = tmp->base_addr;

    // Set the thread handle by reference.
    tmp->hthread = handle;
    if (handle) *handle = tmp->ID;

    // Clear the termination flag.
    tmp->terminated = FALSE;

    // Clear the lock count.
    tmp->lock_count = 0;

    // Clear the update function.
    tmp->update_fn_bank = 0;

    // Append the context to the active list.
    tmp->next = NULL;
    if (first_ctx) {
        SCRIPT_CTX * idx = first_ctx;
        while (idx->next) idx = idx->next;
        idx->next = tmp;
    } else {
        first_ctx = tmp;
    }

    // Push the thread's local variables.
    if (nargs) {
        va_list va;
        va_start(va, nargs);
        for (UINT8 i = nargs; i != 0; --i) {
            *(tmp->stack_ptr++) = va_arg(va, INT16);
        }
    }

    // Return the thread context.
    return tmp;
}

// Terminates the script by ID; returns non-zero if no such thread is running.
BOOLEAN script_terminate(UINT8 ID, BOOLEAN detach) BANKED {
    SCRIPT_CTX * ctx = first_ctx;
    while (ctx) {
        if (ctx->ID == ID) {
            if (ctx->hthread) {
                *(ctx->hthread) |= SCRIPT_TERMINATED;
                if (detach)
                    ctx->hthread = NULL;
            }
            ctx->terminated = TRUE;

            return TRUE;
        } else {
            ctx = ctx->next;
        }
    }

    return FALSE;
}

// Detaches the specific script from the monitoring handle.
BOOLEAN script_detach_hthread(UINT8 ID) BANKED {
    SCRIPT_CTX * ctx = first_ctx;
    while (ctx) {
        if (ctx->ID == ID) {
            if (ctx->hthread)
                ctx->hthread = NULL;

            return TRUE;
        } else {
            ctx = ctx->next;
        }
    }

    return FALSE;
}

// Processes all the contexts.
UINT8 script_runner_update(void) NONBANKED {
    // Prepare.
    static SCRIPT_CTX * old_ctx, * ctx;
    BOOLEAN waitable = TRUE;
    UINT8 counter = INSTRUCTIONS_PER_QUANT;

    // If locked then execute the last context until it is unlocked or terminated.
    if (!vm_lock_state) old_ctx = NULL, ctx = first_ctx;

    // Iterate all the contexts.
    while (ctx) {
        // Prepare.
#if VM_EXCEPTION_ENABLED
        vm_exception_code = EXCEPTION_CODE_NONE;
#endif /* VM_EXCEPTION_ENABLED */
        ctx->waitable = FALSE;
        if ((ctx->terminated != FALSE) || (!VM_STEP(ctx))) {
            // Update the lock state.
            vm_lock_state -= ctx->lock_count;

            // Update the handle if present.
            if (ctx->hthread) *(ctx->hthread) |= (SCRIPT_TERMINATED | SCRIPT_FINISHED);

            // The script is finished, remove from the linked list.
            if (old_ctx) old_ctx->next = ctx->next;
            if (first_ctx == ctx) first_ctx = ctx->next;

            // Add the terminated context to the free contexts list.
            ctx->next = free_ctxs, free_ctxs = ctx;

            // Next context.
            if (old_ctx) ctx = old_ctx->next; else ctx = first_ctx;
        } else {
            // Check exception.
#if VM_EXCEPTION_ENABLED
            if (vm_exception_code) return RUNNER_EXCEPTION;
#endif /* VM_EXCEPTION_ENABLED */

            // Loop until waitable state or quant is expired.
            if (!(ctx->waitable) && (counter--)) continue;

            // Exit while loop if the context switching is locked.
            if (vm_lock_state) break;

            // Switch to the next context.
            waitable &= ctx->waitable;
            old_ctx = ctx, ctx = ctx->next;
            counter = INSTRUCTIONS_PER_QUANT;
        }
    }

    // Return `RUNNER_DONE` (0) if all the threads are finished.
    if (!first_ctx) return RUNNER_DONE;

    // Return `RUNNER_IDLE` (1) if all the threads are in waitable state, otherwise return `RUNNER_BUSY` (2).
    if (waitable) return RUNNER_IDLE;
    else return RUNNER_BUSY;
}
