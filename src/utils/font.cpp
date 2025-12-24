/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "assets.h"
#include "font.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../lib/stb/stb_truetype.h"

/*
** {===========================================================================
** Font
*/

class FontImpl : public Font {
private:
	Bytes::Ptr _data = nullptr;
	int _maxWidth = -1;
	int _maxHeight = -1;
	int _permeation = 1;

	bool _trim = true; // Whether to trim font height.
	Bytes* _glyph = nullptr;

	int _imagePaletted = -1;
	int _imageWidth = -1;
	int _imageHeight = -1;
	int _imageCharacterWidth = -1;
	int _imageCharacterHeight = -1;

	stbtt_fontinfo _fontInfo;
	int _fontHeight = -1;
	float _fontScale = 1.0f;

public:
	FontImpl() {
	}
	virtual ~FontImpl() override {
		clear();

		if (_glyph) {
			Bytes::destroy(_glyph);
			_glyph = nullptr;
		}
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual void* pointer(void) override {
		if (!_data)
			return nullptr;

		return _data->pointer();
	}
	virtual void* pointer(size_t* len) override {
		if (len)
			*len = 0;

		if (!_data)
			return nullptr;

		if (len)
			*len = _data->count();

		return _data->pointer();
	}

	virtual bool imageBased(void) const override {
		return _imagePaletted >= 0 && _imageWidth > 0 && _imageHeight > 0 && _imageCharacterWidth > 0 && _imageCharacterHeight > 0;
	}
	virtual bool ttfBased(void) const override {
		return _fontHeight > 0;
	}

	virtual bool trim(void) const override {
		return _trim;
	}
	virtual void trim(bool val) override {
		_trim = val;
	}

	virtual int glyphCount(void) const override {
		if (imageBased()) {
			const int xCount = _imageWidth / _imageCharacterWidth;
			const int yCount = _imageHeight / _imageCharacterHeight;

			return xCount * yCount;
		} else if (ttfBased()) {
			return _fontInfo.numGlyphs;
		}

		return 0;
	}

	virtual bool measure(Codepoint cp, int* width, int* height, int offset, const Options* options) override {
		if (imageBased())
			return renderWithImage(cp, nullptr, nullptr, width, height, offset, options);
		else if (ttfBased())
			return renderWithFontInfo(cp, nullptr, nullptr, width, height, offset, options);

		return false;
	}

	virtual bool render(
		Codepoint cp,
		class Image* out,
		const Colour* color,
		int* width, int* height,
		int offset,
		const Options* options
	) override {
		if (imageBased())
			return renderWithImage(cp, out, color, width, height, offset, options);
		else if (ttfBased())
			return renderWithFontInfo(cp, out, color, width, height, offset, options);

		return false;
	}

	virtual bool fromFont(const Font* font) override {
		clear();

		if (!font)
			return false;

		const FontImpl* impl = static_cast<const FontImpl*>(font);

		if (impl->_data) {
			_data = Bytes::Ptr(Bytes::create());
			_data->writeBytes(impl->_data.get());
			_data->poke(impl->_data->peek());
		} else {
			_data = nullptr;
		}
		_maxWidth = impl->_maxWidth;
		_maxHeight = impl->_maxHeight;
		_permeation = impl->_permeation;
		_trim = impl->_trim;

		_imagePaletted = impl->_imagePaletted;
		_imageWidth = impl->_imageWidth;
		_imageHeight = impl->_imageHeight;
		_imageCharacterWidth = impl->_imageCharacterWidth;
		_imageCharacterHeight = impl->_imageCharacterHeight;

		memcpy(&_fontInfo, &impl->_fontInfo, sizeof(stbtt_fontinfo));
		_fontHeight = impl->_fontHeight;
		_fontScale = impl->_fontScale;
		if (ttfBased())
			return initializeWithFontInfo(_fontHeight);

		return false;
	}

	virtual bool fromImage(const class Image* src, int width, int height, int permeation) override {
		clear();

		if (!src || width <= 0 || height <= 0)
			return false;

		_data = Bytes::Ptr(Bytes::create());
		_maxWidth = -1;
		_maxHeight = -1;
		_permeation = permeation;
		const int paletted = src->paletted();
		if (paletted)
			_data->writeBytes(src->pixels(), src->width() * src->height() * paletted);
		else
			_data->writeBytes(src->pixels(), src->width() * src->height() * sizeof(Colour));
		if (_glyph)
			_glyph->clear();

		_imagePaletted = paletted;
		_imageWidth = src->width();
		_imageHeight = src->height();
		_imageCharacterWidth = width;
		_imageCharacterHeight = height;

		return true;
	}

	virtual bool fromTtf(const Byte* data, size_t len, int size, int maxWidth, int maxHeight, int permeation) override {
		clear();

		if (!data || !len)
			return false;

		_data = Bytes::Ptr(Bytes::create());
		_data->writeBytes(data, len);
		_maxWidth = maxWidth;
		_maxHeight = maxHeight;
		_permeation = permeation;

		return initializeWithFontInfo(size);
	}

private:
	void clear(void) {
		_data = nullptr;
		_maxWidth = -1;
		_maxHeight = -1;
		_permeation = 1;

		// `_trim` is not cleared.
		// `_glyph` is not cleared.

		_imagePaletted = -1;
		_imageWidth = -1;
		_imageHeight = -1;
		_imageCharacterWidth = -1;
		_imageCharacterHeight = -1;

		memset(&_fontInfo, 0, sizeof(stbtt_fontinfo));
		_fontHeight = -1;
		_fontScale = 1.0f;
	}

	bool renderWithImage(
		Codepoint cp,
		class Image* out /* nullable */,
		const Colour* color,
		int* width /* nullable */, int* height /* nullable */,
		int offset,
		const Options* options /* nullable */
	) {
		if (!_data)
			return false;

		auto fill = [] (
			Image* out, Bytes* data, int width_, int height_, int xIndex, int yIndex, int imagePaletted, int imageWidth, int /* imageHeight */, int imageCharacterWidth, int imageCharacterHeight, const Colour* color, int permeation,
			float strength, int paddingX, int paddingY, int offsetX, int offsetY, std::function<bool(const Colour &, const Colour &)> pass
		) -> void {
			for (int j = 0; j < height_; ++j) {
				for (int i = 0; i < width_; ++i) {
					unsigned a = 0;
					const size_t index = (xIndex * imageCharacterWidth + i) + (yIndex * imageCharacterHeight + j) * imageWidth;
					if (imagePaletted > 0)
						a = data->get(index);
					else
						a = data->get(index * sizeof(Colour) + 3);
					if (permeation > 0)
						a = a >= (unsigned)permeation ? a : 0; // a = a >= (unsigned)permeation ? 255 : 0;

					if (a == 0)
						continue;

					a = (unsigned)(a * strength);

					const int x = i + offsetX;
					const int y = j + offsetY;
					if (x < 0 || x >= out->width() || y < 0 || y >= out->height()) {
						if (!paddingX && !paddingY) {
							GBBASIC_ASSERT(false && "Font position out of bounds.");
						}

						continue;
					}
					Colour col = *color;
					if (col.a == 255) {
						col.a = (Byte)a;
					} else {
						if (a < 255)
							col.a = (Byte)Math::clamp(col.a / 255.0f * a, 0.0f, 255.0f);
					}
					bool passed = true;
					if (pass) {
						Colour bld;
						out->get(x, y, bld);
						passed = pass(col, bld);
					}
					if (passed) {
						out->set(x, y, col);
					}
				}
			}
		};

		if (_imageCharacterWidth <= 0 || _imageCharacterHeight <= 0)
			return false;

		const Effects enabledEffect = options ? options->enabledEffect : Effects::NONE;
		const int shadowOffset = options ? options->shadowOffset : 0;
		const float shadowStrength = options ? options->shadowStrength : 0;
		const unsigned shadowDirection = options ? options->shadowDirection : 4;
		const int outlineOffset = options ? options->outlineOffset : 0;
		const float outlineStrength = options ? options->outlineStrength : 0;
		const int effectThreshold = options ? options->effectThreshold : 0;
		int offsetX = 0;
		int offsetY = 0;
		if (enabledEffect == Effects::SHADOW && shadowOffset > 0) {
			switch ((Directions)shadowDirection) {
			case Directions::DIRECTION_DOWN:
				offsetX = 0;
				offsetY = shadowOffset;

				break;
			case Directions::DIRECTION_RIGHT:
				offsetX = shadowOffset;
				offsetY = 0;

				break;
			case Directions::DIRECTION_UP:
				offsetX = 0;
				offsetY = -shadowOffset;

				break;
			case Directions::DIRECTION_LEFT:
				offsetX = -shadowOffset;
				offsetY = 0;

				break;
			case Directions::DIRECTION_DOWN_RIGHT:
				offsetX = shadowOffset;
				offsetY = shadowOffset;

				break;
			case Directions::DIRECTION_UP_RIGHT:
				offsetX = shadowOffset;
				offsetY = -shadowOffset;

				break;
			case Directions::DIRECTION_UP_LEFT:
				offsetX = -shadowOffset;
				offsetY = -shadowOffset;

				break;
			case Directions::DIRECTION_DOWN_LEFT:
				offsetX = -shadowOffset;
				offsetY = shadowOffset;

				break;
			default: // Do nothing.
				break;
			}
		} else if (enabledEffect == Effects::OUTLINE && outlineOffset > 0) {
			offsetX = outlineOffset * 2;
			offsetY = outlineOffset * 2;
		}

		int width_ = width ? *width : -1;
		int height_ = height ? *height : -1;

		if (width_ <= 0)
			width_ = _imageCharacterWidth;
		if (height_ <= 0)
			height_ = _imageCharacterHeight;

		const int newWidth = width_ + std::abs(offsetX);
		const int newHeight = height_ + std::abs(offsetY);
		if (width)
			*width = newWidth;
		if (height)
			*height = newHeight;

		const int xCount = _imageWidth / _imageCharacterWidth;
		const int yCount = _imageHeight / _imageCharacterHeight;
		if (cp >= (Codepoint)(xCount * yCount))
			return false;
		if (xCount <= 0)
			return false;

		const std::div_t div = std::div((int)cp, xCount);
		const int xIndex = div.rem;
		const int yIndex = div.quot;

		if (out) {
			out->fromBlank(newWidth, newHeight, 0);
			if (enabledEffect == Effects::SHADOW && shadowOffset > 0) {
				const int dx = offsetX >= 0 ? 0 : 1;
				const int dy = offsetY >= 0 ? 0 : 1;
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					shadowStrength, std::abs(offsetX), std::abs(offsetY), offsetX + dx, offsetY + dy,
					nullptr
				);
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					1, std::abs(offsetX), std::abs(offsetY), 0 + dx, 0 + dy,
					[effectThreshold] (const Colour &col, const Colour &) -> bool {
						return col.a >= effectThreshold;
					}
				);
			} else if (enabledEffect == Effects::OUTLINE && outlineOffset > 0) {
				const int dx = 1;
				const int dy = 1;
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, -outlineOffset + dx, 0 + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, outlineOffset + dx, 0 + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, 0 + dx, -outlineOffset + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, 0 + dx, outlineOffset + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					1, outlineOffset * 2, outlineOffset * 2, 0 + dx, 0 + dy,
					[effectThreshold] (const Colour &col, const Colour &) -> bool {
						return col.a >= effectThreshold;
					}
				);
			} else {
				fill(
					out, _data.get(), width_, height_, xIndex, yIndex, _imagePaletted, _imageWidth, _imageHeight, _imageCharacterWidth, _imageCharacterHeight, color, _permeation,
					1, 0, 0, 0, offset,
					nullptr
				);
			}
		}

		return true;
	}

	bool initializeWithFontInfo(int size) {
		bool result = false;
		if (_data && !_data->empty() && (size > 0 && _fontHeight != size)) {
			_fontHeight = size;

			result = !!stbtt_InitFont(&_fontInfo, _data->pointer(), stbtt_GetFontOffsetForIndex(_data->pointer(), 0));
			_fontScale = stbtt_ScaleForPixelHeight(&_fontInfo, (float)_fontHeight);
		}

		return result;
	}
	bool renderWithFontInfo(
		Codepoint cp,
		class Image* out /* nullable */,
		const Colour* color,
		int* width /* nullable */, int* height /* nullable */,
		int offset,
		const Options* options /* nullable */
	) {
		if (!_data)
			return false;

		const int idx = stbtt_FindGlyphIndex(&_fontInfo, cp);
		if (idx == 0)
			return false;

		auto measure = [] (const stbtt_fontinfo &font, Codepoint cp, float scale, int w) -> int {
			int advWidth = 0;
			int leftBearing = 0;
			stbtt_GetCodepointHMetrics(&font, cp, &advWidth, &leftBearing);

			return Math::max(w, (int)(advWidth * scale));
		};
		auto fill = [] (
			Image* out, Bytes* glyph, int width_, int height_, int /* x0 */, int y0, int /* x1 */, int y1, const Colour* color, int permeation,
			float strength, int paddingX, int paddingY, int offsetX, int offsetY, std::function<bool(const Colour &, const Colour &)> pass
		) -> void {
			for (int j = 0; j < height_; ++j) {
				for (int i = 0; i < width_; ++i) {
					unsigned a = glyph->get(i + j * width_);
					if (permeation > 0)
						a = a >= (unsigned)permeation ? a : 0; // a = a >= (unsigned)permeation ? 255 : 0;

					if (a == 0)
						continue;

					a = (unsigned)(a * strength);

					const int x = i + offsetX;
					const int y = (j + height_ + y0 - y1) + offsetY;
					if (x < 0 || x >= out->width() || y < 0 || y >= out->height()) {
						if (!paddingX && !paddingY) {
							// GBBASIC_ASSERT(false && "Font position out of bounds.");
						}

						continue;
					}
					Colour col = *color;
					if (col.a == 255) {
						col.a = (Byte)(a);
					} else {
						if (a < 255)
							col.a = (Byte)Math::clamp(col.a / 255.0f * a, 0.0f, 255.0f);
					}
					bool passed = true;
					if (pass) {
						Colour bld;
						out->get(x, y, bld);
						passed = pass(col, bld);
					}
					if (passed) {
						out->set(x, y, col);
					}
				}
			}
		};

		const Effects enabledEffect = options ? options->enabledEffect : Effects::NONE;
		const int shadowOffset = options ? options->shadowOffset : 0;
		const float shadowStrength = options ? options->shadowStrength : 0;
		const unsigned shadowDirection = options ? options->shadowDirection : 4;
		const int outlineOffset = options ? options->outlineOffset : 0;
		const float outlineStrength = options ? options->outlineStrength : 0;
		const int effectThreshold = options ? options->effectThreshold : 0;
		int offsetX = 0;
		int offsetY = 0;
		if (enabledEffect == Effects::SHADOW && shadowOffset > 0) {
			switch ((Directions)shadowDirection) {
			case Directions::DIRECTION_DOWN:
				offsetX = 0;
				offsetY = shadowOffset;

				break;
			case Directions::DIRECTION_RIGHT:
				offsetX = shadowOffset;
				offsetY = 0;

				break;
			case Directions::DIRECTION_UP:
				offsetX = 0;
				offsetY = -shadowOffset;

				break;
			case Directions::DIRECTION_LEFT:
				offsetX = -shadowOffset;
				offsetY = 0;

				break;
			case Directions::DIRECTION_DOWN_RIGHT:
				offsetX = shadowOffset;
				offsetY = shadowOffset;

				break;
			case Directions::DIRECTION_UP_RIGHT:
				offsetX = shadowOffset;
				offsetY = -shadowOffset;

				break;
			case Directions::DIRECTION_UP_LEFT:
				offsetX = -shadowOffset;
				offsetY = -shadowOffset;

				break;
			case Directions::DIRECTION_DOWN_LEFT:
				offsetX = -shadowOffset;
				offsetY = shadowOffset;

				break;
			default: // Do nothing.
				break;
			}
		} else if (enabledEffect == Effects::OUTLINE && outlineOffset > 0) {
			offsetX = outlineOffset * 2;
			offsetY = outlineOffset * 2;
		}

		if (!_glyph)
			_glyph = Bytes::create();

		int width_ = width ? *width : -1;
		int height_ = height ? *height : -1;

		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		stbtt_GetCodepointBitmapBox(&_fontInfo, cp, _fontScale, _fontScale, &x0, &y0, &x1, &y1);
		if (width_ <= 0)
			width_ = measure(_fontInfo, cp, _fontScale, width_);
		if (height_ <= 0)
			height_ = _fontHeight + y1;
		_glyph->resize(width_ * height_);
		stbtt_MakeCodepointBitmap(&_fontInfo, _glyph->pointer(), width_, height_, width_, _fontScale, _fontScale, cp);

		const int newWidth = Math::min(
			width_ + std::abs(offsetX),
			_maxWidth > 0 ? Math::min(_maxWidth, ASSETS_FONT_MAX_SIZE) : ASSETS_FONT_MAX_SIZE
		);
		const int newHeight = Math::min(
			height_ + std::abs(offsetY),
			_maxHeight > 0 ? Math::min(_maxHeight, ASSETS_FONT_MAX_SIZE) : ASSETS_FONT_MAX_SIZE
		);
		if (width)
			*width = newWidth;
		if (height)
			*height = newHeight;

		if (out) {
			out->fromBlank(newWidth, newHeight, 0);
			if (enabledEffect == Effects::SHADOW && shadowOffset > 0) {
				const int dx = offsetX >= 0 ? 0 : 1;
				const int dy = offsetY >= 0 ? 0 : 1;
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					shadowStrength, std::abs(offsetX), std::abs(offsetY), offsetX + dx, offsetY + dy,
					nullptr
				);
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					1, std::abs(offsetX), std::abs(offsetY), 0 + dx, 0 + dy,
					[effectThreshold] (const Colour &col, const Colour &) -> bool {
						return col.a >= effectThreshold;
					}
				);

				if (_trim) {
					if (height && *height > _fontHeight + dy) {
						out->resize(out->width(), _fontHeight + dy, false);
						*height = _fontHeight + dy;
					}
				}
			} else if (enabledEffect == Effects::OUTLINE && outlineOffset > 0) {
				constexpr const int dx = 1;
				constexpr const int dy = 1;
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, -outlineOffset + dx, 0 + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, outlineOffset + dx, 0 + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, 0 + dx, -outlineOffset + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					outlineStrength, outlineOffset * 2, outlineOffset * 2, 0 + dx, outlineOffset + dy,
					[] (const Colour &, const Colour &bld) -> bool {
						return bld.a == 0;
					}
				);
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					1, outlineOffset * 2, outlineOffset * 2, 0 + dx, 0 + dy,
					[effectThreshold] (const Colour &col, const Colour &) -> bool {
						return col.a >= effectThreshold;
					}
				);

				if (_trim) {
					if (height && *height > _fontHeight + dy * 2) {
						out->resize(out->width(), _fontHeight + dy * 2, false);
						*height = _fontHeight + dy;
					}
				}
			} else {
				fill(
					out, _glyph, width_, height_, x0, y0, x1, y1, color, _permeation,
					1, 0, 0, 0, offset,
					nullptr
				);

				if (_trim) {
					if (height && *height > _fontHeight) {
						out->resize(out->width(), _fontHeight, false);
						*height = _fontHeight;
					}
				}
			}
		}

		_glyph->clear();

		return true;
	}
};

Font::Codepoints::iterator::iterator() {
}

Font::Codepoints::iterator::iterator(int idx, value_type* raw) : _index(idx), _raw(raw) {
}

bool Font::Codepoints::iterator::operator == (const iterator &other) const {
	return _index == other._index;
}

bool Font::Codepoints::iterator::operator != (const iterator &other) const {
	return _index != other._index;
}

bool Font::Codepoints::iterator::operator <= (const iterator &other) const {
	return _index <= other._index;
}

bool Font::Codepoints::iterator::operator >= (const iterator &other) const {
	return _index >= other._index;
}

bool Font::Codepoints::iterator::operator < (const iterator &other) const {
	return _index < other._index;
}

bool Font::Codepoints::iterator::operator > (const iterator &other) const {
	return _index > other._index;
}

Font::Codepoints::iterator &Font::Codepoints::iterator::operator ++ (void) {
	++_index;

	return *this;
}

Font::Codepoints::iterator &Font::Codepoints::iterator::operator -- (void) {
	--_index;

	return *this;
}

Font::Codepoints::iterator Font::Codepoints::iterator::operator ++ (int) {
	const iterator old(*this);
	++_index;

	return old;
}

Font::Codepoints::iterator Font::Codepoints::iterator::operator -- (int) {
	const iterator old(*this);
	--_index;

	return old;
}

Font::Codepoints::iterator Font::Codepoints::iterator::operator + (difference_type n) const {
	return iterator(_index + n, _raw);
}

Font::Codepoints::iterator Font::Codepoints::iterator::operator - (difference_type n) const {
	return iterator(_index - n, _raw);
}

Font::Codepoints::iterator::difference_type Font::Codepoints::iterator::operator - (const iterator &other) const {
	return _index - other._index;
}

Font::Codepoints::iterator &Font::Codepoints::iterator::operator += (difference_type n) {
	_index += n;

	return *this;
}

Font::Codepoints::iterator &Font::Codepoints::iterator::operator -= (difference_type n) {
	_index -= n;

	return *this;
}

Font::Codepoints::iterator::reference Font::Codepoints::iterator::operator * (void) const {
	return _raw[_index];
}

Font::Codepoints::iterator::pointer Font::Codepoints::iterator::operator -> (void) const {
	return &_raw[_index];
}

Font::Codepoints::iterator::reference Font::Codepoints::iterator::operator [] (difference_type offset) const {
	return _raw[_index + offset];
}

Font::Codepoints::Codepoints() {
}

Font::Codepoints::~Codepoints() {
}

unsigned Font::Codepoints::type(void) const {
	return TYPE();
}

bool Font::Codepoints::clone(Codepoints** ptr) const {
	if (!ptr)
		return false;

	*ptr = nullptr;

	Codepoints* result = new Codepoints();

	result->_values = _values;

	*ptr = result;

	return true;
}

bool Font::Codepoints::operator == (const Codepoints &other) const {
	return _values == other._values;
}

bool Font::Codepoints::operator != (const Codepoints &other) const {
	return _values != other._values;
}

Font::Codepoint Font::Codepoints::operator [] (int idx) const {
	return _values[idx];
}

Font::Codepoints::iterator Font::Codepoints::begin(void) {
	iterator result;
	if (!_values.empty())
		result = iterator(0, &_values.front());

	return result;
}

Font::Codepoints::iterator Font::Codepoints::end(void) {
	iterator result;
	if (!_values.empty())
		result = iterator((int)_values.size(), &_values.front());

	return result;
}

int Font::Codepoints::count(void) const {
	return (int)_values.size();
}

void Font::Codepoints::add(Codepoint cp) {
	_values.push_back(cp);
}

Font::Codepoint Font::Codepoints::get(int idx) const {
	return _values[idx];
}

bool Font::Codepoints::set(int idx, Codepoint val) {
	if (idx < 0 || idx >= (int)_values.size())
		return false;

	_values[idx] = val;

	return true;
}

bool Font::Codepoints::remove(int idx) {
	if (idx < 0 || idx >= (int)_values.size())
		return false;

	_values.erase(_values.begin() + idx);

	return true;
}

void Font::Codepoints::clear(void) {
	_values.clear();
}

Font* Font::create() {
	FontImpl* result = new FontImpl();

	return result;
}

void Font::destroy(Font* ptr) {
	FontImpl* impl = static_cast<FontImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
