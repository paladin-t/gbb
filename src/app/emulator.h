/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
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

void emulator(
	class Window* wnd, class Renderer* rnd,
	class Theme* theme,
	Input* input,
	const Device::Ptr &canvasDevice, const Texture::Ptr &canvasTexture, const Texture::Ptr &canvasTextureForBorderFrame,
	const std::string &statusText, const std::string &statusTooltip, float &statusBarWidth, float statusBarHeight, bool showStatus,
	bool &emulatorMuted, int &emulatorSpeed, int &emulatorPreferedSpeed,
	bool integerScale, bool fixRatio,
	bool &onscreenGamepadEnabled, bool onscreenGamepadSwapAB, float onscreenGamepadScale, const Math::Vec2<float> &onscreenGamepadPadding,
	bool &onscreenDebugEnabled,
	Device::CursorTypes cursor,
	bool hasPopup,
	unsigned fps,
	DebugHandler debug
);

/* ===========================================================================} */

#endif /* __EMULATOR_H__ */
