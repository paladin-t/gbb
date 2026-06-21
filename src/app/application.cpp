/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "application.h"
#include "theme.h"
#include "workspace.h"
#include "../compiler/compiler.h"
#include "../utils/archive.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#include "../../lib/imgui_sdl/imgui_sdl.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>
#include <SDL_syswm.h>
#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#endif /* GBBASIC_OS_HTML */

/*
** {===========================================================================
** Macros and constants
*/

#if GBBASIC_MULTITHREAD_ENABLED
#	pragma message("Multithread enabled.")
#else /* GBBASIC_MULTITHREAD_ENABLED */
#	pragma message("Multithread disabled.")
#endif /* GBBASIC_MULTITHREAD_ENABLED */

#ifndef APPLICATION_ICON_FILE
#	define APPLICATION_ICON_FILE "../icon.png"
#endif /* APPLICATION_ICON_FILE */
#ifndef APPLICATION_ARGS_FILE
#	define APPLICATION_ARGS_FILE "../args.txt"
#endif /* APPLICATION_ARGS_FILE */

#ifndef APPLICATION_IDLE_FRAME_RATE
#	define APPLICATION_IDLE_FRAME_RATE 15
#endif /* APPLICATION_IDLE_FRAME_RATE*/
#ifndef APPLICATION_INPUTING_FRAME_RATE
#	define APPLICATION_INPUTING_FRAME_RATE 30
#endif /* APPLICATION_INPUTING_FRAME_RATE*/

#ifndef APPLICATION_ALWAYS_CAPTURE_MOUSE_WHEEL_EVENT
#	define APPLICATION_ALWAYS_CAPTURE_MOUSE_WHEEL_EVENT 1
#endif /* APPLICATION_ALWAYS_CAPTURE_MOUSE_WHEEL_EVENT */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

#if defined GBBASIC_OS_HTML
EM_JS(
	void, applicationFree, (void* ptr), {
		_free(ptr);
	}
);
EM_JS(
	const char*, applicationGetArgs, (), {
		let ret = '';
		if (typeof getArgs == 'function')
			ret = getArgs();
		if (ret == null)
			ret = '';
		const lengthBytes = lengthBytesUTF8(ret) + 1;
		const stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes);

		return stringOnWasmHeap;
	}
);
#else /* GBBASIC_OS_HTML */
static void applicationFree(void*) {
	// Do nothing.
}
static const char* applicationGetArgs(void) {
	return nullptr;
}
#endif /* GBBASIC_OS_HTML */

static void applicationParseArgs(int argc, const char* argv[], Text::Dictionary &options) {
	if (argc == 0 || !argv)
		return;

	int i = 0;
	while (i < argc) {
		const char* arg = argv[i];
		if (*arg == '-') {
			std::string key, val;
			key = arg + 1;
			if (i + 1 < argc) {
				const char* data = argv[i + 1];
				if (*data != '-') {
					val = data;
					if (val.front() == '"' && val.back() == '"') {
						val.erase(val.begin());
						val.pop_back();
					}
					++i;
				}
			}
			options[key] = val;
		} else if (*arg == '\0') {
			// Do nothing.
		} else {
			std::string val = arg;
			if (val.front() == '"' && val.back() == '"') {
				val.erase(val.begin());
				val.pop_back();
			}
			options[""] = val;
		}
		++i;
	}
}
static void applicationLoadArgsFile(const char* path, Text::Dictionary &options) {
	typedef std::vector<const char*> Argv;

	File::Ptr file(File::create());
	if (!file->open(path, Stream::READ))
		return;

	std::string buf;
	file->readString(buf);
	file->close();

	Text::Array args = Text::split(buf, " ");
	Argv argv;
	for (std::string &a : args) {
		a = Text::trim(a);
		if (!a.empty())
			argv.push_back(a.c_str());
	}
	const int argc = (int)argv.size();

	if (argc > 0)
		applicationParseArgs(argc, &argv.front(), options);
}
static void applicationLoadArgsString(const char* str, Text::Dictionary &options) {
	typedef std::vector<const char*> Argv;

	if (!str)
		return;

	const std::string buf = str;

	Text::Array args = Text::split(buf, " ");
	Argv argv;
	for (std::string &a : args) {
		a = Text::trim(a);
		if (!a.empty())
			argv.push_back(a.c_str());
	}
	const int argc = (int)argv.size();

	if (argc > 0)
		applicationParseArgs(argc, &argv.front(), options);
}

typedef std::function<void(Text::Dictionary::const_iterator)> ArgSetter;
static bool applicationGetArgValue(Text::Dictionary &options, const char* key, ArgSetter setArg) {
	Text::Dictionary::const_iterator opt = options.find(key);
	if (opt != options.end()) {
		setArg(opt);

		return true;
	}

	return false;
}
template<typename T> static bool applicationGetArgValue(Text::Dictionary &options, const char* key, T &var, const T &val) {
	Text::Dictionary::const_iterator opt = options.find(key);
	if (opt != options.end()) {
		var = val;

		return true;
	}

	return false;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Custom events
*/

struct EventTypes {
	Uint32 BEGIN = 0;
	Uint32 END = 0;

	EventTypes() {
		BEGIN = 0;
		END = 0;
	}
};

static EventTypes EVENTS;

static void applicationSetupEventTypes(void) {
	const int n = (int)Workspace::ExternalEventTypes::COUNT;
	if (n == 0)
		return;

	const Uint32 ret = SDL_RegisterEvents(n);
	if (ret == (Uint32)-1)
		return;

	EVENTS.BEGIN = ret;
	EVENTS.END = ret + n - 1;
}
static void applicationClearEventTypes(void) {
	EVENTS = EventTypes();
}

/* ===========================================================================} */

/*
** {===========================================================================
** Application
*/

class Application : public NonCopyable {
private:
	struct Context {
		unsigned expectedFrameRate = GBBASIC_ACTIVE_FRAME_RATE;
		unsigned updatedFrameCount = 0;
		double updatedSeconds = 0;
		unsigned fps = 0;

		double delta = 0.0;

		std::string clipboardTextData;

		bool mouseCursorIndicated = false;
		SDL_Cursor* mouseCursors[ImGuiMouseCursor_COUNT] = { };
		bool mousePressed[3] = { false, false, false };
		ImVec2 mousePosition;
		bool mouseCanUseGlobalState = true;

		bool imeCompositing = false;
	};

private:
	bool _opened = false;
	Text::Dictionary _options;
	bool _commandlineOnly = false;
	bool _toUpgrade = false;
	bool _toCompile = false;

	Window* _window = nullptr;
	Renderer* _renderer = nullptr;
	long long _stamp = 0;
	Window::Orientations _orientation = Window::UNKNOWN;

	Workspace* _workspace = nullptr;

	Context _context;

public:
	Application(Workspace* workspace) : _workspace(workspace) {
	}
	~Application() {
		delete _workspace;
		_workspace = nullptr;
	}

	bool open(const Text::Dictionary &options) {
		// Prepare.
		if (_opened)
			return false;
		_opened = true;

		_options = options;

		// Parse the options.
		applicationGetArgValue(_options, WORKSPACE_OPTION_APPLICATION_COMMANDLINE_ONLY_KEY, _commandlineOnly, true);
		std::string toUpgradeToVersion;
		applicationGetArgValue(
			_options, WORKSPACE_OPTION_APPLICATION_UPGRADE_ONLY_KEY,
			[&] (Text::Dictionary::const_iterator opt) -> void {
				_toUpgrade = true;
				toUpgradeToVersion = opt->second;
			}
		);
		applicationGetArgValue(_options, COMPILER_OUTPUT_OPTION_KEY, _toCompile, true);

		// Initialize the platform.
		Platform::open();

		// Initialize the window and renderer.
		bool explicitWndSize = false;
		int wndWidth = GBBASIC_WINDOW_MIN_WIDTH * 3;
		int wndHeight = GBBASIC_WINDOW_MIN_HEIGHT * 3;
		bool fullscreen = false;
		bool maximized = false;
		if (!_commandlineOnly) {
			int scale = 2; // Defaults to 2.
			applicationGetArgValue(_options, WORKSPACE_OPTION_RENDERER_X1_KEY, scale, 1) ||
			applicationGetArgValue(_options, WORKSPACE_OPTION_RENDERER_X2_KEY, scale, 2) ||
			applicationGetArgValue(_options, WORKSPACE_OPTION_RENDERER_X3_KEY, scale, 3) ||
			applicationGetArgValue(_options, WORKSPACE_OPTION_RENDERER_X4_KEY, scale, 4) ||
			applicationGetArgValue(_options, WORKSPACE_OPTION_RENDERER_X5_KEY, scale, 5) ||
			applicationGetArgValue(_options, WORKSPACE_OPTION_RENDERER_X6_KEY, scale, 6);

#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
			bool borderless = false;
			bool highDpi = true;
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_BORDERLESS_ENABLED_KEY, borderless, true);
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY, highDpi, false);
			SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1", SDL_HINT_OVERRIDE);
#elif defined GBBASIC_OS_IOS || defined GBBASIC_OS_ANDROID
			const bool borderless = true;
			const bool highDpi = false;
			SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1", SDL_HINT_OVERRIDE);
#elif defined GBBASIC_OS_HTML
			bool borderless = false;
			bool highDpi = true;
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_BORDERLESS_ENABLED_KEY, borderless, true);
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY, highDpi, false);
			SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
#else /* Platform macro. */
			bool borderless = false;
			bool highDpi = true;
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_BORDERLESS_ENABLED_KEY, borderless, true);
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY, highDpi, false);
			SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
			SDL_SetHintWithPriority(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1", SDL_HINT_OVERRIDE);
#endif /* Platform macro. */
#ifdef SDL_HINT_IME_SHOW_UI
			SDL_SetHintWithPriority(SDL_HINT_IME_SHOW_UI, "1", SDL_HINT_OVERRIDE);
#endif /* SDL_HINT_IME_SHOW_UI */

			int moditorIndex = 0;
			int wndX = -1;
			int wndY = -1;
#if GBBASIC_DISPLAY_AUTO_SIZE_ENABLED
			const int minWndWidth = GBBASIC_WINDOW_MIN_HEIGHT;
			const int minWndHeight = GBBASIC_WINDOW_MIN_HEIGHT;
#else /* GBBASIC_DISPLAY_AUTO_SIZE_ENABLED*/
			const int minWndWidth = GBBASIC_WINDOW_MIN_WIDTH * scale;
			const int minWndHeight = GBBASIC_WINDOW_MIN_HEIGHT * scale;
#endif /* GBBASIC_DISPLAY_AUTO_SIZE_ENABLED */

#if !defined GBBASIC_OS_HTML
			const std::string pref = Path::writableDirectory();
			const std::string path = Path::combine(pref.c_str(), WORKSPACE_PREFERENCES_NAME ".json");
			rapidjson::Document doc;
			File::Ptr file(File::create());
			if (file->open(path.c_str(), Stream::READ)) {
				std::string buf;
				file->readString(buf);
				file->close();
				if (!Json::fromString(doc, buf.c_str()))
					doc.SetNull();
			}
			file = nullptr;
			if (!Jpath::get(doc, moditorIndex, "application", "window", "display_index"))
				moditorIndex = 0;
			if (!Jpath::get(doc, fullscreen, "application", "window", "fullscreen"))
				fullscreen = false;
			if (!Jpath::get(doc, maximized, "application", "window", "maximized"))
				maximized = false;
			if (!Jpath::get(doc, wndX, "application", "window", "position", 0))
				wndX = -1;
			if (!Jpath::get(doc, wndY, "application", "window", "position", 1))
				wndY = -1;
			if (!Jpath::get(doc, wndWidth, "application", "window", "size", 0))
				wndWidth = GBBASIC_WINDOW_MIN_WIDTH * 3;
			if (!Jpath::get(doc, wndHeight, "application", "window", "size", 1))
				wndHeight = GBBASIC_WINDOW_MIN_HEIGHT * 3;

			Text::Dictionary::const_iterator sizeOpt = options.find(WORKSPACE_OPTION_WINDOW_SIZE_KEY);
			if (sizeOpt != options.end()) {
				const std::string sizeStr = sizeOpt->second;
				const Text::Array sizeArr = Text::split(sizeStr, "x");

				do {
					if (sizeArr.size() != 2)
						break;
					int w = 0, h = 0;
					if (!Text::fromString(sizeArr[0], w) || !Text::fromString(sizeArr[1], h))
						break;
					if (w < minWndWidth || h < minWndHeight)
						break;

					wndWidth = w;
					wndHeight = h;
					fullscreen = false;
					maximized = false;

					explicitWndSize = true;
				} while (false);
			}
#endif /* GBBASIC_OS_HTML */

			const bool opengl = false;
			bool alwaysOnTop = false;
			applicationGetArgValue(_options, WORKSPACE_OPTION_WINDOW_ALWAYS_ON_TOP_ENABLED_KEY, alwaysOnTop, true);
			_window = Window::create();
			_window->open(
				GBBASIC_TITLE " v" GBBASIC_VERSION_STRING,
				moditorIndex,
				wndX, wndY,
				wndWidth, wndHeight,
				minWndWidth, minWndHeight,
				borderless,
				highDpi, opengl,
				alwaysOnTop
			);
			if (Platform::isSystemInDarkMode())
				Platform::useDarkMode(_window);

			int driver = 0;
			applicationGetArgValue(
				_options, WORKSPACE_OPTION_RENDERER_DRIVER_KEY,
				[&] (Text::Dictionary::const_iterator opt) -> void {
					const std::string str = opt->second;
					Text::fromString(str, driver);
				}
			);
			_renderer = Renderer::create();
			_renderer->open(_window, !!driver);

			int wndScale = 1;
			if (highDpi) {
				const int wndW = _window->width();
				const int rndW = _renderer->width();
				if (wndW > 0 && rndW > wndW) { // Happens on MacOS, iOS with retina display.
					wndScale *= rndW / wndW;
					if (wndScale <= 0)
						wndScale = 1;
				}
			}

			if (scale != 1 || wndScale != 1)
				_renderer->scale(scale * wndScale); // Scale the renderer.
			if (wndScale != 1)
				_window->scale(wndScale); // Scale the window (on high-DPI monitor).
		}

		// Initialize the FPS.
		unsigned fps = _context.expectedFrameRate;
		applicationGetArgValue(
			_options, WORKSPACE_OPTION_APPLICATION_FPS_KEY,
			[&] (Text::Dictionary::const_iterator opt) -> void {
				const std::string str = opt->second;
				if (Text::fromString(str, fps) && fps >= 1)
					_context.expectedFrameRate = fps;
				else
					fps = _context.expectedFrameRate;
			}
		);

		// Initialize the home screen mode.
#if defined GBBASIC_OS_HTML
		constexpr const bool showRecent = false; // Always use the nodepad mode for HTML.
#else /* Platform macro. */
		bool showRecent = true;
		applicationGetArgValue(_options, WORKSPACE_OPTION_APPLICATION_NOTEPAD_MODE_KEY, showRecent, false);
#endif /* Platform macro. */

		// Initialize the icon.
		if (!_commandlineOnly && Path::fileExists(APPLICATION_ICON_FILE)) {
			File::Ptr file(File::create());
			if (file->open(APPLICATION_ICON_FILE, Stream::READ)) {
				Bytes::Ptr bytes(Bytes::create());
				file->readBytes(bytes.get());
				file->close();

				Image::Ptr img(Image::create());
				if (img->fromBytes(bytes.get())) {
					SDL_Window* wnd = (SDL_Window*)_window->pointer();
					SDL_Surface* sur = (SDL_Surface*)img->pointer();
					SDL_SetWindowIcon(wnd, sur);
				}
			}
		}

		// Initialize the randomizer.
		Math::srand();

		// Initialize the timestamp.
		_stamp = DateTime::ticks();

		// Initialize the GUI system.
		if (!_commandlineOnly) {
			openImGui();
		}

		// Initialize the workspace.
		bool hideSplash = false;
		applicationGetArgValue(_options, WORKSPACE_OPTION_APPLICATION_HIDE_SPLASH_KEY, hideSplash, true);

		bool forceWritable = false;
		applicationGetArgValue(_options, WORKSPACE_OPTION_APPLICATION_FORCE_WRITABLE_KEY, forceWritable, true);

		std::string font;
		applicationGetArgValue(
			_options, COMPILER_FONT_OPTION_KEY,
			[&] (Text::Dictionary::const_iterator opt) -> void {
				font = opt->second;
			}
		);

		_workspace->open(_window, _renderer, font.empty() ? nullptr : font.c_str(), fps, showRecent, hideSplash, forceWritable, _commandlineOnly, _toUpgrade ? toUpgradeToVersion.c_str() : nullptr, _toCompile);

		if (!_commandlineOnly) {
			if (explicitWndSize)
				_workspace->load(_window, _renderer, &wndWidth, &wndHeight);
			else
				_workspace->load(_window, _renderer, nullptr, nullptr);

			if (fullscreen)
				_window->fullscreen(true);
			else if (maximized)
				_window->maximize();
		}

		const std::string writableDir = Path::writableDirectory();
		if (writableDir.empty()) {
#if defined BITTY_OS_WIN
			const std::string possiblePath = "\nDesired path: \"%USERPROFILE%\\AppData\\Roaming\\gbbasic\\data\".";
#elif defined BITTY_OS_MAC
			const std::string possiblePath = "\nDesired path: \"~/Library/Application Support/gbbasic/data\".";
#elif defined BITTY_OS_LINUX
			const std::string possiblePath = "\nDesired path: \"~/.local/share/gbbasic/data\".";
#else /* Platform macro. */
			const std::string possiblePath = "";
#endif /* Platform macro. */
			std::string errorMsg = SDL_GetError();
			if (_commandlineOnly)
				errorMsg = Unicode::toOs(errorMsg);
			const std::string possibleError = "\nError:\n  " + errorMsg;
			const std::string msg =
				"Failed to get the path of a writable directory. May not have permissions." +
				possiblePath +
				possibleError;

			if (_commandlineOnly) {
				Platform::msgbox(msg.c_str(), "Warning");
			} else {
				_workspace->messagePopupBox(msg, nullptr, nullptr, nullptr);
			}
		}

		// Finish.
		return true;
	}
	bool close(void) {
		// Prepare.
		if (!_opened)
			return false;
		_opened = false;

		// Dispose the workspace.
		if (!_commandlineOnly) {
			_workspace->save(_window, _renderer);
		}

		_workspace->close(_window, _renderer, _commandlineOnly);

		// Dispose the GUI system.
		if (!_commandlineOnly) {
			closeImGui();
		}

		// Dispose the timestamp.
		_stamp = 0;

		// Dispose the window and renderer.
		if (!_commandlineOnly) {
			if (_renderer) {
				_renderer->close();
				Renderer::destroy(_renderer);
				_renderer = nullptr;
			}
			if (_window) {
				_window->close();
				Window::destroy(_window);
				_window = nullptr;
			}
		}

		// Dispose the platform.
		Platform::close();

		// Finish.
		return true;
	}

	bool update(void) {
		if (_commandlineOnly) { // Command line only, without window.
			_workspace->compile(nullptr, nullptr, nullptr, _options);

			return false;
		} else if (_toUpgrade) { // With window, but upgrade project only.
			_toUpgrade = false;

			_workspace->upgrade(_window, _renderer, _options);
		} else if (_toCompile) { // With window, but compile only.
			_toCompile = false;

			_workspace->compile(_window, _renderer, nullptr, _options);
		}

		_context.updatedSeconds += _context.delta;
		if (++_context.updatedFrameCount >= _context.expectedFrameRate * 3) { // 3 seconds.
			if (_context.updatedSeconds > 0)
				_context.fps = (unsigned)(_context.updatedFrameCount / _context.updatedSeconds);
			else
				_context.fps = 0;
			_context.expectedFrameRate = APPLICATION_IDLE_FRAME_RATE;
			_context.updatedFrameCount = 0;
			_context.updatedSeconds = 0;
		}

		const long long begin = DateTime::ticks();
		_context.delta = begin >= _stamp ? DateTime::toSeconds(begin - _stamp) : 0;
		_stamp = begin;

		const bool alive = updateImGui(_context.delta, _context.mouseCursorIndicated);

		const ImU32 integer = _workspace->theme()->style()->screenClearingColor;
		const Colour cls(integer & 0x000000ff, (integer >> 8) & 0x000000ff, (integer >> 16) & 0x000000ff, (integer >> 24) & 0x000000ff);
		_renderer->target(nullptr);
		_renderer->clip(0, 0, _renderer->width(), _renderer->height());
		_renderer->clear(&cls);
		bool executing = false;
		{
			ImGui::NewFrame();

			_context.mouseCursorIndicated = false;
			executing = _workspace->update(
				_window, _renderer,
				_context.delta, _context.fps,
				&_context.expectedFrameRate,
				&_context.mouseCursorIndicated
			);

			ImGui::Render();

			ImGuiSDL::Render(ImGui::GetDrawData());

			_workspace->record(_window, _renderer);
		}
		if (!_workspace->skipping())
			_renderer->flush();
		_window->update();

		if (executing) {
			DateTime::sleep(0);
		} else {
			const long long end = DateTime::ticks();
			const long long diff = end >= begin ? end - begin : 0;
			const double elapsed = DateTime::toSeconds(diff);
			const double expected = 1.0 / _context.expectedFrameRate;
			const double rest = expected - elapsed;
			if (rest > 0)
				DateTime::sleep((int)(rest * 1000));
		}

		return alive;
	}

	bool sendWithAppEvents(SDL_Event* event) {
#if defined GBBASIC_OS_IOS || defined GBBASIC_OS_ANDROID
		switch (event->type) {
		case SDL_APP_TERMINATING:
			fprintf(stdout, "SDL: SDL_APP_TERMINATING.\n");

			_primitives->forbid();
			_executable->appTerminating();

			return true;
		case SDL_APP_LOWMEMORY:
			fprintf(stdout, "SDL: SDL_APP_LOWMEMORY.\n");

			return false;
		case SDL_APP_WILLENTERBACKGROUND:
			fprintf(stdout, "SDL: SDL_APP_WILLENTERBACKGROUND.\n");

			return false;
		case SDL_APP_DIDENTERBACKGROUND:
			fprintf(stdout, "SDL: SDL_APP_DIDENTERBACKGROUND.\n");

			return false;
		case SDL_APP_WILLENTERFOREGROUND:
			fprintf(stdout, "SDL: SDL_APP_WILLENTERFOREGROUND.\n");

			return false;
		case SDL_APP_DIDENTERFOREGROUND:
			fprintf(stdout, "SDL: SDL_APP_DIDENTERFOREGROUND.\n");

			return false;
		}
#elif defined GBBASIC_OS_HTML
		if (event->type >= EVENTS.BEGIN && event->type <= EVENTS.END) {
			_workspace->sendExternalEvent(_window, _renderer, (Workspace::ExternalEventTypes)(event->type - EVENTS.BEGIN), event);

			return true;
		} else {
			return false;
		}
#else /* Platform macro. */
		(void)event;
#endif /* Platform macro. */

		return false;
	}

private:
	void openImGui(void) {
		SDL_Window* wnd = (SDL_Window*)_window->pointer();
		SDL_Renderer* rnd = (SDL_Renderer*)_renderer->pointer();

		ImGui::CreateContext();
		ImGuiSDL::Initialize(rnd, GBBASIC_WINDOW_DEFAULT_WIDTH, GBBASIC_WINDOW_DEFAULT_HEIGHT);

		ImGuiIO &io = ImGui::GetIO();

		io.IniFilename = nullptr;

		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
		io.BackendFlags |= ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;

		io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
		io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
		io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
		io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
		io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
		io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
		io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
		io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
		io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
		io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
		io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
		io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
		io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
		io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
		io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

		io.SetClipboardTextFn = [] (void* userdata, const char* text) -> void {
			Context* data = (Context*)userdata;
			(void)data;

			const std::string osstr = Unicode::toOs(text);

			Platform::setClipboardText(osstr.c_str());
		};
		io.GetClipboardTextFn = [] (void* userdata) -> const char* {
			Context* data = (Context*)userdata;

			if (!data->clipboardTextData.empty())
				data->clipboardTextData.clear();

			const std::string osstr = Platform::getClipboardText();

			data->clipboardTextData = Unicode::fromOs(osstr);

			return data->clipboardTextData.c_str();
		};
		io.ClipboardUserData = &_context;

		_context.mouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		_context.mouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
		_context.mouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		_context.mouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
		_context.mouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
		_context.mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
		_context.mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
		_context.mouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		_context.mouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
		_context.mouseCursors[ImGuiMouseCursor_Wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);

		_context.mouseCanUseGlobalState = !!strncmp(SDL_GetCurrentVideoDriver(), "wayland", 7);

		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(wnd, &wmInfo);
#if defined GBBASIC_OS_WIN
		const HWND hwnd = wmInfo.info.win.window;
		io.ImeWindowHandle = hwnd;

		portableFileDialogsActiveWindow = hwnd;
#elif defined GBBASIC_OS_MAC
		// Do nothing.
#elif defined GBBASIC_OS_LINUX
		// Do nothing.
#else /* Platform macro. */
		(void)wnd;

		io.ImeSetInputScreenPosFn = Platform::inputScreenPosition;
#endif /* GBBASIC_OS_WIN */

		fprintf(stdout, "ImGui opened.\n");
	}
	void closeImGui(void) {
		ImGuiIO &io = ImGui::GetIO();

#if defined GBBASIC_OS_WIN
		(void)io;
#else /* GBBASIC_OS_WIN */
		io.ImeSetInputScreenPosFn = nullptr;
#endif /* GBBASIC_OS_WIN */

		_context.clipboardTextData.clear();

		for (ImGuiMouseCursor i = 0; i < ImGuiMouseCursor_COUNT; ++i)
			SDL_FreeCursor(_context.mouseCursors[i]);
		memset(_context.mouseCursors, 0, sizeof(_context.mouseCursors));

		ImGuiSDL::Deinitialize();
		ImGui::DestroyContext();

		fprintf(stdout, "ImGui closed.\n");
	}

	bool updateImGui(double delta, bool mouseCursorIndicated) {
		// Prepare.
		SDL_Window* wnd = (SDL_Window*)_window->pointer();

		ImGuiIO &io = ImGui::GetIO();

		bool alive = true;
		bool reset = false;

		// Poll events.
		SDL_Event evt;
		while (SDL_PollEvent(&evt)) {
			bool ignore = false;

			switch (evt.type) {
			case SDL_QUIT:
#if !defined GBBASIC_OS_HTML
				fprintf(stdout, "SDL: SDL_QUIT.\n");
#endif /* GBBASIC_OS_HTML */

				alive = !_workspace->quit(_window, _renderer);

				break;
			case SDL_WINDOWEVENT:
				switch (evt.window.event) {
				case SDL_WINDOWEVENT_RESIZED: {
						const int w = evt.window.data1, h = evt.window.data2;

#if !defined GBBASIC_OS_HTML
						fprintf(stdout, "SDL: SDL_WINDOWEVENT_RESIZED [%d, %d].\n", w, h);
#endif /* GBBASIC_OS_HTML */

						int nl = 0, nt = 0, nr = 0, nb = 0;
						Platform::notch(&nl, &nt, &nr, &nb);
						const Math::Recti notch(nl, nt, nr, nb);
						const Window::Orientations orientation = _window->orientation();
						_workspace->resized(
							_window, _renderer,
							Math::Vec2i(w, h),
							Math::Vec2i(
								w / _renderer->scale(),
								h / _renderer->scale()
							),
							notch,
							orientation
						);
					}

					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
#if !defined GBBASIC_OS_HTML
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_SIZE_CHANGED.\n");
#endif /* GBBASIC_OS_HTML */

					reset = true;

					break;
				case SDL_WINDOWEVENT_MOVED: {
#if !defined GBBASIC_OS_HTML
						// fprintf(stdout, "SDL: SDL_WINDOWEVENT_MOVED.\n");
#endif /* GBBASIC_OS_HTML */

						float ddpi = 0, hdpi = 0, vdpi = 0;
						if (_window->dpi(&ddpi, &hdpi, &vdpi))
							_workspace->dpi(_window, _renderer, Math::Vec3f(ddpi, hdpi, vdpi));

						const int x = evt.window.data1, y = evt.window.data2;
						_workspace->moved(_window, _renderer, Math::Vec2i(x, y));
					}

					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
#if !defined GBBASIC_OS_HTML
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_MAXIMIZED.\n");
#endif /* GBBASIC_OS_HTML */

					_workspace->maximized(_window, _renderer);

					break;
				case SDL_WINDOWEVENT_RESTORED:
#if !defined GBBASIC_OS_HTML
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_RESTORED.\n");
#endif /* GBBASIC_OS_HTML */

					_workspace->restored(_window, _renderer);

					reset = true;

					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
#if !defined GBBASIC_OS_HTML
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_FOCUS_GAINED.\n");
#endif /* GBBASIC_OS_HTML */

					_workspace->focusGained(_window, _renderer);

					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
#if !defined GBBASIC_OS_HTML
					fprintf(stdout, "SDL: SDL_WINDOWEVENT_FOCUS_LOST.\n");
#endif /* GBBASIC_OS_HTML */

					_workspace->focusLost(_window, _renderer);

					break;
				}

				break;
			case SDL_DROPFILE:
				if (evt.drop.file) {
#if !defined GBBASIC_OS_HTML
					fprintf(stdout, "SDL: SDL_DROPFILE.\n");
#endif /* GBBASIC_OS_HTML */

					_workspace->fileDropped(_window, _renderer, evt.drop.file);

					SDL_free(evt.drop.file);
				}

				break;
			case SDL_DROPBEGIN:
#if !defined GBBASIC_OS_HTML
				fprintf(stdout, "SDL: SDL_DROPBEGIN.\n");
#endif /* GBBASIC_OS_HTML */

				_workspace->dropBegan(_window, _renderer);

				break;
			case SDL_DROPCOMPLETE:
#if !defined GBBASIC_OS_HTML
				fprintf(stdout, "SDL: SDL_DROPCOMPLETE.\n");
#endif /* GBBASIC_OS_HTML */

				_workspace->dropEnded(_window, _renderer);

				break;
			case SDL_RENDER_TARGETS_RESET:
#if !defined GBBASIC_OS_HTML
				fprintf(stdout, "SDL: SDL_RENDER_TARGETS_RESET.\n");
#endif /* GBBASIC_OS_HTML */

				_workspace->renderTargetsReset(_window, _renderer);

				reset = true;

				break;
			case SDL_RENDER_DEVICE_RESET:
#if !defined GBBASIC_OS_HTML
				fprintf(stdout, "SDL: SDL_RENDER_DEVICE_RESET.\n");
#endif /* GBBASIC_OS_HTML */

				break;
			case SDL_MOUSEWHEEL:
				if (evt.wheel.x > 0)
					io.MouseWheelH += 1;
				if (evt.wheel.x < 0)
					io.MouseWheelH -= 1;
				if (evt.wheel.y > 0)
					io.MouseWheel += 1;
				if (evt.wheel.y < 0)
					io.MouseWheel -= 1;

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);

				break;
			case SDL_MOUSEBUTTONDOWN:
				if (evt.button.button == SDL_BUTTON_LEFT)
					_context.mousePressed[0] = true;
				if (evt.button.button == SDL_BUTTON_RIGHT)
					_context.mousePressed[1] = true;
				if (evt.button.button == SDL_BUTTON_MIDDLE)
					_context.mousePressed[2] = true;

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);

				break;
			case SDL_KEYDOWN: // Fall through.
			case SDL_KEYUP: {
					const SDL_Keymod mod = SDL_GetModState();
					int key = evt.key.keysym.scancode;
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_LINUX
					if (key == SDL_SCANCODE_KP_ENTER) {
						key = SDL_SCANCODE_RETURN;
					} else {
						if (mod & KMOD_NUM) {
							switch (key) {
							case SDL_SCANCODE_KP_PERIOD:
								key = SDL_SCANCODE_PERIOD;

								break;
							case SDL_SCANCODE_KP_0:
								key = SDL_SCANCODE_0;

								break;
							case SDL_SCANCODE_KP_1:
								key = SDL_SCANCODE_1;

								break;
							case SDL_SCANCODE_KP_2:
								key = SDL_SCANCODE_2;

								break;
							case SDL_SCANCODE_KP_3:
								key = SDL_SCANCODE_3;

								break;
							case SDL_SCANCODE_KP_4:
								key = SDL_SCANCODE_4;

								break;
							case SDL_SCANCODE_KP_5:
								key = SDL_SCANCODE_5;

								break;
							case SDL_SCANCODE_KP_6:
								key = SDL_SCANCODE_6;

								break;
							case SDL_SCANCODE_KP_7:
								key = SDL_SCANCODE_7;

								break;
							case SDL_SCANCODE_KP_8:
								key = SDL_SCANCODE_8;

								break;
							case SDL_SCANCODE_KP_9:
								key = SDL_SCANCODE_9;

								break;
							default:
								// Do nothing.

								break;
							}
						} else {
							switch (key) {
							case SDL_SCANCODE_KP_PERIOD:
								key = SDL_SCANCODE_DELETE;

								break;
							case SDL_SCANCODE_KP_1:
								key = SDL_SCANCODE_END;

								break;
							case SDL_SCANCODE_KP_3:
								key = SDL_SCANCODE_PAGEDOWN;

								break;
							case SDL_SCANCODE_KP_7:
								key = SDL_SCANCODE_HOME;

								break;
							case SDL_SCANCODE_KP_9:
								key = SDL_SCANCODE_PAGEUP;

								break;
							case SDL_SCANCODE_KP_2:
								key = SDL_SCANCODE_DOWN;

								break;
							case SDL_SCANCODE_KP_4:
								key = SDL_SCANCODE_LEFT;

								break;
							case SDL_SCANCODE_KP_6:
								key = SDL_SCANCODE_RIGHT;

								break;
							case SDL_SCANCODE_KP_8:
								key = SDL_SCANCODE_UP;

								break;
							default:
								// Do nothing.

								break;
							}
						}
					}
#endif /* Platform macro. */
					GBBASIC_ASSERT(key >= 0 && key < GBBASIC_COUNTOF(io.KeysDown));
					if (!_context.imeCompositing) {
						io.KeysDown[key] = evt.type == SDL_KEYDOWN;
#if defined GBBASIC_OS_IOS || defined GBBASIC_OS_ANDROID
						if (evt.type == SDL_KEYDOWN)
							ignore = true;
#endif /* Platform macro. */
						if (evt.type == SDL_KEYUP)
							_workspace->stroke(key);
					}
					io.KeyShift = !!(mod & KMOD_SHIFT);
					if (!!(mod & KMOD_LCTRL) && !(mod & KMOD_RCTRL) && !(mod & KMOD_LALT) && !!(mod & KMOD_RALT)) {
						// FIXME: this is not a perfect solution, just works, to avoid some
						// unexpected key events, eg. RAlt+A on Poland keyboard also triggers
						// LCtrl.
						io.KeyCtrl = false;
					} else {
						io.KeyCtrl = !!(mod & KMOD_CTRL);
					}
					io.KeyAlt = !!(mod & KMOD_ALT);
#if defined GBBASIC_OS_WIN
					io.KeySuper = false;
#else /* GBBASIC_OS_WIN */
					io.KeySuper = !!(mod & KMOD_GUI);
#endif /* GBBASIC_OS_WIN */

					if (!_context.imeCompositing && evt.type == SDL_KEYDOWN && key == SDL_SCANCODE_BACKSPACE) {
						char buf[2];
						buf[0] = (char)SDLK_BACKSPACE;
						buf[1] = '\0';
						_workspace->textInput(_window, _renderer, buf);
					}

					requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);
				}

				break;
			case SDL_TEXTINPUT:
				io.AddInputCharactersUTF8(evt.text.text);

				_workspace->textInput(_window, _renderer, evt.text.text);

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);

				break;
			case SDL_SYSWMEVENT: {
#if defined GBBASIC_OS_WIN
					auto winMsg = evt.syswm.msg->msg.win;
					switch (winMsg.msg) {
					case WM_IME_STARTCOMPOSITION:
						_context.imeCompositing = true;

						break;
					case WM_IME_ENDCOMPOSITION:
						_context.imeCompositing = false;

						break;
					default:
						break;
					}
#endif /* GBBASIC_OS_WIN */
				}

				break;
			default: // Do nothing.
				break;
			}

			if (ignore)
				break;
		}

		// Orientation changed.
		do {
			const Window::Orientations orientation = _window->orientation();
			if (_orientation == orientation)
				break;

#if !defined GBBASIC_OS_HTML
			fprintf(stdout, "SDL: SDL_ORIENTATION_CHANGED.\n");
#endif /* GBBASIC_OS_HTML */

			_orientation = orientation;
			int w = 0, h = 0;
			SDL_GetWindowSize(wnd, &w, &h);
			int nl = 0, nt = 0, nr = 0, nb = 0;
			Platform::notch(&nl, &nt, &nr, &nb);
			const Math::Recti notch(nl, nt, nr, nb);
			_workspace->resized(
				_window, _renderer,
				Math::Vec2i(w, h),
				Math::Vec2i(
					w / _renderer->scale(),
					h / _renderer->scale()
				),
				notch,
				orientation
			);
		} while (false);

		// Reset render device.
		if (reset)
			ImGuiSDL::Reset();

		// Change time data.
		io.DeltaTime = (float)delta;

		// Change display data.
		do {
			const int wndW = _window->width(), wndH = _window->height();
			const int displayW = _renderer->width(), displayH = _renderer->height();
			io.DisplaySize = ImVec2((float)displayW, (float)displayH);
			if (wndW > 0 && wndH > 0)
				io.DisplayFramebufferScale = ImVec2((float)displayW / wndW, (float)displayH / wndH);
		} while (false);

		// Fill mouse/touch data.
		auto assignMouseStates = [&] (Window* wnd, Renderer* rnd, int mouseX, int mouseY) -> void {
			const int scale = rnd->scale() / wnd->scale();
			ImVec2 pos((float)mouseX, (float)mouseY);
			if (scale != 1) {
				pos.x /= scale;
				pos.y /= scale;
			}
			if (_context.mousePosition.x != pos.x || _context.mousePosition.y != pos.y) {
				_context.mousePosition = pos;

				requestFrameRate(APPLICATION_INPUTING_FRAME_RATE);
			}
			io.MousePos = pos;
		};

		do {
			io.MouseDown[0] = _context.mousePressed[0]; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
			io.MouseDown[1] = _context.mousePressed[1];
			io.MouseDown[2] = _context.mousePressed[2];
			_context.mousePressed[0] = _context.mousePressed[1] = _context.mousePressed[2] = false;
		} while (false);

		bool hasTouch = false;
		do {
			const int wndw = _window->width();
			const int wndh = _window->height();

			const int n = SDL_GetNumTouchDevices();
			for (int m = 0; m < n; ++m) {
				SDL_TouchID tid = SDL_GetTouchDevice(m);
				if (tid == 0)
					continue;

				const int f = SDL_GetNumTouchFingers(tid);
				for (int i = 0; i < f; ++i) {
					SDL_Finger* finger = SDL_GetTouchFinger(tid, i);
					if (!finger)
						continue;

					const int touchX = (int)(finger->x * ((float)wndw - Math::EPSILON<float>()));
					const int touchY = (int)(finger->y * ((float)wndh - Math::EPSILON<float>()));
					assignMouseStates(_window, _renderer, touchX, touchY);
					io.MouseDown[0] |= true;
					hasTouch = true;

					break;
				}

				if (hasTouch)
					break;
			}
		} while (false);

		do {
			if (hasTouch)
				break;

			if (io.WantSetMousePos)
				SDL_WarpMouseInWindow(wnd, (int)io.MousePos.x, (int)io.MousePos.y);
			else
				io.MousePos = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());

			int mouseX = 0, mouseY = 0;
			const Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
			io.MouseDown[0] |= !!(mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT));
			io.MouseDown[1] |= !!(mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT));
			io.MouseDown[2] |= !!(mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE));

#if SDL_VERSION_ATLEAST(2, 0, 4) && (defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX)
#	if APPLICATION_ALWAYS_CAPTURE_MOUSE_WHEEL_EVENT
			constexpr const bool capture = true;
#	else /* APPLICATION_ALWAYS_CAPTURE_MOUSE_WHEEL_EVENT */
			SDL_Window* focusedWindow = SDL_GetKeyboardFocus();
			const bool capture = wnd == focusedWindow;
#	endif /* APPLICATION_ALWAYS_CAPTURE_MOUSE_WHEEL_EVENT */
			if (capture) {
				if (_context.mouseCanUseGlobalState) {
					// `SDL_GetMouseState` gives mouse position seemingly based on the last window entered/focused(?)
					// The creation of a new windows at runtime and `SDL_CaptureMouse` both seems to severely mess up with that, so we retrieve that position globally.
					// Won't use this workaround when on Wayland, as there is no global mouse position.
					int wndX = 0, wndY = 0;
					SDL_GetWindowPosition(wnd, &wndX, &wndY);
					SDL_GetGlobalMouseState(&mouseX, &mouseY);
					mouseX -= wndX;
					mouseY -= wndY;
				}
				assignMouseStates(_window, _renderer, mouseX, mouseY);
			}

			// `SDL_CaptureMouse` let the OS know e.g. that our ImGui drag outside the SDL window boundaries shouldn't e.g. trigger the OS window resize cursor.
			// The function is only supported from SDL 2.0.4 (released Jan 2016).
			const bool anyMouseButtonDown = ImGui::IsAnyMouseDown();
			SDL_CaptureMouse(anyMouseButtonDown ? SDL_TRUE : SDL_FALSE);
#else /* Platform macro. */
			if (SDL_GetWindowFlags(wnd) & SDL_WINDOW_INPUT_FOCUS) {
				assignMouseStates(_window, _renderer, mouseX, mouseY);
			}
#endif /* Platform macro. */
		} while (false);

		// Change mouse cursor.
		do {
			if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
				break;

			if (mouseCursorIndicated && _workspace->canvasHovering()) // Skip setting cursor if it's already indicated.
				break;

			ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
			if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None) {
				// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor.
				SDL_ShowCursor(SDL_FALSE);
			} else {
				// Show OS mouse cursor.
				SDL_SetCursor(_context.mouseCursors[imguiCursor] ? _context.mouseCursors[imguiCursor] : _context.mouseCursors[ImGuiMouseCursor_Arrow]);
				SDL_ShowCursor(SDL_TRUE);
			}
		} while (false);

		// Fill navigation data.
		do {
			memset(io.NavInputs, 0, sizeof(io.NavInputs));
			if (!(io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad))
				break;

			SDL_GameController* gameController = SDL_GameControllerOpen(0);
			if (!gameController) {
				io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;

				break;
			}

#define MAP_BUTTON(NAV_NO, BUTTON_NO) { io.NavInputs[NAV_NO] = SDL_GameControllerGetButton(gameController, BUTTON_NO) ? 1.0f : 0.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float vn = (float)(SDL_GameControllerGetAxis(gameController, AXIS_NO) - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }
			constexpr int THUMB_DEAD_ZONE = 8000; // "SDL_gamecontroller.h" suggests using this value.
			MAP_BUTTON(ImGuiNavInput_Activate, SDL_CONTROLLER_BUTTON_A);                // A/Cross.
			MAP_BUTTON(ImGuiNavInput_Cancel, SDL_CONTROLLER_BUTTON_B);                  // B/Circle.
			MAP_BUTTON(ImGuiNavInput_Menu, SDL_CONTROLLER_BUTTON_X);                    // X/Square.
			MAP_BUTTON(ImGuiNavInput_Input, SDL_CONTROLLER_BUTTON_Y);                   // Y/Triangle.
			MAP_BUTTON(ImGuiNavInput_DpadLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT);        // D-Pad left.
			MAP_BUTTON(ImGuiNavInput_DpadRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);      // D-Pad right.
			MAP_BUTTON(ImGuiNavInput_DpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP);            // D-Pad up.
			MAP_BUTTON(ImGuiNavInput_DpadDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN);        // D-Pad down.
			MAP_BUTTON(ImGuiNavInput_FocusPrev, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);    // L1/LB.
			MAP_BUTTON(ImGuiNavInput_FocusNext, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);   // R1/RB.
			MAP_BUTTON(ImGuiNavInput_TweakSlow, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);    // L1/LB.
			MAP_BUTTON(ImGuiNavInput_TweakFast, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);   // R1/RB.
			MAP_ANALOG(ImGuiNavInput_LStickLeft, SDL_CONTROLLER_AXIS_LEFTX, -THUMB_DEAD_ZONE, -32768);
			MAP_ANALOG(ImGuiNavInput_LStickRight, SDL_CONTROLLER_AXIS_LEFTX, +THUMB_DEAD_ZONE, +32767);
			MAP_ANALOG(ImGuiNavInput_LStickUp, SDL_CONTROLLER_AXIS_LEFTY, -THUMB_DEAD_ZONE, -32767);
			MAP_ANALOG(ImGuiNavInput_LStickDown, SDL_CONTROLLER_AXIS_LEFTY, +THUMB_DEAD_ZONE, +32767);

			io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
#undef MAP_BUTTON
#undef MAP_ANALOG
		} while (false);

		// Finish.
		return alive;
	}

	void requestFrameRate(unsigned fps) {
		if (fps > _context.expectedFrameRate) {
			_context.expectedFrameRate = fps;
			_context.updatedFrameCount = 0;
		}
	}
};

class Application* createApplication(class Workspace* workspace, int argc, const char* argv[]) {
	// Extract data.
#if defined GBBASIC_OS_HTML
	EM_ASM({
		FS.mkdir('/docs');
		FS.mkdir('/docs/imgs');
		FS.mkdir('/examples');
		FS.mkdir('/exporters');
		FS.mkdir('/fonts');
		FS.mkdir('/kernels');
		FS.mkdir('/kits');
		FS.mkdir('/music');
		FS.mkdir('/sfx');
		FS.mkdir('/themes');
	});

	const char* package = "/gbbasic.zip";
	do {
		if (!Path::fileExists(package)) {
			fprintf(stderr, "Cannot find package \"%s\".\n", package);

			break;
		}

		Archive::Ptr arc(Archive::create(Archive::ZIP));
		if (!arc->open(package, Stream::READ)) {
			fprintf(stderr, "Cannot open package \"%s\".\n", package);

			break;
		}
		arc->toDirectory("/");
		arc->close();
	} while (false);
#endif /* Platform macro. */

	// Prepare.
	Text::Dictionary options;
	applicationParseArgs(argc, argv, options);
	applicationLoadArgsFile(APPLICATION_ARGS_FILE, options);
	applicationLoadArgsString(applicationGetArgs(), options);

	// Initialize locale.
	const char* loc = Platform::locale("");
	if (loc && Platform::locale() == Platform::Languages::UNSET) {
		std::string loc_ = loc;
		Text::toLowerCase(loc_);
		if (Text::indexOf(loc_, "chinese") != std::string::npos || Text::indexOf(loc_, "china") != std::string::npos) {
			Platform::locale(Platform::Languages::CHINESE);
		} else if (Text::indexOf(loc_, "french") != std::string::npos || Text::indexOf(loc_, "france") != std::string::npos) {
			Platform::locale(Platform::Languages::FRENCH);
		} else if (Text::indexOf(loc_, "german") != std::string::npos || Text::indexOf(loc_, "germany") != std::string::npos) {
			Platform::locale(Platform::Languages::GERMAN);
		} else if (Text::indexOf(loc_, "italian") != std::string::npos || Text::indexOf(loc_, "italy") != std::string::npos) {
			Platform::locale(Platform::Languages::ITALIAN);
		} else if (Text::indexOf(loc_, "japanese") != std::string::npos || Text::indexOf(loc_, "japan") != std::string::npos) {
			Platform::locale(Platform::Languages::JAPANESE);
		} else if (Text::indexOf(loc_, "portuguese") != std::string::npos || Text::indexOf(loc_, "portugal") != std::string::npos) {
			Platform::locale(Platform::Languages::PORTUGUESE);
		} else if (Text::indexOf(loc_, "spanish") != std::string::npos || Text::indexOf(loc_, "spain") != std::string::npos) {
			Platform::locale(Platform::Languages::SPANISH);
		} else {
			Platform::locale(Platform::Languages::ENGLISH);
		}
	}
	Text::locale("C");
	fprintf(stdout, "\n");

	// Initialize working directory.
	Text::Dictionary::const_iterator cwdOpt = options.find(WORKSPACE_OPTION_APPLICATION_CWD_KEY);
	if (cwdOpt == options.end()) {
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
		const std::string path = Path::executableFile();
		std::string dir;
		Path::split(path, nullptr, nullptr, &dir);
		Path::currentDirectory(dir.c_str());
#elif defined GBBASIC_OS_HTML
		Path::currentDirectory("/home");
#endif /* Platform macro. */
	} else {
		std::string path = cwdOpt->second;
		path = Unicode::fromOs(path);
		if (path.size() >= 2 && path.front() == '\"' && path.back() == '\"') {
			path.erase(path.begin());
			path.pop_back();
		}
		Path::uniform(path);
		Path::currentDirectory(path.c_str());
	}

	// Initialize the SDL library.
#if defined GBBASIC_OS_IOS
	constexpr const Uint32 COMPONENTS =
		SDL_INIT_TIMER |
		SDL_INIT_AUDIO | SDL_INIT_VIDEO |
		SDL_INIT_HAPTIC |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR;
	if (SDL_Init(COMPONENTS) < 0)
		fprintf(stderr, "[SDL warning]: %s\n", SDL_GetError());
#elif defined GBBASIC_OS_HTML
	if (SDL_Init(SDL_INIT_EVERYTHING & ~SDL_INIT_HAPTIC) < 0)
		fprintf(stderr, "[SDL warning]: %s\n", SDL_GetError());
#else /* Platform macro. */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		fprintf(stderr, "[SDL warning]: %s\n", SDL_GetError());
#endif /* Platform macro. */

	// Initialize the file monitor.
	FileMonitor::initialize();

	// Create an application instance.
	Application* result = new Application(workspace);
	result->open(options);

	// Setup event handler.
	applicationSetupEventTypes();

	SDL_SetEventFilter(
		[] (void* userdata, SDL_Event* event) -> int {
			Application* app = (Application*)userdata;
			if (app->sendWithAppEvents(event))
				return 0;

			return 1;
		},
		result
	);

	// Recommend desktop version if necessary.
#if defined GBBASIC_OS_HTML
	EM_ASM({
		do {
			if (typeof isDesktopVersionSuggestionTipsEnabled == 'function') {
				if (!isDesktopVersionSuggestionTipsEnabled())
					break;
			}

			if (typeof showTips != 'function')
				break;

			showTips({
				content: '<a href="https://paladin-t.github.io/kits/gbb/" target="_blank">Click</a> to get desktop version.',
				timeout: -1
			});
		} while (false);
	});
#endif /* Platform macro. */

	// Finish.
	return result;
}

void destroyApplication(class Application* &app) {
	// Dispose event handler.
	SDL_SetEventFilter(nullptr, nullptr);

	applicationClearEventTypes();

	// Destroy the application instance.
	app->close();
	delete app;
	app = nullptr;

	// Dispose the file monitor.
	FileMonitor::dispose();

	// Dispose the SDL library.
	SDL_Quit();
}

bool updateApplication(class Application* app) {
	// Update the application.
	const bool result = app->update();

	return result;
}

void pushApplicationEvent(class Application* app, int event, int code, int data0, int data1) {
	SDL_Event evt;
	SDL_memset(&evt, 0, sizeof(SDL_Event));
	evt.type = EVENTS.BEGIN + (UInt32)event;
	evt.user.code = (UInt32)code;
	evt.user.data1 = (void*)(intptr_t)data0;
	evt.user.data2 = (void*)(intptr_t)data1;

	if (evt.type >= EVENTS.BEGIN && evt.type <= EVENTS.END) {
		const Workspace::ExternalEventTypes y = (Workspace::ExternalEventTypes)(evt.type - EVENTS.BEGIN);
		if (y == Workspace::ExternalEventTypes::UNLOAD_WINDOW) {
			app->sendWithAppEvents(&evt); // Send immediate event.

			return;
		}
	}

	SDL_PushEvent(&evt); // Push queued event.
}

/* ===========================================================================} */
