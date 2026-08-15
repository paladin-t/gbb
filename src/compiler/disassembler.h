/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __DISASSEMBLER_H__
#define __DISASSEMBLER_H__

#include "../gbbasic.h"
#include "compiler.h"

/*
** {===========================================================================
** Disassembler
*/

namespace GBBASIC {

class Disassembler : public virtual Object {
public:
	typedef std::shared_ptr<Disassembler> Ptr;

	struct Mnemonic {
		typedef std::vector<Mnemonic> Array;

		UInt8 bank = 0;
		UInt16 address = 0;
		std::string text;
		bool valid = false;
		std::string opcode;
		std::string operands;
		struct {
			Byte data[3];
			UInt8 count = 0;
		} bytes;

		Mnemonic();
		Mnemonic(UInt8 b, UInt16 addr, const std::string &txt, bool valid, Bytes* bytes);
	};

	struct DisassemblingOptions {
		int bankSize = 0; // Const. The size of a ROM bank.
		int startAddress = 0; // Const. The start address of a non-zero ROM bank. (Bank zero always starts from 0.)
		int bank = 0; // Const. The index of the ROM bank of the input `bytes`.
		int addressCursor = 0; // Const. The address cursor in the ROM bank of the input `bytes`.

		DisassemblingOptions();
		DisassemblingOptions(int bankSize_, int startAddr, int b, int addr);
	};

public:
	GBBASIC_CLASS_TYPE('D', 'S', 'M', 'B')

	virtual bool disassemble(Mnemonic::Array &mnemonics, const Bytes::Ptr &bytes, const DisassemblingOptions &options) const = 0;

	static Disassembler* create(void);
	static void destroy(Disassembler* ptr);
};

}

/* ===========================================================================} */

#endif /* __DISASSEMBLER_H__ */
