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

/*
** {===========================================================================
** Debugger
*/

class Debugger {
public:
	virtual bool open(class Renderer* rnd, class Theme* theme) = 0;
	virtual bool close(void) = 0;

	virtual void update(
		class Renderer* rnd, class Theme* theme, class Device* device
	) = 0;

	static Debugger* create(void);
	static void destroy(Debugger* ptr);
};

/* ===========================================================================} */

#endif /* __DEBUGGER_H__ */
