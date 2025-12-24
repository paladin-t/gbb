/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_SCENE_H__
#define __COMMANDS_SCENE_H__

#include "commands_paintable.h"
#include "commands_paintable2d.h"

/*
** {===========================================================================
** Scene commands
*/

namespace Commands {

namespace Scene {

#if GBBASIC_EDITOR_PAINTABLE2D_ENABLED
namespace PaintableBase = Commands::Paintable2D;
#else /* GBBASIC_EDITOR_PAINTABLE2D_ENABLED */
namespace PaintableBase = Commands::Paintable;
#endif /* GBBASIC_EDITOR_PAINTABLE2D_ENABLED */

class Pencil : public PaintableBase::Pencil {
public:
	Pencil();
	virtual ~Pencil() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Line : public PaintableBase::Line {
public:
	Line();
	virtual ~Line() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Box : public PaintableBase::Box {
public:
	Box();
	virtual ~Box() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class BoxFill : public PaintableBase::BoxFill {
public:
	BoxFill();
	virtual ~BoxFill() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Ellipse : public PaintableBase::Ellipse {
public:
	Ellipse();
	virtual ~Ellipse() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class EllipseFill : public PaintableBase::EllipseFill {
public:
	EllipseFill();
	virtual ~EllipseFill() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Fill : public PaintableBase::Fill {
public:
	Fill();
	virtual ~Fill() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Replace : public PaintableBase::Replace {
public:
	Replace();
	virtual ~Replace() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Stamp : public PaintableBase::Stamp {
public:
	Stamp();
	virtual ~Stamp() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Rotate : public PaintableBase::Rotate {
public:
	Rotate();
	virtual ~Rotate() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Flip : public PaintableBase::Flip {
public:
	Flip();
	virtual ~Flip() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Cut : public PaintableBase::Cut {
public:
	Cut();
	virtual ~Cut() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Paste : public PaintableBase::Paste {
public:
	Paste();
	virtual ~Paste() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Delete : public PaintableBase::Delete {
public:
	Delete();
	virtual ~Delete() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetActor : public PaintableBase::Pencil {
public:
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, actorRoutineOverridings)

	GBBASIC_PROPERTY(bool, oldActorRoutineOverridingsFilled)
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, oldActorRoutineOverridings)

public:
	SetActor();
	virtual ~SetActor() override;

	GBBASIC_CLASS_TYPE('S', 'T', 'A', 'S')

	virtual unsigned type(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetActor* with(const ActorRoutineOverriding::Array &actorRoutines);
	using PaintableBase::Pencil::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class MoveActor : public Layered::Layered {
public:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> Getter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> Setter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY(Math::Vec2i, sourcePosition)
	GBBASIC_PROPERTY(int, sourceValue)
	GBBASIC_PROPERTY(Math::Vec2i, destinationPosition)
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, actorRoutineOverridings)

	GBBASIC_PROPERTY(int, oldDestinationValue)
	GBBASIC_PROPERTY(bool, oldActorRoutineOverridingsFilled)
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, oldActorRoutineOverridings)

public:
	MoveActor();
	virtual ~MoveActor() override;

	GBBASIC_CLASS_TYPE('M', 'V', 'A', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual MoveActor* with(Getter get, Setter set);
	virtual MoveActor* with(const Math::Vec2i &pos, int val, bool isSource);
	virtual MoveActor* with(const ActorRoutineOverriding::Array &actorRoutines);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class AddTrigger : public Layered::Layered {
public:
	typedef std::function<bool(int, const Trigger &)> Adder;
	typedef std::function<bool(int, Trigger* /* nullable */)> Remover;

public:
	GBBASIC_PROPERTY_READONLY(Adder, add)
	GBBASIC_PROPERTY_READONLY(Remover, remove)
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(Trigger, trigger)

public:
	AddTrigger();
	virtual ~AddTrigger() override;

	GBBASIC_CLASS_TYPE('A', 'D', 'T', 'R')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual AddTrigger* with(Adder add, Remover remove);
	virtual AddTrigger* with(int index, const Trigger &trigger);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteTrigger : public Layered::Layered {
public:
	typedef std::function<bool(int, const Trigger &)> Adder;
	typedef std::function<bool(int, Trigger* /* nullable */)> Remover;

public:
	GBBASIC_PROPERTY_READONLY(Adder, add)
	GBBASIC_PROPERTY_READONLY(Remover, remove)
	GBBASIC_PROPERTY(int, index)

	GBBASIC_PROPERTY(bool, oldTriggerFilled)
	GBBASIC_PROPERTY(Trigger, oldTrigger)

public:
	DeleteTrigger();
	virtual ~DeleteTrigger() override;

	GBBASIC_CLASS_TYPE('D', 'L', 'T', 'R')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual DeleteTrigger* with(Adder add, Remover remove);
	virtual DeleteTrigger* with(int index);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class EditTrigger : public Layered::Layered {
public:
	typedef std::function<bool(int, Trigger* /* nullable */)> Getter;
	typedef std::function<bool(int, const Trigger &)> Setter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(Trigger, trigger)

	GBBASIC_PROPERTY(bool, oldTriggerFilled)
	GBBASIC_PROPERTY(Trigger, oldTrigger)

public:
	EditTrigger();
	virtual ~EditTrigger() override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class MoveTrigger : public EditTrigger {
public:
	MoveTrigger();
	virtual ~MoveTrigger() override;

	GBBASIC_CLASS_TYPE('M', 'V', 'T', 'R')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual MoveTrigger* with(Getter get, Setter set);
	virtual MoveTrigger* with(int index, const Trigger &trigger);
	virtual MoveTrigger* with(const Trigger &trigger);
	using Layered::Layered::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ResizeTrigger : public EditTrigger {
public:
	ResizeTrigger();
	virtual ~ResizeTrigger() override;

	GBBASIC_CLASS_TYPE('R', 'Z', 'T', 'R')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual ResizeTrigger* with(Getter get, Setter set);
	virtual ResizeTrigger* with(int index, const Trigger &trigger);
	virtual ResizeTrigger* with(const Trigger &trigger);
	using Layered::Layered::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetTriggerColor : public EditTrigger {
public:
	SetTriggerColor();
	virtual ~SetTriggerColor() override;

	GBBASIC_CLASS_TYPE('S', 'T', 'R', 'C')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual SetTriggerColor* with(Getter get, Setter set);
	virtual SetTriggerColor* with(int index, const Trigger &trigger);
	virtual SetTriggerColor* with(const Trigger &trigger);
	using Layered::Layered::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetTriggerRoutines : public EditTrigger {
public:
	SetTriggerRoutines();
	virtual ~SetTriggerRoutines() override;

	GBBASIC_CLASS_TYPE('S', 'T', 'R', 'R')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual SetTriggerRoutines* with(Getter get, Setter set);
	virtual SetTriggerRoutines* with(int index, const Trigger &trigger);
	virtual SetTriggerRoutines* with(const Trigger &trigger);
	using Layered::Layered::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SwapTrigger : public Layered::Layered {
public:
	typedef std::function<bool(int, Trigger* /* nullable */)> Getter;
	typedef std::function<bool(int, const Trigger &)> Setter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY(int, indexA)
	GBBASIC_PROPERTY(Trigger, triggerA)
	GBBASIC_PROPERTY(int, indexB)
	GBBASIC_PROPERTY(Trigger, triggerB)
	GBBASIC_PROPERTY(bool, filled)

public:
	SwapTrigger();
	virtual ~SwapTrigger() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'T', 'R')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SwapTrigger* with(Getter get, Setter set);
	virtual SwapTrigger* with(int indexA, int indexB);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SwitchLayer : public Layered::Layered {
public:
	typedef std::function<void(int, bool)> LayerEnabledSetter;

public:
	GBBASIC_PROPERTY_READONLY(LayerEnabledSetter, setLayerEnabled)
	GBBASIC_PROPERTY_READONLY(bool, enabled)

	GBBASIC_PROPERTY_READONLY(bool, old)

public:
	SwitchLayer();
	virtual ~SwitchLayer() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'L', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SwitchLayer* with(LayerEnabledSetter set);
	virtual SwitchLayer* with(bool enabled_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ToggleMask : public PaintableBase::Pencil {
public:
	ToggleMask();
	virtual ~ToggleMask() override;

	GBBASIC_CLASS_TYPE('T', 'G', 'M', 'S')

	virtual unsigned type(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetName : public Command {
public:
	GBBASIC_PROPERTY(std::string, name)

	GBBASIC_PROPERTY(std::string, old)

public:
	SetName();
	virtual ~SetName() override;

	GBBASIC_CLASS_TYPE('S', 'N', 'M', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetName* with(const std::string &n);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetDefinition : public Command {
public:
	GBBASIC_PROPERTY(scene_t, definition)

	GBBASIC_PROPERTY(scene_t, old)

public:
	SetDefinition();
	virtual ~SetDefinition() override;

	GBBASIC_CLASS_TYPE('S', 'D', 'F', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetDefinition* with(const scene_t &actor);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	virtual bool isSimilarTo(const Command* other) const override;
	virtual bool mergeWith(const Command* other) override;
	UInt64 modificationBits(void) const;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetActorRoutineOverridings : public Command {
public:
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, actorRoutineOverridings)

	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, old)

public:
	SetActorRoutineOverridings();
	virtual ~SetActorRoutineOverridings() override;

	GBBASIC_CLASS_TYPE('S', 'R', 'O', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetActorRoutineOverridings* with(const ActorRoutineOverriding::Array &actorRoutines);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Resize : public Layered::Layered {
public:
	GBBASIC_PROPERTY(Math::Vec2i, size)

	GBBASIC_PROPERTY(int, propertiesBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldProperties)
	GBBASIC_PROPERTY(int, actorsBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldActors)

public:
	Resize();
	virtual ~Resize() override;

	GBBASIC_CLASS_TYPE('R', 'S', 'Z', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Resize* with(const Math::Vec2i &size);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Import : public Layered::Layered {
public:
	GBBASIC_PROPERTY(::Map::Ptr, map)
	GBBASIC_PROPERTY(::Map::Ptr, attributes)
	GBBASIC_PROPERTY(::Map::Ptr, properties)
	GBBASIC_PROPERTY(::Map::Ptr, actors)
	GBBASIC_PROPERTY(Trigger::Array, triggers)
	GBBASIC_PROPERTY(scene_t, definition)
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, actorRoutineOverridings)

	GBBASIC_PROPERTY(int, mapBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldMap)
	GBBASIC_PROPERTY(int, attributesBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldAttributes)
	GBBASIC_PROPERTY(int, propertiesBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldProperties)
	GBBASIC_PROPERTY(int, actorsBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldActors)
	GBBASIC_PROPERTY(Trigger::Array, oldTriggers)
	GBBASIC_PROPERTY(bool, oldDefinitionFilled)
	GBBASIC_PROPERTY(scene_t, oldDefinition)
	GBBASIC_PROPERTY(bool, oldActorRoutineOverridingsFilled)
	GBBASIC_PROPERTY(ActorRoutineOverriding::Array, oldActorRoutineOverridings)

public:
	Import();
	virtual ~Import() override;

	GBBASIC_CLASS_TYPE('I', 'M', 'P', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Import* with(const ::Map::Ptr &map_, const ::Map::Ptr &attrib, const ::Map::Ptr &props, const ::Map::Ptr &actors_, const Trigger::Array* triggers_, const scene_t &def, const ActorRoutineOverriding::Array &actorRoutines);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_SCENE_H__ */
