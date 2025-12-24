/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_layered.h"

/*
** {===========================================================================
** Layered commands
*/

namespace Commands {

namespace Layered {

Layered::Layered() {
	sub(0);
}

Command* Layered::redo(Object::Ptr, int, const Variant*) {
	if (setSub())
		setSub()(sub());

	return this;
}

Command* Layered::undo(Object::Ptr, int, const Variant*) {
	if (setSub())
		setSub()(sub());

	return this;
}

Layered* Layered::with(SubSetter set, int sub_) {
	setSub(set);
	sub(sub_);

	return this;
}

}

}

/* ===========================================================================} */
