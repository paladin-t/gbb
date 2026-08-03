#ifndef __VM_GUI_H__
#define __VM_GUI_H__

#include <stdbool.h>

#include "vm.h"

BANKREF_EXTERN(VM_GUI)
BANKREF_EXTERN(VM_GUI_LABEL)
BANKREF_EXTERN(VM_GUI_PROGRESSBAR)
BANKREF_EXTERN(VM_GUI_MENU)

#ifndef GUI_BLIT_INVERSE_ENABLED
#   define GUI_BLIT_INVERSE_ENABLED                           1
#endif /* GUI_BLIT_INVERSE_ENABLED */

#ifndef GUI_BLIT_FAST_TILE_ALIGNED_8x8_ENABLED
#   define GUI_BLIT_FAST_TILE_ALIGNED_8x8_ENABLED             1
#endif /* GUI_BLIT_FAST_TILE_ALIGNED_8x8_ENABLED */

#define GUI_IMPLEMENT_TOUCH_HANDLING_ENABLED                  1
#define GUI_IMPLEMENT_LABEL_ACCURATE_TOUCH_HANDLING_ENABLED   0
#define GUI_IMPLEMENT_MENU_PAGING_ENABLED                     0

#define GUI_WIDGET_TYPE_NONE                                  0
#define GUI_WIDGET_TYPE_LABEL                                 1
#define GUI_WIDGET_TYPE_PROGRESSBAR                           2
#define GUI_WIDGET_TYPE_MENU                                  3

#define GUI_LABEL_POSITION(X, Y)                              (((X) << 8) | (Y))
#define GUI_LABEL_POSITION_X(P)                               ((UINT8)((P) >> 8))
#define GUI_LABEL_POSITION_Y(P)                               ((UINT8)((P) & 0x00FF))
#define GUI_LABEL_X_OFFSET_OF(X)                              (((X) & 0x0F) << 4)
#define GUI_LABEL_X_OFFSET                                    MUL8(gui_label_x_offset >> 4)

#define GUI_PROGRESSBAR_TILE_COUNT                            5

#define GUI_MENU_CLEAR_LINE_HEIGHT                            do { gui_menu_line_height &= 0xF0; } while (0)
#define GUI_MENU_SET_LINE_HEIGHT(H)                           do { gui_menu_line_height |= (H) - 1; } while (0)
#define GUI_MENU_LINE_HEIGHT                                  ((gui_menu_line_height & 0x0F) + 1)

#define GUI_WAIT_FOR_NONE                                     0xFF
#define GUI_WAIT_FOR_NEXT_PAGE                                0xFE
#define GUI_WAIT_FOR_USER_INPUT                               0xFD

#define GUI_CATEGORY(Y, L)                                    (((Y) << 4) | ((L) & 0x0F))
#define GUI_CATEGORY_TYPE(C)                                  ((C) >> 4)
#define GUI_CATEGORY_LAYER(C)                                 ((C) & 0x0F)

#define GUI_PALETTE(A, B)                                     (((A) << 4) | ((B) & 0x0F))
#define GUI_PALETTE_FIRST(C)                                  ((C) >> 4)
#define GUI_PALETTE_SECOND(C)                                 ((C) & 0x0F)

#define GUI_MARGIN(X, Y)                                      (((X) << 4) | ((Y) & 0x0F))
#define GUI_MARGIN_X(M)                                       ((M) >> 4)
#define GUI_MARGIN_Y(M)                                       ((M) & 0x0F)

#define GUI_GLYPH_TERMINATION                                 0
#define GUI_GLYPH_SPACE                                       1

#define GUI_GLYPH_ARBITRARY_ADDRESS(F) ( \
        (F)                    /* base address    */ + \
        sizeof(glyph_option_t) /* options         */ + \
        sizeof(UINT8)          /* font size       */ + \
        sizeof(UINT16)         /* arbitrary count */   \
    )

#define GUI_GLYPH_IS_TERMINATION(G)                           (((G).bank == GUI_GLYPH_TERMINATION) && ((G).ptr == 0))
#define GUI_GLYPH_IS_SPACE(G)                                 ((G).bank == GUI_GLYPH_SPACE)
#define GUI_GLYPH_IS_ESCAPE_RETURN(G)                         (((G).bank == 0) && ((unsigned char)(G).ptr == '\r'))
#define GUI_GLYPH_IS_ESCAPE_NEW_LINE(G)                       (((G).bank == 0) && ((unsigned char)(G).ptr == '\n'))
#define GUI_GLYPH_IS_ESCAPE_NEW_PAGE(G)                       (((G).bank == 0) && ((unsigned char)(G).ptr == '\f'))
#define GUI_GLYPH_IS_ESCAPE_PLACEHOLDER(G)                    (((G).bank == 0) && ((unsigned char)(G).ptr == '%'))
#define GUI_GLYPH_IS_ESCAPE_INT(G)                            (((G).bank == 0) && ((unsigned char)(G).ptr == 'd'))
#define GUI_GLYPH_IS_ESCAPE_HEX(G)                            (((G).bank == 0) && ((unsigned char)(G).ptr == 'x'))
#define GUI_GLYPH_IS_ESCAPE_CHAR(G)                           (((G).bank == 0) && ((unsigned char)(G).ptr == 'c'))
#define GUI_GLYPH_IS_ESCAPE_PERCENT(G)                        (((G).bank == 0) && ((unsigned char)(G).ptr == '%'))
#define GUI_GLYPH_IS_ESCAPE_SPECIAL(G)                        (((G).bank == 0) && ((unsigned char)(G).ptr == '\\'))
#define GUI_GLYPH_IS_ESCAPE_STACK(G)                          (((G).bank == 0) && ((unsigned char)(G).ptr == '#'))
#define GUI_GLYPH_IS_ESCAPE_BACKSLASH(G)                      (((G).bank == 0) && ((unsigned char)(G).ptr == '\\'))

#define GUI_GLYPH_NO_ADVANCE                                  0x0000
#define GUI_GLYPH_ADVANCE_WIDTH(G)                            ((UINT16)(G).ptr)

#define GUI_GLYPH_WIDTH(G)                                    (((G).size >> 4) + 1)
#define GUI_GLYPH_HEIGHT(G)                                   (((G).size & 0x0F) + 1)
#define GUI_GLYPH_BYTES(W, H)                                 (DIV8((W) * (H)) + (MOD8((W) * (H)) ? 1 : 0))

typedef struct glyph_option_t {
    bool two_bits_per_pixel        : 1;
    bool full_word                 : 1;
    bool full_word_for_non_ascii   : 1;
    bool inverted                  : 1;
    bool tile_aligned_8x8          : 1;
    bool reserved1                 : 1;
    bool reserved2                 : 1;
    bool reserved3                 : 1;
} glyph_option_t;

typedef struct glyph_t {
    // If `bank != 0` and `bank != 1`, `ptr` is for banked address;
    // else if `bank == 1`, `ptr` is for advance size between two spaces;
    // else `bank == 0`, `ptr` is for codepoint.
    UINT8 bank;
    UINT8 * ptr;
    UINT8 size;
} glyph_t;

// For `Label`, `ProgressBar`, `Menu`:
//   The widget category, composed as `(type << 4) | (layer & 0x0F)`.
extern UINT8 gui_widget_category;
// For `Label`, `ProgressBar`, `Menu`:
//   The base tile index.
extern UINT8 gui_base_tile;
// For `Label`, `Menu`:
//   The bank of the tiles data.
// For `ProgressBar`:
//   The base tile index; not specified by `DEF` or put, but tile filling operations.
extern UINT8 gui_tiles_bank;
// For `Label`, `Menu`:
//   The address of the tiles data.
// For `ProgressBar`:
//   The address of the tiles data; not specified by `DEF` or put, but tile filling operations.
extern UINT8 * gui_tiles_address;
// For `Label`, `ProgressBar`, `Menu`:
//   The widget width in tiles.
extern UINT8 gui_width;
// For `Label`, `ProgressBar`, `Menu`:
//   The widget height in tiles.
extern UINT8 gui_height;
// For `Label`, `ProgressBar`, `Menu`:
//   Combined with x and y, in pixels.
extern UINT8 gui_margin;
// For `Label`:
//   The x position of the blit cursor, in pixels.
// For `ProgressBar`:
//   The start x position of the widget, in tiles.
// For `Menu`:
//   The start x position of the widget, in tiles.
extern UINT8 gui_cursor_x;
// For `Label`:
//   The y position of the blit cursor, in pixels.
// For `ProgressBar`:
//   The start y position of the widget, in tiles.
// For `Menu`:
//   The start y position of the widget, in tiles.
extern UINT8 gui_cursor_y;
// For `Label`, `Menu`:
//   Points to the text.
extern const glyph_t * gui_text_ptr;
// For `Label`:
//   The interval between blits.
// For `Menu`:
//   The item count of menu
extern UINT8 gui_interval;
// For `Label`:
//   The blit counter.
// For `Menu`:
//   The item cursor of menu
extern UINT8 gui_ticks;
// For `Label`:
//   Previous button state.
// For `Menu`:
//   The item cursor of menu; could be out of bounds.
extern UINT8 gui_button_previous;
// For `Label`:
//   The touch states.
// For `Menu`:
//   The bank of the event handler.
extern UINT8 gui_extra_uint8;
// For `Label`:
//   The start position combined with x and y of the label, in tiles.
// For `Menu`:
//   The address of the event handler.
extern UINT16 gui_extra_uint16;
// For `Label`:
//   The x offset of the blit cursor, in tiles; takes the higher 4 bits.
// For `ProgressBar`:
//   The blit palettes.
// For `Menu`:
//   The line height, stored as height - 1; takes the lower 4 bits.
extern UINT8 gui_options;

#define gui_widget_handler_bank         gui_extra_uint8
#define gui_widget_handler_address      gui_extra_uint16

#define gui_label_touch_states          gui_extra_uint8
#define gui_label_start_position        gui_extra_uint16
#define gui_label_x_offset              gui_options

#define gui_progressbar_blit_palettes   gui_options

#define gui_menu_item_count             gui_interval
#define gui_menu_item_cursor            gui_ticks
#define gui_menu_line_height            gui_options
#define gui_menu_pointer_cursor         gui_button_previous
#define gui_menu_handler_bank           gui_extra_uint8
#define gui_menu_handler_address        gui_extra_uint16

void gui_init(void) BANKED;

void gui_clear(UINT8 base_tile, UINT16 n) BANKED;
void gui_tile_filled(UINT8 layer, UINT8 first, UINT8 n, UINT8 bank, UINT8 * ptr) BANKED;

void gui_blit_char(UINT8 size, const glyph_t * glyph, const glyph_option_t * option) BANKED;
void gui_blit_arbitrary_int(INT16 val, UINT8 bank, UINT8 size, const glyph_t * arb, const glyph_option_t * option) BANKED;
void gui_blit_arbitrary_hex(UINT16 val, UINT8 bank, UINT8 size, const glyph_t * arb, const glyph_option_t * option) BANKED;
void gui_blit_new_line(UINT8 size) BANKED;

void gui_blit_progressbar_head_body_tail(UINT8 val0, UINT8 val1, UINT8 val2) BANKED;
void gui_blit_progressbar_body_full(void) BANKED;
void gui_blit_progressbar_body_empty(void) BANKED;

void gui_menu_update(void) BANKED;

void vm_def_widget(SCRIPT_CTX * THIS, UINT8 type) OLDCALL BANKED;
void vm_label(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 nargs, UINT8 font_bank, UINT8 * font_ptr) OLDCALL BANKED;
void vm_progressbar(SCRIPT_CTX * THIS, UINT8 nargs) OLDCALL BANKED;
void vm_menu(SCRIPT_CTX * THIS, UINT8 nlines, UINT8 nargs, UINT8 font_bank, UINT8 * font_ptr) OLDCALL BANKED;
void vm_on_widget(SCRIPT_CTX * THIS, UINT8 event_bank, UINT8 * event_ptr) OLDCALL BANKED;

#endif /* __VM_GUI_H__ */
