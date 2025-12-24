/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_ref_resolver.h"

/*
** {===========================================================================
** Ref resolver editor
*/

class EditorRefResolverImpl : public EditorRefResolver {
};

EditorRefResolver* EditorRefResolver::create(void) {
	EditorRefResolverImpl* result = new EditorRefResolverImpl();

	return result;
}

void EditorRefResolver::destroy(EditorRefResolver* ptr) {
	EditorRefResolverImpl* impl = static_cast<EditorRefResolverImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
