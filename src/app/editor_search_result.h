/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_SEARCH_RESULT_H__
#define __EDITOR_SEARCH_RESULT_H__

#include "editing.h"
#include "../utils/editable.h"

/*
** {===========================================================================
** Search result editor
*/

class EditorSearchResult : public Editable, public virtual Object {
public:
	typedef std::function<void(int, bool)> LineClickedHandler;

public:
	GBBASIC_CLASS_TYPE('S', 'R', 'R', 'E')

	virtual const std::string &toString(void) const = 0;
	virtual void fromString(const char* txt, size_t len = 0) = 0;

	virtual void setLineClickedHandler(LineClickedHandler handler) = 0;

	static EditorSearchResult* create(void);
	static void destroy(EditorSearchResult* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_SEARCH_RESULT_H__ */
