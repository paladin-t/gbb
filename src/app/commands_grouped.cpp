/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_grouped.h"
#include "project.h"
#include "../utils/assets.h"
#include "../utils/editable.h"

/*
** {===========================================================================
** Grouped commands
*/

namespace Commands {

namespace Grouped {

BeginGroup::BeginGroup() {
}

Command* BeginGroup::redo(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	// Do nothing.

	return this;
}

Command* BeginGroup::undo(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	// Do nothing.

	return this;
}

BeginGroup* BeginGroup::with(const std::string &name_) {
	name(name_);

	return this;
}

Command* BeginGroup::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

EndGroup::EndGroup() {
}

Command* EndGroup::redo(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	// Do nothing.

	return this;
}

Command* EndGroup::undo(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	// Do nothing.

	return this;
}

EndGroup* EndGroup::with(const std::string &name_) {
	name(name_);

	return this;
}

Command* EndGroup::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Reference::Reference() {
	project(nullptr);
	category((unsigned)AssetsBundle::Categories::NONE);
	page(-1);
	id(0);
	valid(false);
}

Command* Reference::redo(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	BaseAssets::Entry* entry = nullptr;
	switch ((AssetsBundle::Categories)category()) {
	case AssetsBundle::Categories::TILES:
		entry = project()->getTiles(page());

		break;
	default:
		GBBASIC_ASSERT(false && "Not implemented.");

		break;
	}

	if (!entry)
		return this;
	if (!entry->editor)
		return this;

	entry->editor->post(Editable::Messages::REDO, (Variant::Long)id());

	return this;
}

Command* Reference::undo(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	BaseAssets::Entry* entry = nullptr;
	switch ((AssetsBundle::Categories)category()) {
	case AssetsBundle::Categories::TILES:
		entry = project()->getTiles(page());

		break;
	default:
		GBBASIC_ASSERT(false && "Not implemented.");

		break;
	}

	if (!entry)
		return this;
	if (!entry->editor)
		return this;

	entry->editor->post(Editable::Messages::UNDO, (Variant::Long)id());

	return this;
}

Reference* Reference::with(Project* prj, unsigned cat, int pg, int id_) {
	project(prj);
	category(cat);
	page(pg);
	id(id_);
	valid(true);

	return this;
}

Command* Reference::exec(Object::Ptr /* obj */, int /* argc */, const Variant* /* argv */) {
	// Do nothing.

	return this;
}

}

}

/* ===========================================================================} */
