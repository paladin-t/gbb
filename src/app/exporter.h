/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EXPORTER_H__
#define __EXPORTER_H__

#include "../gbbasic.h"
#include "../utils/bytes.h"
#include "../utils/colour.h"
#include "../utils/entry.h"
#include "../utils/input.h"
#include "../utils/localization.h"
#include "../utils/web.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EXPORTER_RULES_DIR
#	define EXPORTER_RULES_DIR "../exporters/" /* Relative path. */
#endif /* EXPORTER_RULES_DIR */

#ifndef EXPORTER_ICON_MIN_WIDTH
#	define EXPORTER_ICON_MIN_WIDTH 128
#endif /* EXPORTER_ICON_MIN_WIDTH */
#ifndef EXPORTER_ICON_MIN_HEIGHT
#	define EXPORTER_ICON_MIN_HEIGHT 128
#endif /* EXPORTER_ICON_MIN_HEIGHT */
#ifndef EXPORTER_ICON_MAX_WIDTH
#	define EXPORTER_ICON_MAX_WIDTH 2048
#endif /* EXPORTER_ICON_MAX_WIDTH */
#ifndef EXPORTER_ICON_MAX_HEIGHT
#	define EXPORTER_ICON_MAX_HEIGHT 2048
#endif /* EXPORTER_ICON_MAX_HEIGHT */

/* ===========================================================================} */

/*
** {===========================================================================
** Exporter
*/

class Exporter : public virtual Object {
public:
	typedef std::shared_ptr<Exporter> Ptr;
	typedef std::vector<Ptr> Array;

	typedef std::function<void(const std::string &, int)> OutputHandler;

	struct Settings {
		bool emulatorShowTitleBar = true;
		bool emulatorShowStatusBar = true;
		bool emulatorShowPreferenceDialogOnEscPress = true;

		unsigned deviceType = 0;
		bool devicePreferSgb = false;
		bool deviceSaveSramOnStop = true;
		Colour deviceClassicPalette[4];

		bool canvasFixRatio = true;
		bool canvasIntegerScale = true;

		Input::Gamepad inputGamepads[INPUT_GAMEPAD_COUNT];
		bool inputOnscreenGamepadEnabled = true;
		bool inputOnscreenGamepadSwapAB = false;
		float inputOnscreenGamepadScale = 1.0f;
		Math::Vec2<float> inputOnscreenGamepadPadding = Math::Vec2<float>(8.0f, 12.0f);

		bool debugOnscreenDebugEnabled = true;

		Settings();
		Settings(const Settings &) = delete;

		Settings &operator = (const Settings &other);

		bool operator != (const Settings &other) const;

		bool toString(std::string &val, bool pretty = true) const;
		bool fromString(const std::string &val);
	};

public:
	GBBASIC_PROPERTY(std::string, path)
	GBBASIC_PROPERTY(Entry, entry)
	GBBASIC_PROPERTY(Localization::Dictionary, title)
	GBBASIC_PROPERTY(int, order)
	GBBASIC_PROPERTY(bool, isMajor)
	GBBASIC_PROPERTY(bool, isMinor)
	GBBASIC_PROPERTY(bool, messageEnabled)
	GBBASIC_PROPERTY(std::string, messageContent)
	GBBASIC_PROPERTY(Text::Array, extensions)
	GBBASIC_PROPERTY(Text::Array, filter)
	GBBASIC_PROPERTY(bool, buildEnabled)
	GBBASIC_PROPERTY(bool, packageArchived)
	GBBASIC_PROPERTY(std::string, packageTemplate)
	GBBASIC_PROPERTY(std::string, packageEntry)
	GBBASIC_PROPERTY(std::string, packageConfig)
	GBBASIC_PROPERTY(std::string, packageArgs)
	GBBASIC_PROPERTY(std::string, packageIcon)
	GBBASIC_PROPERTY(std::string, surfMethod)
	GBBASIC_PROPERTY(std::string, surfEntry)
	GBBASIC_PROPERTY(Text::Array, licenses)

private:
	mutable Web::Ptr _web = nullptr;

public:
	GBBASIC_CLASS_TYPE('X', 'P', 'T', 'R')

	Exporter();
	virtual ~Exporter() override;

	virtual unsigned type(void) const override;

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Object** ptr) const override;

	bool open(const char* path, const char* menu);
	bool close(void);

	/**
	 * @param[out] exported
	 * @param[out] hosted
	 */
	bool run(
		const char* path, Bytes::Ptr rom, std::string* exported /* nullable */, std::string* hosted /* nullable */,
		const std::string &settings, const std::string &args, Bytes::Ptr icon /* nullable */,
		OutputHandler output
	) const;
	void reset(void);

private:
	void gotoBase64(const char* path, Bytes::Ptr rom, std::string* hosted /* nullable */, OutputHandler output) const;
	void gotoWeb(const char* path, Bytes::Ptr rom, std::string* hosted /* nullable */, OutputHandler output) const;
	void gotoUrl(const char* path, Bytes::Ptr rom, std::string* hosted /* nullable */, OutputHandler output) const;
};

/* ===========================================================================} */

#endif /* __EXPORTER_H__ */
