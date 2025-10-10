/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#import "bytes.h"
#import "encoding.h"
#import "file_sandbox.h"
#import "platform.h"
#import "ios/notification/notification.h"
#import "ios/store/store.h"
#import <SDL.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIApplication.h>
#import <UIKit/UIWindow.h>
#import <libgen.h>
#import <mach-o/dyld.h>
#import <pthread.h>
#import <sys/stat.h>
#import <unistd.h>

/*
** {===========================================================================
** Utilities
*/

static NSString* platformTrimPath(NSString* originalPath) {
	NSString* prefixToRemove = @"file://";
	if ([originalPath hasPrefix: prefixToRemove])
		originalPath = [originalPath substringFromIndex: [prefixToRemove length]];
	originalPath = [originalPath stringByRemovingPercentEncoding];
	//originalPath = [originalPath stringByReplacingPercentEscapesUsingEncoding: NSUTF8StringEncoding];

	return originalPath;
}

static Notification* _notification = nullptr;

static bool _pending = false;
static Platform::Transactions* _listening = nullptr;
static Platform::Product::Array* _products = nullptr;
static Platform::Transaction* _transaction = nullptr;
static Platform::Transactions* _transactions = nullptr;

/* ===========================================================================} */

/*
** {===========================================================================
** Basic
*/

void Platform::open(void) {
	_notification = new Notification();

	IAP::open();
	_listening = new Transactions();
	_products = new Product::Array();
	_transaction = new Transaction();
	_transactions = new Transactions();

	IAP::listenToPayment(
		[] (const std::string &id_, const std::string &productId) -> void {
			_listening->ids.push_back(id_);
			_listening->productIds.push_back(productId);
			_listening->ok = true;
		}
	);
}

void Platform::close(void) {
	delete _notification;
	_notification = nullptr;

	IAP::close();
	delete _listening;
	_listening = nullptr;
	delete _products;
	_products = nullptr;
	delete _transaction;
	_transaction = nullptr;
	delete _transactions;
	_transactions = nullptr;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Filesystem
*/

bool Platform::copyFile(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
	NSString* newPath= [NSString stringWithCString: dst encoding: NSUTF8StringEncoding];
	if ([filemgr fileExistsAtPath: newPath])
		[filemgr removeItemAtPath: newPath error: NULL];
	if ([filemgr copyItemAtPath: oldPath toPath: newPath error: NULL] == YES)
		return true;
	else
		return false;
}

bool Platform::copyDirectory(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
	NSString* newPath= [NSString stringWithCString: dst encoding: NSUTF8StringEncoding];
	if ([filemgr fileExistsAtPath: newPath])
		[filemgr removeItemAtPath: newPath error: NULL];
	if ([filemgr copyItemAtPath: oldPath toPath: newPath error: NULL] == YES)
		return true;
	else
		return false;
}

bool Platform::moveFile(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
	NSURL* newPath= [NSURL fileURLWithPath: [NSString stringWithCString: dst encoding: NSUTF8StringEncoding]];
	[filemgr moveItemAtURL: oldPath toURL: newPath error: nil];

	return true;
}

bool Platform::moveDirectory(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
	NSURL* newPath= [NSURL fileURLWithPath: [NSString stringWithCString: dst encoding: NSUTF8StringEncoding]];
	[filemgr moveItemAtURL: oldPath toURL: newPath error: nil];

	return true;
}

bool Platform::removeFile(const char* src, bool toTrash) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	if (toTrash) {
		NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
		if (@available(macOS 10.8, *)) {
			if ([filemgr trashItemAtURL: oldPath resultingItemURL: NULL error: NULL] == YES)
				return true;
			else
				return false;
		} else {
			NSString* nsstr = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
			if ([filemgr removeItemAtPath: nsstr error: NULL] == YES)
				return true;
			else
				return false;
		}
	} else {
		NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
		if ([filemgr removeItemAtPath: oldPath error: NULL] == YES)
			return true;
		else
			return false;
	}
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	if (toTrash) {
		NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
		if (@available(macOS 10.8, *)) {
			if ([filemgr trashItemAtURL: oldPath resultingItemURL: NULL error: NULL] == YES)
				return true;
			else
				return false;
		} else {
			NSString* nsstr = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
			if ([filemgr removeItemAtPath: nsstr error: NULL] == YES)
				return true;
			else
				return false;
		}
	} else {
		NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
		if ([filemgr removeItemAtPath: oldPath error: NULL] == YES)
			return true;
		else
			return false;
	}
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
	if (path.empty())
		return path;

	NSURL* nspath = [NSURL fileURLWithPath: [NSString stringWithCString: path.c_str() encoding: NSUTF8StringEncoding]];
	std::string ret = [platformTrimPath([nspath absoluteString]) cStringUsingEncoding: NSUTF8StringEncoding];
	if (!path.empty() && !ret.empty()) {
		if ((path.back() == '\\' || path.back() == '/') && (ret.back() != '\\' && ret.back() != '/'))
			ret.push_back('/');
		else if ((path.back() != '\\' && path.back() != '/') && (ret.back() == '\\' || ret.back() == '/'))
			ret.pop_back();
	}

	return ret;
}

std::string Platform::executableFile(void) {
	uint32_t bufsize = GBBASIC_MAX_PATH;
	char buf[GBBASIC_MAX_PATH];
	int result = _NSGetExecutablePath(buf, &bufsize);
	if (result == 0) {
		NSURL* fileURL = [NSURL fileURLWithPath: [NSString stringWithCString: buf encoding: NSUTF8StringEncoding]];
		/*NSURL* folderURL = [fileURL URLByDeletingLastPathComponent];
		std::string ret = [platformTrimPath([folderURL absoluteString]) cStringUsingEncoding: NSUTF8StringEncoding];
		if (!ret.empty() && ret.back() != '/')
			ret.push_back('/');*/
		const std::string ret = [platformTrimPath([fileURL absoluteString]) cStringUsingEncoding: NSUTF8StringEncoding];

		return ret;
	}

	return "";
}

std::string Platform::documentDirectory(void) {
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString* documentsDirectory = [paths objectAtIndex: 0];
	std::string ret = [documentsDirectory cStringUsingEncoding: NSUTF8StringEncoding];
	if (!ret.empty() && ret.back() != '/')
		ret.push_back('/');

	return ret;
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
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSString* currentpath = [filemgr currentDirectoryPath];
	std::string ret = [currentpath cStringUsingEncoding: NSUTF8StringEncoding];

	return ret;
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
	UIApplication* application = [UIApplication sharedApplication];
	NSString* nsstr = [NSString stringWithCString: url encoding: NSUTF8StringEncoding];
	NSURL* nsurl = [NSURL URLWithString: nsstr];
	[application openURL: nsurl options: @{ } completionHandler: ^(BOOL success) {
		if (success) {
			 NSLog(@"Opened URL.");
		}
	}];
}

void Platform::browse(const char* dir) {
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
	return "iOS";
}

static Platform::Languages _language = Platform::Languages::UNSET;
static const char* PLATFORM_IOS_LANGUAGE_STRINGS[] = {
	"en",
	"zh",
	"ja",
	"de",
	"fr",
	"it",
	"es",
	"pt"
};
static_assert(GBBASIC_COUNTOF(PLATFORM_IOS_LANGUAGE_STRINGS) == (unsigned)Platform::Languages::COUNT, "Wrong size.");
static Platform::Languages platformIosLanguageOf(const char* lang) {
	for (int i = 0; i < GBBASIC_COUNTOF(PLATFORM_IOS_LANGUAGE_STRINGS); ++i) {
		const char* l = PLATFORM_IOS_LANGUAGE_STRINGS[i];
		if (Text::startsWith(lang, l, true))
			return (Platform::Languages)i;
	}

	return Platform::Languages::ENGLISH;
}

Platform::Languages Platform::language(void) {
	return _language;
}

void Platform::language(Languages lang) {
	do {
		if (lang != Languages::AUTO)
			break;
		NSString* language = [[NSLocale preferredLanguages] objectAtIndex: 0];
		const std::string langstr = [language cStringUsingEncoding: NSUTF8StringEncoding];
		if (langstr.empty())
			break;

		lang = platformIosLanguageOf(langstr.c_str());

		fprintf(stdout, "Language: %s.\n", langstr.c_str());
	} while (false);

	if (lang == Languages::AUTO)
		lang = Languages::UNSET;

	_language = lang;
}

void Platform::threadName(const char* threadName) {
	pthread_setname_np(threadName);
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
	GBBASIC_MISSING

	return "";
}

bool Platform::checkProgram(const char* /* prog */) {
	return false;
}

void Platform::redirectIoToConsole(void) {
	GBBASIC_MISSING
}

bool Platform::applyForPermission(const char* what) {
	if (!what || !*what)
		return false;

	if (strcmp(what, "notification") == 0) {
		return _notification->initialize();
	} else {
		GBBASIC_ASSERT(false && "Unknown.");
	}

	return false;
}

bool Platform::isPermissionGranted(const char* what) {
	if (!what || !*what)
		return false;

	if (strcmp(what, "notification") == 0) {
		return _notification->isEnabled();
	} else {
		GBBASIC_ASSERT(false && "Unknown.");
	}

	return false;
}

bool Platform::requestAd(const char* what, const char* arg) {
	(void)what;
	(void)arg;

	return false;
}

bool Platform::listenedPurchase(Transaction &transaction) {
	if (_listening->empty())
		return false;

	if (_listening->ids.size() != _listening->productIds.size()) {
		GBBASIC_ASSERT(false && "Impossible.");
		_listening->ids.clear();
		_listening->productIds.clear();
		_listening->ok = false;

		return false;
	}

	Transaction transaction_;
	transaction_.id = _listening->ids.front();
	transaction_.productId = _listening->productIds.front();
	transaction_.ok = true;
	transaction = transaction_;

	_listening->ids.erase(_listening->ids.begin());
	_listening->productIds.erase(_listening->productIds.begin());
	if (_listening->empty())
		_listening->ok = false;

	return true;
}

void Platform::prefillProducts(const Product::Array &products) {
	(void)products;

	// Do nothing.
}

bool Platform::requestProducts(const Product::IdArray &ids) {
	if (_pending)
		return false;

	::Product::IdArray ids_;
	for (const std::string &id : ids)
		ids_.push_back(id);

	_pending = true;
	IAP::requestProducts(
		ids_,
		[] (const ::Product* products, int count) -> void {
			_products->clear();
			for (int i = 0; i < count; ++i) {
				Product product;
				product.id = products[i].id;
				product.description = products[i].description;
				product.localizedTitle = products[i].localizedTitle;
				product.localizedDescription = products[i].localizedDescription;
				product.price = products[i].price;
				_products->push_back(product);
			}
			_pending = false;
		}
	);

	return true;
}

bool Platform::respondedProducts(Product::Array &products) {
	if (_pending)
		return false;

	products = *_products;

	return true;
}

bool Platform::requestPurchase(const std::string &id) {
	if (_pending)
		return false;

	*_transaction = Transaction();

	_pending = true;
	IAP::requestPayment(
		id,
		[&] (const std::string &id_, const std::string &productId) -> void {
			_transaction->id = id_;
			_transaction->productId = productId;
			_transaction->ok = true;
			_pending = false;
		},
		[&, id] (void) -> void {
			_transaction->id.clear();
			_transaction->productId = id;
			_transaction->ok = false;
			_pending = false;
		}
	);

	return true;
}

bool Platform::respondedPurchase(Transaction &transaction) {
	if (_pending)
		return false;

	transaction = *_transaction;

	return true;
}

bool Platform::requestRestoring(void) {
	if (_pending)
		return false;

	*_transactions = Transactions();

	_pending = true;
	IAP::restorePurchases(
		[&] (const std::string* tids, int tcount, const std::string* ids, int count) -> void {
			_transactions->ids.clear();
			for (int i = 0; i < tcount; ++i)
				_transactions->ids.push_back(tids[i]);
			_transactions->productIds.clear();
			for (int i = 0; i < count; ++i)
				_transactions->productIds.push_back(ids[i]);
			_transactions->ok = true;
			_pending = false;
		},
		[&] (void) -> void {
			_transactions->ids.clear();
			_transactions->productIds.clear();
			_transactions->ok = false;
			_pending = false;
		}
	);

	return true;
}

bool Platform::respondedRestoring(Transactions &transactions) {
	if (_pending)
		return false;

	transactions = *_transactions;

	return true;
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
	_notification->showNotificationAfter(message, title, tag, sound, badge, interval);
}

void Platform::notifyRepeat(const char* title, const char* message, int tag, const char* sound, int badge, int interval, int repeating_interval) {
	_notification->showRepeatNotification(message, title, tag, sound, badge, interval, repeating_interval);
}

void Platform::notifyDaily(const char* title, const char* message, int tag, const char* sound, int badge, int hour, int minute) {
	_notification->showDailyNotification(message, title, tag, sound, badge, hour, minute);
}

void Platform::cancelNotification(int tag) {
	_notification->cancelNotification(tag);
}

void Platform::cancelAllNotifications(void) {
	_notification->cancelAllNotifications();
}

void Platform::clearBadge(void) {
	UIApplication* application = [UIApplication sharedApplication];
	application.applicationIconBadgeNumber = 0;
}

void Platform::flashWindow(class Window* wnd) {
	// Do nothing.
}

void Platform::msgbox(const char* text, const char* caption) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, caption, text, nullptr);
}

void Platform::openInput(void) {
	SDL_StartTextInput();
}

void Platform::closeInput(void) {
	SDL_StopTextInput();
}

void Platform::inputScreenPosition(int x, int y) {
	SDL_Rect rect{ x, y, 100, 20 };
	SDL_SetTextInputRect(&rect);
}

void Platform::hideStatusBar(void) {
	//UIApplication* application = [UIApplication sharedApplication];
	//application.statusBarHidden = TRUE;
}

int Platform::statusBarHeight(void) {
	//UIApplication* application = [UIApplication sharedApplication];
	//CGSize statusBarSize = [application statusBarFrame].size;

	//return (int)Math::min(statusBarSize.width, statusBarSize.height);

	return 0;
}

bool Platform::notch(int* left, int* top, int* right, int* bottom) {
	if (left)
		*left = 0;
	if (top)
		*top = 0;
	if (right)
		*right = 0;
	if (bottom)
		*bottom = 0;

	if (@available(iOS 13.0, *)) {
		UIApplication* application = [UIApplication sharedApplication];
		UIWindow* window = [application delegate].window;
		UIEdgeInsets area = [window safeAreaInsets];
		if (left)
			*left = (int)area.left;
		if (top)
			*top = (int)area.top;
		if (right)
			*right = (int)area.right;
		if (bottom)
			*bottom = (int)area.bottom;

		return true;
	} else if (@available(iOS 11.0, *)) {
		UIApplication* application = [UIApplication sharedApplication];
		UIEdgeInsets area = [[application keyWindow] safeAreaInsets];
		if (left)
			*left = (int)area.left;
		if (top)
			*top = (int)area.top;
		if (right)
			*right = (int)area.right;
		if (bottom)
			*bottom = (int)area.bottom;

		return true;
	}

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
