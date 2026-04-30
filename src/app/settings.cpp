/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "settings.h"
#include <SDL.h>

/*
** {===========================================================================
** Settings
*/

Settings::Settings() {
	static_assert(INPUT_GAMEPAD_COUNT >= 2, "Wrong size.");

	applicationWindowPosition = Math::Vec2i(-1, -1);

	deviceType = (unsigned)Device::DeviceTypes::COLORED_EXTENDED;
	devicePreferSgb = false;
	deviceSaveSramOnStop = true;
	deviceClassicPalette[0] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_0);
	deviceClassicPalette[1] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_1);
	deviceClassicPalette[2] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_2);
	deviceClassicPalette[3] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_3);

	inputGamepads[0].buttons[Input::UP]     = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_W);
	inputGamepads[0].buttons[Input::DOWN]   = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_S);
	inputGamepads[0].buttons[Input::LEFT]   = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_A);
	inputGamepads[0].buttons[Input::RIGHT]  = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_D);
	inputGamepads[0].buttons[Input::A]      = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_K);
	inputGamepads[0].buttons[Input::B]      = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_J);
	inputGamepads[0].buttons[Input::SELECT] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_1);
	inputGamepads[0].buttons[Input::START]  = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_2);

	inputGamepads[1].buttons[Input::UP]     = Input::Button(Input::JOYSTICK, 0, 1, -1);
	inputGamepads[1].buttons[Input::DOWN]   = Input::Button(Input::JOYSTICK, 0, 1,  1);
	inputGamepads[1].buttons[Input::LEFT]   = Input::Button(Input::JOYSTICK, 0, 0, -1);
	inputGamepads[1].buttons[Input::RIGHT]  = Input::Button(Input::JOYSTICK, 0, 0,  1);
	inputGamepads[1].buttons[Input::A]      = Input::Button(Input::JOYSTICK, 0, 1);
	inputGamepads[1].buttons[Input::B]      = Input::Button(Input::JOYSTICK, 0, 0);
	inputGamepads[1].buttons[Input::SELECT] = Input::Button(Input::JOYSTICK, 0, 2);
	inputGamepads[1].buttons[Input::START]  = Input::Button(Input::JOYSTICK, 0, 3);
}

Settings &Settings::operator = (const Settings &other) {
	applicationWindowDisplayIndex = other.applicationWindowDisplayIndex;
	applicationWindowBorderless = other.applicationWindowBorderless;
	applicationWindowFullscreen = other.applicationWindowFullscreen;
	applicationWindowMaximized = other.applicationWindowMaximized;
	applicationWindowPosition = other.applicationWindowPosition;
	applicationWindowSize = other.applicationWindowSize;
	applicationFirstRun = other.applicationFirstRun;
	applicationLoadedExampleRevision = other.applicationLoadedExampleRevision;

	exporterSettings = other.exporterSettings;
	exporterArgs = other.exporterArgs;

	recentProjectsOrder = other.recentProjectsOrder;
	recentRemoveFromDisk = other.recentRemoveFromDisk;
	recentIconView = other.recentIconView;

	mainShowProjectPathAtTitleBar = other.mainShowProjectPathAtTitleBar;
	mainIndentRule = other.mainIndentRule;
	mainColumnIndicator = other.mainColumnIndicator;
	mainShowWhiteSpaces = other.mainShowWhiteSpaces;
	mainCaseSensitive = other.mainCaseSensitive;
	mainMatchWholeWords = other.mainMatchWholeWords;
	mainGlobalSearch = other.mainGlobalSearch;

	debugShowAstEnabled = other.debugShowAstEnabled;
	debugOnscreenShellEnabled = other.debugOnscreenShellEnabled;
	debugVramInspectorEnabled = other.debugVramInspectorEnabled;
	debugLogEnabled = other.debugLogEnabled;
	debugLogPath = other.debugLogPath;

	deviceType = other.deviceType;
	devicePreferSgb = other.devicePreferSgb;
	deviceSaveSramOnStop = other.deviceSaveSramOnStop;
	for (int i = 0; i < GBBASIC_COUNTOF(deviceClassicPalette); ++i)
		deviceClassicPalette[i] = other.deviceClassicPalette[i];

	emulatorMuted = other.emulatorMuted;
	emulatorSpeed = other.emulatorSpeed;
	emulatorPreferedSpeed = other.emulatorPreferedSpeed;

	canvasFixRatio = other.canvasFixRatio;
	canvasIntegerScale = other.canvasIntegerScale;

	consoleClearOnStart = other.consoleClearOnStart;

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i)
		inputGamepads[i] = other.inputGamepads[i];
	inputOnscreenGamepadEnabled = other.inputOnscreenGamepadEnabled;
	inputOnscreenGamepadSwapAB = other.inputOnscreenGamepadSwapAB;
	inputOnscreenGamepadScale = other.inputOnscreenGamepadScale;
	inputOnscreenGamepadPadding = other.inputOnscreenGamepadPadding;

	return *this;
}

bool Settings::operator != (const Settings &other) const {
	if (applicationWindowDisplayIndex != other.applicationWindowDisplayIndex ||
		applicationWindowBorderless != other.applicationWindowBorderless ||
		applicationWindowFullscreen != other.applicationWindowFullscreen ||
		applicationWindowMaximized != other.applicationWindowMaximized ||
		applicationWindowPosition != other.applicationWindowPosition ||
		applicationWindowSize != other.applicationWindowSize ||
		applicationFirstRun != other.applicationFirstRun ||
		applicationLoadedExampleRevision != other.applicationLoadedExampleRevision
	) {
		return true;
	}

	if (exporterSettings != other.exporterSettings ||
		exporterArgs != other.exporterArgs
	) {
		return true;
	}

	if (recentProjectsOrder != other.recentProjectsOrder ||
		recentRemoveFromDisk != other.recentRemoveFromDisk ||
		recentIconView != other.recentIconView
	) {
		return true;
	}

	if (mainShowProjectPathAtTitleBar != other.mainShowProjectPathAtTitleBar ||
		mainIndentRule != other.mainIndentRule ||
		mainColumnIndicator != other.mainColumnIndicator ||
		mainShowWhiteSpaces != other.mainShowWhiteSpaces ||
		mainCaseSensitive != other.mainCaseSensitive ||
		mainMatchWholeWords != other.mainMatchWholeWords ||
		mainGlobalSearch != other.mainGlobalSearch
	) {
		return true;
	}

	if (debugShowAstEnabled != other.debugShowAstEnabled ||
		debugOnscreenShellEnabled != other.debugOnscreenShellEnabled ||
		debugVramInspectorEnabled != other.debugVramInspectorEnabled ||
		debugLogEnabled != other.debugLogEnabled ||
		debugLogPath != other.debugLogPath
	) {
		return true;
	}

	if (deviceType != other.deviceType ||
		devicePreferSgb != other.devicePreferSgb ||
		deviceSaveSramOnStop != other.deviceSaveSramOnStop
	) {
		return true;
	}
	for (int i = 0; i < GBBASIC_COUNTOF(deviceClassicPalette); ++i) {
		if (deviceClassicPalette[i] != other.deviceClassicPalette[i])
			return true;
	}

	if (emulatorMuted != other.emulatorMuted ||
		emulatorSpeed != other.emulatorSpeed ||
		emulatorPreferedSpeed != other.emulatorPreferedSpeed
	) {
		return true;
	}

	if (canvasFixRatio != other.canvasFixRatio ||
		canvasIntegerScale != other.canvasIntegerScale
	) {
		return true;
	}

	if (consoleClearOnStart != other.consoleClearOnStart) {
		return true;
	}

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		if (inputGamepads[i] != other.inputGamepads[i])
			return true;
	}
	if (inputOnscreenGamepadEnabled != other.inputOnscreenGamepadEnabled ||
		inputOnscreenGamepadSwapAB != other.inputOnscreenGamepadSwapAB ||
		inputOnscreenGamepadScale != other.inputOnscreenGamepadScale ||
		inputOnscreenGamepadPadding != other.inputOnscreenGamepadPadding
	) {
		return true;
	}

	return false;
}

/* ===========================================================================} */
