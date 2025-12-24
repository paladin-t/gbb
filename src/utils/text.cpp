/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "encoding.h"
#include "text.h"
#include <bitset>
#include <cctype>
#include <cfloat>
#include <iomanip>
#include <sstream>
#include <stdarg.h>

/*
** {===========================================================================
** Macros and constants
*/

static const std::locale TEXT_LOCALE("C");

/* ===========================================================================} */

/*
** {===========================================================================
** Text
*/

class TextImpl : public Text {
private:
	std::string _text;

public:
	TextImpl() {
	}
	virtual ~TextImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual const std::string &text(void) const override {
		return _text;
	}
	virtual std::string &text(void) override {
		return _text;
	}
	virtual void text(const std::string &txt) override {
		_text = txt;
	}
	virtual void clear(void) override {
		_text.clear();
	}
};

void Text::locale(const char* loc) {
	const char* ret = setlocale(LC_NUMERIC, loc);
	setlocale(LC_TIME, loc);

	fprintf(stdout, "Set numeric and time locale to \"%s\".\n", ret ? ret : "nil");
}

size_t Text::toLowerCase(char* str, size_t len) {
	size_t i = 0;
	while (i < len) {
		int n = Unicode::expectUtf8(str + i);
		if (n == 0)
			break;
		else if (n == 1)
			str[i] = (char)::tolower(str[i]);
		i += n;
	}

	return i;
}

size_t Text::toLowerCase(std::string &str) {
	size_t i = 0;
	while (i < str.length()) {
		int n = Unicode::expectUtf8(str.c_str() + i);
		if (n == 0)
			break;
		else if (n == 1)
			str[i] = (char)::tolower(str[i]);
		i += n;
	}

	return i;
}

size_t Text::toUpperCase(char* str, size_t len) {
	size_t i = 0;
	while (i < len) {
		int n = Unicode::expectUtf8(str + i);
		if (n == 0)
			break;
		else if (n == 1)
			str[i] = (char)::toupper(str[i]);
		i += n;
	}

	return i;
}

size_t Text::toUpperCase(std::string &str) {
	size_t i = 0;
	while (i < str.length()) {
		int n = Unicode::expectUtf8(str.c_str() + i);
		if (n == 0)
			break;
		else if (n == 1)
			str[i] = (char)::toupper(str[i]);
		i += n;
	}

	return i;
}

bool Text::fromString(const std::string &str, bool &val) {
	if (str.empty())
		return false;

	if (str == "yes" || str == "true") {
		val = true;

		return true;
	}
	if (str == "no" || str == "false") {
		val = false;

		return true;
	}

	return false;
}

bool Text::fromString(const std::string &str, Int32 &val) {
	if (str.empty())
		return false;

	char* convSuc = nullptr;
	if (startsWith(str, "0b", true)) {
		const std::string str_ = str.substr(2);
		val = (Int32)strtoull(str_.c_str(), &convSuc, 2);
	} else {
		val = (Int32)strtoull(str.c_str(), &convSuc, 0);
	}

	return convSuc && *convSuc == '\0';
}

bool Text::fromString(const std::string &str, UInt32 &val) {
	if (str.empty())
		return false;

	char* convSuc = nullptr;
	if (startsWith(str, "0b", true)) {
		const std::string str_ = str.substr(2);
		val = (UInt32)strtoull(str_.c_str(), &convSuc, 2);
	} else {
		val = (UInt32)strtoull(str.c_str(), &convSuc, 0);
	}

	return convSuc && *convSuc == '\0';
}

bool Text::fromString(const std::string &str, Int64 &val) {
	if (str.empty())
		return false;

	char* convSuc = nullptr;
	if (startsWith(str, "0b", true)) {
		const std::string str_ = str.substr(2);
		val = strtoll(str_.c_str(), &convSuc, 2);
	} else {
		val = strtoll(str.c_str(), &convSuc, 0);
	}

	return convSuc && *convSuc == '\0';
}

bool Text::fromString(const std::string &str, UInt64 &val) {
	if (str.empty())
		return false;

	char* convSuc = nullptr;
	if (startsWith(str, "0b", true)) {
		const std::string str_ = str.substr(2);
		val = strtoull(str_.c_str(), &convSuc, 2);
	} else {
		val = strtoull(str.c_str(), &convSuc, 0);
	}

	return convSuc && *convSuc == '\0';
}

bool Text::fromString(const std::string &str, Single &val) {
	if (str.empty())
		return false;

	char* convSuc = nullptr;
	val = (Single)strtod(str.c_str(), &convSuc);

	return convSuc && *convSuc == '\0';
}

bool Text::fromString(const std::string &str, Double &val) {
	if (str.empty())
		return false;

	char* convSuc = nullptr;
	val = strtod(str.c_str(), &convSuc);

	return convSuc && *convSuc == '\0';
}

bool Text::fromHexCharacter(char ch, UInt8 &val) {
	val = 0;

	if (ch >= '0' && ch <= '9') {
		val = ch - '0';

		return true;
	}
	if (ch >= 'a' && ch <= 'f') {
		val = 10 + ch - 'a';

		return true;
	}
	if (ch >= 'A' && ch <= 'F') {
		val = 10 + ch - 'A';

		return true;
	}

	return false;
}

std::string Text::toString(Int32 val, int width, char fill, std::ios::fmtflags flags, bool fixed) {
	try {
		std::stringstream stream;
		if (fixed) {
			stream.imbue(std::locale(""));
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << std::fixed << val;
		} else {
			stream.imbue(TEXT_LOCALE);
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << val;
		}

		return stream.str();
	} catch (...) {
		return std::to_string(val);
	}
}

std::string Text::toString(UInt32 val, int width, char fill, std::ios::fmtflags flags, bool fixed) {
	try {
		std::stringstream stream;
		if (fixed) {
			stream.imbue(std::locale(""));
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << std::fixed << val;
		} else {
			stream.imbue(TEXT_LOCALE);
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << val;
		}

		return stream.str();
	} catch (...) {
		return std::to_string(val);
	}
}

std::string Text::toString(Int64 val, int width, char fill, std::ios::fmtflags flags, bool fixed) {
	try {
		std::stringstream stream;
		if (fixed) {
			stream.imbue(std::locale(""));
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << std::fixed << val;
		} else {
			stream.imbue(TEXT_LOCALE);
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << val;
		}

		return stream.str();
	} catch (...) {
		return std::to_string(val);
	}
}

std::string Text::toString(UInt64 val, int width, char fill, std::ios::fmtflags flags, bool fixed) {
	try {
		std::stringstream stream;
		if (fixed) {
			stream.imbue(std::locale(""));
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << std::fixed << val;
		} else {
			stream.imbue(TEXT_LOCALE);
			stream.width(width);
			stream.fill(fill);
			if (flags)
				stream.setf(flags);
			stream << val;
		}

		return stream.str();
	} catch (...) {
		return std::to_string(val);
	}
}

std::string Text::toString(Single val, int precision, int width, char fill, std::ios::fmtflags flags) {
	std::stringstream stream;
	stream.imbue(TEXT_LOCALE);
	stream.precision(precision);
	stream.width(width);
	stream.fill(fill);
	if (flags)
		stream.setf(flags);
	stream << val;

	return stream.str();
}

std::string Text::toString(Double val, int precision, int width, char fill, std::ios::fmtflags flags) {
	try {
		std::stringstream stream;
		stream.imbue(TEXT_LOCALE);
		stream.precision(precision);
		stream.width(width);
		stream.fill(fill);
		if (flags)
			stream.setf(flags);
		stream << val;

		return stream.str();
	} catch (...) {
		return std::to_string(val);
	}
}

std::string Text::toString(bool val, bool yesNo) {
	if (val) {
		if (yesNo)
			return "yes";
		else
			return "true";
	} else {
		if (yesNo)
			return "no";
		else
			return "false";
	}
}

std::string Text::toString(Int32 val, bool fixed) {
	return toString(val, 0, ',', std::ios::fmtflags(0), fixed);
}

std::string Text::toString(UInt32 val, bool fixed) {
	return toString(val, 0, ',', std::ios::fmtflags(0), fixed);
}

std::string Text::toString(Int64 val, bool fixed) {
	return toString(val, 0, ',', std::ios::fmtflags(0), fixed);
}

std::string Text::toString(UInt64 val, bool fixed) {
	return toString(val, 0, ',', std::ios::fmtflags(0), fixed);
}

std::string Text::toBin(Int8 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(UInt8 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(Int16 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(UInt16 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(Int32 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(UInt32 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(Int64 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toBin(UInt64 val) {
	const std::string result = std::bitset<sizeof(val) * 8>(val).to_string();

	return result;
}

std::string Text::toHex(Int32 val, int width, char fill, bool toupper) {
	std::stringstream stream;
	if (toupper)
		stream.setf(std::ios::uppercase);
	stream <<
		std::setfill(fill) <<
		std::setw(width) <<
		std::hex <<
		val;

	return stream.str();
}

std::string Text::toHex(UInt32 val, int width, char fill, bool toupper) {
	std::stringstream stream;
	if (toupper)
		stream.setf(std::ios::uppercase);
	stream <<
		std::setfill(fill) <<
		std::setw(width) <<
		std::hex <<
		val;

	return stream.str();
}

std::string Text::toHex(Int64 val, int width, char fill, bool toupper) {
	std::stringstream stream;
	if (toupper)
		stream.setf(std::ios::uppercase);
	stream <<
		std::setfill(fill) <<
		std::setw(width) <<
		std::hex <<
		val;

	return stream.str();
}

std::string Text::toHex(UInt64 val, int width, char fill, bool toupper) {
	std::stringstream stream;
	if (toupper)
		stream.setf(std::ios::uppercase);
	stream <<
		std::setfill(fill) <<
		std::setw(width) <<
		std::hex <<
		val;

	return stream.str();
}

std::string Text::toHex(Int32 val, bool toupper) {
	return toHex(val, sizeof(decltype(val)) * 2, '0', toupper);
}

std::string Text::toHex(UInt32 val, bool toupper) {
	return toHex(val, sizeof(decltype(val)) * 2, '0', toupper);
}

std::string Text::toHex(Int64 val, bool toupper) {
	return toHex(val, sizeof(decltype(val)) * 2, '0', toupper);
}

std::string Text::toHex(UInt64 val, bool toupper) {
	return toHex(val, sizeof(decltype(val)) * 2, '0', toupper);
}

std::string Text::toPageNumber(int val) {
#if GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED
	const std::string page = toHex(val, 2, '0', true);
#else /* GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED */
	const std::string page = toString(val);
#endif /* GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED */

	return page;
}

std::string Text::toScaledBytes(Int64 val) {
	if (val < 1024ll) {
		return toString(val) + "B";
	}
	if (val < std::pow(1024ll, 2ll)) {
		const Double val_ = val / 1024.0;

		return toString(val_, 4) + "KB";
	}
	if (val < std::pow(1024ll, 3ll)) {
		const Double val_ = (Int64)(val / 1024.0) / 1024.0;

		return toString(val_, 4) + "MB";
	}
	if (val < std::pow(1024ll, 4ll)) {
		const Double val_ = (Int64)(val / std::pow(1024.0, 2.0)) / 1024.0;

		return toString(val_, 4) + "GB";
	}
	if (val < std::pow(1024ll, 5ll)) {
		const Double val_ = (Int64)(val / std::pow(1024.0, 3.0)) / 1024.0;

		return toString(val_, 4) + "TB";
	}

	const Double val_ = (Int64)(val / std::pow(1024.0, 4.0)) / 1024.0;

	return toString(val_, 4) + "PB";
}

std::string Text::toNumberWithCommas(const std::string &str) {
	// If the string contains a comma, return the original string directly.
	if (str.find(',') != std::string::npos)
		return str;

	// Check if the string consists entirely of digits, allowing a minus sign at the beginning and a decimal point.
	bool hasDecimal = false;
	bool hasSign = false;
	size_t start = 0;

	// Process a possible minus sign.
	if (!str.empty() && str[0] == '-') {
		hasSign = true;
		start = 1;
	}

	// Verify all characters are valid digits or decimal points.
	for (size_t i = start; i < str.size(); ++i) {
		if (str[i] == '.') {
			if (hasDecimal) {
				// Multiple decimal points found, invalid number format.
				return str;
			}
			hasDecimal = true;
		} else if (!isdigit(str[i])) {
			// Non-digit character encountered.
			return str;
		}
	}

	// Separate integer portion and decimal portion.
	std::string integerPart;
	std::string decimalPart;
	size_t decimalPos = str.find('.');

	if (decimalPos != std::string::npos) {
		integerPart = str.substr(0, decimalPos);
		decimalPart = str.substr(decimalPos);
	} else {
		integerPart = str;
	}

	// Format the integer part with comma separators, handling negative sign.
	std::string formattedInteger;
	int count = 0;
	size_t n = integerPart.size();

	// Process digits from right to left.
	for (int i = (int)n - 1; i >= (hasSign ? 1 : 0); --i) {
		formattedInteger = integerPart[i] + formattedInteger;
		++count;
		// Insert comma after every 3 digits.
		if (count % 3 == 0 && i > (hasSign ? 1 : 0))
			formattedInteger = ',' + formattedInteger;
	}

	// Add negative sign back if present.
	if (hasSign) {
		formattedInteger = '-' + formattedInteger;
		// Handle case where only negative sign exists.
		if (n == 1)
			formattedInteger += '0';
	}

	// Combine formatted integer with decimal portion.
	return formattedInteger + decimalPart;
}

std::string Text::toVariableName(const std::string &str) {
	if (str.empty())
		return "_";

	std::string result;
	bool nextCharMustBeValid = true;
	bool prevWasUnderscore = false;
	for (char c : str) {
		if (isalpha(c) || c == '_') {
			result += c;
			nextCharMustBeValid = false;
			prevWasUnderscore = (c == '_');
		} else if (isdigit(c)) {
			if (result.empty())
				result += '_';
			result += c;
			nextCharMustBeValid = false;
			prevWasUnderscore = false;
		} else {
			if (!nextCharMustBeValid && !prevWasUnderscore) {
				result += '_';
				prevWasUnderscore = true;
			}
			nextCharMustBeValid = true;
		}
	}

	if (result.empty())
		return "_";

	if (result.back() == '_')
		result.pop_back();

	return result.empty() ? "_" : result;
}

std::string Text::pascalToSnake(const std::string &str) {
	if (str.empty())
		return "";

	std::string result;
	result += (char)tolower(str[0]);
	for (int i = 1; i < (int)str.length(); ++i) {
		const char c = str[i];
		if (isupper(c)) {
			result += '_';
			result += (char)tolower(c);
		} else {
			result += c;
		}
	}

	return result;
}

std::string Text::snakeToPascal(const std::string &str) {
	if (str.empty())
		return "";

	std::string result;
	bool makeUpper = true;
	for (char c : str) {
		if (c == '_') {
			makeUpper = true;
		} else {
			if (makeUpper) {
				result += (char)toupper(c);
				makeUpper = false;
			} else {
				result += (char)tolower(c);
			}
		}
	}

	return result;
}

std::string Text::sanitizeFilename(const std::string &str, char replacementChar) {
	std::string result = str;

	// Define illegal characters.
	std::string illegalChars = "/\\?%*:|\"<>";

#if defined GBBASIC_OS_WIN
	illegalChars += "&$!';`";
#elif defined GBBASIC_OS_MAC
	// Do nothing.
#elif defined GBBASIC_OS_LINUX
	// Do nothing.
#endif /* Platform macro. */

	// Replace illegal characters.
	for (char &c : result) {
		if (illegalChars.find(c) != std::string::npos) {
			c = replacementChar;
		}
	}

	// Replace control characters.
	for (char &c : result) {
		if (std::iscntrl(static_cast<unsigned char>(c))) {
			c = replacementChar;
		}
	}

	// Replace reserved names.
#if defined GBBASIC_OS_WIN
	constexpr const char* WINDOWS_RESERVED_NAMES[] = {
		"CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
	};

	std::string upperStr = result;
	std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

	for (const char* reserved : WINDOWS_RESERVED_NAMES) {
		if (upperStr == reserved) {
			result = "_" + result;

			break;
		}

		const size_t dotPos = upperStr.find('.');
		if (dotPos != std::string::npos) {
			const std::string nameOnly = upperStr.substr(0, dotPos);
			if (nameOnly == reserved) {
				result = "_" + result;

				break;
			}
		}
	}
#endif /* Platform macro. */

	// Remove if ends with dot.
	if (!result.empty() && (result.back() == '.' || result.back() == ' ')) {
		result.back() = replacementChar;
	}

	size_t pos = 0;
	while ((pos = result.find("..", pos)) != std::string::npos) {
		result.replace(pos, 2, std::string(2, replacementChar));
		pos += 2;
	}

	// Ensure it's not empty.
	if (result.empty()) {
		result = "Unnamed";
	}

	// Limit the length.
	const size_t maxLen = 255; // Safe for most operating systems.
	if (result.length() > maxLen) {
		const size_t lastDot = result.find_last_of('.');
		if (lastDot != std::string::npos && lastDot > result.length() - 10) {
			const std::string ext = result.substr(lastDot);
			const std::string name = result.substr(0, lastDot);

			const size_t keepLen = maxLen - ext.length();
			if (keepLen > 0)
				result = name.substr(0, keepLen) + ext;
			else
				result = name.substr(0, maxLen);
		} else {
			result = result.substr(0, maxLen);
		}
	}

	return result;
}

std::string Text::remove(const std::string &str, const std::string &charsToRemove) {
	std::string result = str;
	for (char ch : charsToRemove)
		result.erase(std::remove(result.begin(), result.end(), ch), result.end());

	return result;
}

std::string Text::trim(const std::string &str, const std::string &delims) {
	if (str.empty())
		return str;

	std::string ret = str;
	ret.erase(0, ret.find_first_not_of(delims));
	ret.erase(ret.find_last_not_of(delims) + 1);

	return ret;
}

std::string Text::trimLeft(const std::string &str, const std::string &delims) {
	if (str.empty())
		return str;

	std::string ret = str;
	ret.erase(0, ret.find_first_not_of(delims));

	return ret;
}

std::string Text::trimRight(const std::string &str, const std::string &delims) {
	if (str.empty())
		return str;

	std::string ret = str;
	ret.erase(ret.find_last_not_of(delims) + 1);

	return ret;
}

std::string Text::limitLeft(const std::string &txt, int width) {
	if ((int)txt.length() <= width)
		return txt;

	return txt.substr(txt.length() - width);
}

std::string Text::limitRight(const std::string &txt, int width) {
	if ((int)txt.length() <= width)
		return txt;

	return txt.substr(0, width);
}

std::string Text::padLeft(const std::string &txt, int width, char fill) {
	std::string result = txt;
	for (int i = 0; i < width - (int)txt.length(); ++i)
		result = fill + result;

	return result;
}

std::string Text::padRight(const std::string &txt, int width, char fill) {
	std::string result = txt;
	for (int i = 0; i < width - (int)txt.length(); ++i)
		result += fill;

	return result;
}

std::string Text::replace(const std::string &str, const std::string &from, const std::string &to, bool all) {
	if (from.empty())
		return str;

	size_t startPos = 0;
	std::string result = str;
	while ((startPos = result.find(from, startPos)) != std::string::npos) {
		result.replace(startPos, from.length(), to);
		startPos += to.length();
		if (!all)
			break;
	}

	return result;
}

std::string Text::join(const Array &array, const std::string &sep) {
	std::string result;
	for (int i = 0; i < (int)array.size(); ++i) {
		const std::string &str = array[i];
		result += str;
		if (i != (int)array.size() - 1)
			result += sep;
	}

	return result;
}

Text::Array Text::split(const std::string &str, const std::string &delims, bool parseUpToNext, size_t maxSplits) {
	// Prepare.
	Array ret;
	size_t numSplits = 0;

	// Use the STL methods.
	size_t start = 0, pos = 0;
	do {
		pos = str.find_first_of(delims, start);
		if (pos == start) {
			if (start != std::string::npos) // Copy the blank line.
				ret.push_back("");
			start = pos + 1;
		} else if (pos == std::string::npos || (maxSplits && numSplits == maxSplits)) {
			// Copy the rest of the string.
			ret.push_back(str.substr(start));

			break;
		} else {
			// Copy up to delimiter.
			ret.push_back(str.substr(start, pos - start));
			start = pos + 1;
		}
		// Parse up to next real data.
		if (parseUpToNext)
			start = str.find_first_not_of(delims, start);
		++numSplits;
	} while (pos != std::string::npos);

	return ret;
}

Text::Array Text::tokenise(const std::string &str, const std::string &singleDelims, const std::string &doubleDelims, bool parseUpToNext, size_t maxSplits) {
	// Prepare.
	Array ret;
	size_t numSplits = 0;
	std::string delims = singleDelims + doubleDelims;

	// Use the STL methods.
	size_t start = 0, pos = 0;
	char curDoubleDelim = '\0';
	do {
		if (curDoubleDelim != '\0')
			pos = str.find(curDoubleDelim, start);
		else
			pos = str.find_first_of(delims, start);

		if (pos == start) {
			char curDelim = str.at(pos);
			if (doubleDelims.find_first_of(curDelim) != std::string::npos)
				curDoubleDelim = curDelim;

			start = pos + 1;
		} else if (pos == std::string::npos || (maxSplits && numSplits == maxSplits)) {
			if (curDoubleDelim != '\0') {
				// Missing closer. Warn or throw exception?
			}
			// Copy the rest of the string.
			ret.push_back(str.substr(start));

			break;
		} else {
			if (curDoubleDelim != '\0')
				curDoubleDelim = '\0';

			// Copy up to delimiter.
			ret.push_back(str.substr(start, pos - start));
			start = pos + 1;
		}
		if (curDoubleDelim == '\0') {
			// Parse up to next real data.
			if (parseUpToNext)
				start = str.find_first_not_of(singleDelims, start);
		}

		++numSplits;
	} while (pos != std::string::npos);

	return ret;
}

bool Text::postfix(const std::string &str, std::string &txt, int &num) {
	txt.clear();
	num = -1;

	for (int i = (int)str.size() - 1; i >= 0; --i) {
		char ch = str[i];
		if (ch < '0' || ch > '9') {
			if (i == (int)str.size() - 1) {
				txt = str;

				return false;
			}

			txt = str.substr(0, i + 1);
			std::string snum = str.substr(i + 1);
			fromString(snum, num);

			return true;
		}
	}

	fromString(str, num);

	return true;
}

std::string Text::repeat(const std::string &str, int count) {
	if (count <= 0)
		return "";

	std::string ret;
	for (int i = 0; i < count; ++i)
		ret += str;

	return ret;
}

size_t Text::indexOf(const std::string &str, char what, size_t start) {
	return str.find_first_of(what, start);
}

size_t Text::indexOf(const std::string &str, const std::string &what, size_t start) {
	return str.find(what, start);
}

size_t Text::lastIndexOf(const std::string &str, char what, size_t start) {
	return str.find_last_of(what, start);
}

size_t Text::lastIndexOf(const std::string &str, const std::string &what, size_t start) {
	return str.find_last_of(what, start);
}

int Text::indexOf(const Array &arr, const std::string &what) {
	for (int i = 0; i < (int)arr.size(); ++i) {
		if (arr[i] == what)
			return i;
	}

	return -1;
}

bool Text::startsWith(const char* str, const char* what, bool caseInsensitive) {
	if (!str || !what)
		return false;
	if (!*str || !*what)
		return false;

	for (int i = 0; ; ++i) {
		const char ch0 = str[i];
		const char ch1 = what[i];
		if (ch0 == '\0' && ch1 == '\0')
			break;
		if (ch0 == '\0' && ch1 != '\0')
			return false;
		if (ch0 != '\0' && ch1 == '\0')
			break;
		if (caseInsensitive) {
			if (::tolower(ch0) != ::tolower(ch1))
				return false;
		} else {
			if (ch0 != ch1)
				return false;
		}
	}

	return true;
}

bool Text::startsWith(const std::string &str, const std::string &what, bool caseInsensitive) {
	size_t thisLen = str.length();
	size_t patternLen = what.length();
	if (thisLen < patternLen || patternLen == 0)
		return false;

	std::string startOfThis = str.substr(0, patternLen);
	if (caseInsensitive) {
		std::string cwhat = what;
		toLowerCase(cwhat);
		toLowerCase(startOfThis);

		return startOfThis == cwhat;
	} else {
		return startOfThis == what;
	}
}

bool Text::endsWith(const std::string &str, const std::string &what, bool caseInsensitive) {
	size_t thisLen = str.length();
	size_t patternLen = what.length();
	if (thisLen < patternLen || patternLen == 0)
		return false;

	std::string endOfThis = str.substr(thisLen - patternLen, patternLen);
	if (caseInsensitive) {
		std::string cwhat = what;
		toLowerCase(cwhat);
		toLowerCase(endOfThis);

		return endOfThis == cwhat;
	} else {
		return endOfThis == what;
	}
}

bool Text::matchWildcard(const std::string &str, const char* wildcard, bool caseInsensitive) {
	struct Char {
		unsigned code = 0;
		size_t length = 0;

		Char() {
		}
		Char(unsigned c, size_t l) : code(c), length(l) {
		}
	};
	auto take1 = [caseInsensitive] (const char* str) -> Char {
		if (*str == '\0')
			return Char(0, 0);
		if (!caseInsensitive)
			return Char((unsigned)str[0], 1);

		int n = Unicode::expectUtf8(str);
		if (n == 0)
			return Char(0, 0);
		else if (n == 1)
			return Char((unsigned)::toupper(str[0]), 1);

		return Char(Unicode::takeUtf8(str, n), n);
	};

	const char* string = str.c_str();

	Char s = take1(string), w = take1(wildcard);
	while (w.code && w.code != '*' && w.code != '?') {
		if (s.code == w.code) {
			string += s.length; s = take1(string);
			wildcard += w.length; w = take1(wildcard);
		} else {
			return false;
		}
	}

	if (!s.code) {
		while (w.code) {
			if (w.code != '*' && w.code != '?')
				return false;
			wildcard += w.length; w = take1(wildcard);
		}

		return true;
	}

	switch (w.code) {
	case '\0':
		return false;
	case '*':
		while (w.code == '*' || w.code == '?') {
			wildcard += w.length; w = take1(wildcard);
		}

		if (!(w.code))
			return true;

		while (s.code) {
			if (matchWildcard(string, wildcard, caseInsensitive))
				return true;
			string += s.length; s = take1(string);
		}

		return false;
	case '?':
		return matchWildcard(string + s.length, wildcard + w.length, caseInsensitive) || matchWildcard(string, wildcard + w.length, caseInsensitive);
	default:
		GBBASIC_ASSERT(false);

		return false;
	}
}

size_t Text::count(const std::string &str, const char what) {
	if (str.empty())
		return 0;

	size_t result = 0;
	const char* tmp = str.c_str();
	while (*tmp) {
		if (*tmp == what)
			++result;
		++tmp;
	}

	return result;
}

const char* Text::styleOf(const std::string &str) {
	const bool rnewline = str.find_first_of('\r') != std::string::npos;
	const bool nnewline = str.find_first_of('\n') != std::string::npos;
	if (rnewline && !nnewline)
		return "macos";
	else if (rnewline && nnewline)
		return "windows";
	else if (!rnewline && nnewline)
		return "unix";

	return "unknown";
}

void Text::stylish(std::string &str, const char* dst, const char* src) {
	if (!dst)
		return;
	if (!src)
		src = styleOf(str);
	if (!src)
		return;
	if (!strcmp(dst, src))
		return;

	char remove = '\0';
	char oldch = '\0';
	char newch = '\0';
	if (!strcmp(dst, "macos")) {
		if (!strcmp(src, "windows")) {
			remove = '\n';
		} else if (!strcmp(src, "unix")) {
			oldch = '\n';
			newch = '\r';
		}
	} else if (!strcmp(dst, "windows")) {
		if (!strcmp(src, "macos")) {
			remove = '\n';
		} else if (!strcmp(src, "unix")) {
			remove = '\r';
		}
	} else if (!strcmp(dst, "unix")) {
		if (!strcmp(src, "windows")) {
			remove = '\r';
		} else if (!strcmp(src, "macos")) {
			oldch = '\r';
			newch = '\n';
		}
	}

	if (remove) {
		str.erase(
			std::remove(str.begin(), str.end(), remove),
			str.end()
		);
	} else if (oldch && newch) {
		std::transform(
			str.begin(), str.end(), str.begin(),
			[&] (char ch) -> char {
				if (ch == oldch)
					return newch;
				else
					return ch;
			}
		);
	}
}

std::string Text::cformat(const char* fmt, ...) {
	const std::string osfmt = Unicode::toOs(fmt);
	fmt = osfmt.c_str();
	char buf[1024 * 64]; // Fixed size.
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(buf, GBBASIC_COUNTOF(buf), fmt, args);
	if (n == -1 || n >= GBBASIC_COUNTOF(buf))
		n = GBBASIC_COUNTOF(buf) - 1;
	buf[n] = '\0';
	va_end(args);

	return buf;
}

std::string Text::format(const std::string &fmt, const std::string &arg, int index) {
	std::string result = fmt;
	const std::string pattern = "{" + toString(index) + "}";
	result = replace(result, pattern, arg, true);

	return result;
}

std::string Text::format(const std::string &fmt, const Variant &arg, int index) {
	return format(fmt, arg.toString(), index);
}

std::string Text::format(const std::string &fmt, const Array &args, int index) {
	std::string result = fmt;
	Array::const_iterator it = args.begin();
	for (int i = 0; i < (int)args.size(); ++i, ++it)
		result = format(result, *it, index + i);

	return result;
}

std::string Text::format(const std::string &fmt, const std::initializer_list<std::string> &args, int index) {
	std::string result = fmt;
	std::initializer_list<std::string>::const_iterator it = args.begin();
	for (int i = 0; i < (int)args.size(); ++i, ++it)
		result = format(result, *it, index + i);

	return result;
}

long Text::strtol(char const* str, char** endptr, int base) {
	const long long ll = ::strtoll(str, endptr, base);
	if (ll & 0xffffffff00000000) {
		*endptr = (char*)str;

		return 0;
	}
	if (**endptr == '\0') {
		long ret = 0;
		memcpy(&ret, &ll, sizeof(long));

		return ret;
	}

	return 0;
}

long long Text::strtoll(char const* str, char** endptr, int base) {
	const long long ll = ::strtoll(str, endptr, base);
	if (**endptr == '\0')
		return ll;

	return 0;
}

unsigned long long Text::strtoull(char const* str, char** endptr, int base) {
	unsigned long long ll = ::strtoll(str, endptr, base);
	if (**endptr == '\0')
		return ll;

	return 0;
}

double Text::strtod(const char* str, char** endptr) {
	// strtod.c
	//
	// See: https://github.com/ringgaard/sanos/blob/master/src/lib/strtod.c.
	//
	// Convert string to double
	//
	// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
	//
	// Redistribution and use in source and binary forms, with or without
	// modification, are permitted provided that the following conditions
	// are met:
	//
	// 1. Redistributions of source code must retain the above copyright
	//    notice, this list of conditions and the following disclaimer.
	// 2. Redistributions in binary form must reproduce the above copyright
	//    notice, this list of conditions and the following disclaimer in the
	//    documentation and/or other materials provided with the distribution.
	// 3. Neither the name of the project nor the names of its contributors
	//    may be used to endorse or promote products derived from this software
	//    without specific prior written permission.
	//
	// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
	// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
	// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
	// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	// SUCH DAMAGE.

	double number;
	int exponent;
	int negative;
	char* p = (char*)str;
	double p10;
	int n;
	int numDigits;
	int numDecimals;

	// Skip leading whitespace.
	while (isspace(*p))
		p++;

	// Handle optional sign.
	negative = 0;
	switch (*p) {
	case '-': negative = 1; // Fall through to increment position.
	case '+': p++;
	}

	number = 0.;
	exponent = 0;
	numDigits = 0;
	numDecimals = 0;

	// Process string of digits.
	while (isdigit(*p)) {
		number = number * 10. + (*p - '0');
		p++;
		numDigits++;
	}

	// Process decimal part.
	if (*p == '.') {
		p++;

		while (isdigit(*p)) {
			number = number * 10. + (*p - '0');
			p++;
			numDigits++;
			numDecimals++;
		}

		exponent -= numDecimals;
	}

	if (numDigits == 0) {
		errno = ERANGE;

		return 0.0;
	}

	// Correct for sign.
	if (negative)
		number = -number;

	// Process an exponent string.
	if (*p == 'e' || *p == 'E') {
		// Handle optional sign.
		negative = 0;
		switch (*++p) {
		case '-': negative = 1; // Fall through to increment position.
		case '+': p++;
		}

		// Process string of digits.
		n = 0;
		while (isdigit(*p)) {
			n = n * 10 + (*p - '0');
			p++;
		}

		if (negative)
			exponent -= n;
		else
			exponent += n;
	}

	if (exponent < DBL_MIN_EXP || exponent > DBL_MAX_EXP) {
		errno = ERANGE;

		return HUGE_VAL;
	}

	// Scale the result.
	p10 = 10.;
	n = exponent;
	if (n < 0)
		n = -n;
	while (n) {
		if (n & 1) {
			if (exponent < 0)
				number /= p10;
			else
				number *= p10;
		}
		n >>= 1;
		p10 *= p10;
	}

	if (number == HUGE_VAL)
		errno = ERANGE;
	if (endptr)
		*endptr = p;

	return number;
}

Text* Text::create(void) {
	TextImpl* result = new TextImpl();

	return result;
}

void Text::destroy(Text* ptr) {
	TextImpl* impl = static_cast<TextImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
