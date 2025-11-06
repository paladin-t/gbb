#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "utils/font.h"
#include "utils/graphics.h"
#include "utils/rtc.h"
#include "utils/sgb.h"

#include "vm_actor.h"
#include "vm_audio.h"
#include "vm_device.h"
#include "vm_graphics.h"
#include "vm_gui.h"
#include "vm_persistence.h"
#include "vm_projectile.h"
#include "vm_scene.h"
#include "vm_serial.h"
#include "vm_trigger.h"

BANKREF(VM_DEVICE)

BANKREF_EXTERN(BUILTIN)

/**< Common. */

extern const unsigned char Font[]; // From builtin inline data.

const volatile BOOLEAN ExtensionMode = TRUE; // Can be overwritten by compiler.

const UINT16 BackgroundPalettes[32] = { // Can be overwritten by compiler.
    0x6BFC, 0x3B11, 0x29A6, 0x1061,
    0x2A80, 0x68E5, 0x128D, 0xFBFB,
    0x93B3, 0xF8A9, 0x8B41, 0xFC76,
    0x4004, 0x3F7C, 0x2620, 0xECCF,
    0x0068, 0x7ECF, 0x40D2, 0x48F6,
    0x1C07, 0x075F, 0x1AA8, 0x6CDE,
    0x040E, 0xDED9, 0xC068, 0x987C,
    0x0055, 0xE51E, 0x0E0E, 0x99F7
};
const UINT16 SpritePalettes[32] = { // Can be overwritten by compiler.
    0xFBFB, 0x3F7C, 0x93B3, 0x2620,
    0x2A80, 0x68E5, 0x128D, 0xFBFB,
    0x93B3, 0xF8A9, 0x8B41, 0xFC76,
    0x4004, 0x3F7C, 0x2620, 0xECCF,
    0x0068, 0x7ECF, 0x40D2, 0x48F6,
    0x1C07, 0x075F, 0x1AA8, 0x6CDE,
    0x040E, 0xDED9, 0xC068, 0x987C,
    0x0055, 0xE51E, 0x0E0E, 0x99F7
};

// Can be overwritten by compiler.
const volatile UINT8 SgbBorderPaletteBank = 0;
const volatile UINT16 SgbBorderPaletteAddress = 0;
const volatile UINT16 SgbBorderPaletteSize = 0;
const volatile UINT8 SgbBorderTilesBank = 0;
const volatile UINT16 SgbBorderTilesAddress = 0;
const volatile UINT16 SgbBorderTilesSize = 0;
const volatile UINT8 SgbBorderMapBank = 0;
const volatile UINT16 SgbBorderMapAddress = 0;
const volatile UINT16 SgbBorderMapSize = 0;

// Can be overwritten by compiler.
const volatile UINT8 SgbPalettesBank = 0;
const volatile UINT16 SgbPalettesAddress = 0;

UINT8 device_type;

UINT8 device_object_sprite_base;

void device_init(void) BANKED {
    for (UINT8 i = 4; i != 0; --i) vsync(); // This delay is required for PAL SNES.
    const BOOLEAN IS_SGB = sgb_check();
    const BOOLEAN IS_CGB = ((!IS_SGB) && (_cpu == CGB_TYPE) && (*(UINT8 *)DEVICE_TYPE_ADDRESS & 0x80)); // Both CGB + SGB modes at once are not supported.
    const BOOLEAN IS_GBA = (_is_GBA && IS_CGB); // The GBA features are only available together with CGB.
    BOOLEAN IS_GBB       = FALSE;

    if (ExtensionMode) {
        const UINT8 ext  = *(UINT8 *)EXTENSION_STATUS_REG;
        IS_GBB           = ext == GB1_TYPE || ext == GB2_TYPE;
    }

    device_type |= DEVICE_TYPE_GB;
    if (IS_CGB) device_type |= DEVICE_TYPE_CGB;
    if (IS_SGB) device_type |= DEVICE_TYPE_SGB;
    if (IS_GBA) device_type |= DEVICE_TYPE_AGB;
    if (IS_GBB) device_type |= DEVICE_TYPE_GBB;

    if (IS_CGB) {
        cpu_fast();
        for (UINT8 i = 0; i != COUNTOF(BackgroundPalettes); ++i) {
            const UINT8 p = DIV4(i);
            const UINT8 e = MOD4(i);
            set_bkg_palette_entry(p, e, BackgroundPalettes[i]);
            set_sprite_palette_entry(p, e, SpritePalettes[i]);
        }
    }

    if (IS_SGB) {
        if (SgbBorderPaletteBank != 0 /* && SgbBorderTilesBank != 0 && SgbBorderMapBank != 0 */) {
            sgb_set_border(
                SgbBorderPaletteBank, (UINT8 *)SgbBorderPaletteAddress, SgbBorderPaletteSize,
                SgbBorderTilesBank, (UINT8 *)SgbBorderTilesAddress, SgbBorderTilesSize,
                SgbBorderMapBank, (UINT8 *)SgbBorderMapAddress, SgbBorderMapSize
            );
        }
        if (SgbPalettesBank != 0) {
            const UINT8 bank = SgbPalettesBank;
            const UINT8 * ptr = (UINT8 *)SgbPalettesAddress;
            const UINT8 n = get_uint8(bank, (UINT8 *)ptr); ++ptr;
            for (UINT8 i = 0; i != n; ++i) {
                sgb_send_packet(bank, ptr, sizeof(sgb_palette_packet_t));
                ptr += sizeof(sgb_palette_packet_t);
            }
        }
    }

    device_object_sprite_base = 0;
}

STATIC void load_default_font(void) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(BANK(BUILTIN)); // From builtin font.
    font_load((void*)Font);
    SWITCH_ROM_BANK(_save);
}

/**< Generic. */

#define DEVICE_TRIGGER_ACTIVE_TRIGGERS                 0x3D

#define DEVICE_RTC_SEC                                 0x61
#define DEVICE_RTC_MIN                                 0x62
#define DEVICE_RTC_HOUR                                0x63
#define DEVICE_RTC_DAY                                 0x64

/**< Options. */

#define DEVICE_OPTION_FAST_CPU_ENABLED                 0x01
#define DEVICE_OPTION_AUTO_UPDATE_ENABLED              0x02
#define DEVICE_OPTION_ACTOR_HIT_WITH_DETAILS_ENABLED   0x03
#define DEVICE_OPTION_OBJECT_SPRITE_BASE               0x04

#define DEVICE_OPTION_SRAM_BANK                        0x11
#define DEVICE_OPTION_SRAM_ENABLED                     0x12

#define DEVICE_OPTION_VRAM_BANK                        0x21
#define DEVICE_OPTION_VRAM_USAGE                       0x22

#define DEVICE_OPTION_SCREEN_ENABLED                   0x31
#define DEVICE_OPTION_SCREEN_MODE                      0x32
#define DEVICE_OPTION_MAP_ENABLED                      0x33
#define DEVICE_OPTION_WINDOW_ENABLED                   0x34
#define DEVICE_OPTION_SPRITE_ENABLED                   0x35
#define DEVICE_OPTION_SPRITE8x16_ENABLED               0x36
#define DEVICE_OPTION_ACTIVE_TRIGGERS               /* 0x3D */ DEVICE_TRIGGER_ACTIVE_TRIGGERS

#define DEVICE_OPTION_SOUND_ENABLED                    0x41
#define DEVICE_OPTION_MUSIC_POSITION                   0x42

#define DEVICE_OPTION_SERIAL_ENABLED                   0x51

#define DEVICE_OPTION_RTC_SEC                       /* 0x61 */ DEVICE_RTC_SEC
#define DEVICE_OPTION_RTC_MIN                       /* 0x62 */ DEVICE_RTC_MIN
#define DEVICE_OPTION_RTC_HOUR                      /* 0x63 */ DEVICE_RTC_HOUR
#define DEVICE_OPTION_RTC_DAY                       /* 0x64 */ DEVICE_RTC_DAY
#define DEVICE_OPTION_RTC_ENABLED                      0x65
#define DEVICE_OPTION_RTC_START                        0x66
#define DEVICE_OPTION_RTC_LATCH                        0x67

void vm_option(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 what = (UINT8)*(--THIS->stack_ptr);
    const UINT16 val = (UINT16)*(--THIS->stack_ptr);
    switch (what) {
    case DEVICE_OPTION_FAST_CPU_ENABLED:
        if (device_type & DEVICE_TYPE_CGB) {
            if (val) cpu_fast(); else cpu_slow();
            *(THIS->stack_ptr++) = TRUE;
        } else {
            *(THIS->stack_ptr++) = FALSE;
        }

        break;
    case DEVICE_OPTION_AUTO_UPDATE_ENABLED:
        if (val) { FEATURE_AUTO_UPDATE_ENABLE; } else { FEATURE_AUTO_UPDATE_DISABLE; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_ACTOR_HIT_WITH_DETAILS_ENABLED:
        if (val) { FEATURE_ACTOR_HIT_WITH_DETAILS_ENABLE; } else { FEATURE_ACTOR_HIT_WITH_DETAILS_DISABLE; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_OBJECT_SPRITE_BASE:
        actor_hardware_sprite_count = device_object_sprite_base = (UINT8)val;
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SRAM_BANK:
        SWITCH_RAM((UINT8)val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SRAM_ENABLED:
        if (val) { ENABLE_RAM; } else { DISABLE_RAM; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_VRAM_BANK: // Fall through.
    case DEVICE_OPTION_VRAM_USAGE:
        if (device_type & DEVICE_TYPE_CGB) {
            VBK_REG = (UINT8)val;
            *(THIS->stack_ptr++) = TRUE;
        } else {
            *(THIS->stack_ptr++) = FALSE;
        }

        break;
    case DEVICE_OPTION_SCREEN_ENABLED:
        if (val) { DISPLAY_ON; } else { DISPLAY_OFF; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SCREEN_MODE:
        if (val == DEVICE_SCREEN_TEXT) {
            // Uninstall VBL and LCD ISRs if they were installed by the graphics mode.
            const UINT8 m = get_mode();
            if ((m & 0x07) == M_DRAWING) // Was graphics mode.
                mode(M_TEXT_OUT);

            // To the text mode.
            SHOW_BKG;
            WX_REG = 0, HIDE_WIN;
            HIDE_SPRITES;
            move_bkg(0, 0);
            load_default_font(); // Initialize the text mode.
        } else if (val == DEVICE_SCREEN_GRAPHICS) {
            // To the graphics mode.
            move_bkg(0, 0);
            color(WHITE, WHITE, AND);
            plot_point(0xFF, 0xFF); // Initialize the graphics mode.
        } else if (val == DEVICE_SCREEN_OBJECTS) {
            // Uninstall VBL and LCD ISRs if they were installed by the graphics mode.
            const UINT8 m = get_mode();
            if ((m & 0x07) == M_DRAWING) // Was graphics mode.
                mode(M_TEXT_OUT);
        }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_MAP_ENABLED:
        if (val) { SHOW_BKG; } else { HIDE_BKG; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_WINDOW_ENABLED:
        if (val) { SHOW_WIN; } else { WX_REG = 0, HIDE_WIN; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SPRITE_ENABLED:
        if (val) { SHOW_SPRITES; } else { HIDE_SPRITES; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SPRITE8x16_ENABLED:
        if (val) { SPRITES_8x16; } else { SPRITES_8x8; }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_ACTIVE_TRIGGERS:
        trigger_dim((UINT8)val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SOUND_ENABLED:
        if (val) {
            NR52_REG = 0x80;
            NR51_REG = 0xFF;
            NR50_REG = 0x77;
        } else {
            NR52_REG = 0x00;
        }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_MUSIC_POSITION:
        hUGE_set_position((unsigned char)(val + 1));
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_SERIAL_ENABLED:
        if (val) set_interrupts(DEVICE_ISR_DEFAULT | DEVICE_ISR_SERIAL); else set_interrupts(DEVICE_ISR_DEFAULT);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_SEC:
        rtc_set(RTC_VALUE_SEC, val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_MIN:
        rtc_set(RTC_VALUE_MIN, val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_HOUR:
        rtc_set(RTC_VALUE_HOUR, val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_DAY:
        rtc_set(RTC_VALUE_DAY, val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_ENABLED:
        if (val) {
            if (!PERSISTENCE_FILE_OPENED)
                ENABLE_RAM;
            FEATURE_RTC_ENABLE;
        } else {
            if (!PERSISTENCE_FILE_OPENED)
                DISABLE_RAM;
            FEATURE_RTC_DISABLE;
        }
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_START:
        rtc_start((UINT8)val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    case DEVICE_OPTION_RTC_LATCH:
        rtc_latch(val);
        *(THIS->stack_ptr++) = TRUE;

        break;
    default:
        *(THIS->stack_ptr++) = FALSE;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_DEVICE_ERROR, what);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}

/**< Query. */

#define DEVICE_QUERY_IS_CGB                            0x01
#define DEVICE_QUERY_IS_SGB                            0x02
#define DEVICE_QUERY_IS_AGB                            0x03
#define DEVICE_QUERY_IS_GBB                            0x04
#define DEVICE_QUERY_MAX_THREADS                       0x05
#define DEVICE_QUERY_ACTIVE_THREADS                    0x06
#define DEVICE_QUERY_FREE_THREADS                      0x07
#define DEVICE_QUERY_CURRENT_THREAD_ID                 0x08

#define DEVICE_QUERY_SRAM_BANKS                        0x11
#define DEVICE_QUERY_SRAM_LENGTH                       0x12

#define DEVICE_QUERY_VRAM_BANKS                        0x21

#define DEVICE_QUERY_MAP_X                             0x31
#define DEVICE_QUERY_MAP_Y                             0x32
#define DEVICE_QUERY_CAMERA_X                          0x33
#define DEVICE_QUERY_CAMERA_Y                          0x34
#define DEVICE_QUERY_MAX_ACTORS                        0x35
#define DEVICE_QUERY_INSTANTIATED_ACTORS               0x36
#define DEVICE_QUERY_FREE_ACTORS                       0x37
#define DEVICE_QUERY_ACTIVE_ACTORS                     0x38
#define DEVICE_QUERY_MAX_PROJECTILES                   0x39
#define DEVICE_QUERY_ACTIVE_PROJECTILES                0x3A
#define DEVICE_QUERY_FREE_PROJECTILES                  0x3B
#define DEVICE_QUERY_MAX_TRIGGERS                      0x3C
#define DEVICE_QUERY_ACTIVE_TRIGGERS                /* 0x3D */ DEVICE_TRIGGER_ACTIVE_TRIGGERS
#define DEVICE_QUERY_FREE_TRIGGERS                     0x3E

#define DEVICE_QUERY_SERIAL_STATUS                     0x51

#define DEVICE_QUERY_RTC_SEC                        /* 0x61 */ DEVICE_RTC_SEC
#define DEVICE_QUERY_RTC_MIN                        /* 0x62 */ DEVICE_RTC_MIN
#define DEVICE_QUERY_RTC_HOUR                       /* 0x63 */ DEVICE_RTC_HOUR
#define DEVICE_QUERY_RTC_DAY                        /* 0x64 */ DEVICE_RTC_DAY

#define DEVICE_QUERY_SYS_TIME                          0x71
#define DEVICE_QUERY_DIV_REG                           0x72

#define DEVICE_QUERY_PLATFORM_FLAGS                    0x81

void vm_query(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const UINT8 what = (UINT8)*(--THIS->stack_ptr);
    switch (what) {
    case DEVICE_QUERY_IS_CGB:
        *(THIS->stack_ptr++) = (device_type & DEVICE_TYPE_CGB) ? TRUE : FALSE;

        break;
    case DEVICE_QUERY_IS_SGB:
        *(THIS->stack_ptr++) = (device_type & DEVICE_TYPE_SGB) ? TRUE : FALSE;

        break;
    case DEVICE_QUERY_IS_AGB:
        *(THIS->stack_ptr++) = (device_type & DEVICE_TYPE_AGB) ? TRUE : FALSE;

        break;
    case DEVICE_QUERY_IS_GBB:
        *(THIS->stack_ptr++) = (device_type & DEVICE_TYPE_GBB) ? TRUE : FALSE;

        break;
    case DEVICE_QUERY_MAX_THREADS:
        *(THIS->stack_ptr++) = VM_MAX_CONTEXTS;

        break;
    case DEVICE_QUERY_ACTIVE_THREADS: {
            UINT8 ret = 0;
            SCRIPT_CTX * ctx = first_ctx;
            while (ctx) {
                if (ctx->terminated == FALSE)
                    ++ret;
                ctx = ctx->next;
            }
            *(THIS->stack_ptr++) = ret;
        }

        break;
    case DEVICE_QUERY_FREE_THREADS: {
            UINT8 ret = 0;
            SCRIPT_CTX * ctx = free_ctxs;
            while (ctx) {
                ++ret;
                ctx = ctx->next;
            }
            *(THIS->stack_ptr++) = ret;
        }

        break;
    case DEVICE_QUERY_CURRENT_THREAD_ID:
        *(THIS->stack_ptr++) = THIS->ID;

        break;
    case DEVICE_QUERY_SRAM_BANKS:
        *(THIS->stack_ptr++) = persistence_file_max_count();

        break;
    case DEVICE_QUERY_SRAM_LENGTH: {
            const INT8 sram_type = *(INT8 *)(SRAM_TYPE_ADDRESS);
            if (sram_type == 0x00) {
                *(THIS->stack_ptr++) = 0; // 0KB.
            } else if (sram_type == 0x01) {
                const UINT16 sram_size = 2048; // 2KB totally (single bank).
                *(THIS->stack_ptr++) = sram_size / sizeof(FDATA) - sizeof(FSIGNATURE);
            } else /* if (sram_type == 0x02 || sram_type == 0x03 || sram_type == 0x04) */ {
                const UINT16 sram_size = 8192; // 8KB per bank.
                *(THIS->stack_ptr++) = sram_size / sizeof(FDATA) - sizeof(FSIGNATURE);
            }
        }

        break;
    case DEVICE_QUERY_VRAM_BANKS:
        *(THIS->stack_ptr++) = (device_type & DEVICE_TYPE_CGB) ? 2 : 1;

        break;
    case DEVICE_QUERY_MAP_X:
        *(THIS->stack_ptr++) = graphics_map_x;

        break;
    case DEVICE_QUERY_MAP_Y:
        *(THIS->stack_ptr++) = graphics_map_y;

        break;
    case DEVICE_QUERY_CAMERA_X:
        *(THIS->stack_ptr++) = scene_camera_x;

        break;
    case DEVICE_QUERY_CAMERA_Y:
        *(THIS->stack_ptr++) = scene_camera_x;

        break;
    case DEVICE_QUERY_MAX_ACTORS:
        *(THIS->stack_ptr++) = ACTOR_MAX_COUNT;

        break;
    case DEVICE_QUERY_INSTANTIATED_ACTORS:
        *(THIS->stack_ptr++) = actor_instantiated_count();

        break;
    case DEVICE_QUERY_FREE_ACTORS:
        *(THIS->stack_ptr++) = actor_free_count();

        break;
    case DEVICE_QUERY_ACTIVE_ACTORS:
        *(THIS->stack_ptr++) = actor_active_count();

        break;
    case DEVICE_QUERY_MAX_PROJECTILES:
        *(THIS->stack_ptr++) = PROJECTILE_MAX_COUNT;

        break;
    case DEVICE_QUERY_ACTIVE_PROJECTILES:
        *(THIS->stack_ptr++) = projectile_active_count();

        break;
    case DEVICE_QUERY_FREE_PROJECTILES:
        *(THIS->stack_ptr++) = projectile_free_count();

        break;
    case DEVICE_QUERY_MAX_TRIGGERS:
        *(THIS->stack_ptr++) = TRIGGER_MAX_COUNT;

        break;
    case DEVICE_QUERY_ACTIVE_TRIGGERS:
        *(THIS->stack_ptr++) = trigger_count;

        break;
    case DEVICE_QUERY_FREE_TRIGGERS:
        *(THIS->stack_ptr++) = TRIGGER_MAX_COUNT - trigger_count;

        break;
    case DEVICE_QUERY_SERIAL_STATUS:
        if (_io_status == IO_IDLE)
            *(THIS->stack_ptr++) = SERIAL_IDLE;
        else if (_io_status == IO_ERROR)
            *(THIS->stack_ptr++) = SERIAL_ERROR;
        else
            *(THIS->stack_ptr++) = SERIAL_BUSY;

        break;
    case DEVICE_QUERY_RTC_SEC:
        *(THIS->stack_ptr++) = rtc_get(RTC_VALUE_SEC);

        break;
    case DEVICE_QUERY_RTC_MIN:
        *(THIS->stack_ptr++) = rtc_get(RTC_VALUE_MIN);

        break;
    case DEVICE_QUERY_RTC_HOUR:
        *(THIS->stack_ptr++) = rtc_get(RTC_VALUE_HOUR);

        break;
    case DEVICE_QUERY_RTC_DAY:
        *(THIS->stack_ptr++) = rtc_get(RTC_VALUE_DAY);

        break;
    case DEVICE_QUERY_SYS_TIME:
        *(THIS->stack_ptr++) = sys_time;

        break;
    case DEVICE_QUERY_DIV_REG:
        *(THIS->stack_ptr++) = DIV_REG;

        break;
    case DEVICE_QUERY_PLATFORM_FLAGS:
        if (device_type & DEVICE_TYPE_GBB)
            *(THIS->stack_ptr++) = *(UINT8 *)PLATFORM_FLAGS_REG;
        else
            *(THIS->stack_ptr++) = 0x00;

        break;
    default:
        *(THIS->stack_ptr++) = FALSE;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_DEVICE_ERROR, what);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}
