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

	IToken::Ptr token(void) const {
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

}

/* ===========================================================================} */

/*
** {===========================================================================
** Evaluator
*/

namespace GBBASIC {

static bool isOperator(const IToken::Ptr &tk) {
	if (tk->type() == IToken::Types::OPERATOR)
		return true;

	return false;
}
static int getArity(const IToken::Ptr &tk) {
	const std::string &str = tk->text();
	const Op::Types y = Op::typeOf(str);
	const Op &op = Op::OPERATORS[(size_t)y];

	return op.oprands;
}

static bool isInt(const Variant &v) {
	return v.isNumber();
}
static Int16 getInt(const Variant &v) {
	return (Int16)(int)v;
}

bool Evaluator::fold(IToken::Array &ret, const IToken::Array &rpn, ErrorHandler onError) {
	return fold(
		ret, rpn,
		[] (const IToken::Ptr &/* tk */) -> IToken::Ptr {
			return nullptr;
		},
		onError
	);
}

bool Evaluator::fold(IToken::Array &ret, const IToken::Array &rpn, TokenResolver resolve, ErrorHandler onError) {
#if !defined GBBASIC_DEBUG
	return false;
#endif /* GBBASIC_DEBUG */

	// Prepare.
	typedef std::function<TreeNode::Ptr(const IToken::Array &, TokenResolver, ErrorHandler)> Parser;
	typedef std::function<Maybe<Variant>(const TreeNode::Ptr &, TokenResolver)> Folder;
	typedef std::function<void(IToken::Array &, const TreeNode::Ptr &, TokenResolver)> Builder;

	// Converting RPN to AST nodes.
	Parser rpnToAst = [] (const IToken::Array &rpn, TokenResolver resolve, ErrorHandler onError) -> TreeNode::Ptr {
		TreeNode::Stack stk;
		for (const IToken::Ptr &tk : rpn) {
			if (!tk)
				continue;

			const IToken::Ptr resolved = tk;//resolve ? resolve(tk) : nullptr;
			const TreeNode::Ptr curNode(new TreeNode(resolved ? resolved : tk));
			if (isOperator(tk)) {
				const int arity = getArity(tk);
				if ((int)stk.size() < arity) {
					if (onError)
						onError("Too few arguments", tk);

					return nullptr;
				}

				for (int i = 0; i < arity; ++i) {
					curNode->add(stk.top());
					stk.pop();
				}
			}

			stk.push(curNode);
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
	fold = [&fold] (const TreeNode::Ptr &ast, TokenResolver resolve) -> Maybe<Variant> {
		typedef std::vector<Variant> Variants;

		const IToken::Ptr tk = ast->token();
		if (!isOperator(tk)) {
			const IToken::Ptr resolved = tk;//resolve ? resolve(tk) : nullptr;
			const IToken::Ptr tk = resolved ? resolved : tk;
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
			const Maybe<Variant> val = fold(child, resolve);
			if (val.empty())
				return Maybe<Variant>(); // Not foldable if there's at least one non-constant node.

			childVals.push_back(val.get());
		}

		for (const Variant &var : childVals) {
			if (!isInt(var)) {
				// TODO

				break;
			}
		}

		Int16 result = 0;
		const Op::Types y = Op::typeOf(tk->text());
		switch (y) {
		case Op::Types::ADD: result = getInt(childVals[0]) + getInt(childVals[1]); break;
		case Op::Types::SUB: result = getInt(childVals[0]) - getInt(childVals[1]); break;
		case Op::Types::MUL: result = getInt(childVals[0]) * getInt(childVals[1]); break;
		case Op::Types::DIV: {
				const Int16 div = getInt(childVals[1]);
				if (div == 0)
					return Maybe<Variant>(); // Avoid divided by zero.

				result = getInt(childVals[0]) / div;
			}

			break;
		case Op::Types::MOD: {
				const Int16 div = getInt(childVals[1]);
				if (div == 0)
					return Maybe<Variant>(); // Avoid divided by zero.

				result = getInt(childVals[0]) % div;
			}

			break;
		case Op::Types::NEG: result = -getInt(childVals[0]); break;
		case Op::Types::ABS: result = (Int16)std::abs(getInt(childVals[0])); break;
		case Op::Types::SQR: result = getInt(childVals[0]) * getInt(childVals[0]); break;
		case Op::Types::SQRT: result = (Int16)std::sqrt(getInt(childVals[0])); break;
		default:
			return Maybe<Variant>();
		}

		return Variant(result);
	};

	// Rebuilding RPN.
	Builder astToRpn = nullptr;
	astToRpn = [&fold, &astToRpn](IToken::Array &rpn, const TreeNode::Ptr &ast, TokenResolver resolve) -> void {
		const Maybe<Variant> val = fold(ast, resolve);
		if (!val.empty()) {
			const Variant var = val.get();
			const IToken::Types y = IToken::Types::INTEGER;
			rpn.push_back(IToken::Ptr(IToken::create(y, var)));

			return;
		}

		if (!isOperator(ast->token())) {
			const IToken::Ptr tk = ast->token();
			const IToken::Ptr resolved = tk;//resolve ? resolve(tk) : nullptr;
			rpn.push_back(resolved ? resolved : tk);

			return;
		}

		const TreeNode::Array &children = ast->children();
		for (const TreeNode::Ptr &child : children) {
			astToRpn(rpn, child, resolve);
		}
		rpn.push_back(ast->token());
	};

	// Fold the expression.
	ret.clear();

	const TreeNode::Ptr ast = rpnToAst(rpn, resolve, onError);
	if (!ast)
		return false;

	astToRpn(ret, ast, resolve);
	
	return true;
}

}

/* ===========================================================================} */
