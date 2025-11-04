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
#include "vm_memory.h"
#include "vm_object.h"
#include "vm_persistence.h"
#include "vm_physics.h"
#include "vm_projectile.h"
#include "vm_scene.h"
#include "vm_scroll.h"
#include "vm_serial.h"
#include "vm_system.h"
#include "vm_trigger.h"

#if defined __SDCC
    // Define addressmod for HOME.
    void ___vm_dummy_fn(void) NONBANKED PRESERVES_REGS(a, b, c, d, e, h, l) { }
    __addressmod ___vm_dummy_fn const HOME;
#else /* __SDCC */
#   define HOME
#endif /* __SDCC */

// Define all the VM instructions: their handlers, bank and parameter lengths in
// bytes. This array must be nonbanked as well as `VM_STEP(...)`.
HOME const SCRIPT_CMD script_cmds[] = {
    /**< Basic instructions. */

    // Halt and nop.
    /*
    { vm_halt,                  BANK(VM_MAIN),              0 }, // 0x00.
    */
    { vm_nop,                   BANK(VM_MAIN),              0 }, // 0x01.

    // Stack manipulation.
    { vm_reserve,               BANK(VM_MAIN),              1 }, // 0x02.
    { vm_push,                  BANK(VM_MAIN),              2 }, // 0x03.
    { vm_push_value,            BANK(VM_MAIN),              2 }, // 0x04.
    { vm_pop,                   BANK(VM_MAIN),              1 }, // 0x05.
    { vm_pop_1,                 BANK(VM_MAIN),              1 }, // 0x06.

    // Invoking.
    { vm_call,                  BANK(VM_MAIN),              2 }, // 0x07. // Unused by compiler.
    { vm_ret,                   BANK(VM_MAIN),              1 }, // 0x08. // Unused by compiler.
    { vm_call_far,              BANK(VM_MAIN),              3 }, // 0x09.
    { vm_ret_far,               BANK(VM_MAIN),              1 }, // 0x0A.

    // Jump, conditional and loop.
    { vm_jump,                  BANK(VM_MAIN),              2 }, // 0x0B.
    { vm_jump_far,              BANK(VM_MAIN),              3 }, // 0x0C.
    { vm_next_bank,             BANK(VM_MAIN),              0 }, // 0x0D.
    { vm_switch,                BANK(VM_MAIN),              5 }, // 0x0E.
    { vm_if,                    BANK(VM_MAIN),              8 }, // 0x0F. // Unused by compiler.
    { vm_if_const,              BANK(VM_MAIN),              8 }, // 0x10.
    { vm_iif,                   BANK(VM_MAIN),              8 }, // 0x11.
    { vm_loop,                  BANK(VM_MAIN),              9 }, // 0x12.
    { vm_for,                   BANK(VM_MAIN),              9 }, // 0x13.

    // Data and value.
    { vm_set,                   BANK(VM_MAIN),              4 }, // 0x14.
    { vm_set_const,             BANK(VM_MAIN),              4 }, // 0x15.
    { vm_set_indirect,          BANK(VM_MAIN),              4 }, // 0x16.
    { vm_get_indirect,          BANK(VM_MAIN),              4 }, // 0x17.
    { vm_acc,                   BANK(VM_MAIN),              5 }, // 0x18.
    { vm_acc_const,             BANK(VM_MAIN),              4 }, // 0x19.
    { vm_rpn,                   BANK(VM_MAIN),              0 }, // 0x1A.
    { vm_get_tlocal,            BANK(VM_MAIN),              4 }, // 0x1B.
    { vm_set_tlocal,            BANK(VM_MAIN),              4 }, // 0x1C.
    { vm_pack,                  BANK(VM_MAIN),             11 }, // 0x1D.
    { vm_unpack,                BANK(VM_MAIN),             11 }, // 0x1E.
    { vm_swap,                  BANK(VM_MAIN),              4 }, // 0x1F.
    { vm_data_ptr,              BANK(VM_MAIN),              3 }, // 0x20.
    { vm_read,                  BANK(VM_MAIN),              5 }, // 0x21.
    { vm_restore,               BANK(VM_MAIN),              3 }, // 0x22.

    // Thread manipulation.
    { vm_begin_thread,          BANK(VM_MAIN),              7 }, // 0x23.
    { vm_join,                  BANK(VM_MAIN),              2 }, // 0x24.
    { vm_terminate,             BANK(VM_MAIN),              3 }, // 0x25.
    { vm_wait,                  BANK(VM_MAIN),              0 }, // 0x26.
    { vm_wait_n,                BANK(VM_MAIN),              2 }, // 0x27.
    { vm_lock,                  BANK(VM_MAIN),              0 }, // 0x28.
    { vm_unlock,                BANK(VM_MAIN),              0 }, // 0x29.

    // Functions.
    { vm_invoke_fn,             BANK(VM_MAIN),              6 }, // 0x2A.
    { vm_locate,                BANK(VM_MAIN),              4 }, // 0x2B.
    { vm_print,                 BANK(VM_MAIN),              2 }, // 0x2C.
    { vm_srand,                 BANK(VM_MAIN),              2 }, // 0x2D.
    { vm_rand,                  BANK(VM_MAIN),              6 }, // 0x2E.
    { vm_peek,                  BANK(VM_MAIN),              5 }, // 0x2F.
    { vm_poke,                  BANK(VM_MAIN),              5 }, // 0x30.
    { vm_fill,                  BANK(VM_MAIN),              6 }, // 0x31.

    /**< Memory instructions. */

    // Memory.
    { vm_memcpy,                BANK(VM_MEMORY),            1 }, // 0x32.
    { vm_memset,                BANK(VM_MEMORY),            0 }, // 0x33.
    { vm_memadd,                BANK(VM_MEMORY),            0 }, // 0x34.

    /**< System instructions. */

    // System.
    { vm_sys_time,              BANK(VM_SYSTEM),            2 }, // 0x35.
    { vm_sleep,                 BANK(VM_SYSTEM),            2 }, // 0x36.
    { vm_raise,                 BANK(VM_SYSTEM),            2 }, // 0x37.
    { vm_reset,                 BANK(VM_SYSTEM),            0 }, // 0x38.
    { vm_dbginfo,               BANK(VM_SYSTEM),            0 }, // 0x39.

    /**< Graphics instructions. */

    // Primitives.
    { vm_color,                 BANK(VM_GRAPHICS),          0 }, // 0x3A.
    { vm_palette,               BANK(VM_GRAPHICS),          1 }, // 0x3B.
    { vm_rgb,                   BANK(VM_GRAPHICS),          0 }, // 0x3C.
    { vm_plot,                  BANK(VM_GRAPHICS),          0 }, // 0x3D.
    { vm_point,                 BANK(VM_GRAPHICS),          0 }, // 0x3E.
    { vm_line,                  BANK(VM_GRAPHICS),          0 }, // 0x3F.
    { vm_rect,                  BANK(VM_GRAPHICS),          1 }, // 0x40.
    { vm_gotoxy,                BANK(VM_GRAPHICS),          0 }, // 0x41.
    { vm_text,                  BANK(VM_GRAPHICS),          2 }, // 0x42.

    // Image operation.
    { vm_image,                 BANK(VM_GRAPHICS),          1 }, // 0x43.

    // Tile operation.
    { vm_fill_tile,             BANK(VM_GRAPHICS),          2 }, // 0x44.

    // Map operation.
    { vm_def_map,               BANK(VM_GRAPHICS),          1 }, // 0x45.
    { vm_map,                   BANK(VM_GRAPHICS),          0 }, // 0x46.
    { vm_mget,                  BANK(VM_GRAPHICS),          0 }, // 0x47.
    { vm_mset,                  BANK(VM_GRAPHICS),          0 }, // 0x48.

    // Window operation.
    { vm_def_window,            BANK(VM_GRAPHICS),          1 }, // 0x49.
    { vm_window,                BANK(VM_GRAPHICS),          0 }, // 0x4A.
    { vm_wget,                  BANK(VM_GRAPHICS),          0 }, // 0x4B.
    { vm_wset,                  BANK(VM_GRAPHICS),          0 }, // 0x4C.

    // Sprite operation.
    { vm_def_sprite,            BANK(VM_GRAPHICS),          1 }, // 0x4D.
    { vm_sprite,                BANK(VM_GRAPHICS),          0 }, // 0x4E.
    { vm_sget,                  BANK(VM_GRAPHICS),          0 }, // 0x4F.
    { vm_sset,                  BANK(VM_GRAPHICS),          0 }, // 0x50.
    { vm_get_sprite_prop,       BANK(VM_GRAPHICS),          0 }, // 0x51.
    { vm_set_sprite_prop,       BANK(VM_GRAPHICS),          0 }, // 0x52.

    /**< Audio instructions. */

    // Music.
    { vm_play,                  BANK(VM_AUDIO),             3 }, // 0x53.
    { vm_stop,                  BANK(VM_AUDIO),             0 }, // 0x54.

    // SFX.
    { vm_sound,                 BANK(VM_AUDIO),             5 }, // 0x55.

    /**< Input instructions. */

    // Gamepad.
    { vm_btn,                   BANK(VM_INPUT),             0 }, // 0x56.
    { vm_btnd,                  BANK(VM_INPUT),             0 }, // 0x57.
    { vm_btnu,                  BANK(VM_INPUT),             0 }, // 0x58.

    // Touch.
    { vm_touch,                 BANK(VM_INPUT),             5 }, // 0x59. GBB EXTENSION.
    { vm_touchd,                BANK(VM_INPUT),             5 }, // 0x5A. GBB EXTENSION.
    { vm_touchu,                BANK(VM_INPUT),             5 }, // 0x5B. GBB EXTENSION.

    // Callback.
    { vm_on_input,              BANK(VM_INPUT),             4 }, // 0x5C.

    /**< Persistence instructions. */

    // File.
    { vm_fopen,                 BANK(VM_PERSISTENCE),       0 }, // 0x5D.
    { vm_fclose,                BANK(VM_PERSISTENCE),       0 }, // 0x5E.
    { vm_fread,                 BANK(VM_PERSISTENCE),       0 }, // 0x5F.
    { vm_fwrite,                BANK(VM_PERSISTENCE),       0 }, // 0x60.

    /**< Serial port instructions. */

    // Serial port.
    { vm_sread,                 BANK(VM_SERIAL),            0 }, // 0x61.
    { vm_swrite,                BANK(VM_SERIAL),            0 }, // 0x62.

    /**< Scene instructions. */

    // Camera and viewport.
    { vm_camera,                BANK(VM_SCENE),             0 }, // 0x63.
    { vm_viewport,              BANK(VM_SCENE),             4 }, // 0x64.

    // Scene initialization.
    { vm_def_scene,             BANK(VM_SCENE),             1 }, // 0x65.
    { vm_load_scene,            BANK(VM_SCENE),             0 }, // 0x66.

    // Scene property.
    { vm_get_scene_prop,        BANK(VM_SCENE),             0 }, // 0x67.
    { vm_set_scene_prop,        BANK(VM_SCENE),             0 }, // 0x68.

    /**< Actor instructions. */

    // Actor constructor/destructor.
    { vm_new_actor,             BANK(VM_ACTOR),             0 }, // 0x69.
    { vm_del_actor,             BANK(VM_ACTOR),             0 }, // 0x6A.

    // Actor initialization.
    { vm_def_actor,             BANK(VM_ACTOR),             1 }, // 0x6B.

    // Actor property.
    { vm_get_actor_prop,        BANK(VM_ACTOR),             0 }, // 0x6C.
    { vm_set_actor_prop,        BANK(VM_ACTOR),             1 }, // 0x6D.

    // Actor animation.
    { vm_play_actor,            BANK(VM_ACTOR),             0 }, // 0x6E.

    // Actor threading.
    { vm_thread_actor,          BANK(VM_ACTOR),             1 }, // 0x6F.

    // Actor motion.
    { vm_move_actor,            BANK(VM_ACTOR),             1 }, // 0x70.

    // Actor lookup.
    { vm_find_actor,            BANK(VM_ACTOR),             1 }, // 0x71.

    // Actor callback.
    { vm_on_actor,              BANK(VM_ACTOR),             4 }, // 0x72.

    /**< Emote instructions. */

    // Emote.
    { vm_emote,                 BANK(VM_EMOTE),             1 }, // 0x73.

    /**< Projectile instructions. */

    // Projectile initialization.
    { vm_def_projectile,        BANK(VM_PROJECTILE),        1 }, // 0x74.
    { vm_start_projectile,      BANK(VM_PROJECTILE),        1 }, // 0x75.

    // Projectile property.
    { vm_get_projectile_prop,   BANK(VM_PROJECTILE),        0 }, // 0x76.
    { vm_set_projectile_prop,   BANK(VM_PROJECTILE),        1 }, // 0x77.

    /**< Trigger instructions. */

    // Trigger initialization.
    { vm_def_trigger,           BANK(VM_TRIGGER),           1 }, // 0x78.

    // Trigger callback.
    { vm_on_trigger,            BANK(VM_TRIGGER),           3 }, // 0x79.

    /**< Object instructions. */

    // Object typing.
    { vm_object_is,             BANK(VM_OBJECT),            1 }, // 0x7A.

    /**< GUI instructions. */

    // GUI widget initialization.
    { vm_def_widget,            BANK(VM_GUI),               1 }, // 0x7B.

    // Widgets.
    { vm_label,                 BANK(VM_GUI_LABEL),         5 }, // 0x7C.
    { vm_progressbar,           BANK(VM_GUI_PROGRESSBAR),   1 }, // 0x7D.
    { vm_menu,                  BANK(VM_GUI_MENU),          5 }, // 0x7E.

    // GUI callback.
    { vm_on_widget,             BANK(VM_GUI),               3 }, // 0x7F.

    /**< Scroll instructions. */

    // Scroll.
    { vm_scroll,                BANK(VM_SCROLL),            0 }, // 0x80.

    /**< Effects instructions. */

    // Effects.
    { vm_fx,                    BANK(VM_EFFECTS),           0 }, // 0x81.

    /**< Physics instructions. */

    // Collision detection.
    { vm_hits,                  BANK(VM_PHYSICS),           1 }, // 0x82.

    /**< Game instructions. */

    // Game loop.
    { vm_update,                BANK(VM_GAME),              0 }, // 0x83.

    /**< Hardware features instructions. */

    // Feature switch and query.
    { vm_option,                BANK(VM_DEVICE),            0 }, // 0x84.
    { vm_query,                 BANK(VM_DEVICE),            0 }, // 0x85.

    // Streaming.
    { vm_stream,                BANK(VM_DEVICE_EXT),        1 }, // 0x86. GBB EXTENSION.

    // Shell.
    { vm_shell,                 BANK(VM_DEVICE_EXT),        1 }  // 0x87. GBB EXTENSION.
};
