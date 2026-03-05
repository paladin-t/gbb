/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "map.h"
#include "renderer.h"
#include <SDL.h>
#include <vector>

/*
** {===========================================================================
** Map
*/

class MapImpl : public Map {
private:
	typedef std::vector<int> Cels;

	struct Sub {
		typedef std::vector<Sub> Array;

		typedef std::shared_ptr<unsigned long> Tick;

		Math::Recti area;
		int colorKey = 0;
		Texture::Ptr texture = nullptr;
		Tick ticks = nullptr; // Timestamp of last touching.

		bool valid = true;

		Sub(const Math::Recti &area_, int colKey) : area(area_), colorKey(colKey) {
		}
		Sub(const Math::Recti &area_, int colKey, Texture::Ptr texture_, Tick ticks_) : area(area_), colorKey(colKey), texture(texture_), ticks(ticks_) {
		}
	};

	typedef std::function<void(int, const Colour &)> ColorSetter;
	typedef std::function<void(int, int, ColorSetter)> PaletteSetter;

private:
	Tiles _tiles;
	int _tileWidth = 0;
	int _tileHeight = 0;
	Cels _cels;
	int _width = 0;
	int _height = 0;

	bool _batch = false;
	mutable unsigned long _ticks = 1;
	mutable Sub::Array _subs;
	int _threshold = -1;

public:
	MapImpl(bool batch) : _batch(batch) {
	}
	virtual ~MapImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Map** ptr, bool represented) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		MapImpl* result = static_cast<MapImpl*>(Map::create(&_tiles, _batch));

		if (represented)
			result->_tiles = _tiles;
		result->_tileWidth = _tileWidth;
		result->_tileHeight = _tileHeight;
		result->_cels = _cels;
		result->_cels.shrink_to_fit();
		result->_width = _width;
		result->_height = _height;
		result->_batch = _batch;

		*ptr = result;

		return true;
	}
	virtual bool clone(Map** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Map* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		// `_tiles` doesn't count.

		// `_tileWidth` and `_tileHeight` don't count.

		for (int i = 0; i < (int)_cels.size(); ++i) {
			result = Math::hash(result, _cels[i]);
		}

		result = Math::hash(result, _width, _height);

		return result;
	}
	virtual int compare(const Map* other) const override {
		const MapImpl* implOther = static_cast<const MapImpl*>(other);

		if (this == other)
			return 0;

		if (!other)
			return 1;

		// `_tiles` doesn't count.

		// `_tileWidth` and `_tileHeight` don't count.

		if (_cels.size() < implOther->_cels.size())
			return -1;
		else if (_cels.size() > implOther->_cels.size())
			return 1;
		for (int i = 0; i < (int)_cels.size(); ++i) {
			const int cell = _cels[i];
			const int celr = implOther->_cels[i];
			if (cell < celr)
				return -1;
			else if (cell > celr)
				return 1;
		}

		if (_width < implOther->_width)
			return -1;
		else if (_width > implOther->_width)
			return 1;

		if (_height < implOther->_height)
			return -1;
		else if (_height > implOther->_height)
			return 1;

		return 0;
	}

	virtual int cleanup(void) override {
		int result = (int)_subs.size();
		_subs.clear();

		return result;
	}
	virtual int cleanup(const Math::Recti &area) override {
		int result = 0;
		for (Sub::Array::iterator it = _subs.begin(); it != _subs.end(); ) {
			const Sub &sub = *it;
			if (Math::intersects(area, sub.area, false)) {
				it = _subs.erase(it);

				++result;

				continue;
			}

			++it;
		}

		return result;
	}

	virtual const Tiles* tiles(Tiles &tiles) const override {
		tiles = Tiles();

		if (_tiles.count.x == 0 || _tiles.count.y == 0)
			return nullptr;

		tiles = _tiles;

		return &tiles;
	}
	virtual void tiles(const Tiles* tiles) override {
		if (tiles) {
			_tiles = *tiles;

			if (_tiles.texture) {
				if (_tiles.count.x <= 0)
					_tiles.count.x = _tiles.texture->width() / GBBASIC_TILE_SIZE;
				if (_tiles.count.y <= 0)
					_tiles.count.y = _tiles.texture->height() / GBBASIC_TILE_SIZE;

				if (_tiles._size.x <= 0)
					_tileWidth = _tiles.texture->width() / _tiles.count.x;
				else
					_tileWidth = _tiles._size.x;
				if (_tiles._size.y <= 0)
					_tileHeight = _tiles.texture->height() / _tiles.count.y;
				else
					_tileHeight = _tiles._size.y;
			}
		} else {
			_tiles = Tiles(nullptr, Math::Vec2i());
			_tileWidth = 0;
			_tileHeight = 0;
		}
	}

	virtual int width(void) const override {
		return _width;
	}
	virtual int height(void) const override {
		return _height;
	}

	virtual Math::Recti aabb(void) const override {
		Math::Recti result(
			std::numeric_limits<Math::Recti::ValueType>::max(),
			std::numeric_limits<Math::Recti::ValueType>::max(),
			std::numeric_limits<Math::Recti::ValueType>::min(),
			std::numeric_limits<Math::Recti::ValueType>::min()
		);

		if (_width == 0)
			result.x0 = result.x1 = 0;
		if (_height == 0)
			result.y0 = result.y1 = 0;

		for (int j = 0; j < _height; ++j) {
			for (int i = 0; i < _width; ++i) {
				const int k = get(i, j);
				if (k != INVALID()) {
					if (i < result.x0)
						result.x0 = i;
					if (i > result.x1)
						result.x1 = i;
					if (j < result.y0)
						result.y0 = j;
					if (j > result.y1)
						result.y1 = j;
				}
			}
		}

		return result;
	}
	virtual bool resize(int width, int height, int default_) override {
		if (width < 0 || width > GBBASIC_MAP_MAX_WIDTH || height < 0 || height > GBBASIC_MAP_MAX_HEIGHT)
			return false;

		if (width == 0 && height == 0) {
			_cels.clear();
			_cels.shrink_to_fit();
		} else {
			Cels tmp;
			tmp.assign((size_t)(width * height), default_);
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					const int dst = i + j * width;
					int c = default_;
					if (i < _width && j < _height) {
						const int src = i + j * _width;
						c = _cels[src];
					}
					tmp[dst] = c;
				}
			}
			_cels = tmp;
			_cels.shrink_to_fit();
		}

		_width = width;
		_height = height;

		return true;
	}
	virtual bool resize(int width, int height) override {
		constexpr const int DEFAULT = 0;

		return resize(width, height, DEFAULT);
	}
	virtual void data(int* buf, size_t len) const override {
		memset(buf, 0, len * sizeof(int));

		if (_cels.empty())
			return;

		memcpy(buf, &_cels.front(), len * sizeof(int));
	}

	virtual Range range(void) const override {
		int minCel = std::numeric_limits<int>::max();
		int maxCel = std::numeric_limits<int>::min();
		const int w = width();
		const int h = height();
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				const int cel = get(i, j);
				if (cel < minCel)
					minCel = cel;
				if (cel > maxCel)
					maxCel = cel;
			}
		}

		if (minCel == std::numeric_limits<int>::max())
			minCel = -1;
		if (maxCel == std::numeric_limits<int>::min())
			maxCel = -1;

		return std::make_pair(minCel, maxCel);
	}

	virtual int get(int x, int y) const override {
		if (x < 0 || x >= _width || y < 0 || y >= _height)
			return INVALID();

		return _cels[x + y * _width];
	}
	virtual bool set(int x, int y, int v, bool expandable) override {
		if (x < 0 || y < 0)
			return false;

		if (x >= _width || y >= _height) {
			if (expandable) {
				bool resized = false;
				if (x >= _width)
					resized = resize(x, _height);
				else if (y >= _height)
					resized = resize(_width, y);
				if (!resized)
					return false;

				for (Sub &sub : _subs)
					sub.valid = false;

				return true;
			} else {
				return false;
			}
		}

		if (_cels[x + y * _width] == v)
			return true;

		_cels[x + y * _width] = v;

		for (Sub &sub : _subs) {
			if (Math::intersects(sub.area, Math::Vec2i(x, y)))
				sub.valid = false;
		}

		return true;
	}

	virtual Texture::Ptr at(int index, Math::Recti* area) const override {
		if (area)
			*area = Math::Recti();

		if (index == INVALID())
			return nullptr;

		if (_tiles.count.x <= 0)
			return nullptr;

		const std::div_t div = std::div(index, (int)_tiles.count.x);
		const int i = div.rem;
		const int j = div.quot;
		if (area)
			*area = Math::Recti::byXYWH(i * _tileWidth, j * _tileHeight, _tileWidth, _tileHeight);

		return _tiles.texture;
	}
	virtual Texture::Ptr at(int x, int y, Math::Recti* area) const override {
		if (area)
			*area = Math::Recti();

		const int index = get(x, y);

		return at(index, area);
	}
	virtual Texture::Ptr sub(class Renderer* rnd, int x, int y, int width, int height, int ncolors, const Colour* colors, int colorKey) const override {
		// Prepare.
		if (!_tiles.texture)
			return nullptr;

		auto cmpByArea = [] (const void* left, const void* right) -> int {
			const Sub* leftptr = (const Sub*)left;
			const Sub* rightptr = (const Sub*)right;

			if (leftptr->area < rightptr->area)
				return -1;
			if (leftptr->area > rightptr->area)
				return 1;

			if (leftptr->colorKey < rightptr->colorKey)
				return -1;
			if (leftptr->colorKey > rightptr->colorKey)
				return 1;

			return 0;
		};
		auto cmpByTicks = [] (const void* left, const void* right) -> int {
			const Sub* leftptr = (const Sub*)left;
			const Sub* rightptr = (const Sub*)right;

			if (*leftptr->ticks < *rightptr->ticks)
				return -1;
			if (*leftptr->ticks > *rightptr->ticks)
				return 1;

			return 0;
		};

		const unsigned long now = _ticks++;

		// Try to get from cache.
		const Math::Recti areavect = Math::Recti::byXYWH(x, y, width, height);
		const Sub key(areavect, colorKey);
		const Sub* cached = _subs.empty() ? nullptr : (Sub*)std::bsearch(&key, &_subs.front(), _subs.size(), sizeof(Sub), cmpByArea);
		if (cached) {
			if (cached->valid) {
				*cached->ticks = now;

				return cached->texture;
			} else {
				const ptrdiff_t offset = cached - &_subs.front();
				_subs.erase(_subs.begin() + offset);
			}
		}

		// Get the sub texture.
		PaletteSetter setPalette = nullptr;
		if (ncolors && colors) {
			setPalette = [ncolors, &colors] (int /* i */, int /* j */, ColorSetter setColor) -> void {
				for (int k = 0; k < ncolors; ++k)
					setColor(k, colors[k]);
			};
		}

		Texture::Ptr result = blip(rnd, x, y, width, height, setPalette);
		if (!result)
			return nullptr;

		// Cache it.
		const Sub::Tick ticks(new unsigned long(now));
		const Sub coming(areavect, colorKey, result, ticks);

		_subs.push_back(coming); // Add.

		if (_threshold >= 0 && (int)_subs.size() > _threshold) {
			std::qsort(&_subs.front(), _subs.size(), sizeof(Sub), cmpByTicks);
			_subs.erase( // Remove the most early ones of there are too many.
				_subs.begin(),
				_subs.begin() + _subs.size() - (size_t)_threshold
			);
		}

		std::qsort(&_subs.front(), _subs.size(), sizeof(Sub), cmpByArea); // Sort.

		// Finish.
		return result;
	}
	virtual Texture::Ptr sub(class Renderer* rnd, int x, int y, int width, int height, Indexed::Ptr colors, int colorKey) const override {
		int n = 0;
		const Colour* cols = nullptr;
		if (colors)
			cols = colors->pointer(&n);

		return sub(rnd, x, y, width, height, n, cols, colorKey);
	}
	virtual int subCacheSize(void) const override {
		return _threshold;
	}
	virtual void subCacheSize(int val) override {
		_threshold = val;
	}

	virtual bool update(double /* delta */, unsigned* /* id */) override {
		// Do nothing.

		return true;
	}

	virtual void render(
		class Renderer* rnd,
		int x, int y,
		const Colour* color, bool colorChanged, bool alphaChanged,
		int scale
	) const override {
		if (!_tiles.texture)
			return;

		const bool batchable = _batch &&
			(rnd->maxTextureWidth() > 0 && rnd->maxTextureHeight() > 0) &&
			(_width * _tileWidth <= rnd->maxTextureWidth() && _height * _tileHeight <= rnd->maxTextureHeight()) &&
			!_tiles.texture->paletted();
		if (batchable) {
			Texture::Ptr batch = sub(rnd, 0, 0, _width, _height, 0, nullptr, 0);
			if (batch) {
				if (scale > 1) {
					const Math::Recti dstRect = Math::Recti::byXYWH(x * scale, y * scale, _width * _tileWidth * scale, _height * _tileHeight * scale);

					rnd->render(batch.get(), nullptr, &dstRect, nullptr, nullptr, false, false, color, colorChanged, alphaChanged);
				} else {
					const Math::Recti dstRect = Math::Recti::byXYWH(x, y, _width * _tileWidth, _height * _tileHeight);

					rnd->render(batch.get(), nullptr, &dstRect, nullptr, nullptr, false, false, color, colorChanged, alphaChanged);
				}

				return;
			}
		}

		const int beginX = Math::clamp((int)(-x / (float)_tileWidth), 0, _width - 1);
		const int endX = Math::clamp((int)((rnd->width() - x) / (float)_tileWidth), 0, _width - 1);
		const int beginY = Math::clamp((int)(-y / (float)_tileHeight), 0, _height - 1);
		const int endY = Math::clamp((int)((rnd->height() - y) / (float)_tileHeight), 0, _height - 1);

		for (int j = beginY; j <= endY; ++j) {
			for (int i = beginX; i <= endX; ++i) {
				const int cel = get(i, j);
				const std::div_t div = std::div(cel, (int)_tiles.count.x);
				const int celX = div.rem;
				const int celY = div.quot;

				const int pixelX = celX * _tileWidth;
				const int pixelY = celY * _tileHeight;
				const Math::Recti srcRect = Math::Recti::byXYWH(pixelX, pixelY, _tileWidth, _tileHeight);

				const int dstX = x + i * _tileWidth;
				const int dstY = y + j * _tileHeight;
				Math::Recti dstRect;
				if (scale > 1) {
					dstRect = Math::Recti::byXYWH(dstX * scale, dstY * scale, _tileWidth * scale, _tileHeight * scale);
				} else {
					dstRect = Math::Recti::byXYWH(dstX, dstY, _tileWidth, _tileHeight);
				}

				rnd->render(_tiles.texture.get(), &srcRect, &dstRect, nullptr, nullptr, false, false, color, colorChanged, alphaChanged);
			}
		}
	}

	virtual bool load(const int* cels, int width, int height) override {
		if (width < 0 || width > GBBASIC_MAP_MAX_WIDTH || height < 0 || height > GBBASIC_MAP_MAX_HEIGHT)
			return false;

		_cels.clear();
		_width = width;
		_height = height;
		for (int j = 0; j < _height; ++j) {
			for (int i = 0; i < _width; ++i)
				_cels.push_back(cels[i + j * width]);
		}
		_cels.shrink_to_fit();

		return true;
	}
	virtual bool load(int cel, int width, int height) override {
		if (width < 0 || width > GBBASIC_MAP_MAX_WIDTH || height < 0 || height > GBBASIC_MAP_MAX_HEIGHT)
			return false;

		_cels.clear();
		_width = width;
		_height = height;
		for (int j = 0; j < _height; ++j) {
			for (int i = 0; i < _width; ++i)
				_cels.push_back(cel);
		}
		_cels.shrink_to_fit();

		return true;
	}
	virtual void unload(void) override {
		_cels.clear();
		_width = _height = 0;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		Tiles tiles_;
		tiles(tiles_);

		rapidjson::Value jstrtiles;
		jstrtiles.SetString("tiles", doc.GetAllocator());
		rapidjson::Value jvaltiles;
		jvaltiles.SetObject();

		rapidjson::Value jstrcount;
		jstrcount.SetString("count", doc.GetAllocator());
		rapidjson::Value jvalcount;
		jvalcount.SetArray();
		jvalcount.PushBack(tiles_.count.x, doc.GetAllocator());
		jvalcount.PushBack(tiles_.count.y, doc.GetAllocator());
		jvaltiles.AddMember(jstrcount, jvalcount, doc.GetAllocator());

		if (tiles_._size.x > 0 && tiles_._size.y > 0) {
			rapidjson::Value jstrsize;
			jstrsize.SetString("size", doc.GetAllocator());
			rapidjson::Value jvalsize;
			jvalsize.SetArray();
			jvalsize.PushBack(tiles_._size.x, doc.GetAllocator());
			jvalsize.PushBack(tiles_._size.y, doc.GetAllocator());
			jvaltiles.AddMember(jstrsize, jvalsize, doc.GetAllocator());
		}
		val.AddMember(jstrtiles, jvaltiles, doc.GetAllocator());

		rapidjson::Value jstrw, jstrh;
		jstrw.SetString("width", doc.GetAllocator());
		jstrh.SetString("height", doc.GetAllocator());
		rapidjson::Value jvalw, jvalh;
		jvalw.SetInt(_width);
		jvalh.SetInt(_height);
		val.AddMember(jstrw, jvalw, doc.GetAllocator());
		val.AddMember(jstrh, jvalh, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		Cels cels(_width * _height);
		if (!cels.empty()) {
			data(&cels.front(), cels.size());
			for (int cel : cels)
				jvaldata.PushBack(cel, doc.GetAllocator());
		}
		val.AddMember(jstrdata, jvaldata, doc.GetAllocator());

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val, Texture::Ptr texture) override {
		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jtiles = val.FindMember("tiles");
		if (jtiles == val.MemberEnd() || !jtiles->value.IsObject())
			return false;

		rapidjson::Value::ConstObject jobjtiles = jtiles->value.GetObject();
		rapidjson::Value::ConstMemberIterator jcount = jobjtiles.FindMember("count");
		if (jcount == jobjtiles.MemberEnd() || !jcount->value.IsArray())
			return false;
		rapidjson::Value::ConstArray count = jcount->value.GetArray();
		if (count.Size() < 2)
			return false;
		if (!count[0].IsInt() || !count[1].IsInt())
			return false;
		const int tileCountX = count[0].GetInt();
		const int tileCountY = count[1].GetInt();

		int tileSizeX = 0;
		int tileSizeY = 0;
		do {
			rapidjson::Value::ConstMemberIterator jsize = jobjtiles.FindMember("size");
			if (jsize == jobjtiles.MemberEnd() || !jsize->value.IsArray())
				break;
			rapidjson::Value::ConstArray size = jsize->value.GetArray();
			if (size.Size() < 2)
				break;
			if (!size[0].IsInt() || !size[1].IsInt())
				break;
			tileSizeX = size[0].GetInt();
			tileSizeY = size[1].GetInt();
		} while (false);

		rapidjson::Value::ConstMemberIterator jw = val.FindMember("width");
		rapidjson::Value::ConstMemberIterator jh = val.FindMember("height");
		if (jw == val.MemberEnd() || jh == val.MemberEnd())
			return false;
		if (!jw->value.IsInt() || !jh->value.IsInt())
			return false;
		const int mapWidth = jw->value.GetInt();
		const int mapHeight = jh->value.GetInt();

		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata == val.MemberEnd() || !jdata->value.IsArray())
			return false;

		rapidjson::Value::ConstArray data = jdata->value.GetArray();
		Cels cels;
		for (rapidjson::SizeType i = 0; i < data.Size() && (int)i < mapWidth * mapHeight; ++i) {
			if (data[i].IsInt())
				cels.push_back(data[i].GetInt());
			else
				cels.push_back(0);
		}

		Tiles tiles_(texture, Math::Vec2i(tileCountX, tileCountY));
		if (tileSizeX > 0 && tileSizeY > 0 && texture)
			tiles_.fit(Math::Vec2i(tileSizeX, tileSizeY));
		tiles(&tiles_);
		if (cels.empty())
			return true;
		if (!load(&cels.front(), mapWidth, mapHeight))
			return false;

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val, Texture::Ptr texture) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval, texture);
	}

private:
	Texture::Ptr blip(Renderer* rnd, int x, int y, int width, int height, PaletteSetter setPalette /* nullable */) const {
		// Prepare.
		typedef std::vector<Colour> Colours;

		if (!_tiles.texture)
			return nullptr;

		x = Math::clamp(x, 0, _width);
		y = Math::clamp(y, 0, _height);
		width = Math::clamp(width, 0, _width);
		height = Math::clamp(height, 0, _height);

		if (width == 0 || height == 0)
			return nullptr;

		// Generate with the area.
		const int paletted = 0; // _tiles.texture->paletted();
		const int bits = paletted ? 1 : 4;
		Texture::Ptr result(Texture::create());
		Byte* pixels = new Byte[(width * _tileWidth) * (height * _tileHeight) * bits];
		memset(pixels, 0, (width * _tileWidth) * (height * _tileHeight) * bits);
		result->fromBytes(rnd, Texture::TARGET, pixels, width * _tileWidth, height * _tileHeight, paletted, Texture::NEAREST);
		result->blend(Texture::BLEND);
		delete [] pixels;

		SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();
		SDL_Texture* tex = (SDL_Texture*)result->pointer(rnd);

		SDL_Texture* prev = SDL_GetRenderTarget(renderer);
		SDL_SetRenderTarget(renderer, tex);
		for (int j = 0; j < height; ++j) {
			for (int i = 0; i < width; ++i) {
				Math::Recti area;
				Texture::Ptr sub = at(x + i, y + j, &area);
				if (!sub)
					continue;

				Colours oldColors;
				const bool changedPalette = !!sub->paletted() && !!setPalette;
				if (changedPalette) {
					for (int k = 0; k < sub->paletteCount(); ++k) {
						const Colour col = sub->paletteAt(k);
						oldColors.push_back(col);
					}

					setPalette(
						i, j,
						[&sub] (int idx, const Colour &col) -> void {
							sub->paletteAt(idx, col);
						}
					);
				}

				SDL_Texture* subTex = (SDL_Texture*)sub->pointer(rnd);
				const SDL_Rect srcRect{
					area.xMin(), area.yMin(),
					area.width(), area.height()
				};
				const SDL_Rect dstRect{
					i * _tileWidth, j * _tileHeight,
					_tileWidth, _tileHeight
				};
				SDL_RenderCopy(renderer, subTex, &srcRect, &dstRect);

				if (changedPalette) {
					for (int k = 0; k < sub->paletteCount(); ++k) {
						const Colour &col = oldColors[k];
						sub->paletteAt(k, col);
					}
				}
			}
		}
		SDL_SetRenderTarget(renderer, prev);

		// Finish.
		return result;
	}
};

Map::Tiles::Tiles() {
}

Map::Tiles::Tiles(Texture::Ptr texture_, const Math::Vec2i &count_) : texture(texture_), count(count_) {
}

Math::Vec2i Map::Tiles::size(void) const {
	Int w = 0;
	Int h = 0;
	if (_size.x <= 0) {
		if (count.x > 0)
			w = texture->width() / count.x;
		else
			w = 0;
	} else {
		w = _size.x;
	}
	if (_size.y <= 0) {
		if (count.x > 0)
			h = texture->height() / count.y;
		else
			h = 0;
	} else {
		h = _size.y;
	}

	return Math::Vec2i(w, h);
}

void Map::Tiles::fit(void) {
	if (_size.x <= 0 && _size.y <= 0)
		return;

	const std::div_t divW = _size.x <= 0 ? std::div_t() : std::div(texture->width(), _size.x);
	const std::div_t divH = _size.y <= 0 ? std::div_t() : std::div(texture->height(), _size.y);
	if (divW.rem == 0 && divH.rem == 0)
		_size = Math::Vec2i(0, 0);
}

void Map::Tiles::fit(const Math::Vec2i &size_) {
	if (size_.x <= 0 && size_.y <= 0)
		return;

	const std::div_t divW = size_.x <= 0 ? std::div_t() : std::div(texture->width(), size_.x);
	const std::div_t divH = size_.y <= 0 ? std::div_t() : std::div(texture->height(), size_.y);
	if (divW.rem == 0 && divH.rem == 0)
		_size = Math::Vec2i(0, 0);
	else
		_size = size_;
}

int Map::INVALID(void) {
	return -1;
}

Map* Map::create(const Tiles* tiles, bool batch) {
	MapImpl* result = new MapImpl(batch);
	result->tiles(tiles);

	return result;
}

void Map::destroy(Map* ptr) {
	MapImpl* impl = static_cast<MapImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
