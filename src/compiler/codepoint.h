/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __CODEPOINT_H__
#define __CODEPOINT_H__

#include "../gbbasic.h"
#include <string>
#include <vector>

/*
** {===========================================================================
** Codepoint
*/

namespace GBBASIC {

class Codepoint {
public:
	struct Data {
		typedef std::string Array;

		Array array;

		Data();
		~Data();

		bool operator == (const Data &other) const;
		bool operator < (const Data &other) const;
		bool operator > (const Data &other) const;
		bool operator >= (const Data &other) const;

		Data &add(char val);
		Data &add(int val);
		template<typename Car, typename ...Cdr> Data &add(Car car, Cdr ...cdr) {
			add(car);
			add(cdr ...);

			return *this;
		}
	};
	struct Collection {
		typedef std::vector<Data> Array;

		Array array;

		Collection();
		~Collection();

		bool ordered(void) const;
		bool has(const Data &val) const;
		bool has(const std::string &val) const;
		Collection &add(const Data &val);
		template<typename Car, typename ...Cdr> Collection &add(const Car &car, const Cdr &...cdr) {
			add(car);
			add(cdr ...);

			return *this;
		}
	};

public:
	static Collection UNICODE_SYMBOLS(void);
};

}

/* ===========================================================================} */

#endif /* __CODEPOINT_H__ */
