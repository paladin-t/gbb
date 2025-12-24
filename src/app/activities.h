/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ACTIVITIES_H__
#define __ACTIVITIES_H__

#include "../gbbasic.h"
#include "../../lib/bigint/include/BigInt.hpp"

/*
** {===========================================================================
** Activities
*/

struct Activities {
	Activities();
	~Activities();

	BigInt opened;
		BigInt openedBasic;
		BigInt openedRom;
	BigInt created;
		BigInt createdBasic;
	BigInt played;
		BigInt playedBasic;
		BigInt playedRom;
	BigInt playedTime;
		BigInt playedTimeBasic;
		BigInt playedTimeRom;
	BigInt wroteCodeLines;

	long long playingStartTimestamp = 0;

	bool fromFile(const char* path);
	bool toFile(const char* path) const;

	static std::string formatTime(const BigInt &bigInt);
};

/* ===========================================================================} */

#endif /* __ACTIVITIES_H__ */
