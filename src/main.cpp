/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "gbbasic.h"
#include "app/application.h"
#include "app/workspace.h"

#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#elif defined GBBASIC_OS_WIN
#	include <fcntl.h>
#	include <io.h>
#	include <Windows.h>
#elif defined GBBASIC_OS_LINUX
	// Do nothing.
#elif defined GBBASIC_OS_MAC
	// Do nothing.
#elif defined GBBASIC_OS_ANDROID
#	include "platform.h"
#	include <SDL.h>
#elif defined GBBASIC_OS_IOS
#	include "platform.h"
#	include <SDL.h>
#else /* Platform macro. */
#	error "Not implemented."
#endif /* Platform macro. */

typedef std::vector<const char*> Argv;

#if defined GBBASIC_OS_HTML
EM_JS(
	bool, mainIsFsSynced, (), {
		return !!Module.syncdone;
	}
);
EM_JS(
	bool, mainGetActiveFrameRate, (), {
		if (typeof getActiveFrameRate != 'function')
			return 60;

		return getActiveFrameRate();
	}
);
#endif /* GBBASIC_OS_HTML */

static Application* app = nullptr;

static int entry(int argc, const char* argv[]) {
#if defined GBBASIC_OS_IOS || defined GBBASIC_OS_ANDROID
	app = createApplication(new Workspace(), argc, argv);
	while (updateApplication(app)) { /* Do nothing. */ }
	destroyApplication(app);
#elif defined GBBASIC_OS_HTML
	constexpr const int STEP = 10;
	emscripten_set_main_loop([] (void) -> void { /* Do nothing. */ }, 0, 0);
	while (!mainIsFsSynced()) {
		emscripten_sleep(STEP);
	}
	app = createApplication(new Workspace(), argc, argv);
	emscripten_cancel_main_loop();
	emscripten_set_main_loop_arg(
		[] (void* arg) -> void {
			Application* app_ = (Application*)arg;
			updateApplication(app_);
		},
		app,
		mainGetActiveFrameRate(), 1
	);
	destroyApplication(app);
#else /* Platform macro. */
	app = createApplication(new Workspace(), argc, argv);
	while (updateApplication(app)) { /* Do nothing. */ }
	destroyApplication(app);
#endif /* Platform macro. */

	return 0;
}

#if defined GBBASIC_OS_HTML /* Platform macro. */
#if defined __EMSCRIPTEN_PTHREADS__
#	pragma message("Using Emscripten thread.")
#else /* __EMSCRIPTEN_PTHREADS__ */
#	pragma message("Not using Emscripten thread.")
#endif /* __EMSCRIPTEN_PTHREADS__ */

#ifndef MAIN_BIN_PATH
#	define MAIN_BIN_PATH "/home/gbbasic.js"
#endif /* MAIN_BIN_PATH */
#ifndef MAIN_DOCUMENT_PATH
#	define MAIN_DOCUMENT_PATH "/Documents"
#endif /* MAIN_DOCUMENT_PATH */
#ifndef MAIN_WRITABLE_PATH
#	define MAIN_WRITABLE_PATH "/data"
#endif /* MAIN_WRITABLE_PATH */

extern char platformBinPath[GBBASIC_MAX_PATH + 1];

extern void platformSetDocumentPathResolver(std::string(*)(void));

extern void platformSetWritablePathResolver(std::string(*)(void));

EM_JS(
	void, initializeHtml, (const char* docPath, const char* writablePath), {
		// Initialize the file system.
		const MAIN_DOCUMENT_PATH = UTF8ToString(docPath);
		const MAIN_WRITABLE_PATH = UTF8ToString(writablePath);

		FS.mkdir(MAIN_DOCUMENT_PATH);
		FS.mount(MEMFS, { }, MAIN_DOCUMENT_PATH);

		Module.syncdone = 0;
		FS.mkdir(MAIN_WRITABLE_PATH);
		FS.mount(IDBFS, { }, MAIN_WRITABLE_PATH);

		FS.syncfs(true, (err) => {
			if (err) {
				Module.printErr(err);
			}
			Module.print("HTML filesystem initialized.");
			Module.syncdone = 1;
		});

		// Initialize unload event handler.
		document.addEventListener('DOMContentLoaded', function () {
			window.addEventListener('unload', function (event) {
				if (typeof ccall == 'function' && !!(Module.ExternalEventTypes)) {
					ccall(
						'pushEvent',
						'number', [ 'number', 'number', 'number', 'number' ],
						[
							Module.ExternalEventTypes.UNLOAD_WINDOW.value,
							0,
							0,
							0
						]
					);
				}
			});
		});
	}
);

extern "C" {

EMSCRIPTEN_KEEPALIVE int pushEvent(int event, int code, int data0, int data1) {
	pushApplicationEvent(app, event, code, data0, data1);

	return 0;
}

}

int main(int argc, const char* argv[]) {
	Argv args;

	initializeHtml(MAIN_DOCUMENT_PATH, MAIN_WRITABLE_PATH);

	memset(platformBinPath, 0, GBBASIC_COUNTOF(platformBinPath));
	snprintf(platformBinPath, GBBASIC_COUNTOF(platformBinPath), "%s", MAIN_BIN_PATH);

	platformSetDocumentPathResolver(
		[] (void) -> std::string {
			return MAIN_DOCUMENT_PATH;
		}
	);

	platformSetWritablePathResolver(
		[] (void) -> std::string {
			return MAIN_WRITABLE_PATH;
		}
	);

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined GBBASIC_OS_WIN /* Platform macro. */
static void openTerminal(void) {
	long hConHandle;
	HANDLE lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp = nullptr;

	::AllocConsole();

	::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = 500;
	::SetConsoleScreenBufferSize(::GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	lStdHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, nullptr, _IONBF, 0);

	lStdHandle = ::GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, nullptr, _IONBF, 0);

	lStdHandle = ::GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, nullptr, _IONBF, 0);

	std::ios::sync_with_stdio();

	freopen("CON", "w", stdout);
	freopen("CON", "r", stdin);
	freopen("CON", "w", stderr);
}

static Argv splitArgs(const char* ln, Text::Array &args) {
	Argv ret;
	Text::Array tmp = args;
	bool withQuate = false;
	args = Text::split(ln, " ");
	for (std::string &a : args) {
		if (withQuate) {
			if (tmp.empty())
				tmp.push_back("");
			a = Text::trimRight(a);
			if (!a.empty() && a.back() == '"')
				withQuate = false;
			tmp.back() += " ";
			tmp.back() += a;
		} else {
			a = Text::trimLeft(a);
			if (a.size() >= 2 && a.front() == '"' && a.back() == '"') {
				tmp.push_back(a);
			} else if (!a.empty() && a.front() == '"') {
				withQuate = true;
				tmp.push_back(a);
			} else {
				tmp.push_back(a);
			}
		}
	}
	args = tmp;
	for (const std::string &a : args) {
		ret.push_back(a.c_str());
	}

	return ret;
}

int CALLBACK WinMain(_In_ HINSTANCE /* hInstance */, _In_ HINSTANCE /* hPrevInstance */, _In_ LPSTR lpCmdLine, _In_ int /* nCmdShow */) {
#if defined GBBASIC_DEBUG
	_CrtSetBreakAlloc(0);
	atexit(
		[] (void) -> void {
			if (!!_CrtDumpMemoryLeaks()) {
				fprintf(stderr, "Memory leak!\n");

				_CrtDbgBreak();
			}
		}
	);

	openTerminal();
#endif /* GBBASIC_DEBUG */

	Text::Array buf;
	Argv args = splitArgs(lpCmdLine, buf);

#if !defined GBBASIC_DEBUG
	for (const char* arg : args) {
		if (arg && strcmp(arg, "-" WORKSPACE_OPTION_APPLICATION_CONSOLE_ENABLED_KEY) == 0) {
			openTerminal();

			break;
		}
	}
#endif /* GBBASIC_DEBUG */

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined GBBASIC_OS_LINUX /* Platform macro. */
extern char platformBinPath[GBBASIC_MAX_PATH + 1];

int main(int argc, const char* argv[]) {
	char buf[GBBASIC_MAX_PATH + 1];
	buf[GBBASIC_MAX_PATH] = '\0';
	Argv args;

	realpath(argv[0], buf);
	memset(platformBinPath, 0, GBBASIC_COUNTOF(platformBinPath));
	if (argc >= 1)
		snprintf(platformBinPath, GBBASIC_COUNTOF(platformBinPath), "%s", buf);

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined GBBASIC_OS_MAC /* Platform macro. */
int main(int argc, const char* argv[]) {
	Argv args;

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined GBBASIC_OS_ANDROID /* Platform macro. */
#if defined __cplusplus
extern "C" {
#endif /* __cplusplus */

FILE* fopen64(char const* fn, char const* m) {
	return fopen(fn, m);
}

long ftello64(FILE* fp) {
	return ftell(fp);
}

int fseeko64(FILE* fp, long off, int ori) {
	return fseek(fp, off, ori);
}

#if defined __cplusplus
}
#endif /* __cplusplus */

extern char platformBinPath[GBBASIC_MAX_PATH + 1];

extern void platformSetDocumentPathResolver(std::string(*)(void));

#if defined __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined main
#	undef main
#endif /* main */

int main(int argc, const char* argv[]) {
	Argv args;

	memset(platformBinPath, 0, GBBASIC_COUNTOF(platformBinPath));
	snprintf(platformBinPath, GBBASIC_COUNTOF(platformBinPath), "%s", "/");

	platformSetDocumentPathResolver(
		[] (void) -> std::string {
			std::string dir;
			const char* cstr = SDL_AndroidGetInternalStoragePath();
			if (!cstr) {
				const int state = SDL_AndroidGetExternalStorageState();
				if (state & SDL_ANDROID_EXTERNAL_STORAGE_WRITE)
					cstr = SDL_AndroidGetExternalStoragePath();
			}
			if (!cstr) {
				cstr = SDL_GetPrefPath("gbbasic", "data");
				if (cstr) {
					dir = cstr;
					SDL_free((void*)cstr);
					cstr = nullptr;
				}
			}
			if (dir.empty() && cstr) {
				dir = cstr;
			}

			return dir;
		}
	);

	if (argc >= 2)
		Platform::currentDirectory(argv[1]);

	for (int i = 2; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}

#if defined __cplusplus
}
#endif /* __cplusplus */
#elif defined GBBASIC_OS_IOS /* Platform macro. */
#if defined __cplusplus
extern "C" {
#endif /* __cplusplus */

int SDL_main(int argc, char* argv[]) {
	Argv args;

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}

#if defined __cplusplus
}
#endif /* __cplusplus */
#endif /* Platform macro. */
