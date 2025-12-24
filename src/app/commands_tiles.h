/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_TILES_H__
#define __COMMANDS_TILES_H__

#include "commands_paintable.h"

/*
** {===========================================================================
** Tiles commands
*/

namespace Commands {

namespace Tiles {

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

class SetName : public Command {
public:
	GBBASIC_PROPERTY(std::string, name)

	GBBASIC_PROPERTY(std::string, old)

public:
	SetName();
	virtual ~SetName() override;

	GBBASIC_CLASS_TYPE('S', 'N', 'M', 'I')

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


class Resize : public Command {
public:
	GBBASIC_PROPERTY(Math::Vec2i, size)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	Resize();
	virtual ~Resize() override;

	GBBASIC_CLASS_TYPE('R', 'S', 'Z', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Resize* with(const Math::Vec2i &size);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Pitch : public Command {
public:
	GBBASIC_PROPERTY(int, width) // In pixels.
	GBBASIC_PROPERTY(int, height) // In pixels.
	GBBASIC_PROPERTY(int, pitch) // In pixels.
	GBBASIC_PROPERTY(int, maxHeight) // In pixels.

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	Pitch();
	virtual ~Pitch() override;

	GBBASIC_CLASS_TYPE('P', 'C', 'H', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Pitch* with(int width_, int height_, int pitch_, int maxHeight_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Import : public Command {
public:
	GBBASIC_PROPERTY(Image::Ptr, image)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	Import();
	virtual ~Import() override;

	GBBASIC_CLASS_TYPE('I', 'M', 'P', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Import* with(const Image::Ptr &img);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_TILES_H__ */
