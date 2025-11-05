/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_ARBITRARY_H__
#define __EDITOR_ARBITRARY_H__

#include "widgets.h"

/*
** {===========================================================================
** Arbitrary editor
*/

class EditorArbitrary : public ImGui::EditorPopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct AppliedHandler : public Handler<AppliedHandler, void> {
		using Handler::Handler;
	};

public:
	static EditorArbitrary* create(
		Renderer* rnd, class Workspace* ws,
		class Theme* theme,
		ChangedHandler changed,
		const std::string &title,
		class Project* prj, int fontIndex,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	static void destroy(EditorArbitrary* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_ARBITRARY_H__ */
