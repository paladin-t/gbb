#ifndef __SGB_H__
#define __SGB_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

void set_sgb_border(
    UINT8 tiledata_bank, const UINT8 * tiledata, UINT16 tiledata_size,
    UINT8 tilemap_bank,  const UINT8 * tilemap,  UINT16 tilemap_size,
    UINT8 palette_bank,  const UINT8 * palette,  UINT16 palette_size
) BANKED;

#endif /* __SGB_H__ */
