/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "debugger.h"
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

#ifndef EMULATOR_CODE_DEBUGGER_MAX_WIDTH
#	define EMULATOR_CODE_DEBUGGER_MAX_WIDTH 480.0f
#endif /* EMULATOR_CODE_DEBUGGER_MAX_WIDTH */
#ifndef EMULATOR_CODE_DEBUGGER_MIN_WIDTH
#	define EMULATOR_CODE_DEBUGGER_MIN_WIDTH 160.0f
#endif /* EMULATOR_CODE_DEBUGGER_MIN_WIDTH */

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
	bool (&emulatorChannelMuted)[DEVICE_AUDIO_CHANNEL_COUNT];
	int* emulatorSpeed = nullptr;
	int* emulatorPreferedSpeed = nullptr;
	bool integerScale;
	bool fixRatio;
	bool* onscreenGamepadEnabled = nullptr;
	bool onscreenGamepadSwapAB;
	float onscreenGamepadScale;
	Math::Vec2<float> onscreenGamepadPadding;
	bool* onscreenDebugEnabled = nullptr;
	Debugger* debugger = nullptr;
	bool* codeDebugEnabled = nullptr;
	bool* codeDebuggerShowObjectBounds = nullptr;
	bool* bringCodeDebuggerToFront = nullptr;
	VramDebugger* vramDebugger = nullptr;
	bool* vramDebugEnabled = nullptr;
	bool* vramDebuggerPreviewPaletteBits = nullptr;
	bool* vramDebuggerShowGrids = nullptr;
	bool* isVramDebuggerActive = nullptr;
	float* debuggerPreviousOuterWidth = nullptr;
	float* debuggerWidth = nullptr;
	float* debuggerHeight = nullptr;
	bool* debuggerResizing = nullptr;
	bool* debuggerResetting = nullptr;
	Device::CursorTypes cursor = Device::CursorTypes::POINTER;
	bool hasPopup;
	unsigned deviceFps;
	unsigned fps;
	bool isNewFrame = false;
	ButtonEventHandler onDeviceButtonClicked = nullptr;
	ButtonEventHandler onCartridgeButtonClicked = nullptr;

	ImVec2 regSize;         // Content region size of the current ImGui window.
	Math::Vec2i srcSize;    // Source texture size of the video buffer to be rendered.
	ImVec2 dstPos;          // Destination position to render the video texture.
	ImVec2 dstSize;         // Destination size to render the video texture.
	Math::Vec2f scale;      // `dstSize` / `srcSize`.
	Math::Vec2i clientSize; // The size of the client area.

	bool canShowOnscreenGamepad = false;
	bool canShowCodeDebugger = false;
	bool canShowVramDebugger = false;
	bool codeDebuggerGotSafeHeight = false;
	bool vramDebuggerGotSafeHeight = false;

	Context(
		Window* window_, Renderer* renderer_,
		Theme* theme_,
		Input* input_,
		const Device::Ptr &canvasDevice_, const Texture::Ptr &canvasTexture_, const Texture::Ptr &canvasTextureForBorderFrame_,
		const std::string &cartridgeStatusText_, const std::string &deviceStatusText_, const std::string &statusTooltip_, float* statusBarWidth_, float statusBarHeight_, bool showStatus_,
		bool* emulatorMuted_, bool (&emulatorChannelMuted_)[DEVICE_AUDIO_CHANNEL_COUNT], int* emulatorSpeed_, int* emulatorFastForwardSpeed_,
		bool integerScale_, bool fixRatio_,
		bool* onscreenGamepadEnabled_, bool onscreenGamepadSwapAB_, float onscreenGamepadScale_, const Math::Vec2<float> onscreenGamepadPadding_,
		bool* onscreenDebugEnabled_,
		Debugger* debugger_, bool* codeDebugEnabled_, bool* codeDebuggerShowObjectBounds_, bool* bringCodeDebuggerToFront_,
		VramDebugger* vramDebugger_, bool* vramDebugEnabled_, bool* vramDebuggerPreviewPaletteBits_, bool* vramDebuggerShowGrids_, bool* isVramDebuggerActive_,
		float* debuggerPreviousOuterWidth_, float* debuggerWidth_, float* debuggerHeight_,  bool* debuggerResizing_, bool* debuggerResetting_,
		Device::CursorTypes cursor_,
		bool hasPopup_,
		unsigned deviceFps_, unsigned fps_,
		bool isNewFrame_,
		ButtonEventHandler onDeviceButtonClicked_, ButtonEventHandler onCartridgeButtonClicked_
	) :
		window(window_), renderer(renderer_),
		theme(theme_),
		input(input_),
		canvasDevice(canvasDevice_.get()), canvasTexture(canvasTexture_.get()), canvasTextureForBorderFrame(canvasTextureForBorderFrame_.get()),
		cartridgeStatusText(cartridgeStatusText_), deviceStatusText(deviceStatusText_), statusTooltip(statusTooltip_), statusBarWidth(statusBarWidth_), statusBarHeight(statusBarHeight_), showStatus(showStatus_),
		emulatorMuted(emulatorMuted_), emulatorChannelMuted(emulatorChannelMuted_), emulatorSpeed(emulatorSpeed_), emulatorPreferedSpeed(emulatorFastForwardSpeed_),
		integerScale(integerScale_), fixRatio(fixRatio_),
		onscreenGamepadEnabled(onscreenGamepadEnabled_), onscreenGamepadSwapAB(onscreenGamepadSwapAB_), onscreenGamepadScale(onscreenGamepadScale_), onscreenGamepadPadding(onscreenGamepadPadding_),
		onscreenDebugEnabled(onscreenDebugEnabled_),
		debugger(debugger_), codeDebugEnabled(codeDebugEnabled_), codeDebuggerShowObjectBounds(codeDebuggerShowObjectBounds_), bringCodeDebuggerToFront(bringCodeDebuggerToFront_),
		vramDebugger(vramDebugger_), vramDebugEnabled(vramDebugEnabled_), vramDebuggerPreviewPaletteBits(vramDebuggerPreviewPaletteBits_), vramDebuggerShowGrids(vramDebuggerShowGrids_), isVramDebuggerActive(isVramDebuggerActive_),
		debuggerPreviousOuterWidth(debuggerPreviousOuterWidth_), debuggerWidth(debuggerWidth_), debuggerHeight(debuggerHeight_),  debuggerResizing(debuggerResizing_), debuggerResetting(debuggerResetting_),
		cursor(cursor_),
		hasPopup(hasPopup_),
		deviceFps(deviceFps_), fps(fps_),
		isNewFrame(isNewFrame_),
		onDeviceButtonClicked(onDeviceButtonClicked_), onCartridgeButtonClicked(onCartridgeButtonClicked_)
	{
	}

	// Begin rendering context.
	void begin(bool showSgbBorder, bool &codeDbg_, bool &vramDbg_) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.WindowBorderSize;

		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		regSize = ImVec2(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		canShowCodeDebugger = canShowVramDebugger = regSize.x >= SCREEN_WIDTH + EMULATOR_VRAM_DEBUGGER_MIN_WIDTH + 2;
		const bool codeDbg = canShowCodeDebugger && (!!debugger && *codeDebugEnabled);
		const bool vramDbg = canShowVramDebugger && (!!vramDebugger && *vramDebugEnabled);
		if (codeDbg || vramDbg) {
			const ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoNav;

			const float height = regSize.y - statusBarHeight - borderSize * 2;
			codeDebuggerGotSafeHeight = debugger->safeHeight() > 0 && height >= debugger->safeHeight();
			vramDebuggerGotSafeHeight = vramDebugger->safeHeight() > 0 && height >= vramDebugger->safeHeight();

			float preferedWidth;
			if (*isVramDebuggerActive) {
				preferedWidth = vramDebuggerGotSafeHeight ?
					(EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + 2) :
					(EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2);
			} else {
				preferedWidth = codeDebuggerGotSafeHeight ?
					(EMULATOR_CODE_DEBUGGER_MAX_WIDTH + 2) :
					(EMULATOR_CODE_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2);
			}
			if (*debuggerWidth <= 0) {
				*debuggerWidth = calculateDebuggerWidth(preferedWidth);
			}
			if (*debuggerPreviousOuterWidth <= 0) {
				*debuggerPreviousOuterWidth = regSize.x;
			}
			if (*debuggerPreviousOuterWidth != regSize.x) {
				*debuggerWidth = *debuggerWidth / *debuggerPreviousOuterWidth * regSize.x;
				*debuggerWidth = calculateDebuggerWidth(preferedWidth);
				*debuggerPreviousOuterWidth = regSize.x;
			}

			const ImVec2 curPos = ImGui::GetCursorScreenPos();
			const ImVec2 size(
				regSize.x - *debuggerWidth,
				regSize.y - (showStatus ? (statusBarHeight - style.ChildBorderSize) : style.ChildBorderSize)
			);
			ImGui::PushClipRect(curPos, curPos + size, false);

			ImGui::BeginChild("#Cvs", ImVec2(regSize.x - *debuggerWidth, regSize.y), true, flags);
		}

		ImVec2 regSize_ = regSize;
		if (codeDbg || vramDbg) {
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
		codeDbg_ = codeDbg;
		vramDbg_ = vramDbg;
	}
	// End rendering context.
	void end(bool codeDbg, bool vramDbg) {
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

		if (codeDbg || vramDbg) {
			ImGui::EndChild();

			ImGui::PopClipRect();
		}
	}

	bool beginDebug(bool codeDbg, bool vramDbg, bool &wasResizing_, bool &isResizing_, bool &isResetting_, float &width_, float &height_, ImGuiWindowFlags &flags_) {
		// Prepare.
		if (!codeDbg && !vramDbg)
			return false;

		// Begin tab bar.
		bool tabOpened = false;
		if (codeDbg && vramDbg) {
			ImGui::SameLine();

			tabOpened = ImGui::BeginTabBar("@Dbg");
		}

		const bool wasResizing = *debuggerResizing;
		bool isResizing = *debuggerResizing;
		bool isResetting = *debuggerResetting;
		int safeHeight;
		bool debuggerGotSafeHeight;
		if (codeDbg && vramDbg) {
			if (*isVramDebuggerActive) {
				safeHeight = vramDebugger->safeHeight();
				debuggerGotSafeHeight = vramDebuggerGotSafeHeight;
			} else {
				safeHeight = debugger->safeHeight();
				debuggerGotSafeHeight = codeDebuggerGotSafeHeight;
			}
		} else if (codeDbg) {
			safeHeight = debugger->safeHeight();
			debuggerGotSafeHeight = codeDebuggerGotSafeHeight;
		} else /* if (vramDbg) */ {
			safeHeight = vramDebugger->safeHeight();
			debuggerGotSafeHeight = vramDebuggerGotSafeHeight;
		}

		if (!tabOpened)
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
			ImGuiWindowFlags_NoNav;
		const float x = (float)ImGui::GetCursorPosX();
		const float height = regSize.y - statusBarHeight - borderSize * 2;
		const bool heightIsSafeForDebugger = height >= safeHeight;
		if (!debuggerGotSafeHeight)
			flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
		const float width = *debuggerWidth;
		const bool heightChanged = *debuggerHeight != height;
		if (heightChanged)
			*debuggerHeight = height;

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

		if (isResetting || heightChanged || safeHeight == 0) {
			*debuggerWidth = 0;
		} else if (isResizing) {
			const ImVec2 mousePos = ImGui::GetMousePos();
			*debuggerWidth = calculateDebuggerWidth(regSize.x - mousePos.x);
		}

		wasResizing_ = wasResizing;
		isResizing_ = isResizing;
		isResetting_ = isResetting;
		width_ = width;
		height_ = height;
		flags_ = flags;

		return tabOpened;
	}
	void endDebug(bool codeDbg, bool vramDbg, bool tabOpened, bool wasResizing, bool isResizing, bool isResetting) {
		// Prepare.
		if (!codeDbg && !vramDbg)
			return;

		// Finish.
		if (wasResizing) {
			ImGui::PopStyleColor();
		}
		ImGui::PopStyleVar();

		*debuggerResizing = isResizing;
		*debuggerResetting = isResetting;

		// End tab bar.
		if (tabOpened) {
			ImGui::EndTabBar();
		}
	}
	void debugCode(bool codeDbg, bool /* vramDbg */, bool tabOpened, const ImVec2 &pos, const ImVec2 &size, ImGuiWindowFlags flags) {
		// Prepare.
		if (!codeDbg)
			return;

		const bool focus = *bringCodeDebuggerToFront;
		if (focus)
			*bringCodeDebuggerToFront = false;

		// Draw the code debugger.
		if (tabOpened) {
			const bool wasVramDebuggerActive = *isVramDebuggerActive;
			ImGuiTabItemFlags tabFlags = ImGuiTabItemFlags_NoTooltip;
			if (focus)
				tabFlags |= ImGuiTabItemFlags_SetSelected;
			if (ImGui::BeginTabItem(theme->windowEmulator_Debugger_Code(), nullptr, tabFlags)) {
				ImGui::SetCursorScreenPos(pos);

				ImGui::BeginChild("#CDbg", ImVec2(size.x, size.y - 19.0f), true, flags);
				{
					debugger->update(true, !tabOpened, *codeDebuggerShowObjectBounds);
				}
				ImGui::EndChild();

				*isVramDebuggerActive = false;

				ImGui::EndTabItem();
			} else {
				if (wasVramDebuggerActive && *debuggerWidth > EMULATOR_VRAM_DEBUGGER_MAX_WIDTH)
					*debuggerWidth = 0;

				debugger->update(false, !tabOpened, *codeDebuggerShowObjectBounds);
			}
		} else {
			ImGui::BeginChild("#CDbg", size, true, flags);
			{
				debugger->update(true, !tabOpened, *codeDebuggerShowObjectBounds);
			}
			ImGui::EndChild();
		}
	}
	void debugVram(bool /* codeDbg */, bool vramDbg, bool tabOpened, const ImVec2 &pos, const ImVec2 &size, ImGuiWindowFlags flags) {
		// Prepare.
		if (!vramDbg)
			return;

		// Draw the VRAM debugger.
		if (tabOpened) {
			if (ImGui::BeginTabItem(theme->windowEmulator_Debugger_Vram(), nullptr, ImGuiTabItemFlags_NoTooltip)) {
				ImGui::SetCursorScreenPos(pos);

				ImGui::BeginChild("#VDbg", ImVec2(size.x, size.y - 19.0f), true, flags);
				{
					vramDebugger->update(
						*vramDebuggerPreviewPaletteBits, *vramDebuggerShowGrids,
						isNewFrame,
						!tabOpened
					);
				}
				ImGui::EndChild();

				*isVramDebuggerActive = true;

				ImGui::EndTabItem();
			}
		} else {
			ImGui::BeginChild("#VDbg", size, true, flags);
			{
				vramDebugger->update(
					*vramDebuggerPreviewPaletteBits, *vramDebuggerShowGrids,
					isNewFrame,
					!tabOpened
				);
			}
			ImGui::EndChild();
		}
	}

private:
	float calculateDebuggerWidth(float width) const {
		ImGuiStyle &style = ImGui::GetStyle();

		float maxWidth;
		if (*isVramDebuggerActive) {
			maxWidth = vramDebuggerGotSafeHeight ?
				(EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + 2) :
				(EMULATOR_VRAM_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2);
			width = std::min(width, std::min(regSize.x - SCREEN_WIDTH, maxWidth));
			width = std::max(width, EMULATOR_VRAM_DEBUGGER_MIN_WIDTH);
		} else {
			maxWidth = codeDebuggerGotSafeHeight ?
				(EMULATOR_CODE_DEBUGGER_MAX_WIDTH + 2) :
				(EMULATOR_CODE_DEBUGGER_MAX_WIDTH + style.ScrollbarSize + 2);
			width = std::min(width, std::min(regSize.x - SCREEN_WIDTH, maxWidth));
			width = std::max(width, EMULATOR_CODE_DEBUGGER_MIN_WIDTH);
		}

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
		if (context.canvasDevice->supportsGettingDuty()) {
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
		if (context.canvasDevice->supportsVariableSpeed()) {
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
		if (context.canvasDevice->supportsMutingAudioChannel()) {
			if (*context.emulatorMuted) {
				if (ImGui::ImageButton(context.theme->iconMuted()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
					*context.emulatorMuted = false;
					for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i) {
						context.emulatorChannelMuted[i] = false;
						context.canvasDevice->muteAudioChannel(i, false);
					}
				}
			} else {
				if (ImGui::ImageButton(context.theme->iconLoud()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
					ImGui::OpenPopup("@Mute");
				}
			}
		} else {
			if (*context.emulatorMuted) {
				if (ImGui::ImageButton(context.theme->iconMuted()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
					*context.emulatorMuted = false;
				}
			} else {
				if (ImGui::ImageButton(context.theme->iconLoud()->pointer(context.renderer), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
					*context.emulatorMuted = true;
				}
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

	// Show the menus.
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

	do {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Mute")) {
			bool channels[DEVICE_AUDIO_CHANNEL_COUNT];
			for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i)
				channels[i] = !context.emulatorChannelMuted[i];

			if (ImGui::MenuItem(context.theme->menu_Channel0Duty1(), nullptr, &channels[0])) {
				context.emulatorChannelMuted[0] = !channels[0];
				context.canvasDevice->muteAudioChannel(0, !channels[0]);
				bool allMuted = true;
				for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i) {
					if (channels[i]) {
						allMuted = false;

						break;
					}
				}
				*context.emulatorMuted = allMuted;
			}
			if (ImGui::MenuItem(context.theme->menu_Channel1Duty2(), nullptr, &channels[1])) {
				context.emulatorChannelMuted[1] = !channels[1];
				context.canvasDevice->muteAudioChannel(1, !channels[1]);
				bool allMuted = true;
				for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i) {
					if (channels[i]) {
						allMuted = false;

						break;
					}
				}
				*context.emulatorMuted = allMuted;
			}
			if (ImGui::MenuItem(context.theme->menu_Channel2Wave(), nullptr, &channels[2])) {
				context.emulatorChannelMuted[2] = !channels[2];
				context.canvasDevice->muteAudioChannel(2, !channels[2]);
				bool allMuted = true;
				for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i) {
					if (channels[i]) {
						allMuted = false;

						break;
					}
				}
				*context.emulatorMuted = allMuted;
			}
			if (ImGui::MenuItem(context.theme->menu_Channel3Noise(), nullptr, &channels[3])) {
				context.emulatorChannelMuted[3] = !channels[3];
				context.canvasDevice->muteAudioChannel(3, !channels[3]);
				bool allMuted = true;
				for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i) {
					if (channels[i]) {
						allMuted = false;

						break;
					}
				}
				*context.emulatorMuted = allMuted;
			}
			ImGui::Separator();
			if (ImGui::MenuItem(context.theme->menu_MuteAllChannels())) {
				for (int i = 0; i < DEVICE_AUDIO_CHANNEL_COUNT; ++i) {
					context.emulatorChannelMuted[i] = true;
					context.canvasDevice->muteAudioChannel(i, true);
				}
				*context.emulatorMuted = true;
			}

			ImGui::EndPopup();
		}
	} while (false);

	do {
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

			if (!!context.debugger) {
				ImGui::Separator();

				if (context.canShowCodeDebugger) {
					ImGui::MenuItem(context.theme->menu_CodeDebugger(), nullptr, context.codeDebugEnabled);
				} else {
					ImGui::BeginDisabled();
					ImGui::MenuItem(context.theme->menu_CodeDebugger(), nullptr, context.codeDebugEnabled);
					ImGui::EndDisabled();
				}
				if (context.canShowCodeDebugger && *context.codeDebugEnabled) {
					ImGui::MenuItem(context.theme->menu_ObjectBounds(), nullptr, context.codeDebuggerShowObjectBounds);
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(context.theme->tooltip_PreviewObjectBounds());
					}
				} else {
					ImGui::BeginDisabled();
					ImGui::MenuItem(context.theme->menu_ObjectBounds(), nullptr, context.codeDebuggerShowObjectBounds);
					ImGui::EndDisabled();
				}
			}

			if (!!context.vramDebugger) {
				ImGui::Separator();

				if (context.canShowVramDebugger) {
					ImGui::MenuItem(context.theme->menu_VramDebugger(), nullptr, context.vramDebugEnabled);
				} else {
					ImGui::BeginDisabled();
					ImGui::MenuItem(context.theme->menu_VramDebugger(), nullptr, context.vramDebugEnabled);
					ImGui::EndDisabled();
				}
				if (context.canShowVramDebugger && *context.vramDebugEnabled) {
					ImGui::MenuItem(context.theme->menu_PaletteBits(), nullptr, context.vramDebuggerPreviewPaletteBits);
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(context.theme->tooltip_PreviewPaletteBitsForColoredOnly());
					}
					ImGui::MenuItem(context.theme->menu_Grids(), nullptr, context.vramDebuggerShowGrids);
				} else {
					ImGui::BeginDisabled();
					ImGui::MenuItem(context.theme->menu_PaletteBits(), nullptr, context.vramDebuggerPreviewPaletteBits);
					ImGui::MenuItem(context.theme->menu_Grids(), nullptr, context.vramDebuggerShowGrids);
					ImGui::EndDisabled();
				}
			}

			ImGui::EndPopup();
		}
	} while (false);
}

void emulator(
	class Window* wnd, class Renderer* rnd,
	class Theme* theme,
	Input* input,
	const Device::Ptr &canvasDevice, const Texture::Ptr &canvasTexture, const Texture::Ptr &canvasTextureForBorderFrame,
	const std::string &cartridgeStatusText, const std::string &deviceStatusText, const std::string &statusTooltip, float &statusBarWidth, float statusBarHeight, bool showStatus,
	bool &emulatorMuted, bool (&emulatorChannelMuted)[DEVICE_AUDIO_CHANNEL_COUNT], int &emulatorSpeed, int &emulatorPreferedSpeed,
	bool integerScale, bool fixRatio,
	bool &onscreenGamepadEnabled, bool onscreenGamepadSwapAB, float onscreenGamepadScale, const Math::Vec2<float> &onscreenGamepadPadding,
	bool &onscreenDebugEnabled,
	class Debugger* debugger, bool &codeDebugEnabled, bool &codeDebuggerShowObjectBounds, bool &bringCodeDebuggerToFront,
	class VramDebugger* vramDebugger, bool &vramDebugEnabled, bool &vramDebuggerPreviewPaletteBits, bool &vramDebuggerShowGrids, bool &isVramDebuggerActive,
	float &debuggerPreviousOuterWidth, float &debuggerWidth, float &debuggerHeight, bool &debuggerResizing, bool &debuggerResetting,
	Device::CursorTypes cursor,
	bool hasPopup,
	unsigned fps,
	bool isNewFrame,
	ButtonEventHandler onDeviceButtonClicked, ButtonEventHandler onCartridgeButtonClicked,
	DebugHandler debug
) {
	// Prepare.
	if (bringCodeDebuggerToFront)
		codeDebugEnabled = true;

	Context context(
		wnd, rnd,
		theme,
		input,
		canvasDevice, canvasTexture, canvasTextureForBorderFrame,
		cartridgeStatusText, deviceStatusText, statusTooltip, &statusBarWidth, statusBarHeight, showStatus,
		&emulatorMuted, emulatorChannelMuted, &emulatorSpeed, &emulatorPreferedSpeed,
		integerScale, fixRatio,
		&onscreenGamepadEnabled, onscreenGamepadSwapAB, onscreenGamepadScale, onscreenGamepadPadding,
		&onscreenDebugEnabled,
		debugger, &codeDebugEnabled, &codeDebuggerShowObjectBounds, &bringCodeDebuggerToFront,
		vramDebugger, &vramDebugEnabled, &vramDebuggerPreviewPaletteBits, &vramDebuggerShowGrids, &isVramDebuggerActive,
		&debuggerPreviousOuterWidth, &debuggerWidth, &debuggerHeight, &debuggerResizing, &debuggerResetting,
		cursor,
		hasPopup,
		canvasDevice->fps(), fps,
		isNewFrame,
		onDeviceButtonClicked, onCartridgeButtonClicked
	);

	GBBASIC_ASSERT(!!canvasDevice && "Impossible.");

	if (!canvasTexture) // Not ready.
		return;

	// Render emulation.
	bool codeDbg = false;
	bool vramDbg = false;
	context.begin(!!canvasTextureForBorderFrame, codeDbg, vramDbg);
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
		const ImVec2 curPos = ImGui::GetCursorScreenPos();
		ImGui::Image(
			canvasTexture->pointer(rnd),
			context.dstSize,
			ImVec2(0, 0), ImVec2(1, 1),
			ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
		);

		// Render highlight area from the code and VRAM debuggers.
		if (codeDbg && !*context.isVramDebuggerActive) {
			const int n = debugger->highlightCount();
			if (n > 0) {
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImGui::PushClipRect(curPos, curPos + context.dstSize, true);
				for (int i = 0; i < n; ++i) {
					Debugger::Highlight highlight;
					if (!debugger->getHighlight(i, &highlight))
						continue;

					const Math::Vec2f start(highlight.area.xMin() * context.scale.x, highlight.area.yMin() * context.scale.y);
					const Math::Vec2f end((highlight.area.xMax() + 1) * context.scale.x, (highlight.area.yMax() + 1) * context.scale.y);
					drawList->AddRect(
						curPos + ImVec2((float)start.x, (float)start.y),
						curPos + ImVec2((float)end.x, (float)end.y),
						ImGui::GetColorU32(ImVec4((float)highlight.color.x, (float)highlight.color.y, (float)highlight.color.z, (float)highlight.color.w)),
						0.0f,
						ImDrawFlags_None,
						2.0f
					);
				}
				ImGui::PopClipRect();
			}
		} else if (vramDbg && *context.isVramDebuggerActive) {
			const int n = vramDebugger->highlightCount();
			if (n > 0) {
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImGui::PushClipRect(curPos, curPos + context.dstSize, true);
				for (int i = 0; i < n; ++i) {
					VramDebugger::Highlight highlight;
					if (!vramDebugger->getHighlight(i, &highlight))
						continue;

					const Math::Vec2f start(highlight.area.xMin() * context.scale.x, highlight.area.yMin() * context.scale.y);
					const Math::Vec2f end((highlight.area.xMax() + 1) * context.scale.x, (highlight.area.yMax() + 1) * context.scale.y);
					drawList->AddRect(
						curPos + ImVec2((float)start.x, (float)start.y),
						curPos + ImVec2((float)end.x, (float)end.y),
						ImGui::GetColorU32(ImVec4((float)highlight.color.x, (float)highlight.color.y, (float)highlight.color.z, (float)highlight.color.w)),
						0.0f,
						ImDrawFlags_None,
						2.0f
					);
				}
				ImGui::PopClipRect();
			}
		}

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
	context.end(codeDbg, vramDbg);

	// Render the debuggers.
	bool wasResizing = false, isResizing = false, isResetting = false;
	float width = 0, height = 0;
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	const bool tabOpened = context.beginDebug(codeDbg, vramDbg, wasResizing, isResizing, isResetting, width, height, flags);
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos();

		context.debugVram(codeDbg, vramDbg, tabOpened, pos, ImVec2(width, height), flags);

		context.debugCode(codeDbg, vramDbg, tabOpened, pos, ImVec2(width, height), flags);
	}
	context.endDebug(codeDbg, vramDbg, tabOpened, wasResizing, isResizing, isResetting);

	// Render the status.
	renderStatus(context);

	// Check the shortcuts.
	shortcuts(context);

	// Show the context menu.
	menu(context);
}

/* ===========================================================================} */
