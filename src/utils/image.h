/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "../gbbasic.h"
#include "indexed.h"
#include <vector>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef IMAGE_PALETTE_BITS
#	define IMAGE_PALETTE_BITS GBBASIC_PALETTE_COLOR_DEPTH
#endif /* IMAGE_PALETTE_BITS */
#ifndef IMAGE_PALETTE_COLOR_COUNT
#	define IMAGE_PALETTE_COLOR_COUNT Math::pow(2, IMAGE_PALETTE_BITS)
#endif /* IMAGE_PALETTE_COLOR_COUNT */

#ifndef IMAGE_PALETTED_HEADER
#	define IMAGE_PALETTED_HEADER { 'I', 'M', 'G', 'P' }
#endif /* IMAGE_PALETTED_HEADER */
#ifndef IMAGE_COLORED_HEADER
#	define IMAGE_COLORED_HEADER { 'I', 'M', 'G', 'C' }
#endif /* IMAGE_COLORED_HEADER */

/* ===========================================================================} */

/*
** {===========================================================================
** Image
*/

/**
 * @brief Image resource object.
 */
class Image : public Cloneable<Image>, public virtual Object {
public:
	typedef std::shared_ptr<Image> Ptr;
	typedef std::weak_ptr<Image> WeakPtr;

	typedef std::vector<Math::Recti> ErrorPoints;

	typedef std::function<Colour(int, int, Byte)> PaletteResolver;

	typedef std::function<void(int /* tx */, int /* ty */)> TileBeginingSerializer;
	typedef std::function<void(int /* tx */, int /* ty */)> TileEndingSerializer;
	typedef std::function<void(int /* tlineno */, int /* lineno */, int /* tx */, int /* ty */, UInt8 /* ln0 */, UInt8 /* ln1 */)> TileLineSerializer;

	typedef std::function<int(void)> TileParserStepGetter;
	typedef std::function<void(int)> TileParserStepSetter;
	typedef std::function<int(void)> TileCountParser;
	typedef std::function<Int64(int /* idx */)> TileDataParser;

public:
	GBBASIC_CLASS_TYPE('I', 'M', 'G', 'A')

	using Cloneable<Image>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const Image* other) const = 0;

	/**
	 * @return `SDL_Surface*`.
	 */
	virtual void* pointer(void) = 0;

	virtual bool blank(void) const = 0;

	virtual const Indexed::Ptr palette(void) const = 0;
	virtual void palette(Indexed::Ptr val) = 0;
	virtual int paletted(void) const = 0;
	virtual int transparentIndex(void) const = 0;
	virtual void transparentIndex(int val) = 0;

	virtual const Byte* pixels(void) const = 0;
	virtual Byte* pixels(void) = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;

	virtual int channels(void) const = 0;

	virtual bool resize(int width, int height, bool stretch) = 0;
	virtual bool pitch(int width_, int height_, int pitch_, int maxHeight_, int unitSize) = 0;

	virtual void flip(bool h, bool v) = 0;

	virtual void cleanup(void) = 0;

	/**
	 * @param[out] col
	 */
	virtual bool get(int x, int y, Colour &col) const = 0;
	virtual bool set(int x, int y, const Colour &col) = 0;
	/**
	 * @param[out] index
	 */
	virtual bool get(int x, int y, int &index) const = 0;
	virtual bool set(int x, int y, int index) = 0;

	virtual Image* quantized2Bpp(void) const = 0;

	virtual void weight(int r, int g, int b, int a) = 0;
	virtual bool quantize(const Colour* colors, int colorCount, bool p2p) = 0;
	virtual bool realize(void) = 0;
	virtual bool realize(PaletteResolver resolve) = 0;

	virtual Colour findLightest(void) const = 0;

	virtual bool canSliceV(int n, int idx) const = 0;
	virtual bool canSliceH(int n, int idx) const = 0;
	virtual bool canSliceV(int n, const Colour &col) const = 0;
	virtual bool canSliceH(int n, const Colour &col) const = 0;

	/**
	 * @param[out] dst
	 */
	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy, bool hFlip, bool vFlip, Byte alpha) const = 0;
	/**
	 * @param[out] dst
	 */
	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy, bool hFlip, bool vFlip) const = 0;
	/**
	 * @param[out] dst
	 */
	virtual bool blit(Image* dst, int x, int y, int w, int h, int sx, int sy) const = 0;

	virtual bool serializeTile(int tw, int th, int tx, int ty, TileLineSerializer serializeLine /* nullable */) const = 0;
	virtual bool serializeTiles(int tw, int th, TileBeginingSerializer serializeBegining /* nullable */, TileEndingSerializer serializeEnding /* nullable */, TileLineSerializer serializeLine /* nullable */) const = 0;
	virtual bool parseTile(int tw, int th, int tx, int ty, TileParserStepGetter getParserStep /* nullable */, TileParserStepSetter setParserStep /* nullable */, TileCountParser parseCount /* nullable */, TileDataParser parseData /* nullable */) = 0;
	virtual bool parseTiles(int tw, int th, TileCountParser parseCount /* nullable */, TileDataParser parseData /* nullable */) = 0;

	/**
	 * @param[out] palette
	 * @param[out] tiles
	 * @param[out] map
	 */
	virtual bool serializeSgbBorder(class Bytes* palette /* nullable */, class Bytes* tiles /* nullable */, class Bytes* map /* nullable */, ErrorPoints* errorPoints /* nullable */) const = 0;

	virtual bool fromBlank(int width, int height, int paletted) = 0;

	virtual bool fromImage(const Image* src) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toRaw(class Bytes* val) const = 0;
	virtual bool fromRaw(const Byte* val, size_t size) = 0;
	virtual bool fromRaw(const class Bytes* val) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toBytes(class Bytes* val, const char* type) const = 0;
	virtual bool fromBytes(const Byte* val, size_t size) = 0;
	virtual bool fromBytes(const class Bytes* val) = 0;

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

	static Image* create(void);
	static Image* create(Indexed::Ptr palette /* nullable */);
	static void destroy(Image* ptr);
};

/* ===========================================================================} */

#endif /* __IMAGE_H__ */
