/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "evaluator.h"
#include <stack>

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

static Int16 getInt16(const Variant &v) {
	return (Int16)(int)v;
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Evaluator
*/

namespace GBBASIC {

Evaluator::Options::Options() {
}

Evaluator::Options::Options(bool ci) :
	caseInsensitive(ci)
{
}

Evaluator::Options::Options(bool ci, TokenResolver r, ErrorHandler err) :
	caseInsensitive(ci),
	resolve(r),
	onError(err)
{
}

bool Evaluator::toRpn(IToken::Array &ret, const IToken::Array &in, const Options &options) {
	// TODO
	return false;
}

bool Evaluator::fold(IToken::Array &ret, const IToken::Array &rpn, const Options &options) {
	// Prepare.
	typedef std::function<TreeNode::Ptr(const IToken::Array &, TokenResolver, ErrorHandler)> Parser;
	typedef std::function<Maybe<Variant>(const TreeNode::Ptr &, ErrorHandler)> Folder;
	typedef std::function<bool(IToken::Array &, const TreeNode::Ptr &, ErrorHandler)> Builder;

	TokenResolver resolve = options.resolve;
	ErrorHandler onError = options.onError;

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
	fold = [&fold] (const TreeNode::Ptr &ast, ErrorHandler onError) -> Maybe<Variant> {
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

		Int16 result = 0;
		const Op::Types y = Op::typeOf((std::string)tk->data());
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
		case Op::Types::SGN:            // Fall through.
		case Op::Types::ABS:            // Fall through.
		case Op::Types::SQR:            // Fall through.
		case Op::Types::SQRT:           // Fall through.
		case Op::Types::SIN:            // Fall through.
		case Op::Types::COS:            // Fall through.
		case Op::Types::ATAN2:          // Fall through.
		case Op::Types::POWI:           // Fall through.
		case Op::Types::MIN:            // Fall through.
		case Op::Types::MAX:            // Fall through.
		default:
			return Maybe<Variant>();
		}

		return Variant(result);
	};

	// Rebuilding RPN.
	Builder astToRpn = nullptr;
	astToRpn = [&options, &fold, &astToRpn] (IToken::Array &rpn, const TreeNode::Ptr &ast, ErrorHandler onError) -> bool {
		if (!ast)
			return false;

		const IToken::Ptr &tk = ast->token();
		const Maybe<Variant> val = fold(ast, onError);
		if (!val.empty()) {
			const Variant var = val.get();
			const IToken::Types y = IToken::Types::NUMBER;
			const TextLocation &begin = tk->begin();
			const TextLocation &end = tk->end();
			rpn.push_back(IToken::Ptr(IToken::create(y, var.toString(), options.caseInsensitive, &begin, &end)));

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
	ret.clear();
	const TreeNode::Ptr ast = rpnToAst(rpn, resolve, onError);
	if (!ast)
		return false;
	if (!astToRpn(ret, ast, onError))
		return false;
	
#if defined GBBASIC_DEBUG
	typedef std::function<std::string(const IToken::Array &, ErrorHandler)> Formatter;

	Formatter toInfixNotation = nullptr;
	toInfixNotation = [&toInfixNotation] (const IToken::Array &rpn, ErrorHandler onError) -> std::string {
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
	};

	const std::string origin = toInfixNotation(rpn, onError);
	const std::string folded = toInfixNotation(ret, onError);

	fprintf(stdout, "Folded %s\n    to %s\n", origin.c_str(), folded.c_str());
#endif /* GBBASIC_DEBUG */

	return true;
}

bool Evaluator::calc(Variant &ret, const IToken::Array &rpn, const Options &options) {
	// TODO
	return false;
}

}

/* ===========================================================================} */
