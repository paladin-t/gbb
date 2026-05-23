/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "assembler.h"

/*
** {===========================================================================
** Assembler
*/

namespace GBBASIC {

class AssemblerImpl : public Assembler {
public:
	AssemblerImpl() {
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

	virtual bool assemble(Bytes::Ptr &bytes, const IToken::Array &tokens, ErrorHandler onError) override {
		(void)tokens;
		(void)onError;

		// Prepare.
		bytes->clear();

		// TODO: inline asm.

		// Finish.
		return true;
	}
};

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
