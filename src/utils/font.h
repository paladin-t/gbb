/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __FONT_H__
#define __FONT_H__

#include "../gbbasic.h"
#include "colour.h"
#include "object.h"

/*
** {===========================================================================
** Font
*/

/**
 * @brief Font resource object.
 */
class Font : public virtual Object {
public:
	typedef std::shared_ptr<Font> Ptr;

	typedef unsigned Codepoint;
	class Codepoints : public Cloneable<Codepoints>, public virtual Object {
	public:
		typedef std::shared_ptr<Codepoints> Ptr;

		typedef std::vector<Codepoint> Array;

		class iterator {
		public:
			typedef Codepoint value_type;
			typedef Codepoint & reference;
			typedef Codepoint* pointer;
			typedef int difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			int _index = -1;
			value_type* _raw = nullptr;

		public:
			iterator();
			iterator(int idx, value_type* raw);

			bool operator == (const iterator &other) const;
			bool operator != (const iterator &other) const;
			bool operator <= (const iterator &other) const;
			bool operator >= (const iterator &other) const;
			bool operator < (const iterator &other) const;
			bool operator > (const iterator &other) const;

			iterator &operator ++ (void);
			iterator &operator -- (void);
			iterator operator ++ (int);
			iterator operator -- (int);
			iterator operator + (difference_type n) const;
			iterator operator - (difference_type n) const;
			difference_type operator - (const iterator &other) const;
			iterator &operator += (difference_type n);
			iterator &operator -= (difference_type n);

			reference operator * (void) const;
			pointer operator -> (void) const;
			reference operator [] (difference_type offset) const;
		};

	private:
		Array _values;

	public:
		GBBASIC_CLASS_TYPE('F', 'N', 'C', 'P')

		Codepoints();
		virtual ~Codepoints() override;

		virtual unsigned type(void) const override;

		virtual bool clone(Codepoints** ptr) const override;
		using Cloneable<Codepoints>::clone;
		using Object::clone;

		bool operator == (const Codepoints &other) const;
		bool operator != (const Codepoints &other) const;

		Codepoint operator [] (int idx) const;

		iterator begin(void);
		iterator end(void);

		int count(void) const;
		void add(Codepoint cp);
		Codepoint get(int idx) const;
		bool set(int idx, Codepoint val);
		bool remove(int idx);
		void clear(void);
		void sort(void);
	};

	enum class Effects : int {
		NONE,
		SHADOW,
		OUTLINE
	};

	struct Options {
		Effects enabledEffect = Effects::NONE;
		int shadowOffset = 1;
		float shadowStrength = 0.3f;
		unsigned shadowDirection = 8;
		int outlineOffset = 1;
		float outlineStrength = 0.3f;
		int effectThreshold = 255;
	};

public:
	GBBASIC_CLASS_TYPE('F', 'N', 'T', 'A')

	virtual void* pointer(void) = 0;
	virtual void* pointer(size_t* len) = 0;

	virtual bool imageBased(void) const = 0;
	virtual bool ttfBased(void) const = 0;

	/**
	 * @brief Gets whether to trim font height, for TTF-based only.
	 */
	virtual bool trim(void) const = 0;
	/**
	 * @brief Sets whether to trim font height, for TTF-based only.
	 */
	virtual void trim(bool val) = 0;

	virtual int glyphCount(void) const = 0;

	virtual bool measure(Codepoint cp, int* width /* nullable */, int* height /* nullable */, int offset, const Options* options /* nullable */) = 0;

	/**
	 * @param[out] out
	 * @param[in, out] width
	 * @param[in, out] height
	 */
	virtual bool render(
		Codepoint cp,
		class Image* out /* nullable */,
		const Colour* color,
		int* width /* nullable */, int* height /* nullable */,
		int offset,
		const Options* options /* nullable */
	) = 0;

	virtual bool fromFont(const Font* font) = 0;

	virtual bool fromImage(const class Image* src, int width, int height, int permeation) = 0;

	virtual bool fromTtf(const Byte* data, size_t len, int size, int maxWidth, int maxHeight, int permeation) = 0;

	static Font* create(void);
	static void destroy(Font* ptr);
};

/* ===========================================================================} */

#endif /* __FONT_H__ */
