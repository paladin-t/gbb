/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_arbitrary.h"
#include "project.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/encoding.h"
#include <SDL.h>

/*
** {===========================================================================
** Arbitrary editor
*/

class EditorArbitraryImpl : public EditorArbitrary {
private:
	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	int _fontIndex = -1;
	Font::Codepoints::Ptr _shadow = nullptr;
	Text::Array _headers;
	ChangedHandler _changed = nullptr; // Foreign.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	AppliedHandler _appliedHandler = nullptr;
	std::string _applyText;

	ImGui::Initializer _init;

public:
	EditorArbitraryImpl(
		Renderer* rnd, Workspace* ws,
		Theme* theme,
		ChangedHandler changed,
		const std::string &title,
		Project* prj, int fontIndex,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt, const char* cancelTxt, const char* applyTxt
	) : _renderer(rnd),
		_workspace(ws),
		_theme(theme),
		_changed(changed),
		_title(title),
		_project(prj), _fontIndex(fontIndex),
		_confirmedHandler(confirm), _canceledHandler(cancel), _appliedHandler(apply)
	{
		const FontAssets::Entry* entry = _project->getFont(_fontIndex);
		const Font::Codepoints &arb = entry->arbitrary;
		Font::Codepoints* ptr = nullptr;
		arb.clone(&ptr);
		_shadow = Font::Codepoints::Ptr(ptr);

		for (int i = 0; i < std::numeric_limits<Byte>::max() + 1 && i < _shadow->count(); ++i) {
			std::u32string u32str;
			u32str.push_back((char32_t)_shadow->get(i));
			std::string ch = Unicode::fromUtf32(u32str);
			ch = translate(ch);
			const std::string str = Text::toString(i) + "(" + ch + ")";
			_headers.push_back(str);
		}

		if (confirmTxt)
			_confirmText = confirmTxt;
		if (cancelTxt)
			_cancelText = cancelTxt;
		if (applyTxt)
			_applyText = applyTxt;
	}
	virtual ~EditorArbitraryImpl() override {
	}

	virtual void update(Workspace* ws) override {
		ImGuiStyle &style = ImGui::GetStyle();

		bool isOpen = true;
		bool toConfirm = false;
		bool toApply = false;
		bool toCancel = false;

		if (_init.begin())
			ImGui::OpenPopup(_title);

		bool arbitrayAppliable = false;
		if (ImGui::BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
			{
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

				constexpr const float width = 266;
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0));
				ImGui::BeginChild("@Pat", ImVec2(width + 4, 110), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
				{
					constexpr const int ITEMS_PER_LINE = 2;
					const float ITEM_WIDTH = (width - style.ScrollbarSize - (ITEMS_PER_LINE - 1)) / ITEMS_PER_LINE;
					constexpr const float HEADER_WIDTH = 60;
					constexpr const float MIN_WIDTH = 19;
					int toRemove = -1;
					for (int i = 0; i < std::numeric_limits<Byte>::max() + 1 && i < _shadow->count(); ++i) {
						const float oldX = ImGui::GetCursorPosX();
						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted(_headers[i]);
						ImGui::SameLine();
						ImGui::SetCursorPosX(oldX + HEADER_WIDTH);
						ImGui::PushID(i);
						{
							char buf[16];
							snprintf(buf, GBBASIC_COUNTOF(buf), "%d", _shadow->get(i));
							ImGui::SetNextItemWidth(ITEM_WIDTH - HEADER_WIDTH - MIN_WIDTH);
							if (ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll) && *buf) {
								const long long num = std::atoll(buf);
								std::u32string u32str;
								u32str.push_back((char32_t)num);
								std::string ch = Unicode::fromUtf32(u32str);
								const int n = Unicode::expectUtf8(ch.c_str());
								if (n > 0) {
									ch = ch.substr(0, n);
									u32str = Unicode::toUtf32(ch);
									ch = translate(ch);
									const std::string str = Text::toString(i) + "(" + ch + ")";
									_headers[i] = str;
									_shadow->set(i, (Font::Codepoint)u32str.front());
								}
							}
							ImGui::SameLine();
							if (ImGui::Button("-", ImVec2(MIN_WIDTH, 0))) {
								toRemove = i;
							}
							ImGui::SameLine();
							if (i % ITEMS_PER_LINE == 0) {
								ImGui::Dummy(ImVec2(4, 0));
								ImGui::SameLine();
							} else {
								ImGui::Dummy(ImVec2(0, 2));
							}
						}
						ImGui::PopID();
						if ((i + 1) % ITEMS_PER_LINE != 0) {
							ImGui::SameLine();
							ImGui::Dummy(ImVec2(1, 1));
							ImGui::SameLine();
						} else {
							ImGui::NewLine(1);
						}
					}
					if (_shadow->count() % 2) {
						ImGui::NewLine();
					}
					if (_shadow->count() < std::numeric_limits<Byte>::max() + 1) {
						if (ImGui::Button("+", ImVec2(ITEM_WIDTH, 0))) {
							const std::string ch = "?";
							const std::string str = Text::toString(_shadow->count()) + "(" + ch + ")";
							_headers.push_back(str);
							_shadow->add('?');
						}
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(5, 0));
						ImGui::SameLine();
					}
					if (ImGui::Button(_theme->generic_Clear(), ImVec2(ITEM_WIDTH, 0))) {
						_headers.clear();
						_shadow->clear();
					}
					if (toRemove >= 0) {
						_headers.erase(_headers.begin() + toRemove);
						_shadow->remove(toRemove);
						for (int j = 0; j < (int)_headers.size(); ++j) {
							std::u32string u32str;
							u32str.push_back((char32_t)_shadow->get(j));
							std::string ch = Unicode::fromUtf32(u32str);
							ch = translate(ch);
							_headers[j] = Text::toString(j) + "(" + ch + ")";
						}
					}
				}
				ImGui::EndChild();
				ImGui::PopStyleColor();
			}
			ImGui::NewLine(1);

			ImGui::Url(
				_theme->menu_Howto().c_str(),
				[&] (void) -> const char* {
					const EntryWithVisibility::Dictionary &dict = ws->links();
					auto it = dict.find(EntryWithVisibility("tutorial/arbitrary-characters"));
					if (it != dict.end() && !it->second.empty())
						return it->second.c_str();

					return "https://paladin-t.github.io/kits/gbb/learn/arbitrary-characters.html";
				}
			);

			ImGui::SameLine();
			ImGui::NewLine(2);

			const char* confirm = _confirmText.c_str();
			const char* apply = _applyText.empty() ? "Apply" : _applyText.c_str();
			const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

			const FontAssets::Entry* entry = _project->getFont(_fontIndex);
			const Font::Codepoints &arb = entry->arbitrary;
			arbitrayAppliable = *_shadow != arb;

			if (_confirmText.empty()) {
				confirm = "Ok";
			}

			ImGui::AlignTextToFramePadding();
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
			ImGui::SetHelpTooltip(_theme->tooltipArbitrary_Note());
#else /* Platform macro. */
			ImGui::PopupHelpTooltip(_theme->tooltipArbitrary_Note());
#endif /* Platform macro. */
			ImGui::SameLine();

			ImGui::CentralizeButton(3);

			if (ImGui::Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_Y)) {
				toConfirm = true;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
				toCancel = true;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (arbitrayAppliable) {
				if (ImGui::Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
					toApply = true;
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
				ImGui::EndDisabled();
			}

			ImGui::SameLine();
			if (ImGui::ImageButton(_theme->iconReset()->pointer(_renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, _theme->tooltip_Reset().c_str())) {
				_shadow->clear();
				_headers.clear();
				for (int i = 0; i < std::numeric_limits<Byte>::max() + 1; ++i) {
					_shadow->add(i);
					std::u32string u32str;
					u32str.push_back((char32_t)i);
					std::string ch = Unicode::fromUtf32(u32str);
					ch = translate(ch);
					const std::string str = Text::toString(i) + "(" + ch + ")";
					_headers.push_back(str);
				}
			}

			if (!_init.begin() && !_init.end())
				ImGui::CentralizeWindow();

			ImGui::EnsureWindowVisible();

			ImGui::EndPopup();
		}

		if (isOpen)
			_init.update();

		if (!isOpen)
			toCancel = true;

		if (toConfirm) {
			_init.reset();

			if (arbitrayAppliable && _changed != nullptr) {
				_changed({ Object::Ptr(_shadow) });
			}
			if (!_confirmedHandler.empty()) {
				_confirmedHandler();

				return;
			}
		}
		if (toApply) {
			if (!_appliedHandler.empty()) {
				_appliedHandler();
			}
			if (arbitrayAppliable && _changed != nullptr) {
				_changed({ Object::Ptr(_shadow) });
			}
			if (!_appliedHandler.empty()) {
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

private:
	std::string translate(const std::string &ch_) const {
		std::string ch = ch_;
		if (ch == "\r")
			ch = "\\r";
		else if (ch == "\n")
			ch = "\\n";
		else if (ch == "\t")
			ch = "\\t";

		return ch;
	}
};

EditorArbitrary* EditorArbitrary::create(
	Renderer* rnd, class Workspace* ws,
	class Theme* theme,
	ChangedHandler changed,
	const std::string &title,
	class Project* prj, int fontIndex,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
	const char* confirmTxt, const char* cancelTxt, const char* applyTxt
) {
	EditorArbitraryImpl* result = new EditorArbitraryImpl(
		rnd, ws,
		theme,
		changed,
		title,
		prj, fontIndex,
		confirm, cancel, apply,
		confirmTxt, cancelTxt, applyTxt
	);

	return result;
}

void EditorArbitrary::destroy(EditorArbitrary* ptr) {
	EditorArbitraryImpl* impl = static_cast<EditorArbitraryImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
