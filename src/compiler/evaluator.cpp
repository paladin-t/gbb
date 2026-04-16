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

class Node {
public:
	typedef std::shared_ptr<Node> Ptr;
	typedef std::vector<Ptr> Array;
	typedef std::stack<Ptr> Stack;

private:
	IToken::Ptr _token = nullptr;
	Array _children;

public:
	Node(const IToken::Ptr &tk) :
		_token(tk)
	{
	}
	~Node() {
	}

	IToken::Ptr getToken(void) const {
		return _token;
	}

	const Array &getChildren(void) const {
		return _children;
	}
	void addChild(const Ptr &child) {
		if (!child)
			return;

		_children.push_back(child);
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
	// TODO: FOR CONSTANT FOLDING.

	return 2; 
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

	(void)ret; 

	auto rpnToTree = [] (const IToken::Array &rpn, TokenResolver resolve, ErrorHandler onError) -> Node::Ptr {
		Node::Stack stk;

		for (const IToken::Ptr &tk : rpn) {
			if (!tk)
				continue;

			const IToken::Ptr token = tk;//resolve ? resolve(tk) : nullptr;
			const Node::Ptr curNode(new Node(token ? token : tk));
			if (isOperator(tk)) {
				const int arity = getArity(tk);
				if ((int)stk.size() < arity) {
					if (onError)
						onError("Too few arguments", tk);

					return false;
				}

				for (int i = 0; i < arity; ++i) {
					curNode->addChild(stk.top());
					stk.pop();
				}
			}

			stk.push(curNode);
		}

		if (stk.size() != 1) {
			if (onError)
				onError("Incomplete expression", rpn.back());

			return false;
		}

		Node::Ptr result = stk.top();

		return result;
	};

	Node::Ptr tree = rpnToTree(rpn, resolve, onError);

	return true; 
}

}

/* ===========================================================================} */
