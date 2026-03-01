/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_map.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Map commands
*/

namespace Commands {

namespace Map {

Pencil::Pencil() {
}

Pencil::~Pencil() {
}

Command* Pencil::create(void) {
	Pencil* result = new Pencil();

	return result;
}

void Pencil::destroy(Command* ptr) {
	Pencil* impl = static_cast<Pencil*>(ptr);
	delete impl;
}

Line::Line() {
}

Line::~Line() {
}

Command* Line::create(void) {
	Line* result = new Line();

	return result;
}

void Line::destroy(Command* ptr) {
	Line* impl = static_cast<Line*>(ptr);
	delete impl;
}

Box::Box() {
}

Box::~Box() {
}

Command* Box::create(void) {
	Box* result = new Box();

	return result;
}

void Box::destroy(Command* ptr) {
	Box* impl = static_cast<Box*>(ptr);
	delete impl;
}

BoxFill::BoxFill() {
}

BoxFill::~BoxFill() {
}

Command* BoxFill::create(void) {
	BoxFill* result = new BoxFill();

	return result;
}

void BoxFill::destroy(Command* ptr) {
	BoxFill* impl = static_cast<BoxFill*>(ptr);
	delete impl;
}

Ellipse::Ellipse() {
}

Ellipse::~Ellipse() {
}

Command* Ellipse::create(void) {
	Ellipse* result = new Ellipse();

	return result;
}

void Ellipse::destroy(Command* ptr) {
	Ellipse* impl = static_cast<Ellipse*>(ptr);
	delete impl;
}

EllipseFill::EllipseFill() {
}

EllipseFill::~EllipseFill() {
}

Command* EllipseFill::create(void) {
	EllipseFill* result = new EllipseFill();

	return result;
}

void EllipseFill::destroy(Command* ptr) {
	EllipseFill* impl = static_cast<EllipseFill*>(ptr);
	delete impl;
}

Fill::Fill() {
}

Fill::~Fill() {
}

Command* Fill::create(void) {
	Fill* result = new Fill();

	return result;
}

void Fill::destroy(Command* ptr) {
	Fill* impl = static_cast<Fill*>(ptr);
	delete impl;
}

Replace::Replace() {
}

Replace::~Replace() {
}

Command* Replace::create(void) {
	Replace* result = new Replace();

	return result;
}

void Replace::destroy(Command* ptr) {
	Replace* impl = static_cast<Replace*>(ptr);
	delete impl;
}

Stamp::Stamp() {
}

Stamp::~Stamp() {
}

Command* Stamp::create(void) {
	Stamp* result = new Stamp();

	return result;
}

void Stamp::destroy(Command* ptr) {
	Stamp* impl = static_cast<Stamp*>(ptr);
	delete impl;
}

Rotate::Rotate() {
}

Rotate::~Rotate() {
}

Command* Rotate::create(void) {
	Rotate* result = new Rotate();

	return result;
}

void Rotate::destroy(Command* ptr) {
	Rotate* impl = static_cast<Rotate*>(ptr);
	delete impl;
}

Flip::Flip() {
}

Flip::~Flip() {
}

Command* Flip::create(void) {
	Flip* result = new Flip();

	return result;
}

void Flip::destroy(Command* ptr) {
	Flip* impl = static_cast<Flip*>(ptr);
	delete impl;
}

Cut::Cut() {
}

Cut::~Cut() {
}

Command* Cut::create(void) {
	Cut* result = new Cut();

	return result;
}

void Cut::destroy(Command* ptr) {
	Cut* impl = static_cast<Cut*>(ptr);
	delete impl;
}

Paste::Paste() {
}

Paste::~Paste() {
}

Command* Paste::create(void) {
	Paste* result = new Paste();

	return result;
}

void Paste::destroy(Command* ptr) {
	Paste* impl = static_cast<Paste*>(ptr);
	delete impl;
}

Delete::Delete() {
}

Delete::~Delete() {
}

Command* Delete::create(void) {
	Delete* result = new Delete();

	return result;
}

void Delete::destroy(Command* ptr) {
	Delete* impl = static_cast<Delete*>(ptr);
	delete impl;
}

SetLocalPaletteEnabled::SetLocalPaletteEnabled() {
	enabled(false);
	old(false);
}

SetLocalPaletteEnabled::~SetLocalPaletteEnabled() {
}

unsigned SetLocalPaletteEnabled::type(void) const {
	return TYPE();
}

const char* SetLocalPaletteEnabled::toString(void) const {
	return "Set local palette enabled";
}

Command* SetLocalPaletteEnabled::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	old(entry->localPaletteEnabled);
	oldPalette(entry->localPalette);
	entry->localPaletteEnabled = enabled();
	entry->localPalette = palette();

	return this;
}

Command* SetLocalPaletteEnabled::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	entry->localPaletteEnabled = old();
	entry->localPalette = oldPalette();

	return this;
}

SetLocalPaletteEnabled* SetLocalPaletteEnabled::with(bool val, const PaletteAssets::Array &pal) {
	enabled(val);
	palette(pal);

	return this;
}

Command* SetLocalPaletteEnabled::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetLocalPaletteEnabled::create(void) {
	SetLocalPaletteEnabled* result = new SetLocalPaletteEnabled();

	return result;
}

void SetLocalPaletteEnabled::destroy(Command* ptr) {
	SetLocalPaletteEnabled* impl = static_cast<SetLocalPaletteEnabled*>(ptr);
	delete impl;
}

SetLocalPalette::SetLocalPalette() {
}

SetLocalPalette::~SetLocalPalette() {
}

unsigned SetLocalPalette::type(void) const {
	return TYPE();
}

const char* SetLocalPalette::toString(void) const {
	return "Set local palette";
}

Command* SetLocalPalette::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	old(entry->localPalette);
	entry->localPalette = palette();

	return this;
}

Command* SetLocalPalette::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	entry->localPalette = old();

	return this;
}

SetLocalPalette* SetLocalPalette::with(const PaletteAssets::Array &pal) {
	palette(pal);

	return this;
}

Command* SetLocalPalette::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

bool SetLocalPalette::isSimilarTo(const Command* other) const {
	if (other->type() != TYPE())
		return false;

	const SetLocalPalette* other_ = Command::as<SetLocalPalette>(other);
	(void)other_;

	return true;
}

bool SetLocalPalette::mergeWith(const Command* other) {
	if (other->type() != TYPE())
		return false;

	const SetLocalPalette* other_ = Command::as<SetLocalPalette>(other);

	palette(other_->palette());

	return true;
}

Command* SetLocalPalette::create(void) {
	SetLocalPalette* result = new SetLocalPalette();

	return result;
}

void SetLocalPalette::destroy(Command* ptr) {
	SetLocalPalette* impl = static_cast<SetLocalPalette*>(ptr);
	delete impl;
}

SetName::SetName() {
}

SetName::~SetName() {
}

unsigned SetName::type(void) const {
	return TYPE();
}

const char* SetName::toString(void) const {
	return "Set name";
}

Command* SetName::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	old(entry->name);
	entry->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	entry->name = old();

	return this;
}

SetName* SetName::with(const std::string &n) {
	name(n);

	return this;
}

Command* SetName::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetName::create(void) {
	SetName* result = new SetName();

	return result;
}

void SetName::destroy(Command* ptr) {
	SetName* impl = static_cast<SetName*>(ptr);
	delete impl;
}

SwitchLayer::SwitchLayer() {
	enabled(false);

	old(false);
}

SwitchLayer::~SwitchLayer() {
}

unsigned SwitchLayer::type(void) const {
	return TYPE();
}

const char* SwitchLayer::toString(void) const {
	return "Toggle layer";
}

Command* SwitchLayer::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	setLayerEnabled()(sub(), enabled());

	return this;
}

Command* SwitchLayer::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	setLayerEnabled()(sub(), old());

	return this;
}

SwitchLayer* SwitchLayer::with(LayerEnabledSetter set) {
	setLayerEnabled(set);

	return this;
}

SwitchLayer* SwitchLayer::with(bool enabled_) {
	enabled(enabled_);
	old(!enabled_);

	return this;
}

Command* SwitchLayer::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SwitchLayer::create(void) {
	SwitchLayer* result = new SwitchLayer();

	return result;
}

void SwitchLayer::destroy(Command* ptr) {
	SwitchLayer* impl = static_cast<SwitchLayer*>(ptr);
	delete impl;
}

ToggleMask::ToggleMask() {
}

ToggleMask::~ToggleMask() {
}

unsigned ToggleMask::type(void) const {
	return TYPE();
}

Command* ToggleMask::create(void) {
	ToggleMask* result = new ToggleMask();

	return result;
}

void ToggleMask::destroy(Command* ptr) {
	ToggleMask* impl = static_cast<ToggleMask*>(ptr);
	delete impl;
}

Resize::Resize() {
	bytes(0);
	attributesBytes(0);
}

Resize::~Resize() {
}

unsigned Resize::type(void) const {
	return TYPE();
}

const char* Resize::toString(void) const {
	return "Resize";
}

Command* Resize::redo(Object::Ptr obj, int argc, const Variant* argv) {
	auto redo_ = [] (int &bytes, Bytes::Ptr &old, ::Map::Ptr map_, const Math::Vec2i size) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			rapidjson::Document doc;
			map_->toJson(doc);
			std::string str;
			Json::toString(doc, str, false);
			tmp->writeString(str);
			bytes = (int)tmp->count();
			int n = LZ4_compressBound((int)tmp->count());
			old->resize((size_t)n);
			n = LZ4_compress_default(
				(const char*)tmp->pointer(), (char*)old->pointer(),
				(int)tmp->count(), (int)old->count()
			);
			GBBASIC_ASSERT(n);
			if (n < (int)tmp->count()) {
				old->resize((size_t)n);
			} else {
				bytes = 0;
				old->clear();
				old->writeBytes(tmp.get());
			}
		}
		map_->resize(size.x, size.y);
	};

	Layered::Layered::redo(obj, argc, argv);

	::Map::Ptr map_ = Object::as<::Map::Ptr>(obj);
	Object::Ptr arg1 = unpack<Object::Ptr>(argc, argv, 1, nullptr);
	::Map::Ptr attrib_ = Object::as<::Map::Ptr>(arg1);

	redo_(bytes(), old(), map_, size());
	redo_(attributesBytes(), oldAttributes(), attrib_, size());

	return this;
}

Command* Resize::undo(Object::Ptr obj, int argc, const Variant* argv) {
	auto undo_ = [] (int bytes, Bytes::Ptr old, ::Map::Ptr map_) -> void {
		if (bytes) {
			Bytes::Ptr tmp(Bytes::create());
			tmp->resize(bytes);
			const int n = LZ4_decompress_safe(
				(const char*)old->pointer(), (char*)tmp->pointer(),
				(int)old->count(), (int)tmp->count()
			);
			(void)n;
			GBBASIC_ASSERT(n == (int)bytes);
			Texture::Ptr texture = nullptr;
			::Map::Tiles tiles;
			if (map_->tiles(tiles))
				texture = tiles.texture;
			std::string str;
			tmp->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			map_->fromJson(doc, texture);
		} else {
			Texture::Ptr texture = nullptr;
			::Map::Tiles tiles;
			if (map_->tiles(tiles))
				texture = tiles.texture;
			std::string str;
			old->poke(0);
			old->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			map_->fromJson(doc, texture);
		}
	};

	Layered::Layered::undo(obj, argc, argv);

	::Map::Ptr map_ = Object::as<::Map::Ptr>(obj);
	Object::Ptr arg1 = unpack<Object::Ptr>(argc, argv, 1, nullptr);
	::Map::Ptr attrib_ = Object::as<::Map::Ptr>(arg1);

	undo_(bytes(), old(), map_);
	undo_(attributesBytes(), oldAttributes(), attrib_);

	return this;
}

Resize* Resize::with(const Math::Vec2i &size_) {
	size(size_);

	return this;
}

Command* Resize::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Resize::create(void) {
	Resize* result = new Resize();

	return result;
}

void Resize::destroy(Command* ptr) {
	Resize* impl = static_cast<Resize*>(ptr);
	delete impl;
}

SetOptimized::SetOptimized() {
	optimized(false);

	old(false);
}

SetOptimized::~SetOptimized() {
}

unsigned SetOptimized::type(void) const {
	return TYPE();
}

const char* SetOptimized::toString(void) const {
	return "Set optimized";
}

Command* SetOptimized::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	old(entry->optimize);
	entry->optimize = optimized();

	return this;
}

Command* SetOptimized::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Map::Ptr ptr = Object::as<::Map::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);

	entry->optimize = old();

	return this;
}

SetOptimized* SetOptimized::with(bool optimized_) {
	optimized(optimized_);

	return this;
}

Command* SetOptimized::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetOptimized::create(void) {
	SetOptimized* result = new SetOptimized();

	return result;
}

void SetOptimized::destroy(Command* ptr) {
	SetOptimized* impl = static_cast<SetOptimized*>(ptr);
	delete impl;
}

Import::Import() {
	hasAttributes(false);
	oldHasAttributesFilled(false);
	oldHasAttributes(false);
	bytes(0);
	attributesBytes(0);
}

Import::~Import() {
}

unsigned Import::type(void) const {
	return TYPE();
}

const char* Import::toString(void) const {
	return "Import";
}

Command* Import::redo(Object::Ptr obj, int argc, const Variant* argv) {
	auto redo_ = [] (::Map::Ptr map, int &bytes, Bytes::Ptr &old, ::Map::Ptr map_) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			rapidjson::Document doc;
			map_->toJson(doc);
			std::string str;
			Json::toString(doc, str, false);
			tmp->writeString(str);
			bytes = (int)tmp->count();
			int n = LZ4_compressBound((int)tmp->count());
			old->resize((size_t)n);
			n = LZ4_compress_default(
				(const char*)tmp->pointer(), (char*)old->pointer(),
				(int)tmp->count(), (int)old->count()
			);
			GBBASIC_ASSERT(n);
			if (n < (int)tmp->count()) {
				old->resize((size_t)n);
			} else {
				bytes = 0;
				old->clear();
				old->writeBytes(tmp.get());
			}
		}
		map_->resize(map->width(), map->height());
		rapidjson::Document doc;
		map->toJson(doc);
		::Map::Tiles tiles;
		map_->tiles(tiles);
		map_->fromJson(doc, tiles.texture);
	};

	Layered::Layered::redo(obj, argc, argv);

	::Map::Ptr map_ = Object::as<::Map::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);
	Object::Ptr arg1 = unpack<Object::Ptr>(argc, argv, 1, nullptr);
	::Map::Ptr attrib_ = Object::as<::Map::Ptr>(arg1);

	if (map())
		redo_(map(), bytes(), old(), map_);
	if (!oldHasAttributesFilled()) {
		oldHasAttributes(entry->hasAttributes);
		oldHasAttributesFilled(true);
	}
	entry->hasAttributes = hasAttributes();
	if (attributes())
		redo_(attributes(), attributesBytes(), oldAttributes(), attrib_);

	return this;
}

Command* Import::undo(Object::Ptr obj, int argc, const Variant* argv) {
	auto undo_ = [] (::Map::Ptr map, int bytes, Bytes::Ptr old, ::Map::Ptr map_) -> void {
		(void)map;

		if (bytes) {
			Bytes::Ptr tmp(Bytes::create());
			tmp->resize(bytes);
			const int n = LZ4_decompress_safe(
				(const char*)old->pointer(), (char*)tmp->pointer(),
				(int)old->count(), (int)tmp->count()
			);
			(void)n;
			GBBASIC_ASSERT(n == (int)bytes);
			Texture::Ptr texture = nullptr;
			::Map::Tiles tiles;
			if (map_->tiles(tiles))
				texture = tiles.texture;
			std::string str;
			tmp->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			map_->fromJson(doc, texture);
		} else {
			Texture::Ptr texture = nullptr;
			::Map::Tiles tiles;
			if (map_->tiles(tiles))
				texture = tiles.texture;
			std::string str;
			old->poke(0);
			old->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			map_->fromJson(doc, texture);
		}
	};

	Layered::Layered::undo(obj, argc, argv);

	::Map::Ptr map_ = Object::as<::Map::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	MapAssets::Entry* entry = (MapAssets::Entry*)(arg0);
	Object::Ptr arg1 = unpack<Object::Ptr>(argc, argv, 1, nullptr);
	::Map::Ptr attrib_ = Object::as<::Map::Ptr>(arg1);

	if (map())
		undo_(map(), bytes(), old(), map_);
	entry->hasAttributes = oldHasAttributes();
	if (attributes())
		undo_(attributes(), attributesBytes(), oldAttributes(), attrib_);

	return this;
}

Import* Import::with(const ::Map::Ptr &map_, bool hasAttrib, const ::Map::Ptr &attrib) {
	hasAttributes(hasAttrib);
	map(map_);
	attributes(attrib);

	return this;
}

Import* Import::with(const ::Map::Ptr &data, int layer) {
	if (layer == ASSETS_MAP_GRAPHICS_LAYER)
		map(data);
	else /* if (layer == ASSETS_MAP_ATTRIBUTES_LAYER) */
		attributes(data);

	return this;
}

Command* Import::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Import::create(void) {
	Import* result = new Import();

	return result;
}

void Import::destroy(Command* ptr) {
	Import* impl = static_cast<Import*>(ptr);
	delete impl;
}

BeginGroup::BeginGroup() {
}

BeginGroup::~BeginGroup() {
}

unsigned BeginGroup::type(void) const {
	return TYPE();
}

const char* BeginGroup::toString(void) const {
	return name().c_str();
}

Command* BeginGroup::create(void) {
	BeginGroup* result = new BeginGroup();

	return result;
}

void BeginGroup::destroy(Command* ptr) {
	BeginGroup* impl = static_cast<BeginGroup*>(ptr);
	delete impl;
}

EndGroup::EndGroup() {
}

EndGroup::~EndGroup() {
}

unsigned EndGroup::type(void) const {
	return TYPE();
}

const char* EndGroup::toString(void) const {
	return name().c_str();
}

Command* EndGroup::create(void) {
	EndGroup* result = new EndGroup();

	return result;
}

void EndGroup::destroy(Command* ptr) {
	EndGroup* impl = static_cast<EndGroup*>(ptr);
	delete impl;
}

Reference::Reference() {
}

Reference::~Reference() {
}

unsigned Reference::type(void) const {
	return TYPE();
}

const char* Reference::toString(void) const {
	return "Reference";
}

Command* Reference::create(void) {
	Reference* result = new Reference();

	return result;
}

void Reference::destroy(Command* ptr) {
	Reference* impl = static_cast<Reference*>(ptr);
	delete impl;
}

namespace AsImage {

// TODO: EDIT MAP AS IMAGE

}

}

}

/* ===========================================================================} */
