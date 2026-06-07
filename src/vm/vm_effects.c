#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "vm_device.h"
#include "vm_effects.h"
#include "vm_graphics.h"
#include "vm_scene.h"

BANKREF(VM_EFFECTS)

#if USE_EFFECTS

static UINT8 effects_pulse_ticks;
static UINT8 effects_pulse_interval;
UINT8 effects_pulse_bank;
POINTER effects_pulse_address;

static const parallax_row_t parallax_rows_defaults[3] = { EFFECTS_PARALLAX_STEP(0, 2, 2), EFFECTS_PARALLAX_STEP(2, 4, 1), EFFECTS_PARALLAX_STEP(4, 0, 0) };
parallax_row_t effects_parallax_rows[3];
parallax_row_t * effects_parallax_row;

static const UINT8 EFFECTS_WOBBLE_OFFSETS[16] = {
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0
};
static const UINT8 * effects_wobble_offset = EFFECTS_WOBBLE_OFFSETS;
UINT8 effects_wobble;

void effects_init(void) BANKED {
    effects_pulse_ticks    = 0;
    effects_pulse_interval = 0;
    effects_pulse_bank     = 0;
    effects_pulse_address  = NULL;

    FEATURE_EFFECT_PARALLAX_DISABLE;
    effects_parallax_row   = effects_parallax_rows;
    memcpy(effects_parallax_rows, parallax_rows_defaults, sizeof(effects_parallax_rows));

    effects_wobble         = 0;

    FEATURE_MAP_MOVEMENT_SET;
    STAT_REG = 0;
}

void effects_pulse_update(void) BANKED {
    if (++effects_pulse_ticks < effects_pulse_interval)
        return;

    effects_pulse_ticks = 0;

    UINT8 * ptr = (UINT8 *)effects_pulse_address;
    const UINT8 m = get_uint8(effects_pulse_bank, ptr++);
    if (FEATURE_EFFECT_PULSE_FLAG == 0) {
        for (UINT8 i = 0; i != m; ++i) {
            const UINT8 t     = get_uint8 (effects_pulse_bank, ptr++);
            const UINT8 n     = get_uint8 (effects_pulse_bank, ptr++);
            const UINT8 bank0 = get_uint8 (effects_pulse_bank, ptr++);
            UINT8 * addr0     = get_ptr_be(effects_pulse_bank, ptr  ); ptr += 2;
            ptr += 3;
            call_v_bbp_oldcall(t, n, bank0, addr0, set_bkg_data);
        }
        FEATURE_EFFECT_PULSE_SET;
    } else {
        for (UINT8 i = 0; i != m; ++i) {
            const UINT8 t     = get_uint8 (effects_pulse_bank, ptr++);
            const UINT8 n     = get_uint8 (effects_pulse_bank, ptr++);
            ptr += 3;
            const UINT8 bank1 = get_uint8 (effects_pulse_bank, ptr++);
            UINT8 * addr1     = get_ptr_be(effects_pulse_bank, ptr  ); ptr += 2;
            call_v_bbp_oldcall(t, n, bank1, addr1, set_bkg_data);
        }
        FEATURE_EFFECT_PULSE_CLEAR;
    }
}

void effects_parallax_sync(void) NAKED NONBANKED {
    if (LYC_REG == 0) {
        const INT16 x = scene_camera_x - graphics_map_x;
        effects_parallax_rows[0].scroll_x = x >> effects_parallax_rows[0].shift;
        effects_parallax_rows[1].scroll_x = x >> effects_parallax_rows[1].shift;
        effects_parallax_rows[2].scroll_x = x;
    }

#if defined __SDCC && defined NINTENDO
__asm
        ld a, (#_scene_camera_y)
        ld hl, #_graphics_map_y
        sub a, (hl)
        ld c, a
        ld a, (#_scene_camera_y + 1)
        ld hl, #_graphics_map_y + 1
        sbc a, (hl)
        ld b, a

        ld hl, #_effects_parallax_row
        ld a, (hl+)
        ld h, (hl)
        ld l, a

        ld a, (hl+)                 ; `scroll_x`.
        ld e, a
        ld a, (hl+)                 ; `next_y`.
        ldh (#_LYC_REG), a
        or a
        jr z, 1$
        ld d, #0
        jr 2$
1$:
        ld d, c                     ; Load the y scroll value.
2$:
        ldh a, (#_STAT_REG)
        bit STATF_B_BUSY, a
        jr nz, 2$

        ld a, e
        ld (#_SCX_REG), a
        ld a, d
        ldh (#_SCY_REG), a

        ldh a, (#_LYC_REG)
        or a
        jr z, 3$

        ld de, #1
        add hl, de                  ; Skip `shift`.

        ld d, h
        ld a, l
        ld hl, #_effects_parallax_row
        ld (hl+), a
        ld (hl), d
        ret
3$:
        ld hl, #_effects_parallax_row
        ld a, #<_effects_parallax_rows
        ld (hl+), a
        ld (hl), #>_effects_parallax_rows
        ret
__endasm;
#else /* __SDCC && NINTENDO */
#   error "Not implemented."
#endif /* __SDCC && NINTENDO */
}

void effects_wobble_sync(void) BANKED {
    const UINT8 step = effects_wobble & 0x0F;
    const UINT8 fx   = (effects_wobble >> 4) & 0x0F;
    const INT16 x    = scene_camera_x - graphics_map_x;
    const UINT8 s    = effects_wobble_offset[LY_REG & 0x07];
    SCX_REG = x + (s << fx);
    if (step != 1) {
        LYC_REG += step;
        if (LYC_REG >= 144)
            LYC_REG -= 144;
    }
}

void effects_wobble_update(void) BANKED {
    effects_wobble_offset = &EFFECTS_WOBBLE_OFFSETS[(game_time >> 2) & 0x07];
}

void vm_fx(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 what = (UINT8)*(--THIS->stack_ptr);
    switch (what) {
    case EFFECTS_PULSE: {
            effects_pulse_ticks       = 0;
            effects_pulse_interval    = (UINT8)*(--THIS->stack_ptr);
            if (effects_pulse_interval) {
                effects_pulse_bank    = THIS->bank;
                effects_pulse_address = (POINTER)THIS->PC;
                const UINT8 n         = get_uint8(THIS->bank, (UINT8 *)THIS->PC++);
                THIS->PC += n * 8;
            } else {
                effects_pulse_bank    = 0;
                effects_pulse_address = NULL;
            }
        }

        break;
    case EFFECTS_PARALLAX: {
            const UINT8 n = (UINT8)*(--THIS->stack_ptr);
            if (n) {
                for (UINT8 i = 0; i < n; ++i) {
                    const UINT8 start = (UINT8)*(--THIS->stack_ptr);
                    const UINT8 end   = (UINT8)*(--THIS->stack_ptr);
                    const INT8 shift  = (INT8)*(--THIS->stack_ptr);
                    EFFECTS_PARALLAX_SET(effects_parallax_rows[i], start, end, shift);
                }
                for (UINT8 i = n; i < 3; ++i) {
                    EFFECTS_PARALLAX_SET(effects_parallax_rows[i], 0, 0, 0);
                }
            }
            if (n && (effects_parallax_rows[0].shift != 0 || effects_parallax_rows[1].shift != 0 || effects_parallax_rows[2].shift != 0)) {
                effects_wobble        = 0;
                LYC_REG = 0;
                STAT_REG |= STATF_LYC;
                FEATURE_EFFECT_PARALLAX_ENABLE;
            } else {
                FEATURE_EFFECT_PARALLAX_DISABLE;
                STAT_REG &= ~(STATF_LYC | STATF_MODE00);
                memcpy(effects_parallax_rows, parallax_rows_defaults, sizeof(effects_parallax_rows));
            }
        }

        break;
    case EFFECTS_WOBBLE: {
            const UINT8 val = (UINT8)*(--THIS->stack_ptr);
            effects_wobble            = val;
            if (val) {
                FEATURE_EFFECT_PARALLAX_DISABLE;
                LYC_REG = 0;
                if ((effects_wobble & 0x0F) == 1)
                    STAT_REG |= STATF_MODE00;
                else
                    STAT_REG |= STATF_LYC;
            } else {
                LYC_REG = 0;
                STAT_REG &= ~(STATF_LYC | STATF_MODE00);
                FEATURE_MAP_MOVEMENT_SET;
            }
        }

        break;
    }
}

#else /* USE_EFFECTS */

void vm_fx(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;

    // Do nothing.
}

#endif /* USE_EFFECTS */
