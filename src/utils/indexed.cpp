/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "image.h"
#include "indexed.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

static const Colour INDEXED_DEFAULT_COLORS_ARRAY[] = INDEXED_DEFAULT_COLORS;

/* ===========================================================================} */

/*
** {===========================================================================
** Indexed
*/

class IndexedImpl : public Indexed {
private:
	Colour* _colors = nullptr;
	int _count = 0;
	bool _dirty = true;

	SDL_Palette* _palette = nullptr;

public:
	IndexedImpl(int count) {
		if (count > 0) {
			_count = count;
			_colors = new Colour[_count];
			memset(_colors, 255, _count * sizeof(Colour));
			memcpy(
				_colors,
				INDEXED_DEFAULT_COLORS_ARRAY,
				Math::min(sizeof(INDEXED_DEFAULT_COLORS_ARRAY), _count * sizeof(Colour))
			);
		} else {
			_count = 0;
		}
	}
	virtual ~IndexedImpl() override {
		if (_palette) {
			SDL_FreePalette(_palette);
			_palette = nullptr;
		}

		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Indexed** ptr, bool /* represented */) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		IndexedImpl* result = static_cast<IndexedImpl*>(Indexed::create(_count));
		if (_colors && _count > 0)
			memcpy(result->_colors, _colors, sizeof(Colour) * _count);

		*ptr = result;

		return true;
	}
	virtual bool clone(Indexed** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Indexed* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual void* pointer(void) override {
		return palette();
	}
	virtual Colour* pointer(int* n) override {
		if (n)
			*n = 0;

		if (_colors && _count) {
			if (n)
				*n = _count;

			_dirty = true;

			return _colors;
		}

		return nullptr;
	}
	virtual const Colour* pointer(int* n) const override {
		if (n)
			*n = 0;

		if (_colors && _count) {
			if (n)
				*n = _count;

			return _colors;
		}

		return nullptr;
	}

	virtual bool validate(void) override {
		return !!palette();
	}

	virtual int depth(void) const override {
		return (int)std::log2(_count);
	}
	virtual int count(void) const override {
		return _count;
	}

	virtual const Colour* get(int index, Colour &col) const override {
		col = Colour();

		if (index < 0 || index >= _count)
			return nullptr;

		col = _colors[index];

		return &col;
	}
	virtual bool set(int index, const Colour* col) override {
		if (index < 0 || index >= _count)
			return false;

		if (!col)
			return false;

		_colors[index] = *col;

		_dirty = true;

		return true;
	}

	virtual bool match(const class Image* img, Lookup &lookup) const override {
		// Prepare.
		struct Dst {
			typedef std::vector<Dst> Array;

			Colour color;
			int gray = 0;
			int index = 0;

			Dst() {
			}
			Dst(const Colour &col, int g, int idx) : color(col), gray(g), index(idx) {
			}
		};
		struct Src {
			typedef std::vector<Src> Array;

			Colour color;
			int gray = 0;

			Src() {
			}
			Src(const Colour &col, int g) : color(col), gray(g) {
			}

			bool operator == (const Src &other) const {
				return color == other.color && gray == other.gray;
			}
		};

		constexpr const int EPSILON = 10;

		lookup.clear();

		// Determine this palette's grayscale.
		Dst::Array pltGrayscale;
		for (int i = 0; i < count(); ++i) {
			Colour col;
			get(i, col);
			const int y = col.toGray();
			const Dst dst(col, y, i);
			pltGrayscale.push_back(dst);
		}
		std::sort(
			pltGrayscale.begin(), pltGrayscale.end(),
			[EPSILON] (const Dst &l, const Dst &r) -> bool {
				if (l.color.a <= EPSILON && r.color.a > EPSILON)
					return true;
				if (l.color.a > EPSILON && r.color.a <= EPSILON)
					return false;

				return l.gray > r.gray;
			}
		);

		// Determine the image's grayscale.
		Src::Array imgGrayscale;
		for (int j = 0; j < img->height(); ++j) {
			for (int i = 0; i < img->width(); ++i) {
				Colour col;
				img->get(i, j, col);
				const int y = col.toGray();
				const Src src(col, y);
				if (std::find(imgGrayscale.begin(), imgGrayscale.end(), src) == imgGrayscale.end())
					imgGrayscale.push_back(src);
			}
		}
		std::sort(
			imgGrayscale.begin(), imgGrayscale.end(),
			[EPSILON] (const Src &l, const Src &r) -> bool {
				if (l.color.a <= EPSILON && r.color.a > EPSILON)
					return false;
				if (l.color.a > EPSILON && r.color.a <= EPSILON)
					return true;

				return l.gray > r.gray;
			}
		);

		// Fill in the look up table.
		if (pltGrayscale.size() == imgGrayscale.size()) {
			// Fill in with the similar set of color values.
			for (int i = 0; i < (int)imgGrayscale.size(); ++i) {
				const Src &src = imgGrayscale[i];
				const Dst &dst = pltGrayscale[i];
				lookup[src.color] = dst.index;
			}
		} else {
			// Fill in with the identical and similar set of color values.
			for (int i = 0; i < (int)imgGrayscale.size(); ++i) {
				const Src &src = imgGrayscale[i];
				for (int j = 0; j < (int)pltGrayscale.size(); ++j) {
					const Dst &dst = pltGrayscale[j];
					const float diff =
						std::abs(src.color.a - dst.color.a) * 0.15f + // Alpha channel is less important (when it's non-zero).
						std::abs(src.color.r - dst.color.r) +
						std::abs(src.color.g - dst.color.g) +
						std::abs(src.color.b - dst.color.b);
					if (
						(src.color.a == 0 && dst.color.a == 0) || // Both are transparent.
						(src.color == dst.color) ||               // Identical color values.
						(diff <= 20)                              // Similar ones.
					) {
						lookup[src.color] = dst.index;
					}
				}
			}

			// Fill in with the rest ones.
			for (int i = 0; i < (int)imgGrayscale.size(); ++i) {
				const Src &src = imgGrayscale[i];
				if (lookup.find(src.color) != lookup.end()) // Already filled.
					continue;

				const float similarity = (float)i / ((int)imgGrayscale.size() - 1) * ((int)pltGrayscale.size() - 1);
				const int idx = Math::clamp((int)similarity, 0, (int)pltGrayscale.size() - 1);
				lookup[src.color] = idx;
			}
		}

		// Finish.
		return true;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		rapidjson::Value jstrcount;
		jstrcount.SetString("count", doc.GetAllocator());
		rapidjson::Value jnumcount;
		jnumcount.SetInt(_count);
		val.AddMember(jstrcount, jnumcount, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		for (int i = 0; i < _count; ++i) {
			const Colour &col = _colors[i];
			rapidjson::Value jcol;
			jcol.SetArray();
			jcol.PushBack(col.r, doc.GetAllocator());
			jcol.PushBack(col.g, doc.GetAllocator());
			jcol.PushBack(col.b, doc.GetAllocator());
			jcol.PushBack(col.a, doc.GetAllocator());
			jvaldata.PushBack(jcol, doc.GetAllocator());
		}
		val.AddMember(jstrdata, jvaldata, doc.GetAllocator());

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		clear();

		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jcount = val.FindMember("count");
		if (jcount == val.MemberEnd() || !jcount->value.IsNumber())
			return false;
		const int count = jcount->value.GetInt();
		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata == val.MemberEnd() || !jdata->value.IsArray())
			return false;

		const rapidjson::Value &jarr = jdata->value;
		if (count != (int)jarr.Size())
			return false;
		_count = jarr.Size();
		_colors = new Colour[_count];
		for (int i = 0; i < _count; ++i)
			_colors[i] = Colour();

		int i = 0;
		while (i < count && i < (int)jarr.Size()) {
			Colour &col = _colors[i];
			const rapidjson::Value &jcol = jarr[i];
			if (jcol.IsArray() && jcol.Size() == 4) {
				const rapidjson::Value &c0 = jcol[0];
				const rapidjson::Value &c1 = jcol[1];
				const rapidjson::Value &c2 = jcol[2];
				const rapidjson::Value &c3 = jcol[3];
				if (c0.IsInt() && c1.IsInt() && c2.IsInt() && c3.IsInt()) {
					col = {
						(Byte)c0.GetInt(),
						(Byte)c1.GetInt(),
						(Byte)c2.GetInt(),
						(Byte)c3.GetInt()
					};
				}
			}

			++i;
		}
		while (i < count) {
			Colour &col = _colors[i];
			col = Colour(0, 0, 0, 0);

			++i;
		}

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	SDL_Palette* palette(void) {
		if (_count <= 0) {
			if (_dirty)
				_dirty = false;

			return nullptr;
		}

		if (!_palette) {
			_palette = SDL_AllocPalette(_count);

			_dirty = true;
		}

		if (_dirty) {
			SDL_SetPaletteColors(_palette, (SDL_Color*)_colors, 0, _count);

			_dirty = false;
		}

		return _palette;
	}

	void clear(void) {
		if (_colors) {
			delete [] _colors;
			_colors = nullptr;
		}
		_count = 0;

		_dirty = true;
	}
};

Indexed* Indexed::create(int count) {
	IndexedImpl* result = new IndexedImpl(count);

	return result;
}

void Indexed::destroy(Indexed* ptr) {
	IndexedImpl* impl = static_cast<IndexedImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
