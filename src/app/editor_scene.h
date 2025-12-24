/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_SCENE_H__
#define __EDITOR_SCENE_H__

#include "editor.h"

/*
** {===========================================================================
** Scene editor
*/

class EditorScene : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('S', 'C', 'N', 'E')

	static EditorScene* create(void);
	static void destroy(EditorScene* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_SCENE_H__ */
