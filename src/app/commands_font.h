/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_FONT_H__
#define __COMMANDS_FONT_H__

#include "command.h"
#include "editing.h"

/*
** {===========================================================================
** Font commands
*/

namespace Commands {

namespace Font {

class SetContent : public Command {
public:
	GBBASIC_PROPERTY(std::string, content)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	SetContent();
	virtual ~SetContent() override;

	GBBASIC_CLASS_TYPE('S', 'C', 'T', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetContent* with(const std::string &content_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetName : public Command {
public:
	GBBASIC_PROPERTY(std::string, name)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(std::string, old)

public:
	SetName();
	virtual ~SetName() override;

	GBBASIC_CLASS_TYPE('S', 'N', 'M', 'F')

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

class ResizeInt : public Command {
public:
	GBBASIC_PROPERTY(int, size)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	ResizeInt();
	virtual ~ResizeInt() override;

	GBBASIC_CLASS_TYPE('R', 'S', 'I', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual ResizeInt* with(int size_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ResizeVec2 : public Command {
public:
	GBBASIC_PROPERTY(Math::Vec2i, size)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Math::Vec2i, old)

public:
	ResizeVec2();
	virtual ~ResizeVec2() override;

	GBBASIC_CLASS_TYPE('R', 'S', 'V', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual ResizeVec2* with(const Math::Vec2i &size_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ChangeMaxSizeVec2 : public Command {
public:
	GBBASIC_PROPERTY(Math::Vec2i, size)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Math::Vec2i, old)

public:
	ChangeMaxSizeVec2();
	virtual ~ChangeMaxSizeVec2() override;

	GBBASIC_CLASS_TYPE('C', 'M', 'V', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual ChangeMaxSizeVec2* with(const Math::Vec2i &size_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetTrim : public Command {
public:
	GBBASIC_PROPERTY(bool, toTrim)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(bool, old)

public:
	SetTrim();
	virtual ~SetTrim() override;

	GBBASIC_CLASS_TYPE('S', 'T', 'M', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetTrim* with(bool trim);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetOffset : public Command {
public:
	GBBASIC_PROPERTY(int, offset)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	SetOffset();
	virtual ~SetOffset() override;

	GBBASIC_CLASS_TYPE('S', 'O', 'F', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetOffset* with(int offset_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Set2Bpp : public Command {
public:
	GBBASIC_PROPERTY(bool, isTwoBitsPerPixel)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(bool, old)

public:
	Set2Bpp();
	virtual ~Set2Bpp() override;

	GBBASIC_CLASS_TYPE('S', '2', 'B', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Set2Bpp* with(bool twoBitsPerPixel_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetEffect : public Command {
public:
	GBBASIC_PROPERTY(int, enabledEffect)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	SetEffect();
	virtual ~SetEffect() override;

	GBBASIC_CLASS_TYPE('S', 'F', 'X', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetEffect* with(int enabledEffect_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetShadowParameters : public Command {
public:
	GBBASIC_PROPERTY(int, offset)
	GBBASIC_PROPERTY(float, strength)
	GBBASIC_PROPERTY(UInt8, direction)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, oldOffset)
	GBBASIC_PROPERTY(float, oldStrength)
	GBBASIC_PROPERTY(UInt8, oldDirection)

public:
	SetShadowParameters();
	virtual ~SetShadowParameters() override;

	GBBASIC_CLASS_TYPE('S', 'S', 'P', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetShadowParameters* with(int offset_, float strength_, UInt8 direction_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetOutlineParameters : public Command {
public:
	GBBASIC_PROPERTY(int, offset)
	GBBASIC_PROPERTY(float, strength)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, oldOffset)
	GBBASIC_PROPERTY(float, oldStrength)

public:
	SetOutlineParameters();
	virtual ~SetOutlineParameters() override;

	GBBASIC_CLASS_TYPE('S', 'O', 'P', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetOutlineParameters* with(int offset_, float strength_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetWordWrap : public Command {
public:
	GBBASIC_PROPERTY(bool, preferFullWord)
	GBBASIC_PROPERTY(bool, preferFullWordForNonAscii)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(bool, oldPreferFullWord)
	GBBASIC_PROPERTY(bool, oldPreferFullWordForNonAscii)

public:
	SetWordWrap();
	virtual ~SetWordWrap() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'W', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetWordWrap* with(bool preferFullWord_, bool preferFullWordForNonAscii_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetThresholds : public Command {
public:
	GBBASIC_PROPERTY(Math::Vec4i, thresholds)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Math::Vec4i, old)

public:
	SetThresholds();
	virtual ~SetThresholds() override;

	GBBASIC_CLASS_TYPE('S', 'T', 'H', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetThresholds* with(Math::Vec4i thresholds_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Invert : public Command {
public:
	GBBASIC_PROPERTY(bool, inverted)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(bool, old)

public:
	Invert();
	virtual ~Invert() override;

	GBBASIC_CLASS_TYPE('I', 'N', 'V', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Invert* with(bool inverted_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetArbitrary : public Command {
public:
	GBBASIC_PROPERTY(::Font::Codepoints, arbitrary)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(::Font::Codepoints, old)

public:
	SetArbitrary();
	virtual ~SetArbitrary() override;

	GBBASIC_CLASS_TYPE('S', 'A', 'R', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetArbitrary* with(const ::Font::Codepoints &arbitrary_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetRanges : public Command {
public:
	GBBASIC_PROPERTY(FontAssets::Entry::CodepointRanges, ranges)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(FontAssets::Entry::CodepointRanges, old)

public:
	SetRanges();
	virtual ~SetRanges() override;

	GBBASIC_CLASS_TYPE('S', 'R', 'N', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetRanges* with(const FontAssets::Entry::CodepointRanges &ranges_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_FONT_H__ */
