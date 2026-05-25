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
** Macros and constants
*/

#ifndef EVALUATOR_DEFAULT_ACCEPTED_RPN_TYPES
#	define EVALUATOR_DEFAULT_ACCEPTED_RPN_TYPES \
		GBBASIC::IToken::Types::OPERATOR | \
		GBBASIC::IToken::Types::SYMBOL | \
		GBBASIC::IToken::Types::BOOLEAN | \
		GBBASIC::IToken::Types::NUMBER | \
		GBBASIC::IToken::Types::COMMENT | \
		GBBASIC::IToken::Types::INTERMEDIA
#endif /* EVALUATOR_DEFAULT_ACCEPTED_RPN_TYPES */

/* ===========================================================================} */

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
	 * @brief Function of unary math.
	 */
	typedef std::function<Int16(Int16)> UnaryMathHandler;
	/**
	 * @brief Function of binary math.
	 */
	typedef std::function<Int16(Int16, Int16)> BinaryMathHandler;
	/**
	 * @brief Function of token resolver.
	 */
	typedef std::function<IToken::Ptr(const IToken::Ptr &)> TokenResolver;
	/**
	 * @brief Function of error handler.
	 */
	typedef std::function<void(const std::string &, const IToken::Ptr &)> ErrorHandler;
	/**
	 * @brief Function of typed-error handler.
	 */
	typedef std::function<void(unsigned, const IToken::Ptr &)> TypedErrorHandler;

	/**
	 * @brief Options for converting expression to RPN.
	 */
	struct OptionsForRpn {
		typedef std::function<bool(const IToken::Ptr &)> ValidationHandler;
		typedef std::function<bool(const IToken::Ptr &, const std::string &)> EqualityHandler;
		typedef std::function<Op(const IToken::Ptr &)> GettingHandler;
		typedef std::function<void(const IToken::Ptr &)> AddingHandler;
		typedef std::function<int(const Op &, const Op &)> ComparisonHandler;

		enum class Errors : unsigned {
			UNEXPECTED_OPERATOR,
			INVALID_EXPRESSION
		};

		IToken::Types acceptedTypes = EVALUATOR_DEFAULT_ACCEPTED_RPN_TYPES;
		ValidationHandler valid = nullptr;
		EqualityHandler equals = nullptr;
		GettingHandler get = nullptr;
		AddingHandler add = nullptr;
		ComparisonHandler compare = nullptr;
		TypedErrorHandler onError = nullptr;

		OptionsForRpn();
		OptionsForRpn(IToken::Types accy, TypedErrorHandler err);
		OptionsForRpn(
			IToken::Types accy,
			ValidationHandler valid_,
			EqualityHandler equals_,
			GettingHandler get_,
			AddingHandler add_,
			ComparisonHandler compare_,
			TypedErrorHandler err
		);
	};
	/**
	 * @brief Options for expression folding.
	 */
	struct OptionsForFolding {
		bool caseInsensitive = true;
		bool acceptFunctionLike = false;
		UnaryMathHandler sqrt = nullptr;
		UnaryMathHandler sin = nullptr;
		UnaryMathHandler cos = nullptr;
		BinaryMathHandler atan2 = nullptr;
		BinaryMathHandler powi = nullptr;
		TokenResolver resolve = [] (const IToken::Ptr &tk) -> IToken::Ptr { return tk; };
		ErrorHandler onError = nullptr;

		OptionsForFolding();
		OptionsForFolding(bool ci, TokenResolver r, ErrorHandler err);
		OptionsForFolding(
			bool ci,
			UnaryMathHandler sqrt_,
			UnaryMathHandler sin_,
			UnaryMathHandler cos_,
			BinaryMathHandler atan2_,
			BinaryMathHandler powi_,
			TokenResolver r,
			ErrorHandler err
		);
	};
	/**
	 * @brief Options for expression calculating.
	 */
	struct OptionsForCalculating {
		bool acceptFunctionLike = false;
		UnaryMathHandler sqrt = nullptr;
		UnaryMathHandler sin = nullptr;
		UnaryMathHandler cos = nullptr;
		BinaryMathHandler atan2 = nullptr;
		BinaryMathHandler powi = nullptr;
		TokenResolver resolve = [] (const IToken::Ptr &tk) -> IToken::Ptr { return tk; };
		ErrorHandler onError = nullptr;

		OptionsForCalculating();
		OptionsForCalculating(TokenResolver r, ErrorHandler err);
		OptionsForCalculating(
			UnaryMathHandler sqrt_,
			UnaryMathHandler sin_,
			UnaryMathHandler cos_,
			BinaryMathHandler atan2_,
			BinaryMathHandler powi_,
			TokenResolver r,
			ErrorHandler err
		);
	};

public:
	Evaluator() = delete;

	/**
	 * @brief Converts infix notation to RPN.
	 *
	 * @param[out] ret The result expression in RPN.
	 * @param[in] in The infix notation to convert.
	 * @param[in] options Options for folding.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool toRpn(IToken::Array* ret /* nullable */, const IToken::Array &in, const OptionsForRpn &options);
	/**
	 * @brief Folds constants in an expression.
	 *
	 * @param[out] ret The result expression.
	 * @param[in] rpn The expression in RPN to evaluate.
	 * @param[in] options Options for folding.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool fold(IToken::Array* ret, const IToken::Array &rpn, const OptionsForFolding &options);
	/**
	 * @brief Calculates an expression.
	 *
	 * @param[out] ret The result value.
	 * @param[in] rpn The expression in RPN to evaluate.
	 * @param[in] options Options for calculating.
	 * @return `true` for succeeded, otherwise `false`.
	 */
	static bool calc(Variant* ret, const IToken::Array &rpn, const OptionsForCalculating &options);
};

}

/* ===========================================================================} */

#endif /* __EVALUATOR_H__ */
