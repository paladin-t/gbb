/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "colour.h"
#include "text.h"

/*
** {===========================================================================
** Colour
*/

Colour::Colour() {
}

Colour::Colour(Byte r_, Byte g_, Byte b_) : r(r_), g(g_), b(b_), a(255) {
}

Colour::Colour(Byte r_, Byte g_, Byte b_, Byte a_) : r(r_), g(g_), b(b_), a(a_) {
}

Colour::Colour(const Colour &other) {
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;
}

Colour &Colour::operator = (const Colour &other) {
	r = other.r;
	g = other.g;
	b = other.b;
	a = other.a;

	return *this;
}

bool Colour::operator == (const Colour &other) const {
	return equals(other);
}

bool Colour::operator != (const Colour &other) const {
	return !equals(other);
}

bool Colour::operator < (const Colour &other) const {
	return compare(other) < 0;
}

bool Colour::operator > (const Colour &other) const {
	return compare(other) > 0;
}

Colour Colour::operator - (void) const {
	return Colour(
		(Byte)Math::clamp(255 - r, 0, 255),
		(Byte)Math::clamp(255 - g, 0, 255),
		(Byte)Math::clamp(255 - b, 0, 255),
		a
	);
}

Colour Colour::operator + (const Colour &other) const {
	return Colour(
		(Byte)Math::clamp(r + other.r, 0, 255),
		(Byte)Math::clamp(g + other.g, 0, 255),
		(Byte)Math::clamp(b + other.b, 0, 255),
		(Byte)Math::clamp(a + other.a, 0, 255)
	);
}

Colour Colour::operator - (const Colour &other) const {
	return Colour(
		(Byte)Math::clamp(r - other.r, 0, 255),
		(Byte)Math::clamp(g - other.g, 0, 255),
		(Byte)Math::clamp(b - other.b, 0, 255),
		(Byte)Math::clamp(a - other.a, 0, 255)
	);
}

Colour Colour::operator * (const Colour &other) const {
	return Colour(
		(Byte)Math::clamp(r * (other.r / 255.0f), 0.0f, 255.0f),
		(Byte)Math::clamp(g * (other.g / 255.0f), 0.0f, 255.0f),
		(Byte)Math::clamp(b * (other.b / 255.0f), 0.0f, 255.0f),
		(Byte)Math::clamp(a * (other.a / 255.0f), 0.0f, 255.0f)
	);
}

Colour Colour::operator * (Real other) const {
	return Colour(
		(Byte)Math::clamp(r * other, (Real)0.0f, (Real)255.0f),
		(Byte)Math::clamp(g * other, (Real)0.0f, (Real)255.0f),
		(Byte)Math::clamp(b * other, (Real)0.0f, (Real)255.0f),
		(Byte)Math::clamp(a * other, (Real)0.0f, (Real)255.0f)
	);
}

Colour &Colour::operator += (const Colour &other) {
	r = (Byte)Math::clamp(r + other.r, 0, 255);
	g = (Byte)Math::clamp(g + other.g, 0, 255);
	b = (Byte)Math::clamp(b + other.b, 0, 255);
	a = (Byte)Math::clamp(a + other.a, 0, 255);

	return *this;
}

Colour &Colour::operator -= (const Colour &other) {
	r = (Byte)Math::clamp(r - other.r, 0, 255);
	g = (Byte)Math::clamp(g - other.g, 0, 255);
	b = (Byte)Math::clamp(b - other.b, 0, 255);
	a = (Byte)Math::clamp(a - other.a, 0, 255);

	return *this;
}

Colour &Colour::operator *= (const Colour &other) {
	r = (Byte)Math::clamp(r * (other.r / 255.0f), 0.0f, 255.0f);
	g = (Byte)Math::clamp(g * (other.g / 255.0f), 0.0f, 255.0f);
	b = (Byte)Math::clamp(b * (other.b / 255.0f), 0.0f, 255.0f);
	a = (Byte)Math::clamp(a * (other.a / 255.0f), 0.0f, 255.0f);

	return *this;
}

Colour &Colour::operator *= (Real other) {
	r = (Byte)Math::clamp(r * other, (Real)0.0f, (Real)255.0f);
	g = (Byte)Math::clamp(g * other, (Real)0.0f, (Real)255.0f);
	b = (Byte)Math::clamp(b * other, (Real)0.0f, (Real)255.0f);
	a = (Byte)Math::clamp(a * other, (Real)0.0f, (Real)255.0f);

	return *this;
}

int Colour::compare(const Colour &other) const {
	if (r < other.r)
		return -1;
	else if (r > other.r)
		return 1;

	if (g < other.g)
		return -1;
	else if (g > other.g)
		return 1;

	if (b < other.b)
		return -1;
	else if (b > other.b)
		return 1;

	if (a < other.a)
		return -1;
	else if (a > other.a)
		return 1;

	return 0;
}

bool Colour::equals(const Colour &other) const {
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

int Colour::toGray(void) const {
	const int y = (int)(r * 0.3 + g * 0.59 + b * 0.11);

	return y;
}

UInt16 Colour::toRGB555(void) const {
	return (UInt16)(((r >> 3) & 0x1f) | (((g >> 3) & 0x1f) << 5) | (((b >> 3) & 0x1f) << 10));
}

UInt32 Colour::toRGBA(void) const {
	return (r) | (g << 8) | (b << 16) | (a << 24);
}

UInt32 Colour::toARGB(void) const {
	return (a) | (r << 8) | (g << 16) | (b << 24);
}

void Colour::toHSV(float* h_, float* s_, float* v_, Byte* a_) const {
	if (h_)
		*h_ = 0.0f;
	if (s_)
		*s_ = 0.0f;
	if (v_)
		*v_ = 0.0f;
	if (a_)
		*a_ = 0;

	const float r_ = r / 255.0f;
	const float g_ = g / 255.0f;
	const float b_ = b / 255.0f;
	
	const float cmax = Math::max(r_, g_, b_);
	const float cmin = Math::min(r_, g_, b_);
	const float delta = cmax - cmin;
	
	const float v = cmax;
	
	float s = 0.0f;
	if (cmax < 1e-5f)
		s = 0;
	else
		s = delta / cmax;
	
	float h = 0.0f;
	if (delta < 1e-5f)
		h = 0;
	else if (cmax == r_)
		h = 60 * (float)std::fmod((g_ - b_) / delta, 6);
	else if (cmax == g_)
		h = 60 * (((b_ - r_) / delta) + 2);
	else
		h = 60 * (((r_ - g_) / delta) + 4);
	if (h < 0)
		h += 360;

	if (h_)
		*h_ = h;
	if (s_)
		*s_ = s;
	if (v_)
		*v_ = v;
	if (a_)
		*a_ = a;
}

void Colour::fromRGB555(UInt16 rgb, Byte a_) {
	r = (Byte)((rgb & 0x1f) << 3);
	g = (Byte)(((rgb >> 5) & 0x1f) << 3);
	b = (Byte)(((rgb >> 10) & 0x1f) << 3);
	a = a_;
}

void Colour::fromRGB555(Byte r_, Byte g_, Byte b_, Byte a_) {
	r = (Byte)(r_ << 3);
	g = (Byte)(g_ << 3);
	b = (Byte)(b_ << 3);
	a = a_;
}

void Colour::fromRGBA(UInt32 rgba) {
	r = (Byte)(rgba & 0x000000ff);
	g = (Byte)((rgba & 0x0000ff00) >> 8);
	b = (Byte)((rgba & 0x00ff0000) >> 16);
	a = (Byte)((rgba & 0xff000000) >> 24);
}

void Colour::fromARGB(UInt32 argb) {
	a = (Byte)(argb & 0x000000ff);
	r = (Byte)((argb & 0x0000ff00) >> 8);
	g = (Byte)((argb & 0x00ff0000) >> 16);
	b = (Byte)((argb & 0xff000000) >> 24);
}

void Colour::fromHSV(float h, float s, float v, Byte a_) {
	h = Math::wrap(h, 0.0f, 360.0f);
	s = Math::clamp(s, 0.0f, 1.0f);
	v = Math::clamp(v, 0.0f, 1.0f);
	
	const float c = v * s;
	const float x = c * (1 - std::abs((float)std::fmod(h / 60.0f, 2) - 1));
	const float m = v - c;
	
	float r_, g_, b_;
	if (h < 60) {
		r_ = c; g_ = x; b_ = 0;
	} else if (h < 120) {
		r_ = x; g_ = c; b_ = 0;
	} else if (h < 180) {
		r_ = 0; g_ = c; b_ = x;
	} else if (h < 240) {
		r_ = 0; g_ = x; b_ = c;
	} else if (h < 300) {
		r_ = x; g_ = 0; b_ = c;
	} else {
		r_ = c; g_ = 0; b_ = x;
	}
	
	r = (Byte)((r_ + m) * 255);
	g = (Byte)((g_ + m) * 255);
	b = (Byte)((b_ + m) * 255);
	a = a_;
}

std::string Colour::toString(void) const {
	std::string ret = "#";
	ret += Text::toHex(r, 2, '0', false);
	ret += Text::toHex(g, 2, '0', false);
	ret += Text::toHex(b, 2, '0', false);
	ret += Text::toHex(a, 2, '0', false);

	return ret;
}

bool Colour::fromString(const std::string &str) {
	std::string hex = str;
	if (hex[0] != '#')
		return false;

	hex = hex.substr(1);

	if (hex.length() == 3 || hex.length() == 4) {
		std::string fullHex;
		for (char c : hex) {
			fullHex += c;
			fullHex += c;
		}
		hex = fullHex;
	}

	if (hex.length() != 6 && hex.length() != 8)
		return false;

	UInt8 b0 = 0;
	UInt8 b1 = 0;
	UInt8 b2 = 0;
	UInt8 b3 = 0;
	UInt8 b4 = 0;
	UInt8 b5 = 0;
	UInt8 b6 = 0;
	UInt8 b7 = 0;
	if (
		!Text::fromHexCharacter(hex[0], b0) ||
		!Text::fromHexCharacter(hex[1], b1) ||
		!Text::fromHexCharacter(hex[2], b2) ||
		!Text::fromHexCharacter(hex[3], b3) ||
		!Text::fromHexCharacter(hex[4], b4) ||
		!Text::fromHexCharacter(hex[5], b5) ||
		!Text::fromHexCharacter(hex[6], b6) ||
		!Text::fromHexCharacter(hex[7], b7)
	) {
		return false;
	}

	r = b0 * 16 + b1;
	g = b2 * 16 + b3;
	b = b4 * 16 + b5;
	a = (hex.length() == 8) ?
		b6 * 16 + b7 : 255;

	return true;
}

Colour Colour::randomize(Byte a) {
	return Colour(
		Math::rand() % (std::numeric_limits<Byte>::max() + 1),
		Math::rand() % (std::numeric_limits<Byte>::max() + 1),
		Math::rand() % (std::numeric_limits<Byte>::max() + 1),
		a
	);
}

Colour Colour::randomize(void) {
	return Colour(
		Math::rand() % (std::numeric_limits<Byte>::max() + 1),
		Math::rand() % (std::numeric_limits<Byte>::max() + 1),
		Math::rand() % (std::numeric_limits<Byte>::max() + 1),
		Math::rand() % (std::numeric_limits<Byte>::max() + 1)
	);
}

Colour Colour::byRGB555(UInt16 rgb, Byte a) {
	Colour result;
	result.fromRGB555(rgb, a);

	return result;
}

Colour Colour::byRGB555(Byte r, Byte g, Byte b, Byte a) {
	Colour result;
	result.fromRGB555(r, g, b, a);

	return result;
}

Colour Colour::byRGBA8888(UInt32 rgba) {
	Colour result;
	result.fromRGBA(rgba);

	return result;
}

Colour Colour::byRGBA8888(Byte r, Byte g, Byte b, Byte a) {
	Colour result(r, g, b, a);

	return result;
}

Colour Colour::byARGB8888(UInt32 argb) {
	Colour result;
	result.fromARGB(argb);

	return result;
}

Colour Colour::byARGB8888(Byte a, Byte r, Byte g, Byte b) {
	Colour result(r, g, b, a);

	return result;
}

Colour Colour::byHSV(float h, float s, float v, Byte a) {
	Colour result;
	result.fromHSV(h, s, v, a);

	return result;
}

Colour Colour::byString(const std::string &str) {
	Colour result;
	result.fromString(str);

	return result;
}

/* ===========================================================================} */
