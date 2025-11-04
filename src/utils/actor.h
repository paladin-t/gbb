/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "../gbbasic.h"
#include "image.h"
#include "texture.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ACTOR_PALETTE_DEPTH
#	define ACTOR_PALETTE_DEPTH 8
#endif /* ACTOR_PALETTE_DEPTH */

/* ===========================================================================} */

/*
** {===========================================================================
** Actor
*/

/**
 * @brief Actor resource object.
 */
class Actor : public Cloneable<Actor>, public virtual Object {
public:
	typedef std::shared_ptr<Actor> Ptr;

	enum class Compactors {
		NORMAL,
		NON_BLANK
	};

	// A tile slice of an actor. This is the underneath structure to store tile
	// data.
	struct Slice {
		typedef std::vector<Slice> Array;

		size_t hash = 0;
		Image::Ptr image = nullptr;

		Math::Vec2i offset; // Not compared.

		Slice();
		Slice(Image::Ptr img, const Math::Vec2i &off);

		bool operator == (const Slice &other) const;
		bool operator != (const Slice &other) const;

		bool equals(const Slice &other) const;
	};
	// A shadow of a frame, which is constructed with slices instead of actual
	// frame data. These referenced slices do not contain identical ones.
	struct Shadow {
		typedef std::vector<Shadow> Array;
		struct Ref {
			typedef std::vector<Ref> Array;

			int index = -1; // Index to an actor's slice.
			Math::Vec2i offset;
			Math::Vec2i step;
			int properties = 0;

			Ref();
			Ref(int idx, const Math::Vec2i &off, const Math::Vec2i &s, int props);

			bool operator == (const Ref &other) const;
			bool operator != (const Ref &other) const;

			bool equals(const Ref &other) const;
		};

		Ref::Array refs;

		Shadow();

		bool operator == (const Shadow &other) const;
		bool operator != (const Shadow &other) const;

		bool equals(const Shadow &other) const;
	};
	// An actor's animation that indices to its shadow (frame).
	typedef std::vector<int> Animation;

	// An actor's actual frame structure for persistence and editing. Different
	// frames of an actor could have identical data.
	struct Frame {
		typedef std::vector<Frame> Array; // A collection of frames.

		typedef std::function<Colour(int, int, Byte)> PaletteResolver;

		Image::Ptr pixels = nullptr;
		Texture::Ptr texture = nullptr;
		Image::Ptr properties = nullptr;
		Math::Vec2i anchor;
		Slice::Array slices; // Slices on this frame.

		Frame();
		Frame(Image::Ptr img, Image::Ptr props, const Math::Vec2i &anchor);
		Frame(const Frame &otehr);

		bool operator < (const Frame &other) const;
		bool operator > (const Frame &other) const;

		size_t hash(void) const;
		int compare(const Frame &other) const;

		int width(void) const; // In pixels.
		int height(void) const; // In pixels.

		Math::Vec2i count(const Actor* actor) const; // In tiles.
		Math::Vec2i count(const Math::Vec2i &tileSize) const; // In tiles.

		bool get(int x, int y, int &index) const;
		bool set(int x, int y, int index);

		bool getProperty(int x, int y, int &index) const; // `x` and `y` are in tiles.
		bool setProperty(int x, int y, int index); // `x` and `y` are in tiles.

		Math::Recti computeContentRect(void) const;

		void cleanup(void);
		bool touch(Renderer* rnd);
		bool touch(Renderer* rnd, PaletteResolver resolve);
		// Slice this frame into slice pieces. Different slices could have identical data.
		int slice(const Actor* actor);
	};

public:
	GBBASIC_CLASS_TYPE('A', 'C', 'T', 'A')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Actor** ptr, bool represented) const = 0;
	using Cloneable<Actor>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const Actor* other) const = 0;

	virtual int cleanup(void) = 0;
	virtual int cleanup(int frame) = 0;

	virtual bool is8x16(void) const = 0;
	virtual void is8x16(bool val) = 0;

	virtual Compactors compactor(void) const = 0;
	virtual void compactor(Compactors c) = 0;

	virtual int count(void) const = 0;

	virtual const Frame* get(int idx) const = 0;
	virtual Frame* get(int idx) = 0;
	virtual bool set(int idx, const Frame &frame) = 0;
	virtual Frame* add(Image::Ptr img, Image::Ptr props, const Math::Vec2i &anchor) = 0;
	virtual bool insert(int index, Image::Ptr img, Image::Ptr props, const Math::Vec2i &anchor) = 0;
	/**
	 * @param[out] img
	 */
	virtual bool remove(int index, Image::Ptr* img /* nullable */, Image::Ptr* props /* nullable */) = 0;

	virtual bool touch(Renderer* rnd, int idx) = 0;
	virtual bool touch(Renderer* rnd, int idx, Frame::PaletteResolver resolve) = 0;

	virtual bool computeSafeContentRect(Math::Recti &rect) const = 0;

	/**
	 * @brief Slices the specific frame into slice pieces.
	 */
	virtual int slice(int idx) = 0;

	/**
	 * @brief Compacts the current actor object to slices and corresponding re-organized frames.
	 *
	 * @param[out] animation
	 * @param[out] shadow
	 * @param[out] slices Does not contain identical ones.
	 * @param[out] fullImage
	 */
	virtual bool compact(Animation &animation, Shadow::Array &shadow, Slice::Array &slices, Image* fullImage /* nullable */) const = 0;

	virtual bool load(bool is8x16_) = 0;
	virtual void unload(void) = 0;

	virtual const std::string &updateRoutine(void) const = 0;
	virtual void updateRoutine(const std::string &val) = 0;
	virtual const std::string &onHitsRoutine(void) const = 0;
	virtual void onHitsRoutine(const std::string &val) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toC(std::string &val, const char* name /* nullable */) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(Indexed::Ptr palette, const rapidjson::Value &val) = 0;
	virtual bool fromJson(Indexed::Ptr palette, const rapidjson::Document &val) = 0;

	static Actor* create(void);
	static void destroy(Actor* ptr);
};

/* ===========================================================================} */

#endif /* __ACTOR_H__ */
