#pragma bank 255

#if defined __SDCC
#   pragma disable_warning 110
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "sgb.h"
#include "utils.h"

#define SGB_CHR_BLOCK0     0
#define SGB_CHR_BLOCK1     1

#define SGB_SCR_FREEZE     1
#define SGB_SCR_UNFREEZE   0

#define SGB_TRANSFER(A, B) \
    do { \
        map_buf[0] = (A); \
        map_buf[1] = (B); \
        sgb_transfer(map_buf); \
    } while (0)
#define SGB_DEF(BANK, PTR, F, N, FUNC) \
    do { \
        call_v_bbp_oldcall((F), (N), (BANK), (PTR), (FUNC)); \
    } while (0)

void sgb_set_border(
    UINT8 palette_bank, const UINT8 * palette, UINT16 palette_size,
    UINT8 tiledata_bank, const UINT8 * tiledata, UINT16 tiledata_size,
    UINT8 tilemap_bank, const UINT8 * tilemap, UINT16 tilemap_size
) BANKED {
    UINT8 map_buf[20];
    memset(map_buf, 0, sizeof(map_buf));

    SGB_TRANSFER((SGB_MASK_EN << 3) | 1, SGB_SCR_FREEZE);

    BGP_REG = OBP0_REG = OBP1_REG = 0xE4;
    SCX_REG = SCY_REG = 0;

    const UINT8 tmp_lcdc = LCDC_REG;

    HIDE_SPRITES, HIDE_WIN, SHOW_BKG;
    DISPLAY_ON;

    // Prepare tilemap for SGB_BORDER_CHR_TRN (should display all 256 tiles).
    UINT8 i = 0;
    for (UINT8 y = 0; y != 14; ++y) {
        UINT8 * dout = map_buf;
        for (UINT8 x = 0; x != 20; ++x) {
            *dout++ = i++;
        }
        set_bkg_submap(0, y, 20, 1, map_buf, 20);
    }
    memset(map_buf, 0, sizeof(map_buf));

    // Transfer tile data.
    UINT8 ntiles = (tiledata_size > 256 * 32) ? 0 : tiledata_size >> 5;
    if ((!ntiles) || (ntiles > 128)) {
        SGB_DEF(tiledata_bank, (UINT8 *)tiledata, 0, 0, set_bkg_data);
        SGB_TRANSFER((SGB_CHR_TRN << 3) | 1, SGB_CHR_BLOCK0);
        if (ntiles) ntiles -= 128;
        tiledata += (128 * 32);
        SGB_DEF(tiledata_bank, (UINT8 *)tiledata, 0, ntiles << 1, set_bkg_data);
        SGB_TRANSFER((SGB_CHR_TRN << 3) | 1, SGB_CHR_BLOCK1);
    } else {
        SGB_DEF(tiledata_bank, (UINT8 *)tiledata, 0, ntiles << 1, set_bkg_data);
        SGB_TRANSFER((SGB_CHR_TRN << 3) | 1, SGB_CHR_BLOCK0);
    }

    // Transfer map and palettes.
    SGB_DEF(tilemap_bank, (UINT8 *)tilemap, 0, (UINT8)(tilemap_size >> 4), set_bkg_data);
    SGB_DEF(palette_bank, (UINT8 *)palette, 128, (UINT8)(palette_size >> 4), set_bkg_data);
    SGB_TRANSFER((SGB_PCT_TRN << 3) | 1, 0);

    LCDC_REG = tmp_lcdc;

    // Clear screen.
    memset(map_buf, 0, 16);
    set_bkg_data(0, 1, map_buf);
    for (UINT8 j = 0; j != 18; ++j) {
        for (UINT8 i = 0; i != 20; ++i) {
            set_bkg_tile_xy(i, j, 0);
        }
    }

    SGB_TRANSFER((SGB_MASK_EN << 3) | 1, SGB_SCR_UNFREEZE);
}
