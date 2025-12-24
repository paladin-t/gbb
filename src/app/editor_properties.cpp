/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editing.h"
#include "editor_properties.h"
#include "project.h"
#include "theme.h"
#include <SDL.h>

/*
** {===========================================================================
** Properties editor
*/

class EditorPropertiesImpl : public EditorProperties {
private:
	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	std::string _title;
	Project* _project = nullptr; // Foreign.
	ImageArray _shadowImageArray;
	Map::Ptr _shadowMap = nullptr;
	Editing::Tiler _cursor;
	Actor::Ptr _object = nullptr; // Foreign.
	int _layer = -1;
	const Text::Array &_frameNames;
	Actor::Shadow::Array _frameShadow;
	ImageArray _propertiesArray; // Foreign.
	bool _applyToAllTiles = false;
	bool _applyToAllFrames = true;
	ChangedHandler _changed = nullptr; // Foreign.

	ConfirmedHandler _confirmedHandler = nullptr;
	std::string _confirmText;
	CanceledHandler _canceledHandler = nullptr;
	std::string _cancelText;
	AppliedHandler _appliedHandler = nullptr;
	std::string _applyText;

	ImGui::Initializer _init;

public:
	EditorPropertiesImpl(
		Renderer* rnd, Workspace* ws,
		Theme* theme,
		Actor::Ptr obj, int layer,
		const Actor::Shadow::Array &shadow,
		const Text::Array &frameNames,
		ChangedHandler changed,
		const std::string &title,
		Project* prj,
		const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
		const char* confirmTxt, const char* cancelTxt, const char* applyTxt
	) : _renderer(rnd),
		_workspace(ws),
		_theme(theme),
		_object(obj), _layer(layer),
		_frameShadow(shadow),
		_frameNames(frameNames),
		_changed(changed),
		_title(title),
		_project(prj),
		_confirmedHandler(confirm), _canceledHandler(cancel), _appliedHandler(apply)
	{
		for (int i = 0; i < _object->count(); ++i) {
			const Actor::Frame* frame = _object->get(i);
			const Image::Ptr &props = frame->properties;
			_propertiesArray.push_back(props);

			Image::Ptr img(Image::create(Indexed::Ptr(Indexed::create(256))));
			img->fromImage(props.get());
			_shadowImageArray.push_back(img);
		}
		changeFrame();

		_applyToAllTiles = _project->preferencesActorApplyPropertiesToAllTiles();
		_applyToAllFrames = _project->preferencesActorApplyPropertiesToAllFrames();

		_cursor.position = Math::Vec2i(0, 0);

		if (confirmTxt)
			_confirmText = confirmTxt;
		if (cancelTxt)
			_cancelText = cancelTxt;
		if (applyTxt)
			_applyText = applyTxt;
	}
	virtual ~EditorPropertiesImpl() override {
	}

	virtual void update(Workspace*) override {
		typedef std::function<void(int, int, Byte)> CelSetter;

		ImGuiStyle &style = ImGui::GetStyle();

		bool isOpen = true;
		bool toConfirm = false;
		bool toApply = false;
		bool toCancel = false;

		if (_init.begin())
			ImGui::OpenPopup(_title);

		bool propertiesAppliable = false;
		if (ImGui::BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
			{
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

				constexpr const float fwidth = 146;
				constexpr const float swidth = 120;
				constexpr const float height = 130;

				const float xPos = ImGui::GetCursorPosX();
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(_theme->dialogPrompt_Frame());
				ImGui::SameLine();
				{
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth((fwidth + swidth) - (ImGui::GetCursorPosX() - xPos));
					int index = _layer;
					const bool changed = ImGui::Combo(
						"",
						&index,
						[] (void* data, int idx, const char** outText) -> bool {
							const std::string* items = (const std::string*)data;
							*outText = items[idx].c_str();

							return true;
						},
						(void*)&_frameNames.front(), (int)_frameNames.size()
					);
					if (changed) {
						_layer = index;

						changeFrame();
					}
				}

				Editing::Tiler cursor = _cursor;
				ImGui::BeginChild("@Pat", ImVec2(fwidth, height), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
				{
					const Actor::Frame* frame = _object->get(_layer);
					const Math::Vec2i gsize(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
					const int texw = frame->pixels->width();
					const int scale = (texw / GBBASIC_TILE_SIZE) <= 4 ?
						Math::clamp((int)std::floor((fwidth - 3) / texw), 1, 16) :
						Math::clamp((int)std::floor(fwidth / texw), 1, 16);
					const float wwidth = (float)(texw * scale);
					const ImVec2 pos = ImGui::GetCursorPos();
					Editing::tiles(
						_renderer,
						_workspace,
						frame->pixels.get(), frame->texture.get(),
						wwidth,
						nullptr, false,
						nullptr,
						nullptr,
						&gsize, false,
						true,
						ImGuiMouseButton_Left,
						[&] (void) -> void {
							ImGui::SetCursorPos(pos);
							const bool clicked = Editing::map(
								_renderer, _workspace,
								_project,
								_shadowMap.get(), gsize,
								false,
								wwidth,
								&cursor, true,
								nullptr,
								nullptr,
								false,
								std::numeric_limits<int>::min(),
								nullptr,
								nullptr,
								nullptr,
								ImGuiMouseButton_Left
							);
							if (clicked)
								_cursor = cursor;
						}
					);
				}
				ImGui::EndChild();
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0));
				ImGui::BeginChild("@Tls", ImVec2(swidth, height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
				{
					const char* tooltipBits[8] = {
						_theme->tooltipActor_Bit0().c_str(),
						_theme->tooltipActor_Bit1().c_str(),
						_theme->tooltipActor_Bit2().c_str(),
						_theme->tooltipActor_Bit3().c_str(),
						_theme->tooltipActor_Bit4().c_str(),
						_theme->tooltipActor_Bit5().c_str(),
						_theme->tooltipActor_Bit6().c_str(),
						_theme->tooltipActor_Bit7().c_str()
					};
					Byte cel = (Byte)_shadowMap->get(cursor.position.x, cursor.position.y);
					bool setBit = false;
					int bit = -1;
					const std::string pos = _theme->status_Pos() + " " + Text::toString(cursor.position.x) + "," + Text::toString(_cursor.position.y);
					const bool changed = Editing::Tools::maskable(
						_renderer,
						_workspace,
						&cel, &setBit, &bit,
						0b10011111,
						swidth,
						false,
						pos.c_str(),
						_theme->tooltip_HighBits().c_str(), _theme->tooltip_LowBits().c_str(),
						tooltipBits
					);
					if (changed && cursor == _cursor) {
						CelSetter set = nullptr;
						if (_applyToAllFrames) {
							set = [this, setBit, bit] (int x, int y, Byte /* cel */) -> void {
								for (int i = 0; i < (int)_shadowImageArray.size(); ++i) {
									int celi = 0;
									_shadowImageArray[i]->get(x, y, celi);
									if (setBit) celi |= 1 << bit;
									else celi &= ~(1 << bit);
									_shadowImageArray[i]->set(x, y, celi);
								}

								int celm = _shadowMap->get(x, y);
								if (setBit) celm |= 1 << bit;
								else celm &= ~(1 << bit);
								_shadowMap->set(x, y, celm, false);
							};
						} else {
							set = [this, setBit, bit] (int x, int y, Byte /* cel */) -> void {
								int celi = 0;
								_shadowImageArray[_layer]->get(x, y, celi);
								if (setBit) celi |= 1 << bit;
								else celi &= ~(1 << bit);
								_shadowImageArray[_layer]->set(x, y, celi);

								int celm = _shadowMap->get(x, y);
								if (setBit) celm |= 1 << bit;
								else celm &= ~(1 << bit);
								_shadowMap->set(x, y, celm, false);
							};
						}

						if (_applyToAllTiles) {
							for (int j = 0; j < _shadowMap->height(); ++j) {
								for (int i = 0; i < _shadowMap->width(); ++i) {
									set(i, j, cel);
								}
							}
						} else {
							set(cursor.position.x, cursor.position.y, cel);
						}
					}

					ImGui::NewLine(2);
					if (ImGui::Checkbox(_theme->tooltipActor_ToAllTiles(), &_applyToAllTiles)) {
						_project->preferencesActorApplyPropertiesToAllTiles(_applyToAllTiles);
						_project->hasDirtyInformation(true);
					}
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(_theme->tooltipActor_CheckToChangeBitToAllTiles());
					}
					ImGui::NewLine(2);
					if (ImGui::Checkbox(_theme->tooltipActor_ToAllFrames(), &_applyToAllFrames)) {
						_project->preferencesActorApplyPropertiesToAllFrames(_applyToAllFrames);
						_project->hasDirtyInformation(true);
					}
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(_theme->tooltipActor_CheckToApplyToAllFrames());
					}
				}
				ImGui::EndChild();
				ImGui::PopStyleColor();
			}
			ImGui::NewLine(1);

			const char* confirm = _confirmText.c_str();
			const char* apply = _applyText.empty() ? "Apply" : _applyText.c_str();
			const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

			propertiesAppliable = false;
			GBBASIC_ASSERT(_propertiesArray.size() == _shadowImageArray.size() && "Impossible.");
			for (int i = 0; i < (int)_propertiesArray.size(); ++i) {
				const Image::Ptr &props = _propertiesArray[i];
				const Image::Ptr &shadow = _shadowImageArray[i];
				if (props->compare(shadow.get()) != 0) {
					propertiesAppliable = true;

					break;
				}
			}

			if (_confirmText.empty()) {
				confirm = "Ok";
			}

			ImGui::CentralizeButton(3);

			if (ImGui::Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_RETURN)) {
				toConfirm = true;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || ImGui::IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
				toCancel = true;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (propertiesAppliable) {
				if (ImGui::Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
					toApply = true;
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
				ImGui::EndDisabled();
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

			if (propertiesAppliable && _changed != nullptr) {
				for (int i = 0; i < (int)_propertiesArray.size(); ++i) {
					const Image::Ptr &props = _propertiesArray[i];
					const Image::Ptr &shadow = _shadowImageArray[i];
					if (props->compare(shadow.get()) != 0) {
						_changed({ Object::Ptr(shadow), i, _applyToAllTiles, _applyToAllFrames, &_shadowImageArray });

						break;
					}
				}
			}
			if (!_confirmedHandler.empty()) {
				_confirmedHandler(_shadowImageArray);

				return;
			}
		}
		if (toApply) {
			if (!_appliedHandler.empty()) {
				_appliedHandler(_shadowImageArray);
			}
			if (propertiesAppliable && _changed != nullptr) {
				for (int i = 0; i < (int)_propertiesArray.size(); ++i) {
					const Image::Ptr &props = _propertiesArray[i];
					const Image::Ptr &shadow = _shadowImageArray[i];
					if (props->compare(shadow.get()) != 0) {
						_changed({ Object::Ptr(shadow), i, _applyToAllTiles, _applyToAllFrames, &_shadowImageArray });

						break;
					}
				}
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
	void changeFrame(void) {
		Texture::Ptr attribtex(_theme->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Math::Vec2i attribcount(attribtex->width() / GBBASIC_TILE_SIZE, attribtex->height() / GBBASIC_TILE_SIZE);
		Map::Tiles tiles(attribtex, attribcount);
		_shadowMap = Map::Ptr(Map::create(&tiles, true));
		_shadowMap->resize(_shadowImageArray[_layer]->width(), _shadowImageArray[_layer]->height());
		const Actor::Shadow &shadow_ = _frameShadow[_layer];
		for (int j = 0; j < _shadowImageArray[_layer]->height(); ++j) {
			int y = j * GBBASIC_TILE_SIZE;
			if (j % 2 != 0)
				y = (j - 1) * GBBASIC_TILE_SIZE;
			for (int i = 0; i < _shadowImageArray[_layer]->width(); ++i) {
				int val = 0;
				_shadowImageArray[_layer]->get(i, j, val);
				const int x = i * GBBASIC_TILE_SIZE;
				for (const Actor::Shadow::Ref &r : shadow_.refs) {
					if (r.offset == Math::Vec2i(x, y)) {
						val |= r.properties & 0b01100000;

						break;
					}
				}
				_shadowMap->set(i, j, val);
			}
		}
	}
};

EditorProperties* EditorProperties::create(
	Renderer* rnd, class Workspace* ws,
	class Theme* theme,
	Actor::Ptr obj, int layer,
	const Actor::Shadow::Array &shadow,
	const Text::Array &frameNames,
	ChangedHandler changed,
	const std::string &title,
	class Project* prj,
	const ConfirmedHandler &confirm, const CanceledHandler &cancel, const AppliedHandler &apply,
	const char* confirmTxt, const char* cancelTxt, const char* applyTxt
) {
	EditorPropertiesImpl* result = new EditorPropertiesImpl(
		rnd, ws,
		theme,
		obj, layer,
		shadow,
		frameNames,
		changed,
		title,
		prj,
		confirm, cancel, apply,
		confirmTxt, cancelTxt, applyTxt
	);

	return result;
}

void EditorProperties::destroy(EditorProperties* ptr) {
	EditorPropertiesImpl* impl = static_cast<EditorPropertiesImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
