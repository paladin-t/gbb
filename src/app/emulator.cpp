/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor.h"
#include "emulator.h"
#include "theme.h"
#include "widgets.h"
#include "../../lib/binjgb/src/emulator.h"
#include <SDL.h>

/*
** {===========================================================================
** Emulator
*/

struct Context {
	Window* window = nullptr;
	Renderer* renderer = nullptr;
	Theme* theme = nullptr;
	Input* input = nullptr;
	Device* canvasDevice = nullptr;
	Texture* canvasTexture = nullptr;
	std::string statusText;
	std::string statusTooltip;
	float* statusBarWidth = nullptr;
	float statusBarHeight;
	bool showStatus;
	bool* emulatorMuted = nullptr;
	int* emulatorSpeed = nullptr;
	int* emulatorPreferedSpeed = nullptr;
	bool integerScale;
	bool fixRatio;
	bool* onscreenGamepadEnabled = nullptr;
	bool onscreenGamepadSwapAB;
	float onscreenGamepadScale;
	Math::Vec2<float> onscreenGamepadPadding;
	bool* onscreenDebugEnabled = nullptr;
	Device::CursorTypes cursor = Device::CursorTypes::POINTER;
	bool hasPopup;
	unsigned deviceFps;
	unsigned fps;

	ImVec2 regSize;
	Math::Vec2i srcSize;
	ImVec2 dstPos;
	ImVec2 dstSize;

	Context(
		Window* window_, Renderer* renderer_,
		Theme* theme_,
		Input* input_,
		const Device::Ptr &canvasDevice_, const Texture::Ptr &canvasTexture_,
		const std::string &statusText_, const std::string &statusTooltip_, float* statusBarWidth_, float statusBarHeight_, bool showStatus_,
		bool* emulatorMuted_, int* emulatorSpeed_, int* emulatorFastForwardSpeed_,
		bool integerScale_, bool fixRatio_,
		bool* onscreenGamepadEnabled_, bool onscreenGamepadSwapAB_, float onscreenGamepadScale_, const Math::Vec2<float> onscreenGamepadPadding_,
		bool* onscreenDebugEnabled_,
		Device::CursorTypes cursor_,
		bool hasPopup_,
		unsigned deviceFps_, unsigned fps_
	) :
		window(window_), renderer(renderer_),
		theme(theme_),
		input(input_),
		canvasDevice(canvasDevice_.get()), canvasTexture(canvasTexture_.get()),
		statusText(statusText_), statusTooltip(statusTooltip_), statusBarWidth(statusBarWidth_), statusBarHeight(statusBarHeight_), showStatus(showStatus_),
		emulatorMuted(emulatorMuted_), emulatorSpeed(emulatorSpeed_), emulatorPreferedSpeed(emulatorFastForwardSpeed_),
		integerScale(integerScale_), fixRatio(fixRatio_),
		onscreenGamepadEnabled(onscreenGamepadEnabled_), onscreenGamepadSwapAB(onscreenGamepadSwapAB_), onscreenGamepadScale(onscreenGamepadScale_), onscreenGamepadPadding(onscreenGamepadPadding_),
		onscreenDebugEnabled(onscreenDebugEnabled_),
		cursor(cursor_),
		hasPopup(hasPopup_),
		deviceFps(deviceFps_), fps(fps_)
	{
	}

	void begin(void) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.WindowBorderSize;

		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		regSize = ImVec2(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		// Calculate the widget area.
		const int texWidth = Math::max(canvasTexture->width(), SCREEN_WIDTH);
		const int texHeight = Math::max(canvasTexture->height(), SCREEN_HEIGHT);
		srcSize = Math::Vec2i(texWidth, texHeight);
		dstPos = ImGui::GetCursorPos();
		dstSize = regSize;
		if (showStatus)
			dstSize.y -= statusBarHeight - style.ChildBorderSize;
		else
			dstSize.y -= style.ChildBorderSize;
		if (integerScale) {
			int w = (int)Math::max(std::floor(dstSize.x / texWidth), 1.0f);
			int h = (int)Math::max(std::floor(dstSize.y / texHeight), 1.0f);
			if (fixRatio) {
				if (w < h)
					h = w;
				else if (w > h)
					w = h;
				w *= texWidth;
				h *= texHeight;
			} else {
				w *= texWidth;
				h *= texHeight;
			}
			dstPos.x += (dstSize.x - w) * 0.5f;
			dstPos.y += (dstSize.y - h) * 0.5f;
			dstSize.x = (float)w;
			dstSize.y = (float)h;
		} else {
			if (fixRatio) {
				const float srcRatio = (float)srcSize.x / (float)srcSize.y;
				const float dstRatio = dstSize.x / dstSize.y;
				if (srcRatio < dstRatio) {
					const float w = dstSize.x;
					dstSize.x = dstSize.y * srcRatio;
					dstPos.x += (w - dstSize.x) * 0.5f;
				} else if (srcRatio > dstRatio) {
					const float h = dstSize.y;
					dstSize.y = dstSize.x / srcRatio;
					dstPos.y += (h - dstSize.y) * 0.5f;
				} else {
					// Do nothing.
				}
			} else {
				// Do nothing.
			}
		}
	}
	void end(void) const {
		// Render the onscreen gamepad.
		int pressed = 0;
		if (*onscreenGamepadEnabled) {
			pressed = input->updateOnscreenGamepad(
				window, renderer,
				theme->fontBlock(),
				onscreenGamepadSwapAB,
				onscreenGamepadScale,
				onscreenGamepadPadding.x, onscreenGamepadPadding.y,
				true
			);
		}

		// Update the input module.
		const ImVec2 wndPos = ImGui::GetWindowPos();
		const Math::Rectf clientArea = Math::Rectf::byXYWH(
			wndPos.x + dstPos.x, wndPos.y + dstPos.y,
			std::ceil(dstSize.x), std::ceil(dstSize.y)
		);
		const Math::Vec2i canvasSize = srcSize;
		const int scale = renderer->scale() / window->scale();

		if (!hasPopup) {
			input->update(window, renderer, &clientArea, &canvasSize, !pressed, scale);

			input->sync();
		}
	}
};

static void renderStatus(const Context &context) {
	ImGuiStyle &style = ImGui::GetStyle();

	if (!context.showStatus)
		return;

	const ImVec2 pos = ImGui::GetCursorPos();
	const ImVec2 pos_ = ImVec2(pos.x, context.regSize.y - context.statusBarHeight + style.ChildBorderSize * 2);
	const bool actived = ImGui::IsWindowFocused();
	if (actived || EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
		ImGui::SetCursorPos(pos_);
		ImGui::Dummy(
			ImVec2(context.regSize.x + style.ChildBorderSize, context.statusBarHeight - style.ChildBorderSize),
			ImGui::GetStyleColorVec4(ImGuiCol_Button)
		);
	}
	ImGui::SetCursorPos(pos_);

	if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	}
	ImGui::Dummy(ImVec2(8, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	if (ImGui::GetWindowWidth() >= 370) {
		ImGui::Text("%s %uFPS", context.statusText.c_str(), context.deviceFps);
	} else {
		ImGui::Text("%s", context.statusText.c_str());
	}
	if (ImGui::IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(context.statusTooltip);
	}
	if (ImGui::GetWindowWidth() >= 430) {
		if (context.canvasDevice->canGetDuty()) {
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::PushID("@Dty");
			{
				int lduty = 0;
				const int duty = context.canvasDevice->getDuty(&lduty);
				float duty_ = (float)duty;
				float lduty_ = (float)lduty;
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(4, 0));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(60.0f);
				ImGui::ProgressBar("", &duty_, &lduty_, 0.0f, 100.0f, "%g%%", true);
			}
			ImGui::PopID();
			ImGui::PopStyleColor(5);
		}
	}
	do {
		ImGui::SameLine();
		float width_ = 0.0f;
		const float wndWidth = ImGui::GetWindowWidth();
		ImGui::SetCursorPosX(wndWidth - *context.statusBarWidth);
		if (context.canvasDevice->isVariableSpeedSupported()) {
			if (*context.emulatorSpeed == DEVICE_BASE_SPEED_FACTOR * 1) {
				if (ImGui::ImageButton(context.theme->iconFastForward()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, context.theme->tooltipEmulator_AlternativeSpeed().c_str())) {
					ImGui::OpenPopup("@Spd");
				}
			} else {
				do {
					WIDGETS_SELECTION_GUARD(context.theme);
					if (ImGui::ImageButton(context.theme->iconNormalSpeed()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, context.theme->tooltipEmulator_NormalSpeed().c_str())) {
						*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 1;
						context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 1);
					}
				} while (false);
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
		}
		if (*context.emulatorMuted) {
			if (ImGui::ImageButton(context.theme->iconMuted()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
				*context.emulatorMuted = false;
			}
		} else {
			if (ImGui::ImageButton(context.theme->iconLoud()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
				*context.emulatorMuted = true;
			}
		}
		width_ += ImGui::GetItemRectSize().x;
		ImGui::SameLine();
		if (ImGui::ImageButton(context.theme->iconViews()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, context.theme->tooltip_View().c_str())) {
			ImGui::OpenPopup("@Views");
		}
		width_ += ImGui::GetItemRectSize().x;
		width_ += style.FramePadding.x;
		*context.statusBarWidth = width_;
	} while (false);
	if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
		ImGui::PopStyleColor(3);
	}

	ImGui::SetCursorPos(pos);

	do {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Spd")) {
			if (ImGui::MenuItem(context.theme->menu_X16())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR * 16;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 16;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 16);
			}
			if (ImGui::MenuItem(context.theme->menu_X8())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR * 8;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 8;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 8);
			}
			if (ImGui::MenuItem(context.theme->menu_X4())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR * 4;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 4;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 4);
			}
			if (ImGui::MenuItem(context.theme->menu_X2())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR * 2;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 2;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 2);
			}
			if (ImGui::MenuItem(context.theme->menu_X1(), nullptr, true)) {
				// Do nothing.
			}
			if (ImGui::MenuItem(context.theme->menu_X0_5())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR / 2;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR / 2;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR / 2);
			}
			if (ImGui::MenuItem(context.theme->menu_X0_2())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR / 5;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR / 5;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR / 5);
			}
			if (ImGui::MenuItem(context.theme->menu_X0_1())) {
				*context.emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR / 10;
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR / 10;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR / 10);
			}

			ImGui::EndPopup();
		}
	} while (false);
}

static void shortcuts(const Context &context) {
	// Prepare.
	if (context.hasPopup)
		return;

	if (!context.showStatus)
		return;

	ImGuiIO &io = ImGui::GetIO();

	// Get key states.
#if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
	const bool modifier = io.KeyCtrl;
#elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
	const bool modifier = io.KeySuper;
#endif /* GBBASIC_MODIFIER_KEY */

	const bool g     = ImGui::IsKeyPressed(SDL_SCANCODE_G);
	const bool slash = ImGui::IsKeyPressed(SDL_SCANCODE_SLASH);

	// Overlay operations.
	if (modifier && !io.KeyShift && !io.KeyAlt) {
		if (g)
			*context.onscreenGamepadEnabled = !*context.onscreenGamepadEnabled;
	}
	if (modifier && !io.KeyShift && !io.KeyAlt) {
		if (slash) {
			if (*context.emulatorSpeed == DEVICE_BASE_SPEED_FACTOR * 1) {
				*context.emulatorSpeed = Math::clamp(*context.emulatorPreferedSpeed, DEVICE_BASE_SPEED_FACTOR / 10, DEVICE_BASE_SPEED_FACTOR * 16);
				context.canvasDevice->speed(Math::clamp(*context.emulatorPreferedSpeed, DEVICE_BASE_SPEED_FACTOR / 10, DEVICE_BASE_SPEED_FACTOR * 16));
			} else {
				*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 1;
				context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 1);
			}
		}
	}
}

static void menu(const Context &context) {
	// Prepare.
	ImGuiStyle &style = ImGui::GetStyle();

	// Show the menu.
	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

	if (ImGui::BeginPopup("@Views")) {
		ImGui::MenuItem(context.theme->menu_OnscreenGamepad(), GBBASIC_MODIFIER_KEY_NAME "+G", context.onscreenGamepadEnabled);

		ImGui::MenuItem(context.theme->menu_OnscreenDebug(), nullptr, context.onscreenDebugEnabled);

		ImGui::EndPopup();
	}
}

void emulator(
	class Window* wnd, class Renderer* rnd,
	class Theme* theme,
	Input* input,
	const Device::Ptr &canvasDevice, const Texture::Ptr &canvasTexture,
	const std::string &statusText, const std::string &statusTooltip, float &statusBarWidth, float statusBarHeight, bool showStatus,
	bool &emulatorMuted, int &emulatorSpeed, int &emulatorPreferedSpeed,
	bool integerScale, bool fixRatio,
	bool &onscreenGamepadEnabled, bool onscreenGamepadSwapAB, float onscreenGamepadScale, const Math::Vec2<float> &onscreenGamepadPadding,
	bool &onscreenDebugEnabled,
	Device::CursorTypes cursor,
	bool hasPopup,
	unsigned fps,
	DebugHandler debug
) {
	// Prepare.
	Context context(
		wnd, rnd,
		theme,
		input,
		canvasDevice, canvasTexture,
		statusText, statusTooltip, &statusBarWidth, statusBarHeight, showStatus,
		&emulatorMuted, &emulatorSpeed, &emulatorPreferedSpeed,
		integerScale, fixRatio,
		&onscreenGamepadEnabled, onscreenGamepadSwapAB, onscreenGamepadScale, onscreenGamepadPadding,
		&onscreenDebugEnabled,
		cursor,
		hasPopup,
		canvasDevice->fps(), fps
	);

	GBBASIC_ASSERT(!!canvasDevice && "Impossible.");

	if (!canvasTexture) // Not ready.
		return;

	// Render emulation.
	context.begin();
	{
		// Render the canvas.
		ImGui::SetCursorPos(context.dstPos);
		ImGui::Image(
			canvasTexture->pointer(rnd),
			context.dstSize,
			ImVec2(0, 0), ImVec2(1, 1),
			ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
		);

		// Update the cursor.
		if (ImGui::IsItemHovered() && !hasPopup) {
			switch (cursor) {
			case Device::CursorTypes::NONE:
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				break;
			case Device::CursorTypes::POINTER:
				ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

				break;
			case Device::CursorTypes::HAND:
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				break;
			case Device::CursorTypes::BUSY:
				ImGui::SetMouseCursor(ImGuiMouseCursor_Wait);

				break;
			}
		}

		// Render the debug information.
		if (*context.onscreenDebugEnabled) {
			if (debug)
				debug();
		}

		// Render the status.
		renderStatus(context);
	}
	context.end();

	// Check the shortcuts.
	shortcuts(context);

	// Show the context menu.
	menu(context);
}

/* ===========================================================================} */
