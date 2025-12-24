/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_actor.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Actor commands
*/

namespace Commands {

namespace Actor {

AddFrame::AddFrame() {
	append(true);
}

AddFrame::~AddFrame() {
}

unsigned AddFrame::type(void) const {
	return TYPE();
}

const char* AddFrame::toString(void) const {
	return "Add frame";
}

Command* AddFrame::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	const int tw = image()->width() / GBBASIC_TILE_SIZE;
	const int th = image()->height() / GBBASIC_TILE_SIZE;
	Math::Vec2i anchor = ActorAssets::computeAnchor(tw, th); // Compute anchor.

	if (append()) {
		const ::Actor::Frame* frame = ptr->count() > 0 ? ptr->get(ptr->count() - 1) : nullptr;
		if (frame)
			anchor = frame->anchor; // Use anchor of the previous frame.

		if (!ptr->add(image(), properties(), anchor))
			return this;

		if (setSub())
			setSub()(ptr->count() - 1);
	} else {
		Layered::Layered::redo(obj, argc, argv);

		const ::Actor::Frame* frame = sub() > 0 ? ptr->get(sub() - 1) : nullptr;
		if (frame)
			anchor = frame->anchor; // Use anchor of the previous frame.

		if (!ptr->insert(sub(), image(), properties(), anchor))
			return this;
	}

	return this;
}

Command* AddFrame::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	Image::Ptr img = nullptr;
	Image::Ptr props = nullptr;

	if (append()) {
		if (!ptr->remove(ptr->count() - 1, &img, &props))
			return this;
	} else {
		if (!ptr->remove(sub(), &img, &props))
			return this;
	}

	return this;
}

AddFrame* AddFrame::with(bool append_, Image::Ptr img, Image::Ptr props) {
	append(append_);
	image(img);
	properties(props);

	return this;
}

Command* AddFrame::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddFrame::create(void) {
	AddFrame* result = new AddFrame();

	return result;
}

void AddFrame::destroy(Command* ptr) {
	AddFrame* impl = static_cast<AddFrame*>(ptr);
	delete impl;
}

CutFrame::CutFrame() {
	append(true);

	filled(false);
}

CutFrame::~CutFrame() {
}

unsigned CutFrame::type(void) const {
	return TYPE();
}

const char* CutFrame::toString(void) const {
	return "Cut frame";
}

Command* CutFrame::redo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	Image::Ptr img = nullptr;
	Image::Ptr props = nullptr;

	if (!filled()) {
		const ::Actor::Frame* frame = ptr->get(sub());
		oldPixels(frame->pixels);
		oldProperties(frame->properties);
		oldAnchor(frame->anchor);
		filled(true);
	}
	if (!ptr->remove(sub(), &img, &props))
		return this;

	return this;
}

Command* CutFrame::undo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	if (!ptr->insert(sub(), nullptr, nullptr, Math::Vec2i(0, 0)))
		return this;

	::Actor::Frame* frame = ptr->get(sub());
	frame->pixels = oldPixels();
	frame->properties = oldProperties();
	frame->anchor = oldAnchor();

	return this;
}

CutFrame* CutFrame::with(bool append_) {
	append(append_);

	return this;
}

Command* CutFrame::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* CutFrame::create(void) {
	CutFrame* result = new CutFrame();

	return result;
}

void CutFrame::destroy(Command* ptr) {
	CutFrame* impl = static_cast<CutFrame*>(ptr);
	delete impl;
}

DeleteFrame::DeleteFrame() {
}

DeleteFrame::~DeleteFrame() {
}

unsigned DeleteFrame::type(void) const {
	return TYPE();
}

const char* DeleteFrame::toString(void) const {
	return "Delete frame";
}

Command* DeleteFrame::create(void) {
	DeleteFrame* result = new DeleteFrame();

	return result;
}

void DeleteFrame::destroy(Command* ptr) {
	DeleteFrame* impl = static_cast<DeleteFrame*>(ptr);
	delete impl;
}

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

Set8x16::Set8x16() {
	eightMulSixteen(false);

	old(false);
}

Set8x16::~Set8x16() {
}

unsigned Set8x16::type(void) const {
	return TYPE();
}

const char* Set8x16::toString(void) const {
	return "Set 8x16";
}

Command* Set8x16::redo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	old(ptr->is8x16());
	ptr->is8x16(eightMulSixteen());

	return this;
}

Command* Set8x16::undo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	ptr->is8x16(old());

	return this;
}

Set8x16* Set8x16::with(bool eightMulSixteen_) {
	eightMulSixteen(eightMulSixteen_);

	return this;
}

Command* Set8x16::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Set8x16::create(void) {
	Set8x16* result = new Set8x16();

	return result;
}

void Set8x16::destroy(Command* ptr) {
	Set8x16* impl = static_cast<Set8x16*>(ptr);
	delete impl;
}

SetProperties::SetProperties() {
}

SetProperties::~SetProperties() {
}

unsigned SetProperties::type(void) const {
	return TYPE();
}

const char* SetProperties::toString(void) const {
	return "Set properties";
}

Paintable::Paint* SetProperties::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);
	penSize(1);

	return this;
}

Paintable::Paint* SetProperties::with(const Math::Vec2i &validSize, const Points &points, const Indices &values) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	for (int i = 0; i < (int)points.size(); ++i) {
		const Math::Vec2i &p = points[i];
		const int idx = values[i];
		if (!valid(p))
			continue;

		Paint::with(validSize, p, idx, nullptr);

		Editing::Dot dot;
		dot.indexed = idx;
		set()(p, dot);
	}

	return this;
}

Command* SetProperties::exec(Object::Ptr, int, const Variant*) {
	return this;
}

Command* SetProperties::create(void) {
	SetProperties* result = new SetProperties();

	return result;
}

void SetProperties::destroy(Command* ptr) {
	SetProperties* impl = static_cast<SetProperties*>(ptr);
	delete impl;
}

SetPropertiesToAll::SetPropertiesToAll() {
}

SetPropertiesToAll::~SetPropertiesToAll() {
}

unsigned SetPropertiesToAll::type(void) const {
	return TYPE();
}

const char* SetPropertiesToAll::toString(void) const {
	return "Set properties to all";
}

Command* SetPropertiesToAll::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);
	(void)entry;

	for (int i = 0; i < ptr->count(); ++i) {
		const Editing::Point::Set &l = points()[i];
		for (const Editing::Point::Set::value_type &q : l) {
			const Math::Vec2i &p = q.position;
			const int val = q.dot.indexed;
			set()(i, p, val);
		}
	}

	return this;
}

Command* SetPropertiesToAll::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);
	(void)entry;

	for (int i = 0; i < ptr->count(); ++i) {
		const Editing::Point::Set &layer = old()[i];
		for (const Editing::Point &p : layer) {
			set()(i, p.position, p.dot);
		}
	}

	return this;
}

SetPropertiesToAll* SetPropertiesToAll::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

SetPropertiesToAll* SetPropertiesToAll::with(const LayeredPoints &points_) {
	points(points_);

	return this;
}

Command* SetPropertiesToAll::exec(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);
	(void)entry;

	for (int i = 0; i < ptr->count(); ++i) {
		old().push_back(Editing::Point::Set());
		Editing::Point::Set &layer = old().back();
		for (const Editing::Point::Set::value_type &q : points()[i]) {
			const Math::Vec2i &p = q.position;
			Editing::Dot dot;
			if (get()(i, p, dot))
				layer.insert(Editing::Point(p, dot));
		}
	}
	old().shrink_to_fit();

	redo(obj, argc, argv);

	return this;
}

Command* SetPropertiesToAll::create(void) {
	SetPropertiesToAll* result = new SetPropertiesToAll();

	return result;
}

void SetPropertiesToAll::destroy(Command* ptr) {
	SetPropertiesToAll* impl = static_cast<SetPropertiesToAll*>(ptr);
	delete impl;
}

SetFigure::SetFigure() {
}

SetFigure::~SetFigure() {
}

unsigned SetFigure::type(void) const {
	return TYPE();
}

const char* SetFigure::toString(void) const {
	return "Set figure";
}

Command* SetFigure::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	old(entry->figure);
	entry->figure = figure();

	return this;
}

Command* SetFigure::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	entry->figure = old();

	return this;
}

SetFigure* SetFigure::with(int n) {
	figure(n);

	return this;
}

Command* SetFigure::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetFigure::create(void) {
	SetFigure* result = new SetFigure();

	return result;
}

void SetFigure::destroy(Command* ptr) {
	SetFigure* impl = static_cast<SetFigure*>(ptr);
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
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	old(entry->name);
	entry->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

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

SetRoutines::SetRoutines() {
}

SetRoutines::~SetRoutines() {
}

unsigned SetRoutines::type(void) const {
	return TYPE();
}

const char* SetRoutines::toString(void) const {
	return "Set routines";
}

Command* SetRoutines::redo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	oldUpdateRoutine(ptr->updateRoutine());
	ptr->updateRoutine(updateRoutine());

	oldOnHitsRoutine(ptr->onHitsRoutine());
	ptr->onHitsRoutine(onHitsRoutine());

	return this;
}

Command* SetRoutines::undo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	ptr->updateRoutine(oldUpdateRoutine());

	ptr->onHitsRoutine(oldOnHitsRoutine());

	return this;
}

SetRoutines* SetRoutines::with(const std::string &m, const std::string &n) {
	updateRoutine(m);
	onHitsRoutine(n);

	return this;
}

Command* SetRoutines::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetRoutines::create(void) {
	SetRoutines* result = new SetRoutines();

	return result;
}

void SetRoutines::destroy(Command* ptr) {
	SetRoutines* impl = static_cast<SetRoutines*>(ptr);
	delete impl;
}

SetViewAs::SetViewAs() {
}

SetViewAs::~SetViewAs() {
}

unsigned SetViewAs::type(void) const {
	return TYPE();
}

const char* SetViewAs::toString(void) const {
	return "Set view as";
}

Command* SetViewAs::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	old(entry->asActor);
	entry->asActor = asActor();

	return this;
}

Command* SetViewAs::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	entry->asActor = old();

	return this;
}

SetViewAs* SetViewAs::with(bool v) {
	asActor(v);

	return this;
}

Command* SetViewAs::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetViewAs::create(void) {
	SetViewAs* result = new SetViewAs();

	return result;
}

void SetViewAs::destroy(Command* ptr) {
	SetViewAs* impl = static_cast<SetViewAs*>(ptr);
	delete impl;
}

SetDefinition::SetDefinition() {
}

SetDefinition::~SetDefinition() {
}

unsigned SetDefinition::type(void) const {
	return TYPE();
}

const char* SetDefinition::toString(void) const {
	return "Set definition";
}

Command* SetDefinition::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	old(entry->definition);
	entry->definition = definition();

	return this;
}

Command* SetDefinition::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	entry->definition = old();

	return this;
}

SetDefinition* SetDefinition::with(const active_t &def) {
	definition(def);

	return this;
}

Command* SetDefinition::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

bool SetDefinition::isSimilarTo(const Command* other) const {
	if (other->type() != TYPE())
		return false;

	const SetDefinition* other_ = Command::as<SetDefinition>(other);

	const UInt64 modl = modificationBits();
	const UInt64 modr = other_->modificationBits();
	if (modl != modr)
		return false;

	if (definition() == other_->old())
		return false;

	return true;
}

bool SetDefinition::mergeWith(const Command* other) {
	if (other->type() != TYPE())
		return false;

	const SetDefinition* other_ = Command::as<SetDefinition>(other);

	definition(other_->definition());

	return true;
}

UInt64 SetDefinition::modificationBits(void) const {
	static_assert(sizeof(definition()) <= sizeof(UInt64) * 8, "Wrong type.");

	UInt64 result = 0;
	const Byte* bytesl = (const Byte*)&definition();
	const Byte* bytesr = (const Byte*)&old();
	for (int i = 0; i < sizeof(active_t); ++i) {
		if (bytesl[i] != bytesr[i])
			result |= 1llu << i;
	}

	return result;
}

Command* SetDefinition::create(void) {
	SetDefinition* result = new SetDefinition();

	return result;
}

void SetDefinition::destroy(Command* ptr) {
	SetDefinition* impl = static_cast<SetDefinition*>(ptr);
	delete impl;
}

Resize::Resize() {
	bytes(0);
	propertiesBytes(0);
}

Resize::~Resize() {
}

unsigned Resize::type(void) const {
	return TYPE();
}

const char* Resize::toString(void) const {
	return "Resize";
}

Command* Resize::redo(Object::Ptr obj, int, const Variant*) {
	auto redo_ = [] (int &bytes, Bytes::Ptr &old, ::Image::Ptr img, const Math::Vec2i size) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			img->toBytes(tmp.get(), "");
			bytes = (int)tmp->count();
			int n = LZ4_compressBound((int)tmp->count());
			if (n < (int)tmp->count()) {
				old->resize((size_t)n);
				n = LZ4_compress_default(
					(const char*)tmp->pointer(), (char*)old->pointer(),
					(int)tmp->count(), (int)old->count()
				);
				GBBASIC_ASSERT(n);
				old->resize((size_t)n);
			} else {
				bytes = 0;
				old->clear();
				old->writeBytes(tmp.get());
			}
		}
		img->resize(size.x, size.y, false);
	};

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	::Actor::Frame* frame = ptr->get(sub());
	::Image::Ptr &img = frame->pixels;
	Texture::Ptr &tex = frame->texture;
	::Image::Ptr &props = frame->properties;

	if (!oldPixels())
		oldAnchor(frame->anchor);
	redo_(bytes(), oldPixels(), img, size());
	redo_(propertiesBytes(), oldProperties(), props, propertiesSize());
	frame->anchor.x = Math::min(frame->anchor.x, frame->width() - 1);
	frame->anchor.y = Math::min(frame->anchor.y, frame->height() - 1);

	tex = nullptr;

	return this;
}

Command* Resize::undo(Object::Ptr obj, int, const Variant*) {
	auto undo_ = [] (int bytes, Bytes::Ptr old, ::Image::Ptr img) -> void {
		if (bytes) {
			Bytes::Ptr tmp(Bytes::create());
			tmp->resize(bytes);
			const int n = LZ4_decompress_safe(
				(const char*)old->pointer(), (char*)tmp->pointer(),
				(int)old->count(), (int)tmp->count()
			);
			(void)n;
			GBBASIC_ASSERT(n == (int)bytes);
			img->fromBytes(tmp.get());
		} else {
			img->fromBytes(old.get());
		}
	};

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	::Actor::Frame* frame = ptr->get(sub());
	::Image::Ptr &img = frame->pixels;
	Texture::Ptr &tex = frame->texture;
	::Image::Ptr &props = frame->properties;

	undo_(bytes(), oldPixels(), img);
	undo_(propertiesBytes(), oldProperties(), props);
	frame->anchor = oldAnchor();

	tex = nullptr;

	return this;
}

Resize* Resize::with(const Math::Vec2i &size_, const Math::Vec2i &propSize) {
	size(size_);
	propertiesSize(propSize);

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

SetAnchor::SetAnchor() {
	filled(false);
}

SetAnchor::~SetAnchor() {
}

unsigned SetAnchor::type(void) const {
	return TYPE();
}

const char* SetAnchor::toString(void) const {
	return "Set anchor";
}

Command* SetAnchor::redo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	::Actor::Frame* frame = ptr->get(sub());
	if (!filled()) {
		old(frame->anchor);
		filled(true);
	}
	frame->anchor = anchor();

	return this;
}

Command* SetAnchor::undo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	::Actor::Frame* frame = ptr->get(sub());
	frame->anchor = old();

	return this;
}

SetAnchor* SetAnchor::with(const Math::Vec2i &anchor_) {
	anchor(anchor_);

	return this;
}

Command* SetAnchor::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetAnchor::create(void) {
	SetAnchor* result = new SetAnchor();

	return result;
}

void SetAnchor::destroy(Command* ptr) {
	SetAnchor* impl = static_cast<SetAnchor*>(ptr);
	delete impl;
}

SetAnchorForAll::SetAnchorForAll() {
	filled(false);
}

SetAnchorForAll::~SetAnchorForAll() {
}

unsigned SetAnchorForAll::type(void) const {
	return TYPE();
}

const char* SetAnchorForAll::toString(void) const {
	return "Set anchor for all";
}

Command* SetAnchorForAll::redo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	if (!filled()) {
		for (int i = 0; i < ptr->count(); ++i) {
			const ::Actor::Frame* frame = ptr->get(i);
			old().push_back(frame->anchor);
		}
		filled(true);
	}
	old().shrink_to_fit();
	for (int i = 0; i < ptr->count(); ++i) {
		::Actor::Frame* frame = ptr->get(i);
		frame->anchor = anchor();
	}

	return this;
}

Command* SetAnchorForAll::undo(Object::Ptr obj, int, const Variant*) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);

	for (int i = 0; i < ptr->count(); ++i) {
		::Actor::Frame* frame = ptr->get(i);
		frame->anchor = old()[i];
	}

	return this;
}

SetAnchorForAll* SetAnchorForAll::with(const Math::Vec2i &anchor_) {
	anchor(anchor_);

	return this;
}

Command* SetAnchorForAll::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetAnchorForAll::create(void) {
	SetAnchorForAll* result = new SetAnchorForAll();

	return result;
}

void SetAnchorForAll::destroy(Command* ptr) {
	SetAnchorForAll* impl = static_cast<SetAnchorForAll*>(ptr);
	delete impl;
}

Import::Import() {
	actorBytes(0);
	figure(0);
	asActor(true);
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
	Layered::Layered::redo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	if (!oldActor()) {
		oldActor(Bytes::Ptr(Bytes::create()));

		rapidjson::Document doc;
		ptr->toJson(doc);
		std::string str;
		Json::toString(doc, str, false);
		Bytes::Ptr tmp(Bytes::create());
		tmp->writeString(str);
		actorBytes((int)tmp->count());
		int n = LZ4_compressBound((int)tmp->count());
		if (n < (int)tmp->count()) {
			oldActor()->resize((size_t)n);
			n = LZ4_compress_default(
				(const char*)tmp->pointer(), (char*)oldActor()->pointer(),
				(int)tmp->count(), (int)oldActor()->count()
			);
			GBBASIC_ASSERT(n);
			oldActor()->resize((size_t)n);
		} else {
			actorBytes(0);
			oldActor()->clear();
			oldActor()->writeBytes(tmp.get());
		}

		oldFigure(entry->figure);
		oldAsActor(entry->asActor);
		oldDefinition(entry->definition);
	}
	rapidjson::Document doc_;
	actor()->toJson(doc_);
	ptr->unload();
	ptr->fromJson(palette(), doc_);
	entry->figure = figure();
	entry->asActor = asActor();
	entry->definition = definition();

	return this;
}

Command* Import::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);

	std::string str;
	if (actorBytes()) {
		Bytes::Ptr tmp(Bytes::create());
		tmp->resize(actorBytes());
		const int n = LZ4_decompress_safe(
			(const char*)oldActor()->pointer(), (char*)tmp->pointer(),
			(int)oldActor()->count(), (int)tmp->count()
		);
		(void)n;
		GBBASIC_ASSERT(n == (int)actorBytes());
		tmp->readString(str);
	} else {
		oldActor()->poke(0);
		oldActor()->readString(str);
	}
	rapidjson::Document doc;
	Json::fromString(doc, str.c_str());
	ptr->fromJson(palette(), doc);
	entry->figure = oldFigure();
	entry->asActor = oldAsActor();
	entry->definition = oldDefinition();

	return this;
}

Import* Import::with(Indexed::Ptr plt, const ::Actor::Ptr &act, int figure_, bool asActor_, const active_t &def) {
	palette(plt);
	actor(act);
	figure(figure_);
	asActor(asActor_);
	definition(def);

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

ImportFrame::ImportFrame() {
	bytes(0);
	frame(0);
}

ImportFrame::~ImportFrame() {
}

unsigned ImportFrame::type(void) const {
	return TYPE();
}

const char* ImportFrame::toString(void) const {
	return "Import frame";
}

Command* ImportFrame::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);
	(void)entry;

	::Actor::Frame* frame_ = ptr->get(frame());
	Image::Ptr &img = frame_->pixels;

	if (!old()) {
		old(Bytes::Ptr(Bytes::create()));

		Bytes::Ptr tmp(Bytes::create());
		if (img->paletted())
			img->toBytes(tmp.get(), "");
		else
			img->toBytes(tmp.get(), "png");
		bytes((int)tmp->count());
		int n = LZ4_compressBound((int)tmp->count());
		if (n < (int)tmp->count()) {
			old()->resize((size_t)n);
			n = LZ4_compress_default(
				(const char*)tmp->pointer(), (char*)old()->pointer(),
				(int)tmp->count(), (int)old()->count()
			);
			GBBASIC_ASSERT(n);
			old()->resize((size_t)n);
		} else {
			bytes(0);
			old()->clear();
			old()->writeBytes(tmp.get());
		}
	}
	img->resize(image()->width(), image()->height(), false);
	img->fromImage(image().get());

	frame_->cleanup();

	return this;
}

Command* ImportFrame::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	ActorAssets::Entry* entry = (ActorAssets::Entry*)(arg0);
	(void)entry;

	::Actor::Frame* frame_ = ptr->get(frame());
	const Image::Ptr &img = frame_->pixels;

	if (bytes()) {
		Bytes::Ptr tmp(Bytes::create());
		tmp->resize(bytes());
		const int n = LZ4_decompress_safe(
			(const char*)old()->pointer(), (char*)tmp->pointer(),
			(int)old()->count(), (int)tmp->count()
		);
		(void)n;
		GBBASIC_ASSERT(n == (int)bytes());
		img->fromBytes(tmp.get());
	} else {
		img->fromBytes(old().get());
	}

	frame_->cleanup();

	return this;
}

ImportFrame* ImportFrame::with(const Image::Ptr &img, int frame_) {
	image(img);
	frame(frame_);

	return this;
}

Command* ImportFrame::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ImportFrame::create(void) {
	ImportFrame* result = new ImportFrame();

	return result;
}

void ImportFrame::destroy(Command* ptr) {
	ImportFrame* impl = static_cast<ImportFrame*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
