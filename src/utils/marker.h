/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __MARKER_H__
#define __MARKER_H__

#include "../gbbasic.h"
#include <utility>

/*
** {===========================================================================
** Marker
*/

template<typename T> struct Marker {
	typedef T ValueType;
	typedef std::pair<ValueType, ValueType> Values;

	ValueType defaultValue = ValueType();
	ValueType previous;
	ValueType current;

	Marker() {
	}
	Marker(const ValueType &def) : defaultValue(def) {
		previous = def;
		current = def;
	}

	Marker &operator = (const ValueType &val) {
		previous = current;
		current = val;

		return *this;
	}

	Values values(void) const {
		return std::make_pair(current, previous);
	}

	const ValueType &get(void) const {
		return current;
	}
	const ValueType &get(ValueType &prev) const {
		prev = previous;

		return current;
	}
	const ValueType &set(const ValueType &val) const {
		previous = current;
		current = val;

		return previous;
	}
};

/* ===========================================================================} */

#endif /* __MARKER_H__ */
