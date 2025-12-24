/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#include "../gbbasic.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef DOCUMENT_MARKDOWN_DIR
#	define DOCUMENT_MARKDOWN_DIR "../docs/" /* Relative path. */
#endif /* DOCUMENT_MARKDOWN_DIR */
#ifndef DOCUMENT_MARKDOWN_EXT
#	define DOCUMENT_MARKDOWN_EXT "md"
#endif /* DOCUMENT_MARKDOWN_EXT */

#ifndef DOCUMENT_MAIN_NAME
#	define DOCUMENT_MAIN_NAME "Manual"
#endif /* DOCUMENT_MAIN_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Document
*/

/**
 * @brief Document viewer.
 */
class Document {
public:
	virtual ~Document();

	virtual const char* title(void) = 0;

	virtual const char* shown(void) = 0;
	virtual void show(const char* doc) = 0;
	virtual void hide(void) = 0;

	virtual void bringToFront(void) = 0;

	virtual void update(class Window* wnd, class Renderer* rnd, const class Theme* theme, bool windowed) = 0;

	static Document* create(void);
	static void destroy(Document* ptr);
};

/* ===========================================================================} */

#endif /* __DOCUMENT_H__ */
