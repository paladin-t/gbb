/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_scene.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Scene commands
*/

namespace Commands {

namespace Scene {

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

SetActor::SetActor() {
	oldActorRoutineOverridingsFilled(false);
}

SetActor::~SetActor() {
}

unsigned SetActor::type(void) const {
	return TYPE();
}

Command* SetActor::redo(Object::Ptr obj, int argc, const Variant* argv) {
	PaintableBase::Pencil::redo(obj, argc, argv);

	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	if (!oldActorRoutineOverridingsFilled()) {
		oldActorRoutineOverridingsFilled(true);
		oldActorRoutineOverridings(entry->actorRoutineOverridings);
	}
	entry->actorRoutineOverridings = actorRoutineOverridings();

	return this;
}

Command* SetActor::undo(Object::Ptr obj, int argc, const Variant* argv) {
	PaintableBase::Pencil::undo(obj, argc, argv);

	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	entry->actorRoutineOverridings = oldActorRoutineOverridings();

	return this;
}

SetActor* SetActor::with(const ActorRoutineOverriding::Array &actorRoutines) {
	actorRoutineOverridings(actorRoutines);

	return this;
}

Command* SetActor::create(void) {
	SetActor* result = new SetActor();

	return result;
}

void SetActor::destroy(Command* ptr) {
	SetActor* impl = static_cast<SetActor*>(ptr);
	delete impl;
}

MoveActor::MoveActor() {
	sourcePosition(Math::Vec2i(-1, -1));
	sourceValue(-1);
	destinationPosition(Math::Vec2i(-1, -1));

	oldDestinationValue(-1);
	oldActorRoutineOverridingsFilled(false);
}

MoveActor::~MoveActor() {
}

unsigned MoveActor::type(void) const {
	return TYPE();
}

const char* MoveActor::toString(void) const {
	return "Move actor";
}

Command* MoveActor::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	do {
		Editing::Dot dot;
		dot.indexed = ::Scene::INVALID_ACTOR();
		set()(sourcePosition(), dot);
	} while (false);

	do {
		Editing::Dot dot;
		dot.indexed = sourceValue();
		set()(destinationPosition(), dot);
	} while (false);

	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	if (!oldActorRoutineOverridingsFilled()) {
		oldActorRoutineOverridingsFilled(true);
		oldActorRoutineOverridings(entry->actorRoutineOverridings);
	}
	entry->actorRoutineOverridings = actorRoutineOverridings();

	return this;
}

Command* MoveActor::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	do {
		Editing::Dot dot;
		dot.indexed = oldDestinationValue();
		set()(destinationPosition(), dot);
	} while (false);

	do {
		Editing::Dot dot;
		dot.indexed = sourceValue();
		set()(sourcePosition(), dot);
	} while (false);

	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	entry->actorRoutineOverridings = oldActorRoutineOverridings();

	return this;
}

MoveActor* MoveActor::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

MoveActor* MoveActor::with(const Math::Vec2i &pos, int val, bool isSource) {
	if (isSource) {
		sourcePosition(pos);
		sourceValue(val);

		Editing::Dot dot;
		dot.indexed = ::Scene::INVALID_ACTOR();
		set()(sourcePosition(), dot);
	} else {
		if (oldDestinationValue() != -1) {
			Editing::Dot dot;
			dot.indexed = oldDestinationValue();
			set()(destinationPosition(), dot);
		}

		destinationPosition(pos);
		oldDestinationValue(val);

		Editing::Dot dot;
		dot.indexed = sourceValue();
		set()(destinationPosition(), dot);
	}

	return this;
}

MoveActor* MoveActor::with(const ActorRoutineOverriding::Array &actorRoutines) {
	actorRoutineOverridings(actorRoutines);

	return this;
}

Command* MoveActor::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* MoveActor::create(void) {
	MoveActor* result = new MoveActor();

	return result;
}

void MoveActor::destroy(Command* ptr) {
	MoveActor* impl = static_cast<MoveActor*>(ptr);
	delete impl;
}

AddTrigger::AddTrigger() {
	index(0);
}

AddTrigger::~AddTrigger() {
}

unsigned AddTrigger::type(void) const {
	return TYPE();
}

const char* AddTrigger::toString(void) const {
	return "Add trigger";
}

Command* AddTrigger::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	add()(index(), trigger());

	return this;
}

Command* AddTrigger::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	remove()(index(), nullptr);

	return this;
}

AddTrigger* AddTrigger::with(Adder add_, Remover remove_) {
	add(add_);
	remove(remove_);

	return this;
}

AddTrigger* AddTrigger::with(int index_, const Trigger &trigger_) {
	index(index_);
	trigger(trigger_);

	return this;
}

Command* AddTrigger::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddTrigger::create(void) {
	AddTrigger* result = new AddTrigger();

	return result;
}

void AddTrigger::destroy(Command* ptr) {
	AddTrigger* impl = static_cast<AddTrigger*>(ptr);
	delete impl;
}

DeleteTrigger::DeleteTrigger() {
	index(0);

	oldTriggerFilled(false);
}

DeleteTrigger::~DeleteTrigger() {
}

unsigned DeleteTrigger::type(void) const {
	return TYPE();
}

const char* DeleteTrigger::toString(void) const {
	return "Delete trigger";
}

Command* DeleteTrigger::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	Trigger old;
	if (remove()(index(), &old)) {
		if (!oldTriggerFilled()) {
			oldTrigger(old);
			oldTriggerFilled(true);
		}
	}

	return this;
}

Command* DeleteTrigger::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	if (oldTriggerFilled()) {
		add()(index(), oldTrigger());
	}

	return this;
}

DeleteTrigger* DeleteTrigger::with(Adder add_, Remover remove_) {
	add(add_);
	remove(remove_);

	return this;
}

DeleteTrigger* DeleteTrigger::with(int index_) {
	index(index_);

	return this;
}

Command* DeleteTrigger::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* DeleteTrigger::create(void) {
	DeleteTrigger* result = new DeleteTrigger();

	return result;
}

void DeleteTrigger::destroy(Command* ptr) {
	DeleteTrigger* impl = static_cast<DeleteTrigger*>(ptr);
	delete impl;
}

EditTrigger::EditTrigger() {
	index(0);

	oldTriggerFilled(false);
}

EditTrigger::~EditTrigger() {
}

Command* EditTrigger::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	if (!oldTriggerFilled()) {
		get()(index(), &oldTrigger());
		oldTriggerFilled(true);
	}
	set()(index(), trigger());

	return this;
}

Command* EditTrigger::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	set()(index(), oldTrigger());

	return this;
}

Command* EditTrigger::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

MoveTrigger::MoveTrigger() {
}

MoveTrigger::~MoveTrigger() {
}

unsigned MoveTrigger::type(void) const {
	return TYPE();
}

const char* MoveTrigger::toString(void) const {
	return "Move trigger";
}

MoveTrigger* MoveTrigger::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

MoveTrigger* MoveTrigger::with(int index_, const Trigger &trigger_) {
	index(index_);
	trigger(trigger_);

	return this;
}

MoveTrigger* MoveTrigger::with(const Trigger &trigger_) {
	trigger(trigger_);

	return this;
}

Command* MoveTrigger::create(void) {
	MoveTrigger* result = new MoveTrigger();

	return result;
}

void MoveTrigger::destroy(Command* ptr) {
	MoveTrigger* impl = static_cast<MoveTrigger*>(ptr);
	delete impl;
}

ResizeTrigger::ResizeTrigger() {
}

ResizeTrigger::~ResizeTrigger() {
}

unsigned ResizeTrigger::type(void) const {
	return TYPE();
}

const char* ResizeTrigger::toString(void) const {
	return "Resize trigger";
}

ResizeTrigger* ResizeTrigger::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

ResizeTrigger* ResizeTrigger::with(int index_, const Trigger &trigger_) {
	index(index_);
	trigger(trigger_);

	return this;
}

ResizeTrigger* ResizeTrigger::with(const Trigger &trigger_) {
	trigger(trigger_);

	return this;
}

Command* ResizeTrigger::create(void) {
	ResizeTrigger* result = new ResizeTrigger();

	return result;
}

void ResizeTrigger::destroy(Command* ptr) {
	ResizeTrigger* impl = static_cast<ResizeTrigger*>(ptr);
	delete impl;
}

SetTriggerColor::SetTriggerColor() {
}

SetTriggerColor::~SetTriggerColor() {
}

unsigned SetTriggerColor::type(void) const {
	return TYPE();
}

const char* SetTriggerColor::toString(void) const {
	return "Set trigger color";
}

SetTriggerColor* SetTriggerColor::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

SetTriggerColor* SetTriggerColor::with(int index_, const Trigger &trigger_) {
	index(index_);
	trigger(trigger_);

	return this;
}

SetTriggerColor* SetTriggerColor::with(const Trigger &trigger_) {
	trigger(trigger_);

	return this;
}

Command* SetTriggerColor::create(void) {
	SetTriggerColor* result = new SetTriggerColor();

	return result;
}

void SetTriggerColor::destroy(Command* ptr) {
	SetTriggerColor* impl = static_cast<SetTriggerColor*>(ptr);
	delete impl;
}

SetTriggerRoutines::SetTriggerRoutines() {
}

SetTriggerRoutines::~SetTriggerRoutines() {
}

unsigned SetTriggerRoutines::type(void) const {
	return TYPE();
}

const char* SetTriggerRoutines::toString(void) const {
	return "Set trigger routines";
}

SetTriggerRoutines* SetTriggerRoutines::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

SetTriggerRoutines* SetTriggerRoutines::with(int index_, const Trigger &trigger_) {
	index(index_);
	trigger(trigger_);

	return this;
}

SetTriggerRoutines* SetTriggerRoutines::with(const Trigger &trigger_) {
	trigger(trigger_);

	return this;
}

Command* SetTriggerRoutines::create(void) {
	SetTriggerRoutines* result = new SetTriggerRoutines();

	return result;
}

void SetTriggerRoutines::destroy(Command* ptr) {
	SetTriggerRoutines* impl = static_cast<SetTriggerRoutines*>(ptr);
	delete impl;
}

SwapTrigger::SwapTrigger() {
	indexA(0);
	indexB(0);
	filled(false);
}

SwapTrigger::~SwapTrigger() {
}

unsigned SwapTrigger::type(void) const {
	return TYPE();
}

const char* SwapTrigger::toString(void) const {
	return "Swap trigger";
}

Command* SwapTrigger::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	if (!filled()) {
		get()(indexA(), &triggerA());
		get()(indexB(), &triggerB());
		filled(true);
	}
	set()(indexA(), triggerB());
	set()(indexB(), triggerA());

	return this;
}

Command* SwapTrigger::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	set()(indexA(), triggerA());
	set()(indexB(), triggerB());

	return this;
}

SwapTrigger* SwapTrigger::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

SwapTrigger* SwapTrigger::with(int indexA_, int indexB_) {
	indexA(indexA_);
	indexB(indexB_);

	return this;
}

Command* SwapTrigger::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SwapTrigger::create(void) {
	SwapTrigger* result = new SwapTrigger();

	return result;
}

void SwapTrigger::destroy(Command* ptr) {
	SwapTrigger* impl = static_cast<SwapTrigger*>(ptr);
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
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	old(entry->name);
	entry->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Actor::Ptr ptr = Object::as<::Actor::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

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
	::Scene::Ptr ptr = Object::as<::Scene::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	old(entry->definition);
	entry->definition = definition();

	return this;
}

Command* SetDefinition::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Scene::Ptr ptr = Object::as<::Scene::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	entry->definition = old();

	return this;
}

SetDefinition* SetDefinition::with(const scene_t &def) {
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
	for (int i = 0; i < sizeof(scene_t); ++i) {
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

SetActorRoutineOverridings::SetActorRoutineOverridings() {
}

SetActorRoutineOverridings::~SetActorRoutineOverridings() {
}

unsigned SetActorRoutineOverridings::type(void) const {
	return TYPE();
}

const char* SetActorRoutineOverridings::toString(void) const {
	return "Set actor routine overridings";
}

Command* SetActorRoutineOverridings::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Scene::Ptr ptr = Object::as<::Scene::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	old(entry->actorRoutineOverridings);
	entry->actorRoutineOverridings = actorRoutineOverridings();

	return this;
}

Command* SetActorRoutineOverridings::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Scene::Ptr ptr = Object::as<::Scene::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);

	entry->actorRoutineOverridings = old();

	return this;
}

SetActorRoutineOverridings* SetActorRoutineOverridings::with(const ActorRoutineOverriding::Array &actorRoutines) {
	actorRoutineOverridings(actorRoutines);

	return this;
}

Command* SetActorRoutineOverridings::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetActorRoutineOverridings::create(void) {
	SetActorRoutineOverridings* result = new SetActorRoutineOverridings();

	return result;
}

void SetActorRoutineOverridings::destroy(Command* ptr) {
	SetActorRoutineOverridings* impl = static_cast<SetActorRoutineOverridings*>(ptr);
	delete impl;
}

Resize::Resize() {
	propertiesBytes(0);
	actorsBytes(0);
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
	auto redo_ = [] (int &bytes, Bytes::Ptr &old, ::Map::Ptr map_, const Math::Vec2i size, int default_) -> void {
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
		map_->resize(size.x, size.y, default_);
	};

	Layered::Layered::redo(obj, argc, argv);

	::Scene::Ptr scene = Object::as<::Scene::Ptr>(obj);
	::Map::Ptr props = scene->propertyLayer();
	::Map::Ptr actors = scene->actorLayer();

	redo_(propertiesBytes(), oldProperties(), props, size(), 0);
	redo_(actorsBytes(), oldActors(), actors, size(), ::Scene::INVALID_ACTOR());
	scene->resized();

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

	::Scene::Ptr scene = Object::as<::Scene::Ptr>(obj);
	::Map::Ptr props = scene->propertyLayer();
	::Map::Ptr actors = scene->actorLayer();

	undo_(propertiesBytes(), oldProperties(), props);
	undo_(actorsBytes(), oldActors(), actors);
	scene->resized();

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

Import::Import() {
	mapBytes(0);
	attributesBytes(0);
	propertiesBytes(0);
	actorsBytes(0);

	oldDefinitionFilled(false);
	oldActorRoutineOverridingsFilled(false);
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

	::Scene::Ptr scene = Object::as<::Scene::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);
	::Map::Ptr map_ = scene->mapLayer();
	::Map::Ptr attribs = scene->attributeLayer();
	::Map::Ptr props = scene->propertyLayer();
	::Map::Ptr actors_ = scene->actorLayer();
	Trigger::Array* triggers_ = scene->triggerLayer();

	if (map())
		redo_(map(), mapBytes(), oldMap(), map_);
	if (attributes())
		redo_(attributes(), attributesBytes(), oldAttributes(), attribs);
	if (properties())
		redo_(properties(), propertiesBytes(), oldProperties(), props);
	if (actors())
		redo_(actors(), actorsBytes(), oldActors(), actors_);
	if (triggers_) {
		if (oldTriggers().empty())
			oldTriggers(*triggers_);
		*triggers_ = triggers();
	}

	if (!oldDefinitionFilled()) {
		oldDefinitionFilled(true);
		oldDefinition(entry->definition);
	}
	entry->definition = definition();

	if (!oldActorRoutineOverridingsFilled()) {
		oldActorRoutineOverridingsFilled(true);
		oldActorRoutineOverridings(entry->actorRoutineOverridings);
	}
	entry->actorRoutineOverridings = actorRoutineOverridings();

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

	::Scene::Ptr scene = Object::as<::Scene::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	SceneAssets::Entry* entry = (SceneAssets::Entry*)(arg0);
	::Map::Ptr map_ = scene->mapLayer();
	::Map::Ptr attribs = scene->attributeLayer();
	::Map::Ptr props = scene->propertyLayer();
	::Map::Ptr actors_ = scene->actorLayer();
	Trigger::Array* triggers_ = scene->triggerLayer();

	if (map())
		undo_(map(), mapBytes(), oldMap(), map_);
	if (attributes())
		undo_(attributes(), attributesBytes(), oldAttributes(), attribs);
	if (properties())
		undo_(properties(), propertiesBytes(), oldProperties(), props);
	if (actors())
		undo_(actors(), actorsBytes(), oldActors(), actors_);
	if (triggers_)
		*triggers_ = oldTriggers();

	entry->definition = oldDefinition();

	entry->actorRoutineOverridings = oldActorRoutineOverridings();

	return this;
}

Import* Import::with(const ::Map::Ptr &map_, const ::Map::Ptr &attrib, const ::Map::Ptr &props, const ::Map::Ptr &actors_, const Trigger::Array* triggers_, const scene_t &def, const ActorRoutineOverriding::Array &actorRoutines) {
	map(map_);
	attributes(attrib);
	properties(props);
	actors(actors_);
	if (triggers_)
		triggers(*triggers_);
	definition(def);
	actorRoutineOverridings(actorRoutines);

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

}

}

/* ===========================================================================} */
