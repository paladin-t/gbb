/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __PARSERS_H__
#define __PARSERS_H__

#include "../gbbasic.h"
#include "plus.h"
#include "../../lib/mpc/mpc.h"
#include <vector>

/*
** {===========================================================================
** Utilities
*/

namespace Parsers {

std::string removeComments(const std::string &src);

}

/* ===========================================================================} */

/*
** {===========================================================================
** C parser
*/

namespace Parsers {

struct CParser : public NonCopyable {
	typedef std::function<void(const char*)> ErrorHandler;

	typedef std::vector<mpc_ast_t*> Children;

	struct Entry {
		typedef std::initializer_list<Entry> List;

		const char* tag = nullptr;
		const char* contents = nullptr;

		Entry();
		Entry(const char* tag_);
		Entry(const char* tag_, const char* contents_);

		bool match(mpc_ast_t* ast) const;
	};

	mpc_parser_t* Ident     = nullptr;
	mpc_parser_t* Number    = nullptr;
	mpc_parser_t* Character = nullptr;
	mpc_parser_t* String    = nullptr;
	mpc_parser_t* Header    = nullptr;
	mpc_parser_t* Factor    = nullptr;
	mpc_parser_t* Term      = nullptr;
	mpc_parser_t* Lexp      = nullptr;
	mpc_parser_t* List      = nullptr;
	mpc_parser_t* Stmt      = nullptr;
	mpc_parser_t* Exp       = nullptr;
	mpc_parser_t* Type      = nullptr;
	mpc_parser_t* Typeident = nullptr;
	mpc_parser_t* Decls     = nullptr;
	mpc_parser_t* Args      = nullptr;
	mpc_parser_t* Body      = nullptr;
	mpc_parser_t* Procedure = nullptr;
	mpc_parser_t* Includes  = nullptr;
	mpc_parser_t* C         = nullptr;

	bool valid = false;
	mpc_result_t result;

	CParser(const char* y, ErrorHandler onError /* nullable */);
	~CParser();

	bool parse(const char* filename, const char* txt, size_t len);

	mpc_ast_t* is(mpc_ast_t* ast /* nullable */, const Entry &entry) const;
	/**
	 * @param[out] children
	 */
	int children(mpc_ast_t* ast /* nullable */, Children* children /* nullable */) const;
	mpc_ast_t* at(Children* children /* nullable */, int idx) const;
	mpc_ast_t* find(Children* children /* nullable */, const Entry &entry) const;
	/**
	 * @param[out] sub
	 */
	int match(Children* children /* nullable */, const Entry::List &pattern, Children* sub /* nullable */) const;

	static const char* const ANY(void);
};

}

/* ===========================================================================} */

#endif /* __PARSERS_H__ */
