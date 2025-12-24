/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_TILES_H__
#define __EDITOR_TILES_H__

#include "editor.h"

/*
** {===========================================================================
** Tiles editor
*/

class EditorTiles : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('T', 'I', 'L', 'E')

	static EditorTiles* create(void);
	static void destroy(EditorTiles* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_TILES_H__ */
