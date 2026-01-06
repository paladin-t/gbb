/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#import "bytes.h"
#import "encoding.h"
#import "file_sandbox.h"
#import "image.h"
#import "platform.h"
#import "../../lib/clip/clip.h"
#import <SDL.h>
#import <AppKit/AppKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
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
	originalPath = [originalPath stringByReplacingPercentEscapesUsingEncoding: NSUTF8StringEncoding];

	return originalPath;
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
			/*dispatch_semaphore_t sema = dispatch_semaphore_create(0);
			__block bool ret = false;
			NSArray* files = [NSArray arrayWithObject: oldPath];
			[[NSWorkspace sharedWorkspace] recycleURLs: files completionHandler:
				^(NSDictionary* newURLs, NSError* error) {
					if (error != nil) {
						NSLog(@"%@", error);
						dispatch_semaphore_signal(sema);

						return;
					}
					for (NSString* file in newURLs) {
						NSLog(@"File %@ recycled to %@", file, [newURLs objectForKey: file]);
					}

					ret = true;
					dispatch_semaphore_signal(sema);
				}
			];
			dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

			return ret;*/

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
			/*dispatch_semaphore_t sema = dispatch_semaphore_create(0);
			__block bool ret = false;
			NSArray* files = [NSArray arrayWithObject: oldPath];
			[[NSWorkspace sharedWorkspace] recycleURLs: files completionHandler:
			 	^(NSDictionary* newURLs, NSError* error) {
					if (error != nil) {
						NSLog(@"%@", error);
						dispatch_semaphore_signal(sema);

						return;
					}
					for (NSString* file in newURLs) {
						NSLog(@"File %@ recycled to %@", file, [newURLs objectForKey: file]);
					}

					ret = true;
					dispatch_semaphore_signal(sema);
				}
			];
			dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

			return ret;*/

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
	if (!cstr)
		return ""; // Fall to blank if failed to get the path.

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
	NSString* nsstr = [NSString stringWithCString: url encoding: NSUTF8StringEncoding];
	NSURL* nsurl = [NSURL URLWithString: nsstr];
	[[NSWorkspace sharedWorkspace] openURL: nsurl];
}

void Platform::browse(const char* dir) {
	NSString* path = [NSString stringWithCString: dir encoding: NSUTF8StringEncoding];
	[[NSWorkspace sharedWorkspace] selectFile: path inFileViewerRootedAtPath: @""];
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
			img->set((int)x, (int)y, col);
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

const char* Platform::os(void) {
	return "MacOS";
}

static Platform::Languages _language = Platform::Languages::UNSET;

Platform::Languages Platform::language(void) {
	return _language;
}

void Platform::language(Languages lang) {
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
	NSString* nstxt = [NSString stringWithCString: text encoding: NSUTF8StringEncoding];
	NSString* nscpt= [NSString stringWithCString: caption encoding: NSUTF8StringEncoding];

	NSAlert* alert = [[NSAlert alloc] init];
	[alert setMessageText: nscpt];
	[alert setInformativeText: nstxt];
	if ([alert runModal] == NSAlertFirstButtonReturn) {
		// Do nothing.
	}
}

void Platform::openInput(void) {
	// Do nothing.
}

void Platform::closeInput(void) {
	// Do nothing.
}

void Platform::inputScreenPosition(int x, int y) {
	SDL_Rect rect{ x, y, 100, 20 };
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
