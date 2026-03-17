/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ASSETS_H__
#define __ASSETS_H__

#include "../gbbasic.h"
#include "actor.h"
#include "bytes.h"
#include "font.h"
#include "image.h"
#include "music.h"
#include "scene.h"
#include "sfx.h"

/*
** {===========================================================================
** Macros and constants
*/

/**< Project, header, formatting. */

#ifndef ASSETS_ROM_ICON_SIZE
#	define ASSETS_ROM_ICON_SIZE 32
#endif /* ASSETS_ROM_ICON_SIZE */

#ifndef ASSETS_NAME_STRING_MAX_LENGTH
#	define ASSETS_NAME_STRING_MAX_LENGTH 64
#endif /* ASSETS_NAME_STRING_MAX_LENGTH */

#ifndef ASSETS_PRETTY_JSON_ENABLED
#	if defined GBBASIC_DEBUG
#		define ASSETS_PRETTY_JSON_ENABLED 0
#	else /* GBBASIC_DEBUG */
#		define ASSETS_PRETTY_JSON_ENABLED 0
#	endif /* GBBASIC_DEBUG */
#endif /* ASSETS_PRETTY_JSON_ENABLED */

/**< Common. */

#define ASSETS_MUL2(A)          ((A) << 1)
#define ASSETS_MUL4(A)          ((A) << 2)
#define ASSETS_MUL8(A)          ((A) << 3)
#define ASSETS_MUL16(A)         ((A) << 4)
#define ASSETS_MUL32(A)         ((A) << 5)
#define ASSETS_MUL64(A)         ((A) << 6)
#define ASSETS_MUL128(A)        ((A) << 7)

#define ASSETS_DIV2(A)          ((A) >> 1)
#define ASSETS_DIV4(A)          ((A) >> 2)
#define ASSETS_DIV8(A)          ((A) >> 3)
#define ASSETS_DIV16(A)         ((A) >> 4)
#define ASSETS_DIV32(A)         ((A) >> 5)
#define ASSETS_DIV64(A)         ((A) >> 6)
#define ASSETS_DIV128(A)        ((A) >> 7)

#define ASSETS_MOD2(A)          ((A) & 0x01)
#define ASSETS_MOD4(A)          ((A) & 0x03)
#define ASSETS_MOD8(A)          ((A) & 0x07)
#define ASSETS_MOD16(A)         ((A) & 0x0f)
#define ASSETS_MOD32(A)         ((A) & 0x1f)

#define ASSETS_TO_SCREEN(A)     ASSETS_DIV16(A)
#define ASSETS_FROM_SCREEN(A)   ASSETS_MUL16(A)

/**< Font. */

#ifndef ASSETS_FONT_DIR
#	define ASSETS_FONT_DIR "../fonts/" /* Relative path. */
#endif /* ASSETS_FONT_DIR */
#ifndef ASSETS_FONT_SUBSTITUTION_CONFIG_FILE
#	define ASSETS_FONT_SUBSTITUTION_CONFIG_FILE ASSETS_FONT_DIR "sub.json"
#endif /* ASSETS_FONT_SUBSTITUTION_CONFIG_FILE */
#ifndef ASSETS_FONT_MAX_SIZE
#	define ASSETS_FONT_MAX_SIZE 16
#endif /* ASSETS_FONT_MAX_SIZE */

/**< Map. */

#ifndef ASSETS_MAP_GRAPHICS_LAYER
#	define ASSETS_MAP_GRAPHICS_LAYER 0
#endif /* ASSETS_MAP_GRAPHICS_LAYER */
#ifndef ASSETS_MAP_ATTRIBUTES_LAYER
#	define ASSETS_MAP_ATTRIBUTES_LAYER 1
#endif /* ASSETS_MAP_ATTRIBUTES_LAYER */

#ifndef ASSETS_MAP_ATTRIBUTES_REF_WIDTH
#	define ASSETS_MAP_ATTRIBUTES_REF_WIDTH 16 /* In tiles. */
#endif /* ASSETS_MAP_ATTRIBUTES_REF_WIDTH */

/**< Actor. */

#ifndef ASSETS_ACTOR_MAX_COUNT
#	define ASSETS_ACTOR_MAX_COUNT 21
#endif /* ASSETS_ACTOR_MAX_COUNT */

#ifndef ASSETS_ACTOR_MAX_ANIMATIONS
#	define ASSETS_ACTOR_MAX_ANIMATIONS 8
#endif /* ASSETS_ACTOR_MAX_ANIMATIONS */

#ifndef ASSETS_ACTOR_MOVE_SPEED
#	define ASSETS_ACTOR_MOVE_SPEED ASSETS_FROM_SCREEN(1)
#endif /* ASSETS_ACTOR_MOVE_SPEED */

#ifndef ASSETS_ACTOR_ANIMATION_INTERVAL
#	define ASSETS_ACTOR_ANIMATION_INTERVAL 15
#endif /* ASSETS_ACTOR_ANIMATION_INTERVAL */

/**< Projectile. */

#ifndef ASSETS_PROJECTILE_MAX_ANIMATIONS
#	define ASSETS_PROJECTILE_MAX_ANIMATIONS 4
#endif /* ASSETS_PROJECTILE_MAX_ANIMATIONS */

/**< Trigger. */

#ifndef ASSETS_TRIGGER_MAX_COUNT
#	define ASSETS_TRIGGER_MAX_COUNT 31
#endif /* ASSETS_TRIGGER_MAX_COUNT */

/**< Scene. */

#ifndef ASSETS_SCENE_GRAPHICS_LAYER
#	define ASSETS_SCENE_GRAPHICS_LAYER ASSETS_MAP_GRAPHICS_LAYER
#endif /* ASSETS_SCENE_GRAPHICS_LAYER */
#ifndef ASSETS_SCENE_ATTRIBUTES_LAYER
#	define ASSETS_SCENE_ATTRIBUTES_LAYER ASSETS_MAP_ATTRIBUTES_LAYER
#endif /* ASSETS_SCENE_ATTRIBUTES_LAYER */
#ifndef ASSETS_SCENE_PROPERTIES_LAYER
#	define ASSETS_SCENE_PROPERTIES_LAYER 2
#endif /* ASSETS_SCENE_PROPERTIES_LAYER */
#ifndef ASSETS_SCENE_ACTORS_LAYER
#	define ASSETS_SCENE_ACTORS_LAYER 4
#endif /* ASSETS_SCENE_ACTORS_LAYER */
#ifndef ASSETS_SCENE_TRIGGERS_LAYER
#	define ASSETS_SCENE_TRIGGERS_LAYER 3
#endif /* ASSETS_SCENE_TRIGGERS_LAYER */
#ifndef ASSETS_SCENE_DEF_LAYER
#	define ASSETS_SCENE_DEF_LAYER 5
#endif /* ASSETS_SCENE_DEF_LAYER */

#ifndef ASSETS_SCENE_ATTRIBUTES_REF_WIDTH
#	define ASSETS_SCENE_ATTRIBUTES_REF_WIDTH ASSETS_MAP_ATTRIBUTES_REF_WIDTH /* In tiles. */
#endif /* ASSETS_SCENE_ATTRIBUTES_REF_WIDTH */
#ifndef ASSETS_SCENE_PROPERTIES_REF_WIDTH
#	define ASSETS_SCENE_PROPERTIES_REF_WIDTH 16 /* In tiles. */
#endif /* ASSETS_SCENE_PROPERTIES_REF_WIDTH */

/**< Types. */

enum class Directions : UInt8 {
	DIRECTION_NONE         = 8,
	DIRECTION_DOWN         = 0,
	DIRECTION_RIGHT        = 1,
	DIRECTION_UP           = 2,
	DIRECTION_LEFT         = 3,
	DIRECTION_DOWN_RIGHT   = 4,
	DIRECTION_UP_RIGHT     = 5,
	DIRECTION_UP_LEFT      = 6,
	DIRECTION_DOWN_LEFT    = 7
};

enum class Layers : UInt8 {
	SCENE_LAYER_MAP       = 0x01, // Must have.
	SCENE_LAYER_ATTR      = 0x02,
	SCENE_LAYER_PROP      = 0x04,
	SCENE_LAYER_ACTOR     = 0x08,
	SCENE_LAYER_TRIGGER   = 0x10,
	SCENE_LAYER_DEF       = 0x20, // Must have.
	SCENE_LAYER_ALL       = (SCENE_LAYER_MAP | SCENE_LAYER_ATTR | SCENE_LAYER_PROP | SCENE_LAYER_ACTOR | SCENE_LAYER_TRIGGER | SCENE_LAYER_DEF)
};

/* ===========================================================================} */

/*
** {===========================================================================
** Shared structures shared between the compiler and VM
*/

#pragma pack(push, 1)

struct upoint8_t {
	UInt8 x = 0;
	UInt8 y = 0;

	upoint8_t();

	bool operator == (const upoint8_t &other) const;
	bool operator != (const upoint8_t &other) const;
	bool operator < (const upoint8_t &other) const;
	bool operator > (const upoint8_t &other) const;

	int compare(const upoint8_t &other) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
	bool fromJson(const rapidjson::Value &val);
};

struct point16_t {
	Int16 x = 0;
	Int16 y = 0;

	point16_t();

	bool operator == (const point16_t &other) const;
	bool operator != (const point16_t &other) const;
	bool operator < (const point16_t &other) const;
	bool operator > (const point16_t &other) const;

	int compare(const point16_t &other) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
	bool fromJson(const rapidjson::Value &val);
};

struct upoint16_t {
	UInt16 x = 0;
	UInt16 y = 0;

	upoint16_t();

	bool operator == (const upoint16_t &other) const;
	bool operator != (const upoint16_t &other) const;
	bool operator < (const upoint16_t &other) const;
	bool operator > (const upoint16_t &other) const;

	int compare(const upoint16_t &other) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
	bool fromJson(const rapidjson::Value &val);
};

struct boundingbox_t {
	Int8 left = 0;
	Int8 right = 0;
	Int8 top = 0;
	Int8 bottom = 0;

	boundingbox_t();
	boundingbox_t(Int8 l, Int8 r, Int8 t, Int8 b);

	bool operator == (const boundingbox_t &other) const;
	bool operator != (const boundingbox_t &other) const;
	bool operator < (const boundingbox_t &other) const;
	bool operator > (const boundingbox_t &other) const;

	int compare(const boundingbox_t &other) const;

	int width(void) const;
	int height(void) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
	bool fromJson(const rapidjson::Value &val);
};

struct glyph_option_t {
	bool two_bits_per_pixel        : 1;
	bool full_word                 : 1;
	bool full_word_for_non_ascii   : 1;
	bool inverted                  : 1;
	bool reserved1                 : 1;
	bool reserved2                 : 1;
	bool reserved3                 : 1;
	bool reserved4                 : 1;

	glyph_option_t();
	glyph_option_t(bool tbpp);
	glyph_option_t(bool tbpp, bool fullword, bool fullword4all, bool inverted_);
};

struct glyph_t {
	// If `bank != 0` and `bank != 1`, enables `ptr` for banked address;
	// elseif `bank == 1`, enables `advance` for advance size between two spaces;
	// else `bank == 0`, enables `codepoint`.
	UInt8 bank = 0;
	union {
		UInt16 ptr = 0;
		UInt16 advance;
		UInt16 codepoint;
	};
	UInt8 size = 0;

	glyph_t();
	glyph_t(UInt8 bank_, UInt16 ptr_, UInt8 sz);
	glyph_t(UInt16 cp);
};

struct animation_t {
	UInt8 begin = 0;
	UInt8 end = 0;

	animation_t();

	bool operator == (const animation_t &other) const;
	bool operator != (const animation_t &other) const;
	bool operator < (const animation_t &other) const;
	bool operator > (const animation_t &other) const;

	int compare(const animation_t &other) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
	bool fromJson(const rapidjson::Value &val);
};

// Shared partially between the compiler and VM.
struct actor_t {
	// Flags.
	bool enabled      : 1;
	bool hidden       : 1;
	bool pinned       : 1;
	bool persistent   : 1;
	bool following; // This property is a field here for the editor and compiler, but it's a global variable in the VM.
	// Properties.
	UInt8 direction = (UInt8)Directions::DIRECTION_DOWN;
	// Behaviour.
	UInt8 behaviour = 0;

	actor_t();
};

// Shared partially between the compiler and VM.
struct projectile_t {
	// Behaviour.
	UInt8 life_time = 0;
	UInt16 initial_offset = 0;

	projectile_t();
};

// Shared partially between the compiler and VM, to define either an actor or projectile.
struct active_t : public actor_t, public projectile_t {
	typedef std::function<std::string(int)> BehaviourSerializer;
	typedef std::function<int(const std::string &)> BehaviourParser;

	// Properties.
	boundingbox_t bounds;
	// Resources.
	UInt8 sprite_bank = 0;
	UInt16 sprite_frames = 0;
	UInt8 animation_interval = ASSETS_ACTOR_ANIMATION_INTERVAL;
	animation_t animations[ASSETS_ACTOR_MAX_ANIMATIONS];
	// Behaviour.
	UInt8 move_speed = ASSETS_ACTOR_MOVE_SPEED;
	// Collision.
	UInt8 collision_group = 0;

	active_t();

	bool operator == (const active_t &other) const;
	bool operator != (const active_t &other) const;
	bool operator < (const active_t &other) const;
	bool operator > (const active_t &other) const;

	size_t hash(void) const;
	int compare(const active_t &other) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc, BehaviourSerializer serializeBehaviour) const;
	bool fromJson(const rapidjson::Value &val, BehaviourParser parseBehaviour);
};

// Shared partially between the compiler and VM.
struct scene_t {
	bool is_16x16_grid     : 1;
	bool is_16x16_player   : 1;
	UInt8 gravity = 0;
	UInt8 jump_gravity = 0;
	UInt8 jump_max_count = 0;
	UInt8 jump_max_ticks = 0;
	UInt8 climb_velocity = 0;

	upoint16_t camera_position;
	upoint8_t camera_deadzone;

	scene_t();

	bool operator == (const scene_t &other) const;
	bool operator != (const scene_t &other) const;
	bool operator < (const scene_t &other) const;
	bool operator > (const scene_t &other) const;

	size_t hash(void) const;
	int compare(const scene_t &other) const;

	bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
	bool fromJson(const rapidjson::Value &val);
};

#pragma pack(pop)

/* ===========================================================================} */

/*
** {===========================================================================
** Semaphore
*/

struct Semaphore {
public:
	typedef std::function<void(void)> WaitHandler;
	typedef std::shared_ptr<bool> StatusPtr;

private:
	WaitHandler _waitHandler = nullptr;
	StatusPtr _working = nullptr;

public:
	Semaphore();
	Semaphore(WaitHandler h, StatusPtr working);

	bool working(void);
	void wait(void);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Base assets
*/

struct BaseAssets {
	struct Entry {
		enum class ParsingStatuses {
			SUCCESS,
			NOT_A_MULTIPLE_OF_8x8,
			OUT_OF_BOUNDS
		};

		class Editable* editor = nullptr; // Non-serialized.
	};
	struct Versionable {
		unsigned revision = 1; // Non-serialized.

		unsigned increaseRevision(void);
	};
	struct Invalidatable {
		unsigned latestRefRevision = 0; // Non-serialized.

		bool isRefRevisionChanged(const Versionable* versionable) const;
		unsigned synchronizeRefRevision(const Versionable* versionable);
	};
};

typedef std::function<void(const char*)> PrintHandler;
typedef std::function<void(const char*, bool)> WarningOrErrorHandler;
typedef std::function<void(int, int, const char*)> TileIssueReportHandler;

/* ===========================================================================} */

/*
** {===========================================================================
** Palette assets
*/

struct PaletteAssets {
	struct Entry : public BaseAssets::Entry, public BaseAssets::Versionable {
		Indexed::Ptr data = nullptr;

		Entry();

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);
	};
	typedef std::vector<Entry> Array;
	typedef std::function<Entry*(int)> Getter;

	typedef std::vector<int> Groups;

	Array entries;

	PaletteAssets();

	bool equals(const PaletteAssets &other) const;
	void copyFrom(const PaletteAssets &other, Groups* groups /* nullable */);
	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	void clear(void);
	const Entry* get(int index) const;
	Entry* get(int index);

	Bytes::Ptr serializeBackgroundPalettes(void) const;
	Bytes::Ptr serializeSpritePalettes(void) const;

	bool serializeBasic(std::string &val) const;

	bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
	bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Font assets
*/

// Glyph table for text rasterization with TTF font.
struct GlyphTable {
	struct Entry {
		Font::Codepoint codepoint = 0;
		int width = 0;
		int height = 0;
		int bank = 0;
		int address = 0;
		bool unknown = false; // For compiler only.
		int refCount = 0;

		Entry();
		Entry(Font::Codepoint cp);
		Entry(Font::Codepoint cp, int ref);

		UInt8 size(void) const;
	};
	typedef std::vector<Entry> Array;

	Array entries; // Baked glyphs.

	GlyphTable();

	int count(void) const;
	bool add(const Entry &entry);
	void sort(void);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(Font::Codepoint cp) const;
	Entry* find(Font::Codepoint cp);
};

struct FontAssets {
	struct Entry : public BaseAssets::Entry {
		enum class Copying {
			REFERENCED,
			WAITING_FOR_COPYING,
			COPIED,
			EMBEDDED
		};

		Copying copying = Copying::REFERENCED; // Non-serialized.
		std::string directory; // Non-serialized. Calculated from path.
		std::string path; // External, file path or referenced/embedded file name.
		std::string encodedTtfData; // Optional, encoded TTF data.
		std::string bitmapData; // Optional, bitmap-based data.
		Math::Vec2i size = Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE);
		Math::Vec2i maxSize = Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
		bool trim = true;
		int offset = GBBASIC_FONT_DEFAULT_OFFSET;
		bool isTwoBitsPerPixel = GBBASIC_FONT_DEFAULT_IS_2BPP;
		bool preferFullWord = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD;
		bool preferFullWordForNonAscii = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII;
		int enabledEffect = (int)Font::Effects::NONE;
		int shadowOffset = 1;
		float shadowStrength = 0.3f;
		UInt8 shadowDirection = (UInt8)Directions::DIRECTION_DOWN_RIGHT;
		int outlineOffset = 1;
		float outlineStrength = 0.3f;
		int thresholds[4];
		bool inverted = false;
		Font::Codepoints arbitrary; // Array of arbitrary characters for runtime formatting, holds up to 256 elements.

		std::string content;
		std::string name;
		int magnification = -1; // Non-polluting.

		bool isAsset = false; // Non-serialized.
		Font::Ptr object = nullptr; // Non-serialized.
		GlyphTable glyphs; // Non-serialized.
		int bank = 0; // Non-serialized.
		int address = 0; // Non-serialized.

		mutable Font::Ptr substitutionObject = nullptr; // Non-serialized. For compiler only.
		mutable int substitutionOffset = GBBASIC_FONT_DEFAULT_OFFSET; // Non-serialized. For compiler only.
		mutable int substitutionThresholds[4]; // Non-serialized. For compiler only.

		Math::Vec2i frameMargin = Math::Vec2i(1, 1); // Readonly. For editor only.
		Math::Vec2i characterPadding = Math::Vec2i(0, 1); // Readonly. For editor only.

		Entry();
		Entry(bool toBeCopied, bool toBeEmbedded, const std::string &dir, const std::string &path_, const Math::Vec2i* size /* nullable */, const std::string &content_, bool generateObject);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		Font::Ptr &touch(void);
		void cleanup(void);

		Font::Ptr &touchSubstitution(int* offset_ /* nullable */, int* thresholds_ /* nullable */) const;
		void cleanupSubstitution(void) const;
	};
	typedef std::vector<Entry> Array;

	Array entries; // Font (TTF) informations.

	FontAssets();

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	void clear(void);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;

	bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
	bool fromString(const std::string &val, const std::string &dir, bool isAsset, WarningOrErrorHandler onWarningOrError /* nullable */);

	/**
	 * @param[out] encodedTtfDataPtr
	 * @param[out] obj
	 */
	static bool loadTtf(std::string* encodedTtfDataPtr /* nullable */, Font::Ptr obj, const std::string &path, int size, int maxWidth, int maxHeight, int permeation, bool trim);
	/**
	 * @param[out] bitmapDataPtr
	 * @param[out] obj
	 */
	static bool loadBitmap(std::string* bitmapDataPtr /* nullable */, Font::Ptr obj, const std::string &path, const Math::Vec2i &size, int permeation);
	/**
	 * @param[out] height
	 * @param[out] offset
	 */
	static bool calculateBestFit(const Entry &font, int* size /* nullable */, int* offset /* nullable */);
	/**
	 * @param[out] width
	 * @param[out] height
	 */
	static bool measure(const Entry &font, const GlyphTable::Entry &glyph, int* width /* nullable */, int* height /* nullable */);
	/**
	 * @param[in, out] font
	 * @param[in, out] glyph
	 * @param[out] buf
	 * @param[out] bytes_
	 */
	static bool bake(Entry &font, GlyphTable::Entry &glyph, Bytes* buf /* nullable */, int* bytes_ /* nullable */);
	static int getBits(const Colour &col, bool isTwoBitsPerPixel, const int thresholds[4], bool inverted);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Code assets
*/

struct CodeAssets {
	struct Entry : public BaseAssets::Entry {
		std::string data;

		Entry(const std::string &val);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	const Entry* get(int index) const;
	Entry* get(int index);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Tiles assets
*/

struct TilesAssets {
	struct Entry : public BaseAssets::Entry, public BaseAssets::Versionable, public BaseAssets::Invalidatable {
		typedef std::function<bool(int, Colour &)> PaletteColorGetter;

		Image::Ptr data = nullptr;
		int ref = 0; // To a palette.
		std::string name;
		int magnification = -1; // Non-polluting.

		Renderer* renderer = nullptr; // Non-serialized. Foreign.
		PaletteAssets::Getter getPalette = nullptr; // Non-serialized. Foreign.
		Indexed::Ptr palette = nullptr; // Non-serialized.
		Texture::Ptr texture = nullptr; // Non-serialized.

		Entry(Renderer* rnd, PaletteAssets::Getter getplt);
		Entry(Renderer* rnd, const std::string &val, PaletteAssets::Getter getplt);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		Texture::Ptr &touch(void);
		void cleanup(void);
		bool refresh(void);

		int estimateFinalByteCount(void) const;

		bool serializeBasic(std::string &val, int page, bool forTiles) const;

		bool serializeDataSequence(std::string &val, int base) const;
		bool parseDataSequence(Image::Ptr &img, const std::string &val, bool isCompressed) const;

		bool serializeJson(std::string &val, bool pretty) const;
		bool parseJson(Image::Ptr &img, const std::string &val, ParsingStatuses &status) const;

		bool serializeImage(Image* val, const Math::Recti* area /* nullable */, PaletteColorGetter getCol /* nullable */) const;
		bool serializeImage(Image* val, const Math::Recti* area /* nullable */) const;
		bool serializeImage(Bytes* val, const char* type, const Math::Recti* area /* nullable */, PaletteColorGetter getCol /* nullable */) const;
		bool serializeImage(Bytes* val, const char* type, const Math::Recti* area /* nullable */) const;
		bool parseImage(Image::Ptr &img, const Image* val, ParsingStatuses &status) const;
		bool parseImage(Image::Ptr &img, const Bytes* val, ParsingStatuses &status) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;
	typedef std::function<Entry*(int)> Getter;

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;
};

/* ===========================================================================} */

/*
** {===========================================================================
** Map assets
*/

struct MapAssets {
	struct Entry : public BaseAssets::Entry, public BaseAssets::Versionable, public BaseAssets::Invalidatable {
		Map::Ptr data = nullptr;
		int ref = 0; // To a tiles.
		bool hasAttributes = false;
		Map::Ptr attributes = nullptr;
		bool editAsImage = false;
		bool editedAsImage = false;
		Image::Ptr image = nullptr;
		bool localPaletteEnabled = false;
		PaletteAssets::Array localPalette;
		std::string name;
		int magnification = -1; // Non-polluting.
		bool allowFlip = false;
		bool optimize = true;

		TilesAssets::Getter getTiles = nullptr; // Non-serialized. Foreign.

		Entry(int ref_, TilesAssets::Getter gettls, Texture::Ptr attribtex);
		Entry(const std::string &val, TilesAssets::Getter gettls, Texture::Ptr attribtex);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		Texture::Ptr touch(void);
		void cleanup(void);
		void cleanup(const Math::Recti &area);
		bool refresh(void);

		int estimateFinalByteCount(void) const;

		bool serializeBasic(std::string &val, int page) const;

		bool serializeDataSequence(std::string &val, int base, int layer) const;
		bool parseDataSequence(Map::Ptr &map, const std::string &val, int layer) const;

		bool serializeJson(std::string &val, bool pretty) const;
		bool parseJson(Map::Ptr &map, bool &hasAttrib, Map::Ptr &attrib, const std::string &val, ParsingStatuses &status) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;
	typedef std::function<Entry*(int)> Getter;

	struct Slice {
		typedef std::vector<Slice> Array;

		size_t hash = 0;
		Image::Ptr image = nullptr;
		int palette = 0;

		Slice();
		Slice(Image::Ptr img, int pal);

		bool operator == (const Slice &other) const;
		bool operator != (const Slice &other) const;

		bool equals(const Slice &other) const;
	};

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;

	/**
	 * @param[out] val
	 */
	static bool serializeImage(const TilesAssets::Entry &tiles_, const MapAssets::Entry &map_, Image* val);
	/**
	 * @param[in, out] palette_
	 * @param[out] tiles_
	 * @param[out] map_
	 */
	static bool parseImage(
		PaletteAssets::Array &palette_, TilesAssets::Entry &tiles_, MapAssets::Entry &map_,
		const Image* val,
		bool allowFlip, bool fillLocalPalette, bool allowSimpleQuantized, bool allowReuse, bool allowTilesOverflow,
		PaletteAssets::Getter getplt, TilesAssets::Getter gettls, int tilesPageCount,
		TileIssueReportHandler onReportTileIssue = nullptr,
		PrintHandler onPrint = nullptr, WarningOrErrorHandler onWarningOrError = nullptr
	);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Audio assets
*/

struct Tracker {
	enum class DataTypes : int {
		NOTE,
		INSTRUMENT,
		EFFECT_CODE,
		EFFECT_PARAMETERS,
		COUNT
	};

	enum class ViewTypes : int {
		TRACKER,
		PIANO,
		WAVES
	};

	struct Location {
		int channel = 0;
		int line = 0;

		Location();
		Location(int ch, int ln);

		static Location INVALID(void);
	};

	struct Cursor {
		int sequence = 0;
		int channel = 0;
		int line = 0;
		DataTypes data = DataTypes::NOTE;

		Cursor();
		Cursor(int seq, int ch, int ln, DataTypes d);
		Cursor(const Cursor &other);

		Cursor &operator = (const Cursor &other);
		bool operator == (const Cursor &other) const;
		bool operator != (const Cursor &other) const;
		bool operator < (const Cursor &other) const;
		bool operator <= (const Cursor &other) const;
		bool operator > (const Cursor &other) const;
		bool operator >= (const Cursor &other) const;

		int compare(const Cursor &other) const;
		bool equals(const Cursor &other) const;

		Cursor nextData(void) const;
		Cursor nextLine(void) const;

		bool valid(void) const;
		void set(int seq, int ch, int ln, DataTypes d);
		void set(int ch, int ln, DataTypes d);

		static Cursor INVALID(void);
	};

	struct Range {
		Cursor first = Cursor::INVALID();
		Cursor second = Cursor::INVALID();

		Range();

		void start(int seq, int ch, int ln, DataTypes data);
		void start(const Cursor &where);
		void end(int seq, int ch, int ln, DataTypes data);
		void end(const Cursor &where);

		Cursor min(void) const;
		Cursor max(void) const;

		Math::Vec2i size(void) const;

		bool startsWith(int ch, int ln, DataTypes data) const;
		bool startsWith(const Cursor &where) const;
		bool endsWith(int ch, int ln, DataTypes data) const;
		bool endsWith(const Cursor &where) const;
		bool single(void) const;
		bool invalid(void) const;
		void clear(void);
	};
};

struct MusicAssets {
	struct Entry : public BaseAssets::Entry {
		Music::Ptr data = nullptr;
		int magnification = -1; // Non-polluting.

		Tracker::ViewTypes lastView = Tracker::ViewTypes::TRACKER;
		int lastNote = 0;
		int lastOctave = 0;

		Entry(void);
		Entry(const std::string &val);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		int estimateFinalByteCount(void) const;

		bool serializeBasic(std::string &val, int page) const;

		bool serializeC(std::string &val) const;
		bool parseC(Music::Ptr &music, const char* filename, const std::string &val, Music::ErrorHandler onError) const;

		bool serializeJson(std::string &val, bool pretty) const;
		bool parseJson(Music::Ptr &music, const std::string &val) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;
};

struct SfxAssets {
	struct Entry : public BaseAssets::Entry {
		Sfx::Ptr data = nullptr;

		Texture::Ptr shape = nullptr; // Non-serialized.

		Entry(void);
		Entry(const std::string &val);
		Entry(const Sfx::Sound &val);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		int estimateFinalByteCount(void) const;

		bool serializeBasic(std::string &val, int page) const;

		bool serializeJson(std::string &val, bool pretty) const;
		bool parseJson(Sfx::Ptr &sfx, const std::string &val) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool insert(const Entry &entry, int index);
	bool remove(int index);
	void clear(void);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;
	bool swap(int leftIndex, int rightIndex);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Actor assets
*/

struct ActorAssets {
	struct Entry : public BaseAssets::Entry, public BaseAssets::Versionable, public BaseAssets::Invalidatable {
		typedef std::function<Colour(int, int, int, Byte)> PaletteResolver;
		typedef std::function<bool(UInt8)> PlayerBehaviourCheckingHandler;

		Actor::Ptr data = nullptr;
		int ref = GBBASIC_ACTOR_DEFAULT_PALETTE; // To a palette.
		int figure = 0; // The iconic figure frame index.
		std::string name;
		int magnification = -1; // Non-polluting.
		bool asActor = true; // Whether the asset is viewed as an actor or projectile.
		active_t definition;

		Renderer* renderer = nullptr; // Non-serialized. Foreign.
		PaletteAssets::Getter getPalette = nullptr; // Non-serialized. Foreign.
		Indexed::Ptr palette = nullptr; // Non-serialized.
		active_t::BehaviourSerializer serializeBehaviour = nullptr; // Non-serialized. Foreign.
		active_t::BehaviourParser parseBehaviour = nullptr; // Non-serialized. Foreign.
		mutable Actor::Animation animation; // Non-serialized.
		mutable Actor::Shadow::Array shadow; // Non-serialized. Correspond to frames.
		mutable Actor::Slice::Array slices; // Non-serialized. Shared by all frames.

		Entry(Renderer* rnd, bool is8x16, PaletteAssets::Getter getplt, active_t::BehaviourSerializer serializeBhvr, active_t::BehaviourParser parseBhvr);
		Entry(Renderer* rnd, const std::string &val, PaletteAssets::Getter getplt, active_t::BehaviourSerializer serializeBhvr, active_t::BehaviourParser parseBhvr);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		void touch(void);
		void touch(PaletteResolver resolve);
		void cleanup(void);
		void cleanup(int frame);
		bool refresh(void);

		Colour colorAt(int frame_, int x, int y, Byte idx) const;
		Texture::Ptr figureTexture(Math::Vec2i* anchor /* nullable */, PaletteResolver resolve /* nullable */) const;

		std::pair<int, int> estimateFinalByteCount(void) const;

		bool serializeBasic(std::string &val, int page, bool asActor) const;

		bool serializeC(std::string &val) const;

		bool serializeDataSequence(int frame, std::string &val, int base) const;
		bool parseDataSequence(int frame, Image::Ptr &img, const std::string &val, bool isCompressed) const;

		bool serializeJson(std::string &val, bool pretty) const;
		bool parseJson(Actor::Ptr &actor, int &figure, bool &asActor, active_t &def, const std::string &val, ParsingStatuses &status) const;

		bool serializeSpriteSheet(Bytes* val, const char* type, int pitch) const;
		bool parseSpriteSheet(Actor::Ptr &actor, const Bytes* val, const Math::Vec2i &n, ParsingStatuses &status) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;
	typedef std::function<Entry*(int)> Getter;

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;

	static bool computeSpriteSheetSlices(const Image::Ptr &img, Math::Vec2i &count);
	static Math::Vec2i computeAnchor(int twidth, int theight);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Scene assets
*/

template<typename T> struct RoutineOverriding {
	typedef std::vector<RoutineOverriding> Array;
	typedef T Routines;

	Math::Vec2i position;
	Routines routines;

	RoutineOverriding() {
	}
	RoutineOverriding(const Math::Vec2i &pos) : position(pos) {
	}

	bool operator < (const RoutineOverriding &other) const {
		return compare(other) < 0;
	}
	bool operator > (const RoutineOverriding &other) const {
		return compare(other) > 0;
	}
	int compare(const RoutineOverriding &other) const {
		if (position.x < other.position.x)
			return -1;
		else if (position.x > other.position.x)
			return 1;

		if (position.y < other.position.y)
			return -1;
		else if (position.y > other.position.y)
			return 1;

		// `routines` doesn't count.

		return 0;
	}
};

struct ActorRoutines {
	std::string update;
	std::string onHits;

	ActorRoutines();
	ActorRoutines(const std::string &up, const std::string &hit);
};

typedef RoutineOverriding<ActorRoutines> ActorRoutineOverriding;

struct Routine {
	static ActorRoutineOverriding* add(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos);
	static bool remove(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos);
	static ActorRoutineOverriding* move(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos, const Math::Vec2i &newPos);
	static void sort(ActorRoutineOverriding::Array &overridings);
	static const ActorRoutineOverriding* search(const ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos);
	static ActorRoutineOverriding* search(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos);

	static bool toJson(const ActorRoutineOverriding &overriding, rapidjson::Value &val, rapidjson::Document &doc);
	static bool fromJson(ActorRoutineOverriding &overriding, const rapidjson::Value &val);
};

struct SceneAssets {
	struct Entry : public BaseAssets::Entry {
		typedef std::vector<int> Ref;
		typedef std::set<int> UniqueRef;
		typedef std::map<int, unsigned> Revisions;

		Scene::Ptr data = nullptr;
		int refMap = 0; // To a map.
		Ref refActors; // Non-serialized. Editor only. To a number of actors.
		UniqueRef uniqueRefActors; // Non-serialized. Editor only. To a number of actors.
		std::string name;
		int magnification = -1; // Non-polluting.
		scene_t definition;
		ActorRoutineOverriding::Array actorRoutineOverridings;

		unsigned mapRevision = 0; // Non-serialized.
		Revisions actorRevisions; // Non-serialized.
		MapAssets::Getter getMap = nullptr; // Non-serialized. Foreign.
		ActorAssets::Getter getActor = nullptr; // Non-serialized. Foreign.

		Entry(int refMap_, MapAssets::Getter getmap, ActorAssets::Getter getact, Texture::Ptr propstex, Texture::Ptr actorstex);
		Entry(const std::string &val, MapAssets::Getter getmap, ActorAssets::Getter getact, Texture::Ptr propstex, Texture::Ptr actorstex);

		size_t hash(void) const;
		int compare(const Entry &other) const;

		bool has16x16PlayerActor(ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour) const;

		Ref getRefActors(UniqueRef* uref /* nullable */) const;
		int updateRefActors(int removedIndex);

		bool isRefMapRevisionChanged(const BaseAssets::Versionable* versionable) const;
		bool isRefActorRevisionChanged(int ref, const BaseAssets::Versionable* versionable) const;
		void synchronizeRefMapRevision(BaseAssets::Versionable* versionable);
		void synchronizeRefActorRevision(int ref, BaseAssets::Versionable* versionable);

		void touch(void);
		void cleanup(void);
		void cleanup(const Math::Recti &area);
		void reload(void);
		bool refresh(void);

		int estimateFinalByteCount(void) const;

		bool serializeBasic(std::string &val, int page, bool loading) const;
		bool serializeBasicForTriggerCallback(std::string &val, int index) const;

		bool serializeDataSequence(std::string &val, int base, int layer) const;
		bool parseDataSequence(Map::Ptr &map, const std::string &val, int layer) const;

		bool serializeJson(std::string &val, bool pretty) const;
		bool parseJson(Scene::Ptr &scene, scene_t &def, ActorRoutineOverriding::Array &actorRoutines, const std::string &val, ParsingStatuses &status) const;

		bool toString(std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */) const;
		bool fromString(const std::string &val, WarningOrErrorHandler onWarningOrError /* nullable */);
		bool fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError /* nullable */);
	};
	typedef std::vector<Entry> Array;

	Array entries;

	bool empty(void) const;
	int count(void) const;
	bool add(const Entry &entry);
	bool remove(int index);
	const Entry* get(int index) const;
	Entry* get(int index);
	const Entry* find(const std::string &name, int* index /* nullable */) const;
	const Entry* fuzzy(const std::string &name, int* index /* nullable */, std::string &gotName) const;
	int indexOf(const std::string &name) const;
};

/* ===========================================================================} */

/*
** {===========================================================================
** Assets bundle
*/

struct AssetsBundle {
	typedef std::shared_ptr<AssetsBundle> Ptr;
	typedef std::shared_ptr<const AssetsBundle> ConstPtr;

	enum class Categories : unsigned { // Ordered, can be used to sort assets by their referencing dependency.
		NONE    = (unsigned)(~0),
		PALETTE = 0,
		FONT,
		CODE,
		TILES,                         // References to `PALETTE`.
		MAP,                           // References to `TILES`.
		MUSIC,
		SFX,
		ACTOR,
			PROJECTILE,
		SCENE,                         // References to `MAP` and `ACTOR`.
			TRIGGER
	};

	typedef std::function<FontAssets::Entry(const FontAssets::Entry*)> FontGetter;
	typedef std::function<std::string(const CodeAssets::Entry*)> CodeGetter;

	PaletteAssets palette;
	FontAssets fonts;
	CodeAssets code;
	TilesAssets tiles;
	MapAssets maps;
	MusicAssets music;
	SfxAssets sfx;
	ActorAssets actors;
	SceneAssets scenes;

	AssetsBundle();
	AssetsBundle(const AssetsBundle &) = delete;
	~AssetsBundle();

	AssetsBundle &operator = (const AssetsBundle &) = delete;

	void clone(AssetsBundle* other, FontGetter getFont, CodeGetter getCode) const;
	void clone(CodeAssets* other, CodeGetter getCode) const;

	static std::string nameOf(Categories category);
};

/* ===========================================================================} */

#endif /* __ASSETS_H__ */
