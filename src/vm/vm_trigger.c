#pragma bank 255

#include <string.h>

#include "vm_trigger.h"

BANKREF(VM_TRIGGER)

#if USE_TRIGGER

trigger_t triggers[TRIGGER_MAX_COUNT];
UINT8 trigger_count = 0;
static UINT8 trigger_last_tile_x;
static UINT8 trigger_last_tile_y;
static UINT8 trigger_last_object;

void trigger_init(void) BANKED {
    memset(triggers, 0, sizeof(triggers));
    trigger_count       = 0;
    trigger_last_tile_x = 0;
    trigger_last_tile_y = 0;
    trigger_last_object = TRIGGER_NONE;
}

void trigger_dim(UINT8 n) BANKED {
    n = MIN(n, TRIGGER_MAX_COUNT);
    for (UINT8 i = n; i < TRIGGER_MAX_COUNT; ++i)
        memset(&triggers[i], 0, sizeof(trigger_t));
    trigger_count = n;
}

UINT8 trigger_at_tile(UINT8 tx, UINT8 ty) BANKED {
    for (UINT8 i = 0; i != trigger_count; ++i) {
        const UINT8 tx_b = triggers[i].x;
        const UINT8 ty_b = triggers[i].y;
        const UINT8 tx_c = tx_b + triggers[i].width - 1;
        const UINT8 ty_c = ty_b + triggers[i].height - 1;

        if (tx + 1 >= tx_b && tx <= tx_c && ty >= ty_b && ty <= ty_c)
            return i;
    }

    return TRIGGER_NONE;
}

BOOLEAN trigger_activate_at_tile(UINT8 tx, UINT8 ty, BOOLEAN force) BANKED {
    if (!force && (tx == trigger_last_tile_x && ty == trigger_last_tile_y))
        return FALSE;

    const UINT8 hit_trigger = trigger_at_tile(tx, ty);
    trigger_last_tile_x = tx;
    trigger_last_tile_y = ty;
    if (hit_trigger != TRIGGER_NONE) {
        if (triggers[hit_trigger].hit_handler_flags & TRIGGER_HAS_ENTER_SCRIPT)
            script_execute(triggers[hit_trigger].hit_handler_bank, triggers[hit_trigger].hit_handler_address, NULL, 2, EVENT_HANDLER_ENTER, hit_trigger);

        return TRUE;
    }

    return FALSE;
}

UINT8 trigger_at_intersection(const boundingbox_t * bb, const upoint16_t * offset) BANKED {
    const UINT8 tile_left   = DIV8(TO_SCREEN(offset->x) + bb->left);
    const UINT8 tile_right  = DIV8(TO_SCREEN(offset->x) + bb->right);
    const UINT8 tile_top    = DIV8(TO_SCREEN(offset->y) + bb->top);
    const UINT8 tile_bottom = DIV8(TO_SCREEN(offset->y) + bb->bottom);
    for (UINT8 i = 0; i != trigger_count; ++i) {
        const UINT8 trigger_left   = triggers[i].x;
        const UINT8 trigger_top    = triggers[i].y;
        const UINT8 trigger_right  = trigger_left + triggers[i].width - 1;
        const UINT8 trigger_bottom = trigger_top + triggers[i].height - 1;
        if (
            (tile_left   <= trigger_right ) &&
            (tile_right  >= trigger_left  ) &&
            (tile_top    <= trigger_bottom) &&
            (tile_bottom >= trigger_top   )
        ) {
            return i;
        }
    }

    return TRIGGER_NONE;
}

BOOLEAN trigger_activate_at_intersection(const boundingbox_t * bb, const upoint16_t * offset, BOOLEAN force) BANKED {
    const UINT8 hit_trigger = trigger_at_intersection(bb, offset);
    if (!force && (hit_trigger == trigger_last_object))
        return FALSE;

    if (trigger_last_object != TRIGGER_NONE && (hit_trigger == TRIGGER_NONE || hit_trigger != trigger_last_object)) {
        BOOLEAN trigger_script_called = FALSE;
        if (triggers[trigger_last_object].hit_handler_flags & TRIGGER_HAS_LEAVE_SCRIPT) { // Leave the previous.
            script_execute(triggers[trigger_last_object].hit_handler_bank, triggers[trigger_last_object].hit_handler_address, NULL, 2, EVENT_HANDLER_LEAVE, trigger_last_object);
            trigger_script_called = TRUE;
        }
        if (hit_trigger != TRIGGER_NONE && triggers[hit_trigger].hit_handler_flags & TRIGGER_HAS_ENTER_SCRIPT) { // Enter the current.
            script_execute(triggers[hit_trigger].hit_handler_bank, triggers[hit_trigger].hit_handler_address, NULL, 2, EVENT_HANDLER_ENTER, hit_trigger);
            trigger_script_called = TRUE;
        }
        trigger_last_object = hit_trigger;

        return trigger_script_called;
    }

    trigger_last_object = hit_trigger;
    if (hit_trigger != TRIGGER_NONE && triggers[hit_trigger].hit_handler_flags & TRIGGER_HAS_ENTER_SCRIPT) { // First enter.
        script_execute(triggers[hit_trigger].hit_handler_bank, triggers[hit_trigger].hit_handler_address, NULL, 2, EVENT_HANDLER_ENTER, hit_trigger);

        return TRUE;
    }

    return FALSE;
}

void vm_def_trigger(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    UINT8 trigger = (UINT8)*(--THIS->stack_ptr);
    UINT8 x;
    UINT8 y;
    UINT8 w;
    UINT8 h;
    UINT8 bank;
    UINT8 * ptr;

    switch (src) {
    case ASSET_SOURCE_STACK:
        x                      = (UINT8)*(--THIS->stack_ptr);
        y                      = (UINT8)*(--THIS->stack_ptr);
        w                      = (UINT8)*(--THIS->stack_ptr);
        h                      = (UINT8)*(--THIS->stack_ptr);

        break;
    case ASSET_SOURCE_READ:
        bank                   = current_data_bank;
        ptr                    = (UINT8 *)current_data_address;
        x                      = get_uint8(bank, ptr++);
        y                      = get_uint8(bank, ptr++);
        w                      = get_uint8(bank, ptr++);
        h                      = get_uint8(bank, ptr++);
        current_data_address  += 4;

        break;
    case ASSET_SOURCE_DATA:
        bank                   = THIS->bank;
        ptr                    = (UINT8 *)THIS->PC;
        x                      = get_uint8(bank, ptr++);
        y                      = get_uint8(bank, ptr++);
        w                      = get_uint8(bank, ptr++);
        h                      = get_uint8(bank, ptr++);
        THIS->PC              += 4;

        break;
    default:
        x                      = 0;
        y                      = 0;
        w                      = 0;
        h                      = 0;

        break;
    }
    triggers[trigger].x        = x;
    triggers[trigger].y        = y;
    triggers[trigger].width    = w;
    triggers[trigger].height   = h;
}

void vm_on_trigger(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED {
    UINT8 trigger = (UINT8)*(--THIS->stack_ptr);
    UINT8 flags   = (UINT8)*(--THIS->stack_ptr);
    if (bank) {
        triggers[trigger].hit_handler_flags     = flags;
        triggers[trigger].hit_handler_bank      = bank;
        triggers[trigger].hit_handler_address   = pc;
    } else {
        triggers[trigger].hit_handler_flags    &= ~flags;
        triggers[trigger].hit_handler_bank      = 0;
        triggers[trigger].hit_handler_address   = NULL;
    }
}

#else /* USE_TRIGGER */

void vm_def_trigger(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    (void)THIS;
    (void)src;

    // Do nothing.
}

void vm_on_trigger(SCRIPT_CTX * THIS, UINT8 bank, UINT8 * pc) OLDCALL BANKED {
    (void)THIS;
    (void)bank;
    (void)pc;

    // Do nothing.
}

#endif /* USE_TRIGGER */
