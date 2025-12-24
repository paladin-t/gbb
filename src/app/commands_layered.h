/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_LAYERED_H__
#define __COMMANDS_LAYERED_H__

#include "command.h"

/*
** {===========================================================================
** Layered commands
*/

namespace Commands {

namespace Layered {

class Layered : public Command {
public:
	typedef std::function<void(int)> SubSetter;

public:
	GBBASIC_PROPERTY_READONLY(SubSetter, setSub)
	GBBASIC_PROPERTY_READONLY(int, sub)

public:
	Layered();

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;

	virtual Layered* with(SubSetter set, int sub_);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_LAYERED_H__ */
