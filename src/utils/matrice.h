/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __MATRICE_H__
#define __MATRICE_H__

#include "../gbbasic.h"
#include <vector>

/*
** {===========================================================================
** Matrice
*/

template<typename T> struct Matrice {
	typedef T ValueType;
	typedef std::vector<ValueType> Array;

	struct ConstColumn {
		const Array* ptr = nullptr;
		int x = 0;
		int w = 0;
		int h = 0;

		ConstColumn() {
		}
		ConstColumn(const Array &raw, int x_, int w_, int h_) :
			ptr(&raw), x(x_), w(w_), h(h_)
		{
		}

		const ValueType &operator [] (int y) const {
			return get(y);
		}

		int height(void) const {
			return h;
		}
		int count(void) const {
			return h;
		}
		int index(int y) const {
			int idx = x + y * w;
			if (idx < 0 || idx >= (int)ptr->size())
				return -1;

			return idx;
		}
		const ValueType &get(int y) const {
			const int idx = index(y);

			return (*ptr)[idx];
		}
	};
	struct NonConstColumn {
		Array* ptr = nullptr;
		int x = 0;
		int w = 0;
		int h = 0;

		NonConstColumn() {
		}
		NonConstColumn(Array &raw, int x_, int w_, int h_) :
			ptr(&raw), x(x_), w(w_), h(h_)
		{
		}

		const ValueType &operator [] (int y) const {
			return get(y);
		}
		ValueType &operator [] (int y) {
			return get(y);
		}

		int height(void) const {
			return h;
		}
		int count(void) const {
			return h;
		}
		int index(int y) const {
			int idx = x + y * w;
			if (idx < 0 || idx >= (int)ptr->size())
				return -1;

			return idx;
		}
		const ValueType &get(int y) const {
			const int idx = index(x, y);

			return (*ptr)[idx];
		}
		ValueType &get(int y) {
			const int idx = index(y);

			return ptr->at(idx);
		}
		bool set(int y, const ValueType &val) const {
			const int idx = index(y);
			if (idx == -1)
				return false;

			(*ptr)[idx] = val;

			return true;
		}
	};

	Array raw;
	int w = 0;
	int h = 0;

	Matrice() {
	}
	Matrice(int w_, int h_) {
		if (w_ * h_ < 0)
			return;

		w = w_;
		h = h_;
		raw.resize(w * h);
	}

	ConstColumn operator [] (int x) const {
		return ConstColumn(raw, x, w, h);
	}
	NonConstColumn operator [] (int x) {
		return NonConstColumn(raw, x, w, h);
	}

	int width(void) const {
		return (int)w;
	}
	int height(void) const {
		return (int)h;
	}
	bool resize(int w_, int h_) {
		if (w_ * h_ < 0)
			return false;

		w = w_;
		h = h_;
		raw.resize(w * h);

		return true;
	}
	int index(int x, int y) const {
		int idx = x + y * width();
		if (idx < 0 || idx >= (int)raw.size())
			return -1;

		return idx;
	}
	const ValueType &get(int x, int y) const {
		const int idx = index(x, y);

		return raw[idx];
	}
	ValueType &get(int x, int y) {
		const int idx = index(x, y);

		return raw[idx];
	}
	bool set(int x, int y, const ValueType &val) const {
		const int idx = index(x, y);
		if (idx == -1)
			return false;

		raw[idx] = val;

		return true;
	}
};

/* ===========================================================================} */

#endif /* __MATRICE_H__ */
