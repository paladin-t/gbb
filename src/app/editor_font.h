/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_FONT_H__
#define __EDITOR_FONT_H__

#include "editor.h"

/*
** {===========================================================================
** Font editor
*/

class EditorFont : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('F', 'N', 'T', 'E')

	static EditorFont* create(void);
	static void destroy(EditorFont* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_FONT_H__ */
