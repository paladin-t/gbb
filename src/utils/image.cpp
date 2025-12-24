/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "image.h"
#include "plus.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../lib/stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../lib/stb/stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../lib/stb/stb_image_write.h"
#include <SDL.h>
#include <set>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef IMAGE_LOCK_SURFACE
#	define IMAGE_LOCK_SURFACE(SUR) \
	ProcedureGuard<bool> GBBASIC_UNIQUE_NAME(__LOCK__)( \
		std::bind( \
			[&] (SDL_Surface* surface) -> bool* { \
				bool* result = (bool*)(uintptr_t)(SDL_MUSTLOCK(SUR) ? 0x1 : 0x0); \
				if (result) \
					SDL_LockSurface(surface); \
				return result; \
			}, \
			(SUR) \
		), \
		std::bind( \
			[&] (SDL_Surface* surface, bool* locked) -> void { \
				if (locked) \
					SDL_UnlockSurface(surface); \
			}, \
			(SUR), std::placeholders::_1 \
		) \
	);
#endif /* IMAGE_LOCK_SURFACE */

static const Byte IMAGE_PALETTED_HEADER_BYTES[] = IMAGE_PALETTED_HEADER;
static const Byte IMAGE_COLORED_HEADER_BYTES[] = IMAGE_COLORED_HEADER;

/* ===========================================================================} */

/*
** {===========================================================================
** Image
*/

class ImageImpl : public Image {
private:
	bool _blank = true;
	Indexed::Ptr _palette = nullptr;
	int _palettedBits = 0;
	int _transparentIndex = 0;
	Byte* _pixels = nullptr;
	int _width = 0;
	int _height = 0;
	int _channels = 0;

	SDL_Surface* _surface = nullptr;

	int _quantizationRedWeight = 1;
	int _quantizationGreenWeight = 1;
	int _quantizationBlueWeight = 1;
	int _quantizationAlphaWeight = 4;

public:
	ImageImpl(Indexed::Ptr palette) : _palette(palette) {
	}
	virtual ~ImageImpl() override {
		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Image** ptr) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		ImageImpl* result = static_cast<ImageImpl*>(Image::create(_palette));

		result->_blank = _blank;
		result->_palettedBits = _palettedBits;
		result->_transparentIndex = _transparentIndex;
		result->_pixels = (Byte*)malloc(_width * _height * _channels);
		memcpy(result->_pixels, _pixels, _width * _height * _channels);
		result->_width = _width;
		result->_height = _height;
		result->_channels = _channels;

		result->_quantizationRedWeight = _quantizationRedWeight;
		result->_quantizationGreenWeight = _quantizationGreenWeight;
		result->_quantizationBlueWeight = _quantizationBlueWeight;
		result->_quantizationAlphaWeight = _quantizationAlphaWeight;

		*ptr = result;

		return true;
	}
	virtual bool clone(Object** ptr) const override {
		Image* obj = nullptr;
		if (!clone(&obj))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		for (int j = 0; j < height(); ++j) {
			for (int i = 0; i < width(); ++i) {
				if (paletted()) {
					int val = 0;
					get(i, j, val);
					result = Math::hash(result, val);
				} else {
					Colour val;
					get(i, j, val);
					if (val.a == 0)
						val.r = val.g = val.b = 255;
					result = Math::hash(result, val.toRGBA());
				}
			}
		}

		return result;
	}
	virtual int compare(const Image* other) const override {
		if (this == other)
			return 0;

		if (!other)
			return 1;

		if (paletted() < other->paletted())
			return -1;
		else if (paletted() > other->paletted())
			return 1;

		if (width() < other->width())
			return -1;
		else if (width() > other->width())
			return 1;

		if (height() < other->height())
			return -1;
		else if (height() > other->height())
			return 1;

		if (blank() && other->blank())
			return 0;

		for (int j = 0; j < height(); ++j) {
			for (int i = 0; i < width(); ++i) {
				if (paletted()) {
					int val0 = 0;
					int val1 = 0;
					get(i, j, val0);
					other->get(i, j, val1);
					if (val0 < val1)
						return -1;
					else if (val0 > val1)
						return 1;
				} else {
					Colour val0;
					Colour val1;
					get(i, j, val0);
					other->get(i, j, val1);
					if (val0.a == 0)
						val0.r = val0.g = val0.b = 255;
					if (val1.a == 0)
						val1.r = val1.g = val1.b = 255;
					if (val0.a != 0 || val1.a != 0) {
						if (val0 < val1)
							return -1;
						else if (val0 > val1)
							return 1;
					}
				}
			}
		}

		return 0;
	}

	virtual void* pointer(void) override {
		return surface();
	}

	virtual bool blank(void) const override {
		return _blank;
	}

	virtual const Indexed::Ptr palette(void) const override {
		return _palette;
	}
	virtual void palette(Indexed::Ptr val) override {
		_palette = val;
	}
	virtual int paletted(void) const override {
		return _palettedBits;
	}
	virtual int transparentIndex(void) const override {
		return _transparentIndex;
	}
	virtual void transparentIndex(int val) override {
		_transparentIndex = val;
	}

	virtual const Byte* pixels(void) const override {
		return _pixels;
	}
	virtual Byte* pixels(void) override {
		return _pixels;
	}

	virtual int width(void) const override {
		return _width;
	}
	virtual int height(void) const override {
		return _height;
	}

	virtual int channels(void) const override {
		return _channels;
	}

	virtual bool resize(int width, int height, bool stretch) override {
		if (width <= 0 || height <= 0)
			return false;

		if (width > GBBASIC_TEXTURE_SAFE_MAX_WIDTH || height > GBBASIC_TEXTURE_SAFE_MAX_HEIGHT)
			return false;

		if (_palettedBits) {
			if (_pixels && stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Byte));
				stbir_resize_uint8_linear(_pixels, _width, _height, 0, tmp, width, height, 0, (stbir_pixel_layout)_channels);
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 1;
			} else if (_pixels && !stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Byte));
				memset(tmp, 0, width * height * sizeof(Byte));
				for (int j = 0; j < height; ++j) {
					for (int i = 0; i < width; ++i) {
						int index = 0;
						if (get(i, j, index)) {
							Byte* unit = &tmp[(i + j * width) * _channels];
							*unit = (Byte)index;
						}
					}
				}
				surface(nullptr);
				_blank = blank;
				if (_pixels) {
					free(_pixels);
					_pixels = nullptr;
				}
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 1;
			} else {
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Byte));
				memset(tmp, 0, width * height * sizeof(Byte));
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 1;
			}
		} else {
			if (_pixels && stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Colour));
				stbir_resize_uint8_linear(_pixels, _width, _height, 0, tmp, width, height, 0, (stbir_pixel_layout)_channels);
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 4;
			} else if (_pixels && !stretch) {
				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Colour));
				memset(tmp, 0, width * height * sizeof(Colour));
				for (int j = 0; j < height; ++j) {
					for (int i = 0; i < width; ++i) {
						Colour col(0, 0, 0, 0);
						if (get(i, j, col)) {
							Byte* unit = &tmp[(i + j * width) * _channels];
							memcpy(unit, &col, sizeof(Colour));
						} else {
							Colour* unit = (Colour*)(&tmp[(i + j * width) * _channels]);
							*unit = Colour(0, 0, 0, 255);
						}
					}
				}
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 4;
			} else {
				Byte* tmp = (Byte*)malloc(width * height * sizeof(Colour));
				memset(tmp, 0, width * height * sizeof(Colour));
				_pixels = tmp;
				_width = width;
				_height = height;
				_channels = 4;
			}
		}

		surface(nullptr);

		return true;
	}
	virtual bool pitch(int width_, int height_, int pitch_, int maxHeight_, int unitSize) override {
		const int limitTilesXCount = width_ / unitSize;
		const int limitTilesYCount = height_ / unitSize;
		const int tilesPitchCount = pitch_ / unitSize;
		const int times = Math::ceilIntegerTimesOf(limitTilesXCount * limitTilesYCount, tilesPitchCount);
		const int newWidth = tilesPitchCount * unitSize;
		const int newHeight = Math::min(times * unitSize, maxHeight_);
		const int oldTilesXCount = width() / unitSize;
		const int oldTilesYCount = height() / unitSize;

		if (newWidth <= 0 || newHeight <= 0)
			return false;

		if (newWidth > GBBASIC_TEXTURE_SAFE_MAX_WIDTH || newHeight > GBBASIC_TEXTURE_SAFE_MAX_HEIGHT)
			return false;

		if (_palettedBits) {
			if (_pixels) {
				Image* img = nullptr;
				if (!clone(&img))
					return false;

				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(newWidth * newHeight * sizeof(Byte));
				memset(tmp, 0, newWidth * newHeight * sizeof(Byte));
				surface(nullptr);
				_blank = blank;
				if (_pixels) {
					free(_pixels);
					_pixels = nullptr;
				}
				_pixels = tmp;
				_width = newWidth;
				_height = newHeight;
				_channels = 1;
				int k = 0;
				for (int j = 0; j < oldTilesYCount; ++j) {
					for (int i = 0; i < oldTilesXCount; ++i) {
						const int x = k % tilesPitchCount;
						const int y = k / tilesPitchCount;
						img->blit(
							this,
							x * unitSize, y * unitSize, unitSize, unitSize,
							i * unitSize, j * unitSize
						);
						++k;
					}
				}

				Image::destroy(img);
			} else {
				Byte* tmp = (Byte*)malloc(newWidth * newHeight * sizeof(Byte));
				memset(tmp, 0, newWidth * newHeight * sizeof(Byte));
				_pixels = tmp;
				_width = newWidth;
				_height = newHeight;
				_channels = 1;
			}
		} else {
			if (_pixels) {
				Image* img = nullptr;
				if (!clone(&img))
					return false;

				const bool blank = _blank;
				Byte* tmp = (Byte*)malloc(newWidth * newHeight * sizeof(Colour));
				memset(tmp, 0, newWidth * newHeight * sizeof(Colour));
				for (int i = 0; i < newWidth * newHeight; i += sizeof(Colour)) {
					*(Colour*)(tmp + i) = Colour(0, 0, 0, 255);
				}
				clear();
				_blank = blank;
				_pixels = tmp;
				_width = newWidth;
				_height = newHeight;
				_channels = 4;
				int k = 0;
				for (int j = 0; j < oldTilesYCount; ++j) {
					for (int i = 0; i < oldTilesXCount; ++i) {
						const int x = k % tilesPitchCount;
						const int y = k / tilesPitchCount;
						img->blit(
							this,
							x * unitSize, y * unitSize, unitSize, unitSize,
							i * unitSize, j * unitSize
						);
						++k;
					}
				}

				Image::destroy(img);
			} else {
				Byte* tmp = (Byte*)malloc(newWidth * newHeight * sizeof(Colour));
				memset(tmp, 0, newWidth * newHeight * sizeof(Colour));
				_pixels = tmp;
				_width = newWidth;
				_height = newHeight;
				_channels = 4;
			}
		}

		surface(nullptr);

		return true;
	}

	virtual void flip(bool h, bool v) override {
		auto swap = [] (Image* self, int x0, int y0, int x1, int y1, bool paletted) -> void {
			if (paletted) {
				int idx0 = 0;
				int idx1 = 0;
				self->get(x0, y0, idx0);
				self->get(x1, y1, idx1);
				self->set(x0, y0, idx1);
				self->set(x1, y1, idx0);
			} else {
				Colour col0;
				Colour col1;
				self->get(x0, y0, col0);
				self->get(x1, y1, col1);
				self->set(x0, y0, col1);
				self->set(x1, y1, col0);
			}
		};

		if (h) {
			for (int y = 0; y < _height; ++y) {
				const int y0 = y;
				const int y1 = y;
				for (int x = 0; x < _width / 2; ++x) {
					const int x0 = x;
					const int x1 = (_width - 1) - x;
					swap(this, x0, y0, x1, y1, !!_palettedBits);
				}
			}
		}
		if (v) {
			for (int y = 0; y < _height / 2; ++y) {
				const int y0 = y;
				const int y1 = (_height - 1) - y;
				for (int x = 0; x < _width; ++x) {
					const int x0 = x;
					const int x1 = x;
					swap(this, x0, y0, x1, y1, !!_palettedBits);
				}
			}
		}
	}

	virtual void cleanup(void) override {
		surface(nullptr);
	}

	virtual bool get(int x, int y, Colour &col) const override {
		if (_palettedBits) {
			int idx = 0;
			if (!get(x, y, idx))
				return false;

			if (!_palette)
				return false;

			if (!_palette->get(idx, col))
				return false;

			return true;
		}

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		memcpy(&col, unit, sizeof(Colour));

		return true;
	}
	virtual bool set(int x, int y, const Colour &col) override {
		if (_palettedBits)
			return false;

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		memcpy(unit, &col, sizeof(Colour));

		if (_surface) {
			IMAGE_LOCK_SURFACE(_surface)
			Colour* pixels = (Colour*)_surface->pixels;
			pixels[x + y * _width] = col;
		}

		if (col.a > 0)
			_blank = false;

		return true;
	}
	virtual bool get(int x, int y, int &index) const override {
		if (!_palettedBits)
			return false;

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		index = *unit;

		return true;
	}
	virtual bool set(int x, int y, int index) override {
		if (!_palettedBits)
			return false;

		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return false;

		if (index < 0 || index >= std::pow(2, _palettedBits))
			return false;

		Byte* unit = &_pixels[(x + y * _width) * _channels];
		*unit = (Byte)index;

		if (_surface) {
			IMAGE_LOCK_SURFACE(_surface)
			Byte* pixels = (Byte*)_surface->pixels;
			pixels[x + y * _width] = (Byte)index;
		}

		if (index != _transparentIndex)
			_blank = false;

		return true;
	}

	virtual Image* quantized2Bpp(void) const override {
		Indexed::Ptr palette = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
		GBBASIC_ASSERT(palette->count() == 4 && "Impossible.");

		Image* result = Image::create(palette);

		GBBASIC_ASSERT(!paletted() && "Not supported.");
		Indexed::Lookup lookup;
		if (!palette->match(this, lookup)) {
			Image::destroy(result);

			return nullptr;
		}

		result->fromBlank(width(), height(), GBBASIC_PALETTE_DEPTH);
		for (int j = 0; j < result->height(); ++j) {
			for (int i = 0; i < result->width(); ++i) {
				Colour col;
				get(i, j, col);
				const int idx = lookup[col];
				result->set(i, j, idx);
			}
		}

		return result;
	}

	virtual void weight(int r, int g, int b, int a) override {
		_quantizationRedWeight = r;
		_quantizationGreenWeight = g;
		_quantizationBlueWeight = b;
		_quantizationAlphaWeight = a;
	}
	virtual bool quantize(const Colour* colors, int colorCount, bool p2p) override {
		if (p2p)
			return quantizeNearest(colors, colorCount);

		return quantizeLinear(colors, colorCount);
	}
	virtual bool realize(void) override {
		return realize(
			[this] (int /* x */, int /* y */, Byte idx) -> Colour {
				Colour col;
				_palette->get(idx, col);

				return col;
			}
		);
	}
	virtual bool realize(PaletteResolver resolve) override {
		if (!_palettedBits)
			return true;
		if (!_palette)
			return false;

		const int size = _width * _height;
		Colour* trueColPixels = (Colour*)malloc(size * sizeof(Colour));

		for (int k = 0; k < size; ++k) {
			const std::div_t div = std::div(k, _width);
			const int x = div.rem;
			const int y = div.quot;
			const Byte idx = _pixels[k];
			const Colour col = resolve(x, y, idx);
			trueColPixels[k] = col;
		}

		free(_pixels);
		_pixels = (Byte*)trueColPixels;

		_palette = nullptr;
		_palettedBits = 0;
		_channels = 4;

		surface(nullptr);

		return true;
	}

	virtual Colour findLightest(void) const override {
		Colour result;

		if (paletted())
			return result;

		int lightest = std::numeric_limits<int>::max();
		for (int j = 0; j < _height; ++j) {
			for (int i = 0; i < _width; ++i) {
				Colour col;
				get(i, j, col);
				if (col.a == 0) {
					result = Colour(255, 255, 255, 0);

					return result;
				}

				const int gray = col.toGray();
				if (gray < lightest) {
					lightest = gray;
					result = col;
				}
			}
		}

		return result;
	}

	virtual bool canSliceV(int n, int idx) const override {
		if (_width % n)
			return false;

		const int s = _width / n;
		if (s < 1)
			return false;

		if (!paletted())
			return false;

		for (int m = s; m < _width; m += s) {
			for (int p = 0; p < _height; ++p) {
				int idx_ = 0;
				get(m, p, idx_);
				const bool isTransparent = idx_ == idx;

				if (!isTransparent)
					return false;
			}
		}

		return true;
	}
	virtual bool canSliceH(int n, int idx) const override {
		if (_height % n)
			return false;

		const int s = _height / n;
		if (s < 1)
			return false;

		if (!paletted())
			return false;

		for (int m = s; m < _height; m += s) {
			for (int p = 0; p < _width; ++p) {
				int idx_ = 0;
				get(m, p, idx_);
				const bool isTransparent = idx_ == idx;

				if (!isTransparent)
					return false;
			}
		}

		return true;
	}
	virtual bool canSliceV(int n, const Colour &col) const override {
		if (_width % n)
			return false;

		const int s = _width / n;
		if (s < 1)
			return false;

		if (paletted())
			return false;

		for (int m = s; m < _width; m += s) {
			for (int p = 0; p < _height; ++p) {
				Colour col_;
				get(m, p, col_);
				const bool isTransparent = !!col.a ? col_ == col : col_.a == 0;

				if (!isTransparent)
					return false;
			}
		}

		return true;
	}
	virtual bool canSliceH(int n, const Colour &col) const override {
		if (_height % n)
			return false;

		const int s = _height / n;
		if (s < 1)
			return false;

		if (paletted())
			return false;

		for (int m = s; m < _height; m += s) {
			for (int p = 0; p < _width; ++p) {
				Colour col_;
				get(m, p, col_);
				const bool isTransparent = !!col.a ? col_ == col : col_.a == 0;

				if (!isTransparent)
					return false;
			}
		}

		return true;
	}

	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy, bool hFlip, bool vFlip, Byte alpha) const override {
		if (!dst)
			return false;

		if (dst == this)
			return false;

		auto plot = [alpha] (const Image* src, Image* dst, int sx, int sy, int dx, int dy, bool paletted) -> void {
			if (paletted) {
				int idx = 0;
				if (src->get(sx, sy, idx))
					dst->set(dx, dy, idx);
			} else {
				Colour col;
				if (src->get(sx, sy, col)) {
					Byte alpha_ = col.a;
					if (alpha < 255) {
						alpha_ = (Byte)(((float)alpha_ / 255) * ((float)alpha / 255) * 255);
					}
					if (alpha_ < 255) {
						Colour col_;
						dst->get(dx, dy, col_);
						const float f = Math::clamp((float)alpha_ / 255, 0.0f, 1.0f);
						col = col * f + col_ * (1.0f - f);
					}

					dst->set(dx, dy, col);
				} else {
					dst->set(dx, dy, Colour(255, 255, 255, 0));
				}
			}
		};

		if (w == 0)
			w = dst->width();
		if (h == 0)
			h = dst->height();
		for (int y_ = 0; y_ < h; ++y_) {
			const int sy_ = sy + y_;
			const int dy_ = vFlip ?
				y + (h - y_ - 1) :
				y + y_;
			for (int x_ = 0; x_ < w; ++x_) {
				const int sx_ = sx + x_;
				const int dx_ = hFlip ?
					x + (w - x_ - 1) :
					x + x_;
				plot(this, dst, sx_, sy_, dx_, dy_, !!_palettedBits);
			}
		}

		return true;
	}
	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy, bool hFlip, bool vFlip) const override {
		return blit(dst, x, y, w, h, sx, sy, hFlip, vFlip, 255);
	}
	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy) const override {
		return blit(dst, x, y, w, h, sx, sy, false, false);
	}

	virtual bool serializeTile(int tw, int th, int tx, int ty, TileLineSerializer serializeLine) const override {
		if (!paletted())
			return false;

		if (tw == 0 || th == 0)
			return false;

		if (!serializeLine)
			return true;

		for (int y = 0; y < th; ++y) {
			UInt8 ln0 = 0;
			UInt8 ln1 = 0;
			const int lineno = ty * th + y;
			for (int x = 0; x < tw; ++x) {
				int idx = 0;
				const int colno = tx * tw + x;
				get(colno, lineno, idx);
				const bool px0 = !!(idx % GBBASIC_PALETTE_DEPTH);
				const bool px1 = !!(idx / GBBASIC_PALETTE_DEPTH);
				ln0 <<= 1;
				ln0 |= px0 ? 0x01 : 0x00;
				ln1 <<= 1;
				ln1 |= px1 ? 0x01 : 0x00;
			}

			serializeLine(y, lineno, tx, ty, ln0, ln1);
		}

		return true;
	}
	virtual bool serializeTiles(int tw, int th, TileBeginingSerializer serializeBegining, TileEndingSerializer serializeEnding, TileLineSerializer serializeLine) const override {
		if (!paletted())
			return false;

		if (tw == 0 || th == 0)
			return false;

		if (!serializeBegining && !serializeEnding && !serializeLine)
			return true;

		const int w = width() / tw;
		const int h = height() / th;
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				if (serializeBegining)
					serializeBegining(i, j);

				serializeTile(tw, th, i, j, serializeLine);

				if (serializeEnding)
					serializeEnding(i, j);
			}
		}

		return true;
	}
	virtual bool parseTile(int tw, int th, int tx, int ty, TileParserStepGetter getParserStep, TileParserStepSetter setParserStep, TileCountParser parseCount, TileDataParser parseData) override {
		if (!paletted())
			return false;

		if (tw == 0 || th == 0)
			return false;

		if (!getParserStep || !setParserStep || !parseCount || !parseData)
			return true;

		const int n = parseCount();
		if (getParserStep() >= n || getParserStep() + 1 >= n)
			return false;

		for (int y = 0; y < th; ++y) {
			if (getParserStep() >= n || getParserStep() + 1 >= n)
				break;

			Int64 ln0 = parseData(getParserStep()); setParserStep(getParserStep() + 1);
			Int64 ln1 = parseData(getParserStep()); setParserStep(getParserStep() + 1);
			for (int x = tw - 1; x >= 0; --x) {
				int idx = (int)((ln0 & 0x01) | ((ln1 & 0x01) << 1));
				idx = Math::clamp(idx, 0, (int)Math::pow(2, paletted()) - 1);
				set(tx * tw + x, ty * th + y, idx);
				ln0 >>= 1;
				ln1 >>= 1;
			}
		}

		return true;
	}
	virtual bool parseTiles(int tw, int th, TileCountParser parseCount, TileDataParser parseData) override {
		if (!paletted())
			return false;

		if (tw == 0 || th == 0)
			return false;

		if (!parseCount || !parseData)
			return true;

		const int n = parseCount();

		int k = 0;
		const int w = width() / tw;
		const int h = height() / th;
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				const bool ret = parseTile(
					tw, th,
					i, j,
					[&] (void) -> int {
						return k;
					},
					[&] (int k_) -> void {
						k = k_;
					},
					parseCount, parseData
				);
				if (!ret)
					break;
			}

			if (k >= n)
				break;
		}

		return true;
	}

	virtual bool serializeSgbBorder(class Bytes* palette_, class Bytes* tiles_, class Bytes* map_, ErrorPoints* errorPoints) const override {
		// See: https://github.com/gbdk-2020/gbdk-2020/tree/develop/gbdk-support/png2asset.

		// Macros and constants.
		#define ABGR8_R                    3
		#define ABGR8_G                    2
		#define ABGR8_B                    1
		#define ABGR8_ALPHA                0
		#define RGBA32(R, G, B, A)         (((R) << 24) | ((G) << 16) | ((B) << 8) | (A))
		#define RGB24(R, G, B)             (((R) << 16) | ((G) << 8) | (B))
		#define RGBA32_TRANSPARENT_WHITE   (RGBA32(255, 255, 255, 0))
		#define RGB8(R, G, B)              (((UInt16)((((B) >> 3) & 0x1f) << 10)) | ((UInt16)((((G) >> 3) & 0x1f) << 5)) | (((R) >> 3) & 0x1f))
		#define BIT(VALUE, INDEX)          (1 & ((VALUE) >> (INDEX)))

		// Type declarations.
		struct IntColorComparer {
			bool operator() (UInt32 c0, UInt32 c1) const {
				const UInt8* c0Ptr = (UInt8*)&c0;
				const UInt8* c1Ptr = (UInt8*)&c1;

				// Compare alpha first, transparent color is considered smaller.
				if (c0Ptr[ABGR8_ALPHA] != c1Ptr[ABGR8_ALPHA]) {
					return c0Ptr[ABGR8_ALPHA] < c1Ptr[ABGR8_ALPHA];
				} else {
					// If a color is fully transparent then consider it identical to any other fully
					// transparent color and do not insert.
					// If this test is reached then alpha channels are identical for the two entries
					// and entry c0 is the one being tested for insertion.
					if (c0Ptr[ABGR8_ALPHA] == 0) return false;

					// Do a compare with luminance in upper bits, and original rgb24 in lower bits.
					// This prefers luminance, but considers RGB values for equal-luminance cases to
					// make sure the compare functor satisifed the strictly weak ordering requirement
					const UInt64 lum0 = (UInt32)(c0Ptr[ABGR8_R] * 299 + c0Ptr[ABGR8_G] * 587 + c0Ptr[ABGR8_B] * 114);
					const UInt64 lum1 = (UInt32)(c1Ptr[ABGR8_R] * 299 + c1Ptr[ABGR8_G] * 587 + c1Ptr[ABGR8_B] * 114);
					const UInt64 rgb0 = RGB24(c0Ptr[ABGR8_R], c0Ptr[ABGR8_G], c0Ptr[ABGR8_B]);
					const UInt64 rgb1 = RGB24(c1Ptr[ABGR8_R], c1Ptr[ABGR8_G], c1Ptr[ABGR8_B]);
					const UInt64 all0 = (lum0 << 24) | rgb0;
					const UInt64 all1 = (lum1 << 24) | rgb1;

					return all0 > all1;
				}
			}
		};
		// This set will keep colors in the palette ordered based on their grayscale values to ensure they look good on DMG.
		// This assumes the palette used in DMG will be 00 01 10 11.
		typedef std::set<UInt32, IntColorComparer> PaletteSet;
		typedef std::vector<PaletteSet> Palettes;

		struct Tile {
			typedef std::vector<Tile> Array;

			typedef std::vector<UInt8> UInt8Buffer;

			Bytes::Ptr data = Bytes::Ptr(Bytes::create());
			UInt8 palette = 0;

			Tile() {
			}
			Tile(int n) {
				data->resize(n);
			}

			bool operator == (const Tile &other) const {
				if (palette != other.palette)
					return false;

				if (data->count() != other.data->count())
					return false;

				for (int i = 0; i < (int)data->count(); ++i) {
					const Byte b0 = data->get(i);
					const Byte b1 = other.data->get(i);
					if (b0 != b1)
						return false;
				}

				return true;
			}

			Tile hFlip(void) const {
				Tile ret;
				for (int j = (int)data->count() - 8; j >= 0; j -= 8) {
					for (int i = 0; i < 8; ++i) {
						ret.data->writeByte(data->get(j + i));
					}
				}
				ret.palette = palette;

				return ret;
			}
			Tile vFlip(void) const {
				Tile ret;
				for (int j = 0; j < (int)data->count(); j += 8) {
					for (int i = 7; i >= 0; --i) {
						ret.data->writeByte(data->get(j + i));
					}
				}
				ret.palette = palette;

				return ret;
			}

			UInt8Buffer getPackedData(int tile_w, int tile_h, int bpp) const {
				UInt8Buffer ret((tile_w / 8) * tile_h * bpp, 0);
				for (int j = 0; j < tile_h; ++j) {
					for (int i = 0; i < 8; ++i) {
						const UInt8 col = data->get(8 * j + i);
						ret[j * 2] |= BIT(col, 0) << (7 - i);
						ret[j * 2 + 1] |= BIT(col, 1) << (7 - i);
						ret[(tile_h + j) * 2] |= BIT(col, 2) << (7 - i);
						ret[(tile_h + j) * 2 + 1] |= BIT(col, 3) << (7 - i);
					}
				}

				return ret;
			}
		};
		typedef std::vector<UInt8> MapAndAttributes;

		// Prepare.
		if (paletted())
			return false;

		if (!palette_ && !tiles_ && !map_ && !errorPoints)
			return true;

		if (palette_)
			palette_->clear();
		if (tiles_)
			tiles_->clear();
		if (map_)
			map_->clear();
		if (errorPoints)
			errorPoints->clear();

		// Functions.
		auto translateColor = [] (Colour &col) -> void {
			if (col == Colour(255, 0, 255)) // Translate magenta to transparent.
				col = Colour(255, 255, 255, 0);
		};
		auto buildPalettes = [errorPoints, translateColor] (const Image* img, int tw, int th, Palettes &palettes, int colorsPerPal) -> int* {
			auto getPaletteColors = [translateColor] (const Image* img, int x, int y, int w, int h) -> PaletteSet {
				PaletteSet ret;

				// For SGB mode, every 16 color palette must have a transparent color as the first entry.
				//
				// For most SGB borders the Game Boy screen region is transparent and those
				// tiles will all be uniform and therefore using only the first palette where
				// they already have a transparent entry. Which means any additional SGB palettes
				// will lack the needed transparent entry at their start.
				//
				// The solution is to automatically insert a transparent color to every palette
				// created for an SGB tile. If another transparent entry is present then they
				// will get automatically merged (regardless of RGB value *fully* transparent
				// colors are now considered identical. see the CmpIntColor functor for details).
				//
				// The use of RGB(255, 255, 255) (0xffffff) for the color is due to colors being sorted
				// light to dark, and historical use of transparent white in existing examples.
				ret.insert((UInt32)RGBA32_TRANSPARENT_WHITE);

				// Now scan the tile for colors.
				for (int j = y; j < y + h; ++j) {
					for (int i = x; i < x + w; ++i) {
						Colour col;
						img->get(i, j, col);
						translateColor(col);
						const UInt32 colVal = RGBA32(col.r, col.g, col.b, col.a);
						ret.insert(colVal);
					}
				}

				for (PaletteSet::iterator it = ret.begin(); it != ret.end(); ++it) {
					if (it != ret.begin() && ((0xff & *it) != 0xff)) // `ret.begin()` should be the only one transparent.
						fprintf(stderr, "Warning: found more than one transparent color in tile at x:%d, y:%d, w:%d, h:%d.\n", x, y, w, h);
				}

				return ret;
			};
			auto findOrCreateSubPalette = [] (const PaletteSet &pal, Palettes &palettes, int colorsPerPal) -> int {
				// Return -1 if colors can't even fit in sub-palette hardware limit.
				if ((int)pal.size() > colorsPerPal)
					return -1;

				// Check if it matches any palettes or create a new one.
				int i;
				for (i = 0; i < (int)palettes.size(); ++i) {
					// Try to merge this palette with any of the palettes (checking if they are
					// equal is not enough since the palettes can have less than 4 colors).
					PaletteSet merged(palettes[i]);
					merged.insert(pal.begin(), pal.end());
					if ((int)merged.size() <= colorsPerPal) {
						if ((int)palettes[i].size() <= colorsPerPal)
							palettes[i] = merged; // Increase colors with this palette (it has less than 4 colors).

						return i; // Found palette.
					}
				}

				if (i == (int)palettes.size()) {
					// Palette not found, add a new one.
					palettes.push_back(pal);
				}

				return i;
			};

			int* palettesPerTile = new int[(img->width() / tw) * (img->height() / th)];
			const int sx = 1;
			const int sy = 1;
			for (int y = 0; y < img->height(); y += th * sy) {
				for (int x = 0; x < img->width(); x += tw * sx) {
					// Get palette colors on [x, y, tw, th].
					const PaletteSet pal = getPaletteColors(img, (x / sx) * sx, (y / sy) * sy, sx * tw, sy * th);

					int subPalIndex = findOrCreateSubPalette(pal, palettes, colorsPerPal);
					if (subPalIndex < 0) {
						fprintf(stderr, "Error: more than %d colors found in tile at x:%d, y:%d, w:%d, h:%d.\n", colorsPerPal, (x / sx) * sx, (y / sy) * sy, sx * tw, sy * th);

						if (errorPoints)
							errorPoints->push_back(Math::Recti::byXYWH((x / sx) * sx, (y / sy) * sy, sx * tw, sy * th));

						subPalIndex = 0; // Force to sub-palette 0, to allow getting a partially-incorrect output image.
					}

					const int dx = ((x / tw) / sx) * sx;
					const int dy = ((y / th) / sy) * sy;
					const int w = (img->width() / tw);
					for (int yy = 0; yy < sy; ++yy) {
						for (int xx = 0; xx < sx; ++xx) {
							palettesPerTile[(dy + yy) * w + dx + xx] = subPalIndex;
						}
					}
				}
			}

			return palettesPerTile;
		};

		auto sliceTile = [] (const Image* img, int x, int y, int tw, int th, Tile &tile, int colorsPerPal) -> bool {
			// Set the palette to 0 when palettes are not stored in tiles to allow tiles to be equal even when their palettes are different.
			tile.palette = 0;

			bool allZero = true;
			for (int j = 0; j < th; ++j) {
				for (int i = 0; i < tw; ++i) {
					int idx = 0;
					img->get(x + i, y + j, idx);
					idx %= colorsPerPal;
					tile.data->set((j * tw) + i, (UInt8)idx);
					allZero = allZero && (idx == 0);
				}
			}

			return !allZero;
		};
		auto findTile = [] (const Tile::Array &tileset, const Tile &t, int &idx, UInt8 &props, UInt8 defaultProps) -> bool {
			Tile::Array::const_iterator it = std::find(tileset.begin(), tileset.end(), t);
			if (it != tileset.end()) {
				idx = (int)(it - tileset.begin());
				props = defaultProps;

				return true;
			}

			Tile tile = t.vFlip();
			it = std::find(tileset.begin(), tileset.end(), tile);
			if (it != tileset.end()) {
				idx = (int)(it - tileset.begin());
				props = defaultProps | (1 << 5);

				return true;
			}

			tile = tile.hFlip();
			it = std::find(tileset.begin(), tileset.end(), tile);
			if (it != tileset.end()) {
				idx = (int)(it - tileset.begin());
				props = defaultProps | (1 << 5) | (1 << 6);

				return true;
			}

			tile = tile.vFlip();
			it = std::find(tileset.begin(), tileset.end(), tile);
			if (it != tileset.end()) {
				idx = (int)(it - tileset.begin());
				props = defaultProps | (1 << 6);

				return true;
			}

			return false;
		};

		auto serializePaletteBytes = [] (const Indexed::Ptr &palette, Bytes* bytes, int totalColorCount, int colorsPerPal) -> void {
			const int paletteStart = 0;
			const int totalPaletteCount = totalColorCount / colorsPerPal;
			for (int i = paletteStart; i < totalPaletteCount; ++i) {
				const Colour* palPtr = palette->pointer(nullptr);
				for (int c = 0; c < colorsPerPal; ++c, ++palPtr) {
					const UInt16 rgb8 = RGB8(palPtr->r, palPtr->g, palPtr->b);
					if (bytes)
						bytes->writeUInt16(rgb8);
				}
			}
		};
		auto serializeTileBytes = [] (const Tile::Array &tiles, Bytes* bytes, int tw, int th, int bpp) -> void {
			const int tilesStart = 0;
			for (Tile::Array::const_iterator it = tiles.begin() + tilesStart; it != tiles.end(); ++it) {
				const Tile::UInt8Buffer packedData = (*it).getPackedData(tw, th, bpp);
				for (Tile::UInt8Buffer::const_iterator it2 = packedData.begin(); it2 != packedData.end(); ++it2) {
					if (bytes)
						bytes->writeUInt8(*it2);
				}
			}
		};
		auto serializeMapData = [] (const MapAndAttributes &map, Bytes* bytes, Image* img) -> void {
			const int lineSize = (int)map.size() / (img->height() / 8);
			for (int j = 0; j < img->height() / 8; ++j) {
				for (int i = 0; i < lineSize; ++i) {
					if (bytes)
						bytes->writeUInt8(map[j * lineSize + i]);
				}
			}
		};

		// Variables and constants.
		Palettes palettes;
		Tile::Array tiles;
		MapAndAttributes map;

		const int tw = GBBASIC_TILE_SIZE;
		const int th = GBBASIC_TILE_SIZE;
		const int bpp = 4;
		const int colorsPerPal = 1 << bpp;
		const int maxPalettes = 4;
		const UInt8 defaultProps = 0;
		const UInt8 baseTile = 0;

		// Generate palettes data, create an indexed image from it.
		const int* palettesPerTile = buildPalettes(this, tw, th, palettes, colorsPerPal);
		int paletteCount = 0;
		if (palettes.size() > maxPalettes) {
			paletteCount = maxPalettes;
		} else {
			paletteCount = (int)palettes.size();
		}

		const int totalColorCount = paletteCount * colorsPerPal;
		Indexed::Ptr palette(Indexed::create(totalColorCount));
		for (int i = 0; i < 255; ++i) {
			const Colour col(0, 0, 0, 0);
			palette->set(i, &col);
		}
		Image::Ptr img(Image::create(Indexed::Ptr(palette)));
		img->fromBlank(width(), height(), 8);

		for (int p = 0; p < paletteCount; ++p) {
			Colour* colPtr = palette->pointer(nullptr);
			colPtr += p * colorsPerPal;
			// When palette size does not equal to `colorsPerPal` the unused colors are left as black.
			for (PaletteSet::iterator it = palettes[p].begin(); it != palettes[p].end(); ++it, ++colPtr) {
				const UInt8* c = (UInt8*)&(*it);
				*colPtr = Colour::byRGBA8888(c[ABGR8_R], c[ABGR8_G], c[ABGR8_B], c[ABGR8_ALPHA]);
			}
		}
		for (int y = 0; y < height(); ++y) {
			for (int x = 0; x < width(); ++x) {
				Colour col;
				get(x, y, col);
				translateColor(col);
				const UInt32 color32 = RGBA32(col.r, col.g, col.b, col.a);
				const UInt8 palette = (UInt8)palettesPerTile[(y / th) * (width() / tw) + (x / tw)];
				const UInt8 index = (UInt8)std::distance(palettes[palette].begin(), palettes[palette].find(color32));
				const int idx = (palette << bpp) + index;
				img->set(x, y, idx);
			}
		}
		delete [] palettesPerTile;

		// Generate map data.
		const int w = width() / tw;
		const int h = height() / th;
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				const int x = i * tw;
				const int y = j * th;

				Tile tile(tw * th);
				sliceTile(img.get(), x, y, tw, th, tile, colorsPerPal);

				// If the tile pattern has not been encountered before then save it.
				int idx = 0;
				UInt8 props = 0;
				if (!findTile(tiles, tile, idx, props, defaultProps)) {
					tiles.push_back(tile);
					idx = (int)tiles.size() - 1;
					props = defaultProps;
				}

				// Creating map tile index and attributes entries is only when processing the the main image.
				map.push_back((UInt8)idx + baseTile);

				int colIdx = 0;
				get(x, y, colIdx);
				const UInt8 palIdx = (UInt8)colIdx >> bpp; // We can pick the palette from the first pixel of this tile.
				props = props << 1;                        // Mirror flags in SGB are on bit 7.
				props |= (palIdx + 4) << 2;                // Palettes are in bits 2, 3, 4 and need to go from 4 to 7.
				map.push_back(props);                      // Also they are stored within the map tiles.
			}
		}

		// Serialize to bytes.
		serializePaletteBytes(palette, palette_, totalColorCount, colorsPerPal);
		serializeTileBytes(tiles, tiles_, tw, th, bpp);
		serializeMapData(map, map_, img.get());

		// Finish.
		return true;

		#undef ABGR8_R
		#undef ABGR8_G
		#undef ABGR8_B
		#undef ABGR8_ALPHA
		#undef RGBA32
		#undef RGB24
		#undef RGBA32_TRANSPARENT_WHITE
		#undef RGB8
		#undef BIT
	}

	virtual bool fromBlank(int width, int height, int paletted) override {
		clear();

		if (width <= 0 || height <= 0)
			return false;

		if (width > GBBASIC_TEXTURE_SAFE_MAX_WIDTH || height > GBBASIC_TEXTURE_SAFE_MAX_HEIGHT)
			return false;

		GBBASIC_ASSERT((paletted == IMAGE_PALETTE_BITS || paletted == 8 || paletted == 2 || paletted == 0) && "Wrong data.");
		_palettedBits = paletted;
		_width = width;
		_height = height;
		if (_palettedBits) {
			_channels = 1;
			_pixels = (Byte*)malloc(_width * _height * sizeof(Byte));
			memset(_pixels, 0, _width * _height * sizeof(Byte));
		} else {
			_palette = nullptr;
			_channels = 4;
			_pixels = (Byte*)malloc(_width * _height * sizeof(Colour));
			memset(_pixels, 0, _width * _height * sizeof(Colour));
		}

		_blank = true;

		return true;
	}

	virtual bool fromImage(const Image* src) override {
		if (!src)
			return false;

		if (src == this)
			return false;

		if (!fromBlank(src->width(), src->height(), src->paletted()))
			return false;

		auto plot = [] (const Image* src, Image* dst, int x, int y, bool paletted) -> void {
			if (paletted) {
				int idx = 0;
				if (src->get(x, y, idx))
					dst->set(x, y, idx);
			} else {
				Colour col;
				if (src->get(x, y, col))
					dst->set(x, y, col);
			}
		};

		for (int y = 0; y < _height && y < src->height(); ++y) {
			for (int x = 0; x < _width && x < src->width(); ++x)
				plot(src, this, x, y, !!_palettedBits);
		}

		_blank = src->blank();

		return true;
	}

	virtual bool toRaw(class Bytes* val) const override {
		val->clear();

		if (!_pixels)
			return false;

		if (_palettedBits) {
			const size_t totalSize = _width * _height;
			val->resize(totalSize);
			Byte* ptr = val->pointer();
			memcpy(ptr, _pixels, _width * _height);
			val->poke(val->count());

			return true;
		} else {
			Byte* ptr = val->pointer();
			memcpy(ptr, _pixels, _width * _height * sizeof(Colour));
			val->poke(val->count());

			return true;
		}
	}
	virtual bool fromRaw(const Byte* val, size_t size) override {
		clear();

		if (!val)
			return false;

		if (_width == 0 || _height == 0)
			return false;

		bool blank = true;
		if (_palettedBits) {
			if (_width * _height > (int)size)
				return false;

			_pixels = (Byte*)malloc(_width * _height);
			for (int i = 0; i < _width * _height; ++i) {
				_pixels[i] = ((Byte*)val)[i];
				if (val[i] != _transparentIndex)
					blank = false;
			}
		}
		if (_pixels) {
			_channels = 1;

			_blank = blank;

			return !!_pixels;
		}

		blank = true;
		do {
			if (_width * _height * sizeof(Colour) > size)
				return false;

			_pixels = (Byte*)malloc(_width * _height * sizeof(Colour));
			for (int i = 0; i < _width * _height; ++i) {
				const Colour &col = ((Colour*)val)[i];
				((Colour*)_pixels)[i] = col;
				if (col.a > 0)
					blank = false;
			}
		} while (false);
		if (_pixels) {
			_channels = 4;

			_blank = blank;

			return !!_pixels;
		}

		return !!_pixels;
	}
	virtual bool fromRaw(const class Bytes* val) override {
		return fromRaw(val->pointer(), val->count());
	}

	virtual bool toBytes(class Bytes* val, const char* type) const override {
		val->clear();

		if (!_pixels)
			return false;

		if (_palettedBits) {
			const size_t totalSize = GBBASIC_COUNTOF(IMAGE_PALETTED_HEADER_BYTES) +
				sizeof(int) + sizeof(int) +
				sizeof(int) +
				_width * _height;
			val->resize(totalSize);
			Byte* ptr = val->pointer();
			memcpy(ptr, IMAGE_PALETTED_HEADER_BYTES, GBBASIC_COUNTOF(IMAGE_PALETTED_HEADER_BYTES));
			ptr += GBBASIC_COUNTOF(IMAGE_PALETTED_HEADER_BYTES);
			memcpy(ptr, &_width, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, &_height, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, &_palettedBits, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, _pixels, _width * _height);
			val->poke(val->count());

			return true;
		} else {
			stbi_write_func* toStream = [] (void* context, void* data, int len) -> void {
				Bytes* bytes = (Bytes*)context;
				if (len == 1) {
					bytes->writeUInt8(*(UInt8*)data);
				} else {
					const size_t count = bytes->count();
					bytes->resize(count + len);
					Byte* ptr = bytes->pointer() + count;
					if (len > 0)
						memcpy(ptr, data, len);
					bytes->poke(count + len);
				}
			};

			if (!strcmp(type, "png")) {
				return !!stbi_write_png_to_func(toStream, val, _width, _height, 4, _pixels, 0);
			} else if (!strcmp(type, "jpg")) {
				return !!stbi_write_jpg_to_func(toStream, val, _width, _height, 4, _pixels, 100);
			} else if (!strcmp(type, "bmp")) {
				return !!stbi_write_bmp_to_func(toStream, val, _width, _height, 4, _pixels);
			} else if (!strcmp(type, "tga")) {
				return !!stbi_write_tga_to_func(toStream, val, _width, _height, 4, _pixels);
			} else {
				const size_t headerSize = GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES) +
					sizeof(int) + sizeof(int) +
					sizeof(int);
				val->resize(headerSize);
				Byte* ptr = val->pointer();
				memcpy(ptr, IMAGE_COLORED_HEADER_BYTES, GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES));
				ptr += GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES);
				memcpy(ptr, &_width, sizeof(int));
				ptr += sizeof(int);
				memcpy(ptr, &_height, sizeof(int));
				ptr += sizeof(int);
				memcpy(ptr, &_palettedBits, sizeof(int));
				ptr += sizeof(int);
				memcpy(ptr, _pixels, _width * _height * sizeof(Colour));
				val->poke(val->count());

				return true;
			}
		}
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		clear();

		if (!val)
			return false;

		bool blank = true;
		if (size > GBBASIC_COUNTOF(IMAGE_PALETTED_HEADER_BYTES) && memcmp(val, IMAGE_PALETTED_HEADER_BYTES, GBBASIC_COUNTOF(IMAGE_PALETTED_HEADER_BYTES)) == 0) {
			val += GBBASIC_COUNTOF(IMAGE_PALETTED_HEADER_BYTES);
			const int* iptr = (int*)val;
			const int width = *iptr++;
			const int height = *iptr++;
			const int bitCount = *iptr++;

			if (width > GBBASIC_TEXTURE_SAFE_MAX_WIDTH || height > GBBASIC_TEXTURE_SAFE_MAX_HEIGHT)
				return false;

			_pixels = (Byte*)malloc(width * height);
			const Byte* bptr = (Byte*)iptr;
			for (int i = 0; i < width * height; ++i) {
				_pixels[i] = bptr[i];
				if (bptr[i] != _transparentIndex)
					blank = false;
			}
			_width = width;
			_height = height;
			_palettedBits = bitCount;
			GBBASIC_ASSERT((_palettedBits == IMAGE_PALETTE_BITS || _palettedBits == 8 || _palettedBits == 2 || _palettedBits == 0) && "Wrong data.");
		}
		if (_pixels) {
			_channels = 1;

			_blank = blank;

			return !!_pixels;
		}

		blank = true;
		if (size > GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES) && memcmp(val, IMAGE_COLORED_HEADER_BYTES, GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES)) == 0) {
			val += GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES);
			const int* iptr = (int*)val;
			const int width = *iptr++;
			const int height = *iptr++;
			const int bitCount = *iptr++;

			if (width > GBBASIC_TEXTURE_SAFE_MAX_WIDTH || height > GBBASIC_TEXTURE_SAFE_MAX_HEIGHT)
				return false;

			_pixels = (Byte*)malloc(width * height * sizeof(Colour));
			const Colour* cptr = (Colour*)iptr;
			for (int i = 0; i < width * height; ++i) {
				const Colour &col = cptr[i];
				((Colour*)_pixels)[i] = col;
				if (col.a > 0)
					blank = false;
			}
			_width = width;
			_height = height;
			_palettedBits = bitCount;
			GBBASIC_ASSERT((_palettedBits == IMAGE_PALETTE_BITS || _palettedBits == 8 || _palettedBits == 2 || _palettedBits == 0) && "Wrong data.");
		}
		if (_pixels) {
			_channels = 4;

			_blank = blank;

			return !!_pixels;
		}

		_pixels = stbi_load_from_memory(val, (int)size, &_width, &_height, &_channels, 4);
		_channels = 4;

		_blank = true;
		for (int i = 0; i < _width * _height; ++i) {
			const Colour &col = ((Colour*)_pixels)[i];
			if (col.a > 0) {
				_blank = false;

				break;
			}
		}

		return !!_pixels;
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		rapidjson::Value jstrwidth, jstrheight;
		jstrwidth.SetString("width", doc.GetAllocator());
		jstrheight.SetString("height", doc.GetAllocator());
		rapidjson::Value jvalwidth, jvalheight;
		jvalwidth.SetInt(_width);
		jvalheight.SetInt(_height);
		val.AddMember(jstrwidth, jvalwidth, doc.GetAllocator());
		val.AddMember(jstrheight, jvalheight, doc.GetAllocator());

		rapidjson::Value jstrdetpth;
		jstrdetpth.SetString("depth", doc.GetAllocator());
		rapidjson::Value jvaldepth;
		jvaldepth.SetInt(_palettedBits);
		val.AddMember(jstrdetpth, jvaldepth, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		for (int j = 0; j < _height; ++j) {
			for (int i = 0; i < _width; ++i) {
				if (_palettedBits) {
					int idx = 0;
					get(i, j, idx);

					jvaldata.PushBack(idx, doc.GetAllocator());
				} else {
					Colour col;
					get(i, j, col);

					jvaldata.PushBack(col.toRGBA(), doc.GetAllocator());
				}
			}
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

		rapidjson::Value::ConstMemberIterator jw = val.FindMember("width");
		rapidjson::Value::ConstMemberIterator jh = val.FindMember("height");
		if (jw == val.MemberEnd() || jh == val.MemberEnd())
			return false;
		if (!jw->value.IsInt() || !jh->value.IsInt())
			return false;
		const int width = jw->value.GetInt();
		const int height = jh->value.GetInt();

		int depth = 0;
		rapidjson::Value::ConstMemberIterator jd = val.FindMember("depth");
		if (jd != val.MemberEnd() && jd->value.IsInt())
			depth = jd->value.GetInt();

		if (!fromBlank(width, height, depth))
			return false;

		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata != val.MemberEnd() && jdata->value.IsArray()) {
			rapidjson::Value::ConstArray data = jdata->value.GetArray();
			int idx = 0;
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					idx = i + j * width;
					if (idx >= (int)data.Size())
						return false;

					if (!data[idx].IsUint())
						return false;

					if (_palettedBits) {
						set(i, j, (int)data[idx].GetUint());
					} else {
						Colour col;
						col.fromRGBA(data[idx].GetUint());
						set(i, j, col);
					}
				}
			}
		} else {
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					if (_palettedBits) {
						set(i, j, 0);
					} else {
						const Colour col(0, 0, 0, 0);
						set(i, j, col);
					}
				}
			}
		}

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	SDL_Surface* surface(void) {
		if (_surface)
			return _surface;

		if (_channels == 1) {
			const int bits = Math::ceilIntegerTimesOf(_palettedBits, 8) * 8;
			_surface = SDL_CreateRGBSurfaceFrom(
				_pixels,
				_width, _height,
				bits, _width,
				0, 0, 0, 0
			);

			SDL_Palette* palette = nullptr;
			if (_palette)
				palette = (SDL_Palette*)_palette->pointer();
			if (palette)
				SDL_SetSurfacePalette(_surface, palette);
		} else if (_channels == 4) {
			_surface = SDL_CreateRGBSurfaceFrom(
				_pixels,
				_width, _height,
				32, _width * 4,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
			);
		}

		return _surface;
	}
	void surface(std::nullptr_t) {
		if (_surface) {
			SDL_FreeSurface(_surface);
			_surface = nullptr;
		}
	}

	void clear(void) {
		surface(nullptr);

		_blank = true;
		_palettedBits = 0;
		_transparentIndex = 0;
		if (_pixels) {
			free(_pixels);
			_pixels = nullptr;
		}
		_width = 0;
		_height = 0;
		_channels = 0;

		_quantizationRedWeight = 1;
		_quantizationGreenWeight = 1;
		_quantizationBlueWeight = 1;
		_quantizationAlphaWeight = 4;
	}

	bool quantizeNearest(const Colour* colors, int colorCount) {
		if (_palettedBits)
			return true;

		const int size = _width * _height;
		const Colour* const palette = colors;
		Byte* palettedPixels = (Byte*)malloc(size * sizeof(Byte));

		for (int k = 0; k < size; ++k) {
			const Colour c = ((Colour*)_pixels)[k];
			int bestd = std::numeric_limits<int>::max(), best = -1;
			for (int i = 0; i < colorCount; ++i) {
				const int red = (int)palette[i].r - (int)c.r;
				const int green = (int)palette[i].g - (int)c.g;
				const int blue = (int)palette[i].b - (int)c.b;
				const int alpha = (int)palette[i].a - (int)c.a;
				int d =
					blue * blue * _quantizationBlueWeight +
					green * green * _quantizationGreenWeight +
					red * red * _quantizationRedWeight;
				d += alpha * alpha * _quantizationAlphaWeight; // Alpha is usually more weighted.
				if (d < bestd) {
					bestd = d;
					best = i;
				}
			}
			if (best == -1)
				best = 0;
			palettedPixels[k] = (Byte)best;
		}

		free(_pixels);
		_pixels = palettedPixels;

		_palettedBits = IMAGE_PALETTE_BITS;
		_channels = 1;

		surface(nullptr);

		return true;
	}
	bool quantizeLinear(const Colour* colors, int colorCount) {
		if (_palettedBits)
			return true;

		const int size = _width * _height;
		const Byte* const palette = (Byte*)colors;
		Byte* palettedPixels = (Byte*)malloc(size * sizeof(Byte));
		constexpr const int BPP = (sizeof(Colour) / sizeof(Byte));

		Byte* ditheredPixels = new Byte[size * 4];
		if (_channels == 4) {
			memcpy(ditheredPixels, _pixels, size * 4);
		} else {
			for (int i = 0; i < size; ++i) {
				ditheredPixels[i * 4] = _pixels[i * 3];
				ditheredPixels[i * 4 + 1] = _pixels[i * 3 + 1];
				ditheredPixels[i * 4 + 2] = _pixels[i * 3 + 2];
				ditheredPixels[i * 4 + 3] = 255;
			}
		}
		for (int k = 0; k < size * 4; k += 4) {
			const int rgb[4] = { ditheredPixels[k + 0], ditheredPixels[k + 1], ditheredPixels[k + 2], ditheredPixels[k + 3] };
			int bestd = std::numeric_limits<int>::max(), best = -1;
			for (int i = 0; i < colorCount; ++i) {
				const int blue = palette[i * BPP + 0] - rgb[0];
				const int green = palette[i * BPP + 1] - rgb[1];
				const int red = palette[i * BPP + 2] - rgb[2];
				const int alpha = palette[i * BPP + 3] - rgb[3];
				int d =
					blue * blue * _quantizationBlueWeight +
					green * green * _quantizationGreenWeight +
					red * red * _quantizationRedWeight;
				d += alpha * alpha * _quantizationAlphaWeight; // Alpha is usually more weighted.
				if (d < bestd) {
					bestd = d;
					best = i;
				}
			}
			if (best == -1)
				best = 0;
			palettedPixels[k / 4] = (Byte)best;
			int diff[4] = {
				ditheredPixels[k + 0] - palette[palettedPixels[k / 4] * BPP + 0],
				ditheredPixels[k + 1] - palette[palettedPixels[k / 4] * BPP + 1],
				ditheredPixels[k + 2] - palette[palettedPixels[k / 4] * BPP + 2],
				ditheredPixels[k + 3] - palette[palettedPixels[k / 4] * BPP + 3]
			};
			if (k + 4 < size * 4) {
				ditheredPixels[k + 4 + 0] = (Byte)Math::clamp(ditheredPixels[k + 4 + 0] + (diff[0] * 7 / 16), 0, 255);
				ditheredPixels[k + 4 + 1] = (Byte)Math::clamp(ditheredPixels[k + 4 + 1] + (diff[1] * 7 / 16), 0, 255);
				ditheredPixels[k + 4 + 2] = (Byte)Math::clamp(ditheredPixels[k + 4 + 2] + (diff[2] * 7 / 16), 0, 255);
				ditheredPixels[k + 4 + 3] = (Byte)Math::clamp(ditheredPixels[k + 4 + 3] + (diff[3] * 7 / 16), 0, 255);
			}
			if (k + _width * 4 + 4 < size * 4) {
				for (int i = 0; i < 3; ++i) {
					ditheredPixels[k + _width * 4 - 4 + i] = (Byte)Math::clamp(ditheredPixels[k + _width * 4 - 4 + i] + (diff[i] * 3 / 16), 0, 255);
					ditheredPixels[k + _width * 4 + i] = (Byte)Math::clamp(ditheredPixels[k + _width * 4 + i] + (diff[i] * 5 / 16), 0, 255);
					ditheredPixels[k + _width * 4 + 4 + i] = (Byte)Math::clamp(ditheredPixels[k + _width * 4 + 4 + i] + (diff[i] * 1 / 16), 0, 255);
				}
			}
		}
		delete [] ditheredPixels;

		free(_pixels);
		_pixels = palettedPixels;

		_palettedBits = IMAGE_PALETTE_BITS;
		_channels = 1;

		surface(nullptr);

		return true;
	}
};

Image* Image::create(void) {
	ImageImpl* result = new ImageImpl(nullptr);

	return result;
}

Image* Image::create(Indexed::Ptr palette) {
	ImageImpl* result = new ImageImpl(palette);

	return result;
}

void Image::destroy(Image* ptr) {
	ImageImpl* impl = static_cast<ImageImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
