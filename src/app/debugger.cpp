/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "debugger.h"

/*
** {===========================================================================
** Debugger
*/

class DebuggerImpl : public Debugger {
private:
	bool _opened = false;

public:
	DebuggerImpl() {
	}
	virtual ~DebuggerImpl() {
		close();
	}

	virtual bool open(class Renderer* rnd, class Theme* /* theme */) override {
		if (_opened)
			return true;

		// TODO
		(void)rnd;

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		// TODO

		_opened = false;

		return true;
	}

	virtual void update(
		class Renderer* rnd, class Theme* theme, class Device* device
	) override {
		// TODO
		(void)rnd;
		(void)theme;
		(void)device;
	}

};

Debugger* Debugger::create(void) {
	DebuggerImpl* result = new DebuggerImpl();

	return result;
}

void Debugger::destroy(Debugger* ptr) {
	DebuggerImpl* impl = static_cast<DebuggerImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
