/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __LOCALIZATION_H__
#define __LOCALIZATION_H__

#include "../gbbasic.h"
#include "json.h"
#include "platform.h"
#include <functional>
#include <map>

/*
** {===========================================================================
** Localization
*/

namespace Localization {

typedef std::map<Platform::Languages, std::string> Dictionary;

typedef std::function<void(Platform::Languages, std::string &)> Transformer;

int parse(Dictionary &dict, const rapidjson::Value &val);

const char* get(const Dictionary &dict);

void transform(Dictionary &dict, Transformer trans /* nullable */);

}

/* ===========================================================================} */

#endif /* __LOCALIZATION_H__ */
