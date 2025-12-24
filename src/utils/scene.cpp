/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "scene.h"

/*
** {===========================================================================
** Scene
*/

class SceneImpl : public Scene {
private:
	Layers _layers;
	int _width = 0;
	int _height = 0;

public:
	SceneImpl() {
	}
	virtual ~SceneImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Scene** ptr, bool represented) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		SceneImpl* result = static_cast<SceneImpl*>(Scene::create());
		Map* map = nullptr;
		if (_layers.map) {
			_layers.map->clone(&map, represented);
			result->_layers.map = Map::Ptr(map);
		}
		if (_layers.attributes) {
			_layers.attributes->clone(&map, represented);
			result->_layers.attributes = Map::Ptr(map);
		}
		if (_layers.properties) {
			_layers.properties->clone(&map, represented);
			result->_layers.properties = Map::Ptr(map);
		}
		if (_layers.actors) {
			_layers.actors->clone(&map, represented);
			result->_layers.actors = Map::Ptr(map);
		}
		result->_layers.triggers = _layers.triggers;
		result->_layers.hasAttributes = _layers.hasAttributes;
		result->_layers.hasProperties = _layers.hasProperties;
		result->_layers.hasActors = _layers.hasActors;
		result->_layers.hasTriggers = _layers.hasTriggers;
		result->_width = _width;
		result->_height = _height;

		*ptr = result;

		return true;
	}
	virtual bool clone(Scene** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Scene* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		if (_layers.map)
			result = Math::hash(result, _layers.map->hash());

		if (_layers.attributes)
			result = Math::hash(result, _layers.attributes->hash());

		if (_layers.properties)
			result = Math::hash(result, _layers.properties->hash());

		if (_layers.actors)
			result = Math::hash(result, _layers.actors->hash());

		for (const Trigger &trigger : _layers.triggers)
			result = Math::hash(result, trigger.x0, trigger.y0, trigger.x1, trigger.y1);

		result = Math::hash(
			result,
			_layers.hasAttributes,
			_layers.hasProperties,
			_layers.hasActors,
			_layers.hasTriggers
		);

		result = Math::hash(result, _width, _height);

		return result;
	}
	virtual int compare(const Scene* other) const override {
		const SceneImpl* implOther = static_cast<const SceneImpl*>(other);

		if (this == other)
			return 0;

		if (!other)
			return 1;

		if (!_layers.map && implOther->_layers.map)
			return -1;
		else if (_layers.map && !implOther->_layers.map)
			return 1;

		if (_layers.map && implOther->_layers.map) {
			const int ret = _layers.map->compare(implOther->_layers.map.get());
			if (ret)
				return ret;
		}

		if (!_layers.attributes && implOther->_layers.attributes)
			return -1;
		else if (_layers.attributes && !implOther->_layers.attributes)
			return 1;

		if (_layers.attributes && implOther->_layers.attributes) {
			const int ret = _layers.attributes->compare(implOther->_layers.attributes.get());
			if (ret)
				return ret;
		}

		if (!_layers.properties && implOther->_layers.properties)
			return -1;
		else if (_layers.properties && !implOther->_layers.properties)
			return 1;

		if (_layers.properties && implOther->_layers.properties) {
			const int ret = _layers.properties->compare(implOther->_layers.properties.get());
			if (ret)
				return ret;
		}

		if (!_layers.actors && implOther->_layers.actors)
			return -1;
		else if (_layers.actors && !implOther->_layers.actors)
			return 1;

		if (_layers.actors && implOther->_layers.actors) {
			const int ret = _layers.actors->compare(implOther->_layers.actors.get());
			if (ret)
				return ret;
		}

		if (_layers.triggers.size() < implOther->_layers.triggers.size())
			return -1;
		else if (_layers.triggers.size() > implOther->_layers.triggers.size())
			return 1;

		for (int i = 0; i < (int)_layers.triggers.size(); ++i) {
			const Trigger &triggerl = _layers.triggers[i];
			const Trigger &triggerr = implOther->_layers.triggers[i];
			if (triggerl < triggerr)
				return -1;
			else if (triggerl > triggerr)
				return 1;
		}

		if (!_layers.hasAttributes && implOther->_layers.hasAttributes)
			return -1;
		else if (_layers.hasAttributes && !implOther->_layers.hasAttributes)
			return 1;

		if (!_layers.hasProperties && implOther->_layers.hasProperties)
			return -1;
		else if (_layers.hasProperties && !implOther->_layers.hasProperties)
			return 1;

		if (!_layers.hasActors && implOther->_layers.hasActors)
			return -1;
		else if (_layers.hasActors && !implOther->_layers.hasActors)
			return 1;

		if (!_layers.hasTriggers && implOther->_layers.hasTriggers)
			return -1;
		else if (_layers.hasTriggers && !implOther->_layers.hasTriggers)
			return 1;

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
		int result = 0;
		if (_layers.map)
			result += _layers.map->cleanup();
		if (_layers.attributes)
			result += _layers.attributes->cleanup();
		if (_layers.properties)
			result += _layers.properties->cleanup();
		if (_layers.actors)
			result += _layers.actors->cleanup();

		return result;
	}

	virtual Map::Ptr mapLayer(void) const override {
		return _layers.map;
	}
	virtual Map::Ptr attributeLayer(void) const override {
		return _layers.attributes;
	}
	virtual Map::Ptr propertyLayer(void) const override {
		return _layers.properties;
	}
	virtual Map::Ptr actorLayer(void) const override {
		return _layers.actors;
	}
	virtual const Trigger::Array* triggerLayer(void) const override {
		return &_layers.triggers;
	}
	virtual Trigger::Array* triggerLayer(void) override {
		return &_layers.triggers;
	}

	virtual bool hasAttributes(void) const override {
		return _layers.hasAttributes;
	}
	virtual void hasAttributes(bool val) override {
		_layers.hasAttributes = val;
	}
	virtual bool hasProperties(void) const override {
		return _layers.hasProperties;
	}
	virtual void hasProperties(bool val) override {
		_layers.hasProperties = val;
	}
	virtual bool hasActors(void) const override {
		return _layers.hasActors;
	}
	virtual void hasActors(bool val) override {
		_layers.hasActors = val;
	}
	virtual bool hasTriggers(void) const override {
		return _layers.hasTriggers;
	}
	virtual void hasTriggers(bool val) override {
		_layers.hasTriggers = val;
	}

	virtual int width(void) const override {
		return _width;
	}
	virtual int height(void) const override {
		return _height;
	}
	virtual bool resized(void) override {
		int w = -1;
		int h = -1;
		if (_layers.map) {
			w = _layers.map->width();
			h = _layers.map->height();
		}
		if (_layers.attributes) {
			if (w == -1 && h == -1) {
				w = _layers.attributes->width();
				h = _layers.attributes->height();
			} else {
				if (w != _layers.attributes->width())
					return false;
				if (h != _layers.attributes->height())
					return false;
			}
		}
		if (_layers.properties) {
			if (w == -1 && h == -1) {
				w = _layers.properties->width();
				h = _layers.properties->height();
			} else {
				if (w != _layers.properties->width())
					return false;
				if (h != _layers.properties->height())
					return false;
			}
		}
		if (_layers.actors) {
			if (w == -1 && h == -1) {
				w = _layers.actors->width();
				h = _layers.actors->height();
			} else {
				if (w != _layers.actors->width())
					return false;
				if (h != _layers.actors->height())
					return false;
			}
		}
		for (int i = 0; i < (int)_layers.triggers.size(); ++i) {
			Trigger &trigger = _layers.triggers[i];
			if (trigger.x1 >= w) {
				trigger.x0 += -(trigger.x1 - w + 1);
				trigger.x1 += -(trigger.x1 - w + 1);
			}
			if (trigger.x0 < 0) {
				trigger.x0 += -trigger.x0;
				trigger.x1 += -trigger.x0;
			}
			if (trigger.y1 >= w) {
				trigger.y0 += -(trigger.y1 - w + 1);
				trigger.y1 += -(trigger.y1 - w + 1);
			}
			if (trigger.y0 < 0) {
				trigger.y0 += -trigger.y0;
				trigger.y1 += -trigger.y0;
			}
		}

		_width = w;
		_height = h;

		return true;
	}

	virtual bool load(int width, int height, Map::Ptr map, Map::Ptr attrib, Map::Ptr props, Map::Ptr actors, const Trigger::Array &triggers) override {
		_layers.clear(true, true);
		_width = width;
		_height = height;

		_layers.map = map;
		_layers.attributes = attrib;
		_layers.properties = props;
		_layers.actors = actors;
		_layers.triggers = triggers;

		if (!_layers.attributes)
			_layers.hasAttributes = false;
		if (!_layers.properties)
			_layers.hasProperties = false;
		if (!_layers.actors)
			_layers.hasActors = false;
		if (_layers.triggers.empty())
			_layers.hasTriggers = false;

		return true;
	}
	virtual void unload(bool clearmap, bool clearattrib) override {
		_layers.clear(clearmap, clearattrib);
		_width = 0;
		_height = 0;
	}
	virtual void reload(Map::Ptr map, Map::Ptr attrib) override {
		_layers.map = map;
		_layers.attributes = attrib;

		if (!_layers.attributes)
			_layers.hasAttributes = false;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		typedef std::vector<int> Cels;

		auto serialize = [] (rapidjson::Value &val, rapidjson::Document &doc, Map::Ptr ptr) -> void {
			if (!ptr)
				return;

			if (ptr->width() == 0 && ptr->height() == 0)
				return;

			Cels cels(ptr->width() * ptr->height());
			ptr->data(&cels.front(), cels.size());
			for (int cel : cels)
				val.PushBack(cel, doc.GetAllocator());
		};

		val.SetObject();

		rapidjson::Value jstrw, jstrh;
		jstrw.SetString("width", doc.GetAllocator());
		jstrh.SetString("height", doc.GetAllocator());
		rapidjson::Value jvalw, jvalh;
		jvalw.SetInt(_width);
		jvalh.SetInt(_height);
		val.AddMember(jstrw, jvalw, doc.GetAllocator());
		val.AddMember(jstrh, jvalh, doc.GetAllocator());

		rapidjson::Value jstrprops;
		jstrprops.SetString("properties", doc.GetAllocator());
		rapidjson::Value jvalprops;
		jvalprops.SetArray();
		serialize(jvalprops, doc, _layers.properties);
		val.AddMember(jstrprops, jvalprops, doc.GetAllocator());

		rapidjson::Value jstractors;
		jstractors.SetString("actors", doc.GetAllocator());
		rapidjson::Value jvalactors;
		jvalactors.SetArray();
		serialize(jvalactors, doc, _layers.actors);
		val.AddMember(jstractors, jvalactors, doc.GetAllocator());

		rapidjson::Value jstrtrigger;
		jstrtrigger.SetString("triggers", doc.GetAllocator());
		rapidjson::Value jvaltrigger;
		jvaltrigger.SetArray();
		for (int i = 0; i < (int)_layers.triggers.size(); ++i) {
			const Trigger &trigger = _layers.triggers[i];

			rapidjson::Value jtrigger;
			jtrigger.SetObject();

			{
				rapidjson::Value jstrx, jstry, jstrwidth, jstrheight;
				jstrx.SetString("x", doc.GetAllocator());
				jstry.SetString("y", doc.GetAllocator());
				jstrwidth.SetString("width", doc.GetAllocator());
				jstrheight.SetString("height", doc.GetAllocator());
				rapidjson::Value jvalx, jvaly, jvalwidth, jvalheight;
				jvalx.SetInt(trigger.xMin());
				jvaly.SetInt(trigger.yMin());
				jvalwidth.SetInt(trigger.width());
				jvalheight.SetInt(trigger.height());

				rapidjson::Value jstrcol;
				jstrcol.SetString("color", doc.GetAllocator());
				rapidjson::Value jvalcol;
				jvalcol.SetArray();
				const Colour &col = trigger.color;
				jvalcol.PushBack(col.r, doc.GetAllocator());
				jvalcol.PushBack(col.g, doc.GetAllocator());
				jvalcol.PushBack(col.b, doc.GetAllocator());
				jvalcol.PushBack(col.a, doc.GetAllocator());

				jtrigger.AddMember(jstrx, jvalx, doc.GetAllocator());
				jtrigger.AddMember(jstry, jvaly, doc.GetAllocator());
				jtrigger.AddMember(jstrwidth, jvalwidth, doc.GetAllocator());
				jtrigger.AddMember(jstrheight, jvalheight, doc.GetAllocator());
				jtrigger.AddMember(jstrcol, jvalcol, doc.GetAllocator());

				if (trigger.eventType != TRIGGER_HAS_NONE) {
					rapidjson::Value jstrevttype;
					jstrevttype.SetString("event_type", doc.GetAllocator());
					rapidjson::Value jvalevttype;
					jvalevttype.SetInt(trigger.eventType);

					jtrigger.AddMember(jstrevttype, jvalevttype, doc.GetAllocator());
				}

				if (!trigger.eventRoutine.empty()) {
					rapidjson::Value jstrevtroutine;
					jstrevtroutine.SetString("event_routine", doc.GetAllocator());
					rapidjson::Value jvalevtroutine;
					jvalevtroutine.SetString(trigger.eventRoutine.c_str(), doc.GetAllocator());

					jtrigger.AddMember(jstrevtroutine, jvalevtroutine, doc.GetAllocator());
				}
			}

			jvaltrigger.PushBack(jtrigger, doc.GetAllocator());
		}
		val.AddMember(jstrtrigger, jvaltrigger, doc.GetAllocator());

		rapidjson::Value jstrhasattrib;
		jstrhasattrib.SetString("has_attributes", doc.GetAllocator());
		rapidjson::Value jvalhasattrib;
		jvalhasattrib.SetBool(_layers.hasAttributes);
		val.AddMember(jstrhasattrib, jvalhasattrib, doc.GetAllocator());

		rapidjson::Value jstrhasprops;
		jstrhasprops.SetString("has_properties", doc.GetAllocator());
		rapidjson::Value jvalhasprops;
		jvalhasprops.SetBool(_layers.hasProperties);
		val.AddMember(jstrhasprops, jvalhasprops, doc.GetAllocator());

		rapidjson::Value jstrhasactors;
		jstrhasactors.SetString("has_actors", doc.GetAllocator());
		rapidjson::Value jvalhasactors;
		jvalhasactors.SetBool(_layers.hasActors);
		val.AddMember(jstrhasactors, jvalhasactors, doc.GetAllocator());

		rapidjson::Value jstrhastriggers;
		jstrhastriggers.SetString("has_triggers", doc.GetAllocator());
		rapidjson::Value jvalhastriggers;
		jvalhastriggers.SetBool(_layers.hasTriggers);
		val.AddMember(jstrhastriggers, jvalhastriggers, doc.GetAllocator());

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val, Texture::Ptr propstex, Texture::Ptr actorstex) override {
		typedef std::vector<int> Cels;

		auto parse = [] (Cels &cels, const rapidjson::Value &val, int w, int h) -> void {
			cels.clear();
			rapidjson::Value::ConstArray data = val.GetArray();
			for (rapidjson::SizeType i = 0; i < data.Size() && (int)i < w * h; ++i) {
				if (data[i].IsInt())
					cels.push_back(data[i].GetInt());
				else
					cels.push_back(0);
			}
		};

		unload(false, false);

		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jw = val.FindMember("width");
		rapidjson::Value::ConstMemberIterator jh = val.FindMember("height");
		if (jw == val.MemberEnd() || jh == val.MemberEnd())
			return false;
		if (!jw->value.IsInt() || !jh->value.IsInt())
			return false;
		const int mapWidth = jw->value.GetInt();
		const int mapHeight = jh->value.GetInt();

		rapidjson::Value::ConstMemberIterator jprops = val.FindMember("properties");
		if (jprops == val.MemberEnd() || !jprops->value.IsArray())
			return false;

		rapidjson::Value::ConstMemberIterator jactors = val.FindMember("actors");
		if (jactors == val.MemberEnd() || !jactors->value.IsArray())
			return false;

		rapidjson::Value::ConstMemberIterator jtriggers = val.FindMember("triggers");
		if (jtriggers == val.MemberEnd() || !jtriggers->value.IsArray())
			return false;

		rapidjson::Value::ConstMemberIterator jhasattribs = val.FindMember("has_attributes");
		if (jhasattribs == val.MemberEnd() || !jhasattribs->value.IsBool())
			return false;

		rapidjson::Value::ConstMemberIterator jhasprops = val.FindMember("has_properties");
		if (jhasprops == val.MemberEnd() || !jhasprops->value.IsBool())
			return false;

		rapidjson::Value::ConstMemberIterator jhasactors = val.FindMember("has_actors");
		if (jhasactors == val.MemberEnd() || !jhasactors->value.IsBool())
			return false;

		rapidjson::Value::ConstMemberIterator jhastriggers = val.FindMember("has_triggers");
		if (jhastriggers == val.MemberEnd() || !jhastriggers->value.IsBool())
			return false;

		const int propsTileCountX = propstex->width() / GBBASIC_TILE_SIZE;
		const int propsTileCountY = propstex->height() / GBBASIC_TILE_SIZE;
		Map::Tiles propsTiles(propstex, Math::Vec2i(propsTileCountX, propsTileCountY));

		Cels celsprops;
		parse(celsprops, jprops->value, mapWidth, mapHeight);
		Map::Ptr propsptr(Map::create(&propsTiles, true));
		if (celsprops.empty()) {
			if (!propsptr->load(0, mapWidth, mapHeight))
				return false;
		} else {
			if (!propsptr->load(&celsprops.front(), mapWidth, mapHeight))
				return false;
		}

		const int actorsTileCountX = actorstex->width() / GBBASIC_TILE_SIZE;
		const int actorsTileCountY = actorstex->height() / GBBASIC_TILE_SIZE;
		Map::Tiles actorsTiles(actorstex, Math::Vec2i(actorsTileCountX, actorsTileCountY));

		Cels celactors;
		Map::Ptr actorsptr(Map::create(&actorsTiles, true));
		if (jactors->value.IsArray())
			parse(celactors, jactors->value, mapWidth, mapHeight);
		if (celactors.empty()) {
			if (!actorsptr->load(INVALID_ACTOR(), mapWidth, mapHeight))
				return false;
		} else {
			if (!actorsptr->load(&celactors.front(), mapWidth, mapHeight))
				return false;
		}

		Trigger::Array triggers;
		if (jtriggers->value.IsArray()) {
			rapidjson::Value::ConstArray jtriggers_ = jtriggers->value.GetArray();
			for (rapidjson::SizeType i = 0; i < jtriggers_.Size(); ++i) {
				Trigger trigger;

				const rapidjson::Value &jtrigger = jtriggers_[i];
				if (!jtrigger.IsObject())
					continue;

				rapidjson::Value::ConstMemberIterator jx = jtrigger.FindMember("x");
				rapidjson::Value::ConstMemberIterator jy = jtrigger.FindMember("y");
				rapidjson::Value::ConstMemberIterator jwidth = jtrigger.FindMember("width");
				rapidjson::Value::ConstMemberIterator jheight = jtrigger.FindMember("height");
				if (jx == jtrigger.MemberEnd() || jy == jtrigger.MemberEnd() || jwidth == jtrigger.MemberEnd() || jheight == jtrigger.MemberEnd())
					continue;
				if (!jx->value.IsInt() || !jy->value.IsInt() || !jwidth->value.IsInt() || !jheight->value.IsInt())
					continue;

				Colour col;
				bool parsedColor = false;
				rapidjson::Value::ConstMemberIterator jcolor = jtrigger.FindMember("color");
				if (jcolor != jtrigger.MemberEnd() && jcolor->value.IsArray()) {
					const rapidjson::Value &jcol = jcolor->value;
					const rapidjson::Value &c0 = jcol[0];
					const rapidjson::Value &c1 = jcol[1];
					const rapidjson::Value &c2 = jcol[2];
					const rapidjson::Value &c3 = jcol[3];
					if (c0.IsInt() && c1.IsInt() && c2.IsInt() && c3.IsInt()) {
						col = {
							(Byte)c0.GetInt(),
							(Byte)c1.GetInt(),
							(Byte)c2.GetInt(),
							(Byte)c3.GetInt()
						};
						parsedColor = true;
					}
				}
				if (!parsedColor) {
					col = Colour::randomize(255);
				}

				rapidjson::Value::ConstMemberIterator jevttype = jtrigger.FindMember("event_type");
				if (jevttype != jtrigger.MemberEnd() && jevttype->value.IsInt()) {
					trigger.eventType = jevttype->value.GetInt();
				}

				rapidjson::Value::ConstMemberIterator jevtroutine = jtrigger.FindMember("event_routine");
				if (jevtroutine != jtrigger.MemberEnd() && jevtroutine->value.IsString()) {
					trigger.eventRoutine = jevtroutine->value.GetString();
				}

				trigger = Trigger::byXYWH(jx->value.GetInt(), jy->value.GetInt(), jwidth->value.GetInt(), jheight->value.GetInt());
				trigger.color = col;

				triggers.push_back(trigger);
			}
		}

		_width = mapWidth;
		_height = mapHeight;

		_layers.properties = propsptr;
		_layers.actors = actorsptr;
		_layers.triggers = triggers;

		_layers.hasAttributes = jhasattribs->value.GetBool();
		_layers.hasProperties = jhasprops->value.GetBool();
		if (jhasactors->value.IsBool())
			_layers.hasActors = jhasactors->value.GetBool();
		else
			_layers.hasActors = false;
		if (jhastriggers->value.IsBool())
			_layers.hasTriggers = jhastriggers->value.GetBool();
		else
			_layers.hasTriggers = false;

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val, Texture::Ptr propstex, Texture::Ptr actorstex) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval, propstex, actorstex);
	}
};

Scene::Layers::Layers() {
}

void Scene::Layers::clear(bool clearmap, bool clearattrib) {
	if (clearmap)
		map = nullptr;
	if (clearattrib)
		attributes = nullptr;
	properties = nullptr;
	actors = nullptr;
	triggers.clear();

	hasAttributes = false;
	hasProperties = false;
	hasActors = false;
	hasTriggers = false;
}

Byte Scene::INVALID_ACTOR(void) {
	return std::numeric_limits<Byte>::max();
}

Byte Scene::INVALID_TRIGGER(void) {
	return std::numeric_limits<Byte>::max();
}

Scene* Scene::create(void) {
	SceneImpl* result = new SceneImpl();

	return result;
}

void Scene::destroy(Scene* ptr) {
	SceneImpl* impl = static_cast<SceneImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
