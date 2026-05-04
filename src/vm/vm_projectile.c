#pragma bank 255

#if defined __SDCC
#   include <gb/drawing.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "vm_actor.h"
#include "vm_projectile.h"
#include "vm_scene.h"

BANKREF(VM_PROJECTILE)

#if USE_PROJECTILE

#define PROJECTILE_START_INDEPENDENTLY   0x00
#define PROJECTILE_START_WITH_ACTOR      0x01
#define PROJECTILE_START_ON_ACTOR        0x02

// Seealso: <gb/metasprites.h>.
#define PROJECTILE_ALLOCATED             (actor_hardware_sprite_count)
#define PROJECTILE_ALLOCATE(N)           (actor_hardware_sprite_count += (N))
#if ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED
STATIC void PROJECTILE_MOVE(projectile_t * projectile, UINT8 x, UINT8 y) NONBANKED {
    if (!projectile->def.sprite_bank) return;
    const UINT8 _save = CURRENT_BANK;
    SWITCH_ROM_BANK(projectile->def.sprite_bank);
    PROJECTILE_ALLOCATE(
        move_metasprite(
            *(projectile->def.sprite_frames + projectile->frame),
            projectile->def.base_tile,
            PROJECTILE_ALLOCATED,
            x, y
        )
    );
    SWITCH_ROM_BANK(_save);
}
#else /* ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED */
#define PROJECTILE_MOVE_(PROJECTILE, X, Y) \
    do { \
        if (!(PROJECTILE)->def.sprite_bank) break; \
        __current_metasprite = (metasprite_t *)get_ptr( \
            (PROJECTILE)->def.sprite_bank, (UINT8 *)(PROJECTILE)->def.sprite_frames + \
                sizeof(metasprite_t *) * (PROJECTILE)->frame \
        ); \
        __current_base_tile = (PROJECTILE)->def.base_tile; \
        __current_base_prop = 0; \
        PROJECTILE_ALLOCATE( \
            call_b_bw( \
                PROJECTILE_ALLOCATED, \
                ((Y) << 8) | (UINT8)(X), \
                (PROJECTILE)->def.sprite_bank, \
                (b_bw_fn)__move_metasprite \
            ) \
        ); \
    } while (0)
#endif /* ACTOR_IMPLEMENT_MOVE_WITH_FUNCTION_ENABLED */

typedef union projectile_index_t {
    projectile_t * ptr;
    UINT16 def;
    UINT16 value;
} projectile_index_t;

projectile_def_t projectile_defs[PROJECTILE_DEF_MAX_COUNT];
projectile_t projectiles[PROJECTILE_MAX_COUNT];
projectile_t * projectile_active_head;
static projectile_t * projectile_inactive_head;

STATIC projectile_def_t * projectile_def_of(projectile_index_t obj) {
    return (obj.value < PROJECTILE_DEF_MAX_COUNT) ?
        &projectile_defs[obj.def] :
        &obj.ptr->def;
}

STATIC void projectile_begin_actor_hit_thread(actor_t * actor, projectile_t * other) {
    if (actor->hit_handler_bank == 0)
        return;
    if (actor->hit_thread_id != 0 && !VM_IS_TERMINATED(actor->hit_thread_id))
        return;

    SCRIPT_CTX * ctx = script_execute(actor->hit_handler_bank, actor->hit_handler_address, &actor->hit_thread_id, 0);
    if (!ctx)
        return;

    if (FEATURE_ACTOR_HIT_WITH_DETAILS_ENABLED) {
        UINT8 dirx = DIRECTION_NONE, diry = DIRECTION_NONE;
        UINT16 dx = 0, dy = 0;
        if (actor->position.x < other->position.x) {
            dirx = DIRECTION_RIGHT;
            dx = other->position.x - actor->position.x;
        } else if (actor->position.x > other->position.x) {
            dirx = DIRECTION_LEFT;
            dx = actor->position.x - other->position.x;
        }
        if (actor->position.y < other->position.y) {
            diry = DIRECTION_DOWN;
            dy = other->position.y - actor->position.y;
        } else if (actor->position.y > other->position.y) {
            diry = DIRECTION_UP;
            dy = actor->position.y - other->position.y;
        }
        const UINT8 dir = (dx > dy + FROM_SCREEN(1)) ? dirx : diry;
        *(ctx->stack_ptr++) = (UINT16)dir; // Initialize thread local with the hit direction.
        *(ctx->stack_ptr++) = (UINT16)other->def.collision_group; // Initialize thread local with the hit target's collision group.
    }
    *(ctx->stack_ptr++) = (UINT16)other; // Initialize thread locals with the projectile's handle.
    *(ctx->stack_ptr++) = (UINT16)actor; // Initialize thread locals with the actor's handle.
}

void projectile_init(void) BANKED {
    memset(projectile_defs, 0, sizeof(projectile_defs));
    memset(projectiles, 0, sizeof(projectiles));
    projectile_active_head   = NULL;
    projectile_inactive_head = NULL;

    for (projectile_t * proj = projectiles; proj < projectiles + PROJECTILE_MAX_COUNT; ++proj) {
        LL_PUSH_HEAD(projectile_inactive_head, proj);
    }
}

void projectile_update(void) BANKED {
    // Prepare.
    projectile_t * next;

    projectile_t * projectile = projectile_active_head;
    projectile_t * prev_projectile = NULL;

    // Update the projectiles.
    while (projectile) {
        // Remove the projectile if it's dead.
        if (projectile->def.life_time == 0) {
            next = projectile->next;
            LL_REMOVE_ITEM(projectile_active_head, projectile, prev_projectile);
            LL_PUSH_HEAD(projectile_inactive_head, projectile);
            projectile = next;

            continue;
        }
        --projectile->def.life_time;

        // Check whether the projectile has reached animation tick frame.
        if ((game_time & projectile->def.animation_interval) == 0) {
            const UINT8 frame_end = projectile->def.animations[projectile->animation].end + 1;
            if (++projectile->frame == frame_end) { // Check whether the projectile has reached end of animation.
                if (!projectile->animation_no_loop) {
                    const UINT8 frame_begin = projectile->def.animations[projectile->animation].begin;
                    projectile->frame = frame_begin;
                } else {
                    --projectile->frame;
                }
            }
        }

        // Move the projectile.
        projectile->position.x += projectile->movement.x;
        projectile->position.y -= projectile->movement.y;

        // Check collision.
        if (IS_FRAME_EVEN) {
            actor_t * hit_actor = actor_hits_in_group(
                &projectile->def.bounds, &projectile->position,
                projectile->def.collision_group // Same collision group.
            );
            if (hit_actor) {
                projectile_begin_actor_hit_thread(hit_actor, projectile);
                if (!projectile->strong) { // Remove the projectile.
                    next = projectile->next;
                    LL_REMOVE_ITEM(projectile_active_head, projectile, prev_projectile);
                    LL_PUSH_HEAD(projectile_inactive_head, projectile);
                    projectile = next;

                    continue;
                }
            }
        }

        const UINT8 x = TO_SCREEN(projectile->position.x) - scene_camera_x;
        const UINT8 y = TO_SCREEN(projectile->position.y) - scene_camera_y;

        if (x > GRAPHICS_WIDTH || y - 8 > GRAPHICS_HEIGHT) { // Out of screen, remove the projectile.
            projectile_t * next = projectile->next;
            LL_REMOVE_ITEM(projectile_active_head, projectile, prev_projectile);
            LL_PUSH_HEAD(projectile_inactive_head, projectile);
            projectile = next;

            continue;
        }

        PROJECTILE_MOVE(projectile, x, y);

        prev_projectile = projectile;
        projectile = projectile->next;
    }
}

void projectile_render(void) BANKED {
    projectile_t * projectile = projectile_active_head;
    projectile_t * prev_projectile = NULL;

    while (projectile) {
        const UINT8 x = TO_SCREEN(projectile->position.x) - scene_camera_x;
        const UINT8 y = TO_SCREEN(projectile->position.y) - scene_camera_y;

        if (x > GRAPHICS_WIDTH || y - 8 > GRAPHICS_HEIGHT) { // Out of the screen, remove the projectile.
            projectile_t * next = projectile->next;
            LL_REMOVE_ITEM(projectile_active_head, projectile, prev_projectile);
            LL_PUSH_HEAD(projectile_inactive_head, projectile);
            projectile = next;

            continue;
        }

        PROJECTILE_MOVE(projectile, x, y);

        prev_projectile = projectile;
        projectile = projectile->next;
    }
}

UINT8 projectile_active_count(void) BANKED {
    UINT8 ret = 0;
    projectile_t * projectile = projectile_active_head;

    while (projectile) {
        ++ret;
        projectile = projectile->next;
    }

    return ret;
}

UINT8 projectile_free_count(void) BANKED {
    UINT8 ret = 0;
    projectile_t * projectile = projectile_inactive_head;

    while (projectile) {
        ++ret;
        projectile = projectile->next;
    }

    return ret;
}

UINT8 projectile_def(UINT8 type, UINT8 base_tile, UINT8 bank, UINT8 * ptr) BANKED {
    projectile_def_t * projectile_def = &projectile_defs[type];
    const UINT8 * old = ptr;
    projectile_def->bounds.left             = get_int8  (bank, ptr++);
    projectile_def->bounds.right            = get_int8  (bank, ptr++);
    projectile_def->bounds.top              = get_int8  (bank, ptr++);
    projectile_def->bounds.bottom           = get_int8  (bank, ptr++);
    projectile_def->base_tile               = base_tile;
    projectile_def->sprite_bank             = get_uint8 (bank, ptr++);
    projectile_def->sprite_frames           = get_ptr   (bank, ptr  ); ptr += 2;
    projectile_def->animation_interval      = get_uint8 (bank, ptr++);
    for (UINT8 i = 0; i != PROJECTILE_MAX_ANIMATIONS; ++i) {
        projectile_def->animations[i].begin = get_uint8 (bank, ptr++);
        projectile_def->animations[i].end   = get_uint8 (bank, ptr++);
    }
    projectile_def->life_time               = get_uint8 (bank, ptr++);
    projectile_def->move_speed              = get_uint8 (bank, ptr++);
    projectile_def->initial_offset          = get_uint16(bank, ptr  ); ptr += 2;
    projectile_def->collision_group         = get_uint8 (bank, ptr++);

    return (UINT8)(ptr - old);
}

projectile_t * projectile_start(UINT8 index, UINT16 x, UINT16 y, UINT8 angle, UINT8 flags) BANKED {
    projectile_t * projectile = projectile_inactive_head;
    if (projectile) {
        memcpy(&projectile->def, &projectile_defs[index], sizeof(projectile_def_t));

        // Set the correct projectile frames based on angle.
        UINT8 dir = DIRECTION_UP;
        if (angle <= 224) {
            if (angle >= 160)
                dir = DIRECTION_LEFT;
            else if (angle > 96)
                dir = DIRECTION_DOWN;
            else if (angle >= 32)
                dir = DIRECTION_RIGHT;
        }

        // Set the projectile flags.
        projectile->animation_no_loop = flags & PROJECTILE_FLAG_ANIMATION_NO_LOOP;
        projectile->strong            = flags & PROJECTILE_FLAG_STRONG;

        // Set the animation.
        projectile->frame     = projectile->def.animations[dir].begin;
        projectile->animation = dir;

        // Set the coordinates.
        UINT16 initial_offset = projectile->def.initial_offset;
        projectile->position.x = x;
        projectile->position.y = y;

        const INT8 sinv = SIN(angle);
        const INT8 cosv = COS(angle);

        // Set the offset by initial amount.
        while (initial_offset > 0xFFu) {
            projectile->position.x += DIV128(sinv * (UINT8)0xFF);
            projectile->position.y -= DIV128(cosv * (UINT8)0xFF);
            initial_offset -= 0xFFu;
        }
        if (initial_offset > 0) {
            projectile->position.x += DIV128(sinv * (UINT8)initial_offset);
            projectile->position.y -= DIV128(cosv * (UINT8)initial_offset);
        }

        point_translate_angle_to_delta(&projectile->movement, angle, projectile->def.move_speed);

        LL_REMOVE_HEAD(projectile_inactive_head);
        LL_PUSH_HEAD(projectile_active_head, projectile);
    }

    return projectile;
}

void vm_def_projectile(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    const UINT8 type       = (UINT8)*(--THIS->stack_ptr);
    const UINT16 base_tile = (UINT16)*(--THIS->stack_ptr);
    UINT8 bank;
    UINT8 * ptr;
    UINT8 size             = 0;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        size = projectile_def(type, base_tile, bank, ptr);
        current_data_address += size;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        size = projectile_def(type, base_tile, bank, ptr);
        THIS->PC += size;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        projectile_def(type, base_tile, bank, ptr);

        break;
    default:
        // Do nothing.

        break;
    }
}

void vm_start_projectile(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED {
    const UINT8 type  = (UINT8)*(--THIS->stack_ptr);
    UINT16 x          = (UINT16)*(--THIS->stack_ptr);
    UINT16 y          = (UINT16)*(--THIS->stack_ptr);
    UINT8 angle       = (UINT8)*(--THIS->stack_ptr);
    const UINT8 flags = (UINT8)*(--THIS->stack_ptr);
    switch (op) {
    case PROJECTILE_START_INDEPENDENTLY:
        // Do nothing.

        break;
    case PROJECTILE_START_WITH_ACTOR: {
            actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
            x += TO_SCREEN(actor->position.x);
            y += TO_SCREEN(actor->position.y);
            if (actor->direction == DIRECTION_UP) {
                // Do nothing.
            } else if (actor->direction == DIRECTION_RIGHT) {
                angle += 64;
            } else if (actor->direction == DIRECTION_DOWN) {
                angle += 128;
            } else if (actor->direction == DIRECTION_LEFT) {
                angle += 192;
            }
        }

        break;
    case PROJECTILE_START_ON_ACTOR: {
            actor_t * actor = (actor_t *)*(--THIS->stack_ptr);
            x += TO_SCREEN(actor->position.x);
            y += TO_SCREEN(actor->position.y);
        }

        break;
    }

    projectile_t * ret = projectile_start(type, FROM_SCREEN(x), FROM_SCREEN(y), angle, flags);

    *(THIS->stack_ptr++) = (UINT16)ret;
}

void vm_get_projectile_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    projectile_index_t obj;
    obj.value     = (UINT16)*(--THIS->stack_ptr);
    const UINT8 p = (UINT8)*(--THIS->stack_ptr);
    switch (p) {
    // Definition properties.
    case PROPERTY_BOUNDS_LEFT:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->bounds.left;

        break;
    case PROPERTY_BOUNDS_RIGHT:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->bounds.right;

        break;
    case PROPERTY_BOUNDS_TOP:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->bounds.top;

        break;
    case PROPERTY_BOUNDS_BOTTOM:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->bounds.bottom;

        break;
    case PROPERTY_BASE_TILE:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->base_tile;

        break;
    case PROPERTY_ANIMATION_INTERVAL:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->animation_interval;

        break;
    case PROPERTY_LIFE_TIME:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->life_time;

        break;
    case PROPERTY_MOVE_SPEED:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->move_speed;

        break;
    case PROPERTY_INITIAL_OFFSET:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->initial_offset;

        break;
    case PROPERTY_COLLISION_GROUP:
        *(THIS->stack_ptr++) = projectile_def_of(obj)->collision_group;

        break;

    // Instance properties.
    case PROPERTY_ANIMATION_LOOP:
        *(THIS->stack_ptr++) = !obj.ptr->animation_no_loop;

        break;
    case PROPERTY_STRONG:
        *(THIS->stack_ptr++) = obj.ptr->strong;

        break;
    case PROPERTY_POSITION_X:
        *(THIS->stack_ptr++) = TO_SCREEN(obj.ptr->position.x);

        break;
    case PROPERTY_POSITION_Y:
        *(THIS->stack_ptr++) = TO_SCREEN(obj.ptr->position.y);

        break;
    case PROPERTY_FRAME_INDEX:
        *(THIS->stack_ptr++) = obj.ptr->frame;

        break;
    case PROPERTY_ANIMATION_INDEX:
        *(THIS->stack_ptr++) = obj.ptr->animation;

        break;
    default:
        *(THIS->stack_ptr++) = FALSE;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_PROJECTILE_ERROR, p);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}

INLINE void projectile_get_animations(SCRIPT_CTX * THIS, UINT8 src, UINT8 * bank_, UINT8 ** ptr_, UINT8 * base_, UINT8 * count_) {
    UINT8 bank;
    UINT8 * ptr;
    UINT8 base;
    UINT8 count;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank  = current_data_bank;
        ptr   = (UINT8 *)current_data_address;
        base  = (UINT8)*(--THIS->stack_ptr);
        count = (UINT8)*(--THIS->stack_ptr);
        current_data_address += MUL2(count);

        break;
    case ASSET_SOURCE_DATA:
        bank  = THIS->bank;
        ptr   = (UINT8 *)THIS->PC;
        base  = get_uint8(bank, ptr++); ++THIS->PC;
        count = get_uint8(bank, ptr++); ++THIS->PC;
        THIS->PC += MUL2(count);

        break;
    case ASSET_SOURCE_FAR:
        bank  = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr   = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        base  = (UINT8)*(--THIS->stack_ptr);
        count = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        bank  = 0;
        ptr   = NULL;
        base  = 0;
        count = 0;

        break;
    }

    *bank_    = bank;
    *ptr_     = ptr;
    *base_    = base;
    *count_   = count;
}

INLINE void projectile_get_animation(SCRIPT_CTX * THIS, UINT8 src, UINT8 * bank_, UINT8 ** ptr_, UINT8 * base_) {
    UINT8 bank;
    UINT8 * ptr;
    UINT8 base;

    switch (src) {
    case ASSET_SOURCE_READ:
        bank = current_data_bank;
        ptr  = (UINT8 *)current_data_address;
        base = (UINT8)*(--THIS->stack_ptr);
        current_data_address += 2;

        break;
    case ASSET_SOURCE_DATA:
        bank = THIS->bank;
        ptr  = (UINT8 *)THIS->PC;
        base = get_uint8(bank, ptr++); ++THIS->PC;
        THIS->PC += 2;

        break;
    case ASSET_SOURCE_FAR:
        bank = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
        ptr  = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
        base = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        bank = 0;
        ptr  = NULL;
        base = 0;

        break;
    }

    *bank_   = bank;
    *ptr_    = ptr;
    *base_   = base;
}

void vm_set_projectile_prop(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    projectile_index_t obj;
    obj.value     = (UINT16)*(--THIS->stack_ptr);
    const UINT8 p = (UINT8)*(--THIS->stack_ptr);
    switch (p) {
    // Definition properties.
    case PROPERTY_BOUNDS:
        projectile_def_of(obj)->bounds.left   = (INT8)*(--THIS->stack_ptr);
        projectile_def_of(obj)->bounds.right  = (INT8)*(--THIS->stack_ptr);
        projectile_def_of(obj)->bounds.top    = (INT8)*(--THIS->stack_ptr);
        projectile_def_of(obj)->bounds.bottom = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_LEFT:
        projectile_def_of(obj)->bounds.left = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_RIGHT:
        projectile_def_of(obj)->bounds.right = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_TOP:
        projectile_def_of(obj)->bounds.top = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BOUNDS_BOTTOM:
        projectile_def_of(obj)->bounds.bottom = (INT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_BASE_TILE:
        projectile_def_of(obj)->base_tile = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_FRAMES: {
            const UINT8 bank                      = get_uint8 (THIS->bank, (UINT8 *)THIS->PC++);
            const UINT8 * ptr                     = get_ptr_be(THIS->bank, (UINT8 *)THIS->PC);   THIS->PC += 2;
            projectile_def_of(obj)->base_tile     = (UINT8)*(--THIS->stack_ptr);
            projectile_def_of(obj)->sprite_bank   = bank;
            projectile_def_of(obj)->sprite_frames = (metasprite_t **)ptr;
        }

        break;
    case PROPERTY_ANIMATION_INTERVAL:
        projectile_def_of(obj)->animation_interval = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_ANIMATIONS: {
            UINT8 bank;
            UINT8 * ptr;
            UINT8 base;
            UINT8 count;
            projectile_get_animations(THIS, src, &bank, &ptr, &base, &count);
            for (UINT8 i = base; i < base + count; ++i) {
                projectile_def_of(obj)->animations[i].begin = get_uint8(bank, ptr++);
                projectile_def_of(obj)->animations[i].end   = get_uint8(bank, ptr++);
            }
            for (UINT8 i = 0; i < base; ++i) {
                projectile_def_of(obj)->animations[i].begin = 0;
                projectile_def_of(obj)->animations[i].end   = 0;
            }
            for (UINT8 i = base + count; i < PROJECTILE_MAX_ANIMATIONS; ++i) {
                projectile_def_of(obj)->animations[i].begin = 0;
                projectile_def_of(obj)->animations[i].end   = 0;
            }
        }

        break;
    case PROPERTY_ANIMATION: {
            UINT8 bank;
            UINT8 * ptr;
            UINT8 base;
            projectile_get_animation(THIS, src, &bank, &ptr, &base);
            projectile_def_of(obj)->animations[base].begin = get_uint8(bank, ptr++);
            projectile_def_of(obj)->animations[base].end   = get_uint8(bank, ptr++);
        }

        break;
    case PROPERTY_LIFE_TIME:
        projectile_def_of(obj)->life_time = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_MOVE_SPEED:
        projectile_def_of(obj)->move_speed = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_INITIAL_OFFSET:
        projectile_def_of(obj)->initial_offset = (UINT16)*(--THIS->stack_ptr);

        break;
    case PROPERTY_COLLISION_GROUP:
        projectile_def_of(obj)->collision_group = (UINT8)*(--THIS->stack_ptr);

        break;

    // Instance properties.
    case PROPERTY_ANIMATION_LOOP:
        obj.ptr->animation_no_loop = !(bool)*(--THIS->stack_ptr);

        break;
    case PROPERTY_STRONG:
        obj.ptr->strong = (bool)*(--THIS->stack_ptr);

        break;
    case PROPERTY_POSITION:
        obj.ptr->position.x = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));
        obj.ptr->position.y = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));

        break;
    case PROPERTY_POSITION_X:
        obj.ptr->position.x = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));

        break;
    case PROPERTY_POSITION_Y:
        obj.ptr->position.y = FROM_SCREEN((UINT16)*(--THIS->stack_ptr));

        break;
    case PROPERTY_FRAME_INDEX:
        obj.ptr->frame = (UINT8)*(--THIS->stack_ptr);

        break;
    case PROPERTY_ANIMATION_INDEX:
        obj.ptr->animation = (UINT8)*(--THIS->stack_ptr);

        break;
    default:
        --THIS->stack_ptr;

#if VM_EXCEPTION_ENABLED
        VM_THROW(EXCEPTION_UNKNOWN_PARAMETER, EXCEPTION_PROJECTILE_ERROR, p);
#endif /* VM_EXCEPTION_ENABLED */

        break;
    }
}

#else /* USE_PROJECTILE */

void vm_def_projectile(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    (void)THIS;
    (void)src;

    // Do nothing.
}

void vm_start_projectile(SCRIPT_CTX * THIS, UINT8 op) OLDCALL BANKED {
    (void)THIS;
    (void)op;

    // Do nothing.
}

void vm_get_projectile_prop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    (void)THIS;

    // Do nothing.
}

void vm_set_projectile_prop(SCRIPT_CTX * THIS, UINT8 src) OLDCALL BANKED {
    (void)THIS;
    (void)src;

    // Do nothing.
}

#endif /* USE_PROJECTILE */
