#ifndef __SGB_H__
#define __SGB_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

typedef struct sgb_palette_packet_t {
    UINT8 cmd;
    UINT16 colors[7];
} sgb_palette_packet_t;

void sgb_send_packet(UINT8 bank, const UINT8 * packet, UINT8 size) BANKED;

void sgb_set_border(
    UINT8 palette_bank, const UINT8 * palette, UINT16 palette_size,
    UINT8 tiledata_bank, const UINT8 * tiledata, UINT16 tiledata_size,
    UINT8 tilemap_bank, const UINT8 * tilemap, UINT16 tilemap_size
) BANKED;

#endif /* __SGB_H__ */
