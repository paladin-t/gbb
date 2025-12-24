/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "localization.h"
#include "../../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Localization
*/

namespace Localization {

int parse(Dictionary &dict, const rapidjson::Value &val) {
	int result = 0;
	std::string tmp;
	if (!Jpath::get(val, tmp, "english"))
		return result;
	dict[Platform::Languages::ENGLISH] = tmp;
	++result;
	if (Jpath::get(val, tmp, "chinese")) {
		dict[Platform::Languages::CHINESE] = tmp;
		++result;
	}
	if (Jpath::get(val, tmp, "french")) {
		dict[Platform::Languages::FRENCH] = tmp;
		++result;
	}
	if (Jpath::get(val, tmp, "german")) {
		dict[Platform::Languages::GERMAN] = tmp;
		++result;
	}
	if (Jpath::get(val, tmp, "italian")) {
		dict[Platform::Languages::ITALIAN] = tmp;
		++result;
	}
	if (Jpath::get(val, tmp, "japanese")) {
		dict[Platform::Languages::JAPANESE] = tmp;
		++result;
	}
	if (Jpath::get(val, tmp, "portuguese")) {
		dict[Platform::Languages::PORTUGUESE] = tmp;
		++result;
	}
	if (Jpath::get(val, tmp, "spanish")) {
		dict[Platform::Languages::SPANISH] = tmp;
		++result;
	}

	return result;
}

const char* get(const Dictionary &dict) {
	std::string entry_;
	Dictionary::const_iterator it = dict.find(Platform::locale());
	if (it == dict.end())
		it = dict.find(Platform::Languages::ENGLISH);
	if (it != dict.end())
		return it->second.c_str();

	return nullptr;
}

void transform(Dictionary &dict, Transformer trans) {
	if (dict.empty())
		return;

	if (trans == nullptr)
		return;

	for (Dictionary::iterator it = dict.begin(); it != dict.end(); ++it) {
		const Platform::Languages lang = it->first;
		std::string &txt = it->second;
		trans(lang, txt);
	}
}

}

/* ===========================================================================} */
