/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor.h"
#include "../utils/datetime.h"
#include "../../lib/imgui/imgui.h"

/*
** {===========================================================================
** Editor
*/

float Editor::Ref::windowWidth(float exp) {
	const float w = ImGui::GetContentRegionAvail().x;
	if (_refreshingTicks < 1) // Delay 1 frame.
		++_refreshingTicks;
	else
		_verticalScrollBarVisible = w < std::floor(exp);

	return w;
}

int Editor::Ref::windowFlags(void) const {
	return (_verticalScrollBarVisible ? ImGuiWindowFlags_AlwaysVerticalScrollbar : ImGuiWindowFlags_None) | ImGuiWindowFlags_NoNav;
}

void Editor::Ref::windowResized(void) {
	_verticalScrollBarVisible = false;
	_refreshingTicks = 0;
}

Editor::Ref::Splitter Editor::Ref::split(void) {
	ImGuiStyle &style = ImGui::GetStyle();

	const ImVec2 content = ImGui::GetContentRegionAvail();
	const float toolsWidth = _verticalScrollBarVisible ?
		24 * 5 + style.ScrollbarSize + 2 :
		24 * 5 + 2;
	const float paintingWidth = content.x - toolsWidth;

	return std::make_pair(paintingWidth, toolsWidth);
}

Editor::Debounce::Debounce() {
	_interval = DateTime::fromSeconds(EDITOR_DEBOUNCE_INTERVAL); // Debounce for 5s.
}

Editor::Debounce::Debounce(double intervalSeconds) {
	_interval = DateTime::fromSeconds(intervalSeconds);
}

Editor::Debounce::~Debounce() {
}

void Editor::Debounce::trigger(void) {
	_manuallyTriggered = true;
}

void Editor::Debounce::modified(void) {
	_lastModificationTimestamp = DateTime::ticks();
}

bool Editor::Debounce::fire(void) {
	if (_manuallyTriggered) {
		_lastModificationTimestamp = 0;
		_manuallyTriggered = false;

		return true;
	}

	if (_lastModificationTimestamp == 0)
		return false;

	const long long now = DateTime::ticks();
	const long long diff = now - _lastModificationTimestamp;
	if (diff >= _interval) {
		_lastModificationTimestamp = 0;

		return true;
	}

	return false;
}

void Editor::Debounce::clear(void) {
	_lastModificationTimestamp = 0;
}

/* ===========================================================================} */
