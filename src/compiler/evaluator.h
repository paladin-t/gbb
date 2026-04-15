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
	typedef std::function<IToken*(const IToken*)> TokenResolver;
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
	static bool eval(Variant &ret, const IToken::Array &expr, ErrorHandler onError /* nullable */);
	/**
	 * @brief Feature usages. Filled by compiler.
	 *
	 * @param[out] ret The result value if succeeded, otherwise `nullptr`.
	 * @param[in] expr The expression to evaluate.
	 * @param[in] resolve The custom token resolver, i.e. for resolving constant symbols, etc.
	 * @param[in] onError The error handler.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool eval(Variant &ret, const IToken::Array &expr, TokenResolver resolve, ErrorHandler onError /* nullable */);
};

}

/* ===========================================================================} */

#endif /* __EVALUATOR_H__ */
