/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITOR_PROPERTIES_H__
#define __EDITOR_PROPERTIES_H__

#include "widgets.h"

/*
** {===========================================================================
** Properties editor
*/

class EditorProperties : public ImGui::EditorPopupBox {
public:
	typedef std::vector<Image::Ptr> ImageArray;

	struct ConfirmedHandler : public Handler<ConfirmedHandler, void, const ImageArray &> {
		using Handler::Handler;
	};
	struct CanceledHandler : public Handler<CanceledHandler, void> {
		using Handler::Handler;
	};
	struct AppliedHandler : public Handler<AppliedHandler, void, const ImageArray &> {
		using Handler::Handler;
	};

public:
	static EditorProperties* create(
		Renderer* rnd, class Workspace* ws,
		class Theme* theme,
		Actor::Ptr obj, int layer,
		const Actor::Shadow::Array &shadow,
		const Text::Array &frameNames,
		ChangedHandler changed,
		const std::string &title,
		class Project* prj,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	static void destroy(EditorProperties* ptr);
};

/* ===========================================================================} */

#endif /* __EDITOR_PROPERTIES_H__ */
