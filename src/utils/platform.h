/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "../gbbasic.h"
#include <string>
#include <vector>

/*
** {===========================================================================
** Platform
*/

/**
 * @brief Platform specific functions.
 *
 * @note GB BASIC uses UTF-8 for almost any string, except string representation
 *   in this module uses UTF16 on Windows. Encoding conversion must be made
 *   properly before and after calling these functions. See the `Unicode` module
 *   for more.
 */
class Platform {
public:
	enum class Languages : unsigned {
		CHINESE,
		ENGLISH,
		FRENCH,
		GERMAN,
		ITALIAN,
		JAPANESE,
		PORTUGUESE,
		SPANISH,

		COUNT,

		UNSET,
		AUTO
	};

	struct Product {
		typedef std::vector<Product> Array;
		typedef std::vector<std::string> IdArray;

		std::string id;
		std::string description;
		std::string localizedTitle;
		std::string localizedDescription;
		std::string price;
	};
	struct Transaction {
		std::string id;
		std::string productId;
		bool ok = false;
	};
	struct Transactions {
		typedef std::vector<std::string> IdArray;

		IdArray ids;
		Product::IdArray productIds;
		bool ok = false;

		bool empty(void) const;
	};

public:
	/**< Basic. */

	static void open(void);
	static void close(void);

	/**< Filesystem. */

	static bool copyFile(const char* src, const char* dst);
	static bool copyDirectory(const char* src, const char* dst);

	static bool moveFile(const char* src, const char* dst);
	static bool moveDirectory(const char* src, const char* dst);

	static bool removeFile(const char* src, bool toTrash);
	static bool removeDirectory(const char* src, bool toTrash);

	static bool makeDirectory(const char* path);
	static void accreditDirectory(const char* path);

	/**
	 * @param[in, out] path
	 */
	static bool ignore(const char* path);
	static bool equal(const char* lpath, const char* rpath);
	static bool isParentOf(const char* lpath, const char* rpath);
	static std::string absoluteOf(const std::string &path);

	static std::string executableFile(void);
	static std::string documentDirectory(void);
	static std::string writableDirectory(void);
	static std::string savedGamesDirectory(void);
	static std::string currentDirectory(void);
	static void currentDirectory(const char* dir);

	/**< Surfing and browsing. */

	static void surf(const char* url);
	static void browse(const char* dir);

	/**< Clipboard. */

	static bool hasClipboardText(void);
	static std::string getClipboardText(void);
	static void setClipboardText(const char* txt);

	static bool isClipboardImageSupported(void);
	static bool hasClipboardImage(void);
	static bool getClipboardImage(class Image* img);
	static bool setClipboardImage(const class Image* img);

	/**< OS. */

	static bool isLittleEndian(void);

	static const char* os(void);

	static const char* locale(const char* loc);
	static Languages locale(void);
	static void locale(Languages lang);

	static Languages language(void);
	static void language(Languages lang);

	static void threadName(const char* threadName);

	static bool loadResource(const char* name, class Bytes* bytes);

	static std::string execute(const char* cmd);

	static bool checkProgram(const char* prog);

	static void redirectIoToConsole(void);

	static void idle(void);

	static bool applyForPermission(const char* what);
	static bool isPermissionGranted(const char* what);

	static bool requestAd(const char* what, const char* arg);
	static bool listenedPurchase(Transaction &transaction);
	static void prefillProducts(const Product::Array &products);
	static bool requestProducts(const Product::IdArray &ids);
	static bool respondedProducts(Product::Array &products);
	static bool requestPurchase(const std::string &id);
	static bool respondedPurchase(Transaction &transaction);
	static bool requestRestoring(void);
	static bool respondedRestoring(Transactions &transactions);

	/**< GUI. */

	static std::string saveFileDialog(const std::string &title, const std::string &default_path = "", const std::vector<std::string> &filters = { "All Files", "*" });

	static void notify(const char* title, const char* message, const char* icon);
	static void notifyAfter(const char* title, const char* message, int tag, const char* sound, int badge, int interval);
	static void notifyRepeat(const char* title, const char* message, int tag, const char* sound, int badge, int interval, int repeating_interval);
	static void notifyDaily(const char* title, const char* message, int tag, const char* sound, int badge, int hour, int minute);
	static void cancelNotification(int tag);
	static void cancelAllNotifications(void);
	static void clearBadge(void);

	static void flashWindow(class Window* wnd);

	static void msgbox(const char* text, const char* caption);

	static void openInput(void);
	static void closeInput(void);
	static void inputScreenPosition(int x, int y);

	static void hideStatusBar(void);
	static int statusBarHeight(void);
	static bool notch(int* left /* nullable */, int* top /* nullable */, int* right /* nullable */, int* bottom /* nullable */);

	static bool isSystemInDarkMode(void);
	static void useDarkMode(class Window* wnd);

	static void setWindowTransparentColor(class Window* wnd, const struct Colour* col /* nullable */);

	/**< Hardware. */

	static int cpuCount(void);
	static bool isMobile(void);
	static void tactile(void);
};

/* ===========================================================================} */

#endif /* __PLATFORM_H__ */
