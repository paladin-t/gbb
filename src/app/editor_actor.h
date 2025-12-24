/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_ACTOR_H__
#define __EDITOR_ACTOR_H__

#include "editor.h"

/*
** {===========================================================================
** Actor editor
*/

class EditorActor : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('A', 'C', 'T', 'R')

	static EditorActor* create(void);
	static void destroy(EditorActor* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_ACTOR_H__ */
