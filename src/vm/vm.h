#ifndef __VM_H__
#define __VM_H__

#if defined __SDCC
#   include <gbdk/far_ptr.h>
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

BANKREF_EXTERN(VM_MAIN)

// Types.
#ifndef FASTUINT8
#   if defined __SDCC && defined NINTENDO
#       define FASTUINT8 SFR
#   else
#       define FASTUINT8 UINT8
#   endif
#endif /* FASTUINT8 */

// Memory bus addresses.
#ifndef ROM_ADDRESS
#   define ROM_ADDRESS                   0x0000
#endif /* ROM_ADDRESS */
#ifndef BROM_ADDRESS
#   define BROM_ADDRESS                  0x4000
#endif /* BROM_ADDRESS */
#ifndef VRAM_ADDRESS
#   define VRAM_ADDRESS                  0x8000
#endif /* VRAM_ADDRESS */
#ifndef VRAM_BEGIN
#   define VRAM_BEGIN                    0x8000
#endif /* VRAM_BEGIN */
#ifndef VRAM_END
#   define VRAM_END                      0x9FFF
#endif /* VRAM_END */
#ifndef SRAM_ADDRESS
#   define SRAM_ADDRESS                  0xA000
#endif /* SRAM_ADDRESS */
#ifndef RAM_ADDRESS
#   define RAM_ADDRESS                   0xC000
#endif /* RAM_ADDRESS */
#ifndef BRAM_ADDRESS
#   define BRAM_ADDRESS                  0xD000
#endif /* BRAM_ADDRESS */
#ifndef OAMRAM_ADDRESS
#   define OAMRAM_ADDRESS                0xFE00
#endif /* OAMRAM_ADDRESS */

// Invoking conventions.
#if defined NINTENDO
#   define STEP_FUNC_ATTR
    typedef UINT16 DUMMY0_t;
    typedef UINT16 DUMMY1_t;
#elif defined SEGA
#   define STEP_FUNC_ATTR Z88DK_FASTCALL
    typedef UINT8 DUMMY0_t;
    typedef UINT16 DUMMY1_t;
#endif /* Platform macros. */

typedef POINTER SCRIPT_CMD_FN;
typedef struct SCRIPT_CMD {
    SCRIPT_CMD_FN fn;
    UINT8 fn_bank;
    UINT8 args_len;
} SCRIPT_CMD;
typedef BOOLEAN (* SCRIPT_UPDATE_FN)(POINTER /* THIS */, UINT8 /* start */, UINT16 * /* stack_frame */) OLDCALL BANKED;

#define FAR_CALL_EX(ADDR, SEG, Y, ...) \
    (__call_banked_addr = (ADDR), __call_banked_bank = (SEG), ((Y)(&__call__banked))(__VA_ARGS__))

// Memory operations.
#define VM_REF_TO_PTR(IDX) \
    (POINTER)(((IDX) < 0) ? THIS->stack_ptr + (IDX) : script_memory + (IDX))
#define VM_STK_TO_PTR(IDX) \
    (POINTER)(((IDX) < 0) ? THIS->stack_ptr + (IDX) : THIS->base_addr + (IDX))

// Script context.
typedef struct SCRIPT_CTX {
    // Program pointer.
    const UINT8 * PC;
    UINT8 bank;
    // Linked list of contexts for cooperative multitasking.
    struct SCRIPT_CTX * next;
    // VM stack pointer.
    UINT16 * stack_ptr;
    UINT16 * base_addr;
    // Thread control.
    UINT8 ID;
    UINT16 * hthread;
    BOOLEAN terminated;
    // Waitable state.
    BOOLEAN waitable;
    // Lock state.
    UINT8 lock_count;
    // Update function.
    POINTER update_fn;
    UINT8 update_fn_bank;
} SCRIPT_CTX;

// Total stack size of all VM threads.
#ifndef VM_TOTAL_CONTEXT_STACK_SIZE
#   define VM_TOTAL_CONTEXT_STACK_SIZE   1024
#endif /* VM_TOTAL_CONTEXT_STACK_SIZE */
// Maximum number of concurrent running VM threads.
#ifndef VM_MAX_CONTEXTS
#   define VM_MAX_CONTEXTS               16
#endif /* VM_MAX_CONTEXTS */
// Stack size of each VM thread.
#define VM_CONTEXT_STACK_SIZE            (VM_TOTAL_CONTEXT_STACK_SIZE / VM_MAX_CONTEXTS)
// Number of shared variables.
#ifndef VM_HEAP_SIZE
#   define VM_HEAP_SIZE                  1024
#endif /* VM_HEAP_SIZE */

// The shared context memory.
extern UINT16 script_memory[VM_HEAP_SIZE + (VM_MAX_CONTEXTS * VM_CONTEXT_STACK_SIZE)];

// The contexts for executing scripts. The `script_runner_init`,
// `script_execute`, `script_runner_update` functions manipulate these
// contexts.
extern SCRIPT_CTX CTXS[VM_MAX_CONTEXTS];
extern SCRIPT_CTX * first_ctx, * free_ctxs;

// Cursor of the data reading stream.
extern UINT8 current_data_bank;
extern UINT16 current_data_address;

// The lock state.
extern UINT8 vm_lock_state;
// The exception flag and parameters.
#define VM_EXCEPTION_ENABLED             1
#if VM_EXCEPTION_ENABLED
extern UINT8 vm_exception_code;   // Exception type.
extern UINT8 vm_exception_source; // Exception source or parameters bank.
extern UINT16 vm_exception_data;  // Exception data or parameters address.
#   define VM_THROW(CODE, SRC, DATA) \
        do { \
            vm_exception_code   = (CODE); \
            vm_exception_source = (SRC); \
            vm_exception_data   = (DATA); \
        } while (0)
#endif /* VM_EXCEPTION_ENABLED */

// Operators.
#define VM_OP_R_INT8                    32
#define VM_OP_R_INT16                   33
#define VM_OP_R_REF                     34
#define VM_OP_EQ                         1
#define VM_OP_LT                         2
#define VM_OP_LE                         3
#define VM_OP_GT                         4
#define VM_OP_GE                         5
#define VM_OP_NE                         6
#define VM_OP_AND                        7
#define VM_OP_OR                         8
#define VM_OP_NOT                        9
#define VM_OP_ADD                       10
#define VM_OP_SUB                       11
#define VM_OP_MUL                       12
#define VM_OP_DIV                       13
#define VM_OP_MOD                       14
#define VM_OP_BAND                      15
#define VM_OP_BOR                       16
#define VM_OP_BXOR                      17
#define VM_OP_SHL                       18
#define VM_OP_SHR                       19
#define VM_OP_BNOT                      20
#define VM_OP_NEG                       21
#define VM_OP_SGN                       22
#define VM_OP_ABS                       23
#define VM_OP_SQR                       24
#define VM_OP_SQRT                      25
#define VM_OP_SIN                       26
#define VM_OP_COS                       27
#define VM_OP_ATAN2                     28
#define VM_OP_POW                       29
#define VM_OP_MIN                       30
#define VM_OP_MAX                       31

// The core instruction functions.
void vm_nop(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_reserve(SCRIPT_CTX * THIS, INT8 ofs) OLDCALL BANKED;
void vm_push(SCRIPT_CTX * THIS, UINT16 value) OLDCALL BANKED;
void vm_push_value(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED;
UINT16 vm_pop(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED;
UINT16 vm_pop_1(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED;
void vm_call(SCRIPT_CTX * THIS, UINT8 * pc) OLDCALL BANKED;
void vm_ret(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED;
void vm_call_far(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED;
void vm_ret_far(SCRIPT_CTX * THIS, UINT8 n) OLDCALL BANKED;
void vm_jump(SCRIPT_CTX * THIS, UINT8 * pc) OLDCALL BANKED;
void vm_jump_far(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED;
void vm_next_bank(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_switch(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, INT16 idx, UINT8 size, UINT8 n, BOOLEAN call) OLDCALL NONBANKED;
void vm_if(SCRIPT_CTX * THIS, UINT8 condition, INT16 idxA, INT16 idxB, UINT8 * pc, UINT8 n) OLDCALL BANKED;
void vm_if_const(SCRIPT_CTX * THIS, UINT8 condition, INT16 idxA, INT16 B, UINT8 * pc, UINT8 n) OLDCALL BANKED;
void vm_iif(SCRIPT_CTX * THIS, INT16 idxR, INT16 idxC, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_loop(SCRIPT_CTX * THIS, INT16 idxA, INT16 B, INT16 C, UINT8 * pc, UINT8 n) OLDCALL BANKED;
void vm_for(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, INT16 idxC, UINT8 * pc, UINT8 n) OLDCALL BANKED;
void vm_set(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_set_const(SCRIPT_CTX * THIS, INT16 idx, INT16 value) OLDCALL BANKED;
void vm_set_indirect(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_get_indirect(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_acc(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, UINT8 n) OLDCALL BANKED;
void vm_acc_const(SCRIPT_CTX * THIS, INT16 idxA, INT16 value) OLDCALL BANKED;
void vm_rpn(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS) OLDCALL NONBANKED;
void vm_get_tlocal(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_set_tlocal(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_pack(SCRIPT_CTX * THIS, INT16 idx, INT16 idx0, INT16 idx1, INT16 idx2, INT16 idx3, BOOLEAN n) OLDCALL BANKED;
void vm_unpack(SCRIPT_CTX * THIS, INT16 idx, INT16 idx0, INT16 idx1, INT16 idx2, INT16 idx3, BOOLEAN n) OLDCALL BANKED;
void vm_swap(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_data_ptr(SCRIPT_CTX * THIS, INT16 idx, BOOLEAN for_bank) OLDCALL BANKED;
void vm_read(SCRIPT_CTX * THIS, INT16 idx, UINT16 size, BOOLEAN copy) OLDCALL BANKED;
void vm_restore(SCRIPT_CTX * THIS, UINT8 bank, INT16 idx) OLDCALL BANKED;
void vm_begin_thread(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc, BOOLEAN put, INT16 idx, UINT8 nargs) OLDCALL BANKED;
void vm_join(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED;
void vm_terminate(SCRIPT_CTX * THIS, INT16 idx, BOOLEAN all) OLDCALL BANKED;
void vm_wait(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_wait_n(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED;
void vm_lock(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_unlock(SCRIPT_CTX * THIS) OLDCALL BANKED;
void vm_invoke_fn(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * fn, UINT8 nparams, INT16 idx) OLDCALL BANKED;
void vm_asm(DUMMY0_t dummy0, DUMMY1_t dummy1, SCRIPT_CTX * THIS, UINT8 bank, UINT8 * fn, UINT16 size) OLDCALL NONBANKED NAKED;
void vm_locate(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB) OLDCALL BANKED;
void vm_print(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 nargs) OLDCALL BANKED;
void vm_srand(SCRIPT_CTX * THIS, INT16 idx) OLDCALL BANKED;
void vm_rand(SCRIPT_CTX * THIS, INT16 idx, INT16 idxMin, INT16 idxMax) OLDCALL BANKED;
void vm_peek(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, BOOLEAN word) OLDCALL BANKED;
void vm_poke(SCRIPT_CTX * THIS, INT16 idxA, INT16 idxB, BOOLEAN word) OLDCALL BANKED;
void vm_fill(SCRIPT_CTX * THIS, INT16 idx, INT16 value, INT16 count) OLDCALL BANKED;

// Instruction size.
#define INSTRUCTION_SIZE                 1
// Quant size.
#define INSTRUCTIONS_PER_QUANT           0x10
// Termination flag.
#define SCRIPT_TERMINATED                0x8000
#define SCRIPT_FINISHED                  0x4000

// Checks whether the current thread is the main.
#define VM_IS_MAIN(SELF)                 ((SELF)->ID == 1)
// Checks whether the thread of the specific ID is terminated.
#define VM_IS_TERMINATED(ID)             ((ID) >> 8)
// Checks whether the thread of the specific ID is finished.
#define VM_IS_FINISHED(ID)               ((ID) & SCRIPT_FINISHED)
// Checks whether the VM is in locked state.
#define VM_IS_LOCKED                     (vm_lock_state != 0)

// Returns zero if the script ends. The bank with the VM code must be active.
BOOLEAN VM_STEP(SCRIPT_CTX * CTX) NAKED NONBANKED STEP_FUNC_ATTR;

// Initializes the script runner contexts.
void script_runner_init(void) BANKED;
// Executes a script in the new allocated context.
SCRIPT_CTX * script_execute(UINT8 bank, UINT8 * pc, UINT16 * handle, UINT8 nargs, ...) BANKED;
// Terminates a script by ID; returns non-zero if no such thread is running.
BOOLEAN script_terminate(UINT8 ID, BOOLEAN detach) BANKED;
// Detaches the specific script from the monitoring handle.
BOOLEAN script_detach_hthread(UINT8 ID) BANKED;

// Running states.
#define RUNNER_DONE                      0
#define RUNNER_IDLE                      1
#define RUNNER_BUSY                      2
#if VM_EXCEPTION_ENABLED
#define RUNNER_EXCEPTION                 3
#endif /* VM_EXCEPTION_ENABLED */

#define EXCEPTION_CODE_NONE              0

// Processes all the contexts.
UINT8 script_runner_update(void) NONBANKED;

#endif /* __VM_H__ */
