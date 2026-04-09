/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __STATIC_ANALYZER_H__
#define __STATIC_ANALYZER_H__

#include "../gbbasic.h"
#include "compiler.h"
#include "../utils/assets.h"
#include "../utils/work_queue.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef STATIC_ANALYZER_INTERVAL
#	define STATIC_ANALYZER_INTERVAL 5.0
#endif /* STATIC_ANALYZER_INTERVAL */

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

class Kernel;

/* ===========================================================================} */

/*
** {===========================================================================
** Static Analyzer
*/

namespace GBBASIC {

class StaticAnalyzer : public virtual Object {
public:
	typedef std::shared_ptr<StaticAnalyzer> Ptr;

	typedef std::function<Semaphore(WorkTaskFunction::OperationHandler/* on work thread */, WorkTaskFunction::SealHandler/* nullable */ /* on main thread */, WorkTaskFunction::DisposeHandler/* nullable */ /* on main thread */)> AnalyzeHandler;
	typedef std::function<void(void)> AnalyzedHandler;

	struct CodePageName {
		typedef std::vector<CodePageName> Array;

		std::string name;

		CodePageName();
		CodePageName(const std::string &n);
	};

public:
	GBBASIC_CLASS_TYPE('A', 'N', 'L', 'Z')

	virtual bool analyzing(void) const = 0;
	virtual bool analyze(const Kernel* krnl, AssetsBundle::Ptr assets, const std::string &preDefinedMacros, AnalyzedHandler analyzed) = 0;

	virtual unsigned getLanguegeDefinitionRevision(void) const = 0;

	virtual const Macro::List* getMacroDefinitions(void) const = 0;

	virtual const Text::Array* getDestinitions(void) const = 0;

	virtual const RamLocation::Dictionary* getRamAllocations(void) const = 0;

	virtual const CodePageName* getCodePageName(int page) const = 0;

	virtual void clear(void) = 0;

	static StaticAnalyzer* create(AnalyzeHandler analyzeHandler);
	static void destroy(StaticAnalyzer* ptr);
};

}

/* ===========================================================================} */

#endif /* __STATIC_ANALYZER_H__ */
