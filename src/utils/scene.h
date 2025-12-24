/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __SCENE_H__
#define __SCENE_H__

#include "../gbbasic.h"
#include "map.h"
#include "trigger.h"

/*
** {===========================================================================
** Scene
*/

/**
 * @brief Scene resource object.
 */
class Scene : public Cloneable<Scene>, public virtual Object {
public:
	typedef std::shared_ptr<Scene> Ptr;

	struct Layers {
		Map::Ptr map = nullptr;        // Graphical.
		Map::Ptr attributes = nullptr; // Color palette, etc.
		Map::Ptr properties = nullptr; // Blocking, ladder, etc.
		Map::Ptr actors = nullptr;
		Trigger::Array triggers;

		bool hasAttributes = false;
		bool hasProperties = false;
		bool hasActors = false;
		bool hasTriggers = false;

		Layers();

		void clear(bool clearmap, bool clearattrib);
	};

public:
	GBBASIC_CLASS_TYPE('S', 'C', 'N', 'A')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Scene** ptr, bool represented) const = 0;
	using Cloneable<Scene>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const Scene* other) const = 0;

	virtual int cleanup(void) = 0;

	virtual Map::Ptr mapLayer(void) const = 0;
	virtual Map::Ptr attributeLayer(void) const = 0;
	virtual Map::Ptr propertyLayer(void) const = 0;
	virtual Map::Ptr actorLayer(void) const = 0;
	virtual const Trigger::Array* triggerLayer(void) const = 0;
	virtual Trigger::Array* triggerLayer(void) = 0;

	virtual bool hasAttributes(void) const = 0;
	virtual void hasAttributes(bool val) = 0;
	virtual bool hasProperties(void) const = 0;
	virtual void hasProperties(bool val) = 0;
	virtual bool hasActors(void) const = 0;
	virtual void hasActors(bool val) = 0;
	virtual bool hasTriggers(void) const = 0;
	virtual void hasTriggers(bool val) = 0;

	virtual int width(void) const = 0;
	virtual int height(void) const = 0;
	virtual bool resized(void) = 0;

	virtual bool load(int width, int height, Map::Ptr map, Map::Ptr attrib, Map::Ptr props, Map::Ptr actors, const Trigger::Array &triggers) = 0;
	virtual void unload(bool clearmap, bool clearattrib) = 0;
	virtual void reload(Map::Ptr map, Map::Ptr attrib) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(const rapidjson::Value &val, Texture::Ptr propstex, Texture::Ptr actorstex) = 0;
	virtual bool fromJson(const rapidjson::Document &val, Texture::Ptr propstex, Texture::Ptr actorstex) = 0;

	static Byte INVALID_ACTOR(void);
	static Byte INVALID_TRIGGER(void);

	static Scene* create(void);
	static void destroy(Scene* ptr);
};

/* ===========================================================================} */

#endif /* __SCENE_H__ */
