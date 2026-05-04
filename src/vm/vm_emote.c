#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_emote.h"

BANKREF(VM_EMOTE)

#if USE_EMOTE

#define EMOTE_SPRITE_TILE_COUNT    4
#define EMOTE_TOTAL_FRAME_COUNT    60
#define EMOTE_BOUNCE_FRAME_COUNT   15

// Motion offsets.
const INT8 EMOTE_OFFSETS[] = {
     2,  1,  0,
    -1, -2, -3, -4, -5, -6,
    -5, -4, -3, -2, -1,  0,
};

// Frames for palette 0.
const metasprite_t EMOTE_8x16_FRAME0[] = {
    METASPR_ITEM(0, 0, 0, 0x00), METASPR_ITEM(0, 8, 2, 0x00),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE0[] = {
    EMOTE_8x16_FRAME0,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME0[] = {
    METASPR_ITEM(0, 0, 0, 0x00), METASPR_ITEM(0, 8, 0, 0x20),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE0[] = {
    EMOTE_8x16_MIRRORED_FRAME0,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME0[] = {
    METASPR_ITEM(0, 0, 0, 0x00), METASPR_ITEM(0, 8, 2, 0x00),
    METASPR_ITEM(8, -8, 1, 0x00), METASPR_ITEM(0, 8, 3, 0x00),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE0[] = {
    EMOTE_8x8_FRAME0,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME0[] = {
    METASPR_ITEM(0, 0, 0, 0x00), METASPR_ITEM(0, 8, 0, 0x20),
    METASPR_ITEM(8, -8, 1, 0x00), METASPR_ITEM(0, 8, 1, 0x20),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE0[] = {
    EMOTE_8x8_MIRRORED_FRAME0,
    NULL
};

// Frames for palette 1.
const metasprite_t EMOTE_8x16_FRAME1[] = {
    METASPR_ITEM(0, 0, 0, 0x01), METASPR_ITEM(0, 8, 2, 0x01),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE1[] = {
    EMOTE_8x16_FRAME1,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME1[] = {
    METASPR_ITEM(0, 0, 0, 0x01), METASPR_ITEM(0, 8, 0, 0x21),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE1[] = {
    EMOTE_8x16_MIRRORED_FRAME1,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME1[] = {
    METASPR_ITEM(0, 0, 0, 0x01), METASPR_ITEM(0, 8, 2, 0x01),
    METASPR_ITEM(8, -8, 1, 0x01), METASPR_ITEM(0, 8, 3, 0x01),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE1[] = {
    EMOTE_8x8_FRAME1,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME1[] = {
    METASPR_ITEM(0, 0, 0, 0x01), METASPR_ITEM(0, 8, 0, 0x21),
    METASPR_ITEM(8, -8, 1, 0x01), METASPR_ITEM(0, 8, 1, 0x21),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE1[] = {
    EMOTE_8x8_MIRRORED_FRAME1,
    NULL
};

// Frames for palette 2.
const metasprite_t EMOTE_8x16_FRAME2[] = {
    METASPR_ITEM(0, 0, 0, 0x02), METASPR_ITEM(0, 8, 2, 0x02),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE2[] = {
    EMOTE_8x16_FRAME2,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME2[] = {
    METASPR_ITEM(0, 0, 0, 0x02), METASPR_ITEM(0, 8, 0, 0x22),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE2[] = {
    EMOTE_8x16_MIRRORED_FRAME2,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME2[] = {
    METASPR_ITEM(0, 0, 0, 0x02), METASPR_ITEM(0, 8, 2, 0x02),
    METASPR_ITEM(8, -8, 1, 0x02), METASPR_ITEM(0, 8, 3, 0x02),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE2[] = {
    EMOTE_8x8_FRAME2,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME2[] = {
    METASPR_ITEM(0, 0, 0, 0x02), METASPR_ITEM(0, 8, 0, 0x22),
    METASPR_ITEM(8, -8, 1, 0x02), METASPR_ITEM(0, 8, 1, 0x22),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE2[] = {
    EMOTE_8x8_MIRRORED_FRAME2,
    NULL
};

// Frames for palette 3.
const metasprite_t EMOTE_8x16_FRAME3[] = {
    METASPR_ITEM(0, 0, 0, 0x03), METASPR_ITEM(0, 8, 2, 0x03),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE3[] = {
    EMOTE_8x16_FRAME3,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME3[] = {
    METASPR_ITEM(0, 0, 0, 0x03), METASPR_ITEM(0, 8, 0, 0x23),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE3[] = {
    EMOTE_8x16_MIRRORED_FRAME3,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME3[] = {
    METASPR_ITEM(0, 0, 0, 0x03), METASPR_ITEM(0, 8, 2, 0x03),
    METASPR_ITEM(8, -8, 1, 0x03), METASPR_ITEM(0, 8, 3, 0x03),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE3[] = {
    EMOTE_8x8_FRAME3,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME3[] = {
    METASPR_ITEM(0, 0, 0, 0x03), METASPR_ITEM(0, 8, 0, 0x23),
    METASPR_ITEM(8, -8, 1, 0x03), METASPR_ITEM(0, 8, 1, 0x23),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE3[] = {
    EMOTE_8x8_MIRRORED_FRAME3,
    NULL
};

// Frames for palette 4.
const metasprite_t EMOTE_8x16_FRAME4[] = {
    METASPR_ITEM(0, 0, 0, 0x04), METASPR_ITEM(0, 8, 2, 0x04),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE4[] = {
    EMOTE_8x16_FRAME4,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME4[] = {
    METASPR_ITEM(0, 0, 0, 0x04), METASPR_ITEM(0, 8, 0, 0x24),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE4[] = {
    EMOTE_8x16_MIRRORED_FRAME4,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME4[] = {
    METASPR_ITEM(0, 0, 0, 0x04), METASPR_ITEM(0, 8, 2, 0x04),
    METASPR_ITEM(8, -8, 1, 0x04), METASPR_ITEM(0, 8, 3, 0x04),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE4[] = {
    EMOTE_8x8_FRAME4,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME4[] = {
    METASPR_ITEM(0, 0, 0, 0x04), METASPR_ITEM(0, 8, 0, 0x24),
    METASPR_ITEM(8, -8, 1, 0x04), METASPR_ITEM(0, 8, 1, 0x24),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE4[] = {
    EMOTE_8x8_MIRRORED_FRAME4,
    NULL
};

// Frames for palette 5.
const metasprite_t EMOTE_8x16_FRAME5[] = {
    METASPR_ITEM(0, 0, 0, 0x05), METASPR_ITEM(0, 8, 2, 0x05),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE5[] = {
    EMOTE_8x16_FRAME5,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME5[] = {
    METASPR_ITEM(0, 0, 0, 0x05), METASPR_ITEM(0, 8, 0, 0x25),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE5[] = {
    EMOTE_8x16_MIRRORED_FRAME5,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME5[] = {
    METASPR_ITEM(0, 0, 0, 0x05), METASPR_ITEM(0, 8, 2, 0x05),
    METASPR_ITEM(8, -8, 1, 0x05), METASPR_ITEM(0, 8, 3, 0x05),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE5[] = {
    EMOTE_8x8_FRAME5,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME5[] = {
    METASPR_ITEM(0, 0, 0, 0x05), METASPR_ITEM(0, 8, 0, 0x25),
    METASPR_ITEM(8, -8, 1, 0x05), METASPR_ITEM(0, 8, 1, 0x25),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE5[] = {
    EMOTE_8x8_MIRRORED_FRAME5,
    NULL
};

// Frames for palette 6.
const metasprite_t EMOTE_8x16_FRAME6[] = {
    METASPR_ITEM(0, 0, 0, 0x06), METASPR_ITEM(0, 8, 2, 0x06),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE6[] = {
    EMOTE_8x16_FRAME6,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME6[] = {
    METASPR_ITEM(0, 0, 0, 0x06), METASPR_ITEM(0, 8, 0, 0x26),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE6[] = {
    EMOTE_8x16_MIRRORED_FRAME6,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME6[] = {
    METASPR_ITEM(0, 0, 0, 0x06), METASPR_ITEM(0, 8, 2, 0x06),
    METASPR_ITEM(8, -8, 1, 0x06), METASPR_ITEM(0, 8, 3, 0x06),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE6[] = {
    EMOTE_8x8_FRAME6,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME6[] = {
    METASPR_ITEM(0, 0, 0, 0x06), METASPR_ITEM(0, 8, 0, 0x26),
    METASPR_ITEM(8, -8, 1, 0x06), METASPR_ITEM(0, 8, 1, 0x26),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE6[] = {
    EMOTE_8x8_MIRRORED_FRAME6,
    NULL
};

// Frames for palette 7.
const metasprite_t EMOTE_8x16_FRAME7[] = {
    METASPR_ITEM(0, 0, 0, 0x07), METASPR_ITEM(0, 8, 2, 0x07),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_METASPRITE7[] = {
    EMOTE_8x16_FRAME7,
    NULL
};
const metasprite_t EMOTE_8x16_MIRRORED_FRAME7[] = {
    METASPR_ITEM(0, 0, 0, 0x07), METASPR_ITEM(0, 8, 0, 0x27),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x16_MIRRORED_METASPRITE7[] = {
    EMOTE_8x16_MIRRORED_FRAME7,
    NULL
};

const metasprite_t EMOTE_8x8_FRAME7[] = {
    METASPR_ITEM(0, 0, 0, 0x07), METASPR_ITEM(0, 8, 2, 0x07),
    METASPR_ITEM(8, -8, 1, 0x07), METASPR_ITEM(0, 8, 3, 0x07),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_METASPRITE7[] = {
    EMOTE_8x8_FRAME7,
    NULL
};
const metasprite_t EMOTE_8x8_MIRRORED_FRAME7[] = {
    METASPR_ITEM(0, 0, 0, 0x07), METASPR_ITEM(0, 8, 0, 0x27),
    METASPR_ITEM(8, -8, 1, 0x07), METASPR_ITEM(0, 8, 1, 0x27),
    METASPR_TERM
};
const metasprite_t * const EMOTE_8x8_MIRRORED_METASPRITE7[] = {
    EMOTE_8x8_MIRRORED_FRAME7,
    NULL
};

// Metasprites for all palettes.
const metasprite_t ** const EMOTE_8x16_METASPRITES[] = {
    EMOTE_8x16_METASPRITE0,
    EMOTE_8x16_METASPRITE1,
    EMOTE_8x16_METASPRITE2,
    EMOTE_8x16_METASPRITE3,
    EMOTE_8x16_METASPRITE4,
    EMOTE_8x16_METASPRITE5,
    EMOTE_8x16_METASPRITE6,
    EMOTE_8x16_METASPRITE7
};
const metasprite_t ** const EMOTE_8x16_MIRRORED_METASPRITES[] = {
    EMOTE_8x16_MIRRORED_METASPRITE0,
    EMOTE_8x16_MIRRORED_METASPRITE1,
    EMOTE_8x16_MIRRORED_METASPRITE2,
    EMOTE_8x16_MIRRORED_METASPRITE3,
    EMOTE_8x16_MIRRORED_METASPRITE4,
    EMOTE_8x16_MIRRORED_METASPRITE5,
    EMOTE_8x16_MIRRORED_METASPRITE6,
    EMOTE_8x16_MIRRORED_METASPRITE7
};

const metasprite_t ** const EMOTE_8x8_METASPRITES[] = {
    EMOTE_8x8_METASPRITE0,
    EMOTE_8x8_METASPRITE1,
    EMOTE_8x8_METASPRITE2,
    EMOTE_8x8_METASPRITE3,
    EMOTE_8x8_METASPRITE4,
    EMOTE_8x8_METASPRITE5,
    EMOTE_8x8_METASPRITE6,
    EMOTE_8x8_METASPRITE7
};
const metasprite_t ** const EMOTE_8x8_MIRRORED_METASPRITES[] = {
    EMOTE_8x8_MIRRORED_METASPRITE0,
    EMOTE_8x8_MIRRORED_METASPRITE1,
    EMOTE_8x8_MIRRORED_METASPRITE2,
    EMOTE_8x8_MIRRORED_METASPRITE3,
    EMOTE_8x8_MIRRORED_METASPRITE4,
    EMOTE_8x8_MIRRORED_METASPRITE5,
    EMOTE_8x8_MIRRORED_METASPRITE6,
    EMOTE_8x8_MIRRORED_METASPRITE7
};

// The emote actor and timer.
actor_t * emote_actor;
UINT8 emote_timer;

void emote_init(void) BANKED {
    emote_actor = NULL;
    emote_timer = 0;
}

void emote_update(void) BANKED {
    if (emote_timer < EMOTE_BOUNCE_FRAME_COUNT) {
        UINT16 y = emote_actor->absolute_movement.y;
        y += EMOTE_OFFSETS[emote_timer];
        emote_actor->position.y = FROM_SCREEN(y);
    }
}

STATIC UINT8 emote_def(UINT16 x, UINT16 y, UINT8 base_tile, BOOLEAN mirrored, UINT8 pal, actor_t * ref_actor, UINT8 bank, UINT8 * ptr) {
    const UINT8 n = mirrored ? (EMOTE_SPRITE_TILE_COUNT / 2) : EMOTE_SPRITE_TILE_COUNT;
    call_v_bbp_oldcall(base_tile, n, bank, ptr, set_sprite_data);

    if (ref_actor) {
        x += TO_SCREEN(ref_actor->position.x);
        y += TO_SCREEN(ref_actor->position.y);
    }
    emote_actor->position.x          = FROM_SCREEN(x);
    emote_actor->position.y          = FROM_SCREEN(y);
    emote_actor->absolute_movement.x = x; // Reused to reserve the position, in screen space;
    emote_actor->absolute_movement.y = y; // same for `y`, these fields are constant during this emote's raising.
    emote_actor->base_tile           = base_tile;
    emote_actor->sprite_bank         = BANK(VM_EMOTE);
    if (LCDC_REG & LCDCF_OBJ16) {
        emote_actor->sprite_frames   = mirrored ?
            (metasprite_t **)EMOTE_8x16_MIRRORED_METASPRITES[pal] :
            (metasprite_t **)EMOTE_8x16_METASPRITES[pal];
    } else {
        emote_actor->sprite_frames   = mirrored ?
            (metasprite_t **)EMOTE_8x8_MIRRORED_METASPRITES[pal] :
            (metasprite_t **)EMOTE_8x8_METASPRITES[pal];
    }
    emote_actor->animation_interval  = ACTOR_ANIMATION_PAUSED;

    return MUL16(n);
}

STATIC void emote_load(SCRIPT_CTX * THIS, UINT8 src) {
    const UINT16 x         = (UINT16)*(--THIS->stack_ptr);
    const UINT16 y         = (UINT16)*(--THIS->stack_ptr);
    const UINT16 base_tile = (UINT16)*(--THIS->stack_ptr);
    const BOOLEAN mirrored = (BOOLEAN)*(--THIS->stack_ptr);
    const UINT8 pal        = (UINT8)*(--THIS->stack_ptr);
    actor_t * ref_actor    = (actor_t *)*(--THIS->stack_ptr);
    UINT8 bank;
    UINT8 * ptr;
    UINT8 size = 0;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        size = emote_def(x, y, base_tile, mirrored, pal, ref_actor, bank, ptr);
        current_data_address += size;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        emote_def(x, y, base_tile, mirrored, pal, ref_actor, bank, ptr);
        THIS->PC += MUL16(EMOTE_SPRITE_TILE_COUNT);

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        emote_def(x, y, base_tile, mirrored, pal, ref_actor, bank, ptr);

        break;
    default:
        // Do nothing.

        break;
    }
}

void vm_emote(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    if (emote_actor == NULL) {
        emote_actor = actor_new();
        if (!emote_actor) {
            THIS->stack_ptr -= 5; // Local parameters.

            return;
        }
        emote_timer = 0;

        emote_load(THIS, src);
        actor_activate(emote_actor);
    }

    if (emote_timer == EMOTE_TOTAL_FRAME_COUNT) {
        actor_delete(emote_actor);
        emote_actor = NULL;
        if (src == ASSET_SOURCE_DATA)
            THIS->PC += MUL16(EMOTE_SPRITE_TILE_COUNT); // Inline data.
        else if (src == ASSET_SOURCE_FAR)
            THIS->PC += 3; // Far pointer.
    } else {
        THIS->waitable = TRUE;
        THIS->PC -= INSTRUCTION_SIZE + sizeof(src); // Wait.
        if (emote_timer == 0) { // Already loaded.
            if (src == ASSET_SOURCE_DATA)
                THIS->PC -= MUL16(EMOTE_SPRITE_TILE_COUNT); // Inline data.
            else if (src == ASSET_SOURCE_FAR)
                THIS->PC -= 3; // Far pointer.
        }
        ++emote_timer;
    }
}

#else /* USE_EMOTE */

void vm_emote(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    (void)THIS;
    (void)src;

    // Do nothing.
}

#endif /* USE_EMOTE */
