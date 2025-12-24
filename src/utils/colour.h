/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COLOUR_H__
#define __COLOUR_H__

#include "../gbbasic.h"
#include "mathematics.h"
#include <string>

/*
** {===========================================================================
** Colour
*/

/**
 * @brief Colour structure.
 *
 * @note Conversion to `UInt32` follows little-endian, vice versa.
 */
struct Colour {
	Byte r = 255;
	Byte g = 255;
	Byte b = 255;
	Byte a = 255;

	Colour();
	Colour(Byte r_, Byte g_, Byte b_);
	Colour(Byte r_, Byte g_, Byte b_, Byte a_);
	Colour(const Colour &other);

	Colour &operator = (const Colour &other);

	bool operator == (const Colour &other) const;
	bool operator != (const Colour &other) const;
	bool operator < (const Colour &other) const;
	bool operator > (const Colour &other) const;

	Colour operator - (void) const;
	Colour operator + (const Colour &other) const;
	Colour operator - (const Colour &other) const;
	Colour operator * (const Colour &other) const;
	Colour operator * (Real other) const;
	Colour &operator += (const Colour &other);
	Colour &operator -= (const Colour &other);
	Colour &operator *= (const Colour &other);
	Colour &operator *= (Real other);

	int compare(const Colour &other) const;
	bool equals(const Colour &other) const;

	int toGray(void) const;

	/**
	 * @brief 5 bits for RGB respectively as little-endian.
	 */
	UInt16 toRGB555(void) const;
	/**
	 * @brief 0xAABBGGRR as little-endian.
	 */
	UInt32 toRGBA(void) const;
	/**
	 * @brief 0xBBGGRRAA as little-endian.
	 */
	UInt32 toARGB(void) const;
	/**
	 * @brief Extracts HSV from colour.
	 *
	 * @param[out] h
	 * @param[out] s
	 * @param[out] v
	 */
	void toHSV(float* h /* nullable */, float* s /* nullable */, float* v /* nullable */, Byte* a = nullptr) const;

	void fromRGB555(UInt16 rgb, Byte a = 255);
	void fromRGB555(Byte r, Byte g, Byte b, Byte a = 255);
	/**
	 * @param[in] rgba 0xAABBGGRR as little-endian.
	 */
	void fromRGBA(UInt32 rgba);
	/**
	 * @param[in] rgba 0xBBGGRRAA as little-endian.
	 */
	void fromARGB(UInt32 argb);
	void fromHSV(float h, float s, float v, Byte a = 255);

	std::string toString(void) const;
	bool fromString(const std::string &str);

	static Colour randomize(Byte a);
	static Colour randomize(void);

	static Colour byRGB555(UInt16 rgb, Byte a = 255);
	static Colour byRGB555(Byte r, Byte g, Byte b, Byte a = 255);

	/**
	 * @param[in] rgba 0xAABBGGRR as little-endian.
	 */
	static Colour byRGBA8888(UInt32 rgba);
	static Colour byRGBA8888(Byte r, Byte g, Byte b, Byte a);
	/**
	 * @param[in] rgba 0xBBGGRRAA as little-endian.
	 */
	static Colour byARGB8888(UInt32 argb);
	static Colour byARGB8888(Byte a, Byte r, Byte g, Byte b);
	static Colour byHSV(float h, float s, float v, Byte a = 255);

	static Colour byString(const std::string &str);
};

/* ===========================================================================} */

#endif /* __COLOUR_H__ */
