/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_CONSOLE_H__
#define __EDITOR_CONSOLE_H__

#include "../utils/editable.h"

/*
** {===========================================================================
** Console editor
*/

class EditorConsole : public Editable, public virtual Object {
public:
	typedef std::function<void(int, bool)> LineClickedHandler;

public:
	GBBASIC_CLASS_TYPE('C', 'O', 'N', 'S')

	virtual const std::string &toString(void) const = 0;
	virtual void fromString(const char* txt, size_t len = 0) = 0;

	virtual void appendText(const char* txt, unsigned col) = 0;
	virtual void moveBottom(bool select = false) = 0;

	virtual void setLineClickedHandler(LineClickedHandler handler) = 0;

	static EditorConsole* create(void);
	static void destroy(EditorConsole* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_CONSOLE_H__ */
