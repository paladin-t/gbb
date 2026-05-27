/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_CODE_H__
#define __EDITOR_CODE_H__

#include "editor.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH
#	define EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH 186
#endif /* EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH */

/* ===========================================================================} */

/*
** {===========================================================================
** Code editor
*/

class EditorCode : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('C', 'O', 'D', 'E')

	virtual void addKeyword(const char* str) = 0;
	virtual void addSymbol(const char* str) = 0;
	virtual void addIdentifier(const char* str) = 0;
	virtual void addPreprocessor(const char* str) = 0;
	virtual void addAsmSymbol(const char* str) = 0;

	virtual const std::string &toString(void) const = 0;
	virtual void fromString(const char* txt, size_t len = 0) = 0;

	virtual bool isToolBarVisible(void) const = 0;

	virtual void renderStatus(class Window* wnd, class Renderer* rnd, class Workspace* ws, float x, float y, float width, float height) = 0;

	static EditorCode* create(class Workspace* ws, bool isMajor);
	static void destroy(EditorCode* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_CODE_H__ */
