#ifndef __VM_TRIGGER_H__
#define __VM_TRIGGER_H__

#include "utils/utils.h"

#include "vm.h"

BANKREF_EXTERN(VM_TRIGGER)

#ifndef USE_TRIGGER
#   define USE_TRIGGER 1
#endif /* USE_TRIGGER */

#if USE_TRIGGER

#ifndef TRIGGER_MAX_COUNT
#   define TRIGGER_MAX_COUNT       31
#endif /* TRIGGER_MAX_COUNT */
#define TRIGGER_NONE               0xFF

#define TRIGGER_HAS_NONE           0x00
#define TRIGGER_HAS_ENTER_SCRIPT   0x01
#define TRIGGER_HAS_LEAVE_SCRIPT   0x02

typedef struct trigger_t {
    UINT8 x;
    UINT8 y;
    UINT8 width;
    UINT8 height;
    UINT8 hit_handler_flags;
    UINT8 hit_handler_bank;
    UINT8 * hit_handler_address;
} trigger_t;

extern trigger_t triggers[TRIGGER_MAX_COUNT];
extern UINT8 trigger_count;

void trigger_init(void) BANKED;

void trigger_dim(UINT8 n) BANKED;

// Find a trigger at the specific position.
// Returns the trigger index or `TRIGGER_NONE` if not found.
UINT8 trigger_at_tile(UINT8 tx, UINT8 ty) BANKED;
// Runs script for the trigger at the specific position if this tile was the most
// recently activated trigger tile.
BOOLEAN trigger_activate_at_tile(UINT8 tx, UINT8 ty, BOOLEAN force) BANKED;

// Find a trigger at the specific intersection.
// Returns the trigger index or `TRIGGER_NONE` if not found.
UINT8 trigger_at_intersection(const boundingbox_t * bb, const upoint16_t * offset) BANKED;
// Runs script for the trigger at the specific position if this intersection was the most
// recently activated trigger area or the player has just left the previous one.
BOOLEAN trigger_activate_at_intersection(const boundingbox_t * bb, const upoint16_t * offset, BOOLEAN force) BANKED;

#endif /* USE_TRIGGER */

void vm_def_trigger(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED;
void vm_on_trigger(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED; // Start.

#endif /* __VM_TRIGGER_H__ */
