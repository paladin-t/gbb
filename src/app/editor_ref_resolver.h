/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_REF_RESOLVER_H__
#define __EDITOR_REF_RESOLVER_H__

#include "widgets.h"

/*
** {===========================================================================
** Ref resolver editor
*/

class EditorRefResolver {
public:
	GBBASIC_CLASS_TYPE('R', 'S', 'V', 'E')

	static EditorRefResolver* create(void);
	static void destroy(EditorRefResolver* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_REF_RESOLVER_H__ */
