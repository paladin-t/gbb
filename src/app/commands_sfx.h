/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_SFX_H__
#define __COMMANDS_SFX_H__

#include "commands_layered.h"
#include "../utils/assets.h"

/*
** {===========================================================================
** Forward declaration
*/

class Window;
class Renderer;
class Workspace;

/* ===========================================================================} */

/*
** {===========================================================================
** Sfx commands
*/

namespace Commands {

namespace Sfx {

class AddPage : public Layered::Layered {
public:
	GBBASIC_PROPERTY_PTR(Window, window) // Foreign.
	GBBASIC_PROPERTY_PTR(Renderer, renderer) // Foreign.
	GBBASIC_PROPERTY_PTR(Workspace, workspace) // Foreign.
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(::Sfx::Sound, sound)

	GBBASIC_PROPERTY(bool, filled)

public:
	AddPage();
	virtual ~AddPage() override;

	GBBASIC_CLASS_TYPE('A', 'D', 'P', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual AddPage* with(Window* wnd, Renderer* rnd, Workspace* ws, int idx, const ::Sfx::Sound &snd);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeletePage : public Layered::Layered {
public:
	GBBASIC_PROPERTY_PTR(Window, window) // Foreign.
	GBBASIC_PROPERTY_PTR(Renderer, renderer) // Foreign.
	GBBASIC_PROPERTY_PTR(Workspace, workspace) // Foreign.
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(::Sfx::Sound, sound)

	GBBASIC_PROPERTY(bool, filled)

public:
	DeletePage();
	virtual ~DeletePage() override;

	GBBASIC_CLASS_TYPE('D', 'L', 'P', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual DeletePage* with(Window* wnd, Renderer* rnd, Workspace* ws, int idx, const ::Sfx::Sound &snd, bool filled_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteAllPages : public Layered::Layered {
public:
	GBBASIC_PROPERTY_PTR(Window, window) // Foreign.
	GBBASIC_PROPERTY_PTR(Renderer, renderer) // Foreign.
	GBBASIC_PROPERTY_PTR(Workspace, workspace) // Foreign.
	GBBASIC_PROPERTY(::Sfx::Sound::Array, sounds)

public:
	DeleteAllPages();
	virtual ~DeleteAllPages() override;

	GBBASIC_CLASS_TYPE('D', 'L', 'A', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual DeleteAllPages* with(Window* wnd, Renderer* rnd, Workspace* ws, const ::Sfx::Sound::Array &snds);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class MoveBackward : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)

public:
	MoveBackward();
	virtual ~MoveBackward() override;

	GBBASIC_CLASS_TYPE('M', 'B', 'W', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual MoveBackward* with(int index);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class MoveForward : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)

public:
	MoveForward();
	virtual ~MoveForward() override;

	GBBASIC_CLASS_TYPE('M', 'F', 'W', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual MoveForward* with(int index);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetName : public Layered::Layered {
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

	virtual SetName* with(const std::string &txt);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetSound : public Layered::Layered {
public:
	GBBASIC_PROPERTY(::Sfx::Sound, wave)

	GBBASIC_PROPERTY(::Sfx::Sound, old)

public:
	SetSound();
	virtual ~SetSound() override;

	GBBASIC_CLASS_TYPE('S', 'S', 'N', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetSound* with(const ::Sfx::Sound &wav);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Import : public Layered::Layered {
public:
	GBBASIC_PROPERTY(::Sfx::Ptr, sfx)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

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

	virtual Import* with(const ::Sfx::Ptr &sfx_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ImportAsNew : public Layered::Layered {
public:
	GBBASIC_PROPERTY_PTR(Window, window) // Foreign.
	GBBASIC_PROPERTY_PTR(Renderer, renderer) // Foreign.
	GBBASIC_PROPERTY_PTR(Workspace, workspace) // Foreign.
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(::Sfx::Ptr, sfx)

public:
	ImportAsNew();
	virtual ~ImportAsNew() override;

	GBBASIC_CLASS_TYPE('I', 'A', 'N', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual ImportAsNew* with(Window* wnd, Renderer* rnd, Workspace* ws, int idx, const ::Sfx::Ptr &sfx_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_SFX_H__ */
