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
	virtual bool open(class Renderer* rnd, class Theme* theme, class Device* device) = 0;
	virtual bool close(void) = 0;

	virtual int safeHeight(void) const = 0;

	virtual int highlightCount(void) const = 0;
	/**
	 * @param[out] area
	 */
	virtual bool getHighlight(int index, Math::Recti* area /* nullable */) const = 0;

	virtual void update(
		class Renderer* rnd, class Theme* theme,
		bool previewPaletteBits, bool showGrids,
		bool isNewFrame,
		bool showTitle
	) = 0;

	static VramDebugger* create(void);
	static void destroy(VramDebugger* ptr);
};

/* ===========================================================================} */

#endif /* __VRAM_DEBUGGER_H__ */
