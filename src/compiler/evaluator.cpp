/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "evaluator.h"

/*
** {===========================================================================
** Evaluator
*/

namespace GBBASIC {

bool Evaluator::eval(Variant &ret, const IToken::Array &expr, ErrorHandler onError) {
	(void)ret;
	(void)expr;
	(void)onError;

	return false;
}

bool Evaluator::eval(Variant &ret, const IToken::Array &expr, TokenResolver resolve, ErrorHandler onError) {
	(void)ret;
	(void)expr;
	(void)resolve;
	(void)onError;

	return false;
}

}

/* ===========================================================================} */
