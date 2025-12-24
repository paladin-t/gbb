/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_ACTOR_H__
#define __COMMANDS_ACTOR_H__

#include "commands_paintable.h"

/*
** {===========================================================================
** Actor commands
*/

namespace Commands {

namespace Actor {

class AddFrame : public Layered::Layered {
public:
	GBBASIC_PROPERTY(bool, append)
	GBBASIC_PROPERTY(Image::Ptr, image)
	GBBASIC_PROPERTY(Image::Ptr, properties)

public:
	AddFrame();
	virtual ~AddFrame() override;

	GBBASIC_CLASS_TYPE('A', 'D', 'F', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual AddFrame* with(bool append, Image::Ptr img, Image::Ptr props);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class CutFrame : public Layered::Layered {
public:
	GBBASIC_PROPERTY(bool, append)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Image::Ptr, oldPixels)
	GBBASIC_PROPERTY(Image::Ptr, oldProperties)
	GBBASIC_PROPERTY(Math::Vec2i, oldAnchor)

public:
	CutFrame();
	virtual ~CutFrame() override;

	GBBASIC_CLASS_TYPE('C', 'T', 'F', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual CutFrame* with(bool append);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteFrame : public CutFrame {
public:
	DeleteFrame();
	virtual ~DeleteFrame() override;

	GBBASIC_CLASS_TYPE('D', 'E', 'L', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Pencil : public Paintable::Pencil {
public:
	Pencil();
	virtual ~Pencil() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Line : public Paintable::Line {
public:
	Line();
	virtual ~Line() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Box : public Paintable::Box {
public:
	Box();
	virtual ~Box() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class BoxFill : public Paintable::BoxFill {
public:
	BoxFill();
	virtual ~BoxFill() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Ellipse : public Paintable::Ellipse {
public:
	Ellipse();
	virtual ~Ellipse() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class EllipseFill : public Paintable::EllipseFill {
public:
	EllipseFill();
	virtual ~EllipseFill() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Fill : public Paintable::Fill {
public:
	Fill();
	virtual ~Fill() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Replace : public Paintable::Replace {
public:
	Replace();
	virtual ~Replace() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Rotate : public Paintable::Rotate {
public:
	Rotate();
	virtual ~Rotate() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Flip : public Paintable::Flip {
public:
	Flip();
	virtual ~Flip() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Cut : public Paintable::Cut {
public:
	Cut();
	virtual ~Cut() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Paste : public Paintable::Paste {
public:
	Paste();
	virtual ~Paste() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Delete : public Paintable::Delete {
public:
	Delete();
	virtual ~Delete() override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Set8x16 : public Command {
public:
	GBBASIC_PROPERTY(bool, eightMulSixteen)

	GBBASIC_PROPERTY(bool, old)

public:
	Set8x16();
	virtual ~Set8x16() override;

	GBBASIC_CLASS_TYPE('S', 'E', 'S', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Set8x16* with(bool eightMulSixteen_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetProperties : public Paintable::Paint {
public:
	SetProperties();
	virtual ~SetProperties() override;

	GBBASIC_CLASS_TYPE('S', 'P', 'R', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(Getter get, Setter set);
	virtual Paint* with(const Math::Vec2i &validSize, const Points &points, const Indices &values) override;
	using Paint::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Paint::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetPropertiesToAll : public Layered::Layered {
public:
	typedef std::vector<Editing::Point::Set> LayeredPoints;

	typedef std::function<bool(int, const Math::Vec2i &, Editing::Dot &)> Getter;
	typedef std::function<bool(int, const Math::Vec2i &, const Editing::Dot &)> Setter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY_READONLY(LayeredPoints, points)

	GBBASIC_PROPERTY(LayeredPoints, old)

public:
	SetPropertiesToAll();
	virtual ~SetPropertiesToAll() override;

	GBBASIC_CLASS_TYPE('S', 'P', 'A', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Layered::Layered::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Layered::Layered::undo;

	SetPropertiesToAll* with(Getter get, Setter set);
	SetPropertiesToAll* with(const LayeredPoints &points_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetFigure : public Command {
public:
	GBBASIC_PROPERTY(int, figure)

	GBBASIC_PROPERTY(int, old)

public:
	SetFigure();
	virtual ~SetFigure() override;

	GBBASIC_CLASS_TYPE('S', 'F', 'G', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetFigure* with(int n);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

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

	GBBASIC_CLASS_TYPE('S', 'N', 'M', 'A')

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

class SetRoutines : public Command {
public:
	GBBASIC_PROPERTY(std::string, updateRoutine)
	GBBASIC_PROPERTY(std::string, onHitsRoutine)

	GBBASIC_PROPERTY(std::string, oldUpdateRoutine)
	GBBASIC_PROPERTY(std::string, oldOnHitsRoutine)

public:
	SetRoutines();
	virtual ~SetRoutines() override;

	GBBASIC_CLASS_TYPE('S', 'R', 'T', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetRoutines* with(const std::string &m, const std::string &n);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetViewAs : public Command {
public:
	GBBASIC_PROPERTY(bool, asActor)

	GBBASIC_PROPERTY(bool, old)

public:
	SetViewAs();
	virtual ~SetViewAs() override;

	GBBASIC_CLASS_TYPE('S', 'V', 'A', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetViewAs* with(bool v);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetDefinition : public Command {
public:
	GBBASIC_PROPERTY(active_t, definition)

	GBBASIC_PROPERTY(active_t, old)

public:
	SetDefinition();
	virtual ~SetDefinition() override;

	GBBASIC_CLASS_TYPE('S', 'D', 'F', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetDefinition* with(const active_t &def);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	virtual bool isSimilarTo(const Command* other) const override;
	virtual bool mergeWith(const Command* other) override;
	UInt64 modificationBits(void) const;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Resize : public Layered::Layered {
public:
	GBBASIC_PROPERTY(Math::Vec2i, size)
	GBBASIC_PROPERTY(Math::Vec2i, propertiesSize)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(int, propertiesBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldPixels)
	GBBASIC_PROPERTY(Bytes::Ptr, oldProperties)
	GBBASIC_PROPERTY(Math::Vec2i, oldAnchor)

public:
	Resize();
	virtual ~Resize() override;

	GBBASIC_CLASS_TYPE('R', 'S', 'Z', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Resize* with(const Math::Vec2i &size, const Math::Vec2i &propSize);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetAnchor : public Layered::Layered {
public:
	GBBASIC_PROPERTY(Math::Vec2i, anchor)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Math::Vec2i, old)

public:
	SetAnchor();
	virtual ~SetAnchor() override;

	GBBASIC_CLASS_TYPE('S', 'A', 'C', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetAnchor* with(const Math::Vec2i &anchor);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetAnchorForAll : public Layered::Layered {
public:
	typedef std::vector<Math::Vec2i> Points;

public:
	GBBASIC_PROPERTY(Math::Vec2i, anchor)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Points, old)

public:
	SetAnchorForAll();
	virtual ~SetAnchorForAll() override;

	GBBASIC_CLASS_TYPE('S', 'A', 'A', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetAnchorForAll* with(const Math::Vec2i &anchor);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Import : public Layered::Layered {
public:
	GBBASIC_PROPERTY(Indexed::Ptr, palette)
	GBBASIC_PROPERTY(::Actor::Ptr, actor)
	GBBASIC_PROPERTY(int, figure)
	GBBASIC_PROPERTY(bool, asActor)
	GBBASIC_PROPERTY(active_t, definition)

	GBBASIC_PROPERTY(int, actorBytes)
	GBBASIC_PROPERTY(Bytes::Ptr, oldActor)
	GBBASIC_PROPERTY(int, oldFigure)
	GBBASIC_PROPERTY(bool, oldAsActor)
	GBBASIC_PROPERTY(active_t, oldDefinition)

public:
	Import();
	virtual ~Import() override;

	GBBASIC_CLASS_TYPE('I', 'M', 'P', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Import* with(Indexed::Ptr plt, const ::Actor::Ptr &act, int figure, bool asActor, const active_t &def);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ImportFrame : public Layered::Layered {
public:
	GBBASIC_PROPERTY(Image::Ptr, image)
	GBBASIC_PROPERTY(int, frame)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	ImportFrame();
	virtual ~ImportFrame() override;

	GBBASIC_CLASS_TYPE('I', 'M', 'F', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual ImportFrame* with(const Image::Ptr &img, int frame_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_ACTOR_H__ */
