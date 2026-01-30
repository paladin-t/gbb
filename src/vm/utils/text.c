#pragma bank 255

#if defined __SDCC
#   include "graphics.h"
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdio.h>

#include "text.h"
#include "utils.h"

#include "../vm_device.h"

UINT8 int16_to_str(INT16 n, unsigned char * s) BANKED {
    const char DEC[] = "0123456789";

    if (n == 0) {
        *s++ = '0';

        return 1;
    }
    if (n == 0x8000 /* -32768 */) { // To avoid a overflow issue.
        *s++ = '-';
        *s++ = '3';
        *s++ = '2';
        *s++ = '7';
        *s++ = '6';
        *s++ = '8';

        return 6;
    }

    UINT8 result = 0;
    if (n < 0) {
        *s++ = '-';
        ++result;
        n = -n;
    }
    UINT8 c = 0;
    if (n < 10) c = 1;
    else if (n < 100) c = 2;
    else if (n < 1000) c = 3;
    else if (n < 10000) c = 4;
    else /* if (n <= 32767) */ c = 5;
    s += c;

    while (n) {
        *(--s) = DEC[n % 10];
        ++result;
        n /= 10;
    }

    return result;
}

UINT8 uint16_to_hex(UINT16 n, unsigned char * s) BANKED {
    const char HEX[] = "0123456789ABCDEF";
    if (n <= 0x0F) {
        *s++ = HEX[(UINT8)n & 0x0F];

        return 1;
    } else if (n <= 0xFF) {
        *s++ = HEX[((UINT8)n >> 4) & 0x0F];
        *s++ = HEX[(UINT8)n & 0x0F];

        return 2;
    } else if (n <= 0x0FFF) {
        *s++ = HEX[(UINT8)(n >> 8) & 0x0F];
        *s++ = HEX[((UINT8)n >> 4) & 0x0F];
        *s++ = HEX[(UINT8)n & 0x0F];

        return 3;
    } else /* if (n <= 0xFFFF) */ {
        *s++ = HEX[(UINT8)(n >> 8) >> 4];
        *s++ = HEX[(UINT8)(n >> 8) & 0x0F];
        *s++ = HEX[((UINT8)n >> 4) & 0x0F];
        *s++ = HEX[(UINT8)n & 0x0F];

        return 4;
    }
}

UINT8 uint16_to_hex_full(UINT16 n, unsigned char * s) BANKED {
    const char HEX[] = "0123456789ABCDEF";
    *s++ = HEX[(UINT8)(n >> 8) >> 4];
    *s++ = HEX[(UINT8)(n >> 8) & 0x0F];
    *s++ = HEX[((UINT8)n >> 4) & 0x0F];
    *s++ = HEX[(UINT8)n & 0x0F];

    return 4;
}

void print_banked(unsigned char * begin, unsigned char * end, BOOLEAN new_line, UINT8 mode) BANKED {
    const UINT8 len = end - begin;
    for (UINT8 i = 0; i != len; ++i) {
        unsigned char * c = begin + i;
        if (mode == TEXT_PRINT_MODE_TEXT) {
            putchar(*c);
        } else if (mode == TEXT_PRINT_MODE_GRAPHICS) {
            wrtchr(*c);
        } else if (mode == TEXT_PRINT_MODE_SHELL) {
            if (i >= TRANSFER_MAX_SIZE)
                break;

            *(UINT8 *)(TRANSFER_ADDRESS + i) = *c;
        }
    }
    if (new_line) {
        if (mode == TEXT_PRINT_MODE_TEXT)
            putchar('\n');
        else if (mode == TEXT_PRINT_MODE_GRAPHICS)
            newline();
    }
}

void print(SCRIPT_CTX * THIS, BOOLEAN new_line, UINT8 nargs, UINT8 mode) NONBANKED {
    unsigned char display_text[80];

    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(THIS->bank);

    const INT16 * args = (INT16 *)THIS->PC;
    const unsigned char * str = THIS->PC + MUL2(nargs) /* `UINT16` for each argument */;
    unsigned char * d = display_text;

    while (*str) {
        if (*str == '%') {
            switch (*++str) {
            case 'd': {
                    INT16 val = *((INT16 *)VM_REF_TO_PTR(*args));
                    d += int16_to_str(val, d);
                }

                break;
            case 'x': {
                    UINT16 val = *((UINT16 *)VM_REF_TO_PTR(*args));
                    d += uint16_to_hex_full(val, d);
                }

                break;
            case 'c': {
                    INT16 val = *((INT16 *)VM_REF_TO_PTR(*args));
                    *d++ = (unsigned char)val;
                }

                break;
            case '%':
                ++str;
                // Fall through.
            default:
                --str;
                *d++ = *str++;

                continue;
            }
        } else if (*str == '\\') {
            switch (*++str) {
            case '#': {
                    UINT16 val = (UINT16)THIS->stack_ptr;
                    d += uint16_to_hex_full(val, d);
                    ++str;
                }

                continue;
            case '\\':
                ++str;
                // Fall through.
            default:
                --str;
                *d++ = *str++;

                continue;
            }
        } else {
            *d++ = *str++;

            continue;
        }

        ++str; ++args;
    }

    *d = 0;

    if (mode != TEXT_PRINT_MODE_NONE)
        print_banked(display_text, d, new_line, mode);

    SWITCH_ROM_BANK(_save);
    THIS->PC = str + 1;
}
