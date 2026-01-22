/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "../gbbasic.h"
#include "device.h"
#include "../utils/texture.h"

/*
** {===========================================================================
** Emulator
*/

typedef std::function<void(void)> DebugHandler;

typedef std::function<void(void)> ButtonEventHandler;

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
	class VramDebugger* vramDebugger /* nullable */, bool &vramDebugEnabled, float &vramDebuggerPreviousOuterWidth, float &vramDebuggerWidth, bool &vramDebuggerResizing, bool &vramDebuggerResetting,
	Device::CursorTypes cursor,
	bool hasPopup,
	unsigned fps,
	ButtonEventHandler onDeviceButtonClicked /* nullable */, ButtonEventHandler onCartridgeButtonClicked /* nullable */,
	DebugHandler debug
);

/* ===========================================================================} */

#endif /* __EMULATOR_H__ */
