#ifndef __VM_DEVICE_H__
#define __VM_DEVICE_H__

#include "vm.h"

BANKREF_EXTERN(VM_DEVICE)
BANKREF_EXTERN(VM_DEVICE_EXT)

/**< The GBB extended conventions. GBB EXTENSION. */

// Hardware model: any one with extension.
#define ANY_EXT_TYPE                   0x01
// Hardware model: grayscale with extension.
#define GB_EXT_TYPE                   (0x10 | ANY_EXT_TYPE)
// Hardware model: colored with extension.
#define CGB_EXT_TYPE                  (0x20 | ANY_EXT_TYPE)
// Hardware model: SGB with extension.
#define SGB_EXT_TYPE                  (0x50 | ANY_EXT_TYPE)

// The first button of the touch device.
#define TOUCH_BUTTON_0                 0x01
// The second button of the touch device.
#define TOUCH_BUTTON_1                 0x02

// Transfer status: ready.
#define TRANSFER_STATUS_READY          0x00
// Transfer status: busy, program is accessing.
#define TRANSFER_STATUS_BUSY           0x01
// Transfer status: filled, program has finished writing.
#define TRANSFER_STATUS_FILLED         0x02

// Streaming status: ready.
#define STREAMING_STATUS_READY         0x00
// Streaming status: busy, program is accessing.
#define STREAMING_STATUS_BUSY          0x01
// Streaming status: filled, program has finished writing.
#define STREAMING_STATUS_FILLED        0x02
// Streaming status: end-of-stream.
#define STREAMING_STATUS_EOS           0x80

// Register of the extension mark.
#define EXTENSION_STATUS_REG           0xFEA0
// Register of the platform flags.
#define PLATFORM_FLAGS_REG             0xFEA1
// Register of the localization flags.
#define LOCALIZATION_FLAGS_REG         0xFEA2 // Not documented.
// Register of the x-axis of the touch device.
#define TOUCH_X_REG                    0xFEA4
// Register of the y-axis of the touch device.
#define TOUCH_Y_REG                    0xFEA5
// Register of the pressed state of the touch device.
#define TOUCH_PRESSED_REG              0xFEA6
// Register of the key modifier flags.
#define KEY_MODIFIER_FLAGS_REG         0xFEA8
// Register of the first valid key code in buffer.
#define KEY_CODE_REG                   0xFEA9
// Register of the streaming status.
#define STREAMING_STATUS_REG           0xFEAC
// Address of the streaming byte.
#define STREAMING_ADDRESS              0xFEAD
// Register of the transfer status.
#define TRANSFER_STATUS_REG            0xFEAF
// Address of the transfer area.
#define TRANSFER_ADDRESS               0xFEB0
// Size of the transfer area.
#define TRANSFER_MAX_SIZE              0x40

/**< The device conventions. */

#define DEVICE_TYPE_ADDRESS            0x0143
#define DEVICE_TYPE_GB                 0x01
#define DEVICE_TYPE_CGB                0x02
#define DEVICE_TYPE_SGB                0x04
#define DEVICE_TYPE_AGB                0x08
#define DEVICE_TYPE_GBB                0x80

#define DEVICE_ISR_DEFAULT            (VBL_IFLAG | LCD_IFLAG | TIM_IFLAG)
#define DEVICE_ISR_SERIAL              SIO_IFLAG

#define DEVICE_SCREEN_TEXT             1
#define DEVICE_SCREEN_GRAPHICS         2
#define DEVICE_SCREEN_OBJECTS          3

/**< The ROM conventions. */

#define ROM_TITLE_ADDRESS              0x0134
#define ROM_TITLE_SIZE                 0x10
#define ROM_CHECKSUM_ADDRESS           0x014E

/**< The SRAM conventions. */

#define SRAM_TYPE_ADDRESS              0x0149
#define SRAM_OFFSET                    _SRAM
#define SRAM_BANK_SIZE                (_RAM - _SRAM)

#define VRAM_SPR0_TILE_ADDRESS         _VRAM8000
#define VRAM_SPR1_TILE_ADDRESS         _VRAM8800
#define VRAM_BKG0_TILE_ADDRESS         _VRAM8800
#define VRAM_BKG1_TILE_ADDRESS         _VRAM9000

#define VRAM_BASE_TILE_ADDRESS(BASE)   ((((BASE) >= 128) ? (VRAM_BKG0_TILE_ADDRESS - MUL16(128)) : (VRAM_BKG1_TILE_ADDRESS)) + MUL16(BASE))

/**< Common. */

__BYTE_REG EXTF;
__BYTE_REG PLTF;
__BYTE_REG LOCF;
__BYTE_REG TCHX;
__BYTE_REG TCHY;
__BYTE_REG TCHF;
__BYTE_REG KEYM;
__BYTE_REG KEYC;
__BYTE_REG STMF;
__BYTE_REG STMB;
__BYTE_REG TRSF;
__BYTE_REG TRSC;

extern UINT8 device_type;

extern UINT8 device_object_sprite_base;

void device_init(void) BANKED;

/**< Options. */

void vm_option(SCRIPT_CTX * THIS) OLDCALL BANKED;

/**< Query. */

void vm_query(SCRIPT_CTX * THIS) OLDCALL BANKED;

/**< Streaming. */

void vm_stream(SCRIPT_CTX * THIS, UINT8 status) OLDCALL BANKED; // GBB EXTENSION.

/**< Shell. */

void vm_shell(SCRIPT_CTX * THIS, UINT8 nargs) OLDCALL BANKED; // GBB EXTENSION.

#endif /* __VM_DEVICE_H__ */
