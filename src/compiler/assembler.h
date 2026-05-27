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

	typedef std::function<bool(const IToken::Ptr &, RamLocation &, std::string &, std::string &)> IdentifierResolver;

	typedef std::function<void(const std::string &, const IToken::Ptr &)> ErrorHandler;

	struct Context {
		struct LabeledDestination {
			typedef std::map<std::string, LabeledDestination> Dictionary;

			int address = 0;

			LabeledDestination();
			LabeledDestination(int a);
		};
		struct LabelRef {
			typedef std::map<std::string, LabelRef> Dictionary;

			enum Types {
				ADDRESS,
				OFFSET
			};

			union {
				int address;
				int offset;
			};
			Types type = Types::ADDRESS;

			LabelRef();
			LabelRef(Types y, int a);
		};

		int size = 0;
		int addressCursor = 0;
		LabeledDestination::Dictionary labels;
		LabelRef::Dictionary labelRefs;
		bool hasRet = false;

		Context();
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

	virtual bool assemble(Bytes::Ptr &bytes, Context &context, const IToken::Array &tokens, const Options &options) const = 0;
	virtual void post(Bytes::Ptr &bytes, Context &context, const Options &options) const = 0;

	static Assembler* create(void);
	static void destroy(Assembler* ptr);
};

}

/* ===========================================================================} */

#endif /* __ASSEMBLER_H__ */
