#ifndef __UTILS_H__
#define __UTILS_H__

#if defined __SDCC
#   include <gbdk/platform.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

/**< Common. */

// Static is dummy when compiling is autobanked, to avoid runtime panic.
#define STATIC
#define INLINE                                     inline

#define COUNTOF(A)                                 (sizeof(A) / sizeof(*(A)))

#define SWAP_UINT16(A, B)                          do { UINT16 T = (A); (A) = (B); (B) = (T); } while (0)

#define SET_FLAG(N, F)                             ((N) |=  (F))
#define CLR_FLAG(N, F)                             ((N) &= ~(F))
#define TGL_FLAG(N, F)                             ((N) ^=  (F))
#define CHK_FLAG(N, F)                             ((N) &   (F))

/**< ROM banking. */

#define SWITCH_ROM_BANK(B)                         SWITCH_ROM(B)

#define ASSET_SOURCE_STACK                         0
#define ASSET_SOURCE_READ                          1
#define ASSET_SOURCE_DATA                          2
#define ASSET_SOURCE_FAR                           3

UINT8 get_uint8(UINT8 bank, UINT8 * ptr) NONBANKED;
void get_chunk(UINT8 * dst, UINT8 bank, UINT8 * ptr, UINT8 size) NONBANKED;
inline INT8 get_int8(UINT8 bank, UINT8 * ptr) {
    union { INT8 val; UINT8 bytes[1]; } u;
    u.bytes[0] = get_uint8(bank, ptr);

    return u.val;
}
inline INT16 get_int16(UINT8 bank, UINT8 * ptr) {
    union { INT16 val; UINT8 bytes[2]; } u;
    u.bytes[0] = get_uint8(bank, ptr);
    u.bytes[1] = get_uint8(bank, ptr + 1);

    return u.val;
}
inline INT16 get_int16_be(UINT8 bank, UINT8 * ptr) {
    union { INT16 val; UINT8 bytes[2]; } u;
    u.bytes[1] = get_uint8(bank, ptr);
    u.bytes[0] = get_uint8(bank, ptr + 1);

    return u.val;
}
inline UINT16 get_uint16(UINT8 bank, UINT8 * ptr) {
    union { UINT16 val; UINT8 bytes[2]; } u;
    u.bytes[0] = get_uint8(bank, ptr);
    u.bytes[1] = get_uint8(bank, ptr + 1);

    return u.val;
}
inline UINT16 get_uint16_be(UINT8 bank, UINT8 * ptr) {
    union { UINT16 val; UINT8 bytes[2]; } u;
    u.bytes[1] = get_uint8(bank, ptr);
    u.bytes[0] = get_uint8(bank, ptr + 1);

    return u.val;
}
inline UINT8 * get_ptr(UINT8 bank, UINT8 * ptr) {
    union { UINT8 * ptr; UINT8 bytes[2]; } u;
    u.bytes[0] = get_uint8(bank, ptr);
    u.bytes[1] = get_uint8(bank, ptr + 1);

    return u.ptr;
}
inline UINT8 * get_ptr_be(UINT8 bank, UINT8 * ptr) {
    union { UINT8 * ptr; UINT8 bytes[2]; } u;
    u.bytes[1] = get_uint8(bank, ptr);
    u.bytes[0] = get_uint8(bank, ptr + 1);

    return u.ptr;
}

typedef void (* v_bbp_fn_oldcall)(UINT8, UINT8, const UINT8 *) OLDCALL;
typedef void (* v_bbbbpb_fn_oldcall)(UINT8, UINT8, UINT8, UINT8, const UINT8 *, UINT8) OLDCALL;
typedef void (* v_www_fn)(UINT16, UINT16, UINT16);
typedef UINT8 (* b_bw_fn)(UINT8, UINT16);

void call_v_bbp_oldcall(UINT8 a, UINT8 b, UINT8 bank, UINT8 * ptr, v_bbp_fn_oldcall func) NONBANKED;
void call_v_bbbbpb_oldcall(UINT8 a, UINT8 b, UINT8 c, UINT8 d, UINT8 bank, UINT8 * ptr, UINT8 e, v_bbbbpb_fn_oldcall func) NONBANKED;
void call_v_www(UINT16 a, UINT16 b, UINT16 c, UINT8 bank, v_www_fn func) NONBANKED;
UINT8 call_b_bw(UINT8 a, UINT16 b, UINT8 bank, b_bw_fn func) NONBANKED;

/**< Features. */

#define FEATURE_AUTO_UPDATE                      0x01
#define FEATURE_MAP_MOVEMENT                     0x02
#define FEATURE_ACTOR_HIT_WITH_DETAILS           0x04
#define FEATURE_SCENE                            0x08
#define FEATURE_RTC                              0x10
#define FEATURE_EFFECT_PULSE                     0x20
#define FEATURE_EFFECT_PARALLAX                  0x40

#define FEATURE_AUTO_UPDATE_ENABLED              (feature_states &   FEATURE_AUTO_UPDATE)
#define FEATURE_AUTO_UPDATE_ENABLE               (feature_states |=  FEATURE_AUTO_UPDATE)
#define FEATURE_AUTO_UPDATE_DISABLE              (feature_states &= ~FEATURE_AUTO_UPDATE)
#define FEATURE_MAP_MOVEMENT_FLAG                (feature_states &   FEATURE_MAP_MOVEMENT)
#define FEATURE_MAP_MOVEMENT_SET                 (feature_states |=  FEATURE_MAP_MOVEMENT)
#define FEATURE_MAP_MOVEMENT_CLEAR               (feature_states &= ~FEATURE_MAP_MOVEMENT)
#define FEATURE_ACTOR_HIT_WITH_DETAILS_ENABLED   (feature_states &   FEATURE_ACTOR_HIT_WITH_DETAILS)
#define FEATURE_ACTOR_HIT_WITH_DETAILS_ENABLE    (feature_states |=  FEATURE_ACTOR_HIT_WITH_DETAILS)
#define FEATURE_ACTOR_HIT_WITH_DETAILS_DISABLE   (feature_states &= ~FEATURE_ACTOR_HIT_WITH_DETAILS)
#define FEATURE_SCENE_ENABLED                    (feature_states &   FEATURE_SCENE)
#define FEATURE_SCENE_ENABLE                     (feature_states |=  FEATURE_SCENE)
#define FEATURE_SCENE_DISABLE                    (feature_states &= ~FEATURE_SCENE)
#define FEATURE_RTC_ENABLED                      (feature_states &   FEATURE_RTC)
#define FEATURE_RTC_ENABLE                       (feature_states |=  FEATURE_RTC)
#define FEATURE_RTC_DISABLE                      (feature_states &= ~FEATURE_RTC)
#define FEATURE_EFFECT_PULSE_FLAG                (feature_states &   FEATURE_EFFECT_PULSE)
#define FEATURE_EFFECT_PULSE_SET                 (feature_states |=  FEATURE_EFFECT_PULSE)
#define FEATURE_EFFECT_PULSE_CLEAR               (feature_states &= ~FEATURE_EFFECT_PULSE)
#define FEATURE_EFFECT_PARALLAX_ENABLED          (feature_states &   FEATURE_EFFECT_PARALLAX)
#define FEATURE_EFFECT_PARALLAX_ENABLE           (feature_states |=  FEATURE_EFFECT_PARALLAX)
#define FEATURE_EFFECT_PARALLAX_DISABLE          (feature_states &= ~FEATURE_EFFECT_PARALLAX)

extern UINT8 feature_states;

/**< Timing. */

extern UINT8 game_time;

#define IS_FRAME_EVEN                            ((game_time & 0x01) == 0)
#define IS_FRAME_ODD                             ((game_time & 0x01) == 1)
#define IS_FRAME_2                               ((game_time & 0x01) == 0)
#define IS_FRAME_4                               ((game_time & 0x03) == 0)
#define IS_FRAME_8                               ((game_time & 0x07) == 0)
#define IS_FRAME_16                              ((game_time & 0x0F) == 0)
#define IS_FRAME_32                              ((game_time & 0x1F) == 0)
#define IS_FRAME_64                              ((game_time & 0x3F) == 0)
#define IS_FRAME_128                             ((game_time & 0x7F) == 0)
#define IS_FRAME_256                             ((game_time & 0xFF) == 0)

/**< Math. */

#define MUL2(A)                                  ((A) << 1)
#define MUL4(A)                                  ((A) << 2)
#define MUL8(A)                                  ((A) << 3)
#define MUL16(A)                                 ((A) << 4)
#define MUL32(A)                                 ((A) << 5)
#define MUL64(A)                                 ((A) << 6)
#define MUL128(A)                                ((A) << 7)

#define DIV2(A)                                  ((A) >> 1)
#define DIV4(A)                                  ((A) >> 2)
#define DIV8(A)                                  ((A) >> 3)
#define DIV16(A)                                 ((A) >> 4)
#define DIV32(A)                                 ((A) >> 5)
#define DIV64(A)                                 ((A) >> 6)
#define DIV128(A)                                ((A) >> 7)

#define MOD2(A)                                  ((A) & 0x01)
#define MOD4(A)                                  ((A) & 0x03)
#define MOD8(A)                                  ((A) & 0x07)
#define MOD16(A)                                 ((A) & 0x0F)
#define MOD32(A)                                 ((A) & 0x1F)

#define SIGN(A)                                  ((A) > 0 ? 1 : ((A) == 0 ? 0 : -1))
#define MIN(A, B)                                (((A) < (B)) ? (A) : (B))
#define MAX(A, B)                                (((A) > (B)) ? (A) : (B))
#define CLAMP(A, LO, HI)                         (((A) < (LO)) ? (LO) : (((A) > (HI)) ? (HI) : (A)))

#define SIN(A)                                   (sine_wave[(UINT8)(A)])
#define COS(A)                                   (sine_wave[(UINT8)((UINT8)(A) + 64u)])

extern const INT8 sine_wave[256];

UINT8 atan2(INT16 y, INT16 x) BANKED;

UINT8 clamp_uint8(INT16 val, UINT8 lo, UINT8 hi) NONBANKED;
UINT8 sqrt_uint16(UINT16 a) NONBANKED;
INT16 pow_int16(INT16 a, INT16 b) NONBANKED;

/**< Point and directions. */

#define DIRECTION_NONE                           8
#define DIRECTION_DOWN                           0
#define DIRECTION_RIGHT                          1
#define DIRECTION_UP                             2
#define DIRECTION_LEFT                           3
#define DIRECTION_COUNT                          4

#define FLIP_DIRECTION(DIR)                      MOD4((DIR) + 2)
#define IS_DIRECTION_HORIZONTAL(DIR)             ((DIR) & 0x01)
#define IS_DIRECTION_VERTICAL(DIR)               (!((DIR) & 0x01))

#define ANGLE_UP                                 0
#define ANGLE_RIGHT                              64
#define ANGLE_DOWN                               128
#define ANGLE_LEFT                               192

#define ANGLE_0DEG                               0
#define ANGLE_45DEG                              32
#define ANGLE_90DEG                              64
#define ANGLE_135DEG                             96
#define ANGLE_180DEG                             128
#define ANGLE_225DEG                             160
#define ANGLE_270DEG                             192
#define ANGLE_315DEG                             224

#define TO_SCREEN(A)                             DIV16(A)
#define FROM_SCREEN(A)                           MUL16(A)

#define TO_SCREEN_TILE(A)                        DIV128(A)
#define FROM_SCREEN_TILE(A)                      MUL128(A)

#define IS_ON_8PX_GRID(A, OX, OY)                (MOD8(TO_SCREEN((A).x)) == (OX) && MOD8(TO_SCREEN((A).y)) == (OY))
#define IS_ON_16PX_GRID(A, OX, OY)               (MOD16(TO_SCREEN((A).x)) == (OX) && MOD16(TO_SCREEN((A).y)) == (OY))

typedef struct point8_t {
    INT8 x;
    INT8 y;
} point8_t;

typedef struct point16_t {
    INT16 x;
    INT16 y;
} point16_t;

typedef struct upoint16_t {
    UINT16 x;
    UINT16 y;
} upoint16_t;

extern const point8_t direction_lookup[4];

inline void point_translate_dir(upoint16_t * point, UINT8 dir, UINT8 speed) {
    point->x += (INT16)(direction_lookup[dir].x * speed);
    point->y += (INT16)(direction_lookup[dir].y * speed);
}
inline void point_translate_dir_uint16(upoint16_t * point, UINT8 dir, UINT16 speed) {
    point->x += (INT16)(direction_lookup[dir].x * speed);
    point->y += (INT16)(direction_lookup[dir].y * speed);
}
inline void point_translate_angle(upoint16_t * point, UINT8 angle, UINT8 speed) {
    point->x += DIV128(SIN(angle) * speed);
    point->y -= DIV128(COS(angle) * speed);
}
inline void point_translate_angle_to_delta(point16_t * point, UINT8 angle, UINT8 speed) {
    point->x = DIV128(SIN(angle) * speed);
    point->y = DIV128(COS(angle) * speed);
}
inline void vector_translate_angle(point16_t * vector, UINT8 angle, UINT8 speed) {
    vector->x = DIV128(SIN(angle) * speed);
    vector->y = DIV128(COS(angle) * speed);
}

/**< Properties. */

// Basic properties.
#define PROPERTY_PALETTE                         1
#define PROPERTY_HFLIP                           2
#define PROPERTY_VFLIP                           3
#define PROPERTY_PRIORITY                        4
#define PROPERTY_BANK                            5
#define PROPERTY_COLOR                           6
#define PROPERTY_HIDDEN                          7

// Advanced properties.
#define PROPERTY_ACTIVE                          8
#define PROPERTY_ENABLED                         9
#define PROPERTY_PINNED                          10
#define PROPERTY_PERSISTENT                      11
#define PROPERTY_FOLLOWING                       12
#define PROPERTY_ANIMATION_LOOP                  13
#define PROPERTY_ANIMATION_PAUSED                14
#define PROPERTY_MOVEMENT_INTERRUPT              15
#define PROPERTY_POSITION                        16
#   define PROPERTY_POSITION_X                   17
#   define PROPERTY_POSITION_Y                   18
#define PROPERTY_DIRECTION                       19
#define PROPERTY_ANGLE                           20
#define PROPERTY_BOUNDS                          21
#   define PROPERTY_BOUNDS_LEFT                  22
#   define PROPERTY_BOUNDS_RIGHT                 23
#   define PROPERTY_BOUNDS_TOP                   24
#   define PROPERTY_BOUNDS_BOTTOM                25
#define PROPERTY_BASE_TILE                       26
#define PROPERTY_FRAMES                          27
#   define PROPERTY_FRAME_INDEX                  28
#define PROPERTY_ANIMATION_INTERVAL              29
#define PROPERTY_ANIMATIONS                      30
#   define PROPERTY_ANIMATION                    31
#   define PROPERTY_ANIMATION_INDEX              32
#define PROPERTY_MOVE_SPEED                      33
#define PROPERTY_BEHAVIOUR                       34
#define PROPERTY_COLLISION_GROUP                 35

#define PROPERTY_STRONG                          36
#define PROPERTY_LIFE_TIME                       37
#define PROPERTY_INITIAL_OFFSET                  38

#define PROPERTY_IS_16x16_GRID                   39
#define PROPERTY_IS_16x16_PLAYER                 40
#define PROPERTY_CLAMP_CAMERA                    41
#define PROPERTY_GRAVITY                         42
#define PROPERTY_JUMP_GRAVITY                    43
#define PROPERTY_JUMP_MAX_COUNT                  44
#define PROPERTY_JUMP_MAX_TICKS                  45
#define PROPERTY_CLIMB_VELOCITY                  46
#define PROPERTY_SIZE                            47
#   define PROPERTY_SIZE_WIDTH                   48
#   define PROPERTY_SIZE_HEIGHT                  49
#define PROPERTY_CAMERA_DEADZONE                 50
#   define PROPERTY_CAMERA_DEADZONE_X            51
#   define PROPERTY_CAMERA_DEADZONE_Y            52
#define PROPERTY_BLOCKING                        53
#   define PROPERTY_BLOCKING_X                   54
#       define PROPERTY_BLOCKING_LEFT            55
#       define PROPERTY_BLOCKING_RIGHT           56
#   define PROPERTY_BLOCKING_Y                   57
#       define PROPERTY_BLOCKING_UP              58
#       define PROPERTY_BLOCKING_DOWN            59

/**< Types. */

#define OBJECT_TYPE_ACTOR                        0x01
#define OBJECT_TYPE_PROJECTILE                   0x02

/**< Events. */

// Events types.
#define EVENT_HANDLER_GOTO                       0x00
#define EVENT_HANDLER_GOSUB                      0x40
#define EVENT_HANDLER_START                      0x80

// Event operations.
#define EVENT_HANDLER_TARGET(S)                  ((S) & 0xC0)
#define EVENT_HANDLER_TYPE(S)                    ((S) & 0x3F)
#define EVENT_HANDLER_IS_GOTO(S)                 (((S) & 0xC0) == EVENT_HANDLER_GOTO)
#define EVENT_HANDLER_IS_GOSUB(S)                ((S) & EVENT_HANDLER_GOSUB)
#define EVENT_HANDLER_IS_START(S)                ((S) & EVENT_HANDLER_START)

// Enter/leave, change and confirm events.
#define EVENT_HANDLER_ENTER                      0x01
#define EVENT_HANDLER_LEAVE                      0x02
#define EVENT_HANDLER_CHANGE                     0x04
#define EVENT_HANDLER_CONFIRM                    0x08

/**< Exceptions. */

#define EXCEPTION_UNKNOWN_PARAMETER              1

#define EXCEPTION_DEVICE_ERROR                   1
#define EXCEPTION_ACTOR_ERROR                    2
#define EXCEPTION_PROJECTILE_ERROR               3
#define EXCEPTION_SCENE_ERROR                    4

/**< Animation. */

typedef struct animation_t {
    UINT8 begin;
    UINT8 end;
} animation_t;

/**< Collision. */

typedef struct boundingbox_t {
    INT8 left;
    INT8 right;
    INT8 top;
    INT8 bottom;
} boundingbox_t;

inline BOOLEAN boundingbox_contains_point(const boundingbox_t * bb, const upoint16_t * point) {
    if ((point->x < bb->left) || (point->x > bb->right))
        return FALSE;
    if ((point->y < bb->top) || (point->y > bb->bottom))
        return FALSE;

    return TRUE;
}
inline BOOLEAN boundingbox_contains(const boundingbox_t * bb, const upoint16_t * offset, const upoint16_t * point) {
    if ((point->x < offset->x + bb->left) || (point->x > offset->x + bb->right))
        return FALSE;
    if ((point->y < offset->y + bb->top) || (point->y > offset->y + bb->bottom))
        return FALSE;

    return TRUE;
}
inline BOOLEAN boundingbox_intersects(const boundingbox_t * bb_a, const upoint16_t * offset_a, const boundingbox_t * bb_b, const upoint16_t * offset_b) {
    if ((offset_b->x + bb_b->left > offset_a->x + bb_a->right) || (offset_b->x + bb_b->right < offset_a->x + bb_a->left))
        return FALSE;
    if ((offset_b->y + bb_b->top > offset_a->y + bb_a->bottom) || (offset_b->y + bb_b->bottom < offset_a->y + bb_a->top))
        return FALSE;

    return TRUE;
}

#endif /* __UTILS_H__ */
