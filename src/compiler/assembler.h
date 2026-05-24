/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include "../gbbasic.h"
#include "compiler.h"

/*
** {===========================================================================
** Assembler
*/

namespace GBBASIC {

class Assembler {
public:
	typedef std::function<void(const std::string &, const IToken::Ptr &)> ErrorHandler;

	struct Options {
		int bank = 0;
		int address = 0;
		ErrorHandler onError = nullptr;

		Options();
		Options(int b, int addr, ErrorHandler onerr);
	};

public:
	Assembler() = delete;

	static bool assemble(Bytes::Ptr &bytes, const IToken::Array &tokens, const Options &options);
};

}

/* ===========================================================================} */

#endif /* __ASSEMBLER_H__ */
