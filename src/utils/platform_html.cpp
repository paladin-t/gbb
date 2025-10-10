/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "encoding.h"
#include "platform.h"
#include "text.h"
#include <SDL.h>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#endif /* GBBASIC_OS_HTML */

/*
** {===========================================================================
** Basic
*/

EM_JS(
	void, platformHtmlFree, (void* ptr), {
		_free(ptr);
	}
);

void Platform::open(void) {
	// Do nothing.
}

void Platform::close(void) {
	// Do nothing.
}

/* ===========================================================================} */

/*
** {===========================================================================
** Filesystem
*/

bool Platform::copyFile(const char* src, const char* dst) {
	std::ifstream is(src, std::ios::in | std::ios::binary);
	std::ofstream os(dst, std::ios::out | std::ios::binary);

	std::copy(
		std::istreambuf_iterator<char>(is),
		std::istreambuf_iterator<char>(),
		std::ostreambuf_iterator<char>(os)
	);

	return true;
}

bool Platform::copyDirectory(const char* src, const char* dst) {
	DIR* dir = nullptr;
	struct dirent* ent = nullptr;
	dir = opendir(src);
	if (!dir)
		return false;

	int result = 0;
	DIR* udir = opendir(dst);
	if (udir)
		closedir(udir);
	else
		makeDirectory(dst);

	while (ent = readdir(dir)) {
		DIR* subDir = nullptr;
		FILE* file = nullptr;
		char absPath[GBBASIC_MAX_PATH] = { 0 };

		if (!ignore(ent->d_name)) {
			snprintf(absPath, GBBASIC_COUNTOF(absPath), "%s/%s", src, ent->d_name);

			if (subDir = opendir(absPath)) {
				closedir(subDir);

				const std::string ustr = absPath;
				snprintf(absPath, GBBASIC_COUNTOF(absPath), "%s/%s", dst, ent->d_name);
				const std::string udst = absPath;
				copyDirectory(ustr.c_str(), udst.c_str());
			} else {
				if (file = fopen(absPath, "r")) {
					fclose(file);

					const std::string ustr = absPath;
					snprintf(absPath, GBBASIC_COUNTOF(absPath), "%s/%s", dst, ent->d_name);
					const std::string udst = absPath;
					if (copyFile(ustr.c_str(), udst.c_str()))
						++result;
				}
			}
		}
	}

	closedir(dir);

	return !!result;
}

bool Platform::moveFile(const char* src, const char* dst) {
	if (!copyFile(src, dst))
		return false;

	return removeFile(src, false);
}

bool Platform::moveDirectory(const char* src, const char* dst) {
	if (!copyDirectory(src, dst))
		return false;

	return removeDirectory(src, false);
}

bool Platform::removeFile(const char* src, bool toTrash) {
	// Delete from hard drive directly.
	return !unlink(src);
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	// Delete from hard drive directly.
	DIR* dir = nullptr;
	struct dirent* ent = nullptr;
	dir = opendir(src);
	if (!dir)
		return true;

	while (ent = readdir(dir)) {
		DIR* subDir = nullptr;
		FILE* file = nullptr;
		char absPath[GBBASIC_MAX_PATH] = { 0 };

		if (!ignore(ent->d_name)) {
			snprintf(absPath, GBBASIC_COUNTOF(absPath), "%s/%s", src, ent->d_name);

			if (subDir = opendir(absPath)) {
				closedir(subDir);

				const std::string ustr = absPath;
				removeDirectory(ustr.c_str(), toTrash);
			} else {
				if (file = fopen(absPath, "r")) {
					fclose(file);

					unlink(absPath);
				}
			}
		}
	}
	rmdir(src);

	closedir(dir);

	return true;
}

bool Platform::makeDirectory(const char* path) {
	return !mkdir(path, 0777);
}

void Platform::accreditDirectory(const char*) {
	// Do nothing.
}

bool Platform::equal(const char* lpath, const char* rpath) {
	const std::string lstr = lpath;
	const std::string rstr = rpath;

	return lstr == rstr;
}

bool Platform::isParentOf(const char* lpath, const char* rpath) {
	const std::string lstr = lpath;
	const std::string rstr = rpath;

	if (lstr == rstr)
		return true;
	if (lstr.length() < rstr.length()) {
		if (rstr.find(lstr, 0) == 0) {
			const std::string tail = rstr.substr(lstr.length());
			if (tail.front() == '/' || tail.front() == '\\')
				return true;
		}
	}

	return false;
}

std::string Platform::absoluteOf(const std::string &path) {
	/*
	char absPath[GBBASIC_MAX_PATH] = { 0 };
	std::string result = realpath(path.c_str(), absPath);

	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
	*/

	if (path.empty())
		return path;

	if (path.front() == '/' || path.front() == '\\')
		return path;
	if (Text::startsWith(path, "file://", true))
		return path;

	std::string result = currentDirectory();
	if ((path.front() != '\\' && path.front() != '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');
	result += path;
	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
}

char platformBinPath[GBBASIC_MAX_PATH + 1];

std::string Platform::executableFile(void) {
	return platformBinPath;
}

static std::string (* platformDocumentPathResolver)(void) = nullptr;

void platformSetDocumentPathResolver(std::string (* resolver)(void)) {
	platformDocumentPathResolver = resolver;
}

std::string Platform::documentDirectory(void) {
	if (platformDocumentPathResolver)
		return platformDocumentPathResolver();

	return "";
}

static std::string (* platformWritablePathResolver)(void) = nullptr;

void platformSetWritablePathResolver(std::string (* resolver)(void)) {
	platformWritablePathResolver = resolver;
}

std::string Platform::writableDirectory(void) {
	if (platformWritablePathResolver)
		return platformWritablePathResolver();

	return "";
}

std::string Platform::savedGamesDirectory(void) {
	return writableDirectory();
}

std::string Platform::currentDirectory(void) {
	char buf[GBBASIC_MAX_PATH + 1];
	buf[GBBASIC_MAX_PATH] = '\0';
	getcwd(buf, GBBASIC_MAX_PATH);

	return buf;
}

void Platform::currentDirectory(const char* dir) {
	chdir(dir);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Surfing and browsing
*/

EM_JS(
	void, platformHtmlSurf, (const char* url), {
		window.open(UTF8ToString(url));
	}
);

void Platform::surf(const char* url) {
	platformHtmlSurf(url);
}

void Platform::browse(const char*) {
	GBBASIC_MISSING
}

/* ===========================================================================} */

/*
** {===========================================================================
** Clipboard
*/

EM_JS(void, platformHtmlCopy, (const char* str), {
	Asyncify.handleAsync(async () => {
		try {
			document.getElementById('clipping').focus();
			const _ = await navigator.clipboard.writeText(UTF8ToString(str));
			document.getElementById('canvas').focus();
		} catch (_) {
			do {
				if (Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__ == undefined)
					Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__ = 0;

				if (Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__ >= 1) // Only prompt once.
					break;

				if (typeof showTips == 'function') {
					showTips({
						content: 'No permission to use system clipboard, fallback to browser simulated.',
						type: 'warning'
					});
				}
				++Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__;
			} while (false);

			Module.clipboardText = UTF8ToString(str); // Fallback to JS variable.
		}
	});
});
EM_JS(char*, platformHtmlPaste, (), {
	return Asyncify.handleAsync(async () => {
		try {
			document.getElementById('clipping').focus();
			const str = await navigator.clipboard.readText();
			document.getElementById('canvas').focus();
			const lengthBytes = lengthBytesUTF8(str) + 1;
			const stringOnWasmHeap = _malloc(lengthBytes);
			stringToUTF8(str, stringOnWasmHeap, lengthBytes);

			return stringOnWasmHeap;
		} catch (_) {
			do {
				if (Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__ == undefined)
					Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__ = 0;

				if (Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__ >= 1) // Only prompt once.
					break;

				if (typeof showTips == 'function') {
					showTips({
						content: 'No permission to use system clipboard, fallback to browser simulated.',
						type: 'warning'
					});
				}
				++Module.__GBBASIC_CLIPBOARD_TIPS_SHOWN_TIMES__;
			} while (false);

			if (typeof Module.clipboardText == 'undefined' || !Module.clipboardText)
				return null;

			const str = Module.clipboardText;
			const lengthBytes = lengthBytesUTF8(str) + 1;
			const stringOnWasmHeap = _malloc(lengthBytes);
			stringToUTF8(str, stringOnWasmHeap, lengthBytes);

			return stringOnWasmHeap;
		}
	});
});

bool Platform::hasClipboardText(void) {
	return true;
}

std::string Platform::getClipboardText(void) {
	const char* cstr = platformHtmlPaste();
	const std::string txt = cstr;
	platformHtmlFree((void*)cstr);
	const std::string osstr = Unicode::toOs(txt);

	return osstr;
}

void Platform::setClipboardText(const char* txt) {
	const std::string utfstr = Unicode::fromOs(txt);

	platformHtmlCopy(utfstr.c_str());
}

bool Platform::isClipboardImageSupported(void) {
	return false;
}

bool Platform::hasClipboardImage(void) {
	return false;
}

bool Platform::getClipboardImage(class Image* img) {
	(void)img;

	return false;
}

bool Platform::setClipboardImage(const class Image* img) {
	(void)img;

	return false;
}

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

const char* Platform::os(void) {
	return "HTML";
}

static Platform::Languages _language = Platform::Languages::UNSET;

Platform::Languages Platform::language(void) {
	return _language;
}

void Platform::language(Languages lang) {
	_language = lang;
}

void Platform::threadName(const char*) {
	// Do nothing.
}

//#include "../play/data.h"

bool Platform::loadResource(const char* name, class Bytes* bytes) {
	(void)name;
	bytes->clear();
	/*if (strcmp(name, "DATA") == 0) {
		if (GBBASIC_COUNTOF(ARCHIVE_DATA) > 1) {
			bytes->writeBytes(ARCHIVE_DATA, GBBASIC_COUNTOF(ARCHIVE_DATA));

			return true;
		}
	}*/

	return false;
}

EM_JS(
	const char*, platformHtmlExecute, (const char* cmd), {
		let ret = eval(UTF8ToString(cmd));
		if (ret == undefined)
			ret = 'undefined';
		else if (ret == null)
			ret = 'null';
		else
			ret = toString(ret);
		const lengthBytes = lengthBytesUTF8(ret) + 1;
		const stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes);

		return stringOnWasmHeap;
	}
);

std::string Platform::execute(const char* cmd) {
	const char* ret_ = platformHtmlExecute(cmd);
	const std::string ret = ret_ ? ret_ : "";
	platformHtmlFree((void*)ret_);

	return ret;
}

bool Platform::checkProgram(const char* /* prog */) {
	return false;
}

void Platform::redirectIoToConsole(void) {
	// Do nothing.
}

bool Platform::applyForPermission(const char* what) {
	(void)what;

	return false;
}

bool Platform::isPermissionGranted(const char* what) {
	(void)what;

	return true;
}

bool Platform::requestAd(const char* what, const char* arg) {
	(void)what;
	(void)arg;

	return false;
}

bool Platform::listenedPurchase(Transaction &transaction) {
	(void)transaction;

	return false;
}

void Platform::prefillProducts(const Product::Array &products) {
	(void)products;
}

bool Platform::requestProducts(const Product::IdArray &ids) {
	(void)ids;

	return false;
}

bool Platform::respondedProducts(Product::Array &products) {
	(void)products;

	return false;
}

bool Platform::requestPurchase(const std::string &id) {
	(void)id;

	return false;
}

bool Platform::respondedPurchase(Transaction &transaction) {
	(void)transaction;

	return false;
}

bool Platform::requestRestoring(void) {
	return false;
}

bool Platform::respondedRestoring(Transactions &transactions) {
	(void)transactions;

	return false;
}

/* ===========================================================================} */

/*
** {===========================================================================
** GUI
*/

std::string Platform::saveFileDialog(const std::string &title, const std::string &default_path, const std::vector<std::string> &filters) {
	return "";
}

void Platform::notify(const char* title_, const char* message_, const char* icon_) {
	std::string cmd = "gbbasicEngineNotify";
	cmd += "('";
	cmd += title_ ? title_ : "";
	cmd += "', '";
	cmd += message_ ? message_ : "";
	cmd += "', '";
	cmd += icon_ ? icon_ : "";
	cmd += "');";
	execute(cmd.c_str());
}

void Platform::notifyAfter(const char* title, const char* message, int tag, const char* sound, int badge, int interval) {
	(void)message;
	(void)title;
	(void)tag;
	(void)sound;
	(void)badge;
	(void)interval;

	// Do nothing.
}

void Platform::notifyRepeat(const char* title, const char* message, int tag, const char* sound, int badge, int interval, int repeating_interval) {
	(void)message;
	(void)title;
	(void)tag;
	(void)sound;
	(void)badge;
	(void)interval;
	(void)repeating_interval;

	// Do nothing.
}

void Platform::notifyDaily(const char* title, const char* message, int tag, const char* sound, int badge, int hour, int minute) {
	(void)message;
	(void)title;
	(void)tag;
	(void)sound;
	(void)badge;
	(void)hour;
	(void)minute;

	// Do nothing.
}

void Platform::cancelNotification(int tag) {
	(void)tag;

	// Do nothing.
}

void Platform::cancelAllNotifications(void) {
	// Do nothing.
}

void Platform::clearBadge(void) {
	// Do nothing.
}

void Platform::flashWindow(class Window* wnd) {
	// Do nothing.
}

void Platform::msgbox(const char* text, const char* caption) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, caption, text, nullptr);
}

void Platform::openInput(void) {
	// Do nothing.
}

void Platform::closeInput(void) {
	// Do nothing.
}

void Platform::inputScreenPosition(int x, int y) {
	SDL_Rect rect{ x, y, 0, 0 };
	SDL_SetTextInputRect(&rect);
}

void Platform::hideStatusBar(void) {
	// Do nothing.
}

int Platform::statusBarHeight(void) {
	return 0;
}

bool Platform::notch(int* left, int* top, int* right, int* bottom) {
	(void)left;
	(void)top;
	(void)right;
	(void)bottom;

	return false;
}

bool Platform::isSystemInDarkMode(void) {
	return false;
}

void Platform::useDarkMode(class Window* wnd) {
	(void)wnd;
}

void Platform::setWindowTransparentColor(class Window* wnd, const struct Colour* col) {
	(void)wnd;
	(void)col;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Hardware
*/

EM_JS(
	bool, platformHasTouchScreen, (), {
		if (typeof isMobile != 'function')
			return false;

		return isMobile();
	}
);

bool Platform::isMobile(void) {
	return platformHasTouchScreen();
}

void Platform::tactile(void) {
	// Do nothing.
}

/* ===========================================================================} */
