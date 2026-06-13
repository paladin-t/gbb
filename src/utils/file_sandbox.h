/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __FILE_SANDBOX_H__
#define __FILE_SANDBOX_H__

#include "../gbbasic.h"
#include "file_handle.h"
#include "text.h"

/*
** {===========================================================================
** File monitor
*/

class FileMonitor {
public:
	static void initialize(void);
	static void dispose(void);

	static void unuse(const char* path);
	static void unuse(const std::string &path);

	static void dump(void);
};

/* ===========================================================================} */

/*
** {===========================================================================
** File dialog
*/

#if defined GBBASIC_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4800)
#	pragma warning(disable : 4819)
#endif /* GBBASIC_CP_VC */
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
#	include "../../lib/portable_file_dialogs/portable-file-dialogs.h"
#	if defined GBBASIC_CP_VC
		// To avoid compile error.
#		if defined CONST
#			undef CONST
#		endif /* CONST */
#		if defined ERROR
#			undef ERROR
#		endif /* ERROR */
#	endif /* GBBASIC_CP_VC */
#elif defined GBBASIC_OS_HTML
namespace pfd {

enum class opt : unsigned char {
	none = 0,
	multiselect     = 0x1,
	force_overwrite = 0x2,
	force_path      = 0x4
};
inline opt operator | (opt a, opt b) { return opt(uint8_t(a) | uint8_t(b)); }
inline bool operator & (opt a, opt b) { return bool(uint8_t(a) & uint8_t(b)); }

enum class icon {
	info = 0,
	warning,
	error,
	question
};

class open_file {
public:
	open_file(
		std::string const &title,
		std::string const &default_path = "",
		std::vector<std::string> const &filters = { "All Files", "*" },
		opt options = opt::none
	);
	open_file(
		std::string const &title,
		std::string const &default_path,
		std::vector<std::string> const &filters,
		bool allow_multiselect
	);

	std::vector<std::string> result();

private:
	std::vector<std::string> _result;
};

class save_file {
public:
	save_file(
		std::string const &title,
		std::string const &default_path = "",
		std::vector<std::string> const &filters = { "All Files", "*" },
		opt options = opt::none
	);
	save_file(
		std::string const &title,
		std::string const &default_path,
		std::vector<std::string> const &filters,
		bool confirm_overwrite
	);

	std::string result();

private:
	std::string _result;
};

class select_folder {
public:
	select_folder(
		std::string const &title,
		std::string const &default_path = "",
		opt options = opt::none
	);

	std::string result();
};

class notify {
public:
	notify(
		std::string const &title,
		std::string const &message,
		icon _icon = icon::info
	);
};

}
#else /* Platform macro. */
#	include "../../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#endif /* Platform macro. */
#if defined GBBASIC_CP_VC
#	pragma warning(pop)
#endif /* GBBASIC_CP_VC */

/* ===========================================================================} */

/*
** {===========================================================================
** File sync
*/

class FileSync : public virtual Object {
public:
	typedef std::shared_ptr<FileSync> Ptr;

	enum class Filters : unsigned {
		PROJECT   = 0,
		CODE      = 1,
		C         = 2,
		JSON      = 3,
		IMAGE     = 4,
		FONT      = 5,
		CSV       = 6,
		VGM       = 7,
		WAV       = 8,
		FX_HAMMER = 9,
		ANY       = 10
	};

private:
	GBBASIC_PROPERTY(std::string, defaultPath)
	GBBASIC_PROPERTY(Filters, filters)

	GBBASIC_PROPERTY_READONLY(std::string, path)
	GBBASIC_PROPERTY_READONLY(Stream::Accesses, access)
	GBBASIC_PROPERTY_READONLY(unsigned, handle)
	GBBASIC_PROPERTY_READONLY(File::Ptr, pointer)

public:
	GBBASIC_CLASS_TYPE('F', 'S', 'Y', 'N')

	virtual void initialize(const std::string &defaultPath, Filters filters = Filters::ANY) = 0;
	virtual void dispose(void) = 0;

	virtual bool supported(void) const = 0;

	virtual bool requestOpen(void) = 0;
	virtual bool requestSave(bool confirmOverwrite = true) = 0;

	virtual File::Ptr open(void) = 0;
	virtual bool close(void) = 0;

	static FileSync* create(void);
	static void destroy(FileSync* ptr);
};

/* ===========================================================================} */

#endif /* __FILE_SANDBOX_H__ */
