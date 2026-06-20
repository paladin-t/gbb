/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_I18N_H__
#define __EDITOR_I18N_H__

#include "editor.h"

/*
** {===========================================================================
** I18n editor
*/

class EditorI18n : public Editor, public virtual Object {
public:
	GBBASIC_CLASS_TYPE('I', '1', '8', 'E')

	static EditorI18n* create(void);
	static void destroy(EditorI18n* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_I18N_H__ */
