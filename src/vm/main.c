#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "utils/exception.h"
#include "utils/utils.h"

#include "vm.h"
#include "vm_game.h"

extern const UINT8 BOOTSTRAP[];
BANKREF_EXTERN(BOOTSTRAP)

inline void setup(void) {
    script_runner_init();
    script_execute(BANK(BOOTSTRAP), BOOTSTRAP, NULL, 0);
}

inline void update(void) {
    while (TRUE) {
        switch (script_runner_update()) {
        case RUNNER_DONE: // Fall through.
        case RUNNER_IDLE:
            if (FEATURE_AUTO_UPDATE_ENABLED)
                game_update();

            vsync();

            break;
        case RUNNER_BUSY:
            break;
#if VM_EXCEPTION_ENABLED
        case RUNNER_EXCEPTION:
            exception_handle_vm_raised();

            return;
#endif /* VM_EXCEPTION_ENABLED */
        default:
            break;
        }
    }
}

void main(void) {
    setup();
    update();
}
