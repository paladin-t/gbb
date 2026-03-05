/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __MAP_H__
#define __MAP_H__

#include "../gbbasic.h"
#include "texture.h"

/*
** {===========================================================================
** Map
*/

/**
 * @brief Map resource object.
 */
class Map : public Cloneable<Map>, public virtual Object {
public:
	typedef std::shared_ptr<Map> Ptr;

	struct Tiles {
		friend class MapImpl;

	public:
		Texture::Ptr texture = nullptr;
		Math::Vec2i count;

	private:
		Math::Vec2i _size;

	public:
		Tiles();
		Tiles(Texture::Ptr texture, const Math::Vec2i &count);

		Math::Vec2i size(void) const;

		void fit(void);
		void fit(const Math::Vec2i &size);
	};

	typedef std::pair<int, int> Range;

public:
	GBBASIC_CLASS_TYPE('M', 'A', 'P', 'A')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Map** ptr, bool represented) const = 0;
	using Cloneable<Map>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const Map* other) const = 0;

	/**
	 * @brief Cleans up all cached sub maps.
	 */
	virtual int cleanup(void) = 0;
	/**
	 * @brief Cleans up the cached sub maps that collide with the given area.
	 */
	virtual int cleanup(const Math::Recti &area) = 0;

	/**
	 * @brief Gets the tile.
	 *
	 * @param[in, out] tiles
	 */
	virtual const Tiles* tiles(Tiles &tiles) const = 0;
	/**
	 * @brief Sets the tile.
	 *
	 * @param[in] tiles
	 */
	virtual void tiles(const Tiles* tiles) = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	/**
	 * @brief This function is slow.
	 */
	virtual Math::Recti aabb(void) const = 0;
	virtual bool resize(int width, int height, int default_) = 0;
	virtual bool resize(int width, int height) = 0;
	/**
	 * @param[out] buf
	 */
	virtual void data(int* buf, size_t len) const = 0;

	virtual Range range(void) const = 0;

	virtual int get(int x, int y) const = 0;
	virtual bool set(int x, int y, int v, bool expandable = false) = 0;

	/**
	 * @brief Gets renderable data at a specific tile index.
	 *
	 * @param[out] area
	 * @return The entire tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr at(int index, Math::Recti* area /* nullable */) const = 0;
	/**
	 * @brief Gets renderable data at a specific position.
	 *
	 * @param[out] area
	 * @return The entire tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr at(int x, int y, Math::Recti* area /* nullable */) const = 0;
	/**
	 * @brief Gets sub renderable data at a specific area.
	 *
	 * @param[in] ncolors The count of the palette colors to be used to generate sub texture.
	 * @param[in] colors The colors of the palette to be used to generate sub texture.
	 *   These palette colors are not used as the basis for key calculation, and won't re-trigger generation.
	 * @param[in] colorKey Optional key of the palette. Only used as the basis for key calculation.
	 * @return The sub tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr sub(class Renderer* rnd, int x, int y, int width, int height, int ncolors, const Colour* colors /* nullable */, int colorKey) const = 0;
	/**
	 * @brief Gets sub renderable data at a specific area.
	 *
	 * @param[in] colors The palette to be used to generate sub texture.
	 *   These palette colors are not used as the basis for key calculation, and won't re-trigger generation.
	 * @param[in] colorKey Optional key of the palette. Only used as the basis for key calculation.
	 * @return The sub tiled texture or `nullptr`.
	 */
	virtual Texture::Ptr sub(class Renderer* rnd, int x, int y, int width, int height, Indexed::Ptr colors /* nullable */, int colorKey) const = 0;
	/**
	 * @brief Gets the cache size of sub map.
	 */
	virtual int subCacheSize(void) const = 0;
	/**
	 * @brief Sets the cache size of sub map.
	 *
	 * @param[in] val The cache size, -1 for no limit.
	 */
	virtual void subCacheSize(int val) = 0;

	virtual bool update(double delta, unsigned* id /* nullable */) = 0;

	virtual void render(
		class Renderer* rnd,
		int x, int y,
		const Colour* color /* nullable */, bool colorChanged, bool alphaChanged,
		int scale
	) const = 0;

	virtual bool load(const int* cels, int width, int height) = 0;
	virtual bool load(int cel, int width, int height) = 0;
	virtual void unload(void) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(const rapidjson::Value &val, Texture::Ptr texture) = 0;
	virtual bool fromJson(const rapidjson::Document &val, Texture::Ptr texture) = 0;

	static int INVALID(void);

	static Map* create(const Tiles* tiles /* nullable */, bool batch);
	static void destroy(Map* ptr);
};

/* ===========================================================================} */

#endif /* __MAP_H__ */
