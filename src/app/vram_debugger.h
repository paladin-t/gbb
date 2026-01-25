/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __VRAM_DEBUGGER_H__
#define __VRAM_DEBUGGER_H__

#include "../gbbasic.h"

/*
** {===========================================================================
** VRAM debugger
*/

class VramDebugger {
public:
	virtual bool open(class Renderer* rnd, class Theme* theme) = 0;
	virtual bool close(void) = 0;

	virtual void update(
		class Renderer* rnd, class Theme* theme, class Device* device,
		bool previewPaletteBits, bool showGrids
	) = 0;

	static VramDebugger* create(void);
	static void destroy(VramDebugger* ptr);
};

/* ===========================================================================} */

#endif /* __VRAM_DEBUGGER_H__ */
