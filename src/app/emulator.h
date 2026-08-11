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
	bool &emulatorMuted, bool (&emulatorChannelMuted)[DEVICE_AUDIO_CHANNEL_COUNT], int &emulatorSpeed, int &emulatorPreferedSpeed,
	bool integerScale, bool fixRatio,
	bool &onscreenGamepadEnabled, bool onscreenGamepadSwapAB, float onscreenGamepadScale, const Math::Vec2<float> &onscreenGamepadPadding,
	bool &onscreenDebugEnabled,
	class Debugger* debugger /* nullable */, bool &codeDebugEnabled, bool &bringCodeDebuggerToFront,
	class VramDebugger* vramDebugger /* nullable */, bool &vramDebugEnabled, bool &vramDebuggerPreviewPaletteBits, bool &vramDebuggerShowGrids,
	float &debuggerPreviousOuterWidth, float &debuggerWidth, float &debuggerHeight, bool &debuggerResizing, bool &debuggerResetting,
	Device::CursorTypes cursor,
	bool hasPopup,
	unsigned fps,
	bool isNewFrame,
	ButtonEventHandler onDeviceButtonClicked /* nullable */, ButtonEventHandler onCartridgeButtonClicked /* nullable */,
	DebugHandler debug
);

/* ===========================================================================} */

#endif /* __EMULATOR_H__ */
