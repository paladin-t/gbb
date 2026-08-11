/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "../gbbasic.h"
#include <vector>

/*
** {===========================================================================
** Debugger
*/

class Debugger {
public:
	struct Breakpoint {
		typedef std::vector<Breakpoint> Array;

		bool enabled = true;
		int page = 0;
		int line = 0;

		Breakpoint();
		Breakpoint(bool enabled_, int pg, int ln);
	};

public:
	virtual bool open(class Renderer* rnd, class Theme* theme) = 0;
	virtual bool close(void) = 0;

	virtual int safeHeight(void) const = 0;

	virtual void update(
		class Renderer* rnd, class Theme* theme, class Device* device
	) = 0;

	virtual void clearBreakpoints(void) = 0;
	virtual void setBreakpoint(int page, int ln, bool brk) = 0;

	static Debugger* create(void);
	static void destroy(Debugger* ptr);
};

/* ===========================================================================} */

#endif /* __DEBUGGER_H__ */
