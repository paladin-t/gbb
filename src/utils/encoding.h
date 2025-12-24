/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ENCODING_H__
#define __ENCODING_H__

#include "../gbbasic.h"
#include <string>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ENCODING_STRING_CONVERTER_WINAPI
#	define ENCODING_STRING_CONVERTER_WINAPI 0
#endif /* ENCODING_STRING_CONVERTER_WINAPI */
#ifndef ENCODING_STRING_CONVERTER_CUSTOM
#	define ENCODING_STRING_CONVERTER_CUSTOM 1
#endif /* ENCODING_STRING_CONVERTER_CUSTOM */
#ifndef ENCODING_STRING_CONVERTER_CODECVT
#	define ENCODING_STRING_CONVERTER_CODECVT 2
#endif /* ENCODING_STRING_CONVERTER_CODECVT */
#ifndef ENCODING_STRING_CONVERTER
#	if defined GBBASIC_OS_WIN
#		define ENCODING_STRING_CONVERTER ENCODING_STRING_CONVERTER_WINAPI
#	elif defined GBBASIC_OS_MAC
#		define ENCODING_STRING_CONVERTER ENCODING_STRING_CONVERTER_CUSTOM
#	elif defined GBBASIC_OS_IOS || defined GBBASIC_OS_IOS_SIM
#		define ENCODING_STRING_CONVERTER ENCODING_STRING_CONVERTER_CUSTOM
#	elif defined GBBASIC_OS_ANDROID
#		define ENCODING_STRING_CONVERTER ENCODING_STRING_CONVERTER_CUSTOM
#	else /* Platform macro. */
#		define ENCODING_STRING_CONVERTER ENCODING_STRING_CONVERTER_CODECVT
#	endif /* Platform macro. */
#endif /* ENCODING_STRING_CONVERTER */

/* ===========================================================================} */

/*
** {===========================================================================
** Unicode
*/

/**
 * @brief Unicode utilities.
 */
class Unicode {
public:
	/**
	 * @brief Converts OS dependent string to UTF-8.
	 */
	static std::string fromOs(const char* str);
	/**
	 * @brief Converts OS dependent string to UTF-8.
	 */
	static std::string fromOs(const std::string &str);
	/**
	 * @brief Converts UTF-8 string to OS dependent.
	 */
	static std::string toOs(const char* str);
	/**
	 * @brief Converts UTF-8 string to OS dependent.
	 */
	static std::string toOs(const std::string &str);

	/**
	 * @brief Converts wide UTF-8 string to char sequence.
	 */
	static std::string fromWide(const wchar_t* str);
	/**
	 * @brief Converts wide UTF-8 string to char sequence.
	 */
	static std::string fromWide(const std::wstring &str);
	/**
	 * @brief Converts char sequence to wide UTF-8 string.
	 */
	static std::wstring toWide(const char* str);
	/**
	 * @brief Converts char sequence to wide UTF-8 string.
	 */
	static std::wstring toWide(const std::string &str);

	/**
	 * @brief Checks whether the specific string contains ASCII characters only,
	 *   this functions stops at '\0'.
	 */
	static bool isAscii(const char* str);
	/**
	 * @brief Checks whether the specific string is a valid UTF-8 string, this
	 *   functions stops at '\0'.
	 */
	static bool isUtf8(const char* str);
	/**
	 * @brief Checks whether the specific string is a printable string that
	 *   contains valid ASCII and UTF-8 characters, this functions stops at the
	 *   end of the string.
	 *
	 * @param[in] str Can contain '\0'.
	 */
	static bool isPrintable(const char* str, size_t len);

	/**
	 * @brief Expects a UTF-8 character.
	 *
	 * @return The byte count of a valid UTF-8 character, or zero for no valid one.
	 */
	static int expectUtf8(const char* ch);
	/**
	 * @brief Takes the specific amount of bytes from the string as a codepoint.
	 */
	static unsigned takeUtf8(const char* ch, int n);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Base64
*/

/**
 * @brief Base64 utilities.
 */
class Base64 {
public:
	/**
	 * @brief Decodes base64 string to bytes.
	 *
	 * @param[out] val
	 */
	static bool toBytes(class Bytes* val, const std::string &str);
	/**
	 * @brief Encodes bytes to base64 string.
	 *
	 * @param[out] val
	 */
	static bool fromBytes(std::string &val, const class Bytes* buf);
};

/* ===========================================================================} */

/*
** {===========================================================================
** LZ4
*/

/**
 * @brief LZ4 utilities.
 */
class Lz4 {
public:
	/**
	 * @brief Decompresses LZ4 bytes to origin bytes.
	 *
	 * @param[out] val
	 */
	static bool toBytes(class Bytes* val, const class Bytes* src);
	/**
	 * @brief Compresses origin bytes to LZ4 bytes.
	 *
	 * @param[out] val
	 */
	static bool fromBytes(class Bytes* val, const class Bytes* src);
};

/* ===========================================================================} */

#endif /* __ENCODING_H__ */
