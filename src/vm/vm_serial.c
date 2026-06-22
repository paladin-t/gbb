#pragma bank 255

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "vm_device.h"
#include "vm_serial.h"

BANKREF(VM_SERIAL)

#define SERIAL_TIMEOUT_FRAME_COUNT   300 // About 5s.

void vm_sread(SCRIPT_CTX * THIS) OLDCALL BANKED {
    const BOOLEAN wait = (BOOLEAN)*(--THIS->stack_ptr);
    receive_byte();
    if (wait) {
#if SERIAL_TIMEOUT_ENABLED
        const UINT16 started = sys_time;
        while (_io_status == IO_RECEIVING) { // Wait until received or timeout.
            if ((UINT16)(sys_time - started) >= SERIAL_TIMEOUT_FRAME_COUNT) {
                *(THIS->stack_ptr++) = SERIAL_TIMEOUT;

                return;
            }
        }
#else /* SERIAL_TIMEOUT_ENABLED */
        while (_io_status == IO_RECEIVING) { /* Wait until received. */ }
#endif /* SERIAL_TIMEOUT_ENABLED */
        if      (_io_status == IO_IDLE)      *(THIS->stack_ptr++) = _io_in;
        else                                 *(THIS->stack_ptr++) = SERIAL_ERROR;
    } else {
        if      (_io_status == IO_IDLE)      *(THIS->stack_ptr++) = _io_in;
        else if (_io_status == IO_RECEIVING) *(THIS->stack_ptr++) = SERIAL_BUSY;
        else                                 *(THIS->stack_ptr++) = SERIAL_ERROR;
    }
}

void vm_swrite(SCRIPT_CTX * THIS) OLDCALL BANKED {
    _io_out            = (UINT8)*(--THIS->stack_ptr);
    const BOOLEAN wait = (BOOLEAN)*(--THIS->stack_ptr);
    send_byte();
    if (wait) {
#if SERIAL_TIMEOUT_ENABLED
        const UINT16 started = sys_time;
        while (_io_status == IO_SENDING) { // Wait until sent or timeout.
            if ((UINT16)(sys_time - started) >= SERIAL_TIMEOUT_FRAME_COUNT) {
                *(THIS->stack_ptr++) = SERIAL_TIMEOUT;

                return;
            }
        }
#else /* SERIAL_TIMEOUT_ENABLED */
        while (_io_status == IO_SENDING) { /* Wait until sent. */ }
#endif /* SERIAL_TIMEOUT_ENABLED */
        if      (_io_status == IO_IDLE)    *(THIS->stack_ptr++) = _io_out;
        else                               *(THIS->stack_ptr++) = SERIAL_ERROR;
    } else {
        if      (_io_status == IO_IDLE)    *(THIS->stack_ptr++) = _io_out;
        else if (_io_status == IO_SENDING) *(THIS->stack_ptr++) = SERIAL_BUSY;
        else                               *(THIS->stack_ptr++) = SERIAL_ERROR;
    }
}
