#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils.h"

/**< ROM banking. */

UINT8 get_uint8(UINT8 bank, const UINT8 * ptr) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(bank);
    const UINT8 result = *ptr;
    SWITCH_ROM_BANK(_save);

    return result;
}

void get_chunk(UINT8 * dst, UINT8 bank, const UINT8 * ptr, UINT8 size) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(bank);
    while (size--)
        *dst++ = *ptr++;
    SWITCH_ROM_BANK(_save);
}

void call_v_bbp_oldcall(UINT8 a, UINT8 b, UINT8 bank, UINT8 * ptr, v_bbp_fn_oldcall func) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(bank);
    func(a, b, ptr);
    SWITCH_ROM_BANK(_save);
}

void call_v_bbbbpb_oldcall(UINT8 a, UINT8 b, UINT8 c, UINT8 d, UINT8 bank, UINT8 * ptr, UINT8 e, v_bbbbpb_fn_oldcall func) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(bank);
    func(a, b, c, d, ptr, e);
    SWITCH_ROM_BANK(_save);
}

void call_v_www(UINT16 a, UINT16 b, UINT16 c, UINT8 bank, v_www_fn func) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(bank);
    func(a, b, c);
    SWITCH_ROM_BANK(_save);
}

UINT8 call_b_bw(UINT8 a, UINT16 b, UINT8 bank, b_bw_fn func) NONBANKED {
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(bank);
    UINT8 ret = func(a, b);
    SWITCH_ROM_BANK(_save);

    return ret;
}

/**< Features. */

UINT8 feature_states = 0;

/**< Timing. */

UINT8 game_time = 0;

/**< Math. */

const INT8 sine_wave[256] = {
       0,    3,    6,    9,   12,   16,   19,   22,   25,   28,   31,   34,   37,   40,   43,   46,
      49,   51,   54,   57,   60,   63,   65,   68,   71,   73,   76,   78,   81,   83,   85,   88,
      90,   92,   94,   96,   98,  100,  102,  104,  106,  107,  109,  111,  112,  113,  115,  116,
     117,  118,  120,  121,  122,  122,  123,  124,  125,  125,  126,  126,  126,  127,  127,  127,
     127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,  122,  121,  120,  118,
     117,  116,  115,  113,  112,  111,  109,  107,  106,  104,  102,  100,   98,   96,   94,   92,
      90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,   60,   57,   54,   51,
      49,   46,   43,   40,   37,   34,   31,   28,   25,   22,   19,   16,   12,    9,    6,    3,
       0,   -3,   -6,   -9,  -12,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
     -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,  -81,  -83,  -85,  -88,
     -90,  -92,  -94,  -96,  -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
    -117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
    -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
    -117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100,  -98,  -96,  -94,  -92,
     -90,  -88,  -85,  -83,  -81,  -78,  -76,  -73,  -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
     -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,  -22,  -19,  -16,  -12,   -9,   -6,   -3
};

UINT8 clamp_uint8(INT16 val, UINT8 lo, UINT8 hi) NONBANKED {
    if (val < lo)
        return lo;
    if (val > hi)
        return hi;

    return (UINT8)val;
}

UINT8 sqrt_uint16(UINT16 a) NONBANKED {
    UINT16 m, y, b;
    m = 0x4000;
    y = 0;
    while (m != 0) {
        b = y | m;
        y >>= 1;
        if (a >= b) {
            a -= b;
            y |= m;
        }
        m >>= 2;
    }

    return (UINT8)y;
}

INT16 pow_int16(INT16 a, UINT8 b) NONBANKED {
    INT16 result = 1;
    while (b != 0) {
        result *= a;
        --b;
    }

    return result;
}

/**< Point and directions. */

const point8_t direction_lookup[4] = {
    { .x =  0, .y =  1 },
    { .x =  1, .y =  0 },
    { .x =  0, .y = -1 },
    { .x = -1, .y =  0 }
};
