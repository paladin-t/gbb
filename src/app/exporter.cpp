/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "device.h"
#include "exporter.h"
#include "../utils/archive.h"
#include "../utils/encoding.h"
#include "../utils/file_handle.h"
#include "../utils/filesystem.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

static constexpr const int EXPORTER_PRINT = 0;
static constexpr const int EXPORTER_WARN = 1;
static constexpr const int EXPORTER_ERROR = 2;

/* ===========================================================================} */

/*
** {===========================================================================
** Exporter
*/

Exporter::Settings::Settings() {
	static_assert(INPUT_GAMEPAD_COUNT >= 2, "Wrong size.");

	deviceType = (unsigned)Device::DeviceTypes::COLORED_EXTENDED;
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
	inputGamepads[1].buttons[Input::A]      = Input::Button(Input::JOYSTICK, 0, 0);
	inputGamepads[1].buttons[Input::B]      = Input::Button(Input::JOYSTICK, 0, 1);
	inputGamepads[1].buttons[Input::SELECT] = Input::Button(Input::JOYSTICK, 0, 2);
	inputGamepads[1].buttons[Input::START]  = Input::Button(Input::JOYSTICK, 0, 3);
}

Exporter::Settings &Exporter::Settings::operator = (const Settings &other) {
	emulatorShowTitleBar = other.emulatorShowTitleBar;
	emulatorShowStatusBar = other.emulatorShowStatusBar;
	emulatorShowPreferenceDialogOnEscPress = other.emulatorShowPreferenceDialogOnEscPress;

	deviceType = other.deviceType;
	deviceSaveSramOnStop = other.deviceSaveSramOnStop;
	for (int i = 0; i < GBBASIC_COUNTOF(deviceClassicPalette); ++i)
		deviceClassicPalette[i] = other.deviceClassicPalette[i];

	canvasFixRatio = other.canvasFixRatio;
	canvasIntegerScale = other.canvasIntegerScale;

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i)
		inputGamepads[i] = other.inputGamepads[i];
	inputOnscreenGamepadEnabled = other.inputOnscreenGamepadEnabled;
	inputOnscreenGamepadSwapAB = other.inputOnscreenGamepadSwapAB;
	inputOnscreenGamepadScale = other.inputOnscreenGamepadScale;
	inputOnscreenGamepadPadding = other.inputOnscreenGamepadPadding;

	debugOnscreenDebugEnabled = other.debugOnscreenDebugEnabled;

	return *this;
}

bool Exporter::Settings::operator != (const Settings &other) const {
	if (emulatorShowTitleBar != other.emulatorShowTitleBar ||
		emulatorShowStatusBar != other.emulatorShowStatusBar ||
		emulatorShowPreferenceDialogOnEscPress != other.emulatorShowPreferenceDialogOnEscPress
		) {
		return true;
	}

	if (deviceType != other.deviceType ||
		deviceSaveSramOnStop != other.deviceSaveSramOnStop
	) {
		return true;
	}
	for (int i = 0; i < GBBASIC_COUNTOF(deviceClassicPalette); ++i) {
		if (deviceClassicPalette[i] != other.deviceClassicPalette[i])
			return true;
	}

	if (canvasFixRatio != other.canvasFixRatio ||
		canvasIntegerScale != other.canvasIntegerScale
	) {
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

	if (debugOnscreenDebugEnabled != other.debugOnscreenDebugEnabled) {
		return true;
	}

	return false;
}

bool Exporter::Settings::toString(std::string &val, bool pretty) const {
	rapidjson::Document doc;
	doc.SetObject();

	Jpath::set(doc, doc, emulatorShowTitleBar, "emulator", "show_title_bar");
	Jpath::set(doc, doc, emulatorShowStatusBar, "emulator", "show_status_bar");
	Jpath::set(doc, doc, emulatorShowPreferenceDialogOnEscPress, "emulator", "show_preference_dialog_on_esc_press");

	Jpath::set(doc, doc, deviceType, "device", "type");
	Jpath::set(doc, doc, deviceSaveSramOnStop, "device", "save_sram_on_stop");
	for (int i = 0; i < GBBASIC_COUNTOF(deviceClassicPalette); ++i) {
		Jpath::set(doc, doc, deviceClassicPalette[i].toRGBA(), "device", "classic_palette", i);
	}

	Jpath::set(doc, doc, canvasFixRatio, "canvas", "fix_ratio");
	Jpath::set(doc, doc, canvasIntegerScale, "canvas", "integer_scale");

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		const Input::Gamepad &pad = inputGamepads[i];
		for (int j = 0; j < Input::BUTTON_COUNT; ++j) {
			const unsigned dev = pad.buttons[j].device;
			Jpath::set(doc, doc, dev, "input", "gamepad", i, j, "device");
			Jpath::set(doc, doc, pad.buttons[j].index, "input", "gamepad", i, j, "index");
			Jpath::set(doc, doc, (unsigned short)pad.buttons[j].type, "input", "gamepad", i, j, "type");
			switch (pad.buttons[j].type) {
			case Input::VALUE:
				Jpath::set(doc, doc, pad.buttons[j].value, "input", "gamepad", i, j, "value");

				break;
			case Input::HAT:
				Jpath::set(doc, doc, pad.buttons[j].hat.index, "input", "gamepad", i, j, "sub");
				Jpath::set(doc, doc, (unsigned short)pad.buttons[j].hat.value, "input", "gamepad", i, j, "value");

				break;
			case Input::AXIS:
				Jpath::set(doc, doc, pad.buttons[j].axis.index, "input", "gamepad", i, j, "sub");
				Jpath::set(doc, doc, pad.buttons[j].axis.value, "input", "gamepad", i, j, "value");

				break;
			}
		}
	}
	Jpath::set(doc, doc, inputOnscreenGamepadEnabled, "input", "onscreen_gamepad", "enabled");
	Jpath::set(doc, doc, inputOnscreenGamepadSwapAB, "input", "onscreen_gamepad", "swap_ab");
	Jpath::set(doc, doc, inputOnscreenGamepadScale, "input", "onscreen_gamepad", "scale");
	Jpath::set(doc, doc, inputOnscreenGamepadPadding.x, "input", "onscreen_gamepad", "padding", 0);
	Jpath::set(doc, doc, inputOnscreenGamepadPadding.y, "input", "onscreen_gamepad", "padding", 1);

	Jpath::set(doc, doc, debugOnscreenDebugEnabled, "debug", "onscreen_debug_enabled");

	Json::toString(doc, val, pretty);

	return true;
}

bool Exporter::Settings::fromString(const std::string &val) {
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	Jpath::get(doc, emulatorShowTitleBar, "emulator", "show_title_bar");
	Jpath::get(doc, emulatorShowStatusBar, "emulator", "show_status_bar");
	Jpath::get(doc, emulatorShowPreferenceDialogOnEscPress, "emulator", "show_preference_dialog_on_esc_press");

	Jpath::get(doc, deviceType, "device", "type");
	Jpath::get(doc, deviceSaveSramOnStop, "device", "save_sram_on_stop");
	for (int i = 0; i < GBBASIC_COUNTOF(deviceClassicPalette); ++i) {
		UInt32 col = 0;
		if (Jpath::get(doc, col, "device", "classic_palette", i))
			deviceClassicPalette[i] = Colour::byRGBA8888(col);
	}

	Jpath::get(doc, canvasFixRatio, "canvas", "fix_ratio");
	Jpath::get(doc, canvasIntegerScale, "canvas", "integer_scale");

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		Input::Gamepad &pad = inputGamepads[i];
		for (int j = 0; j < Input::BUTTON_COUNT; ++j) {
			unsigned dev = pad.buttons[j].device;
			Jpath::get(doc, dev, "input", "gamepad", i, j, "device");
			Jpath::get(doc, pad.buttons[j].index, "input", "gamepad", i, j, "index");
			unsigned type = pad.buttons[j].type;
			Jpath::get(doc, type, "input", "gamepad", i, j, "type");
			pad.buttons[j].type = (Input::Types)type;
			switch (pad.buttons[j].type) {
			case Input::VALUE:
				Jpath::get(doc, pad.buttons[j].value, "input", "gamepad", i, j, "value");

				break;
			case Input::HAT: {
					Jpath::get(doc, pad.buttons[j].hat.index, "input", "gamepad", i, j, "sub");
					unsigned short subType = pad.buttons[j].hat.value;
					Jpath::get(doc, subType, "input", "gamepad", i, j, "value");
					pad.buttons[j].hat.value = (Input::Hat::Types)subType;
				}

				break;
			case Input::AXIS:
				Jpath::get(doc, pad.buttons[j].axis.index, "input", "gamepad", i, j, "sub");
				Jpath::get(doc, pad.buttons[j].axis.value, "input", "gamepad", i, j, "value");

				break;
			}

			pad.buttons[j].device = (Input::Devices)dev;
		}
	}
	Jpath::get(doc, inputOnscreenGamepadEnabled, "input", "onscreen_gamepad", "enabled");
	Jpath::get(doc, inputOnscreenGamepadSwapAB, "input", "onscreen_gamepad", "swap_ab");
	Jpath::get(doc, inputOnscreenGamepadScale, "input", "onscreen_gamepad", "scale");
	Jpath::get(doc, inputOnscreenGamepadPadding.x, "input", "onscreen_gamepad", "padding", 0);
	Jpath::get(doc, inputOnscreenGamepadPadding.y, "input", "onscreen_gamepad", "padding", 1);

	Jpath::get(doc, debugOnscreenDebugEnabled, "debug", "onscreen_debug_enabled");

	return true;
}

Exporter::Exporter() {
	order(-1);
	isMajor(false);
	isMinor(false);
	messageEnabled(false);
	buildEnabled(true);
	packageArchived(false);
}

Exporter::~Exporter() {
}

unsigned Exporter::type(void) const {
	return TYPE();
}

bool Exporter::clone(Object** ptr) const { // Non-clonable.
	if (ptr)
		*ptr = nullptr;

	return false;
}

bool Exporter::open(const char* path_, const char* menu) {
	File::Ptr file(File::create());
	if (!file->open(path_, Stream::READ))
		return false;
	std::string content;
	if (!file->readString(content)) {
		file->close();

		return false;
	}
	file->close();
	file = nullptr;

	rapidjson::Document doc;
	if (!Json::fromString(doc, content.c_str()))
		return false;

	Localization::Dictionary title_;
	int order_ = 0;
	bool isMajor_ = false;
	bool isMinor_ = false;
	bool messageEnabled_ = false;
	std::string messageContent_;
	Text::Array extensions_;
	Text::Array filter_;
	bool buildEnabled_ = true;
	bool packageArchived_ = false;
	std::string packageTemplate_;
	std::string packageEntry_;
	std::string packageConfig_;
	std::string packageArgs_;
	std::string packageIcon_;
	std::string surfMethod_;
	std::string surfEntry_;
	Text::Array licenses_;

	rapidjson::Value* val = nullptr;
	if (!Jpath::get(doc, val, "title") || !val)
		return false;
	if (!Localization::parse(title_, *val))
		return false;

	if (!Jpath::get(doc, order_, "order"))
		order_ = 0;

	std::string frequentlyUsed;
	if (Jpath::get(doc, frequentlyUsed, "frequently_used")) {
		isMajor_ = frequentlyUsed == "major";
		isMinor_ = frequentlyUsed == "minor";
	} else {
		isMajor_ = false;
		isMinor_ = false;
	}

	if (!Jpath::get(doc, messageEnabled_, "message", "enabled"))
		messageEnabled_ = 0;

	if (!Jpath::get(doc, messageContent_, "message", "content"))
		messageContent_.clear();

	std::string tmp;
	int n = Jpath::count(doc, "extensions");
	if (n == 1) {
		if (!Jpath::get(doc, tmp, "extensions", 0))
			return false;
		extensions_.push_back(tmp);
		extensions_.push_back(tmp);
	} else if (n == 2) {
		for (int i = 0; i < 2; ++i) {
			if (!Jpath::get(doc, tmp, "extensions", i))
				return false;
			extensions_.push_back(tmp);
		}
	} else {
		return false;
	}

	n = Jpath::count(doc, "filter");
	if (n % 2)
		return false;
	for (int i = 0; i < n; ++i) {
		if (!Jpath::get(doc, tmp, "filter", i))
			return false;
		filter_.push_back(tmp);
	}

	if (Jpath::has(doc, "build", "enabled")) {
		if (!Jpath::get(doc, buildEnabled_, "build", "enabled"))
			return false;
	}

	if (Jpath::has(doc, "package", "archived")) {
		if (!Jpath::get(doc, packageArchived_, "package", "archived"))
			return false;
	}
	if (Jpath::has(doc, "package", "template")) {
		if (!Jpath::get(doc, packageTemplate_, "package", "template"))
			return false;
	}
	if (Jpath::has(doc, "package", "entry")) {
		if (!Jpath::get(doc, packageEntry_, "package", "entry"))
			return false;
	}
	if (Jpath::has(doc, "package", "config")) {
		if (!Jpath::get(doc, packageConfig_, "package", "config"))
			return false;
	}
	if (Jpath::has(doc, "package", "args")) {
		if (!Jpath::get(doc, packageArgs_, "package", "args"))
			return false;
	}
	if (Jpath::has(doc, "package", "icon")) {
		if (!Jpath::get(doc, packageIcon_, "package", "icon"))
			return false;
	}

	if (Jpath::has(doc, "surf", "method")) {
		if (!Jpath::get(doc, surfMethod_, "surf", "method"))
			return false;
	}
	if (Jpath::has(doc, "surf", "entry")) {
		if (!Jpath::get(doc, surfEntry_, "surf", "entry"))
			return false;
	}

	if (Jpath::has(doc, "licenses")) {
		n = Jpath::count(doc, "licenses");
		for (int i = 0; i < n; ++i) {
			if (!Jpath::get(doc, tmp, "licenses", i))
				return false;
			licenses_.push_back(tmp);
		}
	}

	path(path_);
	title(title_);
	order(order_);
	isMajor(isMajor_);
	isMinor(isMinor_);
	messageEnabled(messageEnabled_);
	messageContent(messageContent_);
	extensions(extensions_);
	filter(filter_);
	buildEnabled(buildEnabled_);
	packageArchived(packageArchived_);
	packageTemplate(packageTemplate_);
	packageEntry(packageEntry_);
	packageConfig(packageConfig_);
	packageArgs(packageArgs_);
	packageIcon(packageIcon_);
	surfMethod(surfMethod_);
	surfEntry(surfEntry_);
	licenses(licenses_);

	std::string entry_;
	const char* entrystr = Localization::get(title_);
	if (entrystr)
		entry_ = entrystr;
	entry_ = menu + std::string("/") + entry_;
	entry(Entry(entry_));

	return true;
}

bool Exporter::close(void) {
	path().clear();
	entry().clear();
	title().clear();
	order(0);
	isMajor(false);
	isMinor(false);
	messageEnabled(false);
	messageContent().clear();
	extensions().clear();
	filter().clear();
	buildEnabled(true);
	packageArchived(false);
	packageTemplate().clear();
	packageEntry().clear();
	packageConfig().clear();
	packageArgs().clear();
	packageIcon().clear();
	surfMethod().clear();
	surfEntry().clear();
	licenses().clear();

	return true;
}

bool Exporter::run(
	const char* path_, Bytes::Ptr rom, std::string* exported, std::string* hosted,
	const std::string &settings, const std::string &args, Bytes::Ptr icon,
	OutputHandler output
) const {
	// Prepare.
	GBBASIC_ASSERT(!messageEnabled() && "Impossible.");

	if (exported)
		exported->clear();
	if (hosted)
		hosted->clear();

	// Export package.
	if (!path_ || !*path_) {
		// Do nothing.
	} else if (packageArchived()) {
		std::string dir;
		Path::split(path(), nullptr, nullptr, &dir);
		const std::string srcPath = Path::combine(dir.c_str(), packageTemplate().c_str());
		if (!Path::copyFile(srcPath.c_str(), path_)) {
			const std::string msg = "Cannot initialize package \"" + std::string(path_) + "\".";
			output(msg, EXPORTER_ERROR);

			return false;
		}
		Archive::Ptr arc(Archive::create(Archive::ZIP));
		if (!arc->open(path_, Stream::APPEND)) {
			const std::string msg = "Cannot open package \"" + std::string(path_) + "\".";
			output(msg, EXPORTER_ERROR);

			return false;
		}
		if (!arc->fromBytes(rom.get(), packageEntry().c_str())) {
			const std::string msg = "Cannot write ROM to package \"" + std::string(path_) + "\".";
			output(msg, EXPORTER_ERROR);

			return false;
		}
		if (!settings.empty() && !packageConfig().empty()) {
			if (!arc->fromString(settings, packageConfig().c_str())) {
				const std::string msg = "Cannot write config to package \"" + std::string(path_) + "\".";
				output(msg, EXPORTER_WARN);
			}
		}
		if (!args.empty() && !packageArgs().empty()) {
			if (!arc->fromString(args, packageArgs().c_str())) {
				const std::string msg = "Cannot write args to package \"" + std::string(path_) + "\".";
				output(msg, EXPORTER_WARN);
			}
		}
		if (icon && !icon->empty() && !packageIcon().empty()) {
			if (!arc->fromBytes(icon.get(), packageIcon().c_str())) {
				const std::string msg = "Cannot write icon to package \"" + std::string(path_) + "\".";
				output(msg, EXPORTER_WARN);
			}
		}

		arc->close();
		if (exported)
			*exported = path_;
	} else if (buildEnabled()) {
		File::Ptr file(File::create());
		if (!file->open(path_, Stream::WRITE)) {
			const std::string msg = "Cannot open \"" + std::string(path_) + "\".";
			output(msg, EXPORTER_ERROR);

			return false;
		}
		if (file->writeBytes(rom.get()) == 0) {
			const std::string msg = "Cannot write to \"" + std::string(path_) + "\".";
			output(msg, EXPORTER_ERROR);

			return false;
		}
		if (exported)
			*exported = path_;
	}

	// Host the exported content.
	if (surfMethod() == "base64") {
		gotoBase64(path_, rom, hosted, output);
	} else if (surfMethod() == "web") {
		gotoWeb(path_, rom, hosted, output);
	} else if (surfMethod() == "url") {
		gotoUrl(path_, rom, hosted, output);
	} else if (surfMethod().empty()) {
		// Do nothing.
	} else {
		GBBASIC_ASSERT(false && "Not implemented.");
	}

	// Finish.
	return true;
}

void Exporter::reset(void) {
	if (_web) {
		_web->close();
		_web = nullptr;
	}
}

void Exporter::gotoBase64(const char* /* path_ */, Bytes::Ptr rom, std::string* hosted, OutputHandler output) const {
	Bytes::Ptr compressed(Bytes::create());
	if (!Lz4::fromBytes(compressed.get(), rom.get()))
		return;

	std::string txt;
	if (!Base64::fromBytes(txt, compressed.get()))
		return;

	const std::string url = Text::format(surfEntry(), { txt });
	if (hosted)
		*hosted = url;
	const std::string msg = "Hosting...";
	output(msg, EXPORTER_PRINT);
	Platform::surf(url.c_str());
}

void Exporter::gotoWeb(const char* path_, Bytes::Ptr /* rom */, std::string* hosted, OutputHandler output) const {
	const unsigned short port = 8081;
	_web = Web::Ptr(Web::create());

	const std::string pathstr = path_;
	Web::RequestedHandler::Callback func = std::bind( // Threaded.
		[pathstr] (std::string hostEntry, OutputHandler output, Web::RequestedHandler* self, const char* method, const char* uri, const char* /* query */, const char* /* body */, const Text::Dictionary &/* headers */) -> bool {
			auto getFullPath = [] (const std::string &hostEntry, const std::string &path_) -> std::string {
				if (hostEntry.empty())
					return path_;

				std::string ret = hostEntry;
				if (!Text::startsWith(path_, "/", false))
					ret += "/";
				ret += path_;

				return ret;
			};

			Web* web = (Web*)self->userdata().get();

			output(method + std::string(": ") + uri, EXPORTER_PRINT);
			if (strcmp(method, "GET") == 0) {
				std::string uri_ = uri;
				std::string name;
				std::string ext;
				Path::split(uri_, &name, &ext, nullptr);
				Text::toLowerCase(ext);

				if (uri_ == "/") {
					const std::string entry = getFullPath(hostEntry, "index.html");
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "text/html");
					arc->close();
				} else if (ext == "htm" || ext == "html") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "text/html");
					arc->close();
				} else if (ext == "css") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "text/css");
					arc->close();
				} else if (ext == "txt") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "text/plain");
					arc->close();
				} else if (ext == "json") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "application/json");
					arc->close();
				} else if (ext == "js") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "application/x-javascript");
					arc->close();
				} else if (ext == "wasm") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "application/wasm");
					arc->close();
				} else if (ext == "data") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "application/octet-stream");
					arc->close();
				} else if (ext == "gb" || ext == "gbc") {
					const std::string entry = getFullPath(hostEntry, uri_);
					Archive::Ptr arc(Archive::create(Archive::ZIP));
					arc->open(pathstr.c_str(), Stream::READ);
					Bytes::Ptr bytes(Bytes::create());
					arc->toBytes(bytes.get(), entry.c_str());
					bytes->poke(0);
					web->respond(bytes.get(), "application/octet-stream");
					arc->close();
				} else {
					const std::string msg = "Cannot find \"" + uri_ + "\".";
					output(msg, EXPORTER_WARN);

					web->respond(404);
				}
			} else {
				const std::string msg = "Method not allowed: " + std::string(method) + ".";
				output(msg, EXPORTER_WARN);

				web->respond(405);
			}

			return true;
		},
		surfEntry(), output, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6
	);
	Any ud(
		_web.get(),
		[] (void* /* ptr */) -> void {
			// Do nothing.
		}
	);
	Web::RequestedHandler cb(func, ud);
	_web->callback(cb);

	_web->open(port, nullptr);

	const std::string url = "http://127.0.0.1:" + Text::toString(port);
	if (hosted)
		*hosted = url;
	const std::string msg = "Hosting at " + url + "...";
	output(msg, EXPORTER_PRINT);
	Platform::surf(url.c_str());
}

void Exporter::gotoUrl(const char* /* path_ */, Bytes::Ptr /* rom */, std::string* hosted, OutputHandler output) const {
	const std::string &url = surfEntry();
	if (hosted)
		*hosted = url;
	const std::string msg = "Surfing...";
	output(msg, EXPORTER_PRINT);
	Platform::surf(url.c_str());
}

/* ===========================================================================} */
