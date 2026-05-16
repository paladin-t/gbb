/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "kernel.h"
#include "static_analyzer.h"
#include "../utils/datetime.h"
#include "../utils/filesystem.h"
#include "../utils/plus.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Macros and constants
*/

#if GBBASIC_COMPILER_ANALYZER_ENABLED
#	pragma message("Static analyzer enabled.")
#endif /* GBBASIC_COMPILER_ANALYZER_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Static Analyzer
*/

namespace GBBASIC {

class StaticAnalyzerImpl : public StaticAnalyzer {
private:
	typedef std::vector<PreprocessorBranch::Array> PagedPreprocessorBranches;

	struct Result {
		Macro::List macrosDefinitions;
		Text::Array destinations;
		RamLocation::Dictionary ramAllocations;
		PagedPreprocessorBranches pagedPreprocessorBranches;
		CodePageName::Array codePageNames;
		std::string errors;

		Result() {
		}
	};

private:
	AnalyzeHandler _analyzeHandler = nullptr; // Foreign.
	int _analyzing = 0;

	unsigned _languageDefinitionRevision = 1;
	Macro::List _macrosDefinitions;
	Text::Array _destinations;
	RamLocation::Dictionary _ramAllocations;
	PagedPreprocessorBranches _pagedPreprocessorBranches;
	CodePageName::Array _codePageNames;
	std::string _errors;

public:
	StaticAnalyzerImpl(AnalyzeHandler analyzeHandler) : _analyzeHandler(analyzeHandler) {
	}
	virtual ~StaticAnalyzerImpl() override {
		_analyzeHandler = nullptr;
		_analyzing = 0;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual bool analyzing(void) const override {
		return !!_analyzing;
	}
	virtual bool analyze(const Kernel* krnl, AssetsBundle::Ptr assets, const std::string &preDefinedMacros, AnalyzedHandler analyzed) override {
		++_analyzing;

		_analyzeHandler(
			std::bind(
				[this] (WorkTask* /* task */, const Kernel* krnl, AssetsBundle::Ptr assets, const std::string &preDefinedMacros) -> uintptr_t { // On work thread.
					Result* result = new Result();
					doAnalyze(result, krnl, assets, preDefinedMacros);

					return (uintptr_t)result;
				},
				std::placeholders::_1, krnl, assets, preDefinedMacros
			),
			[this] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
				Result* result = (Result*)ptr;

				bool diff = false;
				if (!::equals(_macrosDefinitions, result->macrosDefinitions)) {
					_macrosDefinitions.clear();
					std::swap(_macrosDefinitions, result->macrosDefinitions);
					diff |= true;
				}

				if (!::equals(_destinations, result->destinations)) {
					_destinations.clear();
					std::swap(_destinations, result->destinations);
					_destinations.shrink_to_fit();
					diff |= true;
				}

				std::swap(_ramAllocations, result->ramAllocations);

				if (!::equals(_pagedPreprocessorBranches, result->pagedPreprocessorBranches)) {
					_pagedPreprocessorBranches.clear();
					std::swap(_pagedPreprocessorBranches, result->pagedPreprocessorBranches);
					_pagedPreprocessorBranches.shrink_to_fit();
					diff |= true;
				}

				std::swap(_codePageNames, result->codePageNames);
				_codePageNames.shrink_to_fit();

				std::swap(_errors, result->errors);

				if (diff) {
					if (++_languageDefinitionRevision == 0)
						_languageDefinitionRevision = 1;
				}
			},
			[this, analyzed] (WorkTask* task, uintptr_t ptr) -> void { // On main thread.
				Result* result = (Result*)ptr;
				delete result;

				task->disassociated(true);

				if (analyzed)
					analyzed();

				--_analyzing;
			}
		);

		return true;
	}

	virtual unsigned getLanguegeDefinitionRevision(void) const override {
		return _languageDefinitionRevision;
	}

	virtual const Macro::List* getMacroDefinitions(void) const override {
		return &_macrosDefinitions;
	}

	virtual const Text::Array* getDestinitions(void) const override {
		return &_destinations;
	}

	virtual const RamLocation::Dictionary* getRamAllocations(void) const override {
		return &_ramAllocations;
	}

	virtual const PreprocessorBranch::Array* getPreprocessorBranches(int page) const override {
		if (page < 0 || page >= (int)_pagedPreprocessorBranches.size())
			return nullptr;

		return &_pagedPreprocessorBranches[page];
	}

	virtual const CodePageName* getCodePageName(int page) const override {
		if (page < 0 || page >= (int)_codePageNames.size())
			return nullptr;

		return &_codePageNames[page];
	}

	virtual const std::string* getErrors(void) const override {
		if (_errors.empty())
			return nullptr;

		return &_errors;
	}

	virtual void clear(void) override {
		_languageDefinitionRevision = 1;
		_macrosDefinitions.clear();
		_destinations.clear();
		_ramAllocations.clear();
		_pagedPreprocessorBranches.clear();
		_codePageNames.clear();
		_errors.clear();
	}

private:
	static void doAnalyze(Result* result, const Kernel* krnl, const AssetsBundle::Ptr &assets, const std::string &preDefinedMacros) { // On work thread.
		// Prepare.
		std::string dir;
		Path::split(krnl->path(), nullptr, nullptr, &dir);
		const std::string rom = Path::combine(dir.c_str(), krnl->kernelRom().c_str());
		const std::string sym = Path::combine(dir.c_str(), krnl->kernelSymbols().c_str());
		const std::string aliases = Path::combine(dir.c_str(), krnl->kernelAliases().c_str());
		const int bootstrapBank = krnl->bootstrapBank();
		std::string &errors = result->errors;

		Program program;
		Options options;

		program.assets = assets;

		// Initialize the compiler options.
		options.config = krnl->path();
		options.rom = rom;
		options.sym = sym;
		options.aliases = aliases;
		options.macros = preDefinedMacros; // Inject pre-defined macros from the project properties.
		options.passes = Options::Passes::GENERATE; // Only parse the source code and generate for the first pass.
		options.strategies.compatibility = Options::Strategies::Compatibilities::COLORED | Options::Strategies::Compatibilities::EXTENSION;
		options.strategies.bootstrapBank = bootstrapBank;
		options.piping.useWorkQueue = false;
		options.piping.lessConsoleOutput = true;

		// Initialize the output methods.
		options.onPrint = [] (const std::string &msg) -> void {
			fprintf(stdout, "[ANALYZER INFO] %s\n", msg.c_str());
		};
		options.onError = [&errors, &program] (const std::string &msg, bool isWarning, int page, int row, int column) -> void {
			std::string msg_;
			if (row != -1 || column != -1) {
				if (page != -1) {
					msg_ += "Page ";
					msg_ += Text::toPageNumber(page);
					msg_ += ", ";
				}
				msg_ += "Ln ";
				msg_ += Text::toString(row + 1);
				msg_ += ", col ";
				msg_ += Text::toString(column + 1 + program.lineNumberWidth);
				msg_ += ": ";
			}
			msg_ += msg;

			if (!isWarning)
				errors += "  " + msg_ + "\n";

			if (isWarning)
				fprintf(stderr, "[ANALYZER WARNING] %s\n", msg_.c_str());
			else
				fprintf(stderr, "[ANALYZER ERROR] %s\n", msg_.c_str());
		};

		// Compile.
		fprintf(stdout, "[ANALYZER INFO] Start analyzing.\n");

		const long long start = DateTime::ticks();

		const bool codeIsOk =
			   load(program, options) &&
			compile(program, options);
		(void)codeIsOk;

		if (!errors.empty())
			errors = "Errors:\n" + errors;

		doAnalyzeMacroDefinitions(result, program);

		doAnalyzeRamAllocations(result, program);

		doAnalyzeProgram(result, program);

		doAnalyzePagedPreprocessorBranches(result, program);

		doAnalyzeCodePages(result, program);

		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		const std::string msg = "[ANALYZER INFO] Analyzed in " + Text::toString(secs) + "s.\n";
		fprintf(stdout, msg.c_str());

		fprintf(stdout, "[ANALYZER INFO] End analyzing.\n");
	}
	static void doAnalyzeMacroDefinitions(Result* result, Program &program) {
		result->macrosDefinitions.clear();
		std::swap(result->macrosDefinitions, program.compiled.macros);
	}
	static void doAnalyzeRamAllocations(Result* result, Program &program) {
		result->ramAllocations.clear();
		std::swap(result->ramAllocations, program.compiled.allocations);
	}
	static void doAnalyzeProgram(Result* result, Program &program) {
		// Prepare.
		const INode::Ptr &root = program.root; // Get the compiled AST, which could be corrupt.
		if (!root)
			return;

		// Select root.
		Select prog = Select(root);

		// Select destinations.
		Select destinations = prog
			.children(Where(INode::Types::DESTINATION).doFailIfNotAllMatch(false).doRecursive(true));
		if (destinations.ok()) {
			destinations
				.foreach(
					[&] (const Select &, const INode::Ptr &dest, int) -> void {
						const INode::Abstract abs = dest->abstract();
						if (!abs.empty()) {
							std::string name = abs.front();
							Text::toLowerCase(name);
							result->destinations.push_back(name);
						}
					}
				);
		}
	}
	static void doAnalyzePagedPreprocessorBranches(Result* result, Program &program) {
		// Prepare.
		const INode::Ptr &root = program.root; // Get the compiled AST, which could be corrupt.
		if (!root)
			return;

		// Select pages.
		Select pages = Select(root)
			.children(Where(INode::Types::PAGE));

		if (pages.ok()) {
			// Parse the preprocessor branches.
			result->pagedPreprocessorBranches.resize(pages.count());
			pages
				.foreach(
					[&] (const Select &, const INode::Ptr &page, int index) -> void {
						Select preprocIfs = Select(page)
							.children(Where(INode::Types::PREPROCESSOR_IF, false, true));
						if (!preprocIfs.ok())
							return;

						for (int i = 0; i < preprocIfs.count(); ++i) {
							INode::Ptr preprocIf = preprocIfs[i];
							Variant var = nullptr;
							if (!preprocIf->get(var, "branches"))
								continue;

							void* ptr = (void*)var;
							if (!ptr)
								continue;

							const PreprocessorBranch::Array* bptr = (const PreprocessorBranch::Array*)ptr;
							const PreprocessorBranch::Array &toAppend = *bptr;
							PreprocessorBranch::Array &branches = result->pagedPreprocessorBranches[index];
							branches.reserve(branches.size() + toAppend.size());
							branches.insert(
								branches.end(),
								std::make_move_iterator(toAppend.begin()),
								std::make_move_iterator(toAppend.end())
							);
						}
					}
				);
		}
	}
	static void doAnalyzeCodePages(Result* result, Program &program) {
		// Prepare.
		const INode::Ptr &root = program.root; // Get the compiled AST, which could be corrupt.
		if (!root)
			return;

		// Select pages.
		Select pages = Select(root)
			.children(Where(INode::Types::PAGE));

		if (pages.ok()) {
			// Parse the code page names.
			result->codePageNames.resize(pages.count());
			pages
				.foreach(
					[&] (const Select &, const INode::Ptr &page, int index) -> void {
						Select dest = Select(page)
							.firstChild(Where(INode::Types::DESTINATION));
						if (!dest.ok())
							return;

						const INode::Abstract abs = dest->abstract();
						result->codePageNames[index] = CodePageName(abs.front());
					}
				);
		}
	}
};

StaticAnalyzer::CodePageName::CodePageName() {
}

StaticAnalyzer::CodePageName::CodePageName(const std::string &n) : name(n) {
}

StaticAnalyzer* StaticAnalyzer::create(AnalyzeHandler analyzeHandler) {
	StaticAnalyzerImpl* result = new StaticAnalyzerImpl(analyzeHandler);

	return result;
}

void StaticAnalyzer::destroy(StaticAnalyzer* ptr) {
	StaticAnalyzerImpl* impl = static_cast<StaticAnalyzerImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
