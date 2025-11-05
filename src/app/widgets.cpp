/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "document.h"
#include "theme.h"
#include "widgets.h"
#include "workspace.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#include "../utils/generic.h"
#include "../../lib/civetweb/include/civetweb.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/lz4/lib/lz4.h"
#include "../../lib/zlib/zlib.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WIDGETS_ABOUT_REVISION
#	define WIDGETS_ABOUT_REVISION "r29"
#endif /* WIDGETS_ABOUT_REVISION */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

bool operator == (const ImVec2 &left, const ImVec2 &right) {
	return left.x == right.x && left.y == right.y;
}

bool operator != (const ImVec2 &left, const ImVec2 &right) {
	return left.x != right.x || left.y != right.y;
}

/* ===========================================================================} */

/*
** {===========================================================================
** ImGui widgets
*/

namespace ImGui {

bool Initializer::begin(void) const {
	return _ticks == 0 || _ticks == 1;
}

bool Initializer::end(void) const {
	return _ticks >= 2;
}

void Initializer::update(void) {
	if (_ticks < 2)
		++_ticks;
}

void Initializer::reset(void) {
	_ticks = 0;
}

Hierarchy::Hierarchy(BeginHandler begin, EndHandler end) : _begin(begin), _end(end) {
}

void Hierarchy::prepare(void) {
	_opened.push(true);
}

void Hierarchy::finish(void) {
	while (_opened.size() > 1) {
		if (_opened.top() && _end)
			_end();
		_opened.pop();
	}
}

bool Hierarchy::with(Text::Array::const_iterator begin, Text::Array::const_iterator end) {
	Compare::diff(begin, end, _path.begin(), _path.end(), &_dec, &_inc); // Calculate difference between the current entry and the last `path`.
	_path.assign(begin, end); // Assign for calculation during the next loop step.

	for (int i = 0; i < _dec; ++i) {
		if (_opened.top() && _end)
			_end();
		_opened.pop();
	}
	for (const std::string &dir : _inc) {
		if (_opened.top()) {
			if (_begin)
				_opened.push(_begin(dir));
			else
				_opened.push(false);
		} else {
			_opened.push(false);
		}
	}

	return _opened.top();
}

Bubble::Bubble(
	const std::string &content,
	const TimeoutHandler &timeout
) : _content(content),
	_timeoutHandler(timeout)
{
}

Bubble::Bubble(
	const std::string &content,
	const TimeoutHandler &timeout,
	bool exclusive_
) : _content(content),
	_timeoutHandler(timeout)
{
	_exclusive = exclusive_;
}

Bubble::~Bubble() {
}

bool Bubble::exclusive(void) const {
	return _exclusive;
}

void Bubble::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = false;
	bool toClose = false;

	const unsigned long long now = DateTime::ticks();

	if (_init.begin()) {
		_initialTime = now;
		_timeoutTime = now + DateTime::fromSeconds(1.50);
	}

	VariableGuard<decltype(style.PopupBorderSize)> guardPopupBorderSize(&style.PopupBorderSize, style.PopupBorderSize, 0.0f);

	const unsigned long long elapsed = now - _initialTime;
	const unsigned long long total = _timeoutTime - _initialTime;
	const double elapsedSec = DateTime::toSeconds(elapsed);
	const double totalSec = DateTime::toSeconds(total);
	const double fadeSec = totalSec * 0.3;
	const double factor = Math::clamp(1.0 - (elapsedSec - fadeSec) / (totalSec - fadeSec), 0.0, 1.0);
	ImVec4 col = GetStyleColorVec4(ImGuiCol_PopupBg);
	col.w = (float)factor;
	PushStyleColor(ImGuiCol_PopupBg, col);
	col = GetStyleColorVec4(ImGuiCol_Text);
	col.w = (float)factor;
	PushStyleColor(ImGuiCol_Text, col);

	const ImVec2 pos(
		io.DisplaySize.x/* * io.DisplayFramebufferScale*/ * 0.5f,
		io.DisplaySize.y/* * io.DisplayFramebufferScale*/ * 1.0f - GetTextLineHeightWithSpacing() * 3.0f
	);
	SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 1.0f));
	BeginTooltip();
	{
		isOpen = true;

		TextUnformatted(_content);

		if (now >= _timeoutTime) {
			toClose = true;

			CloseCurrentPopup();
		}
	}
	EndTooltip();

	PopStyleColor(2);

	if (isOpen)
		_init.update();

	if (!isOpen)
		toClose = true;

	if (toClose) {
		_init.reset();

		if (!_timeoutHandler.empty()) {
			_timeoutHandler();

			return;
		}
	}
}

PopupBox::PopupBox() {
}

PopupBox::~PopupBox() {
}

bool PopupBox::exclusive(void) const {
	return _exclusive;
}

void PopupBox::dispose(void) {
	// Do nothing.
}

WaitingPopupBox::WaitingPopupBox(
	const std::string &content,
	const TimeoutHandler &timeout
) : _content(content),
	_timeoutHandler(timeout)
{
}

WaitingPopupBox::WaitingPopupBox(
	bool dim, const std::string &content,
	bool instantly, const TimeoutHandler &timeout,
	bool exclusive_
) : _dim(dim), _content(content),
	_instantTimeout(instantly), _timeoutHandler(timeout)
{
	_exclusive = exclusive_;
}

WaitingPopupBox::~WaitingPopupBox() {
}

void WaitingPopupBox::dispose(void) {
	if (!_excuted && !_timeoutHandler.empty()) {
		_excuted = true;

		_timeoutHandler();
	}
}

void WaitingPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiContext &g = *GetCurrentContext();

	bool isOpen = true;
	bool toClose = false;

	const unsigned long long now = DateTime::ticks();

	if (_init.begin()) {
		OpenPopup("#Wait");

		if (_instantTimeout) {
			g.DimBgRatio = 1.0f;
			_timeoutTime = now + DateTime::fromSeconds(0.000001);
		} else {
			_timeoutTime = now + DateTime::fromSeconds(0.25);
		}
	}
	if (!_dim) {
		g.DimBgRatio = 0;
	}

	SetMouseCursor(ImGuiMouseCursor_Wait);

	const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
	SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
	if (_content.empty()) {
		PushStyleColor(ImGuiCol_WindowBg, 0);
		if (BeginPopupModal("#Wait", &isOpen, flags)) {
			if (now >= _timeoutTime) {
				toClose = true;

				CloseCurrentPopup();
			}

			EndPopup();
		}
		PopStyleColor();
	} else {
		if (BeginPopupModal("#Wait", &isOpen, flags)) {
			TextUnformatted(_content);

			if (now >= _timeoutTime) {
				toClose = true;

				CloseCurrentPopup();
			}

			EndPopup();
		}
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toClose = true;

	if (toClose) {
		_init.reset();

		if (!_excuted && !_timeoutHandler.empty()) {
			_excuted = true;

			_timeoutHandler();

			return;
		}
	}
}

MessagePopupBox::MessagePopupBox(
	const std::string &title,
	const std::string &content,
	const ConfirmedHandler &confirm, const DeniedHandler &deny, const CanceledHandler &cancel,
	const char* confirmTxt, const char* denyTxt, const char* cancelTxt,
	const char* confirmTooltips, const char* denyTooltips, const char* cancelTooltips,
	bool exclusive_
) : _title(title),
	_content(content),
	_confirmedHandler(confirm), _deniedHandler(deny), _canceledHandler(cancel)
{
	_exclusive = exclusive_;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (denyTxt)
		_denyText = denyTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;

	if (confirmTooltips)
		_confirmTooltips = confirmTooltips;
	if (denyTooltips)
		_denyTooltips = denyTooltips;
	if (cancelTooltips)
		_cancelTooltips = cancelTooltips;
}

MessagePopupBox::MessagePopupBox(
	const std::string &title,
	const std::string &content,
	const ConfirmedWithOptionHandler &confirm, const DeniedHandler &deny, const CanceledHandler &cancel,
	const char* confirmTxt, const char* denyTxt, const char* cancelTxt,
	const char* confirmTooltips, const char* denyTooltips, const char* cancelTooltips,
	const char* optionBoolTxt, bool optionBoolValue, bool optionBoolValueReadonly,
	bool exclusive_
) : _title(title),
	_content(content),
	_confirmedWithOptionHandler(confirm), _deniedHandler(deny), _canceledHandler(cancel),
	_optionBooleanText(optionBoolTxt), _optionBooleanValue(optionBoolValue), _optionBooleanValueReadonly(optionBoolValueReadonly)
{
	_exclusive = exclusive_;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (denyTxt)
		_denyText = denyTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;

	if (confirmTooltips)
		_confirmTooltips = confirmTooltips;
	if (denyTooltips)
		_denyTooltips = denyTooltips;
	if (cancelTooltips)
		_cancelTooltips = cancelTooltips;
}

MessagePopupBox::~MessagePopupBox() {
}

void MessagePopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toDeny = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		TextUnformatted(_content);

		if (!_optionBooleanText.empty()) {
			if (_optionBooleanValueReadonly) {
				BeginDisabled();
				Checkbox(_optionBooleanText, &_optionBooleanValue);
				EndDisabled();
			} else {
				Checkbox(_optionBooleanText, &_optionBooleanValue);
			}
		}

		const char* confirm = _confirmText.c_str();
		const char* deny = _denyText.empty() ? "No" : _denyText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		if (_confirmText.empty()) {
			if (_deniedHandler.empty())
				confirm = "Ok";
			else
				confirm = "Yes";
		}

		int count = 1;
		if (!_deniedHandler.empty())
			++count;
		if (!_canceledHandler.empty())
			++count;
		CentralizeButton(count);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN) || IsKeyReleased(SDL_SCANCODE_Y) || (_deniedHandler.empty() && _canceledHandler.empty() && IsKeyReleased(SDL_SCANCODE_ESCAPE))) {
			toConfirm = true;

			CloseCurrentPopup();
		}
		if (!_confirmTooltips.empty()) {
			if (IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				SetTooltip(_confirmTooltips);
			}
		}

		if (!_deniedHandler.empty()) {
			SameLine();
			if (Button(deny, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_N) || (_canceledHandler.empty() && IsKeyReleased(SDL_SCANCODE_ESCAPE))) {
				toDeny = true;

				CloseCurrentPopup();
			}
		}
		if (!_denyTooltips.empty()) {
			if (IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				SetTooltip(_denyTooltips);
			}
		}

		if (!_canceledHandler.empty()) {
			SameLine();
			if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
				toCancel = true;

				CloseCurrentPopup();
			}
		}
		if (!_cancelTooltips.empty()) {
			if (IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				SetTooltip(_cancelTooltips);
			}
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler();

			return;
		}

		if (!_confirmedWithOptionHandler.empty()) {
			_confirmedWithOptionHandler(_optionBooleanValue);

			return;
		}
	}
	if (toDeny) {
		_init.reset();

		if (!_deniedHandler.empty()) {
			_deniedHandler();

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

InputPopupBox::InputPopupBox(
	const std::string &title,
	const std::string &content, const std::string &default_, unsigned flags,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _title(title),
	_content(content), _default(default_), _flags(flags),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, _default.c_str(), Math::min(sizeof(_buffer), _default.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

InputPopupBox::~InputPopupBox() {
}

void InputPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		TextUnformatted(_content);

		if (!_init.end())
			SetKeyboardFocusHere();
		InputText("", _buffer, sizeof(_buffer), _flags | ImGuiInputTextFlags_AutoSelectAll);

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_buffer);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

StarterKitsPopupBox::StarterKitsPopupBox(
	Theme* theme,
	const std::string &title,
	const std::string &template_,
	const std::string &content, const std::string &default_, unsigned flags,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _theme(theme),
	_title(title),
	_template(template_),
	_content(content), _default(default_), _flags(flags),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, _default.c_str(), Math::min(sizeof(_buffer), _default.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

StarterKitsPopupBox::~StarterKitsPopupBox() {
}

void StarterKitsPopupBox::update(Workspace* ws) {
	struct Data {
		Theme* theme = nullptr;
		const EntryWithPath::Array* entries = nullptr;

		Data() {
		}
		Data(Theme* t, const EntryWithPath::Array* entries_) : theme(t), entries(entries_) {
		}
	};

	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		PushID("#Tmplt");
		{
			TextUnformatted(_template);

			const EntryWithPath::Array &starterKits = ws->starterKits();
			Data data(_theme, &starterKits);
			Combo(
				"",
				&_templateCursor,
				[] (void* data_, int idx, const char** outText) -> bool {
					Data* data = (Data*)data_;
					Theme* theme = data->theme;
					const EntryWithPath::Array* entries = (const EntryWithPath::Array*)data->entries;

					if (idx == 0) {
						*outText = theme->generic_None().c_str();

						return true;
					}

					const EntryWithPath &entry = (*entries)[idx - 1];
					const char* entry_ = entry.name().c_str();
					*outText = entry_ ? entry_ : "Unknown";

					return true;
				},
				(void*)&data, (int)(starterKits.size() + 1),
				[] (void* data_, int idx, const char** outText) -> bool {
					Data* data = (Data*)data_;
					const EntryWithPath::Array* entries = (const EntryWithPath::Array*)data->entries;

					if (idx == 0) {
						*outText = nullptr;

						return true;
					}

					const EntryWithPath &entry = (*entries)[idx - 1];
					if (!entry.tooltips().empty())
						*outText = entry.tooltips().c_str();

					return true;
				},
				(void*)&data
			);
		}
		PopID();

		PushID("#Name");
		{
			TextUnformatted(_content);

			if (!_init.end())
				SetKeyboardFocusHere();
			InputText("", _buffer, sizeof(_buffer), _flags | ImGuiInputTextFlags_AutoSelectAll);
		}
		PopID();

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_templateCursor - 1, _buffer);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

SortAssetsPopupBox::SortAssetsPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	Project* project,
	unsigned category,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_project(project),
	_category(category),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
	for (int i = 0; i < _project->codePageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::CODE].push_back(i);
	for (int i = 0; i < _project->tilesPageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::TILES].push_back(i);
	for (int i = 0; i < _project->mapPageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::MAP].push_back(i);
	for (int i = 0; i < _project->scenePageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::SCENE].push_back(i);
	for (int i = 0; i < _project->actorPageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::ACTOR].push_back(i);
	for (int i = 0; i < _project->fontPageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::FONT].push_back(i);
	for (int i = 0; i < _project->musicPageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::MUSIC].push_back(i);
	for (int i = 0; i < _project->sfxPageCount(); ++i)
		_orders[(unsigned)AssetsBundle::Categories::SFX].push_back(i);
	_ordersShadow = _orders;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

SortAssetsPopupBox::~SortAssetsPopupBox() {
}

void SortAssetsPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	AssetsBundle::Categories selectTab = AssetsBundle::Categories::NONE;
	if (_init.begin()) {
		selectTab = (AssetsBundle::Categories)_category;

		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 380.0f);
	const float height = Math::min(width, _renderer->height() * 0.8f - 48.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		if (BeginTabBar("@Pref")) {
			if (BeginTabItem(_theme->menu_Code(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::CODE ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Cd", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::CODE];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							const Text::Array &assetNames = ws->getCodePageNames();
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								const std::string &name = assetNames[idx];

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Tiles(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::TILES ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Tls", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::TILES];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								TilesAssets::Entry* entry = _project->getTiles(idx);
								const std::string &name = entry->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Map(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::MAP ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Map", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::MAP];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								MapAssets::Entry* entry = _project->getMap(idx);
								const std::string &name = entry->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Scene(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::SCENE ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Sc", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::SCENE];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								SceneAssets::Entry* entry = _project->getScene(idx);
								const std::string &name = entry->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Actor(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::ACTOR ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Act", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::ACTOR];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								ActorAssets::Entry* entry = _project->getActor(idx);
								const std::string &name = entry->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Font(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::FONT ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Fnt", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::FONT];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								FontAssets::Entry* entry = _project->getFont(idx);
								const std::string &name = entry->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Music(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::MUSIC ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Msc", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::MUSIC];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								MusicAssets::Entry* entry = _project->getMusic(idx);
								const std::string &name = entry->data->pointer()->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->menu_Sfx(), nullptr, ImGuiTabItemFlags_NoTooltip | (selectTab == AssetsBundle::Categories::SFX ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), _theme->style()->tabTextColor)) {
				{
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

					BeginChild("@Sfx", ImVec2(width - style.WindowPadding.x * 2, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
					{
						Order &order = _ordersShadow[(unsigned)AssetsBundle::Categories::SFX];
						if (order.empty()) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->generic_Empty());
						} else {
							for (int i = 0; i < (int)order.size(); ++i) {
								const int idx = order[i];
								SfxAssets::Entry* entry = _project->getSfx(idx);
								const std::string &name = entry->data->pointer()->name;

								PushID(name);
								{
									VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

									const ImVec2 spos = GetCursorScreenPos();
									const ImVec2 pos = GetCursorPos();
									const ImVec2 size(width - style.ChildBorderSize - style.ScrollbarSize - style.WindowPadding.x, 19.0f);
									Dummy(
										size,
										GetStyleColorVec4(IsMouseHoveringRect(spos, spos + size) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg)
									);
									SetCursorPos(pos);

									if (i == 0) {
										BeginDisabled();
										{
											ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconUp()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i - 1]);
										}
									}
									SameLine();
									if (i == (int)order.size() - 1) {
										BeginDisabled();
										{
											ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
										}
										EndDisabled();
									} else {
										if (ImageButton(_theme->iconDown()->pointer(_renderer), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
											std::swap(order[i], order[i + 1]);
										}
									}
									SameLine();

									Dummy(ImVec2(4, 0));
									SameLine();

									AlignTextToFramePadding();
									TextUnformatted(name);
								}
								PopID();
							}
						}
					}
					EndChild();
				}

				EndTabItem();
			}

			EndTabBar();
		}
		Dummy(ImVec2(0, 0));

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		const bool appliable = _orders != _ordersShadow;

		CentralizeButton(2);

		if (appliable) {
			if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
				toConfirm = true;

				CloseCurrentPopup();
			}
		} else {
			BeginDisabled();
			Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			EndDisabled();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_ordersShadow);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

RomBuildSettingsPopupBox::RomBuildSettingsPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	Project* project,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_project(project),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;

	_cartType = _project->cartridgeType();
	_sramType = _project->sramType();
	_hasRtc = _project->hasRtc();
}

RomBuildSettingsPopupBox::~RomBuildSettingsPopupBox() {
}

void RomBuildSettingsPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 380.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		AlignTextToFramePadding();
		TextUnformatted(_theme->windowBuildingSettings_CartridgeTypeOfOutputRom());

		PushID("#CrtTyp");
		{
			AlignTextToFramePadding();
			TextUnformatted(_theme->windowBuildingSettings_Cart());

			SameLine();

			const char* items[] = {
				PROJECT_CARTRIDGE_TYPE_CLASSIC,
				PROJECT_CARTRIDGE_TYPE_COLORED,
				PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED,
				PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION,
				PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION,
				PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION
			};
			int pref = 0;
			for (int i = 0; i < GBBASIC_COUNTOF(items); ++i) {
				if (_cartType == items[i]) {
					pref = i;

					break;
				}
			}
			SetNextItemWidth(GetContentRegionAvail().x);
			if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
				_cartType = items[pref];
		}
		PopID();

		PushID("#SrmTyp");
		{
			const float x = GetCursorPosX();
			const float w = GetWindowContentRegionMax().x - GetWindowContentRegionMin().x - x;

			AlignTextToFramePadding();
			TextUnformatted(_theme->windowBuildingSettings_Sram());

			SameLine();

			const char* items[] = {
				_theme->windowBuildingSettings_Sram_None().c_str(),
				"2KB",
				"8KB",
				"32KB",
				"128KB"
			};
			int pref = 0;
			if (!Text::fromString(_sramType, pref))
				pref = 3;
			const float u = GetCursorPosX() - x;
			SetNextItemWidth(w * 0.5f - u);
			if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
				_sramType = Text::toString(pref);
		}
		PopID();

		PushID("#HasRtc");
		{
			SameLine();
			TextUnformatted(_theme->windowBuildingSettings_Rtc());

			SameLine();

			bool pref = _hasRtc;
			if (Checkbox(_theme->generic_Enabled(), &pref))
				_hasRtc = pref;
		}
		PopID();

		const char* confirm = _confirmText.empty() ? "Build" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_cartType.c_str(), _sramType.c_str(), _hasRtc);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

EmulatorBuildSettingsPopupBox::EmulatorBuildSettingsPopupBox(
	Renderer* rnd,
	Input* input, Theme* theme,
	const std::string &title,
	const std::string &settings, const char* args, bool hasIcon,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _renderer(rnd),
	_input(input), _theme(theme),
	_title(title),
	_hasIcon(hasIcon),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
	_settings.fromString(settings);

	_hasArgs = !!args;
	memset(_argsBuffer, 0, sizeof(_argsBuffer));
	if (args)
		memcpy(_argsBuffer, args, Math::min(sizeof(_argsBuffer), strlen(args)));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

EmulatorBuildSettingsPopupBox::~EmulatorBuildSettingsPopupBox() {
}

void EmulatorBuildSettingsPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 480.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		if (BeginTabBar("@Pref")) {
			if (BeginTabItem(_theme->tabPreferences_Emulator(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Main_Application());

				Checkbox(_theme->windowPreferences_Emulator_ShowTitleBar(), &_settings.emulatorShowTitleBar);

				Checkbox(_theme->windowPreferences_Emulator_ShowStatusBar(), &_settings.emulatorShowStatusBar);

				Checkbox(_theme->windowPreferences_Emulator_ShowPreferenceDialogOnEscPress(), &_settings.emulatorShowPreferenceDialogOnEscPress);

				if (_hasArgs) {
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowPreferences_Emulator_LaunchOptions());
					SameLine();

					SetNextItemWidth(GetContentRegionAvail().x);
					PushID("#Args");
					InputText("", _argsBuffer, sizeof(_argsBuffer), ImGuiInputTextFlags_AutoSelectAll);
					PopID();
				}

				if (_hasIcon) {
					PushID("#Icon");
					{
						const float width = GetContentRegionMax().x;

						AlignTextToFramePadding();
						TextUnformatted(_theme->windowPreferences_Emulator_ApplicationIcon());

						SameLine();

						{
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

							PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
							const float size = 48.0f;
							BeginChild("@Col", ImVec2(size + 4.0f, size + 4.0f), true, ImGuiWindowFlags_NoScrollbar);
							{
								void* iconTex = nullptr;
								if (_iconTexture) {
									iconTex = _iconTexture->pointer(_renderer);
								} else {
									if (_invalidIconTexture == nullptr) {
										_invalidIconTexture = Texture::Ptr(Texture::create());
										_invalidIconTexture->fromImage(_renderer, Texture::STATIC, _theme->imageApp(), Texture::NEAREST);
									}
									iconTex = _invalidIconTexture->pointer(_renderer);
								}
								SetCursorPos(GetCursorPos() + ImVec2(2, 2));
								Image(iconTex, ImVec2(size, size));
							}
							EndChild();
							PopStyleVar();
						}

						SameLine();
						const float btnW = 64.0f;
						const float posX = width - btnW;
						const float posY = GetCursorPosY();
						SetCursorPosX(posX);
						if (Button(_theme->generic_Replace(), ImVec2(btnW, 0))) {
							pfd::open_file open(
								_theme->generic_Open(),
								"",
								GBBASIC_IMAGE_FILE_FILTER,
								pfd::opt::none
							);
							do {
								if (open.result().empty() || open.result().front().empty())
									break;

								std::string path = open.result().front();
								Path::uniform(path);
								if (!Path::fileExists(path.c_str()))
									break;

								File::Ptr file(File::create());
								if (!file->open(path.c_str(), Stream::READ))
									break;

								Bytes::Ptr bytes(Bytes::create());
								if (!file->readBytes(bytes.get())) {
									file->close(); FileMonitor::unuse(path);

									break;
								}

								file->close(); FileMonitor::unuse(path);

								Image::Ptr img(Image::create());
								if (!img->fromBytes(bytes.get()))
									break;
								if (
									img->width() < EXPORTER_ICON_MIN_WIDTH || img->height() < EXPORTER_ICON_MIN_HEIGHT ||
									img->width() > EXPORTER_ICON_MAX_WIDTH || img->height() > EXPORTER_ICON_MAX_HEIGHT
								) {
									_invalidIconSize = true;
								} else {
									img->toBytes(bytes.get(), "png");
									_icon = bytes;
									_iconTexture = Texture::Ptr(Texture::create());
									_iconTexture->fromImage(_renderer, Texture::Usages::STATIC, img.get(), Texture::ScaleModes::NEAREST);
								}
							} while (false);
						}
						if (IsItemHovered()) {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							SetTooltip(_theme->tooltipProperty_ReplaceIcon());
						}
						const float posY_ = GetCursorPosY();
						SetCursorPosX(posX);
						SetCursorPosY(posY + 22.0f);
						if (Button(_theme->dialogPrompt_Rst(), ImVec2(btnW, 0))) {
							_icon = nullptr;
							_iconTexture = nullptr;
							_invalidIconSize = false;
						}
						if (IsItemHovered()) {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							SetTooltip(_theme->tooltipProperty_ResetIcon());
						}
						SetCursorPosY(posY_);

						if (_invalidIconSize) {
							AlignTextToFramePadding();
							TextUnformatted(_theme->windowPreferences_Emulator_InvalidIconSize_512x512IsRecommended());
						}
					}
					PopID();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Device(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Device_Emulator());

				PushID("#Dvc");
				{
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowPreferences_Device_Type());

					SameLine();

					const char* items[] = {
						_theme->windowPreferences_Device_Classic().c_str(),
						_theme->windowPreferences_Device_Colored().c_str(),
						_theme->windowPreferences_Device_ClassicWithExtension().c_str(),
						_theme->windowPreferences_Device_ColoredWithExtension().c_str()
					};
					int pref = (int)_settings.deviceType;
					SetNextItemWidth(GetContentRegionAvail().x);
					if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
						_settings.deviceType = pref;
				}
				PopID();

				// Checkbox(_theme->windowPreferences_Device_SaveSramOnStop(), &_settings.deviceSaveSramOnStop);

				if (_settings.deviceType == (unsigned)Device::DeviceTypes::CLASSIC || _settings.deviceType == (unsigned)Device::DeviceTypes::CLASSIC_EXTENDED) {
					PushID("#ClsPlt");
					{
						AlignTextToFramePadding();
						TextUnformatted(_theme->windowPreferences_Device_EmulationPalette());

						SameLine();

						VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 4));

						const float size = 19;
						for (int i = 0; i < GBBASIC_COUNTOF(_settings.deviceClassicPalette); ++i) {
							PushID(i);

							const Colour &color = _settings.deviceClassicPalette[i];
							const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
							ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size));
							SameLine();

							if (IsItemHovered() && (IsMouseClicked(ImGuiMouseButton_Left) || IsMouseClicked(ImGuiMouseButton_Right))) {
								_activeClassicPaletteShowColorPicker = true;
								_activeClassicPaletteIndex = i;
							}

							PopID();
						}
						SetCursorPosX(GetContentRegionMax().x - (WIDGETS_BUTTON_WIDTH - 18));
						if (Button(_theme->generic_Reset(), ImVec2(WIDGETS_BUTTON_WIDTH - 18, 0))) {
							_settings.deviceClassicPalette[0] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_0);
							_settings.deviceClassicPalette[1] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_1);
							_settings.deviceClassicPalette[2] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_2);
							_settings.deviceClassicPalette[3] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_3);
						}

						if (_activeClassicPaletteShowColorPicker) {
							_activeClassicPaletteShowColorPicker = false;
							OpenPopup("@Pkr");
						}
						do {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

							if (BeginPopup("@Pkr")) {
								VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

								const Colour col = _settings.deviceClassicPalette[_activeClassicPaletteIndex];
								float col4f[] = {
									Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
									Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
									Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
									Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
								};

								if (ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
									const Colour value(
										(Byte)(col4f[0] * 255),
										(Byte)(col4f[1] * 255),
										(Byte)(col4f[2] * 255),
										(Byte)(col4f[3] * 255)
									);
									_settings.deviceClassicPalette[_activeClassicPaletteIndex] = value;
								}

								EndPopup();
							}
						} while (false);
					}
					PopID();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Graphics(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Graphics_Canvas());

				Checkbox(_theme->windowPreferences_Graphics_FixCanvasRatio(), &_settings.canvasFixRatio);

				Checkbox(_theme->windowPreferences_Graphics_IntegerScale(), &_settings.canvasIntegerScale);

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Input(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Input_Gamepads());

				if (_activeGamepadIndex == -1)
					TextUnformatted(_theme->windowPreferences_Input_ClickToChange());
				else
					TextUnformatted(_theme->windowPreferences_Input_ClickAgainToCancelBackspaceToClear());
				ConfigGamepads(
					_input,
					_settings.inputGamepads, INPUT_GAMEPAD_COUNT,
					&_activeGamepadIndex, &_activeButtonIndex,
					_theme->windowPreferences_Input_WaitingForInput().c_str()
				);
				if (CollapsingHeader(_theme->windowPreferences_Input_Onscreen().c_str(), ImGuiTreeNodeFlags_None)) {
					ConfigOnscreenGamepad(
						&_settings.inputOnscreenGamepadEnabled,
						&_settings.inputOnscreenGamepadSwapAB,
						&_settings.inputOnscreenGamepadScale,
						&_settings.inputOnscreenGamepadPadding.x, &_settings.inputOnscreenGamepadPadding.y,
						_theme->windowPreferences_Input_Onscreen_Enabled().c_str(),
						_theme->windowPreferences_Input_Onscreen_SwapAB().c_str(),
						_theme->windowPreferences_Input_Onscreen_Scale().c_str(), _theme->windowPreferences_Input_Onscreen_PaddingX().c_str(), _theme->windowPreferences_Input_Onscreen_PaddingY().c_str()
					);
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Debug(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Debug_Emulation());

				Checkbox(_theme->windowPreferences_Debug_OnscreenDebugEnabled(), &_settings.debugOnscreenDebugEnabled);

				EndTabItem();
			}

			EndTabBar();
		}


		const char* confirm = _confirmText.empty() ? "Build" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			std::string str;
			_settings.toString(str);
			_confirmedHandler(str.c_str(), _argsBuffer, _icon);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

SearchPopupBox::SearchPopupBox(
	Theme* theme,
	const std::string &title,
	Settings &settings,
	const std::string &content, const std::string &default_, unsigned flags,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _theme(theme),
	_title(title),
	_settings(settings),
	_content(content), _default(default_), _flags(flags),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, _default.c_str(), Math::min(sizeof(_buffer), _default.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

SearchPopupBox::~SearchPopupBox() {
}

void SearchPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		TextUnformatted(_content);

		if (!_init.end())
			SetKeyboardFocusHere();
		InputText("", _buffer, sizeof(_buffer), _flags | ImGuiInputTextFlags_AutoSelectAll);

		Checkbox(_theme->windowSearchResult_CaseSensitive(), &_settings.mainCaseSensitive);

		Checkbox(_theme->windowSearchResult_MatchWholeWords(), &_settings.mainMatchWholeWords);

		Checkbox(_theme->windowSearchResult_GlobalSearch(), &_settings.mainGlobalSearch);

		const char* confirm = _confirmText.empty() ? "Search" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_buffer);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

ActorSelectorPopupBox::ActorSelectorPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	Project* project,
	int index,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_project(project),
	_selection(index),
	_confirmedHandler(confirm), _canceledHandler(cancel)
{
}

ActorSelectorPopupBox::~ActorSelectorPopupBox() {
}

void ActorSelectorPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(2, 2));
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));

	const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
	SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		const float width = Math::min(
			Math::clamp(_renderer->width() * 0.85f, 100.0f, 256.0f),
			Math::clamp(_renderer->height() * 0.85f, 100.0f, 256.0f)
		);
		const float height = width / 2.0f;

		const int n = _project->actorPageCount();
		int hovering = std::numeric_limits<int>::min();
		{
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

			BeginChild("@Sel", ImVec2(width * 0.5f, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
			{
				const bool none = _selection == -1;
				if (!none) {
					PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_SliderGrab]);
					PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_SliderGrab]);
					PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_SliderGrab]);
				}
				if (Button(_theme->generic_None().c_str(), ImVec2(GetContentRegionAvail().x, 0))) {
					toConfirm = true;

					_selection = -1;

					CloseCurrentPopup();
				}
				if (IsItemHovered()) {
					hovering = -1;
				}
				if (!none) {
					PopStyleColor(3);
				}

				for (int i = 0; i < n; ++i) {
					const ActorAssets::Entry* actorEntry = _project->getActor(i);
					const bool active = _selection == i;
					if (!active) {
						PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_SliderGrab]);
						PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_SliderGrab]);
						PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_SliderGrab]);
					}
					if (Button(actorEntry->name.c_str(), ImVec2(GetContentRegionAvail().x, 0))) {
						toConfirm = true;

						_selection = i;

						CloseCurrentPopup();
					}
					if (IsItemHovered()) {
						hovering = i;
					}
					if (!active) {
						PopStyleColor(3);
					}
				}

				if (hovering == std::numeric_limits<int>::min())
					hovering = _selection;
			}
			EndChild();
		}

		SameLine();
		{
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 1));

			BeginChild("@Prv", ImVec2(width * 0.5f, height), true, ImGuiWindowFlags_NoScrollbar);
			{
				if (hovering == -1) {
					const ImVec2 txtSize = CalcTextSize(_theme->dialogPrompt_NoPreview().c_str(), _theme->dialogPrompt_NoPreview().c_str() + _theme->dialogPrompt_NoPreview().length());
					const float offsetX = (width * 0.5f - txtSize.x) * 0.5f;
					const float offsetY = (height - txtSize.y) * 0.5f;
					const ImVec2 pos = GetCursorPos();
					SetCursorPos(pos + ImVec2(offsetX, offsetY));
					TextUnformatted(_theme->dialogPrompt_NoPreview());
				} else {
					const ActorAssets::Entry* actorEntry = _project->getActor(hovering);
					ActorAssets::Entry::PaletteResolver resolver = std::bind(&ActorAssets::Entry::colorAt, actorEntry, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
					const Texture::Ptr tex = actorEntry->figureTexture(nullptr, resolver);
					void* texPtr = nullptr;
					if (tex) {
						int texW = tex->width();
						int texH = tex->height();
						const int scale = Math::max((int)Math::min(width / texW, height / texH), 1);
						texW *= scale;
						texH *= scale;
						if (texW > width * 0.5f) {
							texW = (int)(width * 0.5f);
							texH = (int)(texW * ((float)tex->height() / tex->width()));
						}
						if (texH > height * 0.5f) {
							texH = (int)(height * 0.5f);
							texW = (int)(texH * ((float)tex->height() / tex->height()));
						}
						const float offsetX = (width * 0.5f - texW) * 0.5f;
						const float offsetY = (height - texH) * 0.5f;
						const ImVec2 pos = GetCursorPos();
						SetCursorPos(pos + ImVec2(offsetX, offsetY));
						texPtr = tex->pointer(_renderer);
						Image(texPtr, ImVec2((float)texW, (float)texH));
					}
				}
			}
			EndChild();
		}

		const bool tab = IsKeyPressed(SDL_SCANCODE_TAB);
		if (tab) {
			int next = _selection;
			if (io.KeyShift) {
				--next;
				if (next < -1)
					next = (int)n - 1;
			} else {
				++next;
				if (next >= (int)n)
					next = -1;
			}
			_selection = next;
		}

		if (IsKeyReleased(SDL_SCANCODE_SPACE)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		if (IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty())
			_confirmedHandler(_selection);
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty())
			_canceledHandler();
	}
}

FontResolverPopupBox::FontResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	const Math::Vec2i &vec2i, const Math::Vec2i &vec2iMin, const Math::Vec2i &vec2iMax, const char* vec2iTxt, const char* vec2iXTxt,
	const std::string &content, const std::string &default_,
	const Text::Array &filter,
	bool requireExisting,
	const char* browseTxt,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_vec2i(vec2i), _vec2iMin(vec2iMin), _vec2iMax(vec2iMax),
	_content(content), _path(default_),
	_filter(filter),
	_requireExisting(requireExisting),
	_confirmedHandler(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom)
{
	_exists = Path::fileExists(_path.c_str());

	if (browseTxt)
		_browseText = browseTxt;
	if (vec2iTxt)
		_vec2iText = vec2iTxt;
	if (vec2iXTxt)
		_vec2iXText = vec2iXTxt;
	else
		_vec2iXText = "x";

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

FontResolverPopupBox::~FontResolverPopupBox() {
}

const std::string &FontResolverPopupBox::path(void) const {
	return _path;
}

bool FontResolverPopupBox::exists(void) const {
	return _exists;
}

const Math::Vec2i &FontResolverPopupBox::vec2iValue(void) const {
	return _vec2i;
}

void FontResolverPopupBox::vec2iValue(const Math::Vec2i &val) {
	_vec2i = val;
}

const Math::Vec2i &FontResolverPopupBox::vec2iMinValue(void) const {
	return _vec2iMin;
}

void FontResolverPopupBox::vec2iMinValue(const Math::Vec2i &val) {
	_vec2iMin = val;
}

const Math::Vec2i &FontResolverPopupBox::vec2iMaxValue(void) const {
	return _vec2iMax;
}

void FontResolverPopupBox::vec2iMaxValue(const Math::Vec2i &val) {
	_vec2iMax = val;
}

void FontResolverPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		const float oldX = GetCursorPosX();
		AlignTextToFramePadding();
		TextUnformatted(_content);

		SameLine();
		if (!_init.end())
			SetKeyboardFocusHere();
		SetNextItemWidth(128);
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		InputText("", (char*)_path.c_str(), _path.length() + 1, ImGuiInputTextFlags_ReadOnly);
		PopStyleColor();

		SameLine();
		if (Button(_browseText.empty() ? "Browse" : _browseText.c_str())) {
			FileMonitor::unuse(_path);

			std::string path_ = _path;
#if defined GBBASIC_OS_WIN
			path_ = Text::replace(path_, "/", "\\");
#endif /* GBBASIC_OS_WIN */
			pfd::open_file open(
				_theme->generic_Open(),
				path_,
				_filter,
				pfd::opt::none
			);
			if (!open.result().empty() && !open.result().front().empty()) {
				std::string path = open.result().front();
				Path::uniform(path);
				_path = path;
				_exists = Path::fileExists(_path.c_str());
				if (_exists) {
					std::string ext;
					Path::split(_path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					_withSize = ext == "png" || ext == "jpg" || ext == "bmp" || ext == "tga";
				} else {
					_withSize = false;
				}

				if (!_selectedHandler.empty()) {
					if (!_selectedHandler(this)) {
						EndPopup();

						return;
					}
				}
			}
		}
		const float sx = style.ItemSpacing.x;
		style.ItemSpacing.x = 0;
		SameLine();
		style.ItemSpacing.x = sx;
		const float width = GetCursorPosX() - oldX + sx;
		NewLine();

		if (_withSize && !_confirmedHandler.empty()) {
			const float oldX_ = GetCursorPosX();
			AlignTextToFramePadding();
			TextUnformatted(_vec2iText);
			SameLine();

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			const float xWidth = 20;
			const float width_ = (width - (GetCursorPosX() - oldX_) - xWidth - guardItemSpacing.previous().x) * 0.5f;
			const float xPos = GetCursorPosX();
			int val = (int)_vec2i.x;
			PushID("x");
			if (IntegerModifier(_renderer, _theme, &val, _vec2iMin.x, _vec2iMax.x, width_))
				_vec2i.x = val;
			PopID();
			SameLine();
			TextUnformatted(_vec2iXText);
			SameLine();
			SetCursorPosX(xPos + width_ + xWidth + 1);
			val = (int)_vec2i.y;
			PushID("y");
			if (IntegerModifier(_renderer, _theme, &val, _vec2iMin.y, _vec2iMax.y, width_))
				_vec2i.y = val;
			PopID();

			NewLine(guardItemSpacing.previous().y);
		}

		if (!_customHandler.empty()) {
			_customHandler(this, width);
		}

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if ((_requireExisting && _exists) || (!_requireExisting && !_path.empty())) {
			if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
				toConfirm = true;

				CloseCurrentPopup();
			}
		} else {
			BeginDisabled();
			{
				Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			}
			EndDisabled();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			const std::string path = _path;
			ws->delay([path] (void) -> void { FileMonitor::unuse(path); }, path);

			_confirmedHandler(_path.c_str(), _withSize ? &_vec2i : nullptr);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			const std::string path = _path;
			ws->delay([path] (void) -> void { FileMonitor::unuse(path); }, path);

			_canceledHandler();

			return;
		}
	}
}

MapResolverPopupBox::MapResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	const char* withIndexTxt, const char* withImageTxt,
	int index, int indexMin, int indexMax, const char* indexTxt,
	const std::string &content, const std::string &default_,
	const Text::Array &filter,
	bool requireExisting,
	const char* browseTxt,
	bool allowFlip,
	const char* allowFlipTxt, const char* allowFlipTooltipTxt,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_index(index), _indexMin(indexMin), _indexMax(indexMax),
	_content(content), _path(default_),
	_filter(filter),
	_requireExisting(requireExisting),
	_allowFlip(allowFlip),
	_confirmedHandler(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom)
{
	_exists = Path::fileExists(_path.c_str());

	if (withIndexTxt)
		_withIndexText = withIndexTxt;
	else
		_withTilesIndex = false;
	if (withImageTxt)
		_withImageText = withImageTxt;

	if (indexTxt)
		_indexText = indexTxt;

	if (browseTxt)
		_browseText = browseTxt;

	if (allowFlipTxt)
		_allowFlipText = allowFlipTxt;
	if (allowFlipTooltipTxt)
		_allowFlipTooltipText = allowFlipTooltipTxt;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

MapResolverPopupBox::~MapResolverPopupBox() {
}

const std::string &MapResolverPopupBox::path(void) const {
	return _path;
}

bool MapResolverPopupBox::exists(void) const {
	return _exists;
}

int MapResolverPopupBox::indexValue(void) const {
	return _index;
}

void MapResolverPopupBox::indexValue(int val) {
	_index = val;
}

int MapResolverPopupBox::indexMinValue(void) const {
	return _indexMin;
}

void MapResolverPopupBox::indexMinValue(int val) {
	_indexMin = val;
}

int MapResolverPopupBox::indexMaxValue(void) const {
	return _indexMax;
}

void MapResolverPopupBox::indexMaxValue(int val) {
	_indexMax = val;
}

void MapResolverPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		if (!_withIndexText.empty()) {
			if (Checkbox(_withIndexText.empty() ? "From tiles" : _withIndexText.c_str(), &_withTilesIndex)) {
				// Do nothing.
			}
			SameLine();
			bool val = !_withTilesIndex;
			if (Checkbox(_withImageText.empty() ? "From image" : _withImageText.c_str(), &val)) {
				_withTilesIndex = false;
			}
		}

		float width = 0.0f;
		if (_withTilesIndex) {
			const float oldX = GetCursorPosX();
			AlignTextToFramePadding();
			TextUnformatted(_indexText);
			SameLine();

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			width = 251.0f;
			const bool editable = _indexMin != _indexMax;
			const float width_ = width - (GetCursorPosX() - oldX) - guardItemSpacing.previous().x;
			int val = _index;
			PushID("i");
			if (editable) {
				const Text::Array &assetNames = ws->getTilesPageNames();
				const std::string &pg = assetNames[_index];
				if (IntegerModifier(_renderer, _theme, &val, _indexMin, _indexMax, width_, nullptr, pg.c_str())) {
					_index = Math::clamp(val, _indexMin, _indexMax);
				}
			} else {
				BeginDisabled();
				{
					if (IntegerModifier(_renderer, _theme, &val, _indexMin, _indexMax, width_)) {
						// Do nothing.
					}
				}
				EndDisabled();
			}
			PopID();
			SameLine();

			NewLine(guardItemSpacing.previous().y);
			NewLine(4.0f);
		} else {
			const float oldX = GetCursorPosX();
			if (!_withImageText.empty()) {
				AlignTextToFramePadding();
				TextUnformatted(_content);

				SameLine();
				if (!_init.end())
					SetKeyboardFocusHere();
				SetNextItemWidth(128);
				PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
				InputText("", (char*)_path.c_str(), _path.length() + 1, ImGuiInputTextFlags_ReadOnly);
				PopStyleColor();

				SameLine();
				if (Button(_browseText.empty() ? "Browse" : _browseText.c_str())) {
					FileMonitor::unuse(_path);

					std::string path_ = _path;
#if defined GBBASIC_OS_WIN
					path_ = Text::replace(path_, "/", "\\");
#endif /* GBBASIC_OS_WIN */
					pfd::open_file open(
						_theme->generic_Open(),
						path_,
						_filter,
						pfd::opt::none
					);
					if (!open.result().empty() && !open.result().front().empty()) {
						std::string path = open.result().front();
						Path::uniform(path);
						_path = path;
						_exists = Path::fileExists(_path.c_str());

						if (!_selectedHandler.empty()) {
							if (!_selectedHandler(this)) {
								EndPopup();

								return;
							}
						}
					}
				}
			}

			Checkbox(_allowFlipText, &_allowFlip);
			if (!_allowFlipTooltipText.empty() && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				SetTooltip(_allowFlipTooltipText);
			}

			const float sx = style.ItemSpacing.x;
			style.ItemSpacing.x = 0;
			SameLine();
			style.ItemSpacing.x = sx;
			width = GetCursorPosX() - oldX + sx;
			NewLine();
		}

		if (!_customHandler.empty()) {
			_customHandler(this, width);
		}

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (_withTilesIndex || (_requireExisting && _exists) || (!_requireExisting && !_path.empty()) || (!_requireExisting && _withImageText.empty())) {
			if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
				toConfirm = true;

				CloseCurrentPopup();
			}
		} else {
			BeginDisabled();
			{
				Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			}
			EndDisabled();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			const std::string path = _path;
			ws->delay([path] (void) -> void { FileMonitor::unuse(path); }, path);

			if (_withTilesIndex)
				_confirmedHandler(&_index, nullptr, false);
			else
				_confirmedHandler(nullptr, _path.c_str(), _allowFlip);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			const std::string path = _path;
			ws->delay([path] (void) -> void { FileMonitor::unuse(path); }, path);

			_canceledHandler();

			return;
		}
	}
}

SceneResolverPopupBox::SceneResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	Project* project,
	int index, int indexMin, int indexMax, const char* indexTxt,
	bool useGravity, const char* useGravityText,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_project(project),
	_index(index), _indexMin(indexMin), _indexMax(indexMax),
	_useGravity(useGravity),
	_confirmedHandler(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom)
{
	if (indexTxt)
		_indexText = indexTxt;

	if (useGravityText)
		_useGravityText = useGravityText;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

SceneResolverPopupBox::~SceneResolverPopupBox() {
}

int SceneResolverPopupBox::indexValue(void) const {
	return _index;
}

void SceneResolverPopupBox::indexValue(int val) {
	_index = val;
}

int SceneResolverPopupBox::indexMinValue(void) const {
	return _indexMin;
}

void SceneResolverPopupBox::indexMinValue(int val) {
	_indexMin = val;
}

int SceneResolverPopupBox::indexMaxValue(void) const {
	return _indexMax;
}

void SceneResolverPopupBox::indexMaxValue(int val) {
	_indexMax = val;
}

void SceneResolverPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		float width = 0.0f;
		do {
			const float oldX = GetCursorPosX();
			AlignTextToFramePadding();
			TextUnformatted(_indexText);
			SameLine();

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			width = 251.0f;
			const bool editable = _indexMin != _indexMax;
			const float width_ = width - (GetCursorPosX() - oldX) - guardItemSpacing.previous().x;
			int val = _index;
			PushID("i");
			if (editable) {
				const Text::Array &assetNames = ws->getMapPageNames();
				const std::string &pg = assetNames[_index];
				if (IntegerModifier(_renderer, _theme, &val, _indexMin, _indexMax, width_, nullptr, pg.c_str())) {
					_index = Math::clamp(val, _indexMin, _indexMax);
				}
			} else {
				BeginDisabled();
				{
					if (IntegerModifier(_renderer, _theme, &val, _indexMin, _indexMax, width_)) {
						// Do nothing.
					}
				}
				EndDisabled();
			}
			PopID();
			SameLine();

			NewLine(guardItemSpacing.previous().y);
			NewLine(4.0f);

			Checkbox(_useGravityText, &_useGravity);
		} while (false);

		if (!_customHandler.empty()) {
			_customHandler(this, width);
		}

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_index, _useGravity);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

FileResolverPopupBox::FileResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	const std::string &content, const std::string &default_,
	const Text::Array &filter,
	bool requireExisting,
	const char* browseTxt,
	const ConfirmedHandler_Path &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_content(content), _path(default_),
	_filter(filter),
	_requireExisting(requireExisting),
	_confirmedHandler_Path(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom)
{
	_exists = Path::fileExists(_path.c_str());

	if (browseTxt)
		_browseText = browseTxt;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

FileResolverPopupBox::FileResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	const std::string &content, const std::string &default_,
	const Text::Array &filter,
	bool requireExisting,
	const char* browseTxt,
	bool bool_, const char* boolTxt,
	const ConfirmedHandler_PathBoolean &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_content(content), _path(default_),
	_filter(filter),
	_requireExisting(requireExisting),
	_boolean(bool_),
	_confirmedHandler_PathBool(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom)
{
	_exists = Path::fileExists(_path.c_str());

	if (browseTxt)
		_browseText = browseTxt;
	if (boolTxt)
		_booleanText = boolTxt;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

FileResolverPopupBox::FileResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	const std::string &content, const std::string &default_,
	const Text::Array &filter,
	bool requireExisting,
	const char* browseTxt,
	int integer, int min, int max, const char* intTxt,
	const ConfirmedHandler_PathInteger &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_content(content), _path(default_),
	_filter(filter),
	_requireExisting(requireExisting),
	_integer(integer), _integerMin(min), _integerMax(max),
	_confirmedHandler_PathInteger(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom)
{
	_exists = Path::fileExists(_path.c_str());

	if (browseTxt)
		_browseText = browseTxt;
	if (intTxt)
		_inteterText = intTxt;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

FileResolverPopupBox::FileResolverPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	const std::string &content, const std::string &default_,
	const Text::Array &filter,
	bool requireExisting,
	const char* browseTxt,
	const Math::Vec2i &vec2i, const Math::Vec2i &vec2iMin, const Math::Vec2i &vec2iMax, const char* vec2iTxt, const char* vec2iXTxt,
	const ConfirmedHandler_PathVec2i &confirm, const CanceledHandler &cancel,
	const char* confirmTxt, const char* cancelTxt,
	const SelectedHandler &selected,
	const CustomHandler &custom,
	const Vec2iResolver &resolveVec2i, const char* resolveVec2iTxt
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_content(content), _path(default_),
	_filter(filter),
	_requireExisting(requireExisting),
	_vec2i(vec2i), _vec2iMin(vec2iMin), _vec2iMax(vec2iMax),
	_confirmedHandler_PathVec2i(confirm), _canceledHandler(cancel),
	_selectedHandler(selected),
	_customHandler(custom),
	_vec2iResolver(resolveVec2i)
{
	_exists = Path::fileExists(_path.c_str());

	if (browseTxt)
		_browseText = browseTxt;
	if (vec2iTxt)
		_vec2iText = vec2iTxt;
	if (vec2iXTxt)
		_vec2iXText = vec2iXTxt;
	else
		_vec2iXText = "x";

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;

	if (resolveVec2iTxt)
		_vec2iResolvingText = resolveVec2iTxt;
}

FileResolverPopupBox::~FileResolverPopupBox() {
}

bool FileResolverPopupBox::toSave(void) const {
	return _toSave;
}

FileResolverPopupBox* FileResolverPopupBox::toSave(bool val) {
	_toSave = val;

	return this;
}

const std::string &FileResolverPopupBox::path(void) const {
	return _path;
}

bool FileResolverPopupBox::exists(void) const {
	return _exists;
}

bool FileResolverPopupBox::booleanValue(void) const {
	return _boolean;
}

void FileResolverPopupBox::booleanValue(bool val) {
	_boolean = val;
}

int FileResolverPopupBox::integerValue(void) const {
	return _integer;
}

void FileResolverPopupBox::integerValue(int val) {
	_integer = val;
}

int FileResolverPopupBox::integerMinValue(void) const {
	return _integerMin;
}

void FileResolverPopupBox::integerMinValue(int val) {
	_integerMin = val;
}

int FileResolverPopupBox::integerMaxValue(void) const {
	return _integerMax;
}

void FileResolverPopupBox::integerMaxValue(int val) {
	_integerMax = val;
}

const Math::Vec2i &FileResolverPopupBox::vec2iValue(void) const {
	return _vec2i;
}

void FileResolverPopupBox::vec2iValue(const Math::Vec2i &val) {
	_vec2i = val;
}

const Math::Vec2i &FileResolverPopupBox::vec2iMinValue(void) const {
	return _vec2iMin;
}

void FileResolverPopupBox::vec2iMinValue(const Math::Vec2i &val) {
	_vec2iMin = val;
}

const Math::Vec2i &FileResolverPopupBox::vec2iMaxValue(void) const {
	return _vec2iMax;
}

void FileResolverPopupBox::vec2iMaxValue(const Math::Vec2i &val) {
	_vec2iMax = val;
}

void FileResolverPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	if (_toResolveVec2i) {
		Math::Vec2i vec2i;
		if (_vec2iResolver(_path.c_str(), &vec2i))
			_vec2i = vec2i;
		_toResolveVec2i = false;
	}

	if (BeginPopupModal(_title, _canceledHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		const float oldX = GetCursorPosX();
		AlignTextToFramePadding();
		TextUnformatted(_content);

		SameLine();
		if (!_init.end())
			SetKeyboardFocusHere();
		SetNextItemWidth(128);
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		InputText("", (char*)_path.c_str(), _path.length() + 1, ImGuiInputTextFlags_ReadOnly);
		PopStyleColor();

		SameLine();
		if (Button(_browseText.empty() ? "Browse" : _browseText.c_str())) {
			FileMonitor::unuse(_path);

			std::string path_ = _path;
#if defined GBBASIC_OS_WIN
			path_ = Text::replace(path_, "/", "\\");
#endif /* GBBASIC_OS_WIN */
			std::string path;
			if (_toSave) {
				pfd::save_file save(
					ws->theme()->generic_SaveTo(),
					"",
					_filter
				);
				path = save.result();
			} else {
				pfd::open_file open(
					_theme->generic_Open(),
					path_,
					_filter,
					pfd::opt::none
				);
				if (!open.result().empty() && !open.result().front().empty()) {
					path = open.result().front();
				}
			}
			if (!path.empty()) {
				Path::uniform(path);
				_path = path;
				_exists = Path::fileExists(_path.c_str());

				if (!_selectedHandler.empty()) {
					if (!_selectedHandler(this)) {
						EndPopup();

						return;
					}
				}

				if (!_vec2iResolver.empty()) {
					_toResolveVec2i = true;
				}
			}
		}
		const float sx = style.ItemSpacing.x;
		style.ItemSpacing.x = 0;
		SameLine();
		style.ItemSpacing.x = sx;
		const float width = GetCursorPosX() - oldX + sx;
		NewLine();

		if (!_confirmedHandler_PathBool.empty()) {
			Checkbox(_booleanText.empty() ? "Enable" : _booleanText.c_str(), &_boolean);
		}

		if (!_confirmedHandler_PathInteger.empty()) {
			const float oldX_ = GetCursorPosX();
			AlignTextToFramePadding();
			TextUnformatted(_inteterText);
			SameLine();

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			const float width_ = width - (GetCursorPosX() - oldX_) - guardItemSpacing.previous().x;
			PushID("i");
			IntegerModifier(_renderer, _theme, &_integer, _integerMin, _integerMax, width_);
			PopID();

			NewLine(guardItemSpacing.previous().y);
		}

		if (!_confirmedHandler_PathVec2i.empty()) {
			const float oldX_ = GetCursorPosX();
			AlignTextToFramePadding();
			TextUnformatted(_vec2iText);
			SameLine();

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			const float xWidth = 20;
			const float width_ = (width - (GetCursorPosX() - oldX_) - xWidth - guardItemSpacing.previous().x) * 0.5f;
			const float xPos = GetCursorPosX();
			int val = (int)_vec2i.x;
			PushID("x");
			if (IntegerModifier(_renderer, _theme, &val, _vec2iMin.x, _vec2iMax.x, width_))
				_vec2i.x = val;
			PopID();
			SameLine();
			TextUnformatted(_vec2iXText);
			SameLine();
			SetCursorPosX(xPos + width_ + xWidth + 1);
			val = (int)_vec2i.y;
			PushID("y");
			if (IntegerModifier(_renderer, _theme, &val, _vec2iMin.y, _vec2iMax.y, width_))
				_vec2i.y = val;
			PopID();

			if (_toResolveVec2i && !_vec2iResolvingText.empty()) {
				AlignTextToFramePadding();
				TextUnformatted(_vec2iResolvingText);
			}

			NewLine(guardItemSpacing.previous().y);
		}

		if (!_customHandler.empty()) {
			_customHandler(this, width);
		}

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if ((_requireExisting && _exists) || (!_requireExisting && !_path.empty())) {
			if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
				toConfirm = true;

				CloseCurrentPopup();
			}
		} else {
			BeginDisabled();
			{
				Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			}
			EndDisabled();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		const std::string path = _path;
		ws->delay([path] (void) -> void { FileMonitor::unuse(path); }, path);

		if (!_confirmedHandler_Path.empty()) {
			_confirmedHandler_Path(_path.c_str());

			return;
		} else if (!_confirmedHandler_PathBool.empty()) {
			_confirmedHandler_PathBool(_path.c_str(), _boolean);

			return;
		} else if (!_confirmedHandler_PathInteger.empty()) {
			_confirmedHandler_PathInteger(_path.c_str(), _integer);

			return;
		} else if (!_confirmedHandler_PathVec2i.empty()) {
			_confirmedHandler_PathVec2i(_path.c_str(), _vec2i);

			return;
		} else {
			GBBASIC_ASSERT(false && "Impossible.");
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			const std::string path = _path;
			ws->delay([path] (void) -> void { FileMonitor::unuse(path); }, path);

			_canceledHandler();

			return;
		}
	}
}

ProjectPropertyPopupBox::ProjectPropertyPopupBox(
	Renderer* rnd,
	Theme* theme,
	const std::string &title,
	Project* project,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
	const char* confirmTxt, const char* cancelTxt, const char* applyTxt
) : _renderer(rnd),
	_theme(theme),
	_title(title),
	_project(project),
	_confirmedHandler(confirm), _canceledHandler(cancel), _appliedHandler(apply)
{
	_projectShadow = new Project(_project->window(), rnd, _project->workspace());
	*_projectShadow = *_project;

	const std::string &prjTitle = _project->title();
	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, prjTitle.c_str(), Math::min(sizeof(_buffer), prjTitle.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
	if (applyTxt)
		_applyText = applyTxt;
}

ProjectPropertyPopupBox::~ProjectPropertyPopupBox() {
	delete _projectShadow;
	_projectShadow = nullptr;
}

void ProjectPropertyPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toApply = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 480.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		Project* prj = _projectShadow;

		if (BeginTabBar("@Prj")) {
			if (BeginTabItem(_theme->tabProjectProperty_Project(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				PushID("#Title");
				{
					const float x = GetCursorPosX();
					const float w = GetWindowContentRegionMax().x - GetWindowContentRegionMin().x - x;

					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Title());

					SameLine();

					const float u = GetCursorPosX() - x;
					SetNextItemWidth(w * 0.5f - u);
					if (InputText("", _buffer, sizeof(_buffer), ImGuiInputTextFlags_AutoSelectAll))
						prj->title(_buffer);
				}
				PopID();

				PushID("#Path");
				{
					SameLine();
					TextUnformatted(_theme->windowProjectProperty_Path());

					SameLine();

					SetNextItemWidth(GetContentRegionAvail().x);
					const std::string &txt = prj->path();
					PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
					InputText("", (char*)txt.c_str(), txt.length(), ImGuiInputTextFlags_ReadOnly);
					PopStyleColor();
				}
				PopID();

				PushID("#CrtTyp");
				{
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Cart());

					SameLine();

					const char* items[] = {
						PROJECT_CARTRIDGE_TYPE_CLASSIC,
						PROJECT_CARTRIDGE_TYPE_COLORED,
						PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED,
						PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION,
						PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION,
						PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION
					};
					int pref = 0;
					for (int i = 0; i < GBBASIC_COUNTOF(items); ++i) {
						if (prj->cartridgeType() == items[i]) {
							pref = i;

							break;
						}
					}
					SetNextItemWidth(GetContentRegionAvail().x);
					if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
						prj->cartridgeType(items[pref]);
				}
				PopID();

				PushID("#SrmTyp");
				{
					const float x = GetCursorPosX();
					const float w = GetWindowContentRegionMax().x - GetWindowContentRegionMin().x - x;

					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Sram());

					SameLine();

					const char* items[] = {
						_theme->windowProjectProperty_Sram_None().c_str(),
						"2KB",
						"8KB",
						"32KB",
						"128KB"
					};
					int pref = 0;
					if (!Text::fromString(prj->sramType(), pref))
						pref = 3;
					const float u = GetCursorPosX() - x;
					SetNextItemWidth(w * 0.5f - u);
					if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
						prj->sramType(Text::toString(pref));
				}
				PopID();

				PushID("#HasRtc");
				{
					SameLine();
					TextUnformatted(_theme->windowProjectProperty_Rtc());

					SameLine();

					bool pref = prj->hasRtc();
					if (Checkbox(_theme->generic_Enabled(), &pref))
						prj->hasRtc(pref);
				}
				PopID();

				PushID("#Desc");
				{
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Desc());

					SameLine();

					SetNextItemWidth(GetContentRegionAvail().x);
					std::string &txt = prj->description();
					char buf[WIDGETS_TEXT_BUFFER_SIZE];
					const size_t n = Math::min(sizeof(buf) - 1, txt.length());
					memcpy(buf, txt.c_str(), n);
					buf[n] = '\0';
					if (InputText("", (char*)buf, sizeof(buf), ImGuiInputTextFlags_None)) {
						txt = buf;
					}
				}
				PopID();

				PushID("#Athr");
				{
					const float x = GetCursorPosX();
					const float w = GetWindowContentRegionMax().x - GetWindowContentRegionMin().x - x;

					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Author());

					SameLine();

					const float u = GetCursorPosX() - x;
					SetNextItemWidth(w * 0.5f - u);
					std::string &txt = prj->author();
					char buf[WIDGETS_TEXT_BUFFER_SIZE];
					const size_t n = Math::min(sizeof(buf) - 1, txt.length());
					memcpy(buf, txt.c_str(), n);
					buf[n] = '\0';
					if (InputText("", (char*)buf, sizeof(buf), ImGuiInputTextFlags_None)) {
						txt = buf;
					}
				}
				PopID();

				PushID("#Gnr");
				{
					SameLine();
					TextUnformatted(_theme->windowProjectProperty_Genre());

					SameLine();

					SetNextItemWidth(GetContentRegionAvail().x);
					std::string &txt = prj->genre();
					char buf[WIDGETS_TEXT_BUFFER_SIZE];
					const size_t n = Math::min(sizeof(buf) - 1, txt.length());
					memcpy(buf, txt.c_str(), n);
					buf[n] = '\0';
					if (InputText("", (char*)buf, sizeof(buf), ImGuiInputTextFlags_None)) {
						txt = buf;
					}
				}
				PopID();

				PushID("#Ver");
				{
					const float x = GetCursorPosX();
					const float w = GetWindowContentRegionMax().x - GetWindowContentRegionMin().x - x;

					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Version());

					SameLine();

					const float u = GetCursorPosX() - x;
					SetNextItemWidth(w * 0.5f - u);
					std::string &txt = prj->version();
					char buf[WIDGETS_TEXT_BUFFER_SIZE];
					const size_t n = Math::min(sizeof(buf) - 1, txt.length());
					memcpy(buf, txt.c_str(), n);
					buf[n] = '\0';
					if (InputText("", (char*)buf, sizeof(buf), ImGuiInputTextFlags_None)) {
						txt = buf;
					}
				}
				PopID();

				float firstColumnEndX = 0.0f;
				PushID("#Url");
				{
					SameLine();
					firstColumnEndX = GetCursorPosX() - style.ItemSpacing.x;
					TextUnformatted(_theme->windowProjectProperty_Url());

					SameLine();

					SetNextItemWidth(GetContentRegionAvail().x);
					std::string &txt = prj->url();
					char buf[WIDGETS_TEXT_BUFFER_SIZE];
					const size_t n = Math::min(sizeof(buf) - 1, txt.length());
					memcpy(buf, txt.c_str(), n);
					buf[n] = '\0';
					if (InputText("", (char*)buf, sizeof(buf), ImGuiInputTextFlags_None)) {
						txt = buf;
					}
				}
				PopID();

				float secondColumnEndX = 0.0f;
				SameLine();
				secondColumnEndX = GetCursorPosX() - style.ItemSpacing.x;
				NewLine();

				PushID("#Icon");
				{
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Icon());

					SameLine();

					{
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

						PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
						BeginChild("@Col", ImVec2(36.0f, 36.0f), true, ImGuiWindowFlags_NoScrollbar);
						{
							void* iconTex = nullptr;
							if (prj->contentType() == Project::ContentTypes::BASIC) {
								if (prj->touchIconTexture())
									iconTex = prj->touchIconTexture()->pointer(_renderer);
								else if (prj->isExample())
									iconTex = _theme->iconExample()->pointer(_renderer);
								else if (prj->isPlain())
									iconTex = _theme->iconPlainCode()->pointer(_renderer);
								else
									iconTex = _theme->iconProjectOmitted()->pointer(_renderer);
							} else {
								iconTex = _theme->iconRom()->pointer(_renderer);
							}
							SetCursorPos(GetCursorPos() + ImVec2(2, 2));
							Image(iconTex, ImVec2(32.0f, 32.0f));
						}
						EndChild();
						PopStyleVar();
					}
					if (IsItemHovered() && !ws->bubble()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						SetTooltip(_theme->tooltipProperty_ProjectIconDetails());
					}

					SameLine();
					{
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

						PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
						BeginChild("@2Bpp", ImVec2(36.0f, 36.0f), true, ImGuiWindowFlags_NoScrollbar);
						{
							void* iconTex = nullptr;
							if (prj->touchIconTexture2Bpp()) {
								iconTex = prj->touchIconTexture2Bpp()->pointer(_renderer);
								SetCursorPos(GetCursorPos() + ImVec2(2, 2));
								Image(iconTex, ImVec2(32.0f, 32.0f));
								if (IsItemHovered() && IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
									do {
										Bytes::Ptr tiles(Bytes::create());
										if (!prj->touchIconImage2Bpp(tiles))
											break;

										constexpr const size_t SIZE = (GBBASIC_ICON_WIDTH / GBBASIC_TILE_SIZE) * GBBASIC_ICON_HEIGHT * 2;
										if (tiles->count() != SIZE) {
											GBBASIC_ASSERT(false && "Impossible.");

											break;
										}

										std::string txt;
										txt += "const unsigned char Icon[256] = { // 32x32 pixels.\n";
										txt += "  ";
										for (int i = 0; i < (int)tiles->count(); ++i) {
											const Byte byte = tiles->get(i);
											txt += "0x" + Text::toHex(byte, 2, '0', false);
											if (i < (int)tiles->count() - 1) {
												txt += ",";
											}
											if ((i + 1) % 16 == 0) {
												txt += "\n";
												if (i < (int)tiles->count() - 1) {
													txt += "  ";
												}
											}
										}
										txt += "};\n";

										txt += "const unsigned char IconIndices[16] = { // 4x4 tiles.\n";
										txt += "  0,  2,  4,  6,\n";
										txt += "  1,  3,  5,  7,\n";
										txt += "  8, 10, 12, 14,\n";
										txt += "  9, 11, 13, 15\n";
										txt += "};\n";

										Platform::setClipboardText(txt.c_str());

										ws->bubble(_theme->dialogPrompt_CopiedToClipboard(), nullptr);
									} while (false);
								}
							}
						}
						EndChild();
						PopStyleVar();
					}
					if (IsItemHovered() && !ws->bubble()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						SetTooltip(_theme->tooltipProperty_ProjectIcon2BppDetails());
					}

					SameLine();
					const float btnW = 64.0f;
					const float posX = GetContentRegionAvail().x < 330 ? secondColumnEndX - btnW : firstColumnEndX - btnW;
					const float posY = GetCursorPosY();
					SetCursorPosX(posX);
					if (Button(_theme->generic_Replace(), ImVec2(btnW, 0))) {
						pfd::open_file open(
							_theme->generic_Open(),
							"",
							GBBASIC_IMAGE_FILE_FILTER,
							pfd::opt::none
						);
						do {
							if (open.result().empty() || open.result().front().empty())
								break;

							std::string path = open.result().front();
							Path::uniform(path);
							if (!Path::fileExists(path.c_str()))
								break;

							File::Ptr file(File::create());
							if (!file->open(path.c_str(), Stream::READ))
								break;

							Bytes::Ptr bytes(Bytes::create());
							if (!file->readBytes(bytes.get())) {
								file->close(); FileMonitor::unuse(path);

								break;
							}

							file->close(); FileMonitor::unuse(path);

							Image::Ptr img(Image::create());
							if (!img->fromBytes(bytes.get()))
								break;
							if (img->width() != GBBASIC_ICON_WIDTH || img->height() != GBBASIC_ICON_HEIGHT) {
								int w = 0;
								int h = 0;
								if ((float)img->width() / img->height() < (float)GBBASIC_ICON_WIDTH / GBBASIC_ICON_HEIGHT) {
									w = GBBASIC_ICON_WIDTH;
									h = (int)(GBBASIC_ICON_WIDTH * ((float)img->height() / img->width()));
								} else {
									w = (int)(GBBASIC_ICON_HEIGHT * ((float)img->width() / img->height()));
									h = GBBASIC_ICON_HEIGHT;
								}
								if (!img->resize(w, h, true)) {
									break;
								}

								Image::Ptr tmp(Image::create());
								if (!tmp->fromBlank(GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, 0)) {
									break;
								}
								const int x = (w - GBBASIC_ICON_WIDTH) / 2;
								const int y = (h - GBBASIC_ICON_HEIGHT) / 2;
								if (!img->blit(tmp.get(), 0, 0, GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, x, y)) {
									break;
								}
								std::swap(img, tmp);
								img->toBytes(bytes.get(), "png");
							}

							std::string txt;
							if (!Base64::fromBytes(txt, bytes.get()))
								break;

							prj->iconCode(txt);
						} while (false);
					}
					if (IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						SetTooltip(_theme->tooltipProperty_ReplaceIcon());
					}
					const float posY_ = GetCursorPosY();
					SetCursorPosX(posX - 32.0f - 5);
					SetCursorPosY(posY);
					if (Button(_theme->dialogPrompt_Rst(), ImVec2(0, 0))) {
						prj->iconCode("");
					}
					if (IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						SetTooltip(_theme->tooltipProperty_ResetIcon());
					}
					SetCursorPosY(posY_);
				}
				PopID();

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabProjectProperty_Compiling(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowProjectProperty_Compiling_Parser());

				PushID("#CsIns");
				{
					bool pref = prj->caseInsensitive();
					if (Checkbox(_theme->windowProjectProperty_Compiling_Parser_CaseInsensitive(), &pref))
						prj->caseInsensitive(pref);
				}
				PopID();

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabProjectProperty_Advanced(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowProjectProperty_Advanced_SuperFeatures());

				PushID("#SgbBdr");
				{
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowProjectProperty_Advanced_BorderFrame());

					SameLine();

					const char* items[] = {
						_theme->windowProjectProperty_Advanced_BorderFrame_None().c_str(),
						"Builtin",
						_theme->windowProjectProperty_Advanced_BorderFrame_Custom().c_str()
					};
					int type = 0;
					if (prj->borderFrameType() == Project::BorderFrameTypes::NONE) {
						type = 0;
					} else if (prj->borderFrameType() == Project::BorderFrameTypes::CUSTOM) {
						type = GBBASIC_COUNTOF(items) - 1;
					} else {
						type = 1;
						// TODO
					}
					SetNextItemWidth(GetContentRegionAvail().x);
					if (Combo("", &type, items, GBBASIC_COUNTOF(items))) {
						if (type == 0) {
							prj->borderFrameType(Project::BorderFrameTypes::NONE);
						} else if (GBBASIC_COUNTOF(items) - 1) {
							prj->borderFrameType(Project::BorderFrameTypes::CUSTOM);
						} else {
							prj->borderFrameType(Project::BorderFrameTypes::BUILTIN);
							// TODO
						}
					}

					// TODO
					// _theme->dialogPrompt_Rst()
					// _theme->generic_Replace()
				}
				PopID();

				EndTabItem();
			}

			EndTabBar();
		}

		const char* confirm = _confirmText.c_str();
		const char* apply = _applyText.empty() ? "Apply" : _applyText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		const bool appliable =
			prj->title()           != _project->title()           ||
			prj->cartridgeType()   != _project->cartridgeType()   ||
			prj->sramType()        != _project->sramType()        ||
			prj->hasRtc()          != _project->hasRtc()          ||
			prj->caseInsensitive() != _project->caseInsensitive() ||
			prj->description()     != _project->description()     ||
			prj->author()          != _project->author()          ||
			prj->genre()           != _project->genre()           ||
			prj->version()         != _project->version()         ||
			prj->url()             != _project->url()             ||
			prj->iconCode()        != _project->iconCode();

		if (_confirmText.empty()) {
			confirm = "Ok";
		}

		CentralizeButton(3);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (appliable) {
			if (Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
				toApply = true;
			}
		} else {
			BeginDisabled();
			Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			EndDisabled();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_projectShadow);

			return;
		}
	}
	if (toApply) {
		if (!_appliedHandler.empty()) {
			_appliedHandler(_projectShadow);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

PreferencesPopupBox::PreferencesPopupBox(
	Renderer* rnd,
	Input* input, Theme* theme,
	const std::string &title,
	Settings &settings,
	BoolGetter getBorderlessWritable, BoolGetter getBorderless,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
	const char* confirmTxt, const char* cancelTxt, const char* applyTxt
) : _renderer(rnd),
	_input(input), _theme(theme),
	_title(title),
	_settings(settings),
	_getBorderlessWritable(getBorderlessWritable), _getBorderless(getBorderless),
	_confirmedHandler(confirm), _canceledHandler(cancel), _appliedHandler(apply)
{
	_settingsShadow = _settings;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
	if (applyTxt)
		_applyText = applyTxt;
}

PreferencesPopupBox::~PreferencesPopupBox() {
}

void PreferencesPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toApply = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 480.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		const bool borderlessWritable = _getBorderlessWritable();
		const bool borderless = _getBorderless();
		if (_settings.applicationWindowBorderless == _settingsShadow.applicationWindowBorderless) {
			_settings.applicationWindowBorderless = _settingsShadow.applicationWindowBorderless = borderless;
		} else {
			_settings.applicationWindowBorderless = borderless;
		}

		if (BeginTabBar("@Pref")) {
			if (BeginTabItem(_theme->tabPreferences_Main(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				PushID("#App");
				{
					TextUnformatted(_theme->windowPreferences_Main_Application());

					Checkbox(_theme->windowPreferences_Main_ShowProjectPathAtTitleBar(), &_settingsShadow.mainShowProjectPathAtTitleBar);

					Separator();
				}
				PopID();

				PushID("#CdEd");
				{
					TextUnformatted(_theme->windowPreferences_Main_CodeEditor());

					PushID("#Col");
					{
						AlignTextToFramePadding();
						TextUnformatted(_theme->windowPreferences_Main_ColumnIndicator());

						SameLine();

						const char* items[] = {
							_theme->windowPreferences_Main_None().c_str(),
							_theme->windowPreferences_Main_40().c_str(),
							_theme->windowPreferences_Main_60().c_str(),
							_theme->windowPreferences_Main_80().c_str(),
							_theme->windowPreferences_Main_100().c_str(),
							_theme->windowPreferences_Main_120().c_str()
						};
						int pref = (int)_settingsShadow.mainColumnIndicator;
						SetNextItemWidth(GetContentRegionAvail().x);
						if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
							_settingsShadow.mainColumnIndicator = (Settings::ColumnIndicator)pref;
					}
					PopID();

					PushID("#Ind");
					{
						AlignTextToFramePadding();
						TextUnformatted(_theme->windowPreferences_Main_IndentWith());

						SameLine();

						const char* items[] = {
							_theme->windowPreferences_Main_2Spaces().c_str(),
							_theme->windowPreferences_Main_4Spaces().c_str(),
							_theme->windowPreferences_Main_8Spaces().c_str(),
							_theme->windowPreferences_Main_Tab2SpacesWide().c_str(),
							_theme->windowPreferences_Main_Tab4SpacesWide().c_str(),
							_theme->windowPreferences_Main_Tab8SpacesWide().c_str()
						};
						int pref = (int)_settingsShadow.mainIndentRule;
						SetNextItemWidth(GetContentRegionAvail().x);
						if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
							_settingsShadow.mainIndentRule = (Settings::IndentRules)pref;
					}
					PopID();

					Checkbox(_theme->windowPreferences_Main_ShowWhiteSpaces(), &_settingsShadow.mainShowWhiteSpaces);

					Separator();
				}
				PopID();

				PushID("#Cnsl");
				{
					TextUnformatted(_theme->windowPreferences_Main_Console());

					Checkbox(_theme->windowPreferences_Main_ClearOnStart(), &_settingsShadow.consoleClearOnStart);
				}
				PopID();

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Debug(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Debug_Compiler());

				Checkbox(_theme->windowPreferences_Debug_ShowAstEnabled(), &_settingsShadow.debugShowAstEnabled);

				Separator();

				TextUnformatted(_theme->windowPreferences_Debug_Emulation());

				Checkbox(_theme->windowPreferences_Debug_OnscreenDebugEnabled(), &_settingsShadow.debugOnscreenDebugEnabled);

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Device(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Device_Emulator());

				PushID("#Dvc");
				{
					AlignTextToFramePadding();
					TextUnformatted(_theme->windowPreferences_Device_Type());

					SameLine();

					const char* items[] = {
						_theme->windowPreferences_Device_Classic().c_str(),
						_theme->windowPreferences_Device_Colored().c_str(),
						_theme->windowPreferences_Device_ClassicWithExtension().c_str(),
						_theme->windowPreferences_Device_ColoredWithExtension().c_str()
					};
					int pref = (int)_settingsShadow.deviceType;
					SetNextItemWidth(GetContentRegionAvail().x);
					if (Combo("", &pref, items, GBBASIC_COUNTOF(items)))
						_settingsShadow.deviceType = pref;
				}
				PopID();

				Checkbox(_theme->windowPreferences_Device_SaveSramOnStop(), &_settingsShadow.deviceSaveSramOnStop);

				if (_settingsShadow.deviceType == (unsigned)Device::DeviceTypes::CLASSIC || _settingsShadow.deviceType == (unsigned)Device::DeviceTypes::CLASSIC_EXTENDED) {
					PushID("#ClsPlt");
					{
						AlignTextToFramePadding();
						TextUnformatted(_theme->windowPreferences_Device_EmulationPalette());

						SameLine();

						VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(1, 4));

						const float size = 19;
						for (int i = 0; i < GBBASIC_COUNTOF(_settingsShadow.deviceClassicPalette); ++i) {
							PushID(i);

							const Colour &color = _settingsShadow.deviceClassicPalette[i];
							const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
							ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size));
							SameLine();

							if (IsItemHovered() && (IsMouseClicked(ImGuiMouseButton_Left) || IsMouseClicked(ImGuiMouseButton_Right))) {
								_activeClassicPaletteShowColorPicker = true;
								_activeClassicPaletteIndex = i;
							}

							PopID();
						}
						SetCursorPosX(GetContentRegionMax().x - (WIDGETS_BUTTON_WIDTH - 18));
						if (Button(_theme->generic_Reset(), ImVec2(WIDGETS_BUTTON_WIDTH - 18, 0))) {
							_settingsShadow.deviceClassicPalette[0] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_0);
							_settingsShadow.deviceClassicPalette[1] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_1);
							_settingsShadow.deviceClassicPalette[2] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_2);
							_settingsShadow.deviceClassicPalette[3] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_3);
						}

						if (_activeClassicPaletteShowColorPicker) {
							_activeClassicPaletteShowColorPicker = false;
							OpenPopup("@Pkr");
						}
						do {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

							if (BeginPopup("@Pkr")) {
								VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

								const Colour col = _settingsShadow.deviceClassicPalette[_activeClassicPaletteIndex];
								float col4f[] = {
									Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
									Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
									Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
									Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
								};

								if (ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
									const Colour value(
										(Byte)(col4f[0] * 255),
										(Byte)(col4f[1] * 255),
										(Byte)(col4f[2] * 255),
										(Byte)(col4f[3] * 255)
									);
									_settingsShadow.deviceClassicPalette[_activeClassicPaletteIndex] = value;
								}

								EndPopup();
							}
						} while (false);
					}
					PopID();
				}

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Graphics(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
				TextUnformatted(_theme->windowPreferences_Graphics_Application());

				if (borderlessWritable) {
					Checkbox(_theme->windowPreferences_Graphics_BorderlessWindow(), &_settingsShadow.applicationWindowBorderless);
				} else {
					BeginDisabled();
					Checkbox(_theme->windowPreferences_Graphics_BorderlessWindow(), &_settingsShadow.applicationWindowBorderless);
					EndDisabled();
				}

				Checkbox(_theme->windowPreferences_Graphics_Fullscreen(), &_settingsShadow.applicationWindowFullscreen);

				Separator();
#endif /* Platform macro. */

				TextUnformatted(_theme->windowPreferences_Graphics_Canvas());

				Checkbox(_theme->windowPreferences_Graphics_FixCanvasRatio(), &_settingsShadow.canvasFixRatio);

				Checkbox(_theme->windowPreferences_Graphics_IntegerScale(), &_settingsShadow.canvasIntegerScale);

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Input(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Input_Gamepads());

				if (_activeGamepadIndex == -1)
					TextUnformatted(_theme->windowPreferences_Input_ClickToChange());
				else
					TextUnformatted(_theme->windowPreferences_Input_ClickAgainToCancelBackspaceToClear());
				ConfigGamepads(
					_input,
					_settingsShadow.inputGamepads, INPUT_GAMEPAD_COUNT,
					&_activeGamepadIndex, &_activeButtonIndex,
					_theme->windowPreferences_Input_WaitingForInput().c_str()
				);
				if (CollapsingHeader(_theme->windowPreferences_Input_Onscreen().c_str(), ImGuiTreeNodeFlags_None)) {
					ConfigOnscreenGamepad(
						&_settingsShadow.inputOnscreenGamepadEnabled,
						&_settingsShadow.inputOnscreenGamepadSwapAB,
						&_settingsShadow.inputOnscreenGamepadScale,
						&_settingsShadow.inputOnscreenGamepadPadding.x, &_settingsShadow.inputOnscreenGamepadPadding.y,
						_theme->windowPreferences_Input_Onscreen_Enabled().c_str(),
						_theme->windowPreferences_Input_Onscreen_SwapAB().c_str(),
						_theme->windowPreferences_Input_Onscreen_Scale().c_str(), _theme->windowPreferences_Input_Onscreen_PaddingX().c_str(), _theme->windowPreferences_Input_Onscreen_PaddingY().c_str()
					);
				}

				EndTabItem();
			}

			EndTabBar();
		}

		const char* confirm = _confirmText.c_str();
		const char* apply = _applyText.empty() ? "Apply" : _applyText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		const bool appliable = _settings != _settingsShadow;

		if (_confirmText.empty()) {
			confirm = "Ok";
		}

		CentralizeButton(3);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) && _init.end()) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if ((Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) && _init.end()) {
			toCancel = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (appliable) {
			if (Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
				toApply = true;
			}
		} else {
			BeginDisabled();
			Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			EndDisabled();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler(_settingsShadow);

			return;
		}
	}
	if (toApply) {
		if (!_appliedHandler.empty()) {
			_appliedHandler(_settingsShadow);

			return;
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_canceledHandler.empty()) {
			_canceledHandler();

			return;
		}
	}
}

ActivitiesPopupBox::ActivitiesPopupBox(
	Renderer* rnd,
	const std::string &title,
	Workspace* ws,
	const ConfirmedHandler &confirm,
	const char* confirmTxt
) : _renderer(rnd),
	_title(title),
	_confirmedHandler(confirm)
{
	const Activities &activities = ws->activities();
	 _activities = Text::format(
		ws->theme()->windowActivities_ContentInfo(),
		{
			Text::toNumberWithCommas    (activities.opened.to_string()),
				Text::toNumberWithCommas(activities.openedBasic.to_string()),
				Text::toNumberWithCommas(activities.openedRom.to_string()),
			Text::toNumberWithCommas    (activities.created.to_string()),
				Text::toNumberWithCommas(activities.createdBasic.to_string()),
			Text::toNumberWithCommas    (activities.played.to_string()),
				Text::toNumberWithCommas(activities.playedBasic.to_string()),
				Text::toNumberWithCommas(activities.playedRom.to_string()),
			Activities::formatTime      (activities.playedTime),
				Activities::formatTime  (activities.playedTimeBasic),
				Activities::formatTime  (activities.playedTimeRom),
			Text::toNumberWithCommas    (activities.wroteCodeLines.to_string())
		}
	);

	if (confirmTxt)
		_confirmText = confirmTxt;
}

ActivitiesPopupBox::~ActivitiesPopupBox() {
}

void ActivitiesPopupBox::update(Workspace* ws) {
	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 480.0f);
	const float height = Math::clamp(_renderer->height() * 0.3f, 64.0f, 256.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		TextUnformatted(ws->theme()->windowActivities_Activities());

		const float border = GetCursorPosX();
		InputTextMultiline(
			"##Ipt",
			(char*)_activities.c_str(), _activities.length(),
			ImVec2(width - border * 2, height),
			ImGuiInputTextFlags_ReadOnly
		);

		const char* confirm = _confirmText.c_str();

		if (_confirmText.empty()) {
			confirm = "Ok";
		}

		CentralizeButton();

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN) || IsKeyReleased(SDL_SCANCODE_Y)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		if (IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm || toCancel) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler();

			return;
		}
	}
}

AboutPopupBox::AboutPopupBox(
	Renderer* rnd,
	const std::string &title,
	const ConfirmedHandler &confirm,
	const char* confirmTxt,
	const char* licenses
) : _renderer(rnd),
	_title(title),
	_confirmedHandler(confirm)
{
	_desc = "v" GBBASIC_VERSION_STRING_WITH_SUFFIX;
	_desc += " " WIDGETS_ABOUT_REVISION;

	_specs += "Built for " GBBASIC_OS ", ";
	_specs += Platform::isLittleEndian() ? "little-endian" : "big-endian";
	_specs += ", with " GBBASIC_CP;
	_specs += "\n";

	_specs += "Driver:\n";
	_specs += "  ";
	_specs += rnd->driver();
	_specs += "\n";

	_specs += "Licenses:\n";
	_specs += "  https://paladin-t.github.io/kits/gbb/about.html\n";

	_specs += "Libraries:\n";
	_specs += "        SDL v" + Text::toString(SDL_MAJOR_VERSION) + "." + Text::toString(SDL_MINOR_VERSION) + "." + Text::toString(SDL_PATCHLEVEL) + "\n";
	_specs += "      ImGui v" IMGUI_VERSION "\n";
	_specs += "     binjgb v" "0.1.11" "\n";
	_specs += "  RapidJSON v" RAPIDJSON_VERSION_STRING "\n";
	_specs += "        LZ4 v" LZ4_VERSION_STRING "\n";
	_specs += "       zlib v" ZLIB_VERSION "\n";
#if !defined GBBASIC_OS_HTML
	_specs += "   CivetWeb v" CIVETWEB_VERSION "\n";
#endif /* GBBASIC_OS_HTML */

	_specs += "Kernel and Toolchain:\n";
	_specs += "      GBBVM v1.3\n";
	_specs += "  GBDK-2020 v4.4\n";

	if (licenses)
		_specs += licenses;

	if (confirmTxt)
		_confirmText = confirmTxt;
}

AboutPopupBox::~AboutPopupBox() {
}

void AboutPopupBox::update(Workspace*) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin()) {
		OpenPopup(_title);

		const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
		SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const float width = Math::clamp(_renderer->width() * 0.8f, 280.0f, 480.0f);
	const float height = Math::clamp(_renderer->height() * 0.3f, 64.0f, 256.0f);
	SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		Url(GBBASIC_TITLE, "https://paladin-t.github.io/kits/gbb/");
		SameLine();
		TextUnformatted(_desc);

		{
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			TextUnformatted("  by ");
			SameLine();
			Url("Tony Wang", "https://paladin-t.github.io/");
			SameLine();
			TextUnformatted(", 2023-2025");
			NewLine();
		}
		Separator();

		const float border = GetCursorPosX();
		InputTextMultiline(
			"##Ipt",
			(char*)_specs.c_str(), _specs.length(),
			ImVec2(width - border * 2, height),
			ImGuiInputTextFlags_ReadOnly
		);

		const char* confirm = _confirmText.c_str();

		if (_confirmText.empty()) {
			confirm = "Ok";
		}

		CentralizeButton();

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN) || IsKeyReleased(SDL_SCANCODE_Y)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		if (IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm || toCancel) {
		_init.reset();

		if (!_confirmedHandler.empty()) {
			_confirmedHandler();

			return;
		}
	}
}

ImVec2 GetMousePosOnCurrentItem(const ImVec2* ref_pos) {
	const ImVec2 ref = ref_pos ? *ref_pos : GetCursorScreenPos();

	ImVec2 pos = GetMousePos();
	pos -= ref;

	return pos;
}

void PushID(const std::string &str_id) {
	PushID(str_id.c_str(), str_id.c_str() + str_id.length());
}

Rect LastItemRect(void) {
	ImGuiContext &g = *GetCurrentContext();

	return std::make_pair(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max);
}

void Dummy(const ImVec2 &size, ImU32 col) {
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + size);
	ItemSize(size);
	ItemAdd(bb, 0);

	drawList->AddRectFilled(pos, pos + size, col);
}

void Dummy(const ImVec2 &size, const ImVec4 &col) {
	Dummy(size, ColorConvertFloat4ToU32(col));
}

void NewLine(float height) {
	Dummy(ImVec2(0, height));
}

void ScrollToItem(int flags, float offset_y) {
	ImGuiContext &g = *GetCurrentContext();
	ImGuiWindow* window = GetCurrentWindow();
	ImRect rect = g.LastItemData.NavRect;
	rect.Min.y += offset_y;
	rect.Max.y += offset_y;
	ScrollToRectEx(window, rect, flags);
}

ImVec2 WindowResizingPadding(void) {
	constexpr const float WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS = 4.0f;

	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	ImVec2 paddingRegular = style.TouchExtraPadding;
	ImVec2 paddingForResizeFromEdges = io.ConfigWindowsResizeFromEdges ? ImMax(style.TouchExtraPadding, ImVec2(WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS)) : paddingRegular;

	return paddingForResizeFromEdges;
}

bool Begin(const std::string &name, bool* p_open, ImGuiWindowFlags flags) {
	return Begin(name.c_str(), p_open, flags);
}

void CentralizeWindow(void) {
	ImGuiIO &io = GetIO();

	const float maxWidth = io.DisplaySize.x/* * io.DisplayFramebufferScale.x*/;
	const float maxHeight = io.DisplaySize.y/* * io.DisplayFramebufferScale.y*/;

	const float width = GetWindowWidth();
	const float height = GetWindowHeight();

	const ImVec2 pos((maxWidth - width) * 0.5f, (maxHeight - height) * 0.5f);
	SetWindowPos(pos);
}

void EnsureWindowVisible(void) {
	ImGuiIO &io = GetIO();

	const float maxWidth = io.DisplaySize.x/* * io.DisplayFramebufferScale.x*/;
	const float maxHeight = io.DisplaySize.y/* * io.DisplayFramebufferScale.y*/;

	if (maxWidth == 0 || maxHeight == 0)
		return;

	float currWidth = GetWindowWidth();
	float currHeight = GetWindowHeight();
	if (currWidth > maxWidth)
		currWidth = maxWidth;
	if (currHeight > maxHeight)
		currHeight = maxHeight;
	if (currWidth != maxWidth || currHeight != maxHeight)
		SetWindowSize(ImVec2(currWidth, currHeight));
	const float width = currWidth;
	const float height = currHeight;

	ImVec2 pos = GetWindowPos();
	if (pos.x < 0)
		pos.x = 0;
	if (pos.y < 0)
		pos.y = 0;
	if (pos.x + width > maxWidth)
		pos.x = maxWidth - width;
	if (pos.y + height > maxHeight)
		pos.y = maxHeight - height;
	if (pos.x < 0)
		pos.x = (maxWidth - width) * 0.5f;
	if (pos.y < 0)
		pos.y = (maxHeight - height) * 0.5f;
	if (pos != GetWindowPos())
		SetWindowPos(pos);
}

void OpenPopup(const std::string &str_id, ImGuiPopupFlags popup_flags) {
	OpenPopup(str_id.c_str(), popup_flags);
}

bool BeginPopupModal(const std::string &name, bool* p_open, ImGuiWindowFlags flags) {
	return BeginPopupModal(name.c_str(), p_open, flags);
}

float TitleBarHeight(void) {
	ImGuiStyle &style = GetStyle();

	return GetFontSize() + style.FramePadding.y * 2.0f;
}

bool TitleBarCustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImGuiWindow* window = GetCurrentWindow();

	const ImGuiID id = GetID(label);

	const ImRect titleBarRect = window->TitleBarRect();
	const float padR = style.FramePadding.x;
	const float buttonSz = GetFontSize();

	ImVec2 position;
	if (pos && pos->x > 0 && pos->y > 0) {
		position = *pos;
	} else {
		position = ImVec2(
			titleBarRect.Max.x - (padR + buttonSz) * 2 - style.FramePadding.x/* - padR*/,
			titleBarRect.Min.y
		);
		if (pos)
			*pos = position;
	}
	if (pos)
		pos->x -= padR + buttonSz;

	PushClipRect(titleBarRect.Min, titleBarRect.Max, false);

	const ImRect bb(position, position + ImVec2(GetFontSize(), GetFontSize()) + style.FramePadding * 2.0f);
	const bool isClipped = !ItemAdd(bb, id);

	bool hovered = false, held = false;
	const bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (!isClipped) {
		const ImVec2 center = bb.GetCenter();

		if (draw)
			draw(center, held, hovered, tooltip);
	}

	PopClipRect();

	return pressed;
}

ImVec2 ScrollbarSize(bool horizontal) {
	ImGuiWindow* window = GetCurrentWindow();
	const ImRect rect = GetWindowScrollbarRect(window, horizontal ? ImGuiAxis_X : ImGuiAxis_Y);

	return ImVec2(rect.GetWidth(), rect.GetHeight());
}

ImVec2 CustomButtonAutoPosition(void) {
	return ImVec2(-1, -1);
}

bool CustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	const ImGuiID id = GetID(label);

	const float padR = style.FramePadding.x;
	const float buttonSz = GetFontSize();

	SameLine();

	ImVec2 position = GetCursorScreenPos();
	position.x = GetWindowPos().x + GetWindowWidth() - (padR + buttonSz) - style.FramePadding.x;
	if (pos && pos->x > 0 && pos->y > 0) {
		position = *pos;
	} else {
		if (pos)
			*pos = position;
	}
	if (pos)
		pos->x -= padR + buttonSz;

	const ImRect bb(position, position + ImVec2(GetFontSize(), GetFontSize()) + style.FramePadding * 2.0f);
	const bool isClipped = !ItemAdd(bb, id);

	bool hovered = false, held = false;
	ButtonBehavior(bb, id, &hovered, &held);
	if (!hovered) {
		if (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
			hovered = true;
	}

	bool pressed = false;
	if (hovered && bb.Contains(GetMousePos())) {
		ClearActiveID();
		SetHoveredID(id);
		pressed = IsMouseClicked(ImGuiMouseButton_Left);
	}

	if (!isClipped) {
		const ImVec2 center = bb.GetCenter();

		if (draw)
			draw(center, held, hovered, tooltip);
	}

	NewLine();

	return pressed;
}

void CustomAddButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, 0), center + ImVec2(lnExtent, 0), lnCol, 1);
	drawList->AddLine(center + ImVec2(0, -lnExtent), center + ImVec2(0, lnExtent), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomRemoveButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, 0), center + ImVec2(lnExtent, 0), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomRenameButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddRect(center + ImVec2(-lnExtent, -lnExtent * 0.5f), center + ImVec2(lnExtent, lnExtent * 0.5f), lnCol);
	drawList->AddLine(center + ImVec2(lnExtent * 0.25f, -lnExtent), center + ImVec2(lnExtent * 0.25f, lnExtent), lnCol, 1);
	drawList->AddLine(center + ImVec2(lnExtent * -0.1f, -lnExtent), center + ImVec2(lnExtent * 0.6f, -lnExtent), lnCol, 1);
	drawList->AddLine(center + ImVec2(lnExtent * -0.1f, lnExtent), center + ImVec2(lnExtent * 0.6f, lnExtent), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomClearButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	const int lnSeg = 4;
	const int lnStep = (int)(lnExtent * 2 / (lnSeg - 1));
	const int lnHeight = lnStep * (lnSeg - 1);
	float yOff = -lnExtent + (lnExtent * 2 - lnHeight) / 2;
	for (int i = 0; i < lnSeg; ++i) {
		drawList->AddLine(center + ImVec2(-lnExtent, yOff), center + ImVec2(lnExtent, yOff), lnCol, 1);
		yOff += lnStep;
	}

	const ImU32 xCol = GetColorU32(ImVec4(0.93f, 0.27f, 0.27f, 1));
	const float xExtent = lnExtent;
	drawList->AddLine(
		center + ImVec2(-lnExtent, -lnExtent),
		center + ImVec2(-lnExtent, -lnExtent) + ImVec2(xExtent, xExtent),
		xCol, 1
	);
	drawList->AddLine(
		center + ImVec2(-lnExtent, -lnExtent) + ImVec2(0, xExtent),
		center + ImVec2(-lnExtent, -lnExtent) + ImVec2(xExtent, 0),
		xCol, 1
	);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomMinButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, lnExtent - 1), center + ImVec2(lnExtent, lnExtent - 1), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomMaxButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddRect(center + ImVec2(-lnExtent, -lnExtent), center + ImVec2(lnExtent, lnExtent), lnCol);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomCloseButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, -lnExtent), center + ImVec2(lnExtent, lnExtent), lnCol);
	drawList->AddLine(center + ImVec2(lnExtent, -lnExtent), center + ImVec2(-lnExtent, lnExtent), lnCol);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomPlayButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddTriangleFilled(
		center + ImVec2(-lnExtent, -lnExtent),
		center + ImVec2(lnExtent, 0),
		center + ImVec2(-lnExtent, lnExtent),
		lnCol
	);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomStopButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddRectFilled(center + ImVec2(-lnExtent, -lnExtent), center + ImVec2(lnExtent, lnExtent), lnCol);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

bool LineGraph(const ImVec2 &size, ImU32 border_col, ImU32 grid_col, ImU32 content_col, ImU32 drawing_col, const Math::Vec2i &grid_step, const int* old_values, int* new_values, int min, int max, int count, bool circular, ImVec2* prev_pos, bool* mouse_down, const char* tooltip_fmt) {
	GBBASIC_ASSERT(min <= max && "Wrong data.");

	ImGuiStyle &style = GetStyle();
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return false;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + size);
	ItemSize(size);
	ItemAdd(bb, 0);

	constexpr const int padding = 1;
	const int diff = max - min;
	const ImVec2 content_size = size - ImVec2(padding * 2, padding * 2);
	const float unit_x = circular ?
		content_size.x / count :
		content_size.x / (count - 1);
	const float unit_y = content_size.y / diff;
	const bool hovered = IsItemHovered();
	const bool down = IsMouseDown(ImGuiMouseButton_Left);
	const bool drawing = hovered && down;

	auto map = [unit_x, unit_y] (const ImVec2 &point_pos) -> Math::Vec2i {
		const int x_index = (int)std::floor(point_pos.x / unit_x);
		const int y_index = (int)std::round(point_pos.y / unit_y);

		return Math::Vec2i(x_index, y_index);
	};
	auto paint = [min, max, diff, map] (int* values, const ImVec2 &point_pos) -> void {
		const Math::Vec2i mapp = map(point_pos);
		const int x_index = mapp.x;
		const int y_index = mapp.y;
		values[x_index] = Math::clamp(min + diff - y_index, min, max);
	};
	auto render = [min, circular, drawList, pos, content_size, padding, diff, unit_x] (const int* values, int count, ImU32 col) -> void {
		float first_x = 0.0f, first_y = 0.0f;
		float prev_x = 0.0f, prev_y = 0.0f;
		for (int i = 0; i < count; ++i) {
			const int old_val = values[i];
			const float factor = Math::clamp((float)(old_val - min) / (float)diff, 0.0f, 1.0f);
			const float x = pos.x + unit_x * i;
			const float y = pos.y + (content_size.y - 1) * (1 - factor) + padding;
			if (i != 0) {
				drawList->AddLine(ImVec2(prev_x, prev_y), ImVec2(x, y), col);
			}
			if (i == 0) {
				first_x = x;
				first_y = y;
			}
			prev_x = x;
			prev_y = y;
		}
		if (circular) {
			drawList->AddLine(ImVec2(prev_x, prev_y), ImVec2(pos.x + unit_x * count, first_y), col);
		}
	};

	drawList->PushClipRect(pos, pos + size, true);
	for (int i = 1; i < count; i += grid_step.x) {
		const float x = pos.x + unit_x * i + padding;
		const float y0 = pos.y + padding;
		const float y1 = pos.y + content_size.y + padding;
		drawList->AddLine(ImVec2(x, y0), ImVec2(x, y1), grid_col);
	}
	for (int j = 1; j < diff; j += grid_step.y) {
		const float x0 = pos.x + padding;
		const float x1 = pos.x + content_size.x + padding;
		const float y = pos.y + unit_y * j + padding;
		drawList->AddLine(ImVec2(x0, y), ImVec2(x1, y), grid_col);
	}
	render(old_values, count, content_col);
	if (drawing && new_values)
		render(new_values, count, drawing_col);
	drawList->PopClipRect();
	drawList->AddRect(pos, pos + size, border_col);

	if (hovered && (!tooltip_fmt || *tooltip_fmt)) {
		const ImVec2 mouse_pos = GetMousePos();
		const ImVec2 point_pos = mouse_pos - (pos + ImVec2((float)padding, (float)padding)) + ImVec2(unit_x * 0.5f, 0.0f);
		const Math::Vec2i mapp = map(point_pos);
		const int x_index = mapp.x;
		const int y_index = mapp.y;
		char buf[16];
		snprintf(buf, GBBASIC_COUNTOF(buf), tooltip_fmt ? tooltip_fmt : "%d: %X", x_index, Math::clamp(min + diff - y_index, min, max));

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(buf);
	}

	bool clicked = false;
	if (new_values) {
		if (mouse_down && *mouse_down && !hovered && down) {
			*mouse_down = false;

			if (prev_pos)
				*prev_pos = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());

			clicked = true;
		} else if (drawing) {
			if (mouse_down)
				*mouse_down = true;

			const ImVec2 mouse_pos = GetMousePos();
			const ImVec2 point_pos = mouse_pos - (pos + ImVec2((float)padding, (float)padding)) + ImVec2(unit_x * 0.5f, 0.0f);

			if (prev_pos) {
				if (std::isnan(prev_pos->x) || std::isnan(prev_pos->y)) {
					*prev_pos = point_pos;
				} else {
					const Math::Vec2f p0(prev_pos->x, prev_pos->y);
					const Math::Vec2f p1(point_pos.x, point_pos.y);
					Math::Vec2f dir = p1 - p0;
					if (dir.x != 0 && dir != Math::Vec2f(0, 0)) {
						const Real len = dir.normalize();
						const Real step = std::abs(unit_x / dir.x) * 0.3f;
						dir *= step;
						const int n = (int)(len / step - 1);
						for (int i = 0; i < n; ++i) {
							const Math::Vec2f p = p0 + dir * i;
							paint(new_values, ImVec2((float)p.x, (float)p.y));
						}
					}

					*prev_pos = point_pos;
				}
			}
			paint(new_values, point_pos);
		} else if (hovered && IsMouseReleased(ImGuiMouseButton_Left)) {
			if (mouse_down)
				*mouse_down = false;

			if (prev_pos)
				*prev_pos = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());

			clicked = true;
		}
	}

	return clicked;
}

bool BarGraph(const ImVec2 &size, ImU32 border_col, ImU32 grid_col, ImU32 content_col, ImU32 drawing_col, const Math::Vec2i &grid_step, const int* old_values, int* new_values, int min, int max, int count, float base, ImVec2* prev_pos, bool* mouse_down, const char* tooltip_fmt) {
	GBBASIC_ASSERT(min <= max && "Wrong data.");

	ImGuiStyle &style = GetStyle();
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return false;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + size);
	ItemSize(size);
	ItemAdd(bb, 0);

	constexpr const int padding = 1;
	const int diff = max - min;
	const ImVec2 content_size = size - ImVec2(padding * 2, padding * 2);
	const float base_y = pos.y + content_size.y * base + padding;
	const float unit_x = content_size.x / count;
	const float unit_y = content_size.y / (diff + 1);
	const bool hovered = IsItemHovered();
	const bool down = IsMouseDown(ImGuiMouseButton_Left);
	const bool drawing = hovered && down;

	auto map = [unit_x, unit_y] (const ImVec2 &point_pos) -> Math::Vec2i {
		const int x_index = (int)std::floor(point_pos.x / unit_x);
		const int y_index = (int)std::floor(point_pos.y / unit_y);

		return Math::Vec2i(x_index, y_index);
	};
	auto paint = [min, max, diff, map] (int* values, const ImVec2 &point_pos) -> void {
		const Math::Vec2i mapp = map(point_pos);
		const int x_index = mapp.x;
		const int y_index = mapp.y;
		values[x_index] = Math::clamp(min + diff - y_index, min, max);
	};
	auto render = [min, drawList, pos, content_size, padding, diff, base_y, unit_x] (const int* values, int count, ImU32 col) -> void {
		for (int i = 0; i < count; ++i) {
			const int old_val = values[i];
			const float factor = Math::clamp((float)(old_val - min) / (float)diff, 0.0f, 1.0f);
			const float x = pos.x + unit_x * i + padding;
			const float y = pos.y + content_size.y * (1 - factor) + padding;
			const float min_y = Math::min(base_y, y);
			const float max_y = Math::max(base_y, y);
			drawList->AddRectFilled(ImVec2(std::floor(x), std::floor(min_y)), ImVec2(std::ceil(x + unit_x), std::ceil(max_y)), col);
		}
	};

	drawList->PushClipRect(pos, pos + size, true);
	render(old_values, count, content_col);
	if (drawing && new_values)
		render(new_values, count, drawing_col);
	for (int i = 1; i < count; i += grid_step.x) {
		const float x = pos.x + unit_x * i + padding;
		const float y0 = pos.y + padding;
		const float y1 = pos.y + content_size.y + padding;
		drawList->AddLine(ImVec2(x, y0), ImVec2(x, y1), grid_col);
	}
	for (int j = 1; j < diff; j += grid_step.y) {
		const float x0 = pos.x + padding;
		const float x1 = pos.x + content_size.x + padding;
		const float y = pos.y + unit_y * j + padding;
		drawList->AddLine(ImVec2(x0, y), ImVec2(x1, y), grid_col);
	}
	drawList->PopClipRect();
	drawList->AddRect(pos, pos + size, border_col);

	if (hovered && (!tooltip_fmt || *tooltip_fmt)) {
		const ImVec2 mouse_pos = GetMousePos();
		const ImVec2 point_pos = mouse_pos - (pos + ImVec2((float)padding, (float)padding));
		const Math::Vec2i mapp = map(point_pos);
		const int x_index = mapp.x;
		const int y_index = mapp.y;
		char buf[16];
		snprintf(buf, GBBASIC_COUNTOF(buf), tooltip_fmt ? tooltip_fmt : "%d: %X", x_index, Math::clamp(min + diff - y_index, min, max));

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(buf);
	}

	bool clicked = false;
	if (new_values) {
		if (mouse_down && *mouse_down && !hovered && down) {
			*mouse_down = false;

			if (prev_pos)
				*prev_pos = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());

			clicked = true;
		} else if (drawing) {
			if (mouse_down)
				*mouse_down = true;

			const ImVec2 mouse_pos = GetMousePos();
			const ImVec2 point_pos = mouse_pos - (pos + ImVec2((float)padding, (float)padding));

			if (prev_pos) {
				if (std::isnan(prev_pos->x) || std::isnan(prev_pos->y)) {
					*prev_pos = point_pos;
				} else {
					const Math::Vec2f p0(prev_pos->x, prev_pos->y);
					const Math::Vec2f p1(point_pos.x, point_pos.y);
					Math::Vec2f dir = p1 - p0;
					if (dir.x != 0 && dir != Math::Vec2f(0, 0)) {
						const Real len = dir.normalize();
						const Real step = std::abs(unit_x / dir.x) * 0.3f;
						dir *= step;
						const int n = (int)(len / step - 1);
						for (int i = 0; i < n; ++i) {
							const Math::Vec2f p = p0 + dir * i;
							paint(new_values, ImVec2((float)p.x, (float)p.y));
						}
					}

					*prev_pos = point_pos;
				}
			}
			paint(new_values, point_pos);
		} else if (hovered && IsMouseReleased(ImGuiMouseButton_Left)) {
			if (mouse_down)
				*mouse_down = true;

			if (prev_pos)
				*prev_pos = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());

			clicked = true;
		}
	}

	return clicked;
}

void TextUnformatted(const std::string &text) {
	TextUnformatted(text.c_str(), text.c_str() + text.length());
}

bool Url(const char* label, const char* link, bool adj) {
	ImVec2 pos = GetCursorPos();
	TextColored(ImColor(41, 148, 255, 255), label);
	if (IsItemHovered()) {
		std::string ul = label;
		std::transform(ul.begin(), ul.end(), ul.begin(), [] (char) -> char { return '_'; });
		SetCursorPos(pos);
		if (adj)
			AlignTextToFramePadding();
		TextColored(ImColor(41, 148, 255, 255), ul.c_str());

		SetMouseCursor(ImGuiMouseCursor_Hand);
	}
	if (IsItemHovered() && IsMouseReleased(ImGuiMouseButton_Left)) { // Used `IsItemHovered()` instead of `IsItemClicked()` to avoid a clicking issue.
		if (link) {
			const std::string osstr = Unicode::toOs(link);

			Platform::surf(osstr.c_str());
		}

		return true;
	}

	return false;
}

void OpenPopupTooltip(const char* id) {
	auto Do = [] (const char* str_id, ImGuiPopupFlags popup_flags) -> void {
		ImGuiContext &g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return;
		ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;
		IM_ASSERT(id != 0);
		OpenPopupEx(id, popup_flags);
	};

	Do(id, ImGuiPopupFlags_None);
}

void PopupTooltip(const char* id, const std::string &text) {
	auto Do = [] (const char* str_id) -> bool {
		ImGuiContext &g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;
		ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;
		IM_ASSERT(id != 0);

		return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	};

	ImGuiStyle &style = GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

	if (Do(id)) {
		TextUnformatted(text);

		EndPopup();
	}
}

bool PopupTooltip(const char* id, const std::string &text, const char* clear_txt) {
	auto Do = [] (const char* str_id) -> bool {
		ImGuiContext &g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;
		ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;
		IM_ASSERT(id != 0);

		return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	};

	ImGuiStyle &style = GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

	bool result = false;

	if (Do(id)) {
		TextUnformatted(text);

		if (clear_txt) {
			NewLine(4);
			if (Button(clear_txt))
				result = true;
		}

		EndPopup();
	}

	return result;
}

void PopupHelpTooltip(const std::string &text) {
	auto Do = [] (const char* str_id, ImGuiPopupFlags popup_flags) -> bool {
		ImGuiContext &g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;
		ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;
		IM_ASSERT(id != 0);
		if ((IsMouseReleased(ImGuiPopupFlags_MouseButtonLeft) || IsMouseReleased(ImGuiPopupFlags_MouseButtonRight)) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
			OpenPopupEx(id, popup_flags);

		return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	};

	ImGuiStyle &style = GetStyle();

	TextUnformatted("[?]");

	if (Do("Tooltip context", ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_MouseButtonRight)) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		TextUnformatted(text);

		EndPopup();
	}
}

void SetTooltip(const std::string &text) {
	SetTooltip(text.c_str());
}

void SetHelpTooltip(const std::string &text) {
	ImGuiStyle &style = GetStyle();

	TextUnformatted("[?]");

	if (!text.empty() && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(text);
	}
}

bool Checkbox(const std::string &label, bool* v) {
	return Checkbox(label.c_str(), v);
}

bool Combo(
	const char* label,
	int* current_item,
	const char* const items[], int items_count,
	const char* const tooltips[],
	int popup_max_height_in_items
) {
	auto Items_ArrayGetter = [] (void* data, int idx, const char** out_text) -> bool {
		const char* const* items = (const char* const*)data;
		if (out_text)
			*out_text = items[idx];

		return true;
	};

	const bool value_changed = Combo(
		label,
		current_item,
		Items_ArrayGetter, (void*)items, items_count,
		Items_ArrayGetter, (void*)tooltips,
		popup_max_height_in_items
	);

	return value_changed;
}

bool Combo(
	const char* label,
	int* current_item,
	bool (* items_getter)(void* /* data */, int /* idx */, const char** /* out_text */), void* data, int items_count,
	bool (* tooltips_getter)(void* /* data */, int /* idx */, const char** /* out_text */), void* tooltips,
	int popup_max_height_in_items
) {
	auto CalcMaxPopupHeightFromItemCount = [] (int items_count) -> float {
		ImGuiContext &g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;

		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	};

	ImGuiContext &g = *GImGui;
	ImGuiStyle &style = GetStyle();

	const char* preview_value = nullptr;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	bool value_changed = false;
	for (int i = 0; i < items_count; ++i) {
		PushID(i);

		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		if (Selectable(item_text, item_selected)) {
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			SetItemDefaultFocus();

		if (IsItemHovered()) {
			const char* tooltip_text = nullptr;
			if (!tooltips_getter(tooltips, i, &tooltip_text))
				tooltip_text = "*Unknown tooltip*";

			if (tooltip_text) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				SetTooltip(tooltip_text);
			}
		}

		PopID();
	}

	EndCombo();

	if (value_changed)
		MarkItemEdited(g.LastItemData.ID);

	return value_changed;
}

bool Combo(const std::string &label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items) {
	return Combo(label.c_str(), current_item, items, items_count, popup_max_height_in_items);
}

bool Combo(const std::string &label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items) {
	return Combo(label.c_str(), current_item, items_separated_by_zeros, popup_max_height_in_items);
}

bool Combo(const std::string &label, int* current_item, bool (* items_getter)(void* /* data */, int /* idx */, const char** /* out_text */), void* data, int items_count, int popup_max_height_in_items) {
	return Combo(label.c_str(), current_item, items_getter, data, items_count, popup_max_height_in_items);
}

void Indicator(const ImVec2 &min, const ImVec2 &max, float thickness) {
	ImDrawList* drawList = GetWindowDrawList();

	const bool tick = !!(((int)(DateTime::toSeconds(DateTime::ticks()) * 1)) % 2);
	drawList->AddRect(
		min, max,
		tick ? IM_COL32_WHITE : IM_COL32_BLACK,
		0, ImDrawFlags_RoundCornersNone,
		thickness
	);
}

void Indicator(const char* label, const ImVec2 &pos) {
	const bool tick = !!(((int)(DateTime::toSeconds(DateTime::ticks()) * 1)) % 2);
	const ImVec2 old = GetCursorPos();
	SetCursorPos(pos);
	TextColored(ColorConvertU32ToFloat4(tick ? IM_COL32(255, 0, 0, 255) : IM_COL32_BLACK_TRANS), label);
	SetCursorPos(old);
}

static bool ProgressBar(const char* label, void* p_data, void* p_data_text, const void* p_min, const void* p_max, const char* format, bool readonly) {
	// See: `SliderScalar` in "./lib/imgui/imgui_widgets.cpp".

	// Prepare.
	ImGuiWindow* window = GetCurrentWindow();

	if (window->SkipItems)
		return false;

	ImGuiContext &g = *GetCurrentContext();
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = CalcItemWidth();

	const ImVec2 label_size = CalcTextSize(label, nullptr, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	// Add an item.
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	// Default format string when passing `nullptr`.
	if (!format)
		format = DataTypeGetInfo(ImGuiDataType_Float)->PrintFmt;

	// Tabbing or Ctrl+LMB on progress turns it into an input box.
	const bool hovered = ItemHoverable(frame_bb, id);
	bool temp_input_is_active = !readonly && TempInputIsActive(id);
	bool temp_input_start = false;
	if (!readonly && !temp_input_is_active) {
		const bool input_requested_by_tabbing = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
		const bool clicked = (hovered && g.IO.MouseClicked[0]);
		if (input_requested_by_tabbing || clicked || g.NavActivateId == id || g.NavActivateInputId == id) {
			SetActiveID(id, window);
			SetFocusID(id, window);
			FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || g.NavActivateInputId == id) {
				temp_input_start = true;
			}
		}
	}

	// Our current specs do NOT clamp when using Ctrl+LMB manual input, but we should eventually add a flag for that.
	if (temp_input_is_active || temp_input_start)
		return TempInputScalar(frame_bb, id, label, ImGuiDataType_Float, p_data, format); // , p_min, p_max);

	// Draw frame.
	const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	RenderNavHighlight(frame_bb, id);
	RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

	// Slider behaviour.
	const float grab_sz = Math::min(style.GrabMinSize, frame_bb.GetWidth());
	const ImRect slider_bb(frame_bb.Min.x - grab_sz, frame_bb.Min.y, frame_bb.Max.x + grab_sz, frame_bb.Max.y);
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(slider_bb, id, ImGuiDataType_Float, p_data, p_min, p_max, format, ImGuiSliderFlags_None, &grab_bb, 0.0f);
	if (value_changed)
		MarkItemEdited(id);

	// Render grab.
	if (grab_bb.Max.x > grab_bb.Min.x) {
		window->DrawList->PushClipRect(frame_bb.Min, frame_bb.Max);
		window->DrawList->AddRectFilled(
			frame_bb.Min,
			ImVec2(grab_bb.GetCenter().x, grab_bb.Max.y),
			GetColorU32(!readonly && g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab),
			style.GrabRounding
		);
		window->DrawList->PopClipRect();
	}

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), ImGuiDataType_Float, p_data_text, format);
	RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, nullptr, ImVec2(0.5f, 0.5f));

	if (label_size.x > 0.0f)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	// Finish.
	return value_changed;
}

bool IntegerModifier(Renderer* rnd, Theme* theme, int* val, int min, int max, float width, const char* fmt, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	bool changed = false;
	if (ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
		*val = Math::max(*val - 1, min);
		changed = true;
	}
	if (tooltip) {
		if (IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltip);
		}
	}
	SameLine();
	SetNextItemWidth(width - 21 * 2);
	if (DragInt("", val, 1.0f, min, max, fmt)) {
		changed = true;
	}
	if (tooltip) {
		if (IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltip);
		}
	}
	SameLine();
	if (ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
		*val = Math::min(*val + 1, max);
		changed = true;
	}
	if (tooltip) {
		if (IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltip);
		}
	}

	return changed;
}

bool ByteMatrice(Renderer*, Theme* theme, Byte* val, bool* set, int* bit, Byte editable, float width, const char* tooltipLnHigh, const char* tooltipLnLow, const char** tooltipBits) {
	ImGuiStyle &style = GetStyle();

	constexpr const char HEX[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
	Byte byte = *val;

	bool result = false;

	const float xPos = GetCursorPosX();
	const float size = std::floor(width / 5.0f);
	const char* tooltipBits_[8] = {
		theme->tooltip_Bit0().c_str(),
		theme->tooltip_Bit1().c_str(),
		theme->tooltip_Bit2().c_str(),
		theme->tooltip_Bit3().c_str(),
		theme->tooltip_Bit4().c_str(),
		theme->tooltip_Bit5().c_str(),
		theme->tooltip_Bit6().c_str(),
		theme->tooltip_Bit7().c_str()
	};
	if (!tooltipLnHigh) {
		tooltipLnHigh = theme->tooltip_HighBits().c_str();
	}
	if (!tooltipLnLow) {
		tooltipLnLow = theme->tooltip_LowBits().c_str();
	}
	if (!tooltipBits) {
		tooltipBits = tooltipBits_;
	}

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	char buf[2] = { '\0', '\0' };
	buf[0] = HEX[(byte >> 4) & 0x0f];
	PushStyleColor(ImGuiCol_Button, 0);
	PushStyleColor(ImGuiCol_ButtonHovered, 0);
	PushStyleColor(ImGuiCol_ButtonActive, 0);
	{
		Button(buf, ImVec2(size, size));

		if (tooltipLnHigh && IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltipLnHigh);
		}
	}
	PopStyleColor(3);
	SameLine();
	for (int i = 7; i >= 4; --i) {
		const bool on = !!(byte & (1 << i));
		const bool ed = !!(editable & (1 << i));
		buf[0] = on ? '1' : '0';
		if (on) {
			if (ed) {
				PushStyleColor(ImGuiCol_Button, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
				PushStyleColor(ImGuiCol_ButtonHovered, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
				PushStyleColor(ImGuiCol_ButtonActive, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
			} else {
				PushStyleColor(ImGuiCol_Button, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
				PushStyleColor(ImGuiCol_ButtonHovered, ColorConvertU32ToFloat4(theme->style()->selectedButtonHoveredColor));
				PushStyleColor(ImGuiCol_ButtonActive, ColorConvertU32ToFloat4(theme->style()->selectedButtonActiveColor));
			}
		}
		PushID(i);
		if (ed) {
			if (Button(buf, ImVec2(size, size))) {
				result = true;

				if (on) {
					byte &= ~(1 << i);
					if (set) *set = false;
					if (bit) *bit = i;
				} else {
					byte |= 1 << i;
					if (set) *set = true;
					if (bit) *bit = i;
				}
				*val = byte;
			}
		} else {
			Button(buf, ImVec2(size, size));
		}
		PopID();
		if (on) {
			PopStyleColor(3);
		}

		if (tooltipBits && tooltipBits[i] && IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltipBits[i]);
		}

		SameLine();
	}
	NewLine();

	SetCursorPosX(xPos);

	buf[0] = HEX[byte & 0x0f];
	PushStyleColor(ImGuiCol_Button, 0);
	PushStyleColor(ImGuiCol_ButtonHovered, 0);
	PushStyleColor(ImGuiCol_ButtonActive, 0);
	{
		Button(buf, ImVec2(size, size));

		if (tooltipLnLow && IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltipLnLow);
		}
	}
	PopStyleColor(3);
	SameLine();
	for (int i = 3; i >= 0; --i) {
		const bool on = !!(byte & (1 << i));
		const bool ed = !!(editable & (1 << i));
		buf[0] = on ? '1' : '0';
		if (on) {
			if (ed) {
				PushStyleColor(ImGuiCol_Button, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
				PushStyleColor(ImGuiCol_ButtonHovered, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
				PushStyleColor(ImGuiCol_ButtonActive, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
			} else {
				PushStyleColor(ImGuiCol_Button, ColorConvertU32ToFloat4(theme->style()->selectedButtonColor));
				PushStyleColor(ImGuiCol_ButtonHovered, ColorConvertU32ToFloat4(theme->style()->selectedButtonHoveredColor));
				PushStyleColor(ImGuiCol_ButtonActive, ColorConvertU32ToFloat4(theme->style()->selectedButtonActiveColor));
			}
		}
		PushID(i);
		if (ed) {
			if (Button(buf, ImVec2(size, size))) {
				result = true;

				if (on) {
					byte &= ~(1 << i);
					if (set) *set = false;
					if (bit) *bit = i;
				} else {
					byte |= 1 << i;
					if (set) *set = true;
					if (bit) *bit = i;
				}
				*val = byte;
			}
		} else {
			Button(buf, ImVec2(size, size));
		}
		PopID();
		if (on) {
			PopStyleColor(3);
		}

		if (tooltipBits && tooltipBits[i] && IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			SetTooltip(tooltipBits[i]);
		}

		SameLine();
	}
	NewLine();

	return result;
}

bool ByteMatrice(Renderer* rnd, Theme* theme, Byte* val, Byte editable, float width, const char* tooltipLnHigh, const char* tooltipLnLow, const char** tooltipBits) {
	return ByteMatrice(rnd, theme, val, nullptr, nullptr, editable, width, tooltipLnHigh, tooltipLnLow, tooltipBits);
}

bool ProgressBar(const char* label, float* v, float v_min, float v_max, const char* format, bool readonly) {
	return ProgressBar(label, v, v, &v_min, &v_max, format, readonly);
}

bool ProgressBar(const char* label, float* v, float* v_text, float v_min, float v_max, const char* format, bool readonly) {
	return ProgressBar(label, v, v_text, &v_min, &v_max, format, readonly);
}

bool Button(const std::string &label, const ImVec2 &size) {
	return Button(label.c_str(), size);
}

bool Button(const char* label, const ImVec2 &size_arg_, ImGuiMouseButton* mouse_button, bool* double_clicked, bool highlight) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	//ImGuiButtonFlags flags_ = mouse_button ?
	//	ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle :
	//	ImGuiButtonFlags_None;

	ImGuiContext &g = *GImGui;
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, nullptr, true);

	ImVec2 pos = window->DC.CursorPos;
	//if ((flags_ & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset)
	//	pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	const ImVec2 size = CalcItemSize(size_arg_, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	//if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
	//	flags_ |= ImGuiButtonFlags_Repeat;

	const bool hovered = IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	bool held = IsMouseDown(ImGuiMouseButton_Left);
	bool pressed = false;
	ImGuiMouseButton button = ImGuiMouseButton_Left;

	if (!held && mouse_button && IsMouseDown(ImGuiMouseButton_Right))
		held = true;
	if (!held && mouse_button && IsMouseDown(ImGuiMouseButton_Middle))
		held = true;
	if (hovered) {
		if (IsMouseClicked(ImGuiMouseButton_Left)) {
			pressed = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Right)) {
			pressed = true;
			button = ImGuiMouseButton_Right;
		} else if (IsMouseClicked(ImGuiMouseButton_Middle)) {
			pressed = true;
			button = ImGuiMouseButton_Middle;
		}
		if (pressed) {
			if (mouse_button)
				*mouse_button = button;
		}
	}
	if (double_clicked) {
		if (IsMouseDoubleClicked(button))
			*double_clicked = true;
	}

	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	if (highlight) {
		ImRect bb_ = bb;
		if (bb_.GetWidth() <= 8)
			--bb_.Max.x;
		RenderNavHighlight(bb_, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_AlwaysDraw | ImGuiNavHighlightFlags_NoRounding);
	}
	RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size, style.ButtonTextAlign, &bb);

	return pressed;
}

bool Button(const std::string &label, const ImVec2 &size, ImGuiMouseButton* mouse_button, bool* double_clicked, bool highlight) {
	return Button(label.c_str(), size, mouse_button, double_clicked, highlight);
}

void CentralizeButton(int count, float width) {
	ImGuiStyle &style = GetStyle();

	const float xAdv = (GetWindowWidth() - width * count - style.ItemSpacing.x * (count - 1)) * 0.5f;
	SetCursorPosX(Math::max(xAdv, 0.0f));
}

bool ColorButton(const char* desc_id, const ImVec4 &col, ImGuiColorEditFlags flags, const ImVec2 &size, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	const bool result = ColorButton(desc_id, col, flags, size);
	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}

	return result;
}

bool ImageButton(ImTextureID texture_id, const ImVec2 &size, const ImVec4& tint_col, bool selected, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	if (selected) {
		const ImVec4 btnCol = GetStyleColorVec4(ImGuiCol_CheckMark);
		PushStyleColor(ImGuiCol_Button, btnCol);
		PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
		PushStyleColor(ImGuiCol_ButtonActive, btnCol);
	}

	const bool result = ImageButton(texture_id, size, ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1, 1, 1, 0), tint_col);

	if (selected) {
		PopStyleColor(3);
	}

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}

	return result;
}

bool AdjusterButton(const char* label, size_t label_len, const ImVec2* sizeptr, const ImVec2 &charSize, ImGuiMouseButton* mouse_button, bool* double_clicked, bool highlight) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	if (label_len == 0)
		label_len = strlen(label);
	const ImVec2 size_arg_ = sizeptr ? *sizeptr : ImVec2(charSize.x * label_len, charSize.y);
	//ImGuiButtonFlags flags_ = mouse_button ?
	//	ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle :
	//	ImGuiButtonFlags_None;

	ImGuiContext &g = *GImGui;
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, nullptr, true);

	ImVec2 pos = window->DC.CursorPos;
	//if ((flags_ & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset)
	//	pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	const ImVec2 size = CalcItemSize(size_arg_, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	//if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
	//	flags_ |= ImGuiButtonFlags_Repeat;

	const bool hovered = IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	bool held = IsMouseDown(ImGuiMouseButton_Left);
	bool pressed = false;
	ImGuiMouseButton button = ImGuiMouseButton_Left;

	if (!held && mouse_button && IsMouseDown(ImGuiMouseButton_Right))
		held = true;
	if (!held && mouse_button && IsMouseDown(ImGuiMouseButton_Middle))
		held = true;
	if (hovered) {
		if (IsMouseClicked(ImGuiMouseButton_Left)) {
			pressed = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Right)) {
			pressed = true;
			button = ImGuiMouseButton_Right;
		} else if (IsMouseClicked(ImGuiMouseButton_Middle)) {
			pressed = true;
			button = ImGuiMouseButton_Middle;
		}
		if (pressed) {
			if (mouse_button)
				*mouse_button = button;
		}
	}
	if (double_clicked) {
		if (IsMouseDoubleClicked(button))
			*double_clicked = true;
	}

	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	if (highlight) {
		ImRect bb_ = bb;
		if (bb_.GetWidth() <= 8)
			--bb_.Max.x;
		RenderNavHighlight(bb_, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_AlwaysDraw | ImGuiNavHighlightFlags_NoRounding);
	}
	RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size, style.ButtonTextAlign, &bb);

	return pressed;
}

bool AdjusterButton(const char* label, size_t label_len, const ImVec2 &charSize, ImGuiMouseButton* mouse_button, bool* double_clicked, bool highlight) {
	return AdjusterButton(label, label_len, nullptr, charSize, mouse_button, double_clicked, highlight);
}

bool AdjusterButton(const std::string &label, const ImVec2* sizeptr, const ImVec2 &charSize, ImGuiMouseButton* mouse_button, bool* double_clicked, bool highlight) {
	return AdjusterButton(label.c_str(), label.length(), sizeptr, charSize, mouse_button, double_clicked, highlight);
}

bool AdjusterButton(const std::string &label, const ImVec2 &charSize, ImGuiMouseButton* mouse_button, bool* double_clicked, bool highlight) {
	return AdjusterButton(label.c_str(), label.length(), nullptr, charSize, mouse_button, double_clicked, highlight);
}

static void NineGridsImageHorizontally(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool top_down) {
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + dst_size);
	ItemSize(dst_size);
	ItemAdd(bb, 0);

	const float width = src_size.x / 3.0f;
	const float height = src_size.y / 3.0f;

	auto middle = [&] (void) -> void {
		drawList->AddImage( // Middle-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 1.0f / 3.0f), ImVec2(2.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Middle-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 1.0f / 3.0f), ImVec2(1.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Middle-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 1.0f / 3.0f), ImVec2(1.0f, 2.0f / 3.0f)
		);
	};
	auto top = [&] (void) -> void {
		drawList->AddImage( // Top-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(1.0f / 3.0f, 0.0f), ImVec2(2.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Top-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Min.y + height),
			ImVec2(0.0f, 0.0f), ImVec2(1.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Top-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(2.0f / 3.0f, 0.0f), ImVec2(1.0f, 1.0f / 3.0f)
		);
	};
	auto bottom = [&] (void) -> void {
		drawList->AddImage( // Bottom-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 2.0f / 3.0f), ImVec2(2.0f / 3.0f, 1.0f)
		);
		drawList->AddImage( // Bottom-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 2.0f / 3.0f), ImVec2(1.0f / 3.0f, 1.0f)
		);
		drawList->AddImage( // Bottom-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 2.0f / 3.0f), ImVec2(1.0f, 1.0f)
		);
	};

	if (top_down) {
		middle();
		top();
		bottom();
	} else {
		middle();
		bottom();
		top();
	}
}

static void NineGridsImageVertically(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool left_to_right) {
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + dst_size);
	ItemSize(dst_size);
	ItemAdd(bb, 0);

	const float width = src_size.x / 3.0f;
	const float height = src_size.y / 3.0f;

	auto middle = [&] (void) -> void {
		drawList->AddImage( // Middle-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 1.0f / 3.0f), ImVec2(2.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Top-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(1.0f / 3.0f, 0.0f), ImVec2(2.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Bottom-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 2.0f / 3.0f), ImVec2(2.0f / 3.0f, 1.0f)
		);
	};
	auto left = [&] (void) -> void {
		drawList->AddImage( // Middle-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 1.0f / 3.0f), ImVec2(1.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Top-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Min.y + height),
			ImVec2(0.0f, 0.0f), ImVec2(1.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Bottom-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 2.0f / 3.0f), ImVec2(1.0f / 3.0f, 1.0f)
		);
	};
	auto right = [&] (void) -> void {
		drawList->AddImage( // Middle-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 1.0f / 3.0f), ImVec2(1.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Top-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(2.0f / 3.0f, 0.0f), ImVec2(1.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Bottom-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 2.0f / 3.0f), ImVec2(1.0f, 1.0f)
		);
	};

	if (left_to_right) {
		middle();
		right();
		left();
	} else {
		middle();
		left();
		right();
	}
}

bool BulletButtons(float width, float height, int count, bool* changed, UInt32 &mask, ImGuiMouseButton* mouse_button, const char* non_selected_text, ImU32 col, const char** tooltips, ImU32 tooltip_col) {
	GBBASIC_ASSERT(count <= sizeof(mask) * 8 && "Wrong size.");

	ImGuiStyle &style = GetStyle();

	bool result = false;
	if (changed)
		*changed = false;
	const ImVec2 pos = GetCursorScreenPos();
	const ImVec2 buttonSize(width / count, height);
	for (int i = 0; i < count; ++i) {
		const bool selected = !!(mask & (1 << i));
		PushID(i + 1);
		if (selected) {
			PushStyleColor(ImGuiCol_Button, col);
			PushStyleColor(ImGuiCol_ButtonHovered, col);
			PushStyleColor(ImGuiCol_ButtonActive, col);
			if (Button("", buttonSize)) {
				if (changed)
					*changed = true;
				mask &= ~(1 << i);
			}
			PopStyleColor(3);

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		} else {
			const ImU32 col_ = GetColorU32(ImGuiCol_FrameBg);
			PushStyleColor(ImGuiCol_Button, col_);
			PushStyleColor(ImGuiCol_ButtonHovered, col_);
			PushStyleColor(ImGuiCol_ButtonActive, col_);
			if (Button(non_selected_text ? non_selected_text : "", buttonSize)) {
				if (changed)
					*changed = true;
				mask |= 1 << i;
			}
			PopStyleColor(3);

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		}
		PopID();
		SameLine();
	}
	NewLine();
	const bool hovering = IsMouseHoveringRect(pos, pos + ImVec2(width, height));
	if (hovering) {
		if (IsMouseClicked(ImGuiMouseButton_Left)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Left;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Right)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Right;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Middle)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Middle;
			result = true;
		}
	}

	return result;
}

bool BulletButtons(float width, float height, int count, bool* changed, UInt32 &mask, ImGuiMouseButton* mouse_button, const char** texts, ImU32 col, const char** tooltips, ImU32 tooltip_col) {
	GBBASIC_ASSERT(count <= sizeof(mask) * 8 && "Wrong size.");

	ImGuiStyle &style = GetStyle();

	bool result = false;
	if (changed)
		*changed = false;
	const ImVec2 pos = GetCursorScreenPos();
	const ImVec2 buttonSize(width / count, height);
	for (int i = 0; i < count; ++i) {
		const bool selected = !!(mask & (1 << i));
		PushID(i + 1);
		if (selected) {
			PushStyleColor(ImGuiCol_Button, col);
			PushStyleColor(ImGuiCol_ButtonHovered, col);
			PushStyleColor(ImGuiCol_ButtonActive, col);
			PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_WindowBg));
			if (Button(texts && texts[i] ? texts[i] : "", buttonSize)) {
				if (changed)
					*changed = true;
				mask &= ~(1 << i);
			}
			PopStyleColor(4);

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		} else {
			const ImU32 col_ = GetColorU32(ImGuiCol_FrameBg);
			PushStyleColor(ImGuiCol_Button, col_);
			PushStyleColor(ImGuiCol_ButtonHovered, col_);
			PushStyleColor(ImGuiCol_ButtonActive, col_);
			if (Button(texts && texts[i] ? texts[i] : "", buttonSize)) {
				if (changed)
					*changed = true;
				mask |= 1 << i;
			}
			PopStyleColor(3);

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		}
		PopID();
		SameLine();
	}
	NewLine();
	const bool hovering = IsMouseHoveringRect(pos, pos + ImVec2(width, height));
	if (hovering) {
		if (IsMouseClicked(ImGuiMouseButton_Left)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Left;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Right)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Right;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Middle)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Middle;
			result = true;
		}
	}

	return result;
}

bool BulletButtons(float width, float height, int count, bool* changed, int* index, ImGuiMouseButton* mouse_button, const char* non_selected_text, ImU32 col, const char** tooltips, ImU32 tooltip_col) {
	ImGuiStyle &style = GetStyle();

	bool result = false;
	if (changed)
		*changed = false;
	const ImVec2 pos = GetCursorScreenPos();
	const ImVec2 buttonSize(width / count, height - 1);
	for (int i = 0; i < count; ++i) {
		const bool selected = index && *index == i;
		PushID(i + 1);
		if (selected) {
			PushStyleColor(ImGuiCol_Button, col);
			PushStyleColor(ImGuiCol_ButtonHovered, col);
			PushStyleColor(ImGuiCol_ButtonActive, col);
			if (Button("", buttonSize)) {
				if (changed)
					*changed = true;
				if (index)
					*index = i;
			}
			PopStyleColor(3);

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		} else {
			if (Button(non_selected_text ? non_selected_text : "", buttonSize)) {
				if (changed)
					*changed = true;
				if (index)
					*index = i;
			}

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		}
		PopID();
		SameLine();
	}
	NewLine();
	const bool hovering = IsMouseHoveringRect(pos, pos + ImVec2(width, height));
	if (hovering) {
		if (IsMouseClicked(ImGuiMouseButton_Left)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Left;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Right)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Right;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Middle)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Middle;
			result = true;
		}
	}

	return result;
}

bool BulletButtons(float width, float height, int count, bool* changed, int* index, ImGuiMouseButton* mouse_button, const char** texts, ImU32 col, const char** tooltips, ImU32 tooltip_col) {
	ImGuiStyle &style = GetStyle();

	bool result = false;
	if (changed)
		*changed = false;
	const ImVec2 pos = GetCursorScreenPos();
	const ImVec2 buttonSize(width / count, height - 1);
	for (int i = 0; i < count; ++i) {
		const bool selected = index && *index == i;
		PushID(i + 1);
		if (selected) {
			PushStyleColor(ImGuiCol_Button, col);
			PushStyleColor(ImGuiCol_ButtonHovered, col);
			PushStyleColor(ImGuiCol_ButtonActive, col);
			PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_WindowBg));
			if (Button(texts && texts[i] ? texts[i] : "", buttonSize)) {
				if (changed)
					*changed = true;
				if (index)
					*index = i;
			}
			PopStyleColor(4);

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		} else {
			if (Button(texts && texts[i] ? texts[i] : "", buttonSize)) {
				if (changed)
					*changed = true;
				if (index)
					*index = i;
			}

			if (tooltips && IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				PushStyleColor(ImGuiCol_Text, tooltip_col);
				SetTooltip(tooltips[i]);
				PopStyleColor();
			}
		}
		PopID();
		SameLine();
	}
	NewLine();
	const bool hovering = IsMouseHoveringRect(pos, pos + ImVec2(width, height));
	if (hovering) {
		if (IsMouseClicked(ImGuiMouseButton_Left)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Left;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Right)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Right;
			result = true;
		} else if (IsMouseClicked(ImGuiMouseButton_Middle)) {
			if (mouse_button)
				*mouse_button = ImGuiMouseButton_Middle;
			result = true;
		}
	}

	return result;
}

bool PianoButtons(float width, float height, int* index, const char** tooltips, ImU32 tooltip_col) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	constexpr const ImU32 WHITE_KEY_COLOR = 0xffffffff;
	constexpr const ImU32 WHITE_KEY_DOWN_COLOR = 0xffdfdfdf;
	constexpr const ImU32 BLACK_KEY_COLOR = 0xff000000;
	constexpr const ImU32 BLACK_KEY_DOWN_COLOR = 0xff505050;

	const float cellWidth = width / 12;
	const float thinWhiteWidth = cellWidth * 1.5f;
	const float thickWhiteWidth = cellWidth * 2.0f;
	const float whiteHeight = height;
	const float blackWidth = cellWidth;
	const float blackHeight = height * 0.5f + 1;
	const ImVec2 thinWhiteSize(thinWhiteWidth, whiteHeight);
	const ImVec2 thickWhiteSize(thickWhiteWidth, whiteHeight);
	const ImVec2 blackSize(blackWidth, blackHeight);

	const ImVec2 oldPos = GetCursorScreenPos();
	ImVec2 pos = oldPos;
	int indexH = -1;
	int indexC = -1;

	const bool hovering = IsMouseHoveringRect(pos, pos + ImVec2(width, height));
	const bool pressed = hovering && IsMouseDown(ImGuiMouseButton_Left);
	const bool clicked = hovering && IsMouseClicked(ImGuiMouseButton_Left);
	drawList->AddRectFilled(pos, pos + ImVec2(width, height), WHITE_KEY_COLOR);

	pos.x += cellWidth;

	if (IsMouseHoveringRect(pos, pos + blackSize)) {
		if (indexH == -1)
			indexH = 1;
		if (clicked)
			indexC = 1;

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[1]);
			PopStyleColor();
		}
	}
	pos.x += cellWidth * 2;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + blackSize)) {
		if (indexH == -1)
			indexH = 3;
		if (indexC == -1 && clicked)
			indexC = 3;

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[3]);
			PopStyleColor();
		}
	}
	pos.x += cellWidth * 3;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + blackSize)) {
		if (indexH == -1)
			indexH = 6;
		if (indexC == -1 && clicked)
			indexC = 6;

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[6]);
			PopStyleColor();
		}
	}
	pos.x += cellWidth * 2;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + blackSize)) {
		if (indexH == -1)
			indexH = 8;
		if (indexC == -1 && clicked)
			indexC = 8;

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[8]);
			PopStyleColor();
		}
	}
	pos.x += cellWidth * 2;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + blackSize)) {
		if (indexH == -1)
			indexH = 10;
		if (indexC == -1 && clicked)
			indexC = 10;

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[10]);
			PopStyleColor();
		}
	}
	pos.x += cellWidth * 2;

	pos = oldPos;

	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thinWhiteSize)) {
		if (indexH == -1)
			indexH = 0;
		if (indexC == -1 && clicked)
			indexC = 0;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thinWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[0]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thinWhiteWidth;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thickWhiteSize)) {
		if (indexH == -1)
			indexH = 2;
		if (indexC == -1 && clicked)
			indexC = 2;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thickWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thickWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[2]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thickWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thickWhiteWidth;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thinWhiteSize)) {
		if (indexH == -1)
			indexH = 4;
		if (indexC == -1 && clicked)
			indexC = 4;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thinWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[4]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thinWhiteWidth;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thinWhiteSize)) {
		if (indexH == -1)
			indexH = 5;
		if (indexC == -1 && clicked)
			indexC = 5;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thinWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[5]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thinWhiteWidth;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thickWhiteSize)) {
		if (indexH == -1)
			indexH = 7;
		if (indexC == -1 && clicked)
			indexC = 7;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thickWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thickWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[7]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thickWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thickWhiteWidth;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thickWhiteSize)) {
		if (indexH == -1)
			indexH = 9;
		if (indexC == -1 && clicked)
			indexC = 9;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thickWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thickWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[9]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thickWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thickWhiteWidth;
	if (indexH == -1 && IsMouseHoveringRect(pos, pos + thinWhiteSize)) {
		if (indexH == -1)
			indexH = 11;
		if (indexC == -1 && clicked)
			indexC = 11;
		if (pressed)
			drawList->AddRectFilled(pos, pos + thinWhiteSize, WHITE_KEY_DOWN_COLOR);
		else
			drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);

		if (tooltips) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			PushStyleColor(ImGuiCol_Text, tooltip_col);
			SetTooltip(tooltips[11]);
			PopStyleColor();
		}
	} else {
		drawList->AddRect(pos, pos + thinWhiteSize, BLACK_KEY_COLOR);
	}
	pos.x += thinWhiteWidth;

	pos = oldPos;
	pos.x += cellWidth;

	if (indexH == 1) {
		if (pressed)
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_DOWN_COLOR);
		else
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	} else {
		drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	}
	pos.x += cellWidth * 2;
	if (indexH == 3) {
		if (pressed)
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_DOWN_COLOR);
		else
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	} else {
		drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	}
	pos.x += cellWidth * 3;
	if (indexH == 6) {
		if (pressed)
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_DOWN_COLOR);
		else
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	} else {
		drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	}
	pos.x += cellWidth * 2;
	if (indexH == 8) {
		if (pressed)
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_DOWN_COLOR);
		else
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	} else {
		drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	}
	pos.x += cellWidth * 2;
	if (indexH == 10) {
		if (pressed)
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_DOWN_COLOR);
		else
			drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	} else {
		drawList->AddRectFilled(pos, pos + blackSize, BLACK_KEY_COLOR);
	}
	pos.x += cellWidth * 2;

	if (index) {
		if (indexC != -1)
			*index = indexC;
	}

	return indexC != -1;
}

void NineGridsImage(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool horizontal, bool normal) {
	if (horizontal)
		NineGridsImageHorizontally(texture_id, src_size, dst_size, normal);
	else
		NineGridsImageVertically(texture_id, src_size, dst_size, normal);
}

bool BeginMenu(const std::string &label, bool enabled) {
	return BeginMenu(label.c_str(), enabled);
}

bool BeginMenu(ImGuiID id, ImTextureID texture_id, const ImVec2 &size, bool enabled, ImU32 col) {
	// See: `BeginMenuEx` in "./lib/imgui/imgui_widgets.cpp".

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	bool menu_is_open = IsPopupOpen(id, ImGuiPopupFlags_None);

	// Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
	ImGuiWindowFlags flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
	if (window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu))
		flags |= ImGuiWindowFlags_ChildWindow;

	// If a menu with same the ID was already submitted, we will append to it, matching the behavior of Begin().
	// We are relying on a O(N) search - so O(N log N) over the frame - which seems like the most efficient for the expected small amount of BeginMenu() calls per frame.
	// If somehow this is ever becoming a problem we can switch to use e.g. ImGuiStorage mapping key to last frame used.
	if (g.MenusIdSubmittedThisFrame.contains(id))
	{
		if (menu_is_open)
			menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
		else
			g.NextWindowData.ClearFlags();          // we behave like Begin() and need to consume those values
		return menu_is_open;
	}

	// Tag menu as used. Next time BeginMenu() with same ID is called it will append to existing menu
	g.MenusIdSubmittedThisFrame.push_back(id);

	bool pressed;
	bool menuset_is_open = !(window->Flags & ImGuiWindowFlags_Popup) && (g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].OpenParentId == window->IDStack.back());
	ImGuiWindow* backed_nav_window = g.NavWindow;
	if (menuset_is_open)
		g.NavWindow = window;  // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent)

	// The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
	// However the final position is going to be different! It is chosen by FindBestWindowPosForPopup().
	// e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
	ImVec2 popup_pos, pos = window->DC.CursorPos;
	PushID(id);
	if (!enabled)
		BeginDisabled();
	const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
	if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
	{
		// Menu inside an horizontal menu bar
		// Selectable extend their highlight by half ItemSpacing in each direction.
		// For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
		popup_pos = ImVec2(pos.x - 1.0f - IM_FLOOR(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + window->MenuBarHeight());
		window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * 0.5f);
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
		float w = size.x;
		ImVec2 icon_pos(window->DC.CursorPos.x + offsets->OffsetIcon, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
		pressed = Selectable("", menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups, ImVec2(w, 0.0f));
		window->DrawList->AddImage(
			texture_id,
			ImVec2(icon_pos.x, icon_pos.y), ImVec2(icon_pos.x, icon_pos.y) + size,
			ImVec2(0, 0), ImVec2(1, 1),
			col
		);
		PopStyleVar();
		window->DC.CursorPos.x += IM_FLOOR(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
	}
	else
	{
		// Menu inside a regular/vertical menu
		// (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
		//  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
		popup_pos = ImVec2(pos.x, pos.y - style.WindowPadding.y);
		float w = size.x;
		float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);
		ImVec2 icon_pos(window->DC.CursorPos.x + offsets->OffsetIcon, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
		pressed = Selectable("", menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(w, 0.0f));
		window->DrawList->AddImage(
			texture_id,
			ImVec2(icon_pos.x, icon_pos.y), ImVec2(icon_pos.x, icon_pos.y) + size,
			ImVec2(0, 0), ImVec2(1, 1),
			col
		);
		RenderArrow(window->DrawList, pos + ImVec2(offsets->OffsetMark + extra_w + g.FontSize * 0.30f, 0.0f), GetColorU32(ImGuiCol_Text), ImGuiDir_Right);
	}
	if (!enabled)
		EndDisabled();

	const bool hovered = (g.HoveredId == id) && enabled;
	if (menuset_is_open)
		g.NavWindow = backed_nav_window;

	bool want_open = false;
	bool want_close = false;
	if (window->DC.LayoutType == ImGuiLayoutType_Vertical) // (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
	{
		// Close menu when not hovering it anymore unless we are moving roughly in the direction of the menu
		// Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
		bool moving_toward_other_child_menu = false;
		ImGuiWindow* child_menu_window = (g.BeginPopupStack.Size < g.OpenPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].SourceWindow == window) ? g.OpenPopupStack[g.BeginPopupStack.Size].Window : NULL;
		if (g.HoveredWindow == window && child_menu_window != NULL && !(window->Flags & ImGuiWindowFlags_MenuBar))
		{
			float ref_unit = g.FontSize; // FIXME-DPI
			ImRect next_window_rect = child_menu_window->Rect();
			ImVec2 ta = (g.IO.MousePos - g.IO.MouseDelta);
			ImVec2 tb = (window->Pos.x < child_menu_window->Pos.x) ? next_window_rect.GetTL() : next_window_rect.GetTR();
			ImVec2 tc = (window->Pos.x < child_menu_window->Pos.x) ? next_window_rect.GetBL() : next_window_rect.GetBR();
			float extra = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, ref_unit * 0.5f, ref_unit * 2.5f);   // add a bit of extra slack.
			ta.x += (window->Pos.x < child_menu_window->Pos.x) ? -0.5f : +0.5f;                     // to avoid numerical issues (FIXME: ??)
			tb.y = ta.y + ImMax((tb.y - extra) - ta.y, -ref_unit * 8.0f);                           // triangle is maximum 200 high to limit the slope and the bias toward large sub-menus // FIXME: Multiply by fb_scale?
			tc.y = ta.y + ImMin((tc.y + extra) - ta.y, +ref_unit * 8.0f);
			moving_toward_other_child_menu = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
			//GetForegroundDrawList()->AddTriangleFilled(ta, tb, tc, moving_toward_other_child_menu ? IM_COL32(0,128,0,128) : IM_COL32(128,0,0,128)); // [DEBUG]
		}
		if (menu_is_open && !hovered && g.HoveredWindow == window && g.HoveredIdPreviousFrame != 0 && g.HoveredIdPreviousFrame != id && !moving_toward_other_child_menu)
			want_close = true;

		// Open
		if (!menu_is_open && pressed) // Click/activate to open
			want_open = true;
		else if (!menu_is_open && hovered && !moving_toward_other_child_menu) // Hover to open
			want_open = true;
		if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right) // Nav-Right to open
		{
			want_open = true;
			NavMoveRequestCancel();
		}
	}
	else
	{
		// Menu bar
		if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
		{
			want_close = true;
			want_open = menu_is_open = false;
		}
		else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
		{
			want_open = true;
		}
		else if (g.NavId == id && g.NavMoveDir == ImGuiDir_Down) // Nav-Down to open
		{
			want_open = true;
			NavMoveRequestCancel();
		}
	}

	if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
		want_close = true;
	if (want_close && IsPopupOpen(id, ImGuiPopupFlags_None))
		ClosePopupToLevel(g.BeginPopupStack.Size, true);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Openable | (menu_is_open ? ImGuiItemStatusFlags_Opened : 0));
	PopID();

	if (!menu_is_open && want_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
	{
		// Don't recycle same menu level in the same frame, first close the other menu and yield for a frame.
		OpenPopup(id);
		return false;
	}

	menu_is_open |= want_open;
	if (want_open)
		OpenPopup(id);

	if (menu_is_open)
	{
		SetNextWindowPos(popup_pos, ImGuiCond_Always); // Note: this is super misleading! The value will serve as reference for FindBestWindowPosForPopup(), not actual pos.
		menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
	}
	else
	{
		g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	}

	return menu_is_open;
}

bool MenuItem(const std::string &label, const char* shortcut, bool selected, bool enabled) {
	return MenuItem(label.c_str(), shortcut, selected, enabled);
}

bool MenuItem(const std::string &label, const char* shortcut, bool* selected, bool enabled) {
	return MenuItem(label.c_str(), shortcut, selected, enabled);
}

bool MenuBarButton(const std::string &label, const ImVec2 &size, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImGuiWindow* window = GetCurrentWindow();

	const ImRect menuBarRect = window->MenuBarRect();

	PushClipRect(menuBarRect.Min + ImVec2(0, 1), menuBarRect.Max + ImVec2(0, -1), false);

	const bool result = Button(label, size);

	PopClipRect();

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}

	return result;
}

bool MenuBarImageButton(ImTextureID texture_id, const ImVec2 &size, const ImVec4& tint_col, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImGuiWindow* window = GetCurrentWindow();

	const ImRect menuBarRect = window->MenuBarRect();

	PushClipRect(menuBarRect.Min + ImVec2(0, 1), menuBarRect.Max + ImVec2(0, -1), false);

	const bool result = ImageButton(texture_id, size, ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1, 1, 1, 0), tint_col);

	PopClipRect();

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}

	return result;
}

void MenuBarTextUnformatted(const std::string &text) {
	ImGuiWindow* window = GetCurrentWindow();

	const ImRect menuBarRect = window->MenuBarRect();

	PushClipRect(menuBarRect.Min + ImVec2(0, 1), menuBarRect.Max + ImVec2(0, -1), false);

	TextUnformatted(text);

	PopClipRect();
}

float ColorPickerMinWidthForInput(void) {
	return 186;
}

float TabBarHeight(void) {
	ImGuiStyle &style = GetStyle();

	return GetFontSize() + style.FramePadding.y * 2.0f;
}

bool BeginTabItem(const std::string &str_id, const std::string &label, bool* p_open, ImGuiTabItemFlags flags) {
	PushID(str_id);
	const bool result = BeginTabItem(label.c_str(), p_open, flags);
	PopID();

	return result;
}

bool BeginTabItem(const std::string &label, bool* p_open, ImGuiTabItemFlags flags) {
	return BeginTabItem(label.c_str(), p_open, flags);
}

bool BeginTabItem(const std::string &label, bool* p_open, ImGuiTabItemFlags flags, ImU32 col) {
	PushStyleColor(ImGuiCol_Text, col);
	const bool result = BeginTabItem(label.c_str(), p_open, flags);
	PopStyleColor();

	return result;
}

void TabBarTabListPopupButton(TabBarDropper dropper) {
	ImGuiStyle &style = GetStyle();

	ImGuiWindow* window = GetCurrentWindow();
	ImGuiTabBar* tab_bar = GImGui->CurrentTabBar;

	const float tab_list_popup_button_width = GetFontSize() + style.FramePadding.y;
	const ImVec2 backup_cursor_pos = window->DC.CursorPos;
	window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x - style.FramePadding.y, tab_bar->BarRect.Min.y);
	tab_bar->BarRect.Min.x += tab_list_popup_button_width;

	ImVec4 arrow_col = GetStyleColorVec4(ImGuiCol_Text);
	arrow_col.w *= 0.5f;
	PushStyleColor(ImGuiCol_Text, arrow_col);
	PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0));
	const bool open = BeginCombo("##V", nullptr, ImGuiComboFlags_NoPreview);
	PopStyleColor(2);

	if (open) {
		if (dropper)
			dropper();

		EndCombo();
	}

	window->DC.CursorPos = backup_cursor_pos;
}

bool BeginTable(const std::string &str_id, int column, ImGuiTableFlags flags, const ImVec2 &outer_size, float inner_width) {
	return BeginTable(str_id.c_str(), column, flags, outer_size, inner_width);
}

void TableSetupColumn(const std::string &label, ImGuiTableColumnFlags flags, float init_width_or_weight, ImU32 user_id) {
	TableSetupColumn(label.c_str(), flags, init_width_or_weight, user_id);
}

static bool TreeNodeBehavior(ImGuiID id, ImTextureID texture_id, ImTextureID open_tex_id, bool* checked, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImGuiButtonFlags button_flags, ImU32 col) {
	// See: `TreeNodeBehavior` in "./lib/imgui/imgui_widgets.cpp".

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
	const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

	if (!label_end)
		label_end = FindRenderedTextEnd(label);
	const ImVec2 label_size = CalcTextSize(label, label_end, false);

	// We vertically grow up to current line height up the typical widget height.
	const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
	ImRect frame_bb;
	frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
	frame_bb.Min.y = window->DC.CursorPos.y;
	frame_bb.Max.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->ClipRect.Max.x : window->WorkRect.Max.x;
	frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
	if (display_frame)
	{
		// Framed header expand a little outside the default padding, to the edge of InnerClipRect
		// (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
		frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
		frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
	}

	const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);           // Collapser arrow width + Spacing
	const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                    // Latch before ItemSize changes it
	const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);  // Include collapser
	ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
	ItemSize(ImVec2(text_width, frame_height), padding.y);

	// For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
	ImRect interact_bb = frame_bb;
	if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0)
	{
		//interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;
		interact_bb.Max.x = frame_bb.Min.x + frame_bb.GetWidth();
	}

	// Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
	// For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
	// This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
	const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
	bool is_open = TreeNodeBehaviorIsOpen(id, flags);
	if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

	bool item_add = ItemAdd(interact_bb, id);
	g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
	g.LastItemData.DisplayRect = frame_bb;

	if (!item_add)
	{
		if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			TreePushOverrideID(id);
		IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
		return is_open;
	}

	if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
		button_flags |= ImGuiButtonFlags_AllowItemOverlap;
	if (!is_leaf)
		button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

	// We allow clicking on the arrow section with keyboard modifiers held, in order to easily
	// allow browsing a tree while preserving selection with code implementing multi-selection patterns.
	// When clicking on the rest of the tree node we always disallow keyboard modifiers.
	const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
	const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
	const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
	if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
		button_flags |= ImGuiButtonFlags_NoKeyModifiers;

	// Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
	// Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
	// - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
	// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
	// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
	// - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
	// - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
	// It is rather standard that arrow click react on Down rather than Up.
	// We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
	if (is_mouse_x_over_arrow)
		button_flags |= ImGuiButtonFlags_PressedOnClick;
	else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	else
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

	bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
	const bool was_selected = selected;

	bool hovered = false, held = false;
	bool pressed = checked ? false : ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
	bool toggled = false;
	if (!is_leaf)
	{
		if (pressed && g.DragDropHoldJustPressedId != id)
		{
			if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
				toggled = true;
			if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
				toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
			if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseDoubleClicked[0])
				toggled = true;
		}
		else if (pressed && g.DragDropHoldJustPressedId == id)
		{
			IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
			if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
				toggled = true;
		}

		if (g.NavId == id && g.NavMoveDir && g.NavMoveDir == ImGuiDir_Left && is_open)
		{
			toggled = true;
			NavMoveRequestCancel();
		}
		if (g.NavId == id && g.NavMoveDir && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
		{
			toggled = true;
			NavMoveRequestCancel();
		}

		if (toggled)
		{
			is_open = !is_open;
			window->DC.StateStorage->SetInt(id, is_open);
			g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}
	if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
		SetItemAllowOverlap();

	// In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
	if (selected != was_selected) //-V547
		g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

	// Render
	const ImU32 text_col = GetColorU32(ImGuiCol_Text);
	ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
	if (display_frame)
	{
		// Framed type
		const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
		RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
		RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		if (flags & ImGuiTreeNodeFlags_Bullet)
		{
			RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
		}
		else if (!is_leaf)
		{
			//RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
			const float offset_x = text_offset_x - 1;
			if (checked)
			{
				SetCursorScreenPos(ImVec2(text_pos.x - offset_x, text_pos.y));
				Checkbox(label, checked);
			}
			else
			{
				window->DrawList->AddImage(
					is_open ? open_tex_id : texture_id,
					ImVec2(text_pos.x - offset_x, text_pos.y), ImVec2(text_pos.x - offset_x, text_pos.y) + ImVec2(g.FontSize, g.FontSize),
					ImVec2(0, 0), ImVec2(1, 1),
					col
				);
			}
		}
		else // Leaf without bullet, left-adjusted text
			text_pos.x -= text_offset_x;
		if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
			frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

		if (g.LogEnabled)
			LogSetNextTextDecoration("###", "###");
		RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
	}
	else
	{
		// Unframed typed for tree nodes
		if (hovered || selected)
		{
			const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
		}
		RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		if (flags & ImGuiTreeNodeFlags_Bullet)
		{
			RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
		}
		else if (!is_leaf)
		{
			//RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);
			const float offset_x = text_offset_x - 1;
			if (checked)
			{
				SetCursorScreenPos(ImVec2(text_pos.x - offset_x, text_pos.y));
				Checkbox(label, checked);
			}
			else
			{
				window->DrawList->AddImage(
					is_open ? open_tex_id : texture_id,
					ImVec2(text_pos.x - offset_x, text_pos.y), ImVec2(text_pos.x - offset_x, text_pos.y) + ImVec2(g.FontSize, g.FontSize),
					ImVec2(0, 0), ImVec2(1, 1),
					col
				);
			}
		}
		if (g.LogEnabled)
			LogSetNextTextDecoration(">", NULL);
		if (!checked)
			RenderText(text_pos, label, label_end, false);
	}

	if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		TreePushOverrideID(id);
	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
	return is_open;
}

bool TreeNode(ImTextureID texture_id, ImTextureID open_tex_id, const std::string &label, ImGuiTreeNodeFlags flags, ImGuiButtonFlags button_flags, ImU32 col) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(GetID(label.c_str()), texture_id, open_tex_id, nullptr, flags, label.c_str(), nullptr, button_flags, col);
}

bool TreeNode(bool* checked, const std::string &label, ImGuiTreeNodeFlags flags, ImGuiButtonFlags button_flags) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(GetID(label.c_str()), nullptr, nullptr, checked, flags, label.c_str(), nullptr, button_flags, IM_COL32_WHITE);
}

bool CollapsingHeader(const char* label, float width, ImGuiTreeNodeFlags flags) {
	bool result = false;
	ImGuiWindow* window = GetCurrentWindow();
	const float dx = GetCursorPosX();
	const float oldX = window->WorkRect.Max.x;
	const float w = window->WorkRect.Max.x - window->WorkRect.Min.x - dx;
	window->WorkRect.Max.x -= w - width;
	result = CollapsingHeader(label, flags);
	window->WorkRect.Max.x = oldX;

	return result;
}

bool Selectable(const std::string &label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
	return Selectable(label.c_str(), selected, flags, size);
}

bool Selectable(const std::string &label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
	return Selectable(label.c_str(), p_selected, flags, size);
}

bool ExporterMenu(
	const Exporter::Array &exporters,
	const char* /* menu */,
	Exporter* &selected,
	ExporterMenuHandler prev, ExporterMenuHandler post
) {
	bool result = false;

	selected = nullptr;

	int layer = 0;
	bool opened = false;
	Hierarchy hierarchy(
		[prev, &layer, &opened] (const std::string &dir) -> bool {
			++layer;

			const bool ret = BeginMenu(dir);

			if (layer == 1) {
				opened = ret;
				if (opened) {
					if (prev) {
						prev();

						Separator();
					}
				}
			}

			return ret;
		},
		[post, &layer, &opened] (void) -> void {
			if (layer == 1) {
				if (opened) {
					if (post) {
						Separator();

						post();
					}
				}
			}

			EndMenu();

			--layer;
		}
	);
	hierarchy.prepare();

	for (const Exporter::Ptr &ex : exporters) {
		const Entry &entry = ex->entry();

		if (entry.parts().empty())
			continue;

		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;

		if (hierarchy.with(begin, end)) {
			const std::string file = entry.parts().back();
			bool clicked = false;
			if (ex->isMajor())
				clicked = MenuItem(file, GBBASIC_MODIFIER_KEY_NAME "+Shift+B");
			else if (ex->isMinor())
				clicked = MenuItem(file, GBBASIC_MODIFIER_KEY_NAME "+Shift+V");
			else
				clicked = MenuItem(file);
			if (clicked) {
				selected = ex.get();

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool ExampleMenu(
	const EntryWithPath::List &examples,
	std::string &selected
) {
	bool result = false;

	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (const EntryWithPath &entry : examples) {
		const std::string &path = entry.path();
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string gbbExt = "." GBBASIC_RICH_PROJECT_EXT;
			const std::string basExt = "." GBBASIC_PLAIN_PROJECT_EXT;
			if (Text::endsWith(file, gbbExt, true))
				file = file.substr(0, file.length() - gbbExt.length());
			else if (Text::endsWith(file, basExt, true))
				file = file.substr(0, file.length() - basExt.length());
			if (MenuItem(file)) {
				selected = path;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool MusicMenu(
	const Entry::Dictionary &music,
	std::string &selected
) {
	bool result = false;

	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Entry::Dictionary::value_type kv : music) {
		const Entry &entry = kv.first;
		const std::string &path = kv.second;
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotJson = ".json";
			if (Text::endsWith(file, dotJson, true))
				file = file.substr(0, file.length() - dotJson.length());
			const bool clicked = MenuItem(file);
			if (clicked) {
				selected = path;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool SfxMenu(
	const Entry::Dictionary &sfx,
	std::string &selected
) {
	bool result = false;

	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Entry::Dictionary::value_type kv : sfx) {
		const Entry &entry = kv.first;
		const std::string &path = kv.second;
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotJson = ".json";
			if (Text::endsWith(file, dotJson, true))
				file = file.substr(0, file.length() - dotJson.length());
			const bool clicked = MenuItem(file);
			if (clicked) {
				selected = path;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool DocumentMenu(
	const Entry::Dictionary &documents,
	std::string &selected
) {
	bool result = false;

	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Entry::Dictionary::value_type kv : documents) {
		const Entry &entry = kv.first;
		const std::string &path = kv.second;
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotMd = "." DOCUMENT_MARKDOWN_EXT;
			if (Text::endsWith(file, dotMd, true))
				file = file.substr(0, file.length() - dotMd.length());
			const bool isManual = file == DOCUMENT_MAIN_NAME;
			bool clicked = false;
			if (isManual)
				clicked = MenuItem(file, "F1");
			else
				clicked = MenuItem(file);
			if (clicked) {
				selected = path;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool LinkMenu(
	const Entry::Dictionary &links,
	std::string &url, std::string &message
) {
	bool result = false;

	url.clear();
	message.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Entry::Dictionary::value_type kv : links) {
		const Entry &entry = kv.first;
		const std::string &content = kv.second;
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotMd = "." DOCUMENT_MARKDOWN_EXT;
			if (Text::endsWith(file, dotMd, true))
				file = file.substr(0, file.length() - dotMd.length());
			const bool isManual = file == DOCUMENT_MAIN_NAME;
			bool clicked = false;
			if (isManual)
				clicked = MenuItem(file, "F1");
			else
				clicked = MenuItem(file);
			if (clicked) {
				const size_t httpIdx = Text::indexOf(content, "http://");
				const size_t httpsIdx = Text::indexOf(content, "https://");
				if (httpIdx != std::string::npos) {
					url = content.substr(httpIdx);
					message = content.substr(0, httpIdx);
				} else if (httpsIdx != std::string::npos) {
					url = content.substr(httpsIdx);
					message = content.substr(0, httpsIdx);
				}
				url = Text::trim(url);
				message = Text::trim(message);

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

void ConfigGamepads(
	Input* input,
	Input::Gamepad* pads, int pad_count,
	int* active_pad_index, int* active_btn_index,
	const char* label_wait
) {
	constexpr const char* const KEY_NAMES[] = {
		"    Up",
		"  Down",
		"  Left",
		" Right",
		"Select",
		" Start",
		"     A",
		"     B"
	};

	for (int i = 0; i < pad_count; ++i) {
		Input::Gamepad &pad = pads[i];

		constexpr const int PLACEHOLDER_INDEX = 6;
		char buf[] = {
			'J', 'o', 'y', 'p', 'a', 'd', '?', '\t', ' ', ' ', ' ', ' ', ' ', '\0'
		};
		buf[PLACEHOLDER_INDEX] = (char)(i + '1'); // Put pad index at the first placeholder.
		if (CollapsingHeader(buf, ImGuiTreeNodeFlags_None)) {
			for (int b = 0; b < Input::BUTTON_COUNT; ++b) {
				buf[PLACEHOLDER_INDEX + 1] = (char)(b + '1'); // Put button index at the second placeholder.

				PushID(buf);

				if (b == Input::SELECT)
					Separator();
				else if (b == Input::A)
					Separator();

				const std::string key = input->nameOf(pad.buttons[b]);

				AlignTextToFramePadding();
				Text(KEY_NAMES[b]);

				SameLine();
				if (active_pad_index && *active_pad_index == i && active_btn_index && *active_btn_index == b) {
					if (Button(label_wait ? label_wait : "Waiting for input...", ImVec2(-1, 0))) {
						*active_pad_index = -1;
						*active_btn_index = -1;
					}
					Input::Button btn;
					if (input->pressed(btn)) {
						if (btn.device == Input::KEYBOARD && btn.value == SDL_SCANCODE_BACKSPACE) {
							btn.device = Input::INVALID;
							btn.index = 0;
							btn.value = 0;
						}
						pad.buttons[b] = btn;
						*active_pad_index = -1;
						*active_btn_index = -1;
					}
				} else {
					if (Button(key.c_str(), ImVec2(-1, 0))) {
						*active_pad_index = i;
						*active_btn_index = b;
					}
				}

				PopID();
			}
		}
	}
}

void ConfigOnscreenGamepad(
	bool* enabled,
	bool* swap_ab,
	float* scale, float* padding_x, float* padding_y,
	const char* label_enabled,
	const char* label_swap_ab,
	const char* label_scale, const char* label_padding_x, const char* label_padding_y
) {
	Checkbox(label_enabled ? label_enabled : "Enabled", enabled);

	Checkbox(label_swap_ab ? label_swap_ab : "Swap A/B", swap_ab);

	PushID("@Scl");
	{
		TextUnformatted(label_scale ? label_scale : "    Scale");
		SameLine();
		PushItemWidth(-1.0f);
		if (DragFloat("", scale, 0.005f, 1.0f, INPUT_GAMEPAD_MAX_SCALE, "%.1f"))
			*scale = Math::clamp(*scale, 1.0f, INPUT_GAMEPAD_MAX_SCALE);
		PopItemWidth();
	}
	PopID();

	PushID("@PadX");
	{
		TextUnformatted(label_padding_x ? label_padding_x : "Padding X");
		SameLine();
		PushItemWidth(-1.0f);
		if (DragFloat("", padding_x, 0.05f, 0.0f, INPUT_GAMEPAD_MAX_X_PADDING, "%.1f%%"))
			*padding_x = Math::clamp(*padding_x, 0.0f, INPUT_GAMEPAD_MAX_X_PADDING);
		PopItemWidth();
	}
	PopID();

	PushID("@PadY");
	{
		TextUnformatted(label_padding_y ? label_padding_y : "Padding Y");
		SameLine();
		PushItemWidth(-1.0f);
		if (DragFloat("", padding_y, 0.05f, 0.0f, INPUT_GAMEPAD_MAX_Y_PADDING, "%.1f%%"))
			*padding_y = Math::clamp(*padding_y, 0.0f, INPUT_GAMEPAD_MAX_Y_PADDING);
		PopItemWidth();
	}
	PopID();
}

}

/* ===========================================================================} */
