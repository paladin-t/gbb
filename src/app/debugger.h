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
	struct FarPtr {
		int bank = -1;
		int address = -1;

		FarPtr();
		FarPtr(int b, int addr);

		bool equals(int b, int addr) const;
		bool invalid(void) const;
	};

	struct Breakpoint {
		typedef std::vector<Breakpoint> Array;

		enum class Types {
			NONE,
			BASIC,
			ASM
		};

		int page = 0;
		int row = 0; // 1-based.

		bool enabled = true;
		Types type = Types::NONE;
		int id = -1;
		FarPtr hitPointer;
		FarPtr vmPointer;

		Breakpoint();
		Breakpoint(int pg, int ln);
		Breakpoint(int pg, int ln, bool enabled_);

		bool operator < (const Breakpoint &other) const;

		int compare(const Breakpoint &other) const;
	};

public:
	virtual bool open(class Renderer* rnd, class Workspace* ws, class Theme* theme, class Device* device) = 0;
	virtual bool close(void) = 0;

	virtual int safeHeight(void) const = 0;

	virtual void update(
		class Renderer* rnd, class Theme* theme,
		bool visible,
		bool showTitle
	) = 0;

	virtual void start(void) = 0;
	virtual void stop(void) = 0;

	virtual void pause(void) = 0;
	virtual void resume(void) = 0;

	virtual void clearBreakpoints(void) = 0;
	virtual void setBreakpoint(int page, int ln, bool brk) = 0;
	virtual void removeBreakpoint(int page, int ln) = 0;

	virtual void step(void) = 0;

	virtual bool breakpointHit(void) = 0;

	static Debugger* create(void);
	static void destroy(Debugger* ptr);
};

/* ===========================================================================} */

#endif /* __DEBUGGER_H__ */
