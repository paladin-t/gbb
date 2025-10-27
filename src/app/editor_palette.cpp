/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_palette.h"
#include "project.h"
#include "theme.h"
#include <SDL.h>

/*
** {===========================================================================
** Palette editor
*/

class EditorPaletteImpl : public EditorPalette {
private:
	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	PaletteAssets _shadow;
	int _group = -1;
	int _shadowGroup = -1;
	ChangedHandler _changed = nullptr; // Foreign.
	int _editingColorGroup = -1;
	int _editingColorIndex = -1;
	bool _openColorPicker = false;

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	AppliedHandler _appliedHandler = nullptr;
	std::string _applyText;

	ImGui::Initializer _init;

public:
	EditorPaletteImpl(
		Renderer* rnd, class Workspace* ws,
		Theme* theme,
		int group, ChangedHandler changed,
		const std::string &title,
		Project* prj,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt, const char* cancelTxt, const char* applyTxt
	) : _renderer(rnd),
		_workspace(ws),
		_theme(theme),
		_group(group), _shadowGroup(group), _changed(changed),
		_title(title),
		_project(prj),
		_confirmedHandler(confirm), _canceledHandler(cancel), _appliedHandler(apply)
	{
		const PaletteAssets &assets = _project->touchPalette();
		_shadow.copyFrom(assets, nullptr);

		if (confirmTxt)
			_confirmText = confirmTxt;
		if (cancelTxt)
			_cancelText = cancelTxt;
		if (applyTxt)
			_applyText = applyTxt;
	}
	virtual ~EditorPaletteImpl() override {
	}

	virtual void update(Workspace*) override {
		ImGuiStyle &style = ImGui::GetStyle();

		bool isOpen = true;
		bool toConfirm = false;
		bool toApply = false;
		bool toCancel = false;

		if (_init.begin())
			ImGui::OpenPopup(_title);

		constexpr const int ORD[] = {
			0, 8,
			1, 9,
			2, 10,
			3, 11,
			4, 12,
			5, 13,
			6, 14,
			7, 15
		};
		constexpr const char* const GRP[] = {
			"BG0",  "BG1",  "BG2",  "BG3",  "BG4",  "BG5",  "BG6",  "BG7",
			"OBJ0", "OBJ1", "OBJ2", "OBJ3", "OBJ4", "OBJ5", "OBJ6", "OBJ7"
		};

		bool paletteAppliable = false;
		bool groupAppliable = false;
		if (ImGui::BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
			GBBASIC_ASSERT(_shadow.count() == GBBASIC_COUNTOF(ORD) && _shadow.count() == GBBASIC_COUNTOF(GRP) && "Impossible.");
			const int n = Math::min((int)GBBASIC_COUNTOF(ORD), (int)GBBASIC_COUNTOF(GRP), _shadow.count());
			for (int k = 0; k < n; ++k) {
				const int j = ORD[k];
				PaletteAssets::Entry* entry = _shadow.get(j);
				Indexed::Ptr &palette = entry->data;

				bool active = _shadowGroup == j;
				if (_group == -1) {
					ImGui::BeginDisabled();
					ImGui::Checkbox(GRP[j], &active);
					ImGui::EndDisabled();
				} else {
					if (ImGui::Checkbox(GRP[j], &active)) {
						_shadowGroup = j;
					}
				}
				ImGui::SameLine();

				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

				for (int i = 0; i < palette->count(); ++i) {
					Colour color;
					palette->get(i, color);
					const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

					ImGui::PushID(i);

					const float size = 19.0f;
					if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size))) {
						// Do nothing.
					}
					ImGui::SameLine();

					ImGui::PopID();

					if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
						_editingColorGroup = j;
						_editingColorIndex = i;
						_openColorPicker = true;
					}
				}

				if (k % 2 == 0) {
					ImGui::Dummy(ImVec2(3, 0));
					ImGui::SameLine();
				} else {
					ImGui::NewLine();
					ImGui::NewLine(3);
				}
			}
			if (_openColorPicker) {
				_openColorPicker = false;
				ImGui::OpenPopup("@Pkr");
			}
			do {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

				if (ImGui::BeginPopup("@Pkr")) {
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					PaletteAssets::Entry* entry = _shadow.get(_editingColorGroup);
					Indexed::Ptr &palette = entry->data;

					Colour col;
					palette->get(_editingColorIndex, col);
					float col4f[] = {
						Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
						Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
						Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
						Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
					};

					if (ImGui::ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
						const Colour value(
							(Byte)(col4f[0] * 255),
							(Byte)(col4f[1] * 255),
							(Byte)(col4f[2] * 255),
							(Byte)(col4f[3] * 255)
						);
						palette->set(_editingColorIndex, &value);
					}

					ImGui::EndPopup();
				} else {
					_editingColorGroup = -1;
					_editingColorIndex = -1;
				}
			} while (false);

			const char* confirm = _confirmText.c_str();
			const char* apply = _applyText.empty() ? "Apply" : _applyText.c_str();
			const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

			const PaletteAssets &assets = _project->touchPalette();
			paletteAppliable = !assets.equals(_shadow);
			groupAppliable = _group != _shadowGroup;

			if (_confirmText.empty()) {
				confirm = "Ok";
			}

			ImGui::AlignTextToFramePadding();
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
			ImGui::SetHelpTooltip(_theme->tooltipPalette_Note());
#else /* Platform macro. */
			ImGui::PopupHelpTooltip(_theme->tooltipPalette_Note());
#endif /* Platform macro. */
			ImGui::SameLine();

			ImGui::CentralizeButton(3);

			if (ImGui::Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_RETURN) || ImGui::IsKeyReleased(SDL_SCANCODE_Y)) {
				toConfirm = true;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
				toCancel = true;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (paletteAppliable || groupAppliable) {
				if (ImGui::Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
					toApply = true;
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
				ImGui::EndDisabled();
			}

			ImGui::SameLine();
			if (ImGui::ImageButton(_theme->iconCopy()->pointer(_renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, _theme->tooltip_GenerateAndCopyCode().c_str())) {
				std::string txt;
				if (_shadow.serializeBasic(txt))
					Platform::setClipboardText(txt.c_str());
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

			if (_confirmedHandler.empty()) {
				if ((paletteAppliable || groupAppliable) && _changed != nullptr) {
					_group = _shadowGroup;
					_changed({ _shadowGroup });
				}
			} else {
				const ChangedHandler changed = _changed;
				const int shadowGroup = _shadowGroup;

				_confirmedHandler(_shadow);

				if ((paletteAppliable || groupAppliable) && changed != nullptr) {
					changed({ shadowGroup });
				}

				return;
			}
		}
		if (toApply) {
			if (!_appliedHandler.empty()) {
				_appliedHandler(_shadow);
			}
			if ((paletteAppliable || groupAppliable) && _changed != nullptr) {
				_group = _shadowGroup;
				_changed({ _shadowGroup });
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
};

EditorPalette* EditorPalette::create(
	Renderer* rnd, class Workspace* ws,
	class Theme* theme,
	int group, ChangedHandler changed,
	const std::string &title,
	class Project* prj,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
	const char* confirmTxt, const char* cancelTxt, const char* applyTxt
) {
	EditorPaletteImpl* result = new EditorPaletteImpl(
		rnd, ws,
		theme,
		group, changed,
		title,
		prj,
		confirm, cancel, apply,
		confirmTxt, cancelTxt, applyTxt
	);

	return result;
}

void EditorPalette::destroy(EditorPalette* ptr) {
	EditorPaletteImpl* impl = static_cast<EditorPaletteImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
