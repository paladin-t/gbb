/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_PALETTE_H__
#define __EDITOR_PALETTE_H__

#include "widgets.h"

/*
** {===========================================================================
** Palette editor
*/

class EditorPalette : public ImGui::EditorPopupBox {
public:
	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const PaletteAssets &> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct AppliedHandler : public Handler<AppliedHandler, void, const PaletteAssets &> {
		using Handler::Handler;
	};

public:
	static EditorPalette* create(
		Renderer* rnd, class Workspace* ws,
		class Theme* theme,
		int group, ChangedHandler changed,
		const std::string &title,
		class Project* prj,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	static void destroy(EditorPalette* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_PALETTE_H__ */
