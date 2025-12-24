/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "encoding.h"
#include "parsers.h"
#include "text.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PARSERS_C_SYNTAX
#	define PARSERS_C_SYNTAX \
	" ident     : /[a-zA-Z_][a-zA-Z0-9_]*/                           ;              \n" \
	" number    : /0x[0-9a-fA-F]+/|/[0-9]+/                          ;              \n" \
	" character : /'.'/                                              ;              \n" \
	" string    : /\"(\\\\.|[^\"])*\"/                               ;              \n" \
	" header    : /<(\\\\.|[^>])*>/                                  ;              \n" \
	"                                                                               \n" \
	" factor    : '(' <lexp> ')'                                                    \n" \
	"           | <number>                                                          \n" \
	"           | <character>                                                       \n" \
	"           | <string>                                                          \n" \
	"           | <ident> '(' <lexp>? (',' <lexp>)* ')'                             \n" \
	"           | <ident>                                                           \n" \
	"           ;                                                                   \n" \
	"                                                                               \n" \
	" term      : <factor> (('*' | '/' | '%') <factor>)*             ;              \n" \
	" lexp      : ('*' | '&')? <term> (('+' | '-') <term>)*          ;              \n" \
	" list      : <lexp> (',' <lexp>)* ','?                          ;              \n" \
	"                                                                               \n" \
	" stmt      : '{' <stmt>* '}'                                                   \n" \
	"           | \"while\"  '(' <exp> ')' <stmt>                                   \n" \
	"           | \"if\"     '(' <exp> ')' <stmt>                                   \n" \
	"           | \"printf\" '(' <lexp>? ')' ';'                                    \n" \
	"           | \"return\" <lexp>? ';'                                            \n" \
	"           | <ident>    '=' <lexp> ';'                                         \n" \
	"           | <ident>    '(' <ident>? (',' <ident>)* ')' ';'                    \n" \
	"           ;                                                                   \n" \
	"                                                                               \n" \
	" exp       : <lexp> '>'    <lexp>                                              \n" \
	"           | <lexp> '<'    <lexp>                                              \n" \
	"           | <lexp> \">=\" <lexp>                                              \n" \
	"           | <lexp> \"<=\" <lexp>                                              \n" \
	"           | <lexp> \"!=\" <lexp>                                              \n" \
	"           | <lexp> \"==\" <lexp>                                              \n" \
	"           ;                                                                   \n" \
	"                                                                               \n" \
	" type      : \"static\"? \"const\"?                                            \n" \
	"             (                                                                 \n" \
	"                 %s                                             |              \n" \
	"                 \"int\"                                        |              \n" \
	"                 \"float\"                                      |              \n" \
	"                 \"char\"                                       |              \n" \
	"                 \"unsigned\" \"char\"                                         \n" \
	"             ) '*'?                                                            \n" \
	"             \"const\"?                                                        \n" \
	"           ;                                                                   \n" \
	" typeident : <type> <ident>                                     ;              \n" \
	" decls     : (                                                                 \n" \
	"                 <typeident> ';'                                |              \n" \
	"                 <typeident>         '=' <lexp> ';'             |              \n" \
	"                 <typeident>         '=' '{' <list> '}' ';'     |              \n" \
	"                 <typeident> '[' ']' '=' '{' <list> '}' ';'     |              \n" \
	"                 <ident>             '=' <lexp> ';'                            \n" \
	"             )*                                                                \n" \
	"           ;                                                                   \n" \
	" args      : <typeident>? (',' <typeident>)*                    ;              \n" \
	" body      : '{' <decls> <stmt>* '}'                            ;              \n" \
	" procedure : <type> <ident> '(' <args> ')' <body>               ;              \n" \
	"                                                                               \n" \
	" includes  : (                                                                 \n" \
	"                 '#' \"include\" <string>                       |              \n" \
	"                 '#' \"include\" <header>                                      \n" \
	"             )*                                                                \n" \
	"           ;                                                                   \n" \
	"                                                                               \n" \
	" c         : /^/ <includes> <decls> <procedure>* /$/            ;              \n"
#endif /* PARSERS_C_SYNTAX */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

namespace Parsers {

typedef std::vector<std::wstring> WStringArray;

static size_t toLowerCase(std::wstring &str) {
	size_t i = 0;
	while (i < str.length()) {
		wchar_t ch = str[i];
		if (ch >= std::numeric_limits<char>::min() && ch <= std::numeric_limits<char>::max())
			str[i] = (wchar_t)::tolower(ch);
		++i;
	}

	return i;
}

static std::wstring replace(const std::wstring &str, const std::wstring &from, const std::wstring &to, bool all = true) {
	if (from.empty())
		return str;

	size_t startPos = 0;
	std::wstring result = str;
	while ((startPos = result.find(from, startPos)) != std::wstring::npos) {
		result.replace(startPos, from.length(), to);
		startPos += to.length();
		if (!all)
			break;
	}

	return result;
}

static WStringArray split(const std::wstring &str, const std::wstring &delims, bool parseUpToNext = true, size_t maxSplits = 0) {
	WStringArray ret;
	size_t numSplits = 0;

	size_t start = 0, pos = 0;
	do {
		pos = str.find_first_of(delims, start);
		if (pos == start) {
			if (start != std::wstring::npos)
				ret.push_back(L"");
			start = pos + 1;
		} else if (pos == std::wstring::npos || (maxSplits && numSplits == maxSplits)) {
			ret.push_back(str.substr(start));

			break;
		} else {
			ret.push_back(str.substr(start, pos - start));
			start = pos + 1;
		}
		if (parseUpToNext)
			start = str.find_first_not_of(delims, start);
		++numSplits;
	} while (pos != std::wstring::npos);

	return ret;
}

static size_t indexOf(const std::wstring &str, const std::wstring &what, size_t start = 0) {
	return str.find(what, start);
}

std::string removeComments(const std::string &src) {
	// Prepare.
	std::wstring wsrc = Unicode::toWide(src);
	wsrc = replace(wsrc, L"\r\n", L"\n");
	wsrc = replace(wsrc, L"\r", L"\n");

	// Remove multi-line comments.
	for (; ; ) {
		// C style.
		const size_t begin = indexOf(wsrc, L"/*");
		const size_t end = begin == std::wstring::npos ? std::wstring::npos : indexOf(wsrc, L"*/", begin + 2);
		if (begin == std::wstring::npos || end == std::wstring::npos)
			break;

		const std::wstring head = wsrc.substr(0, begin);
		const std::wstring tail = wsrc.substr(end + 2);
		wsrc = head + tail;
	}

	// Remove single-line comments.
	WStringArray wlines = split(wsrc, L"\n");
	for (std::wstring &wln : wlines) {
		// C style.
		size_t pos = indexOf(wln, L"//");
		if (pos != std::wstring::npos)
			wln = wln.substr(0, pos);

		// BASIC style.
		std::wstring wlnlc = wln;
		toLowerCase(wlnlc);
		pos = indexOf(wlnlc, L"rem");
		if (pos != std::wstring::npos)
			wln = wln.substr(0, pos);

		pos = indexOf(wln, L"'");
		if (pos != std::wstring::npos)
			wln = wln.substr(0, pos);

		// Assembly style.
		pos = indexOf(wln, L";");
		if (pos != std::wstring::npos)
			wln = wln.substr(0, pos);

		// Shell/Python style.
		pos = indexOf(wln, L"#");
		if (pos != std::wstring::npos)
			wln = wln.substr(0, pos);
	}

	// Finish.
	wsrc.clear();
	for (const std::wstring &wln : wlines) {
		wsrc += wln;
		wsrc += L"\n";
	}

	const std::string result = Unicode::fromWide(wsrc);

	return result;
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** C parser
*/

namespace Parsers {

CParser::Entry::Entry() {
}

CParser::Entry::Entry(const char* tag_) : tag(tag_) {
}

CParser::Entry::Entry(const char* tag_, const char* contents_) : tag(tag_), contents(contents_) {
}

bool CParser::Entry::match(mpc_ast_t* ast) const {
	if (!tag)
		return false;

	if (tag && ast->tag) {
		if (strcmp(tag, ANY()) != 0) {
			if (strcmp(ast->tag, tag) != 0)
				return false;
		}
	}
	if (contents && ast->contents) {
		if (strcmp(contents, ANY()) != 0) {
			if (strcmp(ast->contents, contents) != 0)
				return false;
		}
	}

	return true;
}

CParser::CParser(const char* y, ErrorHandler onError) {
	Ident     = mpc_new("ident");
	Number    = mpc_new("number");
	Character = mpc_new("character");
	String    = mpc_new("string");
	Header    = mpc_new("header");
	Factor    = mpc_new("factor");
	Term      = mpc_new("term");
	Lexp      = mpc_new("lexp");
	List      = mpc_new("list");
	Stmt      = mpc_new("stmt");
	Exp       = mpc_new("exp");
	Type      = mpc_new("type");
	Typeident = mpc_new("typeident");
	Decls     = mpc_new("decls");
	Args      = mpc_new("args");
	Body      = mpc_new("body");
	Procedure = mpc_new("procedure");
	Includes  = mpc_new("includes");
	C         = mpc_new("c");

	const std::string fmt = Text::replace(PARSERS_C_SYNTAX, "%s", "{0}", false);
	const std::string syntax = Text::format(fmt, { y });
	mpc_err_t* err = mpca_lang(
		MPCA_LANG_DEFAULT,
		syntax.c_str(),
		Ident, Number, Character, String, Header,
		Factor,
		Term, Lexp, List,
		Stmt,
		Exp,
		Type, Typeident, Decls, Args, Body, Procedure,
		Includes,
		C,
		nullptr
	);

	if (err == nullptr) {
		valid = true;
	} else {
		if (onError) {
			char* str = mpc_err_string(err);
			onError(str);
			free(str);
		} else {
			mpc_err_print(err);
		}
		mpc_err_delete(err);

		GBBASIC_ASSERT(false && "Wrong data.");
	}
}

CParser::~CParser() {
	mpc_cleanup(
		19,
		Ident, Number, Character, String, Header,
		Factor,
		Term, Lexp, List,
		Stmt,
		Exp,
		Type, Typeident, Decls, Args, Body, Procedure,
		Includes,
		C
	);
}

bool CParser::parse(const char* filename, const char* txt, size_t len) {
	return !!mpc_nparse(filename, txt, len, C, &result);
}

mpc_ast_t* CParser::is(mpc_ast_t* ast, const Entry &entry) const {
	if (!ast)
		return nullptr;
	if (!entry.match(ast))
		return nullptr;

	return ast;
}

int CParser::children(mpc_ast_t* ast, Children* children) const {
	if (children)
		children->clear();

	if (!ast)
		return 0;

	if (children) {
		for (int i = 0; i < ast->children_num; ++i)
			children->push_back(ast->children[i]);
	}

	return ast->children_num;
}

mpc_ast_t* CParser::at(Children* children, int idx) const {
	if (!children)
		return nullptr;

	if (idx < 0 || idx >= (int)children->size())
		return nullptr;

	mpc_ast_t* ast = (*children)[idx];

	return ast;
}

mpc_ast_t* CParser::find(Children* children, const Entry &entry) const {
	if (!children)
		return nullptr;

	for (int i = 0; i < (int)children->size(); ++i) {
		mpc_ast_t* ast = (*children)[i];
		if (entry.match(ast))
			return ast;
	}

	return nullptr;
}

int CParser::match(Children* children, const Entry::List &pattern, Children* sub) const {
	if (sub)
		sub->clear();

	if (!children)
		return 0;
	if (pattern.size() == 0)
		return 0;
	if (children->size() < pattern.size())
		return 0;

	Entry::List::const_iterator it = pattern.begin();
	for (int i = 0; i < (int)pattern.size(); ++i, ++it) {
		const Entry &entry = *it;
		if (!entry.match((*children)[i]))
			return 0;
	}

	for (int i = 0; i < (int)pattern.size(); ++i, ++it) {
		sub->push_back((*children)[0]);
		children->erase(children->begin());
	}

	return (int)pattern.size();
}

const char* const CParser::ANY(void) {
	return "*";
}

}

/* ===========================================================================} */
