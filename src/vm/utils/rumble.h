#ifndef __RUMBLE_H__
#define __RUMBLE_H__

#if defined __SDCC
#   include <gb/gb.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#define MBC_RAM_BANK_0       0u

#define MBC_RUMBLE_BIT_ON    0b00001000u
#define MBC_RUMBLE_BIT_OFF   0b00000000u

#define MBC5_RUMBLE_ON       SWITCH_RAM_MBC5(MBC_RAM_BANK_0 | MBC_RUMBLE_BIT_ON)
#define MBC5_RUMBLE_OFF      SWITCH_RAM_MBC5(MBC_RAM_BANK_0 | MBC_RUMBLE_BIT_OFF)

inline void rumble_start(UINT8 mask) {
    if (sys_time & mask)
        MBC5_RUMBLE_ON;
    else
        MBC5_RUMBLE_OFF;
}
inline void rumble_stop(void) {
    MBC5_RUMBLE_OFF;
}

#endif /* __RUMBLE_H__ */
