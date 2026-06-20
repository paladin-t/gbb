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
#include "../utils/text.h"

/*
** {===========================================================================
** I18n commands
*/

namespace Commands {

namespace I18n {

class AddItem : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(std::string, item)

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

	virtual AddItem* with(int index_);
	virtual AddItem* with(int index_, const std::string &item_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteItem : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(Text::Array, old)

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

	virtual DeleteItem* with(int index_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SwapItems : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index0)
	GBBASIC_PROPERTY(int, index1)

public:
	SwapItems();
	virtual ~SwapItems() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'I', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SwapItems* with(int index0_, int index1_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class AddLanguage : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(std::string, language)

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

	virtual AddLanguage* with(int index_, const std::string &lang);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteLanguage : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(std::string, oldLanguage)
	GBBASIC_PROPERTY(Text::Array, oldColumn)

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

	virtual DeleteLanguage* with(int index_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SwapLanguages : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index0)
	GBBASIC_PROPERTY(int, index1)

public:
	SwapLanguages();
	virtual ~SwapLanguages() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'L', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SwapLanguages* with(int index0_, int index1_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class RenameLanguage : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(std::string, language)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(std::string, old)

public:
	RenameLanguage();
	virtual ~RenameLanguage() override;

	GBBASIC_CLASS_TYPE('R', 'N', 'L', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual RenameLanguage* with(int index_, const std::string &lang);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ChangeContent : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(int, language)
	GBBASIC_PROPERTY(std::string, content)

	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(std::string, old)

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

	virtual ChangeContent* with(int lang, int index_, const std::string &newVal_);
	using Layered::Layered::with;

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

class Import : public Layered::Layered {
public:
	GBBASIC_PROPERTY(::I18n::Ptr, i18n)

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

	virtual Import* with(const ::I18n::Ptr &i18n_);
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
