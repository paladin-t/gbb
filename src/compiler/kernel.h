/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "../gbbasic.h"
#include "../utils/entry.h"
#include "../utils/localization.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef KERNEL_SYSTEM_BINARIES_DIR
#	define KERNEL_SYSTEM_BINARIES_DIR "../kernels/" /* Relative path. */
#endif /* KERNEL_SYSTEM_BINARIES_DIR */
#ifndef KERNEL_USER_BINARIES_DIR
#	define KERNEL_USER_BINARIES_DIR "kernels" /* Relative path. */
#endif /* KERNEL_USER_BINARIES_DIR */

#ifndef KERNEL_SNIPPET_TYPE_SEPARATOR
#	define KERNEL_SNIPPET_TYPE_SEPARATOR "separator"
#endif /* KERNEL_SNIPPET_TYPE_SEPARATOR */

#ifndef KERNEL_BEHAVIOUR_TYPE_HIDDEN
#	define KERNEL_BEHAVIOUR_TYPE_HIDDEN "hidden"
#endif /* KERNEL_BEHAVIOUR_TYPE_HIDDEN */
#ifndef KERNEL_BEHAVIOUR_TYPE_PLAYER
#	define KERNEL_BEHAVIOUR_TYPE_PLAYER "player"
#endif /* KERNEL_BEHAVIOUR_TYPE_PLAYER */

/* ===========================================================================} */

/*
** {===========================================================================
** Kernel
*/

namespace GBBASIC {

class Kernel : public virtual Object {
public:
	typedef std::shared_ptr<Kernel> Ptr;
	typedef std::vector<Ptr> Array;

	struct Snippet {
		typedef std::vector<Snippet> Array;

		std::string type;
		std::string content;
		Localization::Dictionary name;

		bool isSeparator = false;

		Snippet();
		Snippet(const std::string &y, const std::string &c, const Localization::Dictionary &dict);
	};

	struct Animations {
		typedef std::vector<Animations> Array;
		typedef std::vector<Localization::Dictionary> Names;

		std::string type;
		Names names;

		Animations();
		Animations(const std::string &y, const Names &names_);
	};

	struct Behaviour {
		typedef std::vector<Behaviour> Array;

		std::string type;
		std::string id;
		int value = 0;
		std::string syntax;
		Localization::Dictionary name;
		int animation = -1;

		Behaviour();
		Behaviour(const std::string &y, const std::string &id_, int val, const std::string &syn, const Localization::Dictionary &dict, int anim);
	};

public:
	GBBASIC_PROPERTY(std::string, path)
	GBBASIC_PROPERTY_READONLY(bool, readonly)
	GBBASIC_PROPERTY(std::string, id)
	GBBASIC_PROPERTY(Entry, entry)
	GBBASIC_PROPERTY(Localization::Dictionary, title)
	GBBASIC_PROPERTY(Localization::Dictionary, description)
	GBBASIC_PROPERTY(std::string, kernelRom)
	GBBASIC_PROPERTY(std::string, kernelSymbols)
	GBBASIC_PROPERTY(std::string, kernelAliases)
	GBBASIC_PROPERTY(std::string, kernelSourceCode)
	GBBASIC_PROPERTY(int, bootstrapBank)
	GBBASIC_PROPERTY(std::string, memoryHeapSize)
	GBBASIC_PROPERTY(std::string, memoryStackSize)
	GBBASIC_PROPERTY(int, objectsMaxActorCount)
	GBBASIC_PROPERTY(int, objectsMaxTriggerCount)
	GBBASIC_PROPERTY(Snippet::Array, snippets)
	GBBASIC_PROPERTY(Animations::Array, animations)
	GBBASIC_PROPERTY(int, projectileAnimationIndex)
	GBBASIC_PROPERTY(Behaviour::Array, behaviours)
	// `properties` and `natives` are not stored in this `Kernel` structure.

public:
	GBBASIC_CLASS_TYPE('K', 'R', 'N', 'L')

	Kernel();
	virtual ~Kernel() override;

	virtual unsigned type(void) const override;

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Object** ptr) const override;

	bool open(const char* path);
	bool close(void);
};

}

/* ===========================================================================} */

#endif /* __KERNEL_H__ */
