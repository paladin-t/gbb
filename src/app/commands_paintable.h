/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_PAINTABLE_H__
#define __COMMANDS_PAINTABLE_H__

#include "commands_layered.h"
#include "editing.h"

/*
** {===========================================================================
** Paintable commands
*/

namespace Commands {

namespace Paintable {

class Paint : public Layered::Layered {
public:
	typedef std::vector<Math::Vec2i> Points;
	typedef std::vector<Colour> Colours;
	typedef std::vector<int> Indices;

	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> Getter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> Setter;

	typedef std::function<void(const Math::Vec2i &, const Editing::Dot &)> Populator;

	typedef std::function<void(int, int)> Plotter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY_READONLY(Editing::Tools::BitwiseOperations, bitwiseOperation)
	GBBASIC_PROPERTY_READONLY(int, penSize)
	GBBASIC_PROPERTY(Editing::Point::Set, points)

	GBBASIC_PROPERTY(Editing::Point::Set, old)

public:
	Paint();
	virtual ~Paint() override;

	virtual const Editing::Dot* find(const Editing::Point &pos) const;
	virtual int populated(Populator populator) const;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Paint* with(Editing::Tools::BitwiseOperations bitOp);
	virtual Paint* with(Getter get, Setter set, int penSize);
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter);
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter);
	virtual Paint* with(const Math::Vec2i &validSize, const Points &points, const Colours &values);
	virtual Paint* with(const Math::Vec2i &validSize, const Points &points, const Indices &values);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

protected:
	void plot(int x, int y, Plotter proc);
};

class Pencil : public Paint {
public:
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, lastPosition)

public:
	Pencil();
	virtual ~Pencil() override;

	GBBASIC_CLASS_TYPE('P', 'P', 'C', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Line : public Paint {
public:
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	Line();
	virtual ~Line() override;

	GBBASIC_CLASS_TYPE('P', 'L', 'N', 'E')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Box : public Paint {
public:
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	Box();
	virtual ~Box() override;

	GBBASIC_CLASS_TYPE('P', 'B', 'O', 'X')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class BoxFill : public Paint {
public:
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	BoxFill();
	virtual ~BoxFill() override;

	GBBASIC_CLASS_TYPE('P', 'B', 'X', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Ellipse : public Paint {
public:
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	Ellipse();
	virtual ~Ellipse() override;

	GBBASIC_CLASS_TYPE('P', 'L', 'P', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class EllipseFill : public Paint {
public:
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	EllipseFill();
	virtual ~EllipseFill() override;

	GBBASIC_CLASS_TYPE('P', 'L', 'P', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Fill : public Paint {
public:
	Fill();
	virtual ~Fill() override;

	GBBASIC_CLASS_TYPE('P', 'F', 'I', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	Paint* with(Getter get, Setter set);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, const Colour &col);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, int idx);
	using Paint::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Paint::exec;
};

class Replace : public Paint {
public:
	Replace();
	virtual ~Replace() override;

	GBBASIC_CLASS_TYPE('P', 'R', 'P', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	Paint* with(Getter get, Setter set);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, const Colour &col);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, int idx);
	using Paint::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Paint::exec;
};

class Blit : public Layered::Layered {
public:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> Getter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> Setter;

	typedef std::function<void(const Math::Vec2i &, const Editing::Dot &)> Populator;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY_READONLY(Math::Recti, area)
	GBBASIC_PROPERTY(Editing::Dot::Array, dots)

	GBBASIC_PROPERTY(Editing::Dot::Array, old)

public:
	Blit();
	virtual ~Blit() override;

	virtual int populated(Populator populator) const;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Blit* with(Getter get, Setter set);
	virtual Blit* with(int dir);
	virtual Blit* with(const Math::Recti &area);
	virtual Blit* with(const Math::Recti &area, const Editing::Dot::Array &dots);
	virtual Blit* with(const Math::Recti &area, const Colour* col);
	virtual Blit* with(const Math::Recti &area, const int* idx);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class Stamp : public Blit {
public:
	Stamp();
	virtual ~Stamp() override;

	GBBASIC_CLASS_TYPE('P', 'S', 'T', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

class Rotate : public Blit {
public:
	GBBASIC_PROPERTY_READONLY(int, direction)

public:
	Rotate();
	virtual ~Rotate() override;

	GBBASIC_CLASS_TYPE('P', 'R', 'O', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Blit* with(int dir) override;
	virtual Blit* with(const Math::Recti &area) override;
	using Blit::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Blit::exec;
};

class Flip : public Blit {
public:
	GBBASIC_PROPERTY_READONLY(int, direction)

public:
	Flip();
	virtual ~Flip() override;

	GBBASIC_CLASS_TYPE('P', 'F', 'L', 'P')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Blit* with(int dir) override;
	virtual Blit* with(const Math::Recti &area) override;
	using Blit::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Blit::exec;
};

class Cut : public Blit {
public:
	Cut();
	virtual ~Cut() override;

	GBBASIC_CLASS_TYPE('P', 'C', 'U', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

class Paste : public Blit {
public:
	Paste();
	virtual ~Paste() override;

	GBBASIC_CLASS_TYPE('P', 'P', 'S', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

class Delete : public Blit {
public:
	Delete();
	virtual ~Delete() override;

	GBBASIC_CLASS_TYPE('P', 'D', 'E', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_PAINTABLE_H__ */
