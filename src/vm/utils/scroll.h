#ifndef __SCROLL_H__
#define __SCROLL_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

/**
 * @brief Gets the base address of the window layer.
 */
UINT8 * get_win_addr(void) OLDCALL PRESERVES_REGS(b, c, h, l);

/**
 * @brief Gets the base address of the background layer.
 */
UINT8 * get_bkg_addr(void) OLDCALL PRESERVES_REGS(b, c, h, l);

/**
 * @brief Scrolls rectangle area of VRAM tilemap by base address one row up.
 *
 * @param base the address of the top-left corner
 * @param w the width of the area
 * @param h the height of the area
 * @param data the tile id to fill the bottom row
 */
void scroll_up(UINT8 * base, UINT8 w, UINT8 h, UINT8 data) OLDCALL BANKED PRESERVES_REGS(b, c);
/**
 * @brief Scrolls rectangle area of VRAM tilemap by base address one row down.
 *
 * @param base the address of the top-left corner
 * @param w the width of the area
 * @param h the height of the area
 * @param data the tile id to fill the top row
 */
void scroll_down(UINT8 * base, UINT8 w, UINT8 h, UINT8 data) OLDCALL BANKED PRESERVES_REGS(b, c);

#endif /* __SCROLL_H__ */
