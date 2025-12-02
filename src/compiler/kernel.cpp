/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "kernel.h"
#include "../utils/assets.h"
#include "../utils/file_handle.h"
#include "../utils/filesystem.h"
#include "../../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Kernel
*/

namespace GBBASIC {

Kernel::Snippet::Snippet() {
}

Kernel::Snippet::Snippet(const std::string &y, const std::string &c, const Localization::Dictionary &dict) :
	type(y), content(c), name(dict)
{
}

Kernel::Animations::Animations() {
}

Kernel::Animations::Animations(const std::string &y, const Names &names_) : type(y), names(names_) {
}

Kernel::Behaviour::Behaviour() {
}

Kernel::Behaviour::Behaviour(const std::string &y, const std::string &id_, int val, const std::string &syn, const Localization::Dictionary &dict, int anim) :
	type(y), id(id_), value(val), syntax(syn), name(dict), animation(anim)
{
}

Kernel::Kernel() {
	readonly(false);
	bootstrapBank(0);
	objectsMaxActorCount(ASSETS_ACTOR_MAX_COUNT);
	objectsMaxTriggerCount(ASSETS_TRIGGER_MAX_COUNT);
	projectileAnimationIndex(-1);
}

Kernel::~Kernel() {
}

unsigned Kernel::type(void) const {
	return TYPE();
}

bool Kernel::clone(Object** ptr) const { // Non-clonable.
	if (ptr)
		*ptr = nullptr;

	return false;
}

bool Kernel::open(const char* path_) {
	// Read from file.
	File::Ptr file(File::create());
	if (!file->open(path_, Stream::READ))
		return false;
	std::string content;
	if (!file->readString(content)) {
		file->close();

		return false;
	}
	file->close();
	file = nullptr;

	rapidjson::Document doc;
	if (!Json::fromString(doc, content.c_str()))
		return false;

	// Parse the properties.
	bool readonly_ = false;
	std::string id_;
	Localization::Dictionary title_;
	std::string kernelRom_;
	std::string kernelSymbols_;
	std::string kernelAliases_;
	std::string kernelSourceCode_;
	int bootstrapBank_ = 0;
	std::string memoryHeapSize_;
	std::string memoryStackSize_;
	int objectsMaxActorCount_ = ASSETS_ACTOR_MAX_COUNT;
	int objectsMaxTriggerCount_ = ASSETS_TRIGGER_MAX_COUNT;
	Snippet::Array snippets_;
	Animations::Array animations_;
	int projectileAnimationIndex_ = -1;
	Behaviour::Array behaviours_;

	const std::string writableDir = Path::writableDirectory();
	const std::string userPath = Path::combine(writableDir.c_str(), KERNEL_USER_BINARIES_DIR);
	const std::string absdir0 = Path::absoluteOf(userPath);
	const std::string abspath0 = Path::absoluteOf(path_);
	readonly_ = !Path::isParentOf(absdir0.c_str(), abspath0.c_str());

	if (!Jpath::get(doc, id_, "id"))
		return false;

	rapidjson::Value* val = nullptr;
	if (!Jpath::get(doc, val, "title") || !val)
		return false;
	if (!Localization::parse(title_, *val))
		return false;

	if (!Jpath::get(doc, kernelRom_, "kernel", "rom"))
		return false;
	if (!Jpath::get(doc, kernelSymbols_, "kernel", "symbols"))
		return false;
	if (!Jpath::get(doc, kernelAliases_, "kernel", "aliases"))
		return false;
	if (!Jpath::get(doc, kernelSourceCode_, "kernel", "source_code"))
		return false;

	if (!Jpath::get(doc, bootstrapBank_, "bootstrap", "bank"))
		return false;

	if (!Jpath::get(doc, memoryHeapSize_, "memory", "heap_size"))
		return false;
	if (!Jpath::get(doc, memoryStackSize_, "memory", "stack_size"))
		return false;

	if (!Jpath::get(doc, objectsMaxActorCount_, "objects", "mac_actor_count"))
		objectsMaxActorCount_ = ASSETS_ACTOR_MAX_COUNT;
	if (!Jpath::get(doc, objectsMaxTriggerCount_, "objects", "mac_trigger_count"))
		objectsMaxTriggerCount_ = ASSETS_TRIGGER_MAX_COUNT;

	if (Jpath::has(doc, "snippets")) {
		const int n = Jpath::count(doc, "snippets");
		for (int i = 0; i < n; ++i) {
			std::string y;
			if (!Jpath::get(doc, y, "snippets", i, "type"))
				return false;

			std::string val_;
			const Jpath::Types type = Jpath::typeOf(doc, "snippets", i, "content");
			if (type == Jpath::STRING) {
				if (!Jpath::get(doc, val_, "snippets", i, "content"))
					return false;
			} else {
				const int m = Jpath::count(doc, "snippets", i, "content");
				for (int j = 0; j < m; ++j) {
					std::string ln;
					if (Jpath::get(doc, ln, "snippets", i, "content", j)) {
						if (!val_.empty())
							val_ += "\n";
						val_ += ln;
					}
				}
			}

			Localization::Dictionary name;
			val = nullptr;
			if (!Jpath::get(doc, val, "snippets", i, "name") || !val)
				return false;
			if (!Localization::parse(name, *val))
				return false;

			Snippet snippet(y, val_, name);
			if (y == KERNEL_SNIPPET_TYPE_SEPARATOR)
				snippet.isSeparator = true;
			snippets_.push_back(snippet);
		}
	}

	if (Jpath::has(doc, "animations")) {
		const int n = Jpath::count(doc, "animations");
		for (int i = 0; i < n; ++i) {
			std::string y;
			if (!Jpath::get(doc, y, "animations", i, "type"))
				return false;

			Animations::Names names;
			if (!Jpath::has(doc, "animations", i, "names") || Jpath::typeOf(doc, "animations", i, "names") != Jpath::ARRAY)
				return false;

			const int m = Jpath::count(doc, "animations", i, "names");
			for (int j = 0; j < m; ++j) {
				Localization::Dictionary name;
				val = nullptr;
				if (!Jpath::get(doc, val, "animations", i, "names", j) || !val)
					return false;
				if (!Localization::parse(name, *val))
					return false;

				names.push_back(name);
			}
			names.shrink_to_fit();

			animations_.push_back(Animations(y, names));
		}
	}
	auto findAnimationByType = [&animations_] (const std::string &y) -> int {
		for (int i = 0; i < (int)animations_.size(); ++i) {
			const Animations &anim = animations_[i];
			if (anim.type == y)
				return i;
		}

		return -1;
	};
	projectileAnimationIndex_ = findAnimationByType("projectile");

	if (Jpath::has(doc, "behaviours")) {
		const int n = Jpath::count(doc, "behaviours");
		for (int i = 0; i < n; ++i) {
			std::string y;
			if (!Jpath::get(doc, y, "behaviours", i, "type"))
				return false;

			std::string id;
			if (!Jpath::get(doc, id, "behaviours", i, "id"))
				return false;

			const bool hidden = y == KERNEL_BEHAVIOUR_TYPE_HIDDEN; // Is hidden.
			if (hidden)
				continue;

			int val_ = 0;
			if (Jpath::typeOf(doc, "behaviours", i, "value") == Jpath::NUMBER) {
				if (!Jpath::get(doc, val_, "behaviours", i, "value"))
					return false;
			} else {
				std::string sval;
				if (!Jpath::get(doc, sval, "behaviours", i, "value"))
					return false;

				if (!Text::fromString(sval, val_))
					return false;
			}

			std::string syn;
			switch (Jpath::typeOf(doc, "behaviours", i, "syntax")) {
			case Jpath::NIL:
				// Do nothing.

				break;
			case Jpath::STRING:
				if (!Jpath::get(doc, syn, "behaviours", i, "syntax"))
					return false;

				break;
			default:
				// Wrong data.

				return false;
			}

			Localization::Dictionary name;
			val = nullptr;
			if (!Jpath::get(doc, val, "behaviours", i, "name") || !val)
				return false;
			if (!Localization::parse(name, *val))
				return false;

			int animIndex = -1;
			std::string animation;
			if (Jpath::typeOf(doc, "behaviours", i, "animation") != Jpath::STRING || !Jpath::get(doc, animation, "behaviours", i, "animation") || animation.empty())
				animIndex = -1;
			else
				animIndex = findAnimationByType(animation);

			behaviours_.push_back(Behaviour(y, id, val_, syn, name, animIndex));
		}
	}

	snippets_.shrink_to_fit();
	animations_.shrink_to_fit();
	behaviours_.shrink_to_fit();

	path(path_);
	readonly(readonly_);
	id(id_);
	title(title_);
	kernelRom(kernelRom_);
	kernelSymbols(kernelSymbols_);
	kernelAliases(kernelAliases_);
	kernelSourceCode(kernelSourceCode_);
	bootstrapBank(bootstrapBank_);
	memoryHeapSize(memoryHeapSize_);
	memoryStackSize(memoryStackSize_);
	objectsMaxActorCount(objectsMaxActorCount_);
	objectsMaxTriggerCount(objectsMaxTriggerCount_);
	snippets(snippets_);
	animations(animations_);
	projectileAnimationIndex(projectileAnimationIndex_);
	behaviours(behaviours_);

	// Setup the entry.
	std::string entry_;
	const char* entrystr = Localization::get(title_);
	if (entrystr)
		entry_ = entrystr;
	entry(Entry(entry_));

	// Finish.
	return true;
}

bool Kernel::close(void) {
	// Clear the properties.
	path().clear();
	id().clear();
	entry().clear();
	title().clear();
	kernelRom().clear();
	kernelSymbols().clear();
	kernelAliases().clear();
	kernelSourceCode().clear();
	bootstrapBank(0);
	memoryHeapSize().clear();
	memoryStackSize().clear();
	objectsMaxActorCount(ASSETS_ACTOR_MAX_COUNT);
	objectsMaxTriggerCount(ASSETS_TRIGGER_MAX_COUNT);
	snippets().clear();
	animations().clear();
	projectileAnimationIndex(-1);
	behaviours().clear();

	// Finish.
	return true;
}

}

/* ===========================================================================} */
