/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "../gbbasic.h"
#include "device.h"
#include "../utils/input.h"

/*
** {===========================================================================
** Settings
*/

struct Settings {
	enum IndentRules : unsigned {
		SPACE_2,
		SPACE_4,
		SPACE_8,
		TAB_2,
		TAB_4,
		TAB_8
	};
	enum ColumnIndicator : unsigned {
		COL_NONE,
		COL_40,
		COL_60,
		COL_80,
		COL_100,
		COL_120
	};

	int applicationWindowDisplayIndex = 0;
	bool applicationWindowBorderless = false; // Non-serialized.
	bool applicationWindowFullscreen = false;
	bool applicationWindowMaximized = false;
	Math::Vec2i applicationWindowPosition;
	Math::Vec2i applicationWindowSize;
	bool applicationFirstRun = true;
	unsigned applicationLoadedExampleRevision = 0;

	std::string exporterSettings;
	std::string exporterArgs;
	Bytes::Ptr exporterIcon = nullptr; // Non-serialized.

	unsigned recentProjectsOrder = 0;
	bool recentRemoveFromDisk = false;
	bool recentIconView = true;

	bool mainShowProjectPathAtTitleBar = true;
	IndentRules mainIndentRule = SPACE_2;
	ColumnIndicator mainColumnIndicator = COL_80;
	bool mainShowWhiteSpaces = true;
	bool mainCaseSensitive = false;
	bool mainMatchWholeWords = false;
	bool mainGlobalSearch = false;
	bool mainCorrectYZForShortcuts = false;

	bool debugShowAstEnabled = false;
	bool debugOnscreenShellEnabled = true;
	bool debugCodeInspectorEnabled = true;
	bool debugCodeShowObjectBounds = true;
	bool debugVramInspectorEnabled = true;
	bool debugVramPreviewPaletteBits = true;
	bool debugVramShowGrids = true;
	bool debugLogEnabled = false;
	std::string debugLogPath;

	unsigned deviceType = 0;
	bool devicePreferSgb = false;
	bool deviceSaveSramOnStop = true;
	Colour deviceClassicPalette[4];

	bool emulatorMuted = false;
	bool emulatorChannelMuted[DEVICE_AUDIO_CHANNEL_COUNT];
	int emulatorSpeed = DEVICE_BASE_SPEED_FACTOR * 1;
	int emulatorPreferedSpeed = DEVICE_BASE_SPEED_FACTOR * DEVICE_DEFAULT_PREFERED_SPEED;

	bool canvasFixRatio = true;
	bool canvasIntegerScale = true;

	bool consoleClearOnStart = true;

	Input::Gamepad inputGamepads[INPUT_GAMEPAD_COUNT];
	bool inputOnscreenGamepadEnabled = true;
	bool inputOnscreenGamepadSwapAB = false;
	float inputOnscreenGamepadScale = 1.0f;
	Math::Vec2<float> inputOnscreenGamepadPadding = Math::Vec2<float>(8.0f, 12.0f);

	Settings();
	Settings(const Settings &) = delete;

	Settings &operator = (const Settings &other);

	bool operator != (const Settings &other) const;
};

/* ===========================================================================} */

#endif /* __SETTINGS_H__ */
