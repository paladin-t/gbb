/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "disassembler.h"

/*
** {===========================================================================
** Disassembler
*/

namespace GBBASIC {

class DisassemblerImpl : public Disassembler {
public:
	DisassemblerImpl() {
	}
	virtual ~DisassemblerImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual bool disassemble(Mnemonic::Array &mnemonic, const Bytes::Ptr &bytes, const DisassemblingOptions &options) const override {
		// Prepare.
		mnemonic.clear();

		// TODO

		// Finish.
		return true;
	}
};

Disassembler::Mnemonic::Mnemonic() {
}

Disassembler::Mnemonic::Mnemonic(const std::string &txt, UInt8 b, UInt16 addr) :
	text(txt),
	bank(b), address(addr)
{
}

Disassembler::DisassemblingOptions::DisassemblingOptions() {
}

Disassembler::DisassemblingOptions::DisassemblingOptions(int bankSize_, int startAddr) :
	bankSize(bankSize_), startAddress(startAddr)
{
}

Disassembler* Disassembler::create(void) {
	DisassemblerImpl* result = new DisassemblerImpl();

	return result;
}

void Disassembler::destroy(Disassembler* ptr) {
	DisassemblerImpl* impl = static_cast<DisassemblerImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
