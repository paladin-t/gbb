/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_GROUPED_H__
#define __COMMANDS_GROUPED_H__

#include "command.h"

/*
** {===========================================================================
** Forward declaration
*/

class Project;

/* ===========================================================================} */

/*
** {===========================================================================
** Grouped commands
*/

namespace Commands {

namespace Grouped {

class BeginGroup : public Command {
public:
	GBBASIC_PROPERTY_READONLY(std::string, name)

public:
	BeginGroup();

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	BeginGroup* with(const std::string &name_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class EndGroup : public Command {
public:
	GBBASIC_PROPERTY_READONLY(std::string, name)

public:
	EndGroup();

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	EndGroup* with(const std::string &name_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class Reference : public Command {
public:
	GBBASIC_PROPERTY_READONLY_PTR(Project, project)
	GBBASIC_PROPERTY_READONLY(unsigned, category)
	GBBASIC_PROPERTY_READONLY(int, page)
	GBBASIC_PROPERTY_READONLY(unsigned, id)
	GBBASIC_PROPERTY(bool, valid)

public:
	Reference();

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Reference* with(Project* prj, unsigned cat, int pg, int id_);

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_GROUPED_H__ */
