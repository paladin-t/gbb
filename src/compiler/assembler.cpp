/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "assembler.h"
#include "../utils/platform.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Utilities
*/

namespace GBBASIC {

// See: https://gbdev.io/gb-opcodes/optables/
//
//   n8 : immediate 8 bit data.
//   n16: immediate 16 bit data.
//   a8 : 8 bit unsigned data, which are added to $FF00 in certain instructions (replacement for missing IN and OUT instructions).
//   a16: 16 bit address.
//   e8 : 8 bit signed data, which are added to program counter.
//
//  `ld a, (hl+)` has the alternative mnemonics `ld a, (hli)` and `ldi a, (hl)`.
//  `ld (hl+), a` has the alternative mnemonics `ld (hli), a` and `ldi (hl), a`.
//  `ld a, (hl-)` has the alternative mnemonics `ld a, (hld)` and `ldd a, (hl)`.
//  `ld (hl-), a` has the alternative mnemonics `ld (hld), a` and `ldd (hl), a`.
//  `ld hl, sp+e8` has the alternative mnemonics `ldhl sp, e8`.
//  ALU instructions (`add`, `adc`, `sub`, `sbc`, `and`, `xor`, `or`, and `cp`) can be written with the left-hand side `a` omitted.
//  Thus for example `add a, b` has the alternative mnemonic `add b`, and `cp a, 0xf` has the alternative mnemonic `cp 0xf`.

static constexpr const char* const ASSEMBLER_OPCODE_MNEMONIC[256] = {
	/*       x0            x1           x2            x3           x4             x5           x6            x7           x8             x9           xA            xB         xC            xD          xE            xF      */
	/* 0x */ "nop",        "ld bc,n16", "ld (bc),a",  "inc bc",    "inc b",       "dec b",     "ld b,n8",    "rlca",      "ld (a16),sp", "add hl,bc", "ld a,(bc)",  "dec bc",  "inc c",      "dec c",    "ld c,n8",    "rrca",
	/* 1x */ "stop",       "ld de,n16", "ld (de),a",  "inc de",    "inc d",       "dec d",     "ld d,n8",    "rla",       "jr e8",       "add hl,de", "ld a,(de)",  "dec de",  "inc e",      "dec e",    "ld e,n8",    "rra",
	/* 2x */ "jr nz,e8",   "ld hl,n16", "ld (hl+),a", "inc hl",    "inc h",       "dec h",     "ld h,n8",    "daa",       "jr z,e8",     "add hl,hl", "ld a,(hl+)", "dec hl",  "inc l",      "dec l",    "ld l,n8",    "cpl",
	/* 3x */ "jr nc,e8",   "ld sp,n16", "ld (hl-),a", "inc sp",    "inc (hl)",    "dec (hl)",  "ld (hl),n8", "scf",       "jr c,e8",     "add hl,sp", "ld a,(hl-)", "dec sp",  "inc a",      "dec a",    "ld a,n8",    "ccf",
	/* 4x */ "ld b,b",     "ld b,c",    "ld b,d",     "ld b,e",    "ld b,h",      "ld b,l",    "ld b,(hl)",  "ld b,a",    "ld c,b",      "ld c,c",    "ld c,d",     "ld c,e",  "ld c,h",     "ld c,l",   "ld c,(hl)",  "ld c,a",
	/* 5x */ "ld d,b",     "ld d,c",    "ld d,d",     "ld d,e",    "ld d,h",      "ld d,l",    "ld d,(hl)",  "ld d,a",    "ld e,b",      "ld e,c",    "ld e,d",     "ld e,e",  "ld e,h",     "ld e,l",   "ld e,(hl)",  "ld e,a",
	/* 6x */ "ld h,b",     "ld h,c",    "ld h,d",     "ld h,e",    "ld h,h",      "ld h,l",    "ld h,(hl)",  "ld h,a",    "ld l,b",      "ld l,c",    "ld l,d",     "ld l,e",  "ld l,h",     "ld l,l",   "ld l,(hl)",  "ld l,a",
	/* 7x */ "ld (hl),b",  "ld (hl),c", "ld (hl),d",  "ld (hl),e", "ld (hl),h",   "ld (hl),l", "halt",       "ld (hl),a", "ld a,b",      "ld a,c",    "ld a,d",     "ld a,e",  "ld a,h",     "ld a,l",   "ld a,(hl)",  "ld a,a",
	/* 8x */ "add a,b",    "add a,c",   "add a,d",    "add a,e",   "add a,h",     "add a,l",   "add a,(hl)", "add a,a",   "adc a,b",     "adc a,c",   "adc a,d",    "adc a,e", "adc a,h",    "adc a,l",  "adc a,(hl)", "adc a,a",
	/* 9x */ "sub a,b",    "sub a,c",   "sub a,d",    "sub a,e",   "sub a,h",     "sub a,l",   "sub a,(hl)", "sub a,a",   "sbc a,b",     "sbc a,c",   "sbc a,d",    "sbc a,e", "sbc a,h",    "sbc a,l",  "sbc a,(hl)", "sbc a,a",
	/* Ax */ "and a,b",    "and a,c",   "and a,d",    "and a,e",   "and a,h",     "and a,l",   "and a,(hl)", "and a,a",   "xor a,b",     "xor a,c",   "xor a,d",    "xor a,e", "xor a,h",    "xor a,l",  "xor a,(hl)", "xor a,a",
	/* Bx */ "or a,b",     "or a,c",    "or a,d",     "or a,e",    "or a,h",      "or a,l",    "or a,(hl)",  "or a,a",    "cp a,b",      "cp a,c",    "cp a,d",     "cp a,e",  "cp a,h",     "cp a,l",   "cp a,(hl)",  "cp a,a",
	/* Cx */ "ret nz",     "pop bc",    "jp nz,a16",  "jp a16",    "call nz,a16", "push bc",   "add a,n8",   "rst 00H",   "ret z",       "ret",       "jp z,a16",   nullptr,   "call z,a16", "call a16", "adc a,n8",   "rst 08H",
	/* Dx */ "ret nc",     "pop de",    "jp nc,a16",  nullptr,     "call nc,a16", "push de",   "sub a,n8",   "rst 10H",   "ret c",       "reti",      "jp c,a16",   nullptr,   "call c,a16", nullptr,    "sbc a,n8",   "rst 18H",
	/* Ex */ "ldh (a8),a", "pop hl",    "ld (c),a",   nullptr,     nullptr,       "push hl",   "and a,n8",   "rst 20H",   "add sp,e8",   "jp hl",     "ld (a16),a", nullptr,   nullptr,      nullptr,    "xor a,n8",   "rst 28H",
	/* Fx */ "ldh a,(a8)", "pop af",    "ld a,(c)",   "di",        nullptr,       "push af",   "or a,n8",    "rst 30H",   "ld hl,spr8",  "ld sp,hl",  "ld a,(a16)", "ei",      nullptr,      nullptr,    "cp a,n8",    "rst 38H"
};
static constexpr const char* const ASSEMBLER_CB_OPCODE_MNEMONIC[256] = {
	/*       x0         x1         x2         x3         x4         x5         x6            x7         x8         x9         xA         xB         xC         xD         xE            xF      */
	/* 0x */ "rlc b",   "rlc c",   "rlc d",   "rlc e",   "rlc h",   "rlc l",   "rlc (hl)",   "rlc a",   "rrc b",   "rrc c",   "rrc d",   "rrc e",   "rrc h",   "rrc l",   "rrc (hl)",   "rrc a",
	/* 1x */ "rl b",    "rl c",    "rl d",    "rl e",    "rl h",    "rl l",    "rl (hl)",    "rl a",    "rr b",    "rr c",    "rr d",    "rr e",    "rr h",    "rr l",    "rr (hl)",    "rr a",
	/* 2x */ "sla b",   "sla c",   "sla d",   "sla e",   "sla h",   "sla l",   "sla (hl)",   "sla a",   "sra b",   "sra c",   "sra d",   "sra e",   "sra h",   "sra l",   "sra (hl)",   "sra a",
	/* 3x */ "swap b",  "swap c",  "swap d",  "swap e",  "swap h",  "swap l",  "swap (hl)",  "swap a",  "srl b",   "srl c",   "srl d",   "srl e",   "srl h",   "srl l",   "srl (hl)",   "srl a",
	/* 4x */ "bit 0,b", "bit 0,c", "bit 0,d", "bit 0,e", "bit 0,h", "bit 0,l", "bit 0,(hl)", "bit 0,a", "bit 1,b", "bit 1,c", "bit 1,d", "bit 1,e", "bit 1,h", "bit 1,l", "bit 1,(hl)", "bit 1,a",
	/* 5x */ "bit 2,b", "bit 2,c", "bit 2,d", "bit 2,e", "bit 2,h", "bit 2,l", "bit 2,(hl)", "bit 2,a", "bit 3,b", "bit 3,c", "bit 3,d", "bit 3,e", "bit 3,h", "bit 3,l", "bit 3,(hl)", "bit 3,a",
	/* 6x */ "bit 4,b", "bit 4,c", "bit 4,d", "bit 4,e", "bit 4,h", "bit 4,l", "bit 4,(hl)", "bit 4,a", "bit 5,b", "bit 5,c", "bit 5,d", "bit 5,e", "bit 5,h", "bit 5,l", "bit 5,(hl)", "bit 5,a",
	/* 7x */ "bit 6,b", "bit 6,c", "bit 6,d", "bit 6,e", "bit 6,h", "bit 6,l", "bit 6,(hl)", "bit 6,a", "bit 7,b", "bit 7,c", "bit 7,d", "bit 7,e", "bit 7,h", "bit 7,l", "bit 7,(hl)", "bit 7,a",
	/* 8x */ "res 0,b", "res 0,c", "res 0,d", "res 0,e", "res 0,h", "res 0,l", "res 0,(hl)", "res 0,a", "res 1,b", "res 1,c", "res 1,d", "res 1,e", "res 1,h", "res 1,l", "res 1,(hl)", "res 1,a",
	/* 9x */ "res 2,b", "res 2,c", "res 2,d", "res 2,e", "res 2,h", "res 2,l", "res 2,(hl)", "res 2,a", "res 3,b", "res 3,c", "res 3,d", "res 3,e", "res 3,h", "res 3,l", "res 3,(hl)", "res 3,a",
	/* Ax */ "res 4,b", "res 4,c", "res 4,d", "res 4,e", "res 4,h", "res 4,l", "res 4,(hl)", "res 4,a", "res 5,b", "res 5,c", "res 5,d", "res 5,e", "res 5,h", "res 5,l", "res 5,(hl)", "res 5,a",
	/* Bx */ "res 6,b", "res 6,c", "res 6,d", "res 6,e", "res 6,h", "res 6,l", "res 6,(hl)", "res 6,a", "res 7,b", "res 7,c", "res 7,d", "res 7,e", "res 7,h", "res 7,l", "res 7,(hl)", "res 7,a",
	/* Cx */ "set 0,b", "set 0,c", "set 0,d", "set 0,e", "set 0,h", "set 0,l", "set 0,(hl)", "set 0,a", "set 1,b", "set 1,c", "set 1,d", "set 1,e", "set 1,h", "set 1,l", "set 1,(hl)", "set 1,a",
	/* Dx */ "set 2,b", "set 2,c", "set 2,d", "set 2,e", "set 2,h", "set 2,l", "set 2,(hl)", "set 2,a", "set 3,b", "set 3,c", "set 3,d", "set 3,e", "set 3,h", "set 3,l", "set 3,(hl)", "set 3,a",
	/* Ex */ "set 4,b", "set 4,c", "set 4,d", "set 4,e", "set 4,h", "set 4,l", "set 4,(hl)", "set 4,a", "set 5,b", "set 5,c", "set 5,d", "set 5,e", "set 5,h", "set 5,l", "set 5,(hl)", "set 5,a",
	/* Fx */ "set 6,b", "set 6,c", "set 6,d", "set 6,e", "set 6,h", "set 6,l", "set 6,(hl)", "set 6,a", "set 7,b", "set 7,c", "set 7,d", "set 7,e", "set 7,h", "set 7,l", "set 7,(hl)", "set 7,a"
};

static constexpr const char* const ASSEMBLER_OPRAND_PATTERN_FOR_N8[] = {
	"ld b,n8",    "ld c,n8",
	"ld d,n8",    "ld e,n8",
	"ld h,n8",    "ld l,n8",
	"ld (hl),n8", "ld a,n8",
	"add a,n8",   "adc a,n8",
	"sub a,n8",   "sbc a,n8",
	"and a,n8",   "xor a,n8",
	"or a,n8",    "cp a,n8"
};
static constexpr const char* const ASSEMBLER_OPRAND_PATTERN_FOR_N16[] = {
	"ld bc,n16",
	"ld de,n16",
	"ld hl,n16",
	"ld sp,n16"
};
static constexpr const char* const ASSEMBLER_OPRAND_PATTERN_FOR_A8[] = {
	"ldh (a8),a",
	"ldh a,(a8)"
};
static constexpr const char* const ASSEMBLER_OPRAND_PATTERN_FOR_A16[] = {
	"ld (a16),sp",
	"jp nz,a16",   "jp a16",      "call nz,a16", "jp z,a16",   "call z,a16", "call a16",
	"jp nc,a16",   "call nc,a16", "jp c,a16",    "call c,a16",
	"ld (a16),a",
	"ld a,(a16)"
};
static constexpr const char* const ASSEMBLER_OPRAND_PATTERN_FOR_E8[] = {
	"jr e8",
	"jr nz,e8",   "jr z,e8",
	"jr nc,e8",   "jr c,e8",
	"add sp,e8",

	"ldhl sp,e8", "ld hl,sp+e8"
};

static constexpr const UInt8 ASSEMBLER_OPCODE_SIZE[256] = {
	/*       x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
	/* 0x */ 1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1,
	/* 1x */ 1, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
	/* 2x */ 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
	/* 3x */ 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
	/* 4x */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 5x */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 6x */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 7x */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 8x */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 9x */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* Ax */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* Bx */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* Cx */ 1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1,
	/* Dx */ 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1,
	/* Ex */ 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1,
	/* Fx */ 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1
};

static constexpr const UInt8 ASSEMBLER_OPCODE_NOP  = 0x00;
static constexpr const UInt8 ASSEMBLER_OPCODE_STOP = 0x10;
static constexpr const UInt8 ASSEMBLER_OPCODE_CB   = 0xcb;

}

/* ===========================================================================} */

/*
** {===========================================================================
** Assembler
*/

namespace GBBASIC {

class AssemblerImpl : public Assembler {
private:
	typedef std::map<std::string, int> Dictionary;

	typedef std::set<std::string> Set;

	struct OprandPattern {
		typedef std::vector<OprandPattern> Array;

		std::string pattern;
		std::string type;

		OprandPattern() {
		}
		OprandPattern(const std::string &p, const std::string &y) : pattern(p), type(y) {
		}
	};

private:
	Dictionary _opcodeDictionary;
	Dictionary _cbOpcodeDictionary;
	OprandPattern::Array _oprandPatterns;
	Text::Dictionary _mnemonicAliases;
	Set _registers;

public:
	AssemblerImpl() {
		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_OPCODE_MNEMONIC); ++i) {
			const char* op = ASSEMBLER_OPCODE_MNEMONIC[i];
			if (!op)
				continue;

			_opcodeDictionary.insert(std::make_pair(op, i));
		}
		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_CB_OPCODE_MNEMONIC); ++i) {
			const char* op = ASSEMBLER_CB_OPCODE_MNEMONIC[i];
			if (!op)
				continue;

			_cbOpcodeDictionary.insert(std::make_pair(op, i));
		}

		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_OPRAND_PATTERN_FOR_N8); ++i) {
			const char* pattern = ASSEMBLER_OPRAND_PATTERN_FOR_N8[i];
			_oprandPatterns.push_back(OprandPattern(pattern, "n8"));
		}
		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_OPRAND_PATTERN_FOR_N16); ++i) {
			const char* pattern = ASSEMBLER_OPRAND_PATTERN_FOR_N16[i];
			_oprandPatterns.push_back(OprandPattern(pattern, "n16"));
		}
		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_OPRAND_PATTERN_FOR_A8); ++i) {
			const char* pattern = ASSEMBLER_OPRAND_PATTERN_FOR_A8[i];
			_oprandPatterns.push_back(OprandPattern(pattern, "a8"));
		}
		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_OPRAND_PATTERN_FOR_A16); ++i) {
			const char* pattern = ASSEMBLER_OPRAND_PATTERN_FOR_A16[i];
			_oprandPatterns.push_back(OprandPattern(pattern, "a16"));
		}
		for (int i = 0; i < GBBASIC_COUNTOF(ASSEMBLER_OPRAND_PATTERN_FOR_E8); ++i) {
			const char* pattern = ASSEMBLER_OPRAND_PATTERN_FOR_E8[i];
			_oprandPatterns.push_back(OprandPattern(pattern, "e8"));
		}

		_mnemonicAliases = {
			{ "ld a,(hli)", "ld a,(hl+)"  },
			{ "ldi a,(hl)", "ld a,(hl+)"  },
			{ "ld (hli),a", "ld (hl+),a"  },
			{ "ldi (hl),a", "ld (hl+),a"  },
			{ "ld a,(hld)", "ld a,(hl-)"  },
			{ "ldd a,(hl)", "ld a,(hl-)"  },
			{ "ld (hld),a", "ld (hl-),a"  },
			{ "ldd (hl),a", "ld (hl-),a"  },
			{ "ldhl sp,e8", "ld hl,sp+e8" },

			{ "add b",      "add a,b"     },
			{ "add c",      "add a,c"     },
			{ "add d",      "add a,d"     },
			{ "add e",      "add a,e"     },
			{ "add h",      "add a,h"     },
			{ "add l",      "add a,l"     },
			{ "add (hl)",   "add a,(hl)"  },
			{ "add a",      "add a,a"     },
			{ "add n8",     "add a,n8"    },
			{ "adc b",      "adc a,b"     },
			{ "adc c",      "adc a,c"     },
			{ "adc d",      "adc a,d"     },
			{ "adc e",      "adc a,e"     },
			{ "adc h",      "adc a,h"     },
			{ "adc l",      "adc a,l"     },
			{ "adc (hl)",   "adc a,(hl)"  },
			{ "adc a",      "adc a,a"     },
			{ "adc n8",     "adc a,n8"    },
			{ "sub b",      "sub a,b"     },
			{ "sub c",      "sub a,c"     },
			{ "sub d",      "sub a,d"     },
			{ "sub e",      "sub a,e"     },
			{ "sub h",      "sub a,h"     },
			{ "sub l",      "sub a,l"     },
			{ "sub (hl)",   "sub a,(hl)"  },
			{ "sub a",      "sub a,a"     },
			{ "sub n8",     "sub a,n8"    },
			{ "sbc b",      "sbc a,b"     },
			{ "sbc c",      "sbc a,c"     },
			{ "sbc d",      "sbc a,d"     },
			{ "sbc e",      "sbc a,e"     },
			{ "sbc h",      "sbc a,h"     },
			{ "sbc l",      "sbc a,l"     },
			{ "sbc (hl)",   "sbc a,(hl)"  },
			{ "sbc a",      "sbc a,a"     },
			{ "sbc n8",     "sbc a,n8"    },
			{ "and b",      "and a,b"     },
			{ "and c",      "and a,c"     },
			{ "and d",      "and a,d"     },
			{ "and e",      "and a,e"     },
			{ "and h",      "and a,h"     },
			{ "and l",      "and a,l"     },
			{ "and (hl)",   "and a,(hl)"  },
			{ "and a",      "and a,a"     },
			{ "and n8",     "and a,n8"    },
			{ "xor b",      "xor a,b"     },
			{ "xor c",      "xor a,c"     },
			{ "xor d",      "xor a,d"     },
			{ "xor e",      "xor a,e"     },
			{ "xor h",      "xor a,h"     },
			{ "xor l",      "xor a,l"     },
			{ "xor (hl)",   "xor a,(hl)"  },
			{ "xor a",      "xor a,a"     },
			{ "xor n8",     "xor a,n8"    },
			{ "or b",       "or a,b"      },
			{ "or c",       "or a,c"      },
			{ "or d",       "or a,d"      },
			{ "or e",       "or a,e"      },
			{ "or h",       "or a,h"      },
			{ "or l",       "or a,l"      },
			{ "or (hl)",    "or a,(hl)"   },
			{ "or a",       "or a,a"      },
			{ "or n8",      "or a,n8"     },
			{ "cp b",       "cp a,b"      },
			{ "cp c",       "cp a,c"      },
			{ "cp d",       "cp a,d"      },
			{ "cp e",       "cp a,e"      },
			{ "cp h",       "cp a,h"      },
			{ "cp l",       "cp a,l"      },
			{ "cp (hl)",    "cp a,(hl)"   },
			{ "cp a",       "cp a,a"      },
			{ "cp n8",      "cp a,n8"     }
		};

		_registers = {
			"af", "bc", "de", "hl", "sp",
			"a", "b", "c", "d", "e", "h", "l",
			"z", "nz", "nc"
		};
	}
	virtual ~AssemblerImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual bool assemble(Bytes::Ptr &bytes, Cotnext &ctx, const IToken::Array &tokens, const Options &options) const override {
		// Prepare.
		int cursor = 0;
		bytes->clear();

		// Error handlers.
		auto throwInvalidLineBegin = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Invalid line begin";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};

		// Parser operations.
		auto must = [&] (IToken::Types y, Variant d = nullptr) -> auto {
			// Expect a token that matches the specific pattern, move the cursor to the next location if matched,
			// otherwise return `nullptr`.
			return [&, y, d] (int &idx) -> IToken::Ptr {
				if (idx >= (int)tokens.size())
					return nullptr;

				const IToken::Ptr &tk = tokens[idx];
				if (tk->isNot(y))
					return nullptr;
				if (d != nullptr && tk->data() != d)
					return nullptr;

				++idx;

				return tk;
			};
		};
		auto maybe = [&] (IToken::Types y, Variant d = nullptr) -> auto {
			// Expect a token that matches the specific pattern, move the cursor to the next location,
			// otherwise return `nullptr`.
			return [&, y, d] (int &idx) -> IToken::Ptr {
				if (idx >= (int)tokens.size())
					return nullptr;

				const IToken::Ptr &tk = tokens[idx++];
				if (tk->isNot(y))
					return nullptr;
				if (d != nullptr && tk->data() != d)
					return nullptr;

				return tk;
			};
		};

		// Parse the lines.
		while (cursor < (int)tokens.size()) {
			// Prepare.
			int lnBegin = -1;
			int lnEnd = -1;

			// Parse a line.
			lnBegin = cursor;
			if (!must(IToken::Types::INTEGER)(cursor)) return throwInvalidLineBegin(lnBegin);
			while (cursor < (int)tokens.size()) {
				if (maybe(IToken::Types::END_OF_LINE)(cursor)) {
					lnEnd = cursor;

					break;
				}
			}
			if (!assembleLine(bytes, ctx, tokens, lnBegin, lnEnd, options)) return false;
		}

		// Finish.
		return true;
	}

private:
	bool assembleLine(Bytes::Ptr &bytes, Cotnext &ctx, const IToken::Array &tokens, int lnBegin, int lnEnd, const Options &options) const {
		// Prepare.
		typedef std::vector<int> Oprands;

		int cursor = 0;

		// Error handlers.
		auto throwByteExpected = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Byte expected";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwCommaExpected = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Comma expected";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwIdHasNotBeenDeclared = [&] (int idx, const std::string &id) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = Text::format("ID \"{0}\" has not been decleared", { id });
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwIdHasNotBeenDeclaredDidYouMean = [&] (int idx, const std::string &id, const std::string &fuzzy) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = Text::format("ID \"{0}\" has not been decleared, did you mean \"{1}\"", { id, fuzzy });
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwInvalidFormat = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Invalid format";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwInvalidOpcode = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Invalid opcode";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwTooManyOprands = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Too many oprands";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwUnexpectedComma = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Unexpected comma";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};
		auto throwWordExpected = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Word expected";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};

		// Emitters.
		auto emitUInt8 = [] (Bytes::Ptr &bytes, Cotnext &ctx, UInt8 data) -> Byte* {
			int n = 0;
			const size_t m = bytes->peek();
			n += bytes->writeUInt8(data);

			ctx.size += n;

			ctx.addressCursor += n;

			Byte* result = bytes->pointer() + m;

			return result;
		};
		auto emitUInt16 = [] (Bytes::Ptr &bytes, Cotnext &ctx, UInt16 data) -> Byte* {
			int n = 0;
			const size_t m = bytes->peek();
			union {
				UInt16 data;
				UInt8 bytes[2];
			} u;
			u.data = data;
			if (!Platform::isLittleEndian())
				std::swap(u.bytes[0], u.bytes[1]);
			n += bytes->writeUInt8(u.bytes[0]);
			n += bytes->writeUInt8(u.bytes[1]);

			ctx.size += n;

			ctx.addressCursor += n;

			Byte* result = bytes->pointer() + m;

			return result;
		};

		// Parse the tokens.
		std::string mnemonic;
		std::string opcode;
		Oprands oprands;
		std::string oprandType;

		cursor = lnBegin;
		++cursor; // Ignore the line number.

		const IToken::Ptr &tkop = tokens[cursor];
		if (tkop->is(IToken::Types::COMMENT)) return true; // Ignore this line with comment.

		if (!tkop->is(IToken::Types::IDENTIFIER)) return throwInvalidOpcode(cursor); // Expect opcode.
		opcode =(std::string)tkop->data();
		mnemonic = opcode;
		mnemonic += " ";
		++cursor;
		Text::toLowerCase(opcode);

		for (; cursor < lnEnd; ++cursor) {
			// Prepare.
			if (cursor == lnEnd - 1) continue; // Ignore the line end.

			// Parse data.
			const IToken::Ptr &tk = tokens[cursor];
			if (opcode == "db") {
				if (tk->is(IToken::Types::STRING)) {
					const std::string data = (std::string)tk->data();
					for (std::string::value_type oprand : data)
						oprands.push_back(oprand);
					mnemonic += data;
				} else if (tk->is(IToken::Types::NUMBER)) {
					const int oprand = (int)(Int)tk->data();
					oprands.push_back(oprand);
					const std::string data = "0x" + Text::toHex(oprand, 2, '0', true);
					mnemonic += data;
				} else {
					return throwByteExpected(cursor);
				}

				const IToken::Ptr tk_ = (cursor + 1 >= 0 && cursor + 1 < (int)tokens.size()) ? tokens[++cursor] : nullptr;
				if (tk_->is(IToken::Types::OPERATOR)) {
					const std::string data = (std::string)tk_->data();
					if (data != ",") return throwCommaExpected(cursor);
					if (cursor == lnEnd - 1) return throwUnexpectedComma(cursor);
					mnemonic += data;
				}

				continue;
			} else if (opcode == "dw") {
				if (tk->is(IToken::Types::NUMBER)) {
					const int oprand = (int)(Int)tk->data();
					oprands.push_back(oprand);
					const std::string data = "0x" + Text::toHex(oprand, 4, '0', true);
					mnemonic += data;
				} else {
					return throwByteExpected(cursor);
				}

				const IToken::Ptr tk_ = (cursor + 1 >= 0 && cursor + 1 < (int)tokens.size()) ? tokens[++cursor] : nullptr;
				if (tk_->is(IToken::Types::OPERATOR)) {
					const std::string data = (std::string)tk_->data();
					if (data != ",") return throwCommaExpected(cursor);
					if (cursor == lnEnd - 1) return throwUnexpectedComma(cursor);
					mnemonic += data;
				}

				continue;
			}

			// Parse instructions.
			if (tk->is(IToken::Types::IDENTIFIER)) {
				const std::string data = (std::string)tk->data();
				if (_registers.find(data) == _registers.end()) {
					// Resolve BASIC identifiers.
					RamLocation loc;
					std::string id;
					std::string fuzzyName;
					if (!options.resolveIdentifier(tk, loc, id, fuzzyName)) {
						if (!fuzzyName.empty())
							return throwIdHasNotBeenDeclaredDidYouMean(cursor, id, fuzzyName);

						return throwIdHasNotBeenDeclared(cursor, id);
					}

					const int oprand = loc.address;
					oprands.push_back(oprand);
					mnemonic += "*";
				} else {
					mnemonic += data;
				}
			} else if (tk->is(IToken::Types::OPERATOR)) {
				mnemonic += (std::string)tk->data();
			} else if (tk->is(IToken::Types::NUMBER)) {
				const int oprand = (int)(Int)tk->data();
				if (opcode == "bit" || opcode == "res" || opcode == "set") {
					mnemonic += Text::toString(oprand);
				} else {
					oprands.push_back(oprand);
					mnemonic += "*";
				}
			} else if (tk->is(IToken::Types::COMMENT)) {
				// Do nothing.
			} else {
				// Do nothing.
			}
		}

		Text::toLowerCase(mnemonic);
		mnemonic = Text::trim(mnemonic);

		// Translate the data it's data declaration.
		if (opcode == "db") {
			for (int oprand : oprands)
				emitUInt8(bytes, ctx, (UInt8)oprand);

			return true;
		} else if (opcode == "dw") {
			for (int oprand : oprands)
				emitUInt16(bytes, ctx, (UInt16)oprand);

			return true;
		}

		// Translate the oprand.
		if (!oprands.empty()) {
			if (oprands.size() > 1) return throwTooManyOprands(cursor);

			for (const OprandPattern &pattern : _oprandPatterns) {
				if (Text::matchWildcard(pattern.pattern, mnemonic.c_str(), false)) {
					mnemonic = pattern.pattern;
					oprandType = pattern.type;

					break;
				}
			}
		}

		// Translate the mnemonic.
		Text::Dictionary::const_iterator ait = _mnemonicAliases.find(mnemonic);
		if (ait != _mnemonicAliases.end())
			mnemonic = ait->second;

		Dictionary::const_iterator opit = _opcodeDictionary.find(mnemonic);
		if (opit != _opcodeDictionary.end()) {
			// Emit the opcode.
			const int op = opit->second;
			GBBASIC_ASSERT(op >= 0 && op < GBBASIC_COUNTOF(ASSEMBLER_OPCODE_MNEMONIC) && "Invalid opcode.");
			emitUInt8(bytes, ctx, (UInt8)op);

			// Specialized for `stop`.
			if (op == ASSEMBLER_OPCODE_STOP) {
				emitUInt8(bytes, ctx, (UInt8)ASSEMBLER_OPCODE_NOP);

				return true;
			}

			// Resolve jump destination.
			// TODO

			// Emit the oprand.
			const int sz = ASSEMBLER_OPCODE_SIZE[op];
			const int restSz = sz - 1;
			if (restSz == 0) {
				GBBASIC_ASSERT(oprandType.empty() && "Invalid opcode.");

				return true;
			}

			if (restSz == 1) {
				GBBASIC_ASSERT((oprandType == "n8" || oprandType == "a8" || oprandType == "e8") && !oprands.empty() && "Invalid oprand.");

				if (oprands.empty())
					return false;

				const int oprand = oprands.front();
				if (oprand < std::numeric_limits<Int8>::min() || oprand > std::numeric_limits<UInt8>::max())
					return throwByteExpected(cursor);

				emitUInt8(bytes, ctx, (UInt8)oprand);

				return true;
			} else if (restSz == 2) {
				GBBASIC_ASSERT((oprandType == "n16" || oprandType == "a16") && !oprands.empty() && "Invalid oprand.");

				if (oprands.empty())
					return false;

				const int oprand = oprands.front();
				if (oprand < std::numeric_limits<Int16>::min() || oprand > std::numeric_limits<UInt16>::max())
					return throwWordExpected(cursor);

				emitUInt16(bytes, ctx, (UInt16)oprand);

				return true;
			}

			GBBASIC_ASSERT(false && "Invalid opcode.");

			return false;
		}

		Dictionary::const_iterator cbopit = _cbOpcodeDictionary.find(mnemonic);
		if (cbopit != _cbOpcodeDictionary.end()) {
			emitUInt8(bytes, ctx, (UInt8)ASSEMBLER_OPCODE_CB); // Prefix.

			const int op = cbopit->second;
			GBBASIC_ASSERT(op >= 0 && op < GBBASIC_COUNTOF(ASSEMBLER_OPCODE_MNEMONIC) && "Invalid opcode.");
			emitUInt8(bytes, ctx, (UInt8)op);

			return true;
		}

		return throwInvalidFormat(cursor);
	}
};

Assembler::Cotnext::Cotnext() {
}

Assembler::Options::Options() {
}

Assembler::Options::Options(int b, int addr, IdentifierResolver resolveid, ErrorHandler onerr) :
	bank(b), address(addr),
	resolveIdentifier(resolveid),
	onError(onerr)
{
}

Assembler* Assembler::create(void) {
	AssemblerImpl* result = new AssemblerImpl();

	return result;
}

void Assembler::destroy(Assembler* ptr) {
	AssemblerImpl* impl = static_cast<AssemblerImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
