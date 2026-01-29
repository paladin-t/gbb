;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Bytecode.
; Calling convention:
;         args: Big-endian
;         order: Left-to-right (leftmost argument pushed first)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Aliases.

        .ARG0                   = -1
        .ARG1                   = -2
        .ARG2                   = -3
        .ARG3                   = -4
        .ARG4                   = -5
        .ARG5                   = -6
        .ARG6                   = -7
        .ARG7                   = -8
        .ARG8                   = -9
        .ARG9                   = -10
        .ARG10                  = -11
        .ARG11                  = -12
        .ARG12                  = -13
        .ARG13                  = -14
        .ARG14                  = -15
        .ARG15                  = -16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Basic instructions.

; Halt and nop.

OP_VM_HALT                      = 0x00
.macro VM_HALT
        .db OP_VM_HALT 
.endm

OP_VM_NOP                       = 0x01
.macro VM_NOP
        .db OP_VM_NOP
.endm

; Stack manipulation.

OP_VM_RESERVE                   = 0x02
.macro VM_RESERVE ARG0
        .db OP_VM_RESERVE, #<ARG0
.endm

OP_VM_PUSH                      = 0x03
.macro VM_PUSH ARG0
        .db OP_VM_PUSH, #>ARG0, #<ARG0
.endm

OP_VM_PUSH_VALUE                = 0x04
.macro VM_PUSH_VALUE ARG0
        .db OP_VM_PUSH_VALUE, #>ARG0, #<ARG0
.endm

OP_VM_POP                       = 0x05
.macro VM_POP ARG0
        .db OP_VM_POP, #<ARG0
.endm

OP_VM_POP_1                     = 0x06
.macro VM_POP_1 ARG0
        .db OP_VM_POP_1, #<ARG0
.endm

; Invoking.

OP_VM_CALL                      = 0x07
.macro VM_CALL ARG0
        .db OP_VM_CALL, #>ARG0, #<ARG0
.endm

OP_VM_RET                       = 0x08
.macro VM_RET
        .db OP_VM_RET, 0
.endm
.macro VM_RET_N ARG0
        .db OP_VM_RET, #<ARG0
.endm

OP_VM_CALL_FAR                  = 0x09
.macro VM_CALL_FAR BANK, ADDR
        .db OP_VM_CALL_FAR, #>ADDR, #<ADDR, #<BANK
.endm

OP_VM_RET_FAR                   = 0x0A
.macro VM_RET_FAR
        .db OP_VM_RET_FAR, 0
.endm
.macro VM_RET_FAR_N ARG0
        .db OP_VM_RET_FAR, #<ARG0
.endm

; Jump, conditional and loop.

OP_VM_JUMP                      = 0x0B
.macro VM_JUMP ARG0
        .db OP_VM_JUMP, #>ARG0, #<ARG0
.endm

OP_VM_JUMP_FAR                  = 0x0C
.macro VM_JUMP_FAR BANK, ADDR
        .db OP_VM_JUMP_FAR, #>ADDR, #<ADDR, #<BANK
.endm

OP_VM_NEXT_BANK                 = 0x0D
.macro VM_NEXT_BANK
        .db OP_VM_NEXT_BANK
.endm

OP_VM_SWITCH                    = 0x0E
.macro VM_SWITCH IDX, SIZE, N, CALL
        .db OP_VM_SWITCH, #<CALL, #<N, #<SIZE, #>IDX, #<IDX
.endm
.macro .CASE BANK, ADDR, VAL
        .dw #>VAL, #<VAL, #>ADDR, #<ADDR, #<BANK
.endm

OP_VM_IF                        = 0x0F
        .EQ                     = 1
        .LT                     = 2
        .LE                     = 3
        .GT                     = 4
        .GE                     = 5
        .NE                     = 6
.macro VM_IF CONDITION, IDXA, IDXB, ADDR, N
        .db OP_VM_IF, #<N, #>ADDR, #<ADDR, #>IDXB, #<IDXB, #>IDXA, #<IDXA, #<CONDITION
.endm

OP_VM_IF_CONST                  = 0x10
.macro VM_IF_CONST CONDITION, IDXA, B, ADDR, N
        .db OP_VM_IF_CONST, #<N, #>ADDR, #<ADDR, #>B, #<B, #>IDXA, #<IDXA, #<CONDITION
.endm

OP_VM_IIF                       = 0x11
.macro VM_IIF IDXR, IDXC, IDXA, IDXB
        .db OP_VM_IIF, #>IDXB, #<IDXB, #>IDXA, #<IDXA, #>IDXC, #<IDXC, #>IDXR, #<IDXR
.endm

OP_VM_LOOP                      = 0x12
.macro VM_LOOP IDXA, B, C, ADDR, NPOP
        .db OP_VM_LOOP, #<NPOP, #>ADDR, #<ADDR, #>C, #<C, #>B, #<B, #>IDXA, #<IDXA
.endm

OP_VM_FOR                       = 0x13
.macro VM_FOR IDXA, IDXB, IDXC, ADDR, NPOP
        .db OP_VM_FOR, #<NPOP, #>ADDR, #<ADDR, #>IDXC, #<IDXC, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

; Data and value.

OP_VM_SET                       = 0x14
.macro VM_SET IDXA, IDXB
        .db OP_VM_SET, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_SET_CONST                 = 0x15
.macro VM_SET_CONST IDX, VAL
        .db OP_VM_SET_CONST, #>VAL, #<VAL, #>IDX, #<IDX
.endm

OP_VM_SET_INDIRECT              = 0x16
.macro VM_SET_INDIRECT IDXA, IDXB
        .db OP_VM_SET_INDIRECT, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_GET_INDIRECT              = 0x17
.macro VM_GET_INDIRECT IDXA, IDXB
        .db OP_VM_GET_INDIRECT, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_ACC                       = 0x18
.macro VM_ACC IDXA, IDXB, NPOP
        .db OP_VM_ACC, #<NPOP, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_ACC_CONST                 = 0x19
.macro VM_ACC_CONST IDXA, VAL
        .db OP_VM_ACC_CONST, #>VAL, #<VAL, #>IDXA, #<IDXA
.endm

OP_VM_RPN                       = 0x1A
        ; .EQ                   = 1
        ; .LT                   = 2
        ; .LE                   = 3
        ; .GT                   = 4
        ; .GE                   = 5
        ; .NE                   = 6
        .AND                    = 7
        .OR                     = 8
        .NOT                    = 9
        .ADD                    = 10
        .SUB                    = 11
        .MUL                    = 12
        .DIV                    = 13
        .MOD                    = 14
        .BAND                   = 15
        .BOR                    = 16
        .BXOR                   = 17
        .SHL                    = 18
        .SHR                    = 19
        .BNOT                   = 20
        .NEG                    = 21
        .SGN                    = 22
        .ABS                    = 23
        .SQR                    = 24
        .SQRT                   = 25
        .SIN                    = 26
        .COS                    = 27
        .ATAN2                  = 28
        .POWI                   = 29
        .MIN                    = 30
        .MAX                    = 31
.macro VM_RPN
        .db OP_VM_RPN
.endm
.macro .R_INT8 ARG0
        .db 32, #<ARG0
.endm
.macro .R_INT16 ARG0
        .db 33
        .dw #ARG0
.endm
.macro .R_REF ARG0
        .db 34
        .dw #ARG0
.endm
.macro .R_OPERATOR ARG0
        .db ARG0
.endm
.macro .R_STOP
        .db 0
.endm

OP_VM_GET_TLOCAL                = 0x1B
.macro VM_GET_TLOCAL IDXA, IDXB
        .db OP_VM_GET_TLOCAL, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_SET_TLOCAL                = 0x1C
.macro VM_SET_TLOCAL IDXA, IDXB
        .db OP_VM_SET_TLOCAL, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_PACK                      = 0x1D
.macro VM_PACK IDX, IDX0, IDX1, IDX2, IDX3, N
        .db OP_VM_PACK, #<N, #>IDX3, #<IDX3, #>IDX2, #<IDX2, #>IDX1, #<IDX1, #>IDX0, #<IDX0, #>IDX, #<IDX
.endm

OP_VM_UNPACK                    = 0x1E
.macro VM_UNPACK IDX, IDX0, IDX1, IDX2, IDX3, N
        .db OP_VM_UNPACK, #<N, #>IDX3, #<IDX3, #>IDX2, #<IDX2, #>IDX1, #<IDX1, #>IDX0, #<IDX0, #>IDX, #<IDX
.endm

OP_VM_SWAP                      = 0x1F
.macro VM_SWAP IDXA, IDXB
        .db OP_VM_SWAP, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_DATA_PTR                  = 0x20
.macro VM_DATA_PTR IDX, FORB
        .db OP_VM_DATA_PTR, #<FORB, #>IDX, #<IDX
.endm

OP_VM_READ                      = 0x21
.macro VM_READ IDX, SIZE, COPY
        .db OP_VM_READ, #<COPY, #>SIZE, #<SIZE, #>IDX, #<IDX
.endm

OP_VM_RESTORE                   = 0x22
.macro VM_RESTORE BANK, IDX
        .db OP_VM_RESTORE, #>IDX, #<IDX, #<BANK
.endm

; Thread manipulation.

OP_VM_BEGIN_THREAD              = 0x23
.macro VM_BEGIN_THREAD BANK, THREADPROC, PUT, HTHREAD, NARGS
        .db OP_VM_BEGIN_THREAD, #<NARGS, #>HTHREAD, #<HTHREAD, #<PUT, #>THREADPROC, #<THREADPROC, #<BANK
.endm

OP_VM_JOIN                      = 0x24
.macro VM_JOIN IDX
        .db OP_VM_JOIN, #>IDX, #<IDX
.endm

OP_VM_TERMINATE                 = 0x25
.macro VM_TERMINATE IDX, ALL
        .db OP_VM_TERMINATE, #<ALL, #>IDX, #<IDX
.endm

OP_VM_WAIT                      = 0x26
.macro VM_WAIT
        .db OP_VM_WAIT
.endm

OP_VM_WAIT_N                    = 0x27
.macro VM_WAIT_N IDX
        .db OP_VM_WAIT_N, #>IDX, #<IDX
.endm

OP_VM_LOCK                      = 0x28
.macro VM_LOCK
        .db OP_VM_LOCK
.endm

OP_VM_UNLOCK                    = 0x29
.macro VM_UNLOCK
        .db OP_VM_UNLOCK
.endm

; Functions.

OP_VM_INVOKE_FN                 = 0x2A
.macro VM_INVOKE_FN BANK, ADDR, N, IDX
        .db OP_VM_INVOKE_FN, #>IDX, #<IDX, #<N, #>ADDR, #<ADDR, #<BANK
.endm

OP_VM_LOCATE                    = 0x2B
.macro VM_LOCATE IDXA, IDXB
        .db OP_VM_LOCATE, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_PRINT                     = 0x2C
.macro VM_PRINT NEWLN, N
        .db OP_VM_PRINT, #<N, #<NEWLN
.endm

OP_VM_SRAND                     = 0x2D
.macro VM_SRAND IDX
        .db OP_VM_SRAND, #>IDX, #<IDX
.endm
.macro VM_RANDOMIZE
        VM_RESERVE              2
        VM_GET_UINT8            .ARG0, _DIV_REG
        VM_GET_UINT8            .ARG1, _sys_time
        VM_RPN
            .R_INT16            256
            .R_OPERATOR         .MUL
            .R_OPERATOR         .ADD
            .R_STOP
        VM_SRAND                .ARG0
        VM_POP                  1
.endm

OP_VM_RAND                      = 0x2E
.macro VM_RAND IDX, IDXMIN, IDXMAX
        .db OP_VM_RAND
        .db #>IDXMAX, #<IDXMAX, #>IDXMIN, #<IDXMIN, #>IDX, #<IDX
.endm

OP_VM_PEEK                      = 0x2F
.macro VM_PEEK IDXA, IDXB, WORD
        .db OP_VM_PEEK, #<WORD, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_POKE                      = 0x30
.macro VM_POKE IDXA, IDXB, WORD
        .db OP_VM_POKE, #<WORD, #>IDXB, #<IDXB, #>IDXA, #<IDXA
.endm

OP_VM_FILL                      = 0x31
.macro VM_FILL DEST, VALUE, COUNT
        .db OP_VM_FILL, #>COUNT, #<COUNT, #>VALUE, #<VALUE, #>DEST, #<DEST
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Memory instructions.

; Memory.

OP_VM_MEMCPY                    = 0x32
.macro VM_MEMCPY SRC
        .db OP_VM_MEMCPY, #<SRC
.endm

OP_VM_MEMSET                    = 0x33
.macro VM_MEMSET
        .db OP_VM_MEMSET
.endm

OP_VM_MEMADD                    = 0x34
.macro VM_MEMADD
        .db OP_VM_MEMADD
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; System instructions.

; System.

OP_VM_GET_SYS_TIME              = 0x35
.macro VM_GET_SYS_TIME IDX
        .db OP_VM_GET_SYS_TIME, #>IDX, #<IDX
.endm

OP_VM_SLEEP                     = 0x36
.macro VM_SLEEP IDX
        .db OP_VM_SLEEP, #>IDX, #<IDX
.endm

OP_VM_RAISE                     = 0x37
.macro VM_RAISE CODE, SIZE
        .db OP_VM_RAISE, #<SIZE, #<CODE
.endm

OP_VM_RESET                     = 0x38
.macro VM_RESET
        .db OP_VM_RESET
.endm

OP_VM_DBGINFO                   = 0x39
.macro VM_DBGINFO
        .db OP_VM_DBGINFO
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Graphics instructions.

; Primitives.

OP_VM_COLOR                     = 0x3A
.macro VM_COLOR
        .db OP_VM_COLOR
.endm

OP_VM_PALETTE                   = 0x3B
.macro VM_PALETTE NARGS
        .db OP_VM_PALETTE, #<NARGS
.endm

OP_VM_RGB                       = 0x3C
.macro VM_RGB
        .db OP_VM_RGB
.endm

OP_VM_PLOT                      = 0x3D
.macro VM_PLOT
        .db OP_VM_PLOT
.endm

OP_VM_POINT                     = 0x3E
.macro VM_POINT
        .db OP_VM_POINT
.endm

OP_VM_LINE                      = 0x3F
.macro VM_LINE
        .db OP_VM_LINE
.endm

OP_VM_RECT                      = 0x40
.macro VM_RECT FILL
        .db OP_VM_RECT, #<FILL
.endm

OP_VM_GOTOXY                    = 0x41
.macro VM_GOTOXY
        .db OP_VM_GOTOXY
.endm

OP_VM_TEXT                      = 0x42
.macro VM_TEXT NEWLN, NARGS
        .db OP_VM_TEXT, #<NARGS, #<NEWLN
.endm

; Image operation.

OP_VM_IMAGE                     = 0x43
.macro VM_IMAGE SRC
        .db OP_VM_IMAGE, #<SRC
.endm

; Tile operation.

OP_VM_FILL_TILE                 = 0x44
.macro VM_FILL_TILE SRC, LAYER
        .db OP_VM_FILL_TILE, #<LAYER, #<SRC
.endm

; Map operation.

OP_VM_DEF_MAP                   = 0x45
.macro VM_DEF_MAP SRC
        .db OP_VM_DEF_MAP, #<SRC
.endm

OP_VM_MAP                       = 0x46
.macro VM_MAP
        .db OP_VM_MAP
.endm

OP_VM_MGET                      = 0x47
.macro VM_MGET
        .db OP_VM_MGET
.endm

OP_VM_MSET                      = 0x48
.macro VM_MSET
        .db OP_VM_MSET
.endm

; Window operation.

OP_VM_DEF_WINDOW                = 0x49
.macro VM_DEF_WINDOW SRC
        .db OP_VM_DEF_WINDOW, #<SRC
.endm

OP_VM_WINDOW                    = 0x4A
.macro VM_WINDOW
        .db OP_VM_WINDOW
.endm

OP_VM_WGET                      = 0x4B
.macro VM_WGET
        .db OP_VM_WGET
.endm

OP_VM_WSET                      = 0x4C
.macro VM_WSET
        .db OP_VM_WSET
.endm

; Sprite operation.

OP_VM_DEF_SPRITE                = 0x4D
.macro VM_DEF_SPRITE SRC
        .db OP_VM_DEF_SPRITE, #<SRC
.endm

OP_VM_SPRITE                    = 0x4E
.macro VM_SPRITE
        .db OP_VM_SPRITE
.endm

OP_VM_SGET                      = 0x4F
.macro VM_SGET
        .db OP_VM_SGET
.endm

OP_VM_SSET                      = 0x50
.macro VM_SSET
        .db OP_VM_SSET
.endm

OP_VM_GET_SPRITE_PROP           = 0x51
.macro VM_GET_SPRITE_PROP
        .db OP_VM_GET_SPRITE_PROP
.endm

OP_VM_SET_SPRITE_PROP           = 0x52
.macro VM_SET_SPRITE_PROP
        .db OP_VM_SET_SPRITE_PROP
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Audio instructions.

; Music.

OP_VM_PLAY                      = 0x53
.macro VM_PLAY BANK, ADDR
        .db OP_VM_PLAY, #>ADDR, #<ADDR, #<BANK
.endm

OP_VM_STOP                      = 0x54
.macro VM_STOP
        .db OP_VM_STOP
.endm

; SFX.

OP_VM_SOUND                     = 0x55
.macro VM_SOUND BANK, ADDR, PRI, N
        .db OP_VM_SOUND, #<N, #<PRI, #>ADDR, #<ADDR, #<BANK
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Input instructions.

; Gamepad.

OP_VM_BTN                       = 0x56
.macro VM_BTN
        .db OP_VM_BTN
.endm

OP_VM_BTND                      = 0x57
.macro VM_BTND
        .db OP_VM_BTND
.endm

OP_VM_BTNU                      = 0x58
.macro VM_BTNU
        .db OP_VM_BTNU
.endm

; Touch.

OP_VM_TOUCH                     = 0x59
.macro VM_TOUCH LOC, IDXX, IDXY
        .db OP_VM_TOUCH, #>IDXY, #<IDXY, #>IDXX, #<IDXX, #<LOC
.endm

OP_VM_TOUCHD                    = 0x5A
.macro VM_TOUCHD LOC, IDXX, IDXY
        .db OP_VM_TOUCHD, #>IDXY, #<IDXY, #>IDXX, #<IDXX, #<LOC
.endm

OP_VM_TOUCHU                    = 0x5B
.macro VM_TOUCHU LOC, IDXX, IDXY
        .db OP_VM_TOUCHU, #>IDXY, #<IDXY, #>IDXX, #<IDXX, #<LOC
.endm

; Callback.

OP_VM_ON_INPUT                  = 0x5C
.macro VM_ON_INPUT BANK, PROC, OPTION
        .db OP_VM_ON_INPUT, #<OPTION, #>PROC, #<PROC, #<BANK
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Persistence instructions.

; File.

OP_VM_FOPEN                     = 0x5D
.macro VM_FOPEN
        .db OP_VM_FOPEN
.endm

OP_VM_FCLOSE                    = 0x5E
.macro VM_FCLOSE
        .db OP_VM_FCLOSE
.endm

OP_VM_FREAD                     = 0x5F
.macro VM_FREAD
        .db OP_VM_FREAD
.endm

OP_VM_FWRITE                    = 0x60
.macro VM_FWRITE
        .db OP_VM_FWRITE
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Serial port instructions.

; Serial port.

OP_VM_SREAD                     = 0x61
.macro VM_SREAD
        .db OP_VM_SREAD
.endm

OP_VM_SWRITE                    = 0x62
.macro VM_SWRITE
        .db OP_VM_SWRITE
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Scene instructions.

; Camera and viewport.

OP_VM_CAMERA                    = 0x63
.macro VM_CAMERA
        .db OP_VM_CAMERA
.endm

OP_VM_VIEWPORT                  = 0x64
.macro VM_VIEWPORT IDXX, IDXY
        .db OP_VM_VIEWPORT, #>IDXY, #<IDXY, #>IDXX, #<IDXX
.endm

; Scene initialization.

OP_VM_DEF_SCENE                 = 0x65
.macro VM_DEF_SCENE SRC
        .db OP_VM_DEF_SCENE, #<SRC
.endm

OP_VM_LOAD_SCENE                = 0x66
.macro VM_LOAD_SCENE
        .db OP_VM_LOAD_SCENE
.endm

; Scene property.

OP_VM_GET_SCENE_PROP            = 0x67
.macro VM_GET_SCENE_PROP
        .db OP_VM_GET_SCENE_PROP
.endm

OP_VM_SET_SCENE_PROP            = 0x68
.macro VM_SET_SCENE_PROP
        .db OP_VM_SET_SCENE_PROP
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Actor instructions.

; Actor constructor/destructor.

OP_VM_NEW_ACTOR                 = 0x69
.macro VM_NEW_ACTOR
        .db OP_VM_NEW_ACTOR
.endm

OP_VM_DEL_ACTOR                 = 0x6A
.macro VM_DEL_ACTOR
        .db OP_VM_DEL_ACTOR
.endm

; Actor initialization.

OP_VM_DEF_ACTOR                 = 0x6B
.macro VM_DEF_ACTOR SRC
        .db OP_VM_DEF_ACTOR, #<SRC
.endm

; Actor property.

OP_VM_GET_ACTOR_PROP            = 0x6C
.macro VM_GET_ACTOR_PROP
        .db OP_VM_GET_ACTOR_PROP
.endm

OP_VM_SET_ACTOR_PROP            = 0x6D
.macro VM_SET_ACTOR_PROP SRC
        .db OP_VM_SET_ACTOR_PROP, #<SRC
.endm

; Actor animation.

OP_VM_PLAY_ACTOR                = 0x6E
.macro VM_PLAY_ACTOR
        .db OP_VM_PLAY_ACTOR
.endm

; Actor threading.

OP_VM_THREAD_ACTOR              = 0x6F
.macro VM_THREAD_ACTOR OP
        .db OP_VM_THREAD_ACTOR, #<OP
.endm

; Actor motion.

OP_VM_MOVE_ACTOR                = 0x70
.macro VM_MOVE_ACTOR OP
        .db OP_VM_MOVE_ACTOR, #<OP
.endm

; Actor lookup.

OP_VM_FIND_ACTOR                = 0x71
.macro VM_FIND_ACTOR OPTION
        .db OP_VM_FIND_ACTOR, #<OPTION
.endm

; Actor callback.

OP_VM_ON_ACTOR                  = 0x72
.macro VM_ON_ACTOR BANK, PROC, OPTION
        .db OP_VM_ON_ACTOR, #<OPTION, #>PROC, #<PROC, #<BANK
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Emote instructions.

; Emote.

OP_VM_EMOTE                     = 0x73
.macro VM_EMOTE SRC
        .db OP_VM_EMOTE, #<SRC
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Projectile instructions.

; Projectile initialization.

OP_VM_DEF_PROJECTILE            = 0x74
.macro VM_DEF_PROJECTILE SRC
        .db OP_VM_DEF_PROJECTILE, #<SRC
.endm

OP_VM_START_PROJECTILE          = 0x75
.macro VM_START_PROJECTILE OP
        .db OP_VM_START_PROJECTILE, #<OP
.endm

; Projectile property.

OP_VM_GET_PROJECTILE_PROP       = 0x76
.macro VM_GET_PROJECTILE_PROP
        .db OP_VM_GET_PROJECTILE_PROP
.endm

OP_VM_SET_PROJECTILE_PROP       = 0x77
.macro VM_SET_PROJECTILE_PROP SRC
        .db OP_VM_SET_PROJECTILE_PROP, #<SRC
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Trigger instructions.

; Trigger initialization.

OP_VM_DEF_TRIGGER               = 0x78
.macro VM_DEF_TRIGGER SRC
        .db OP_VM_DEF_TRIGGER, #<SRC
.endm

; Trigger callback.

OP_VM_ON_TRIGGER                = 0x79
.macro VM_ON_TRIGGER BANK, ADDR
        .db OP_VM_ON_TRIGGER, #>ADDR, #<ADDR, #<BANK
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Object instructions.

; Object typing.

OP_VM_OBJECT_IS                 = 0x7A
.macro VM_OBJECT_IS TYPE
        .db OP_VM_OBJECT_IS, #<TYPE
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GUI instructions.

; GUI widget initialization.

OP_VM_DEF_WIDGET                = 0x7B
.macro VM_DEF_WIDGET TYPE
        .db OP_VM_DEF_WIDGET, #<TYPE
.endm

; Widgets.

OP_VM_LABEL                     = 0x7C
.macro VM_LABEL NEWLN, NARGS, FBANK, FADDR
        .db OP_VM_LABEL, #>FADDR, #<FADDR, #<FBANK, #<NARGS, #<NEWLN
.endm

OP_VM_PROGRESSBAR               = 0x7D
.macro VM_PROGRESSBAR NARGS
        .db OP_VM_PROGRESSBAR, #<NARGS
.endm

OP_VM_MENU                      = 0x7E
.macro VM_MENU NLNS, NARGS, FBANK, FPTR
        .db OP_VM_MENU, #>FPTR, #<FPTR, #<FBANK, #<NARGS, #<NLNS
.endm

; GUI callback.

OP_VM_ON_WIDGET                 = 0x7F
.macro VM_ON_WIDGET BANK, PROC
        .db OP_VM_ON_WIDGET, #>PROC, #<PROC, #<BANK
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Scroll instructions.

; Scroll.

OP_VM_SCROLL                    = 0x80
.macro VM_SCROLL SHAPES
        .db OP_VM_SCROLL, #<SHAPES
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Effects instructions.

; Effects.

OP_VM_FX                        = 0x81
.macro VM_FX
        .db OP_VM_FX
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Physics instructions.

; Collision detection.

OP_VM_HITS                      = 0x82
.macro VM_HITS SHAPES
        .db OP_VM_HITS, #<SHAPES
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Game instructions.

; Game loop.

OP_VM_UPDATE                    = 0x83
.macro VM_UPDATE
        .db OP_VM_UPDATE
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Hardware features instructions.

; Feature switch and query.

OP_VM_OPTION                    = 0x84
.macro VM_OPTION
        .db OP_VM_OPTION
.endm

OP_VM_QUERY                     = 0x85
.macro VM_QUERY
        .db OP_VM_QUERY
.endm

; Streaming.

OP_VM_STREAM                    = 0x86
.macro VM_STREAM STATUS
        .db OP_VM_STREAM, #<STATUS
.endm

; Shell.

OP_VM_SHELL                     = 0x87
.macro VM_SHELL NARGS
        .db OP_VM_SHELL, #<NARGS
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
