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
#include "compiler.h"

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
	 * @brief Function of token resolver.
	 */
	typedef std::function<IToken::Ptr(const IToken::Ptr &)> TokenResolver;
	/**
	 * @brief Function of error handler.
	 */
	typedef std::function<void(const std::string &, const IToken::Ptr &)> ErrorHandler;

	/**
	 * @brief Options for expression evaluation.
	 */
	struct Options {
		bool caseInsensitive = true;
		TokenResolver resolve = [] (const IToken::Ptr &tk) -> IToken::Ptr { return tk; };
		ErrorHandler onError = nullptr;

		Options();
		Options(bool ci);
		Options(bool ci, TokenResolver r, ErrorHandler err);
	};

public:
	/**
	 * @brief Converts infix notation to RPN.
	 *
	 * @param[out] ret The result expression in RPN.
	 * @param[in] in The infix notation to convert.
	 * @param[in] options Options for folding.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool toRpn(IToken::Array &ret, const IToken::Array &in, const Options &options);
	/**
	 * @brief Folds constants in an expression.
	 *
	 * @param[out] ret The result expression.
	 * @param[in] rpn The expression in RPN to evaluate.
	 * @param[in] options Options for folding.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool fold(IToken::Array &ret, const IToken::Array &rpn, const Options &options);
	/**
	 * @brief Calculates an expression.
	 *
	 * @param[out] ret The result value.
	 * @param[in] rpn The expression in RPN to evaluate.
	 * @param[in] options Options for calculating.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool calc(Variant &ret, const IToken::Array &rpn, const Options &options);
};

}

/* ===========================================================================} */

#endif /* __EVALUATOR_H__ */
