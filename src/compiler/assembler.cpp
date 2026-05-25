/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "assembler.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ANYTHING
#	define ANYTHING "?"
#endif /* ANYTHING */

/* ===========================================================================} */

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
//   ld a,(c) has alternative mnemonic ld a,($FF00+c).
//   ld c,(a) has alternative mnemonic ld ($FF00+c),a.
//   ldh a,(a8) has alternative mnemonic ld a,($FF00+a8).
//   ldh (a8),a has alternative mnemonic ld ($FF00+a8),a.
//   ld a,(hl+) has alternative mnemonic ld a,(hli) or ldi a,(hl).
//   ld (hl+),a has alternative mnemonic ld (hli),a or ldi (hl),a.
//   ld a,(hl-) has alternative mnemonic ld a,(hld) or ldd a,(hl).
//   ld (hl-),a has alternative mnemonic ld (hld),a or ldd (hl),a.
//   ld hl,sp+e8 has alternative mnemonic ldhl sp,e8.

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

private:
	Dictionary _opcodeDictionary;
	Dictionary _cbOpcodeDictionary;
	Text::Dictionary _mnemonicAliases;

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
		_mnemonicAliases = {
			{ "ld a,(hli)", "ld a,(hl+)" },
			{ "ldi a,(hl)", "ld a,(hl+)" },
			{ "ld (hli),a", "ld (hl+),a" },
			{ "ldi (hl),a", "ld (hl+),a" },
			{ "ld a,(hld)", "ld a,(hl-)" },
			{ "ldd a,(hl)", "ld a,(hl-)" },
			{ "ld (hld),a", "ld (hl-),a" },
			{ "ldd (hl),a", "ld (hl-),a" },
			{ "ldhl sp,e8", "ld hl,sp+e8" }
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

	virtual bool assemble(Bytes::Ptr &bytes, Cotnext &ctx, const IToken::Array &tokens, const Options &options) override {
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
				if (d == ANYTHING) {
					++idx;

					return tk;
				}
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
				if (d == ANYTHING)
					return tk;
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
	bool assembleLine(Bytes::Ptr &bytes, Cotnext &ctx, const IToken::Array &tokens, int lnBegin, int lnEnd, const Options &options) {
		// Prepare.
		int cursor = 0;

		// Error handlers.
		auto throwInvalidOpcode = [&] (int idx = -1) -> bool {
			if (idx < 0 || idx >= (int)tokens.size())
				idx = cursor;
			const IToken::Ptr tk = (cursor >= 0 && cursor < (int)tokens.size()) ? tokens[cursor] : nullptr;
			const std::string msg = "Invalid opcode";
			if (options.onError)
				options.onError(msg, tk);

			return false;
		};

		// Emitters.
		auto emit = [] (Bytes::Ptr &bytes, Cotnext &ctx, UInt8 data) -> Byte* {
			int n = 0;
			const size_t m = bytes->peek();
			n += bytes->writeUInt8(data);

			ctx.size += n;

			ctx.addressCursor += n;

			Byte* result = bytes->pointer() + m;

			return result;
		};

		// Parse the tokens.
		std::string mnemonic;

		cursor = lnBegin;
		++cursor; // Ignore the line number.

		const IToken::Ptr &tkop = tokens[cursor];
		if (!tkop->is(IToken::Types::IDENTIFIER)) return throwInvalidOpcode(cursor);
		mnemonic = (std::string)tkop->data();
		mnemonic += " ";
		++cursor;

		for (; cursor < lnEnd; ++cursor) {
			if (cursor == lnEnd - 1) continue; // Ignore the line end.

			const IToken::Ptr &tk = tokens[cursor];
			if (tk->is(IToken::Types::IDENTIFIER)) {
				mnemonic += (std::string)tk->data();
			} else if (tk->is(IToken::Types::OPERATOR)) {
				mnemonic += (std::string)tk->data();
			}
			// TODO
		}

		Text::toLowerCase(mnemonic);
		mnemonic = Text::trim(mnemonic);

		// Translate the mnemonic.
		Text::Dictionary::const_iterator ait = _mnemonicAliases.find(mnemonic);
		if (ait != _mnemonicAliases.end())
			mnemonic = ait->second;

		Dictionary::const_iterator opit = _opcodeDictionary.find(mnemonic);
		if (opit != _opcodeDictionary.end()) {
			const int op = opit->second;
			emit(bytes, ctx, (UInt8)op);
			// TODO

			return true;
		}

		Dictionary::const_iterator cbopit = _cbOpcodeDictionary.find(mnemonic);
		if (cbopit != _cbOpcodeDictionary.end()) {
			const int op = cbopit->second;
			emit(bytes, ctx, (UInt8)op);
			// TODO

			return true;
		}

		return false;
	}
};

Assembler::Cotnext::Cotnext() {
}

Assembler::Options::Options() {
}

Assembler::Options::Options(int b, int addr, ErrorHandler onerr) :
	bank(b),
	address(addr),
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
