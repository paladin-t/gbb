/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_I18N_H__
#define __COMMANDS_I18N_H__

#include "commands_layered.h"
#include "../utils/assets.h"

/*
** {===========================================================================
** I18n commands
*/

namespace Commands {

namespace I18n {

class AddItem : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	// TODO: i18n.

	GBBASIC_PROPERTY(bool, filled)

public:
	AddItem();
	virtual ~AddItem() override;

	GBBASIC_CLASS_TYPE('A', 'D', 'I', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	// TODO: i18n.
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteItem : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	// TODO: i18n.

	GBBASIC_PROPERTY(bool, filled)

public:
	DeleteItem();
	virtual ~DeleteItem() override;

	GBBASIC_CLASS_TYPE('D', 'L', 'I', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	// TODO: i18n.
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class AddLanguage : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	// TODO: i18n.

	GBBASIC_PROPERTY(bool, filled)

public:
	AddLanguage();
	virtual ~AddLanguage() override;

	GBBASIC_CLASS_TYPE('A', 'D', 'L', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	// TODO: i18n.
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteLanguage : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	// TODO: i18n.

	GBBASIC_PROPERTY(bool, filled)

public:
	DeleteLanguage();
	virtual ~DeleteLanguage() override;

	GBBASIC_CLASS_TYPE('D', 'L', 'L', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	// TODO: i18n.
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ChangeContent : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	// TODO: i18n.

	GBBASIC_PROPERTY(bool, filled)

public:
	ChangeContent();
	virtual ~ChangeContent() override;

	GBBASIC_CLASS_TYPE('C', 'G', 'C', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	// TODO: i18n.
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Import : public Layered::Layered {
public:
	// TODO: i18n.

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

	// TODO: i18n.
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_I18N_H__ */
