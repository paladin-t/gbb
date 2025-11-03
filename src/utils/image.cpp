/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
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
