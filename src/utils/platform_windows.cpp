/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "encoding.h"
#include "file_sandbox.h"
#include "filesystem.h"
#include "image.h"
#include "platform.h"
#include "window.h"
#include "../../lib/clip/clip.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <direct.h>
#include <dwmapi.h>
#include <fcntl.h>
#include <filesystem>
namespace filesystem = std::experimental::filesystem;
#include <io.h>
#include <ShlObj.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
#	define PLATFORM_WINDOWS_IAP_DEBUG_ENABLED 1
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
#ifndef PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT
#	define PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT 30
#endif /* PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT */

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#	define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif /* DWMWA_USE_IMMERSIVE_DARK_MODE */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
	DWORD dwType;     // Must be 0x1000.
	LPCSTR szName;    // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1 for caller thread).
	DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void platformThreadName(uint32_t threadId, const char* threadName) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = threadId;
	info.dwFlags = 0;

	__try {
		::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
	}
}

#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
static bool _pending = false;
static int _delay = 0;
static Platform::Product::Array* _prefilled = nullptr;
static Platform::Product::Array* _products = nullptr;
static std::string* _productId = nullptr;
static unsigned _id = 1;
static Platform::Transaction* _transaction = nullptr;
static Platform::Transactions* _transactions = nullptr;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Basic
*/

void Platform::open(void) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	_prefilled = new Product::Array();
	_products = new Product::Array();
	_productId = new std::string();
	_transaction = new Transaction();
	_transactions = new Transactions();
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	// Do nothing.
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

void Platform::close(void) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	delete _prefilled;
	_prefilled = nullptr;
	delete _products;
	_products = nullptr;
	delete _productId;
	_productId = nullptr;
	delete _transaction;
	_transaction = nullptr;
	delete _transactions;
	_transactions = nullptr;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	// Do nothing.
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
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
	SHFILEOPSTRUCTA op;
	memset(&op, 0, sizeof(SHFILEOPSTRUCTA));
	op.hwnd = nullptr;
	op.wFunc = FO_DELETE;
	op.pFrom = src;
	op.pTo = nullptr;
	op.fFlags = FOF_NO_UI | FOF_FILESONLY | (toTrash ? FOF_ALLOWUNDO : 0);
	const int ret = ::SHFileOperationA(&op);

	return !ret;
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	SHFILEOPSTRUCTA op;
	memset(&op, 0, sizeof(SHFILEOPSTRUCTA));
	op.hwnd = nullptr;
	op.wFunc = FO_DELETE;
	op.pFrom = src;
	op.pTo = nullptr;
	op.fFlags = FOF_NO_UI | (toTrash ? FOF_ALLOWUNDO : 0);
	const int ret = ::SHFileOperationA(&op);

	return !ret;
}

bool Platform::makeDirectory(const char* path) {
	return !_mkdir(path);
}

void Platform::accreditDirectory(const char* path) {
	HANDLE hFile = nullptr;
	WIN32_FIND_DATAA fileInfo;
	char szPath[GBBASIC_MAX_PATH];
	char szFolderInitialPath[GBBASIC_MAX_PATH];
	char wildcard[GBBASIC_MAX_PATH] = "\\*.*";

	strcpy(szPath, path);
	strcpy(szFolderInitialPath, path);
	strcat(szFolderInitialPath, wildcard);

	hFile = ::FindFirstFileA(szFolderInitialPath, &fileInfo);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if (!ignore(fileInfo.cFileName)) {
				strcpy(szPath, path);
				strcat(szPath, "\\");
				strcat(szPath, fileInfo.cFileName);
				if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					// It is a sub directory.
					::SetFileAttributesA(szPath, FILE_ATTRIBUTE_NORMAL);
					accreditDirectory(szPath);
				} else {
					// It is a file.
					::SetFileAttributesA(szPath, FILE_ATTRIBUTE_NORMAL);
				}
			}
		} while (::FindNextFileA(hFile, &fileInfo) == TRUE);

		// Close handle.
		::FindClose(hFile);
		const DWORD dwError = ::GetLastError();
		if (dwError == ERROR_NO_MORE_FILES) {
			// Attributes have been successfully changed.
		}
	}
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

	const filesystem::path ret = filesystem::absolute(path);
	std::string result = ret.string();
	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
}

std::string Platform::executableFile(void) {
	char buf[GBBASIC_MAX_PATH];
	::GetModuleFileNameA(nullptr, buf, sizeof(buf));

	const std::string result = buf;

	return result;
}

std::string Platform::documentDirectory(void) {
	CHAR myDoc[GBBASIC_MAX_PATH];
	const HRESULT ret = ::SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, myDoc);

	if (ret == S_OK) {
		const std::string osstr = myDoc;

		return osstr;
	} else {
		std::string dir = getenv("APPDATA");
		dir = Path::combine(dir.c_str(), "bitty3d", "data");

		const std::string osstr = Unicode::toOs(dir);

		return osstr;
	}
}

std::string Platform::writableDirectory(void) {
	const char* cstr = SDL_GetPrefPath("gbbasic", "data");
	if (!cstr)
		return ""; // Fall to blank if failed to get the path.

	const std::string osstr = Unicode::toOs(cstr);
	SDL_free((void*)cstr);

	return osstr;
}

std::string Platform::savedGamesDirectory(void) {
	PWSTR savedGames = nullptr;
	const HRESULT ret = ::SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGames);

	if (ret == S_OK) {
		const std::string osstr = Unicode::toOs(Unicode::fromWide(savedGames));
		::CoTaskMemFree(savedGames);

		return osstr;
	} else {
		std::string dir = getenv("APPDATA");
		dir = Path::combine(dir.c_str(), "bitty3d", "data");

		const std::string osstr = Unicode::toOs(dir);

		return osstr;
	}
}

std::string Platform::currentDirectory(void) {
	char dir[GBBASIC_MAX_PATH];
	::GetCurrentDirectoryA(sizeof(dir), dir);

	const std::string result = dir;

	return result;
}

void Platform::currentDirectory(const char* dir) {
	const std::string osstr = dir;
	::SetCurrentDirectoryA(osstr.c_str());
}

/* ===========================================================================} */

/*
** {===========================================================================
** Surfing and browsing
*/

void Platform::surf(const char* url) {
	const HRESULT result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(result)) {
		fprintf(stderr, "Cannot initialize COM.\n");

		return;
	}
	::ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWDEFAULT);
	::CoUninitialize();
}

void Platform::browse(const char* dir) {
	const HRESULT result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(result)) {
		fprintf(stderr, "Cannot initialize COM.\n");

		return;
	}
	::ShellExecuteA(nullptr, "open", dir, nullptr, nullptr, SW_SHOWDEFAULT);
	::CoUninitialize();
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
	return true;
}

bool Platform::hasClipboardImage(void) {
	return clip::has(clip::image_format());
}

bool Platform::getClipboardImage(class Image* img) {
	if (!img)
		return false;

	clip::image img_;
	if (!clip::get_image(img_))
		return false;

	const clip::image_spec &spec = img_.spec();
	if (spec.bits_per_pixel > 32)
		return false;

	if (!img->resize((int)spec.width, (int)spec.height, false))
		return false;

	for (unsigned long y = 0; y < spec.height; ++y) {
		const char* ptr = img_.data() + spec.bytes_per_row * y;
		for (unsigned long x = 0; x < spec.width; ++x) {
			const unsigned* pixels = (const unsigned*)ptr;
			const unsigned pixel = *pixels;
			const unsigned r = (pixel & spec.red_mask) >> spec.red_shift;
			const unsigned g = (pixel & spec.green_mask) >> spec.green_shift;
			const unsigned b = (pixel & spec.blue_mask) >> spec.blue_shift;
			const unsigned a = (pixel & spec.alpha_mask) >> spec.alpha_shift;
			const Colour col((Byte)r, (Byte)g, (Byte)b, (Byte)a);
			img->set(x, y, col);
			ptr += spec.bits_per_pixel / 8;
		}
	}

	return true;
}

bool Platform::setClipboardImage(const class Image* img) {
	if (!img)
		return false;
	if (img->paletted())
		return false;

	clip::image_spec spec;
	spec.width = img->width();
	spec.height = img->height();
	spec.bits_per_pixel = 32;
	spec.bytes_per_row = spec.width * 4;
	spec.red_mask = 0x000000ff;
	spec.green_mask = 0x0000ff00;
	spec.blue_mask = 0x00ff0000;
	spec.alpha_mask = 0xff000000;
	spec.red_shift = 0;
	spec.green_shift = 8;
	spec.blue_shift = 16;
	spec.alpha_shift = 24;
	const clip::image img_(img->pixels(), spec);
	clip::set_image(img_);

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

bool platformRedirectedIoToConsole = false;

static const WORD MAX_CONSOLE_LINES = 500;

const char* Platform::os(void) {
	return "Windows";
}

static Platform::Languages _language = Platform::Languages::UNSET;

Platform::Languages Platform::language(void) {
	return _language;
}

void Platform::language(Languages lang) {
	_language = lang;
}

void Platform::threadName(const char* threadName) {
	platformThreadName(::GetCurrentThreadId(), threadName);
}

bool Platform::loadResource(const char* name, class Bytes* bytes) {
	bytes->clear();
	if (strcmp(name, "DATA") == 0) {
		HMODULE hModule = GetModuleHandle(nullptr);
		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(102), L"BYTES");
		if (hResource == 0)
			return false;
		HGLOBAL hMemory = LoadResource(hModule, hResource);
		if (hMemory == 0)
			return false;
		DWORD dwSize = SizeofResource(hModule, hResource);
		LPVOID lpAddress = LockResource(hMemory);
		{
			bytes->writeBytes((Byte*)lpAddress, (size_t)dwSize);
		}
		FreeResource(hMemory);

		return true;
	}

	return false;
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
	// Prepare.
	if (platformRedirectedIoToConsole)
		return;

	platformRedirectedIoToConsole = true;

	long hConHandle = 0;
	HANDLE lStdHandle = nullptr;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp = nullptr;

	// Allocate a console for this app.
	::AllocConsole();

	// Set the screen buffer to be big enough to let us scroll text.
	::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	::SetConsoleScreenBufferSize(::GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// Redirect unbuffered STDOUT to the console.
	lStdHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, nullptr, _IONBF, 0);

	// Redirect unbuffered STDIN to the console.
	lStdHandle = ::GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, nullptr, _IONBF, 0);

	// Redirect unbuffered STDERR to the console.
	lStdHandle = ::GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, nullptr, _IONBF, 0);

	// Make `cout`, `wcout`, `cin`, `wcin`, `wcerr`, `cerr`, `wclog` and `clog`;
	// point to console as well.
	std::ios::sync_with_stdio();

	// Reopen files.
	freopen("CON", "w", stdout);
	freopen("CON", "r", stdin);
	freopen("CON", "w", stderr);
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
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	static bool proceeded = true;
	if (!proceeded) {
		proceeded = true;
		_transaction->id = Text::toString(_id++);
		_transaction->productId = "1";
		_transaction->ok = true;

		msgbox("Purchased.", "Urban Player");

		transaction = *_transaction;

		return true;
	}

	return false;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)transaction;

	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

void Platform::prefillProducts(const Product::Array &products) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	*_prefilled = products;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)products;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

bool Platform::requestProducts(const Product::IdArray &ids) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	if (_pending)
		return false;

	Product::IdArray ids_;
	for (const std::string &id : ids)
		ids_.push_back(id);

	_pending = true;

	return true;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)ids;

	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

bool Platform::respondedProducts(Product::Array &products) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	if (_delay < PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT) {
		if (++_delay >= PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT) {
			_delay = 0;

			_products->clear();
			for (int i = 0; i < (int)_prefilled->size(); ++i) {
				Product product;
				product.id = (*_prefilled)[i].id;
				product.description = (*_prefilled)[i].description;
				product.localizedTitle = (*_prefilled)[i].localizedTitle;
				product.localizedDescription = (*_prefilled)[i].localizedDescription;
				product.price = (*_prefilled)[i].price;
				_products->push_back(product);
			}
			_pending = false;
		}
	}

	if (_pending)
		return false;

	products = *_products;

	return true;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)products;

	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

bool Platform::requestPurchase(const std::string &id) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	if (_pending)
		return false;

	*_productId = id;
	*_transaction = Transaction();

	_pending = true;

	return true;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)id;

	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

bool Platform::respondedPurchase(Transaction &transaction) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	if (_delay < PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT) {
		if (++_delay >= PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT) {
			_delay = 0;

			auto succeeded = [&] (void) -> void {
				_transaction->id = Text::toString(_id++);
				_transaction->productId = *_productId;
				_transaction->ok = true;
				_pending = false;

				msgbox("Purchased.", "Urban Player");
			};
			/*auto failed = [&] (void) -> void {
				_transaction->id = Text::toString(_id++);
				_transaction->productId = *_productId;
				_transaction->ok = false;
				_pending = false;

				msgbox("Cannot purchase in App Store.", "Urban Player");
			};*/
			succeeded();
		}
	}

	if (_pending)
		return false;

	transaction = *_transaction;

	return true;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)transaction;

	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

bool Platform::requestRestoring(void) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	if (_pending)
		return false;

	*_transactions = Transactions();

	_pending = true;

	return true;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
}

bool Platform::respondedRestoring(Transactions &transactions) {
#if PLATFORM_WINDOWS_IAP_DEBUG_ENABLED
	if (_delay < PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT) {
		if (++_delay >= PLATFORM_WINDOWS_IAP_DELAY_FRAME_COUNT) {
			_delay = 0;

			/*auto succeeded = [&] (const std::string* tids, int tcount, const std::string* ids, int count) -> void {
				_transactions->ids.clear();
				for (int i = 0; i < tcount; ++i)
					_transactions->ids.push_back(tids[i]);
				_transactions->productIds.clear();
				for (int i = 0; i < count; ++i)
					_transactions->productIds.push_back(ids[i]);
				_transactions->ok = true;
				_pending = false;

				msgbox("Restored.", "Urban Player");
			};*/
			auto failed = [&] (void) -> void {
				_transactions->ids.clear();
				_transactions->productIds.clear();
				_transactions->ok = false;
				_pending = false;

				msgbox("Nothing to restore.", "Urban Player");
			};
			failed();
		}
	}

	if (_pending)
		return false;

	transactions = *_transactions;

	return true;
#else /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
	(void)transactions;

	return false;
#endif /* PLATFORM_WINDOWS_IAP_DEBUG_ENABLED */
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
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo((SDL_Window*)wnd->pointer(), &wmInfo);
	FlashWindow(wmInfo.info.win.window, true);
}

void Platform::msgbox(const char* text, const char* caption) {
	::MessageBoxA(nullptr, text, caption, MB_OK);
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
	char buf[4];
	memset(buf, GBBASIC_COUNTOF(buf) * sizeof(char), 0);
	DWORD cbData = (DWORD)(GBBASIC_COUNTOF(buf) * sizeof(char));
	const LSTATUS res = ::RegGetValueW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		L"AppsUseLightTheme",
		RRF_RT_REG_DWORD,
		nullptr,
		buf,
		&cbData
	);

	if (res != ERROR_SUCCESS)
		return false;

	const int i = (int)(buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0]);

	return i != 1;
}

void Platform::useDarkMode(class Window* wnd) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo((SDL_Window*)wnd->pointer(), &wmInfo);

	HWND hWnd = wmInfo.info.win.window;
	const BOOL value = TRUE;
	::DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

	::SetWindowPos(
		hWnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED
	);
	::RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
}

void Platform::setWindowTransparentColor(class Window* wnd, const struct Colour* col) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo((SDL_Window*)wnd->pointer(), &wmInfo);

	HWND hWnd = wmInfo.info.win.window;
	LONG style = ::GetWindowLong(hWnd, GWL_EXSTYLE);
	if (col) {
		style |= WS_EX_LAYERED;
		::SetWindowLong(hWnd, GWL_EXSTYLE, style);
		::SetLayeredWindowAttributes(hWnd, RGB(col->r, col->g, col->b), 0, LWA_COLORKEY);
	} else {
		style &= ~WS_EX_LAYERED;
		::SetWindowLong(hWnd, GWL_EXSTYLE, style);
	}
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
