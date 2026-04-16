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

public:
	/**
	 * @brief Folds constants in an expression.
	 *
	 * @param[out] ret The result expression.
	 * @param[in] rpn The expression to evaluate.
	 * @param[in] onError The error handler.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool fold(IToken::Array &ret, const IToken::Array &rpn, ErrorHandler onError /* nullable */);
	/**
	 * @brief Folds constants in an expression.
	 *
	 * @param[out] ret The result expression.
	 * @param[in] rpn The expression to evaluate.
	 * @param[in] resolve The custom token resolver, i.e. for resolving constant symbols, etc.
	 * @param[in] onError The error handler.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool fold(IToken::Array &ret, const IToken::Array &rpn, TokenResolver resolve, ErrorHandler onError /* nullable */);
};

}

/* ===========================================================================} */

#endif /* __EVALUATOR_H__ */
