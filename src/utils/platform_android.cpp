/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "encoding.h"
#include "platform.h"
#include "text.h"
#include <SDL.h>
#include <dirent.h>
#include <fstream>
#include <jni.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

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

std::string Platform::writableDirectory(void) {
	const char* cstr = SDL_GetPrefPath("gbbasic", "data");
	const std::string osstr = Unicode::toOs(cstr);
	SDL_free((void*)cstr);

	return osstr;
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

void Platform::surf(const char* url) {
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	jobject activity = (jobject)SDL_AndroidGetActivity();
	jclass clazz = env->GetObjectClass(activity);
	jstring jUrl = env->NewStringUTF(url);
	jmethodID methodId = env->GetMethodID(clazz, "platformSurf", "(Ljava/lang/String;)V");

	env->CallVoidMethod(activity, methodId, jUrl);

	env->DeleteLocalRef(jUrl);
	env->DeleteLocalRef(clazz);
	env->DeleteLocalRef(activity);
}

void Platform::browse(const char*) {
	GBBASIC_MISSING
}

/* ===========================================================================} */

/*
** {===========================================================================
** Clipboard
*/

bool Platform::hasClipboardText(void) {
	return !!SDL_HasClipboardText();
}

std::string Platform::getClipboardText(void) {
	const char* cstr = SDL_GetClipboardText();
	const std::string txt = cstr;
	const std::string osstr = Unicode::toOs(txt);
	SDL_free((void*)cstr);

	return osstr;
}

void Platform::setClipboardText(const char* txt) {
	const std::string utfstr = Unicode::fromOs(txt);

	SDL_SetClipboardText(utfstr.c_str());
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
	return "Android";
}

void Platform::threadName(const char* threadName) {
	prctl(PR_SET_NAME, threadName, 0, 0, 0);
}

std::string Platform::execute(const char* cmd) {
	const int ret_ = system(cmd);
	const std::string ret = Text::toString(ret_);

	return ret;
}

bool Platform::checkProgram(const char* /* prog */) {
	return false;
}

void Platform::redirectIoToConsole(void) {
	GBBASIC_MISSING
}

/* ===========================================================================} */

/*
** {===========================================================================
** GUI
*/

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

bool Platform::isMobile(void) {
	return true;
}

void Platform::tactile(void) {
	// Do nothing.
}

/* ===========================================================================} */
