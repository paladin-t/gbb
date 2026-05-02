#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#if defined __SDCC
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include "platformer.h"
#include "pointnclick.h"
#include "topdown.h"
#if defined USE_SHOOTING
#   include "shooting.h"
#endif /* USE_SHOOTING */

#define CONTROLLER_ALWAYS_BEHAVE                                 0x80
#define CONTROLLER_RIGIDLY_BEHAVE                                0x40
#define CONTROLLER_BEHAVIOUR_OPTIONS                            (CONTROLLER_ALWAYS_BEHAVE | CONTROLLER_RIGIDLY_BEHAVE)

#define CONTROLLER_BEHAVIOUR_NONE                                0x00
#define CONTROLLER_BEHAVIOUR_PLATFORMER_PLAYER                   0x01
#define CONTROLLER_BEHAVIOUR_PLATFORMER_MOVE                     0x02
#define CONTROLLER_BEHAVIOUR_PLATFORMER_IDLE                     0x03
#define CONTROLLER_BEHAVIOUR_TOPDOWN_PLAYER                      0x04
#   define CONTROLLER_BEHAVIOUR_TOPDOWN_PLAYER_ARBITRARY         0x05
#define CONTROLLER_BEHAVIOUR_TOPDOWN_MOVE                        0x06
#   define CONTROLLER_BEHAVIOUR_TOPDOWN_MOVE_ARBITRARY           0x07
#define CONTROLLER_BEHAVIOUR_TOPDOWN_IDLE                        0x08
#define CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER                  0x09
#   define CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_MOUSE   (0x0A | CONTROLLER_ALWAYS_BEHAVE)
#   define CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_TOUCH   (0x0B | CONTROLLER_ALWAYS_BEHAVE)
#if defined USE_SHOOTING
#   define CONTROLLER_BEHAVIOUR_SHOOTING_PLAYER                  0x0C
#   define CONTROLLER_BEHAVIOUR_SHOOTING_MOVE                    0x0D
#   define CONTROLLER_BEHAVIOUR_SHOOTING_IDLE                    0x0E
#endif /* USE_SHOOTING */

#define CONTROLLER_MOVABLE_FLAG_NONE                             0x00
#define CONTROLLER_MOVABLE_FLAG_COLLISIONS                       0x01
#define CONTROLLER_MOVABLE_FLAG_FULL                             0x02

#define CONTROLLER_NEGATIVE_SPEED_OF(A)                         ((A)->move_speed >= FROM_SCREEN(1) ? TO_SCREEN(-(A)->move_speed) : -1)
#define CONTROLLER_POSITIVE_SPEED_OF(A)                         ((A)->move_speed >= FROM_SCREEN(1) ? TO_SCREEN((A)->move_speed) : 1)

INLINE BOOLEAN controller_behave_actor(actor_t * actor, UINT8 bhvr) {
    switch (bhvr & ~CONTROLLER_BEHAVIOUR_OPTIONS) {
    // Platformer.
    case CONTROLLER_BEHAVIOUR_PLATFORMER_PLAYER:
        return controller_behave_platformer_player(actor);

    case CONTROLLER_BEHAVIOUR_PLATFORMER_MOVE:
        return controller_behave_platformer_move(actor);

    case CONTROLLER_BEHAVIOUR_PLATFORMER_IDLE:
        return controller_behave_platformer_idle(actor);

    // Top-down.
    case CONTROLLER_BEHAVIOUR_TOPDOWN_PLAYER:
        return controller_behave_topdown_player(actor);
    case CONTROLLER_BEHAVIOUR_TOPDOWN_PLAYER_ARBITRARY:
        return controller_behave_topdown_player_arbitrary(actor, bhvr & CONTROLLER_RIGIDLY_BEHAVE);

    case CONTROLLER_BEHAVIOUR_TOPDOWN_MOVE:
        return controller_behave_topdown_move(actor);
    case CONTROLLER_BEHAVIOUR_TOPDOWN_MOVE_ARBITRARY:
        return controller_behave_topdown_move_arbitrary(actor, bhvr & CONTROLLER_RIGIDLY_BEHAVE);

    case CONTROLLER_BEHAVIOUR_TOPDOWN_IDLE:
        return controller_behave_topdown_idle(actor);

    // Point & Click.
    case CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER:
        return controller_behave_pointnclick_player(actor, CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER);
    case CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_MOUSE & ~CONTROLLER_BEHAVIOUR_OPTIONS:
        return controller_behave_pointnclick_player(actor, CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_MOUSE);
    case CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_TOUCH & ~CONTROLLER_BEHAVIOUR_OPTIONS:
        return controller_behave_pointnclick_player(actor, CONTROLLER_BEHAVIOUR_POINTNCLICK_PLAYER_WITH_TOUCH);

    // Scroll Shooting.
#if defined USE_SHOOTING
    case CONTROLLER_BEHAVIOUR_SHOOTING_PLAYER:
        return controller_behave_shooting_player(actor);
    case CONTROLLER_BEHAVIOUR_SHOOTING_MOVE:
        return controller_behave_shooting_move(actor);
    case CONTROLLER_BEHAVIOUR_SHOOTING_IDLE:
        return controller_behave_shooting_idle(actor);
#endif /* USE_SHOOTING */

    // Others.
    default:
        // Do nothing.
        return FALSE;
    }
}

BOOLEAN controller_is_actor_movable(actor_t * actor, INT8 dx, INT8 dy, UINT8 flags) BANKED;

BOOLEAN controller_is_actor_toward_another(actor_t * actor, actor_t * other) BANKED;

#endif /* __CONTROLLER_H__ */
