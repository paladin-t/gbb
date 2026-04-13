/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EVALUATOR_H__
#define __EVALUATOR_H__

#include "../gbbasic.h"
#include "../utils/object.h"
#include <functional>
#include <vector>

/*
** {===========================================================================
** Evaluator
*/

namespace GBBASIC {

/**
 * @brief Expression evaluator.
 */
class Evaluator {
public:
	/**
	 * @brief Structure of evaluation token.
	 */
	struct Token {
		typedef std::vector<Token> Array;

		typedef std::function<Token(const Token &)> Resolver;

		enum class Types : unsigned {
			NONE             =  0,
			OPERATOR         =  1 << 1,
			SYMBOL           = (1 << 2) | (1 << 3),
				KEYWORD      =  1 << 2,
				IDENTIFIER   =  1 << 3,
			NOTHING          =  1 << 4,
			BOOLEAN          =  1 << 5,
			NUMBER           = (1 << 6) | (1 << 7),
				INTEGER      =  1 << 6,
				REAL         =  1 << 7,
			STRING           =  1 << 8,
			MACRO            =  1 << 9,
			ANY              =  0xffffffff
		};

		Types type = Types::NONE;
		Variant data = nullptr;

		Token();
		Token(Types y, const Variant &v);
	};

	/**
	 * @brief Function of error handler.
	 */
	typedef std::function<void(const std::string &)> ErrorHandler;

public:
	/**
	 * @brief Feature usages. Filled by compiler.
	 *
	 * @param[out] ret The result value if succeeded, otherwise `nullptr`.
	 * @param[in] expr The expression to evaluate.
	 * @param[in] onError The error handler.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool eval(Variant &ret, const Token::Array &expr, ErrorHandler onError /* nullable */);
	/**
	 * @brief Feature usages. Filled by compiler.
	 *
	 * @param[out] ret The result value if succeeded, otherwise `nullptr`.
	 * @param[in] expr The expression to evaluate.
	 * @param[in] resolve The custom token resolver, i.e. for resolving constant symbols, etc.
	 * @param[in] onError The error handler.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool eval(Variant &ret, const Token::Array &expr, Token::Resolver resolve, ErrorHandler onError /* nullable */);
};

}

/* ===========================================================================} */

#endif /* __EVALUATOR_H__ */
