/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "evaluator.h"
#include "../utils/text.h"
#include <stack>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EVALUATOR_DEBUG
#	if defined GBBASIC_DEBUG
#		define EVALUATOR_DEBUG 1
#	else /* GBBASIC_DEBUG */
#		define EVALUATOR_DEBUG 0
#	endif /* GBBASIC_DEBUG */
#endif /* EVALUATOR_DEBUG */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

namespace GBBASIC {

class TreeNode {
public:
	typedef std::shared_ptr<TreeNode> Ptr;
	typedef std::vector<Ptr> Array;
	typedef std::stack<Ptr> Stack;

private:
	IToken::Ptr _token = nullptr;
	Array _children;

public:
	TreeNode(const IToken::Ptr &tk) :
		_token(tk)
	{
	}
	~TreeNode() {
	}

	const IToken::Ptr &token(void) const {
		return _token;
	}

	const Array &children(void) const {
		return _children;
	}
	void add(const Ptr &child) {
		if (!child)
			return;

		_children.insert(_children.begin(), child);
	}
};

static bool isOperator(const IToken::Ptr &tk) {
	if (!tk)
		return false;

	if (tk->is(IToken::Types::OPERATOR))
		return true;
	if (tk->is(IToken::Types::INTERMEDIA))
		return true;

	return false;
}

static int getArity(const IToken::Ptr &tk) {
	if (!tk)
		return 0;

	const std::string str = (std::string)tk->data();
	const Op::Types y = Op::typeOf(str);
	const Op &op = Op::OPERATORS[(size_t)y];

	return op.oprands;
}

static bool isFunctionLike(Op::Types y) {
	switch (y) {
	case Op::Types::SGN: // Fall through.
	case Op::Types::ABS: // Fall through.
	case Op::Types::SQR: // Fall through.
	case Op::Types::SQRT: // Fall through.
	case Op::Types::SIN: // Fall through.
	case Op::Types::COS: // Fall through.
	case Op::Types::ATAN2: // Fall through.
	case Op::Types::POWI: // Fall through.
	case Op::Types::MIN: // Fall through.
	case Op::Types::MAX:
		return true;
	default:
		return false;
	}
}

static Int16 getInt16(const Variant &v) {
	return (Int16)(int)v;
}

#if EVALUATOR_DEBUG
static std::string toInfixNotationString(const IToken::Array &rpn, Evaluator::ErrorHandler onError) {
	typedef std::stack<std::string> StringStack;

	StringStack stk;
	for (const IToken::Ptr &tk : rpn) {
		if (!tk)
			continue;

		if (isOperator(tk)) {
			const std::string sym = tk->data().toString();
			const int arity = getArity(tk);

			if (arity == 1) {
				if (stk.empty()) {
					if (onError)
						onError("Invalid RPN, missing operand for unary operator", tk);

					return "";
				}

				const std::string operand = stk.top();
				stk.pop();
				stk.push(sym + "(" + operand + ")");
			} else if (arity == 2) {
				if (stk.size() < 2) {
					if (onError)
						onError("Invalid RPN, missing operands for binary operator", tk);

					return "";
				}

				const std::string right = stk.top();
				stk.pop();
				const std::string left = stk.top();
				stk.pop();
				stk.push("(" + left + " " + sym + " " + right + ")");
			} else {
				stk.push(sym);
			}
		} else {
			const std::string txt = tk->data().toString();
			stk.push(txt);
		}
	}
	if (stk.size() != 1) {
		if (onError)
			onError("Invalid RPN expression", nullptr);

		return "";
	}

	return stk.top();
}
#endif /* EVALUATOR_DEBUG */

}

/* ===========================================================================} */

/*
** {===========================================================================
** Evaluator
*/

namespace GBBASIC {

Evaluator::OptionsForRpn::OptionsForRpn() {
}

Evaluator::OptionsForRpn::OptionsForRpn(IToken::Types accy, TypedErrorHandler err) :
	acceptedTypes(accy),
	onError(err)
{
}

Evaluator::OptionsForRpn::OptionsForRpn(
	IToken::Types accy,
	ValidationHandler valid_,
	EqualityHandler equals_,
	GettingHandler get_,
	AddingHandler add_,
	ComparisonHandler compare_,
	TypedErrorHandler err
) :
	acceptedTypes(accy),
	valid(valid_),
	equals(equals_),
	get(get_),
	add(add_),
	compare(compare_),
	onError(err)
{
}

Evaluator::OptionsForFolding::OptionsForFolding() {
}

Evaluator::OptionsForFolding::OptionsForFolding(bool ci, TokenResolver r, ErrorHandler err) :
	caseInsensitive(ci),
	acceptFunctionLike(false),
	resolve(r),
	onError(err)
{
}

Evaluator::OptionsForFolding::OptionsForFolding(
	bool ci,
	UnaryMathHandler sqrt_,
	UnaryMathHandler sin_,
	UnaryMathHandler cos_,
	BinaryMathHandler atan2_,
	BinaryMathHandler powi_,
	TokenResolver r,
	ErrorHandler err
) :
	caseInsensitive(ci),
	acceptFunctionLike(true),
	sqrt(sqrt_),
	sin(sin_),
	cos(cos_),
	atan2(atan2_),
	powi(powi_),
	resolve(r),
	onError(err)
{
}

Evaluator::OptionsForCalculating::OptionsForCalculating() {
}

Evaluator::OptionsForCalculating::OptionsForCalculating(TokenResolver r, ErrorHandler err) :
	acceptFunctionLike(false),
	resolve(r),
	onError(err)
{
}

Evaluator::OptionsForCalculating::OptionsForCalculating(
	UnaryMathHandler sqrt_,
	UnaryMathHandler sin_,
	UnaryMathHandler cos_,
	BinaryMathHandler atan2_,
	BinaryMathHandler powi_,
	TokenResolver r,
	ErrorHandler err
) :
	acceptFunctionLike(true),
	sqrt(sqrt_),
	sin(sin_),
	cos(cos_),
	atan2(atan2_),
	powi(powi_),
	resolve(r),
	onError(err)
{
}

bool Evaluator::toRpn(IToken::Array* ret, const IToken::Array &in, const OptionsForRpn &options) {
	// Prepare.
	typedef std::stack<IToken::Ptr> Stack;

	const IToken::Types acceptedTypes = options.acceptedTypes;
	const TypedErrorHandler onError = options.onError;

	Stack stack;
	if (ret)
		ret->clear();

	OptionsForRpn::ValidationHandler valid = options.valid;
	OptionsForRpn::EqualityHandler equals = options.equals;
	OptionsForRpn::GettingHandler get = options.get;
	OptionsForRpn::AddingHandler add = options.add;
	OptionsForRpn::ComparisonHandler compare = options.compare;

	if (valid == nullptr) {
		valid = [] (const IToken::Ptr &tk) -> bool {
			return isOperator(tk);
		};
	}
	if (equals == nullptr) {
		equals = [] (const IToken::Ptr &tk, const std::string &key_) -> bool {
			const std::string key = (std::string)tk->data();

			return key_ == key;
		};
	}
	if (get == nullptr) {
		get = [] (const IToken::Ptr &tk) -> Op {
			const std::string str = (std::string)tk->data();
			const Op::Types y = Op::typeOf(str);

			return Op::OPERATORS[(size_t)y];
		};
	}
	if (add == nullptr) {
		add = [&ret] (const IToken::Ptr &tk) -> void {
			if (ret)
				ret->push_back(tk);
		};
	}
	if (compare == nullptr) {
		compare = [] (const Op &left, const Op &right) -> int {
			return left.precedence - right.precedence;
		};
	}

	// Iterate the tokens.
	auto raiseInvalidExpression = [] (const IToken::Ptr &tk, TypedErrorHandler onError) -> void {
		onError((unsigned)OptionsForRpn::Errors::INVALID_EXPRESSION, tk);
	};
	IToken::Ptr expectsOperator(IToken::create());
	IToken::Ptr expectsOperand = nullptr;
	for (const IToken::Ptr &tk : in) {
		if ((unsigned)(acceptedTypes & tk->type()) == 0) {
			raiseInvalidExpression(tk, onError);

			return false;
		}

		switch (tk->type()) {
		case IToken::Types::OPERATOR:
			if (expectsOperator == nullptr) {
				if (tk->data() == "(") { // Is a left parenthesis.
					expectsOperand = tk;
				} else {
					const bool allowUnaryNot = expectsOperand && expectsOperand->is(IToken::Types::OPERATOR) &&
						(
							expectsOperand->data() == "("      ||
							expectsOperand->data() == "="      ||
							expectsOperand->data() == "<"      ||
							expectsOperand->data() == "<="     ||
							expectsOperand->data() == ">"      ||
							expectsOperand->data() == ">="     ||
							expectsOperand->data() == "<>"     ||
							expectsOperand->data() == "+"      ||
							expectsOperand->data() == "-"      ||
							expectsOperand->data() == "*"      ||
							expectsOperand->data() == "/"      ||
							expectsOperand->data() == "mod"    ||
							expectsOperand->data() == "and"    ||
							expectsOperand->data() == "or"     ||
							expectsOperand->data() == "not"    ||
							expectsOperand->data() == "band"   ||
							expectsOperand->data() == "bor"    ||
							expectsOperand->data() == "bxor"   ||
							expectsOperand->data() == "lshift" ||
							expectsOperand->data() == "rshift" ||
							expectsOperand->data() == "bnot"
						);
					const bool isUnaryNot = tk->type() == IToken::Types::OPERATOR &&
						(tk->data() == "not" || tk->data() == "bnot"); // Is a unary `NOT` or `BNOT`.
					if (allowUnaryNot && isUnaryNot) {
						expectsOperand = tk;

						break;
					} else {
						if (onError)
							onError((unsigned)OptionsForRpn::Errors::UNEXPECTED_OPERATOR, tk);

						return false;
					}
				}
			} else {
				if (tk->data() == ")") {
					expectsOperator = IToken::Ptr(IToken::create());
				} else {
					expectsOperator = nullptr;
				}
			}
			if (tk->data() != "(" && tk->data() != ")") {
				expectsOperand = tk;
			}

			break;
		case IToken::Types::SYMBOL: case IToken::Types::KEYWORD: case IToken::Types::IDENTIFIER: // Fall through.
		case IToken::Types::BOOLEAN:                                                             // Fall through.
		case IToken::Types::NUMBER: case IToken::Types::INTEGER: case IToken::Types::REAL:       // Fall through.
		case IToken::Types::INTERMEDIA: case IToken::Types::MATH: case IToken::Types::STATEMENT: case IToken::Types::ARRAY: case IToken::Types::MACRO:
			expectsOperator = tk;
			expectsOperand = nullptr;

			break;
		case IToken::Types::STRING:
			expectsOperator = tk;
			expectsOperand = nullptr;

			break;
		case IToken::Types::COMMENT:
			continue;
		default:
			raiseInvalidExpression(tk, onError);

			return false;
		}

		if (valid(tk)) { // The token is an operator.
			while (!stack.empty() && valid(stack.top())) { // While there is an operator (y) at the top of the operators stack.
				// Either (x) is left-associative and its precedence is less or equal to that of (y), or
				// (x) is right-associative and its precedence is less than (y).
				const Op currentOperator = get(tk); // The current operator.
				const Op lastOperator = get(stack.top()); // The top operator from the stack.
				if ((currentOperator.associativity == -1 && compare(currentOperator, lastOperator) <= 0) || (currentOperator.associativity == 1 && compare(currentOperator, lastOperator) < 0)) {
					// Pop (y) from the stack.
					// Add (y) to the output collection.
					const IToken::Ptr top = stack.top();
					stack.pop();
					add(top);

					continue;
				}

				break;
			}
			// Push the new operator on the stack.
			stack.push(tk);
		} else if (equals(tk, "(")) { // The token is a left parenthesis.
			// Push it on the stack.
			stack.push(tk);
		} else if (equals(tk, ")")) { // The token is a right parenthesis.
			while (!stack.empty() && !equals(stack.top(), "(")) {
				// Until the top token (from the stack) is left parenthesis,
				// pop from the stack to the output collection.
				const IToken::Ptr top = stack.top();
				stack.pop();
				add(top);
			}
			// Also pop the left parenthesis but don't include it in the output collection.
			if (!stack.empty())
				stack.pop();
		} else { // Otherwise.
			// Add the token to the output collection.
			add(tk);
		}
	}
	if (expectsOperand) {
		if (onError)
			onError((unsigned)OptionsForRpn::Errors::UNEXPECTED_OPERATOR, expectsOperand);

		return false;
	}

	while (!stack.empty()) { // While there are still operator tokens in the stack.
		// Pop them to the output collection.
		const IToken::Ptr top = stack.top();
		stack.pop();
		add(top);
	}

	// Finish.
	return true;
}

bool Evaluator::fold(IToken::Array* ret, const IToken::Array &rpn, const OptionsForFolding &options) {
	// Prepare.
	typedef std::function<TreeNode::Ptr(const IToken::Array &, TokenResolver, ErrorHandler)> Parser;
	typedef std::function<Maybe<Variant>(const TreeNode::Ptr &, ErrorHandler)> Folder;
	typedef std::function<bool(IToken::Array &, const TreeNode::Ptr &, ErrorHandler)> Builder;

	GBBASIC_ASSERT(ret);

	const bool caseInsensitive = options.caseInsensitive;
	const bool acceptFunctionLike = options.acceptFunctionLike;
	const TokenResolver resolve = options.resolve;
	const ErrorHandler onError = options.onError;

	// Converting RPN to AST nodes.
	Parser rpnToAst = [] (const IToken::Array &rpn, TokenResolver resolve, ErrorHandler onError) -> TreeNode::Ptr {
		TreeNode::Stack stk;
		for (const IToken::Ptr &tk : rpn) {
			if (!tk)
				continue;

			const IToken::Ptr resolved = resolve ? resolve(tk) : nullptr;
			if (!resolved) // Not a foldable token.
				continue;

			const TreeNode::Ptr node(new TreeNode(resolved));
			if (isOperator(resolved)) {
				const int arity = getArity(resolved);
				if (arity == 0) // Not a foldable operator.
					return nullptr;

				if ((int)stk.size() < arity) {
					if (onError)
						onError("Too few arguments", resolved);

					return nullptr;
				}

				for (int i = 0; i < arity && !stk.empty(); ++i) {
					node->add(stk.top());
					stk.pop();
				}
			}

			stk.push(node);
		}
		if (stk.size() != 1) {
			if (onError)
				onError("Incomplete expression", rpn.back());

			return nullptr;
		}

		const TreeNode::Ptr result = stk.top();

		return result;
	};

	// Folding constants in the AST.
	Folder fold = nullptr;
	fold = [&options, acceptFunctionLike, &fold] (const TreeNode::Ptr &ast, ErrorHandler onError) -> Maybe<Variant> {
		typedef std::vector<Variant> Variants;

		if (!ast)
			return Maybe<Variant>();

		const IToken::Ptr &tk = ast->token();
		if (!isOperator(tk)) {
			if (tk->is(IToken::Types::NOTHING))
				return Variant(0);
			else if (tk->is(IToken::Types::BOOLEAN))
				return Variant((bool)tk->data() ? 1 : 0);
			else if (tk->is(IToken::Types::NUMBER))
				return Variant((Int16)(Int)tk->data());

			return Maybe<Variant>();
		}

		const TreeNode::Array &children = ast->children();
		Variants childVals;
		for (const TreeNode::Ptr &child : children) {
			const Maybe<Variant> val = fold(child, onError);
			if (val.empty())
				return Maybe<Variant>(); // Not foldable if there's at least one non-constant node.

			childVals.push_back(val.get());
		}
		const int arity = getArity(tk);
		if ((int)childVals.size() < arity) {
			GBBASIC_ASSERT(false && "Impossible.");

			if (onError)
				onError("Too few arguments", tk);

			return nullptr;
		}

		#define FUNC1(RET, FUNC) if (options.##FUNC) { (RET) = options.##FUNC(getInt16(childVals[0])); } else { return Maybe<Variant>(); }
		#define FUNC2(RET, FUNC) if (options.##FUNC) { (RET) = options.##FUNC(getInt16(childVals[0]), getInt16(childVals[1])); } else { return Maybe<Variant>(); }
		Int16 result = 0;
		const Op::Types y = Op::typeOf((std::string)tk->data());
		if (!acceptFunctionLike && isFunctionLike(y)) {
			return Maybe<Variant>();
		}
		switch (y) {
		case Op::Types::EQ:             result =  (getInt16(childVals[0]) == getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::LT:             result =  (getInt16(childVals[0]) <  getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::LE:             result =  (getInt16(childVals[0]) <= getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::GT:             result =  (getInt16(childVals[0]) >  getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::GE:             result =  (getInt16(childVals[0]) >= getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::NE:             result =  (getInt16(childVals[0]) != getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::AND:            result =  (getInt16(childVals[0]) && getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::OR:             result =  (getInt16(childVals[0]) || getInt16(childVals[1]) ? 1 : 0); break;
		case Op::Types::NOT:            result = (!getInt16(childVals[0])                           ? 1 : 0); break;
		case Op::Types::ADD:            result =   getInt16(childVals[0]) +  getInt16(childVals[1]);          break;
		case Op::Types::SUB:            result =   getInt16(childVals[0]) -  getInt16(childVals[1]);          break;
		case Op::Types::MUL:            result =   getInt16(childVals[0]) *  getInt16(childVals[1]);          break;
		case Op::Types::DIV: {
				const Int16 div = getInt16(childVals[1]);
				if (div == 0)
					return Maybe<Variant>(); // Avoid divided by zero.

				result = getInt16(childVals[0]) / div;
			}

			break;
		case Op::Types::MOD: {
				const Int16 div = getInt16(childVals[1]);
				if (div == 0)
					return Maybe<Variant>(); // Avoid divided by zero.

				result = getInt16(childVals[0]) % div;
			}

			break;
		case Op::Types::BITWISE_AND:    result =   getInt16(childVals[0]) &  getInt16(childVals[1]);          break;
		case Op::Types::BITWISE_OR:     result =   getInt16(childVals[0]) |  getInt16(childVals[1]);          break;
		case Op::Types::BITWISE_XOR:    result =   getInt16(childVals[0]) ^  getInt16(childVals[1]);          break;
		case Op::Types::BITWISE_LSHIFT: result =   getInt16(childVals[0]) << getInt16(childVals[1]);          break;
		case Op::Types::BITWISE_RSHIFT: result =   getInt16(childVals[0]) >> getInt16(childVals[1]);          break;
		case Op::Types::BITWISE_NOT:    result =  ~getInt16(childVals[0]);                                    break;
		case Op::Types::NEG:            result =  -getInt16(childVals[0]);                                    break;
		case Op::Types::SGN:            result = (Int16)Math::sign(getInt16(childVals[0]));                   break;
		case Op::Types::ABS:            result = (Int16)std::abs(getInt16(childVals[0]));                     break;
		case Op::Types::SQR:            result = getInt16(childVals[0]) * getInt16(childVals[0]);             break;
		case Op::Types::SQRT:           FUNC1(result, sqrt);                                                  break;
		case Op::Types::SIN:            FUNC1(result, sin);                                                   break;
		case Op::Types::COS:            FUNC1(result, cos);                                                   break;
		case Op::Types::ATAN2:          FUNC2(result, atan2);                                                 break;
		case Op::Types::POWI:           FUNC2(result, powi);                                                  break;
		case Op::Types::MIN:            result = Math::min(getInt16(childVals[0]), getInt16(childVals[1]));   break;
		case Op::Types::MAX:            result = Math::max(getInt16(childVals[0]), getInt16(childVals[1]));   break;
		default:
			return Maybe<Variant>();
		}
		#undef FUNC1
		#undef FUNC2

		return Variant(result);
	};

	// Rebuilding RPN.
	Builder astToRpn = nullptr;
	astToRpn = [caseInsensitive, &fold, &astToRpn] (IToken::Array &rpn, const TreeNode::Ptr &ast, ErrorHandler onError) -> bool {
		if (!ast)
			return false;

		const IToken::Ptr &tk = ast->token();
		const Maybe<Variant> val = fold(ast, onError);
		if (!val.empty()) {
			const Variant var = val.get();
			const IToken::Types y = IToken::Types::NUMBER;
			const TextLocation &begin = tk->begin();
			const TextLocation &end = tk->end();
			rpn.push_back(IToken::Ptr(IToken::create(y, var.toString(), caseInsensitive, &begin, &end)));

			return isOperator(tk);
		}

		if (!isOperator(tk)) {
			rpn.push_back(tk);

			return false; // Not folded.
		}

		int folded = 0;
		const TreeNode::Array &children = ast->children();
		for (const TreeNode::Ptr &child : children) {
			if (astToRpn(rpn, child, onError))
				++folded;
		}
		rpn.push_back(tk);

		return !!folded;
	};

	// Fold the expression.
	if (ret)
		ret->clear();
	IToken::Array ret_;
	const TreeNode::Ptr ast = rpnToAst(rpn, resolve, onError);
	if (!ast)
		return false;
	if (!astToRpn(ret_, ast, onError))
		return false;
	
#if EVALUATOR_DEBUG
	const std::string origin = toInfixNotationString(ret_, onError);
	const std::string folded = toInfixNotationString(ret_, onError);

	fprintf(stdout, "Folded %s\n    to %s\n", origin.c_str(), folded.c_str());
#endif /* EVALUATOR_DEBUG */

	if (ret)
		std::swap(*ret, ret_);

	return true;
}

bool Evaluator::calc(Variant* ret, const IToken::Array &rpn, const OptionsForCalculating &options) {
	typedef std::stack<Variant> Stack;
	typedef std::vector<Variant> Variants;

	GBBASIC_ASSERT(ret);

	const bool acceptFunctionLike = options.acceptFunctionLike;
	const TokenResolver resolve = options.resolve;
	const ErrorHandler onError = options.onError;

	Stack stk;
	if (ret)
		*ret = nullptr;

	// Iterate the tokens.
	for (const IToken::Ptr &tk : rpn) {
		if (!tk)
			continue;

		const IToken::Ptr resolved = resolve ? resolve(tk) : nullptr;
		if (!resolved) // Not a calculable token.
			continue;

		if (!isOperator(resolved)) {
			if (resolved->is(IToken::Types::NOTHING)) {
				stk.push(Variant(0));
			} else if (resolved->is(IToken::Types::BOOLEAN)) {
				stk.push(Variant((bool)resolved->data() ? 1 : 0));
			} else if (resolved->is(IToken::Types::NUMBER)) {
				stk.push(Variant((Int16)(Int)resolved->data()));
			} else {
				if (onError) {
					const std::string msg = "Invalid operand {0}";
					onError(Text::format(msg, { resolved->caseSensitiveText() }), resolved);
				}

				return false;
			}
		} else {
			const int arity = getArity(resolved);
			if (arity == 0) {
				if (onError) {
					const std::string msg = "Invalid operator {0}";
					onError(Text::format(msg, { resolved->caseSensitiveText() }), resolved);
				}

				return false;
			}
			if ((int)stk.size() < arity) {
				if (onError)
					onError("Too few arguments", resolved);

				return false;
			}
			Variants childVals;
			for (int i = 0; i < arity; ++i) {
				childVals.insert(childVals.begin(), stk.top());
				stk.pop();
			}

			#define FUNC1(RET, FUNC) if (options.##FUNC) { (RET) = options.##FUNC(getInt16(childVals[0])); } else { raiseUnsupportedOperator(resolved, onError); return false; }
			#define FUNC2(RET, FUNC) if (options.##FUNC) { (RET) = options.##FUNC(getInt16(childVals[0]), getInt16(childVals[1])); } else { raiseUnsupportedOperator(resolved, onError); return false; }
			auto raiseUnsupportedOperator = [] (const IToken::Ptr &resolved, ErrorHandler onError) -> void {
				if (onError) {
					const std::string msg = "Unsupported operator {0}";
					onError(Text::format(msg, { resolved->caseSensitiveText() }), resolved);
				}
			};
			Int16 result = 0;
			const Op::Types y = Op::typeOf((std::string)resolved->data());
			if (!acceptFunctionLike && isFunctionLike(y)) {
				raiseUnsupportedOperator(resolved, onError);

				return false;
			}
			switch (y) {
			case Op::Types::EQ:             result =  (getInt16(childVals[0]) == getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::LT:             result =  (getInt16(childVals[0]) <  getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::LE:             result =  (getInt16(childVals[0]) <= getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::GT:             result =  (getInt16(childVals[0]) >  getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::GE:             result =  (getInt16(childVals[0]) >= getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::NE:             result =  (getInt16(childVals[0]) != getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::AND:            result =  (getInt16(childVals[0]) && getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::OR:             result =  (getInt16(childVals[0]) || getInt16(childVals[1]) ? 1 : 0); break;
			case Op::Types::NOT:            result = (!getInt16(childVals[0])                           ? 1 : 0); break;
			case Op::Types::ADD:            result =   getInt16(childVals[0]) +  getInt16(childVals[1]);          break;
			case Op::Types::SUB:            result =   getInt16(childVals[0]) -  getInt16(childVals[1]);          break;
			case Op::Types::MUL:            result =   getInt16(childVals[0]) *  getInt16(childVals[1]);          break;
			case Op::Types::DIV: {
					const Int16 div = getInt16(childVals[1]);
					if (div == 0) {
						if (onError)
							onError("Divided by zero", resolved);

						return false;
					}

					result = getInt16(childVals[0]) / div;
				}

				break;
			case Op::Types::MOD: {
					const Int16 div = getInt16(childVals[1]);
					if (div == 0) {
						if (onError)
							onError("Divided by zero", resolved);

						return false;
					}

					result = getInt16(childVals[0]) % div;
				}

				break;
			case Op::Types::BITWISE_AND:    result =   getInt16(childVals[0]) &  getInt16(childVals[1]);          break;
			case Op::Types::BITWISE_OR:     result =   getInt16(childVals[0]) |  getInt16(childVals[1]);          break;
			case Op::Types::BITWISE_XOR:    result =   getInt16(childVals[0]) ^  getInt16(childVals[1]);          break;
			case Op::Types::BITWISE_LSHIFT: result =   getInt16(childVals[0]) << getInt16(childVals[1]);          break;
			case Op::Types::BITWISE_RSHIFT: result =   getInt16(childVals[0]) >> getInt16(childVals[1]);          break;
			case Op::Types::BITWISE_NOT:    result =  ~getInt16(childVals[0]);                                    break;
			case Op::Types::NEG:            result =  -getInt16(childVals[0]);                                    break;
			case Op::Types::SGN:            result = (Int16)Math::sign(getInt16(childVals[0]));                   break;
			case Op::Types::ABS:            result = (Int16)std::abs(getInt16(childVals[0]));                     break;
			case Op::Types::SQR:            result = getInt16(childVals[0]) * getInt16(childVals[0]);             break;
			case Op::Types::SQRT:           FUNC1(result, sqrt);                                                  break;
			case Op::Types::SIN:            FUNC1(result, sin);                                                   break;
			case Op::Types::COS:            FUNC1(result, cos);                                                   break;
			case Op::Types::ATAN2:          FUNC2(result, atan2);                                                 break;
			case Op::Types::POWI:           FUNC2(result, powi);                                                  break;
			case Op::Types::MIN:            result = Math::min(getInt16(childVals[0]), getInt16(childVals[1]));   break;
			case Op::Types::MAX:            result = Math::max(getInt16(childVals[0]), getInt16(childVals[1]));   break;
			default:
				raiseUnsupportedOperator(resolved, onError);

				return false;
			}
			#undef FUNC1
			#undef FUNC2

			stk.push(Variant(result));
		}
	}

	if (stk.size() != 1) {
		if (onError)
			onError("Incomplete expression", rpn.empty() ? nullptr : rpn.back());

		return false;
	}

	// Finish.
	Variant ret_ = stk.top();

#if EVALUATOR_DEBUG
	const std::string origin = toInfixNotationString(rpn, onError);
	const std::string calculated = ret_.toString();

	fprintf(stdout, "Calculated %s from %s\n", calculated.c_str(), origin.c_str());
#endif /* EVALUATOR_DEBUG */

	if (ret)
		std::swap(*ret, ret_);

	return true;
}

}

/* ===========================================================================} */
