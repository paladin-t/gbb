/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_MAP_H__
#define __EDITOR_MAP_H__

#include "editor.h"

/*
** {===========================================================================
** Map editor
*/

class EditorMap : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('M', 'A', 'P', 'E')

	static EditorMap* create(void);
	static void destroy(EditorMap* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_MAP_H__ */
