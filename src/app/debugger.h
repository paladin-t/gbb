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
#include "../utils/mathematics.h"
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

	enum class Categories {
		NONE,
		BASIC,
		ASM
	};

	struct Breakpoint {
		typedef std::vector<Breakpoint> Array;

		int page = 0;
		int row = 0; // 1-based.

		bool enabled = true;
		Categories type = Categories::NONE;
		int id = -1;
		FarPtr hitPointer;
		FarPtr vmPointer;
		mutable unsigned hitCount = 0;

		Breakpoint();
		Breakpoint(int pg, int ln);
		Breakpoint(int pg, int ln, bool enabled_);

		bool operator < (const Breakpoint &other) const;

		int compare(const Breakpoint &other) const;
	};

	struct Highlight {
		Math::Recti area;
		Math::Vec4f color;

		Highlight();
		Highlight(const Math::Recti &a, const Math::Vec4f &col);
	};

public:
	virtual bool open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Theme* theme, class Device* device) = 0;
	virtual bool close(void) = 0;

	virtual int safeHeight(void) const = 0;

	virtual int highlightCount(void) const = 0;
	/**
	 * @param[out] highlight
	 */
	virtual bool getHighlight(int index, Highlight* highlight /* nullable */) const = 0;

	virtual void update(bool visible, bool showTitle, bool showObjBounds) = 0;

	virtual void start(void) = 0;
	virtual void stop(void) = 0;

	virtual void pause(void) = 0;
	virtual void resume(void) = 0;

	virtual void clearBreakpoints(void) = 0;
	virtual void setBreakpoint(int page, int ln, bool brk) = 0;
	virtual void removeBreakpoint(int page, int ln) = 0;
	virtual void toggleBreakpoint(void) = 0;

	virtual void step(bool toNextAsmInst) = 0;

	virtual bool breakpointHit(void) = 0;

	static Debugger* create(void);
	static void destroy(Debugger* ptr);
};

/* ===========================================================================} */

#endif /* __DEBUGGER_H__ */
