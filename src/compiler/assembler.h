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

extern const UInt8 ASSEMBLER_OPCODE_SIZE[256];
extern const char* const ASSEMBLER_OPCODE_MNEMONICS[256];
extern const char* const ASSEMBLER_CB_OPCODE_MNEMONICS[256];

}

/* ===========================================================================} */

/*
** {===========================================================================
** Assembler
*/

namespace GBBASIC {

class Assembler : public virtual Object {
public:
	typedef std::shared_ptr<Assembler> Ptr;

	typedef std::function<void(const std::string &, const IToken::Ptr &)> ErrorHandler;

	struct Context {
		struct LabeledDestination {
			typedef std::map<std::string, LabeledDestination> Dictionary;

			int address = 0; // Local address.

			LabeledDestination();
			LabeledDestination(int a);
		};
		struct LabelRef {
			typedef std::vector<LabelRef> Array;

			enum Types {
				ADDRESS,
				OFFSET
			};

			int tokenIndex = -1;
			std::string label;
			int offset = 0; // Instruction offset from lexical token.
			Types type = Types::ADDRESS;

			LabelRef();
			LabelRef(int tkidx, const std::string &lbl, int off, Types y);
		};

		int size = 0; // Current emitted size.
		int addressCursor = 0; // Current emitting address.
		LabeledDestination::Dictionary labels;
		LabelRef::Array labelRefs;
		bool hasRet = false;

		Context();
	};

	enum class IdentifierResolvingResults {
		NONE,
		IDENTIFIER,
		NUMBER
	};

	struct AssemblingOptions {
		typedef std::function<IdentifierResolvingResults(const IToken::Ptr &, int &, RamLocation &, std::string &, std::string &)> IdentifierResolver;

		int bank = 0; // The bank of the current assembly block.
		int address = 0; // The start address of the current assembly block.
		IdentifierResolver resolveIdentifier = nullptr;
		ErrorHandler onError = nullptr;

		AssemblingOptions();
		AssemblingOptions(int b, int addr, IdentifierResolver resolveid, ErrorHandler onerr);
	};
	struct PostingOptions {
		typedef std::function<IdentifierResolvingResults(const IToken::Ptr &, int &, RamLocation &, std::string &, std::string &)> IdentifierResolver;
		typedef std::function<Byte*(const Byte*)> ArgsResolver;

		int bank = 0; // The bank of the current assembly block.
		int baseAddress = 0; // The start address of the current assembly block.
		bool nonbanked = false;
		IdentifierResolver resolveIdentifier = nullptr;
		ArgsResolver resolveArgs = nullptr;
		ErrorHandler onError = nullptr;

		PostingOptions();
		PostingOptions(int b, int addr, bool nonbanked_, IdentifierResolver resolveid, ArgsResolver resolveargs, ErrorHandler onerr);
	};

public:
	GBBASIC_CLASS_TYPE('A', 'S', 'M', 'B')

	virtual bool assemble(Bytes::Ptr &bytes, Context &context, const IToken::Array &tokens, const AssemblingOptions &options) const = 0;
	virtual bool post(Bytes::Ptr &bytes, Context &context, const IToken::Array &tokens, const PostingOptions &options) const = 0;

	static Assembler* create(void);
	static void destroy(Assembler* ptr);
};

}

/* ===========================================================================} */

#endif /* __ASSEMBLER_H__ */
