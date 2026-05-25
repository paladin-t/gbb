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

class Assembler : public virtual Object {
public:
	typedef std::shared_ptr<Assembler> Ptr;

	typedef std::function<bool(const IToken::Ptr &, RamLocation &)> IdentifierResolver;

	typedef std::function<void(const std::string &, const IToken::Ptr &)> ErrorHandler;

	struct Cotnext {
		int size = 0;
		int addressCursor = 0;

		Cotnext();
	};

	struct Options {
		int bank = 0;
		int address = 0;
		IdentifierResolver resolveIdentifier = nullptr;
		ErrorHandler onError = nullptr;

		Options();
		Options(int b, int addr, IdentifierResolver resolveid, ErrorHandler onerr);
	};

public:
	GBBASIC_CLASS_TYPE('A', 'S', 'M', 'B')

	virtual bool assemble(Bytes::Ptr &bytes, Cotnext &ctx, const IToken::Array &tokens, const Options &options) const = 0;

	static Assembler* create(void);
	static void destroy(Assembler* ptr);
};

}

/* ===========================================================================} */

#endif /* __ASSEMBLER_H__ */
