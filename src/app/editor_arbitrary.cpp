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
	FontAssets::Entry::CodepointRanges _rangesShadow;
	int _selectedPresetRangeIndex = -1;
	Font::Codepoint _rangeStart = 0;
	Font::Codepoint _rangeEnd = 0;
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
		_rangesShadow = entry->ranges;

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
		bool isOpen = true;
		bool toConfirm = false;
		bool toApply = false;
		bool toCancel = false;

		if (_init.begin())
			ImGui::OpenPopup(_title);

		bool arbitrayAppliable = false;
		bool rangesAppliable = false;
		if (ImGui::BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
			{
				if (ImGui::BeginTabBar("@Tabs")) {
					if (ImGui::BeginTabItem(ws->theme()->windowArbitrary_Basic())) {
						updateArbitraryTab(ws);

						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ws->theme()->windowArbitrary_Ranges())) {
						updateRangesTab(ws);

						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
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
			rangesAppliable = _rangesShadow != entry->ranges;

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
			if (arbitrayAppliable || rangesAppliable) {
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
				_changed({ Object::Ptr(_shadow), Variant(0) });
			}
			if (rangesAppliable && _changed != nullptr) {
				_changed({ Variant((void*)&_rangesShadow), Variant(1) });
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
				_changed({ Object::Ptr(_shadow), Variant(0) });
			}
			if (rangesAppliable && _changed != nullptr) {
				_changed({ Variant((void*)&_rangesShadow), Variant(1) });
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
	void updateArbitraryTab(Workspace*) {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

		constexpr const float width = 280;
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
					snprintf(buf, GBBASIC_COUNTOF(buf), "\\u%04X", (unsigned)_shadow->get(i));
					ImGui::SetNextItemWidth(ITEM_WIDTH - HEADER_WIDTH - MIN_WIDTH);
					if (ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_AutoSelectAll) && *buf) {
						Font::Codepoint cp = 0;
						if (buf[0] == '\\' && (buf[1] == 'u' || buf[1] == 'U')) {
							unsigned hex = 0;
							if (sscanf(buf + 2, "%x", &hex) == 1 && hex <= 0x10ffff && !(hex >= 0xd800 && hex <= 0xdfff))
								cp = (Font::Codepoint)hex;
						} else {
							const long long num = std::atoll(buf);
							if (num >= 0 && num <= 0x10ffff && !(num >= 0xd800 && num <= 0xdfff))
								cp = (Font::Codepoint)num;
						}
						if (cp > 0) {
							std::u32string u32str;
							u32str.push_back((char32_t)cp);
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
	void updateRangesTab(Workspace* ws) {
		struct Preset {
			const char* name = nullptr;
			FontAssets::Entry::CodepointRanges ranges;

			constexpr Preset(const char* n, const FontAssets::Entry::CodepointRanges &r)
				: name(n), ranges(r)
			{
			}
		};

		const Preset PRESETS[] = {
			{ "Basic Latin",                 FontAssets::Entry::CodepointRanges{ { 0x0000, 0x007f } } },
			{ "Latin-1 Supplement",          FontAssets::Entry::CodepointRanges{ { 0x0080, 0x00ff } } },
			{ "Latin Extended-A",            FontAssets::Entry::CodepointRanges{ { 0x0100, 0x017f } } },
			{ "Latin Extended-B",            FontAssets::Entry::CodepointRanges{ { 0x0180, 0x024f } } },
			{ "Greek and Coptic",            FontAssets::Entry::CodepointRanges{ { 0x0370, 0x03ff } } },
			{ "Cyrillic",                    FontAssets::Entry::CodepointRanges{ { 0x0400, 0x04ff } } },
			{ "Cyrillic Supplement",         FontAssets::Entry::CodepointRanges{ { 0x0500, 0x052f } } },
			{ "CJK Symbols and Punctuation", FontAssets::Entry::CodepointRanges{ { 0x3000, 0x303f } } },
			{ "Hiragana",                    FontAssets::Entry::CodepointRanges{ { 0x3040, 0x309f } } },
			{ "Katakana",                    FontAssets::Entry::CodepointRanges{ { 0x30a0, 0x30ff } } },
			{ "CJK Ideographs (4E00-5BFF)",  FontAssets::Entry::CodepointRanges{ { 0x4e00, 0x5bff } } },
			{ "CJK Ideographs (5C00-67FF)",  FontAssets::Entry::CodepointRanges{ { 0x5c00, 0x67ff } } },
			{ "CJK Ideographs (6800-73FF)",  FontAssets::Entry::CodepointRanges{ { 0x6800, 0x73ff } } },
			{ "CJK Ideographs (7400-7FFF)",  FontAssets::Entry::CodepointRanges{ { 0x7400, 0x7fff } } },
			{ "CJK Ideographs (8000-8BFF)",  FontAssets::Entry::CodepointRanges{ { 0x8000, 0x8bff } } },
			{ "CJK Ideographs (8C00-97FF)",  FontAssets::Entry::CodepointRanges{ { 0x8c00, 0x97ff } } },
			{ "CJK Ideographs (9800-9FFF)",  FontAssets::Entry::CodepointRanges{ { 0x9800, 0x9fff } } }
		};
		const char* items[GBBASIC_COUNTOF(PRESETS)];
		for (int i = 0; i < GBBASIC_COUNTOF(PRESETS); ++i)
			items[i] = PRESETS[i].name;

		if (_selectedPresetRangeIndex == -1) {
			_selectedPresetRangeIndex = 0;
			const FontAssets::Entry::CodepointRanges &ranges = PRESETS[_selectedPresetRangeIndex].ranges;
			if (!ranges.empty()) {
				_rangeStart = ranges.front().first;
				_rangeEnd = ranges.front().second;
			}
		}

		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

		constexpr const float width = 280;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0));
		ImGui::BeginChild("@Rng", ImVec2(width + 4, 90), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
		{
			int toRemove = -1;
			for (int i = 0; i < (int)_rangesShadow.size(); ++i) {
				ImGui::PushID(i);
				{
					Font::Codepoint first = _rangesShadow[i].first;
					Font::Codepoint last = _rangesShadow[i].second;
					char buf[64];
					snprintf(buf, GBBASIC_COUNTOF(buf), "U+%04X - U+%04X (%u)", (unsigned)first, (unsigned)last, (unsigned)(last - first + 1));
					const float posX = ImGui::GetContentRegionAvail().x - 19;
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(buf);
					ImGui::SameLine();
					ImGui::SetCursorPosX(posX);
					if (ImGui::Button("-", ImVec2(19, 0))) {
						toRemove = i;
					}
				}
				ImGui::PopID();
				ImGui::NewLine(1);
			}
			if (toRemove >= 0) {
				_rangesShadow.erase(_rangesShadow.begin() + toRemove);
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::NewLine(1);

		float clrPosY = ImGui::GetCursorPosY();
		ImGui::PushID("Pst");
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->windowArbitrary_Ranges_Preset());
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(4, 0));
			ImGui::SameLine();
			do {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 19 - 1 * 2 - 43);
				if (ImGui::Combo("", &_selectedPresetRangeIndex, items, GBBASIC_COUNTOF(items))) {
					// Do nothing.
				}
			} while (false);

			ImGui::SameLine();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(19, 0))) {
				if (_selectedPresetRangeIndex >= 0 && _selectedPresetRangeIndex < GBBASIC_COUNTOF(PRESETS)) {
					const FontAssets::Entry::CodepointRanges &ranges = PRESETS[_selectedPresetRangeIndex].ranges;
					for (const FontAssets::Entry::CodepointRange &range : ranges)
						_rangesShadow.push_back(range);
				}
			}
		}
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::NewLine(1);
		ImGui::NewLine(1);

		ImGui::PushID("New");
		{
			char startBuf[16];
			char endBuf[16];
			snprintf(startBuf, GBBASIC_COUNTOF(startBuf), "\\u%04X", (unsigned)_rangeStart);
			snprintf(endBuf, GBBASIC_COUNTOF(endBuf), "\\u%04X", (unsigned)_rangeEnd);

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->windowArbitrary_Ranges_Range());
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(4, 0));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80);
			if (ImGui::InputText("##start", startBuf, sizeof(startBuf), ImGuiInputTextFlags_AutoSelectAll) && *startBuf) {
				_rangeStart = parseCodepoint(startBuf);
			}

			ImGui::SameLine();
			ImGui::Dummy(ImVec2(3, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("-");
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(4, 0));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(80);
			if (ImGui::InputText("##end", endBuf, sizeof(endBuf), ImGuiInputTextFlags_AutoSelectAll) && *endBuf) {
				_rangeEnd = parseCodepoint(endBuf);
			}

			ImGui::SameLine();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(19, 0))) {
				if (_rangeStart <= _rangeEnd) {
					_rangesShadow.push_back(std::make_pair(_rangeStart, _rangeEnd));
				}
			}

			ImGui::SameLine();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::SetCursorPosY(clrPosY);
			if (ImGui::Button(_theme->generic_Clear(), ImVec2(0, 39))) {
				_rangesShadow.clear();
			}
		}
		ImGui::PopID();
	}

	static std::string translate(const std::string &ch_) {
		std::string ch = ch_;
		if (ch == "\r")
			ch = "\\r";
		else if (ch == "\n")
			ch = "\\n";
		else if (ch == "\t")
			ch = "\\t";

		return ch;
	}

	static Font::Codepoint parseCodepoint(const char* buf) {
		if (!buf || !*buf)
			return 0;

		if (buf[0] == '\\' && (buf[1] == 'u' || buf[1] == 'U')) {
			unsigned hex = 0;
			if (sscanf(buf + 2, "%x", &hex) == 1 && hex <= 0x10ffff && !(hex >= 0xd800 && hex <= 0xdfff))
				return (Font::Codepoint)hex;
		} else {
			const long long num = std::atoll(buf);
			if (num >= 0 && num <= 0x10ffff && !(num >= 0xd800 && num <= 0xdfff))
				return (Font::Codepoint)num;
		}

		return 0;
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
