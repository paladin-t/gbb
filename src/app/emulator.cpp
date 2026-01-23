/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor.h"
#include "emulator.h"
#include "theme.h"
#include "vram_debugger.h"
#include "widgets.h"
#include "../../lib/binjgb/src/emulator.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EMULATOR_SGB_PADDING_X
#	define EMULATOR_SGB_PADDING_X SGB_SCREEN_LEFT
#endif /* EMULATOR_SGB_PADDING_X */
#ifndef EMULATOR_SGB_PADDING_Y
#	define EMULATOR_SGB_PADDING_Y SGB_SCREEN_TOP
#endif /* EMULATOR_SGB_PADDING_Y */

#ifndef EMULATOR_VRAM_DEBUGGER_MAX_WIDTH
#	define EMULATOR_VRAM_DEBUGGER_MAX_WIDTH 256.0f
#endif /* EMULATOR_VRAM_DEBUGGER_MAX_WIDTH */
#ifndef EMULATOR_VRAM_DEBUGGER_MIN_WIDTH
#	define EMULATOR_VRAM_DEBUGGER_MIN_WIDTH 160.0f
#endif /* EMULATOR_VRAM_DEBUGGER_MIN_WIDTH */

/* ===========================================================================} */

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
	Texture* canvasTextureForBorderFrame = nullptr;
	std::string cartridgeStatusText;
	std::string deviceStatusText;
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
	VramDebugger* vramDebugger = nullptr;
	bool* vramDebugEnabled = nullptr;
	float* vramDebuggerPreviousOuterWidth = nullptr;
	float* vramDebuggerWidth = nullptr;
	bool* vramDebuggerResizing = nullptr;
	bool* vramDebuggerResetting = nullptr;
	Device::CursorTypes cursor = Device::CursorTypes::POINTER;
	bool hasPopup;
	unsigned deviceFps;
	unsigned fps;
	ButtonEventHandler onDeviceButtonClicked = nullptr;
	ButtonEventHandler onCartridgeButtonClicked = nullptr;

	ImVec2 regSize;         // Content region size of the current ImGui window.
	Math::Vec2i srcSize;    // Source texture size of the video buffer to be rendered.
	ImVec2 dstPos;          // Destination position to render the video texture.
	ImVec2 dstSize;         // Destination size to render the video texture.
	Math::Vec2f scale;      // `dstSize` / `srcSize`.
	Math::Vec2i clientSize; // The size of the client area.

	bool canShowOnscreenGamepad = false;
	bool canShowVramDebugger = false;

	Context(
		Window* window_, Renderer* renderer_,
		Theme* theme_,
		Input* input_,
		const Device::Ptr &canvasDevice_, const Texture::Ptr &canvasTexture_, const Texture::Ptr &canvasTextureForBorderFrame_,
		const std::string &cartridgeStatusText_, const std::string &deviceStatusText_, const std::string &statusTooltip_, float* statusBarWidth_, float statusBarHeight_, bool showStatus_,
		bool* emulatorMuted_, int* emulatorSpeed_, int* emulatorFastForwardSpeed_,
		bool integerScale_, bool fixRatio_,
		bool* onscreenGamepadEnabled_, bool onscreenGamepadSwapAB_, float onscreenGamepadScale_, const Math::Vec2<float> onscreenGamepadPadding_,
		bool* onscreenDebugEnabled_,
		VramDebugger* vramDebugger_, bool* vramDebugEnabled_, float* vramDebuggerPreviousOuterWidth_, float* vramDebuggerWidth_,  bool* vramDebuggerResizing_, bool* vramDebuggerResetting_,
		Device::CursorTypes cursor_,
		bool hasPopup_,
		unsigned deviceFps_, unsigned fps_,
		ButtonEventHandler onDeviceButtonClicked_, ButtonEventHandler onCartridgeButtonClicked_
	) :
		window(window_), renderer(renderer_),
		theme(theme_),
		input(input_),
		canvasDevice(canvasDevice_.get()), canvasTexture(canvasTexture_.get()), canvasTextureForBorderFrame(canvasTextureForBorderFrame_.get()),
		cartridgeStatusText(cartridgeStatusText_), deviceStatusText(deviceStatusText_), statusTooltip(statusTooltip_), statusBarWidth(statusBarWidth_), statusBarHeight(statusBarHeight_), showStatus(showStatus_),
		emulatorMuted(emulatorMuted_), emulatorSpeed(emulatorSpeed_), emulatorPreferedSpeed(emulatorFastForwardSpeed_),
		integerScale(integerScale_), fixRatio(fixRatio_),
		onscreenGamepadEnabled(onscreenGamepadEnabled_), onscreenGamepadSwapAB(onscreenGamepadSwapAB_), onscreenGamepadScale(onscreenGamepadScale_), onscreenGamepadPadding(onscreenGamepadPadding_),
		onscreenDebugEnabled(onscreenDebugEnabled_),
		vramDebugger(vramDebugger_), vramDebugEnabled(vramDebugEnabled_), vramDebuggerPreviousOuterWidth(vramDebuggerPreviousOuterWidth_), vramDebuggerWidth(vramDebuggerWidth_),  vramDebuggerResizing(vramDebuggerResizing_), vramDebuggerResetting(vramDebuggerResetting_),
		cursor(cursor_),
		hasPopup(hasPopup_),
		deviceFps(deviceFps_), fps(fps_),
		onDeviceButtonClicked(onDeviceButtonClicked_), onCartridgeButtonClicked(onCartridgeButtonClicked_)
	{
	}

	// Begin rendering context.
	bool begin(bool showSgbBorder) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.WindowBorderSize;

		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		regSize = ImVec2(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		canShowVramDebugger = regSize.x >= SCREEN_WIDTH + EMULATOR_VRAM_DEBUGGER_MIN_WIDTH + 2;
		const bool vramDbg = canShowVramDebugger && (!!vramDebugger && *vramDebugEnabled);
		if (vramDbg) {
			const ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoNav;

			if (*vramDebuggerWidth <= 0) {
				*vramDebuggerWidth = calculateVramDebuggerWidth(EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2);
			}
			if (*vramDebuggerPreviousOuterWidth <= 0) {
				*vramDebuggerPreviousOuterWidth = regSize.x;
			}
			if (*vramDebuggerPreviousOuterWidth != regSize.x) {
				*vramDebuggerWidth = *vramDebuggerWidth / *vramDebuggerPreviousOuterWidth * regSize.x;
				*vramDebuggerWidth = calculateVramDebuggerWidth(EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2);
				*vramDebuggerPreviousOuterWidth = regSize.x;
			}

			const ImVec2 curPos = ImGui::GetCursorScreenPos();
			const ImVec2 size(
				regSize.x - *vramDebuggerWidth,
				regSize.y - (showStatus ? (statusBarHeight - style.ChildBorderSize) : style.ChildBorderSize)
			);
			ImGui::PushClipRect(curPos, curPos + size, false);

			ImGui::BeginChild("#Cvs", ImVec2(regSize.x - *vramDebuggerWidth, regSize.y), true, flags);
		}

		ImVec2 regSize_ = regSize;
		if (vramDbg) {
			const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
			const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
			regSize_ = ImVec2(
				regMax.x - regMin.x - borderSize * 2,
				regMax.y - regMin.y - borderSize * 2
			);
		}

		// Determine the source texture size.
		clientSize = Math::Vec2i(Math::max(canvasTexture->width(), SCREEN_WIDTH), Math::max(canvasTexture->height(), SCREEN_HEIGHT));
		srcSize = showSgbBorder ?
			Math::Vec2i(Math::max(canvasTexture->width(), SGB_SCREEN_WIDTH), Math::max(canvasTexture->height(), SGB_SCREEN_HEIGHT)) :
			clientSize;

		// Calculate the client area.
		dstPos = ImGui::GetCursorPos();
		dstSize = regSize_;
		dstSize.y -= showStatus ? (statusBarHeight - style.ChildBorderSize) : style.ChildBorderSize;

		if (integerScale) {
			const int xTimes = (int)Math::max(std::floor(dstSize.x / srcSize.x), 1.0f);
			const int yTimes = (int)Math::max(std::floor(dstSize.y / srcSize.y), 1.0f);
			int w = 0;
			int h = 0;
			if (fixRatio) {
				if (xTimes < yTimes) {
					w = xTimes * srcSize.x;
					h = xTimes * srcSize.y;
				} else if (xTimes > yTimes) {
					w = yTimes * srcSize.x;
					h = yTimes * srcSize.y;
				} else {
					w = xTimes * srcSize.x;
					h = yTimes * srcSize.y;
				}
			} else {
				w = xTimes * srcSize.x;
				h = yTimes * srcSize.y;
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

		// Determine the final scale.
		scale = Math::Vec2f(dstSize.x / srcSize.x, dstSize.y / srcSize.y);

		// Adjust the client area if SGB border is present.
		if (showSgbBorder) {
			dstPos.x = (float)(dstPos.x + scale.x * EMULATOR_SGB_PADDING_X);
			dstPos.y = (float)(dstPos.y + scale.y * EMULATOR_SGB_PADDING_Y);
			dstSize.x = (float)(dstSize.x - scale.x * (EMULATOR_SGB_PADDING_X * 2));
			dstSize.y = (float)(dstSize.y - scale.y * (EMULATOR_SGB_PADDING_Y * 2));
		}

		// Finish.
		return vramDbg;
	}
	// End rendering context.
	void end(bool vramDbg) {
		// Render the onscreen gamepad.
		canShowOnscreenGamepad = false;

		int pressed = 0;
		if (*onscreenGamepadEnabled) {
			pressed = input->updateOnscreenGamepad(
				window, renderer,
				theme->fontBlock(),
				onscreenGamepadSwapAB,
				onscreenGamepadScale,
				onscreenGamepadPadding.x, onscreenGamepadPadding.y,
				true,
				&canShowOnscreenGamepad
			);
		}

		// Update the input module.
		const ImVec2 wndPos = ImGui::GetWindowPos(); // The ImGui window position.
		const Math::Rectf clientArea = Math::Rectf::byXYWH( // The destination client area to render the game texture.
			wndPos.x + dstPos.x, wndPos.y + dstPos.y,
			std::ceil(dstSize.x), std::ceil(dstSize.y)
		);
		const Math::Vec2i &canvasSize = clientSize;
		const int scale_ = renderer->scale() / window->scale();

		if (!hasPopup) {
			input->update(window, renderer, &clientArea, &canvasSize, !pressed, scale_);

			input->sync();
		}

		if (vramDbg) {
			ImGui::EndChild();

			ImGui::PopClipRect();
		}
	}

	void debugVram(bool vramDbg) {
		// Prepare.
		if (!vramDbg)
			return;

		const bool wasResizing = *vramDebuggerResizing;
		bool isResizing = *vramDebuggerResizing;
		bool isResetting = *vramDebuggerResetting;

		ImGui::SameLine();

		ImGuiStyle &style = ImGui::GetStyle();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		if (wasResizing) {
			const ImVec4 &col = ImGui::GetStyleColorVec4(ImGuiCol_ResizeGripActive);
			ImGui::PushStyleColor(ImGuiCol_Border, col);
		}
		const float borderSize = style.WindowBorderSize;

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_AlwaysVerticalScrollbar |
			ImGuiWindowFlags_NoNav;
		const float x = (float)ImGui::GetCursorPosX();
		const float width = *vramDebuggerWidth;
		const float height = regSize.y - statusBarHeight - borderSize * 2;

		// Resize.
		const float gripMarginX = ImGui::WindowResizingPadding().x;
		const float gripPaddingY = 4.0f;
		if (isResizing && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isResizing = false;
		}
		if (isResetting && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isResetting = false;
		}
		const bool isHoveringRect = ImGui::IsMouseHoveringRect(
			ImVec2(x, gripPaddingY),
			ImVec2(x + gripMarginX, height - gripPaddingY - style.ScrollbarSize),
			false
		);
		if (isHoveringRect && !hasPopup) {
			isResizing = ImGui::IsMouseDown(ImGuiMouseButton_Left);

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				isResetting = true;

			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			isResizing = false;
		}
		if (isResizing && !isResetting) {
			flags &= ~ImGuiWindowFlags_NoResize;
		}

		if (isResetting) {
			*vramDebuggerWidth = 0;
		} else if (isResizing) {
			const ImVec2 mousePos = ImGui::GetMousePos();
			*vramDebuggerWidth = calculateVramDebuggerWidth(regSize.x - mousePos.x);
		}

		// Draw the VRAM debugger.
		ImGui::BeginChild("#VDbg", ImVec2(width, height), true, flags);
		{
			vramDebugger->update(renderer, theme, canvasDevice);
		}
		ImGui::EndChild();

		// Finish.
		if (wasResizing) {
			ImGui::PopStyleColor();
		}
		ImGui::PopStyleVar();

		*vramDebuggerResizing = isResizing;
		*vramDebuggerResetting = isResetting;
	}

private:
	float calculateVramDebuggerWidth(float width) const {
		ImGuiStyle &style = ImGui::GetStyle();

		width = std::min(width, std::min(regSize.x - SCREEN_WIDTH, EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2));
		width = std::max(width, EMULATOR_VRAM_DEBUGGER_MIN_WIDTH);

		return std::floor(width);
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
	do {
		VariableGuard<decltype(style.FramePadding.x)> guardFramePaddingX(&style.FramePadding.x, style.FramePadding.x, 0);

		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		if (context.onDeviceButtonClicked) {
			if (ImGui::Button(context.deviceStatusText))
				context.onDeviceButtonClicked();
		} else {
			ImGui::TextUnformatted(context.deviceStatusText);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(context.statusTooltip);
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("/");
		ImGui::SameLine();
		if (context.onCartridgeButtonClicked) {
			if (ImGui::Button(context.cartridgeStatusText))
				context.onCartridgeButtonClicked();
		} else {
			ImGui::TextUnformatted(context.cartridgeStatusText);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(context.statusTooltip);
		}
		ImGui::SameLine();
		if (ImGui::GetWindowWidth() >= 370) {
			ImGui::Text(" %uFPS", context.deviceFps);
		}
	} while (false);
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
				WIDGETS_SELECTION_GUARD(context.theme);

				if (ImGui::ImageButton(context.theme->iconNormalSpeed()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, context.theme->tooltipEmulator_NormalSpeed().c_str())) {
					*context.emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 1;
					context.canvasDevice->speed(DEVICE_BASE_SPEED_FACTOR * 1);
				}
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
		if (context.canShowOnscreenGamepad) {
			if (g)
				*context.onscreenGamepadEnabled = !*context.onscreenGamepadEnabled;
		}
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
		if (context.canShowOnscreenGamepad) {
			ImGui::MenuItem(context.theme->menu_OnscreenGamepad(), GBBASIC_MODIFIER_KEY_NAME "+G", context.onscreenGamepadEnabled);
		} else {
			ImGui::BeginDisabled();
			ImGui::MenuItem(context.theme->menu_OnscreenGamepad(), GBBASIC_MODIFIER_KEY_NAME "+G", context.onscreenGamepadEnabled);
			ImGui::EndDisabled();
		}

		ImGui::MenuItem(context.theme->menu_OnscreenDebug(), nullptr, context.onscreenDebugEnabled);

		if (!!context.vramDebugger) {
			ImGui::Separator();

			if (context.canShowVramDebugger) {
				ImGui::MenuItem(context.theme->menu_VramDebugger(), nullptr, context.vramDebugEnabled);
			} else {
				ImGui::BeginDisabled();
				ImGui::MenuItem(context.theme->menu_VramDebugger(), nullptr, context.vramDebugEnabled);
				ImGui::EndDisabled();
			}
		}

		ImGui::EndPopup();
	}
}

void emulator(
	class Window* wnd, class Renderer* rnd,
	class Theme* theme,
	Input* input,
	const Device::Ptr &canvasDevice, const Texture::Ptr &canvasTexture, const Texture::Ptr &canvasTextureForBorderFrame,
	const std::string &cartridgeStatusText, const std::string &deviceStatusText, const std::string &statusTooltip, float &statusBarWidth, float statusBarHeight, bool showStatus,
	bool &emulatorMuted, int &emulatorSpeed, int &emulatorPreferedSpeed,
	bool integerScale, bool fixRatio,
	bool &onscreenGamepadEnabled, bool onscreenGamepadSwapAB, float onscreenGamepadScale, const Math::Vec2<float> &onscreenGamepadPadding,
	bool &onscreenDebugEnabled,
	class VramDebugger* vramDebugger, bool &vramDebugEnabled, float &vramDebuggerPreviousOuterWidth, float &vramDebuggerWidth, bool &vramDebuggerResizing, bool &vramDebuggerResetting,
	Device::CursorTypes cursor,
	bool hasPopup,
	unsigned fps,
	ButtonEventHandler onDeviceButtonClicked, ButtonEventHandler onCartridgeButtonClicked,
	DebugHandler debug
) {
	// Prepare.
	Context context(
		wnd, rnd,
		theme,
		input,
		canvasDevice, canvasTexture, canvasTextureForBorderFrame,
		cartridgeStatusText, deviceStatusText, statusTooltip, &statusBarWidth, statusBarHeight, showStatus,
		&emulatorMuted, &emulatorSpeed, &emulatorPreferedSpeed,
		integerScale, fixRatio,
		&onscreenGamepadEnabled, onscreenGamepadSwapAB, onscreenGamepadScale, onscreenGamepadPadding,
		&onscreenDebugEnabled,
		vramDebugger, &vramDebugEnabled, &vramDebuggerPreviousOuterWidth, &vramDebuggerWidth, &vramDebuggerResizing, &vramDebuggerResetting,
		cursor,
		hasPopup,
		canvasDevice->fps(), fps,
		onDeviceButtonClicked, onCartridgeButtonClicked
	);

	GBBASIC_ASSERT(!!canvasDevice && "Impossible.");

	if (!canvasTexture) // Not ready.
		return;

	// Render emulation.
	const bool vramDbg = context.begin(!!canvasTextureForBorderFrame);
	{
		// Render the canvas.
		if (canvasTextureForBorderFrame) {
			const Math::Vec2f &scale = context.scale;
			ImVec2 pos = context.dstPos - ImVec2((float)(EMULATOR_SGB_PADDING_X * scale.x), (float)(EMULATOR_SGB_PADDING_Y * scale.y));
			pos.x = std::floor(pos.x);
			pos.y = std::floor(pos.y);
			const ImVec2 size = context.dstSize + ImVec2((float)(EMULATOR_SGB_PADDING_X * scale.x * 2), (float)(EMULATOR_SGB_PADDING_Y * scale.y * 2));
			ImGui::SetCursorPos(pos);
			ImGui::Image(
				canvasTextureForBorderFrame->pointer(rnd),
				size,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);
		}
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
	}
	context.end(vramDbg);

	// Render VRAM debugger.
	context.debugVram(vramDbg);

	// Render the status.
	renderStatus(context);

	// Check the shortcuts.
	shortcuts(context);

	// Show the context menu.
	menu(context);
}

/* ===========================================================================} */
