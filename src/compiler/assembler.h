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

public:
	GBBASIC_CLASS_TYPE('A', 'S', 'M', 'B')

	virtual bool assemble(Bytes::Ptr &bytes, const IToken::Array &tokens) = 0;

	static Assembler* create(void);
	static void destroy(Assembler* ptr);
};

}

/* ===========================================================================} */

#endif /* __ASSEMBLER_H__ */
