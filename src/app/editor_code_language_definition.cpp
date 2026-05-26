/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_code_language_definition.h"
#include "../compiler/compiler.h"
#include "../utils/encoding.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Utilities
*/

#if defined GBBASIC_OS_WIN
#	define memicmp _memicmp
#else /* GBBASIC_OS_WIN */
static int memicmp(const void* l, const void* r, size_t count) {
	if (count == 0)
		return 0;

	const unsigned char* lp = (const unsigned char*)l;
	const unsigned char* rp = (const unsigned char*)r;

	for (size_t i = 0; i < count; ++i) {
		unsigned char lc = lp[i];
		unsigned char rc = rp[i];

		if (lc >= 'A' && lc <= 'Z')
			lc = lc - 'A' + 'a';
		if (rc >= 'A' && rc <= 'Z')
			rc = rc - 'A' + 'a';

		if (lc != rc)
			return (int)lc - (int)rc;
	}

	return 0;
}
#endif /* GBBASIC_OS_WIN */

/* ===========================================================================} */

/*
** {===========================================================================
** Code language definition
*/

static bool tokenizeString(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd, char quote) {
	const char* p = inBegin;
	if (*p != quote)
		return false;
	++p;

	while (p < inEnd) {
		if (*p == quote) {
			outBegin = inBegin;
			outEnd = p + 1;

			return true;
		}

		if (p - 1 >= inBegin && *(p - 1) == '\\' && *p == '\\') { // Backslash.
			// Do nothing.
		} else if (*p == '\\' && p + 1 < inEnd && p[1] == quote) { // Quote.
			++p;
		}

		p += Unicode::expectUtf8(p);
	}

	return false;
}

static bool tokenizePreprocessor(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd) {
	// Prepare.
	const char        SHARP          = '#';
	const std::string PREPROCESSOR   = "adefgilmnorsw";
	const char        IF[]           = { '#', 'i', 'f' };
	const char        ELSE[]         = { '#', 'e', 'l', 's', 'e' };
	const char        ELSE_IF[]      = { '#', 'e', 'l', 's', 'e', ' ', 'i', 'f' };
	const char        ELSEIF[]       = { '#', 'e', 'l', 's', 'e', 'i', 'f' };
	const char        END[]          = { '#', 'e', 'n', 'd' };
	const char        END_IF[]       = { '#', 'e', 'n', 'd', ' ', 'i', 'f' };
	const char        ENDIF[]        = { '#', 'e', 'n', 'd', 'i', 'f' };
	const char        MESSAGE[]      = { '#', 'm', 'e', 's', 's', 'a', 'g', 'e' };
	const char        WARN[]         = { '#', 'w', 'a', 'r', 'n' };
	const char        ERROR[]        = { '#', 'e', 'r', 'r', 'o', 'r' };

	auto contains = [] (const std::string &pattern, const char ch) -> bool {
		return Text::indexOf(pattern, ch) != std::string::npos;
	};

	// Expect '#'.
	const char* p = inBegin;
	if (*p != SHARP)
		return false;
	++p;

	// Expect preprocessor.
	if (!contains(PREPROCESSOR, *p))
		return false;
	while (p < inEnd) {
		if (!contains(PREPROCESSOR, *p))
			break;

		outBegin = inBegin;
		outEnd = p + 1;

		++p;
	}

	if (memicmp(outBegin, ELSE, GBBASIC_COUNTOF(ELSE)) == 0 || memicmp(outBegin, END, GBBASIC_COUNTOF(END)) == 0) {
		const char* q = p;
		bool hasIf = false;
		if (*q == ' ') {
			++q;
			if (*q == 'i' || *q == 'I') {
				++q;
				if (*q == 'f' || *q == 'F') {
					++q;
					hasIf = true;
				}
			}
		}
		if (hasIf) {
			p = q;
			outBegin = inBegin;
			outEnd = p;
		}
	}

	// Check the token.
	if (!outBegin || !outEnd || outBegin >= outEnd)
		return false;

	if (memicmp(outBegin, IF, GBBASIC_COUNTOF(IF)) == 0)
		return true;
	if (memicmp(outBegin, ELSE, GBBASIC_COUNTOF(ELSE)) == 0)
		return true;
	if (memicmp(outBegin, ELSE_IF, GBBASIC_COUNTOF(ELSE_IF)) == 0)
		return true;
	if (memicmp(outBegin, ELSEIF, GBBASIC_COUNTOF(ELSEIF)) == 0)
		return true;
	if (memicmp(outBegin, END, GBBASIC_COUNTOF(END)) == 0)
		return true;
	if (memicmp(outBegin, END_IF, GBBASIC_COUNTOF(END_IF)) == 0)
		return true;
	if (memicmp(outBegin, ENDIF, GBBASIC_COUNTOF(ENDIF)) == 0)
		return true;
	if (memicmp(outBegin, MESSAGE, GBBASIC_COUNTOF(MESSAGE)) == 0)
		return true;
	if (memicmp(outBegin, WARN, GBBASIC_COUNTOF(WARN)) == 0)
		return true;
	if (memicmp(outBegin, ERROR, GBBASIC_COUNTOF(ERROR)) == 0)
		return true;

	return false;
}

static bool tokenizeAssetDestination(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd) {
	// Prepare.
	const char        PAGE          = '#';
	const char        PAGE_SEP      = ':';
	const std::string PAGE_NUMBER   = "0123456789xXabcdefABCDEF";
	const std::string CLOSE_SYM     = "#" "()[]<>=+-*/,;:" " \t" "\'\"";

	auto contains = [] (const std::string &pattern, const char ch) -> bool {
		return Text::indexOf(pattern, ch) != std::string::npos;
	};

	// Expect '#'.
	const char* p = inBegin;
	if (*p != PAGE)
		return false;
	++p;

	// Expect page number.
	if (!contains(PAGE_NUMBER, *p))
		return false;
	while (p < inEnd) {
		if (!contains(PAGE_NUMBER, *p))
			break;

		outBegin = inBegin;
		outEnd = p + 1;

		++p;
	}

	// Expect ':'.
	if (*p != PAGE_SEP)
		return true;
	++p;

	// Expect line number or label.
	int m = 0;
	while (*p != '\0') {
		const int n = Unicode::expectUtf8(p);
		if (n == 0)
			break;

		if (n == 1 && contains(CLOSE_SYM, *p))
			break;

		outBegin = inBegin;
		outEnd = p + n;

		p += n;

		m += n;
	}

	return m > 0;
}

static bool tokenize(const char* inBegin, const char* inEnd, const char* &outBegin, const char* &outEnd, ImGui::CodeEditor::PaletteIndex &paletteIndex) {
	paletteIndex = ImGui::CodeEditor::PaletteIndex::Max;

	while (inBegin < inEnd && isascii(*inBegin) && isblank(*inBegin))
		++inBegin;

	if (inBegin == inEnd) {
		outBegin = inEnd;
		outEnd = inEnd;
		paletteIndex = ImGui::CodeEditor::PaletteIndex::Default;
	} else if (tokenizeString(inBegin, inEnd, outBegin, outEnd, '"')) {
		paletteIndex = ImGui::CodeEditor::PaletteIndex::String;
	} else if (tokenizePreprocessor(inBegin, inEnd, outBegin, outEnd)) {
		paletteIndex = ImGui::CodeEditor::PaletteIndex::Preprocessor;
	} else if (tokenizeAssetDestination(inBegin, inEnd, outBegin, outEnd)) {
		paletteIndex = ImGui::CodeEditor::PaletteIndex::Symbol;
	}

	return paletteIndex != ImGui::CodeEditor::PaletteIndex::Max;
}

ImGui::CodeEditor::LanguageDefinition EditorCodeLanguageDefinition::languageDefinition(const char* kernelConfigPath) {
	// Prepare.
	ImGui::CodeEditor::LanguageDefinition langDef;

	Text::Set added;

	// Keywords.
	constexpr const char* const KEYWORDS[] = {
		// Type definition.
		"int", "uint",

		// Blank and remark.
		"do",

		// Declaration.
		"var", "const", "let", "local", "dim",

		// Conditional.
		"if", "then", "else", "elseif", "endif", "iif",
		"select", "case", "endselect",
		"on", "off",
		"error",

		// Loop.
		"for", "to", "step", "next",
		"while", "wend",
		"repeat", "until",
		"exit", "continue",

		// Jump.
		"goto", "gosub",
		"return",
		"end",
		"call",
		"asm",
		"beginasm", "endasm",

		// Scope and macros.
		"begin",
		"begindo", "enddo",
		"begindef", "enddef",
		"def", "fn",

		// Data stream.
		"data", "read", "restore",

		// Object.
		"nothing",

		// Boolean.
		"false", "true"
	};
	for (const char* const k : KEYWORDS) {
		const std::string key(k);
		const std::pair<Text::Set::const_iterator, bool> it = added.insert(key);
		if (!it.second) {
			fprintf(stderr, "Duplicated keyword: \"%s\".\n", k);

			GBBASIC_ASSERT(false && "Duplicated keyword.");
		}

		langDef.Keys.insert(key);
	}

	// Important identifiers.
	constexpr const char* const IMPORTANT_IDENTIFIERS[] = {
		// Operators.
		"mod", "and", "or", "not", "band", "bor", "bxor", "bnot", "lshift", "rshift",
		"sgn", "abs", "sqr", "sqrt", "sin", "cos", "atan2", "pow", "min", "max",
		"asc",
		"deg",
		"len",
		"randomize", "rnd",
		"bankof", "addressof",

		// Thread.
		"arg", "start", "join", "kill", "wait", "lock", "unlock",

		// Peek and poke.
		"peek", "poke",

		// Stack.
		"reserve", "push", "pop", "top", "stack",

		// Memory.
		"pack", "unpack",
		"swap",
		"inc", "dec",

		// Object.
		"new", "del",
		"fill", "load",
		"get", "set",
		"property", "width", "height",
		"move",
		"find",
		"control",
		"auto",
		"is",
		"with"
	};
	for (const char* const k : IMPORTANT_IDENTIFIERS) {
		const std::string key(k);
		const std::pair<Text::Set::const_iterator, bool> it = added.insert(key);
		if (!it.second) {
			fprintf(stderr, "Duplicated identifier: \"%s\".\n", k);

			GBBASIC_ASSERT(false && "Duplicated identifier.");
		}

		langDef.Keys.insert(key);
	}

	// Identifiers.
	constexpr const char* const IDENTIFIERS[] = {
		// Output.
		"locate", "print", "cls",

		// System.
		"sleep",
		"raise",
		"reset",

		// Graphics.
		"color", "palette", "rgb", "hsv",
		"plot", "point", "line", "rect", "rectfill", "text", "image",
		"tile", "map", "window", "sprite",

		// Audio.
		"play", "stop",
		"sound", "beep",

		// Input.
		"btn", "btnd", "btnu",
		"touch", "touchd", "touchu",

		// Persistence.
		"fopen", "fclose", "fread", "fwrite",

		// Serial port.
		"serial", "sread", "swrite",

		// Debug.
		"dbginfo",
		"filler",

		// Memory.
		"memset", "memcpy", "memadd",

		// Scene.
		"camera", "viewport",
		"scene",

		// Actor.
		"actor",

		// Emote.
		"emote",

		// Projectile.
		"projectile",

		// Trigger.
		"trigger",

		// GUI.
		"label",
		"progressbar",
		"menu",
		"dialog",

		// Scroll.
		"scroll",

		// Game.
		"poll", "update",

		// Physics.
		"hits",

		// Effects.
		"fx",

		// Device.
		"screen",
		"option", "query",
		"stream",
		"shell",

		// System.
		"time"
	};
	for (const char* const k : IDENTIFIERS) {
		const std::string key(k);
		const std::pair<Text::Set::const_iterator, bool> it = added.insert(key);
		if (!it.second) {
			fprintf(stderr, "Duplicated identifier: \"%s\".\n", k);

			GBBASIC_ASSERT(false && "Duplicated identifier.");
		}

		ImGui::CodeEditor::Identifier id;
		id.Declaration = "Builtin function";
		langDef.Ids.insert(std::make_pair(key, id));
	}
	for (int i = 0; i < COMPILER_STACK_ARGUMENT_MAX_COUNT; ++i) {
		const std::string key = "stack" + Text::toString(i);
		const std::pair<Text::Set::const_iterator, bool> it = added.insert(key);
		if (!it.second) {
			fprintf(stderr, "Duplicated identifier: \"%s\".\n", key.c_str());

			GBBASIC_ASSERT(false && "Duplicated identifier.");
		}

		ImGui::CodeEditor::Identifier id;
		id.Declaration = "Builtin function";
		langDef.Ids.insert(std::make_pair(key, id));
	}

	// Pre-processing identifiers.
	GBBASIC::identifiers(
		kernelConfigPath,
		[&] (const std::string &key, const std::string &type) -> void {
			if (type == "constant") {
				std::string key_(key);
				Text::toLowerCase(key_);
				const std::pair<Text::Set::const_iterator, bool> it = added.insert(key_);
				if (!it.second) {
					fprintf(stderr, "Duplicated builtin: \"%s\".\n", key.c_str());

					GBBASIC_ASSERT(false && "Duplicated builtin.");
				}

				ImGui::CodeEditor::Identifier id;
				id.Declaration = "Constant";
				langDef.PreprocIds.insert(std::make_pair(key_, id));
			} else if (type == "native") {
				std::string key_(key);
				Text::toLowerCase(key_);

				ImGui::CodeEditor::Identifier id;
				id.Declaration = "Native function";
				langDef.Ids.insert(std::make_pair(key_, id));
			} else {
				// Do nothing.
			}
		}
	);

	// Assembly keywords.
	constexpr const char* const ASM_KEYWORDS[] = {
		// Load/transfer.
		"ld", "ldh", "ldi", "ldd", "ldio",

		// Arithmetic.
		"add", "adc", "sub", "sbc",

		// Logic.
		"and", "or", "xor", "cp",

		// Increment/decrement.
		"inc", "dec",

		// Rotate/shift.
		"rlca", "rrca", "rla", "rra",
		"rlc", "rrc", "rl", "rr",
		"sla", "sra", "srl",
		"swap",

		// Bit operations.
		"bit", "set", "res",

		// Jump/call.
		"jp", "jr", "call", "ret", "reti", "rst",

		// Stack.
		"push", "pop",

		// Control.
		"nop", "halt", "stop", "di", "ei",

		// Flags.
		"daa", "cpl", "scf", "ccf",

		// Data.
		"db", "dw"
	};
	for (const char* const k : ASM_KEYWORDS) {
		langDef.AsmKeys.insert(std::string(k));
	}

	// Assembly registers.
	constexpr const char* const ASM_REGISTERS[] = {
		"a", "b", "c", "d", "e", "h", "l",
		"hl", "bc", "de", "sp", "af",
		"nz", "z", "nc", "c"
	};
	for (const char* const k : ASM_REGISTERS) {
		ImGui::CodeEditor::Identifier id;
		id.Declaration = "Register";
		langDef.AsmIds.insert(std::make_pair(std::string(k), id));
	}

	// Other syntax rules.
	langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, ImGui::CodeEditor::PaletteIndex>(
		"'.*|rem$|rem[ \t](.*)?",
		ImGui::CodeEditor::PaletteIndex::Comment
	));
	langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, ImGui::CodeEditor::PaletteIndex>(
		"[+-]?0[x][0-9a-f]+",
		ImGui::CodeEditor::PaletteIndex::Number
	));
	langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, ImGui::CodeEditor::PaletteIndex>(
		"[+-]?0[b][0-1]+",
		ImGui::CodeEditor::PaletteIndex::Number
	));
	langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, ImGui::CodeEditor::PaletteIndex>(
		"[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([e][+-]?[0-9]+)?",
		ImGui::CodeEditor::PaletteIndex::Number
	));
	langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, ImGui::CodeEditor::PaletteIndex>(
		"[_]*[a-z_][a-z0-9_]*[$]?",
		ImGui::CodeEditor::PaletteIndex::Identifier
	));
	langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, ImGui::CodeEditor::PaletteIndex>(
		"[\\*\\/\\+\\-\\(\\)\\[\\]\\=\\<\\>\\,\\;\\:]",
		ImGui::CodeEditor::PaletteIndex::Punctuation
	));

	langDef.Tokenize = std::bind(
		&tokenize,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5
	);

	langDef.CommentStart.clear();
	langDef.CommentEnd.clear();
	langDef.SimpleCommentHead = "'";

	langDef.RangedCharPatterns = {
		std::make_pair('"', '"'),
		std::make_pair('(', ')'),
		std::make_pair('[', ']')
	};

	langDef.CaseSensitive = false;

	langDef.Name = "GB BASIC";

	// Finish.
	return langDef;
}

/* ===========================================================================} */
