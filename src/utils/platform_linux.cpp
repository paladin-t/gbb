/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "encoding.h"
#include "file_sandbox.h"
#include "platform.h"
#include <SDL.h>
#include <dirent.h>
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#include <glib.h>
#include <pwd.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
** {===========================================================================
** Macros and constants
*/

static_assert(sizeof(time_t) >= 8, "Wrong size.");

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

static const char* platformNowString(void) {
	time_t ct;
	struct tm* timeInfo = nullptr;
	static char buf[80];
	time(&ct);
	timeInfo = localtime(&ct);
	strftime(buf, GBBASIC_COUNTOF(buf), "%FT%T", timeInfo);

	return buf;
}

static bool platformDirectoryExists(const char* path) {
	DIR* dir = opendir(path);
	const bool result = !!dir;
	if (dir)
		closedir(dir);

	return result;
}

static void platformSplitFileName(const std::string &qualifiedName, std::string &outBaseName, std::string &outPath) {
	const std::string path = qualifiedName;
	const size_t i = path.find_last_of('/'); // Split based on final '/'.

	if (i == std::string::npos) {
		outPath.clear();
		outBaseName = qualifiedName;
	} else {
		outBaseName = path.substr(i + 1, path.size() - i - 1);
		outPath = path.substr(0, i + 1);
	}
}

static void platformSplitBaseFileName(const std::string &fullname, std::string &outBaseName, std::string &out_ext) {
	const size_t i = fullname.find_last_of(".");
	if (i == std::string::npos) {
		out_ext.clear();
		outBaseName = fullname;
	} else {
		out_ext = fullname.substr(i + 1);
		outBaseName = fullname.substr(0, i);
	}
}

static void platformSplitFullFileName(const std::string &qualifiedName, std::string &outBaseName, std::string &out_ext, std::string &outPath) {
	std::string fullName;
	platformSplitFileName(qualifiedName, fullName, outPath);
	platformSplitBaseFileName(fullName, outBaseName, out_ext);
}

static bool platformRemoveToTrashBin(const char* src) {
	// Get the `${HOME}` directory.
	struct passwd* pw = getpwuid(getuid());
	std::string homedir = pw ? pw->pw_dir : "~";
	if (homedir.back() != '/')
		homedir += '/';

	std::string trs;
	Text::Array paths;
	paths.push_back(homedir + ".local/share/Trash/");
	paths.push_back(homedir + ".trash/");
	paths.push_back(homedir + "root/.local/share/Trash/");
	for (const std::string &p : paths) {
		if (platformDirectoryExists(p.c_str())) {
			trs = p;

			break;
		}
	}
	if (trs.empty())
		return false;

	const std::string trsInfo = trs + "info/";
	const std::string trsFiles = trs + "files/";
	if (!platformDirectoryExists(trsInfo.c_str()) || !platformDirectoryExists(trsFiles.c_str()))
		return false;

	// Get information.
	const char* encodedChars = "!*'();:@&=+$,/?#[]";
	char* uri = g_uri_escape_string(src, encodedChars, true);
	std::string info;
	info += "[Trash Info]\nPath=";
	info += uri;
	info += "\nDeletionDate=";
	info += platformNowString();
	info += "\n";
	free(uri);

	std::string trsName, trsExt, trsDir;
	platformSplitFullFileName(src, trsName, trsExt, trsDir);
	if (!trsExt.empty())
		trsExt = "." + trsExt;
	std::string trsNameExt = trsName + trsExt;
	std::string infoPath = trsInfo + trsNameExt + ".trashinfo";
	std::string filePath = trsFiles + trsNameExt;
	int nr = 1;
	while (platformDirectoryExists(infoPath.c_str()) || platformDirectoryExists(filePath.c_str())) {
		char buf[GBBASIC_MAX_PATH];
		++nr;
		sprintf(buf, "%s.%d%s", trsName.c_str(), nr, trsExt.c_str());
		trsNameExt = buf;
		infoPath = trsInfo + trsNameExt + ".trashinfo";
		filePath = trsFiles + trsNameExt;
	}

	// Try to delete to trash bin, and update its information.
	std::error_code ret;
	filesystem::rename(src, filePath.c_str(), ret);
	if (!!ret.value())
		return false;

	FILE* fp = fopen(infoPath.c_str(), "wb");
	if (fp) {
		fwrite(info.c_str(), sizeof(char), info.length(), fp);
		fclose(fp);
	}

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Basic
*/

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
	const filesystem::copy_options options =
		filesystem::copy_options::overwrite_existing |
		filesystem::copy_options::recursive;

	std::error_code ret;
	filesystem::copy(
		src, dst,
		options,
		ret
	);

	return !ret.value();
}

bool Platform::copyDirectory(const char* src, const char* dst) {
	const filesystem::copy_options options =
		filesystem::copy_options::overwrite_existing |
		filesystem::copy_options::recursive;

	std::error_code ret;
	filesystem::copy(
		src, dst,
		options,
		ret
	);

	return !ret.value();
}

bool Platform::moveFile(const char* src, const char* dst) {
	std::error_code ret;
	filesystem::rename(src, dst, ret);

	return !ret.value();
}

bool Platform::moveDirectory(const char* src, const char* dst) {
	std::error_code ret;
	filesystem::rename(src, dst, ret);

	return !ret.value();
}

bool Platform::removeFile(const char* src, bool toTrash) {
	// Delete to trash bin.
	if (toTrash) {
		if (platformRemoveToTrashBin(src))
			return true;
	}

	// Delete from hard drive directly.
	return !unlink(src);
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	// Delete to trash bin.
	if (toTrash) {
		if (platformRemoveToTrashBin(src))
			return true;
	}

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

				std::string ustr = absPath;
				removeDirectory(ustr.c_str(), toTrash);
			} else {
				if (file = fopen(absPath, "r")) {
					fclose(file);

					std::error_code _;
					filesystem::remove(absPath, _);
				}
			}
		}
	}
	std::error_code ret;
	filesystem::remove(src, ret);

	closedir(dir);

	return !ret.value();
}

bool Platform::makeDirectory(const char* path) {
	return !mkdir(path, 0777);
}

void Platform::accreditDirectory(const char*) {
	// Do nothing.
}

bool Platform::equal(const char* lpath, const char* rpath) {
	const filesystem::path lp = lpath;
	const filesystem::path rp = rpath;

	return lp == rp;
}

bool Platform::isParentOf(const char* lpath, const char* rpath) {
	const filesystem::path lp = lpath;
	filesystem::path rp = rpath;

	if (lp == rp)
		return true;
	while (!rp.empty()) {
		rp = rp.parent_path();
		if (lp == rp)
			return true;
	}

	return false;
}

std::string Platform::absoluteOf(const std::string &path) {
	if (path.empty())
		return path;

	filesystem::path ret = filesystem::absolute(path);
	std::string result = ret.string();
	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
}

char platformBinPath[GBBASIC_MAX_PATH + 1];

std::string Platform::executableFile(void) {
	return platformBinPath;
}

std::string Platform::documentDirectory(void) {
	// Get the `${HOME}` directory.
	struct passwd* pw = getpwuid(getuid());
	std::string homeDir = pw ? pw->pw_dir : "~";
	if (homeDir.back() != '/')
		homeDir += '/';
	std::string doc;

	// Try to retrieve from XDG config.
	do {
		std::string dirs = homeDir;
		dirs += ".config/user-dirs.dirs";
		FILE* fp = fopen(dirs.c_str(), "r");
		if (!fp)
			break;

		const long curPos = ftell(fp);
		fseek(fp, 0L, SEEK_END);
		const long len = ftell(fp);
		fseek(fp, curPos, SEEK_SET);
		char* buf = (char*)malloc((size_t)(len + 1));
		fread(buf, 1, len, fp);
		buf[len] = '\0';
		fclose(fp);
		const std::string str = buf;
		free(buf);
		buf = nullptr;

		const char* begin = strstr(str.c_str(), "XDG_DOCUMENTS_DIR=\"");
		if (!begin)
			break;
		begin = strchr(begin, '/');
		if (!begin)
			break;
		++begin;
		const char* end = strstr(begin, "\"");
		if (!end)
			break;
		doc.assign(begin, end);
		if (doc.back() != '/')
			doc += '/';
		doc = homeDir + doc;

		return doc;
	} while (false);

	// Try to retrieve `${HOME}/Documents`.
	doc = homeDir + "Documents";
	struct stat st;
	if (stat(doc.c_str(), &st) == 0) {
		if (st.st_mode & S_IFDIR != 0) {
			if (doc.back() != '/') doc += '/';

			return doc;
		}
	}

	// Fall to return the `${HOME}` directory.
	return homeDir;
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
	std::string cmd = "xdg-open \"";
	cmd += url;
	cmd += "\"";
	system(cmd.c_str());
}

void Platform::browse(const char* dir) {
	std::string cmd = "xdg-open \"";
	cmd += dir;
	cmd += "\"";
	system(cmd.c_str());
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
	return "Linux";
}

static Platform::Languages _language = Platform::Languages::UNSET;

Platform::Languages Platform::language(void) {
	return _language;
}

void Platform::language(Languages lang) {
	_language = lang;
}

void Platform::threadName(const char* threadName) {
	prctl(PR_SET_NAME, threadName, 0, 0, 0);
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

std::string Platform::execute(const char* cmd) {
	const int ret_ = system(cmd);
	const std::string ret = Text::toString(ret_);

	return ret;
}

bool Platform::checkProgram(const char* prog) {
	if (prog == nullptr)
		return false;

	const std::string program = prog;
	int exitCode = -1;
	pfd::internal::executor async;
	async.start_process({ "/bin/sh", "-c", "which " + program });
	async.result(&exitCode);

	return exitCode == 0;
}

void Platform::redirectIoToConsole(void) {
	GBBASIC_MISSING
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
	pfd::save_file dlg(title, default_path, filters);
	std::string path = dlg.result();

	return path;
}

void Platform::notify(const char* title_, const char* message_, const char* icon_) {
	const std::string title = title_ ? title_ : "";
	const std::string message = message_ ? message_ : "";
	const std::string icon = icon_ ? icon_ : "";
	pfd::icon i = pfd::icon::info;
	if (icon == "warning")
		i = pfd::icon::warning;
	else if (icon == "error")
		i = pfd::icon::error;
	else if (icon == "question")
		i = pfd::icon::question;
	pfd::notify notify(title, message, i);
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
	SDL_Rect rect{ x, y, 20, 20 };
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
	const int n = SDL_GetNumTouchDevices();

	return !!n;
}

void Platform::tactile(void) {
	// Do nothing.
}

/* ===========================================================================} */
