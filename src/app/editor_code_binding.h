/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_CODE_BINDER_H__
#define __EDITOR_CODE_BINDER_H__

#include "widgets.h"

/*
** {===========================================================================
** Code binding editor
*/

class EditorCodeBinding : public ImGui::EditorPopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct GotoHandler : public Handler<GotoHandler, void, int, int> {
		using Handler::Handler;
	};

public:
	virtual void addKeyword(const char* str) = 0;
	virtual void addSymbol(const char* str) = 0;
	virtual void addIdentifier(const char* str) = 0;
	virtual void addPreprocessor(const char* str) = 0;

	virtual const std::string toString(void) const = 0;
	virtual void fromString(const char* txt, size_t len = 0) = 0;

	static EditorCodeBinding* create(
		Renderer* rnd, class Workspace* ws,
		class Theme* theme,
		ChangedHandler changed,
		const std::string &title,
		class Project* prj,
		const std::string &index,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const GotoHandler &goto_,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* gotoTxt /* nullable */
	);
	static void destroy(EditorCodeBinding* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_CODE_BINDER_H__ */
