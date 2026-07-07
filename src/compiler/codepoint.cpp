/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "codepoint.h"
#include <algorithm>
#include <limits>

/*
** {===========================================================================
** Codepoint
*/

namespace GBBASIC {

Codepoint::Data::Data() {
}

Codepoint::Data::~Data() {
}

bool Codepoint::Data::operator == (const Data &other) const {
	return array == other.array;
}

bool Codepoint::Data::operator < (const Data &other) const {
	return array < other.array;
}

bool Codepoint::Data::operator > (const Data &other) const {
	return array > other.array;
}

bool Codepoint::Data::operator >= (const Data &other) const {
	return array >= other.array;
}

Codepoint::Data &Codepoint::Data::add(char val) {
	array.push_back(val);

	return *this;
}

Codepoint::Data &Codepoint::Data::add(int val) {
	if (val < std::numeric_limits<unsigned char>::min() || val > std::numeric_limits<unsigned char>::max()) {
		GBBASIC_ASSERT(false && "Wrong data.");

		val = 0;
	}

	union { char ch; int int_; } u;
	u.int_ = val;
	add(u.ch);

	return *this;
}

Codepoint::Collection::Collection() {
}

Codepoint::Collection::~Collection() {
}

bool Codepoint::Collection::ordered(void) const {
	const Data* prev = nullptr;
	for (const Data &data : array) {
		if (prev == nullptr) {
			prev = &data;
		} else {
			if (*prev >= data)
				return false;
		}
	}

	return true;
}

bool Codepoint::Collection::has(const Data &val) const {
	if (array.empty())
		return false;

	void* ret = std::bsearch(
		&val,
		array.data(), array.size(), sizeof(Data),
		[] (const void* lptr, const void* rptr) -> int {
			const Data &l = *(const Data*)lptr;
			const Data &r = *(const Data*)rptr;

			if (l < r)
				return -1;
			if (l > r)
				return 1;

			return 0;
		}
	);
	if (ret == nullptr)
		return false;

	return true;
}

bool Codepoint::Collection::has(const std::string &val) const {
	Data data;
	for (char ch : val)
		data.add(ch);

	return has(data);
}

Codepoint::Collection &Codepoint::Collection::add(const Data &val) {
	array.push_back(val);

	return *this;
}

Codepoint::Collection Codepoint::UNICODE_SYMBOLS(void) {
	const Collection result = Collection()
		.add(Data().add(0xe2, 0x80, 0x98))  // ‘
		.add(Data().add(0xe2, 0x80, 0x99))  // ’
		.add(Data().add(0xe2, 0x80, 0x9a))  // ‚
		.add(Data().add(0xe2, 0x80, 0x9b))  // ‛
		.add(Data().add(0xe2, 0x80, 0x9c))  // “
		.add(Data().add(0xe2, 0x80, 0x9d))  // ”
		.add(Data().add(0xe2, 0x80, 0x9e))  // „
		.add(Data().add(0xe2, 0x80, 0x9f))  // ‟
		.add(Data().add(0xe2, 0x80, 0xa2))  // •
		.add(Data().add(0xe2, 0x80, 0xa4))  // ․
		.add(Data().add(0xe2, 0x80, 0xa5))  // ‥
		.add(Data().add(0xe2, 0x80, 0xa6))  // …
		.add(Data().add(0xe2, 0x80, 0xa7))  // ‧
		.add(Data().add(0xe2, 0x80, 0xb2))  // ′
		.add(Data().add(0xe2, 0x80, 0xb3))  // ″
		.add(Data().add(0xe2, 0x80, 0xb5))  // ‵
		.add(Data().add(0xe2, 0x80, 0xb9))  // ‹
		.add(Data().add(0xe2, 0x80, 0xba))  // ›

		.add(Data().add(0xe3, 0x80, 0x81))  // 、
		.add(Data().add(0xe3, 0x80, 0x82))  // 。
		.add(Data().add(0xe3, 0x80, 0x88))  // 〈
		.add(Data().add(0xe3, 0x80, 0x89))  // 〉
		.add(Data().add(0xe3, 0x80, 0x8a))  // 《
		.add(Data().add(0xe3, 0x80, 0x8b))  // 》
		.add(Data().add(0xe3, 0x80, 0x8c))  // 「
		.add(Data().add(0xe3, 0x80, 0x8d))  // 」
		.add(Data().add(0xe3, 0x80, 0x8e))  // 『
		.add(Data().add(0xe3, 0x80, 0x8f))  // 』
		.add(Data().add(0xe3, 0x80, 0x90))  // 【
		.add(Data().add(0xe3, 0x80, 0x91))  // 】
		.add(Data().add(0xe3, 0x80, 0x94))  // 〔
		.add(Data().add(0xe3, 0x80, 0x95))  // 〕
		.add(Data().add(0xe3, 0x80, 0x96))  // 〖
		.add(Data().add(0xe3, 0x80, 0x97))  // 〗
		.add(Data().add(0xe3, 0x80, 0x98))  // 〘
		.add(Data().add(0xe3, 0x80, 0x99))  // 〙
		.add(Data().add(0xe3, 0x80, 0x9a))  // 〚
		.add(Data().add(0xe3, 0x80, 0x9b))  // 〛
		.add(Data().add(0xe3, 0x80, 0x9d))  // 〝
		.add(Data().add(0xe3, 0x80, 0x9e))  // 〞
		.add(Data().add(0xe3, 0x80, 0x9f))  // 〟

		.add(Data().add(0xef, 0xbc, 0x81))  // ！
		.add(Data().add(0xef, 0xbc, 0x82))  // ＂
		.add(Data().add(0xef, 0xbc, 0x83))  // ＃
		.add(Data().add(0xef, 0xbc, 0x84))  // ＄
		.add(Data().add(0xef, 0xbc, 0x85))  // ％
		.add(Data().add(0xef, 0xbc, 0x86))  // ＆
		.add(Data().add(0xef, 0xbc, 0x87))  // ＇
		.add(Data().add(0xef, 0xbc, 0x88))  // （
		.add(Data().add(0xef, 0xbc, 0x89))  // ）
		.add(Data().add(0xef, 0xbc, 0x8a))  // ＊
		.add(Data().add(0xef, 0xbc, 0x8b))  // ＋
		.add(Data().add(0xef, 0xbc, 0x8c))  // ，
		.add(Data().add(0xef, 0xbc, 0x8d))  // －
		.add(Data().add(0xef, 0xbc, 0x8e))  // ．
		.add(Data().add(0xef, 0xbc, 0x8f))  // ／
		.add(Data().add(0xef, 0xbc, 0x9a))  // ：
		.add(Data().add(0xef, 0xbc, 0x9b))  // ；
		.add(Data().add(0xef, 0xbc, 0x9c))  // ＜
		.add(Data().add(0xef, 0xbc, 0x9d))  // ＝
		.add(Data().add(0xef, 0xbc, 0x9e))  // ＞
		.add(Data().add(0xef, 0xbc, 0x9f))  // ？
		.add(Data().add(0xef, 0xbc, 0xa0))  // ＠
		.add(Data().add(0xef, 0xbc, 0xbb))  // ［
		.add(Data().add(0xef, 0xbc, 0xbc))  // ＼
		.add(Data().add(0xef, 0xbc, 0xbd))  // ］
		.add(Data().add(0xef, 0xbc, 0xbe))  // ＾

		.add(Data().add(0xef, 0xbd, 0x80))  // ｀
		.add(Data().add(0xef, 0xbd, 0x9b))  // ｛
		.add(Data().add(0xef, 0xbd, 0x9d))  // ｝
		.add(Data().add(0xef, 0xbd, 0x9f))  // ｟
		.add(Data().add(0xef, 0xbd, 0xa0))  // ｠
		.add(Data().add(0xef, 0xbd, 0xa2))  // ｢
		.add(Data().add(0xef, 0xbd, 0xa3)); // ｣

	GBBASIC_ASSERT(result.ordered() && "Wrong data.");

	return result;
}

}

/* ===========================================================================} */
