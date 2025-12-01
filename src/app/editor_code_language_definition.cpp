/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_code_language_definition.h"
#include "../compiler/compiler.h"
#include "../utils/encoding.h"
#include "../utils/text.h"

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
		"time",

		// Invokables.
		"peek_banked",
		"clear_text",
		"wait_for", "wait_until_confirm",
		"set_sgb_border",
		/* "error", */
		"camera_shake"
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
