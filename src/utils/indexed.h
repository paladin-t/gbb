/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __INDEXED_H__
#define __INDEXED_H__

#include "../gbbasic.h"
#include "cloneable.h"
#include "colour.h"
#include "json.h"
#include <map>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef INDEXED_DEFAULT_COLORS
#	define INDEXED_DEFAULT_COLORS { \
		Colour::byRGBA8888(0xffd0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255),  \
		Colour::byRGBA8888(0x80d0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		Colour::byRGB555(0x2a80, 255),    Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGB555(0x93b3, 255),    Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		Colour::byRGB555(0x4004, 255),    Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255),  \
		Colour::byRGB555(0x0068, 255),    Colour::byRGB555(0x7ecf, 255),    Colour::byRGB555(0x40d2, 255),    Colour::byRGB555(0x48f6, 255),  \
		Colour::byRGB555(0x1c07, 255),    Colour::byRGB555(0x075f, 255),    Colour::byRGB555(0x1aa8, 255),    Colour::byRGB555(0x6cde, 255),  \
		Colour::byRGB555(0x040e, 255),    Colour::byRGB555(0xded9, 255),    Colour::byRGB555(0xc068, 255),    Colour::byRGB555(0x987c, 255),  \
		Colour::byRGB555(0x0055, 255),    Colour::byRGB555(0xe51e, 255),    Colour::byRGB555(0x0e0e, 255),    Colour::byRGB555(0x99f7, 255)   \
	}
#endif /* INDEXED_DEFAULT_COLORS */

#ifndef INDEXED_DEFAULT_COLORS_FOR_SUPER_PALETTES
#	define INDEXED_DEFAULT_COLORS_FOR_SUPER_PALETTES { \
		Colour::byRGBA8888(0xffd0f8e0),   Colour::byRGBA8888(0xff70c088),   Colour::byRGBA8888(0xff566834),   Colour::byRGBA8888(0xff201808), \
		                                  Colour::byRGB555(0x68e5, 255),    Colour::byRGB555(0x128d, 255),    Colour::byRGB555(0xfbfb, 255),  \
		Colour::byRGBA8888(0xffd0f8e0),   Colour::byRGB555(0xf8a9, 255),    Colour::byRGB555(0x8b41, 255),    Colour::byRGB555(0xfc76, 255),  \
		                                  Colour::byRGB555(0x3f7c, 255),    Colour::byRGB555(0x2620, 255),    Colour::byRGB555(0xeccf, 255)   \
	}
#endif /* INDEXED_DEFAULT_COLORS_FOR_SUPER_PALETTES */

/* ===========================================================================} */

/*
** {===========================================================================
** Indexed
*/

/**
 * @brief Indexed resource object.
 */
class Indexed : public Cloneable<Indexed>, public virtual Object {
public:
	typedef std::shared_ptr<Indexed> Ptr;

	typedef std::map<Colour, int> Lookup;

public:
	GBBASIC_CLASS_TYPE('I', 'D', 'X', 'D')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Indexed** ptr, bool represented) const = 0;
	using Cloneable<Indexed>::clone;
	using Object::clone;

	/**
	 * @return `SDL_Palette*`.
	 */
	virtual void* pointer(void) = 0;
	/**
	 * @brief Calling this function will make the palette dirty, thus graphics objects need to be refreshed.
	 *
	 * @return `const Colour*`.
	 */
	virtual Colour* pointer(int* n /* nullable */) = 0;
	/**
	 * @return `const Colour*`.
	 */
	virtual const Colour* pointer(int* n /* nullable */) const = 0;

	virtual bool validate(void) = 0;

	virtual int depth(void) const = 0;
	virtual int count(void) const = 0;

	/**
	 * @param[out] col
	 */
	virtual const Colour* get(int index, Colour &col) const = 0;
	virtual bool set(int index, const Colour* col) = 0;

	/**
	 * @param[out] lookup
	 */
	virtual bool match(const class Image* img, Lookup &lookup) const = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(const rapidjson::Value &val) = 0;
	virtual bool fromJson(const rapidjson::Document &val) = 0;

	static Indexed* create(int count);
	static void destroy(Indexed* ptr);
};

/* ===========================================================================} */

#endif /* __INDEXED_H__ */
