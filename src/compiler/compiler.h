/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "../gbbasic.h"
#include "pipeline.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef COMPILER_ALIASES_OPTION_KEY
#	define COMPILER_ALIASES_OPTION_KEY "l"
#endif /* COMPILER_ALIASES_OPTION_KEY */
#ifndef COMPILER_AST_OPTION_KEY
#	define COMPILER_AST_OPTION_KEY "a"
#endif /* COMPILER_AST_OPTION_KEY */
#ifndef COMPILER_BOOTSTRAP_OPTION_KEY
#	define COMPILER_BOOTSTRAP_OPTION_KEY "b"
#endif /* COMPILER_BOOTSTRAP_OPTION_KEY */
#ifndef COMPILER_CART_TYPE_OPTION_KEY
#	define COMPILER_CART_TYPE_OPTION_KEY "g"
#endif /* COMPILER_CART_TYPE_OPTION_KEY */
#ifndef COMPILER_CASE_INSENSITIVE_OPTION_KEY
#	define COMPILER_CASE_INSENSITIVE_OPTION_KEY "i"
#endif /* COMPILER_CASE_INSENSITIVE_OPTION_KEY */
#ifndef COMPILER_DECLARATION_REQUIRED_OPTION_KEY
#	define COMPILER_DECLARATION_REQUIRED_OPTION_KEY "d"
#endif /* COMPILER_DECLARATION_REQUIRED_OPTION_KEY */
#ifndef COMPILER_EXPLICIT_LINE_NUMBER_OPTION_KEY
#	define COMPILER_EXPLICIT_LINE_NUMBER_OPTION_KEY "n"
#endif /* COMPILER_EXPLICIT_LINE_NUMBER_OPTION_KEY */
#ifndef COMPILER_FONT_OPTION_KEY
#	define COMPILER_FONT_OPTION_KEY "f"
#endif /* COMPILER_FONT_OPTION_KEY */
#ifndef COMPILER_HEAP_SIZE_OPTION_KEY
#	define COMPILER_HEAP_SIZE_OPTION_KEY "h"
#endif /* COMPILER_HEAP_SIZE_OPTION_KEY */
#ifndef COMPILER_INPUT_OPTION_KEY
#	define COMPILER_INPUT_OPTION_KEY ""
#endif /* COMPILER_INPUT_OPTION_KEY */
#ifndef COMPILER_OPTIMIZE_CODE_OPTION_KEY
#	define COMPILER_OPTIMIZE_CODE_OPTION_KEY "z"
#endif /* COMPILER_OPTIMIZE_CODE_OPTION_KEY */
#ifndef COMPILER_OPTIMIZE_ASSETS_OPTION_KEY
#	define COMPILER_OPTIMIZE_ASSETS_OPTION_KEY "p"
#endif /* COMPILER_OPTIMIZE_ASSETS_OPTION_KEY */
#ifndef COMPILER_OUTPUT_OPTION_KEY
#	define COMPILER_OUTPUT_OPTION_KEY "o"
#endif /* COMPILER_OUTPUT_OPTION_KEY */
#ifndef COMPILER_RAM_TYPE_OPTION_KEY
#	define COMPILER_RAM_TYPE_OPTION_KEY "m"
#endif /* COMPILER_RAM_TYPE_OPTION_KEY */
#ifndef COMPILER_ROM_OPTION_KEY
#	define COMPILER_ROM_OPTION_KEY "r"
#endif /* COMPILER_ROM_OPTION_KEY */
#ifndef COMPILER_RTC_OPTION_KEY
#	define COMPILER_RTC_OPTION_KEY "c"
#endif /* COMPILER_RTC_OPTION_KEY */
#ifndef COMPILER_STACK_SIZE_OPTION_KEY
#	define COMPILER_STACK_SIZE_OPTION_KEY "k"
#endif /* COMPILER_STACK_SIZE_OPTION_KEY */
#ifndef COMPILER_SYM_OPTION_KEY
#	define COMPILER_SYM_OPTION_KEY "s"
#endif /* COMPILER_SYM_OPTION_KEY */
#ifndef COMPILER_TITLE_OPTION_KEY
#	define COMPILER_TITLE_OPTION_KEY "t"
#endif /* COMPILER_TITLE_OPTION_KEY */

#ifndef COMPILER_PROGRAM_BEGIN
#	define COMPILER_PROGRAM_BEGIN "<program>"
#endif /* COMPILER_PROGRAM_BEGIN */
#ifndef COMPILER_PROGRAM_END
#	define COMPILER_PROGRAM_END "</program>"
#endif /* COMPILER_PROGRAM_END */
#ifndef COMPILER_INFO_BEGIN
#	define COMPILER_INFO_BEGIN "<info>"
#endif /* COMPILER_INFO_BEGIN */
#ifndef COMPILER_INFO_END
#	define COMPILER_INFO_END "</info>"
#endif /* COMPILER_INFO_END */
#ifndef COMPILER_ENGINE_BEGIN
#	define COMPILER_ENGINE_BEGIN "<engine>"
#endif /* COMPILER_ENGINE_BEGIN */
#ifndef COMPILER_ENGINE_END
#	define COMPILER_ENGINE_END "</engine>"
#endif /* COMPILER_ENGINE_END */
#ifndef COMPILER_PALETTE_BEGIN
#	define COMPILER_PALETTE_BEGIN "<palette>"
#endif /* COMPILER_PALETTE_BEGIN */
#ifndef COMPILER_PALETTE_END
#	define COMPILER_PALETTE_END "</palette>"
#endif /* COMPILER_PALETTE_END */
#ifndef COMPILER_FONT_BEGIN
#	define COMPILER_FONT_BEGIN "<font>"
#endif /* COMPILER_FONT_BEGIN */
#ifndef COMPILER_FONT_END
#	define COMPILER_FONT_END "</font>"
#endif /* COMPILER_FONT_END */
#ifndef COMPILER_CODE_BEGIN
#	define COMPILER_CODE_BEGIN "<code page=\"{0}\">"
#endif /* COMPILER_CODE_BEGIN */
#ifndef COMPILER_CODE_END
#	define COMPILER_CODE_END "</code>"
#endif /* COMPILER_CODE_END */
#ifndef COMPILER_TILES_BEGIN
#	define COMPILER_TILES_BEGIN "<tiles page=\"{0}\">"
#endif /* COMPILER_TILES_BEGIN */
#ifndef COMPILER_TILES_END
#	define COMPILER_TILES_END "</tiles>"
#endif /* COMPILER_TILES_END */
#ifndef COMPILER_MAP_BEGIN
#	define COMPILER_MAP_BEGIN "<map page=\"{0}\">"
#endif /* COMPILER_MAP_BEGIN */
#ifndef COMPILER_MAP_END
#	define COMPILER_MAP_END "</map>"
#endif /* COMPILER_MAP_END */
#ifndef COMPILER_MUSIC_BEGIN
#	define COMPILER_MUSIC_BEGIN "<music page=\"{0}\">"
#endif /* COMPILER_MUSIC_BEGIN */
#ifndef COMPILER_MUSIC_END
#	define COMPILER_MUSIC_END "</music>"
#endif /* COMPILER_MUSIC_END */
#ifndef COMPILER_SFX_BEGIN
#	define COMPILER_SFX_BEGIN "<sfx page=\"{0}\">"
#endif /* COMPILER_SFX_BEGIN */
#ifndef COMPILER_SFX_END
#	define COMPILER_SFX_END "</sfx>"
#endif /* COMPILER_SFX_END */
#ifndef COMPILER_ACTOR_BEGIN
#	define COMPILER_ACTOR_BEGIN "<actor page=\"{0}\">"
#endif /* COMPILER_ACTOR_BEGIN */
#ifndef COMPILER_ACTOR_END
#	define COMPILER_ACTOR_END "</actor>"
#endif /* COMPILER_ACTOR_END */
#ifndef COMPILER_SCENE_BEGIN
#	define COMPILER_SCENE_BEGIN "<scene page=\"{0}\">"
#endif /* COMPILER_SCENE_BEGIN */
#ifndef COMPILER_SCENE_END
#	define COMPILER_SCENE_END "</scene>"
#endif /* COMPILER_SCENE_END */

#ifndef COMPILER_INVALID_INSTRUCTION
#	define COMPILER_INVALID_INSTRUCTION 0xff
#endif /* COMPILER_INVALID_INSTRUCTION */

#ifndef COMPILER_PLACEHOLDER
#	define COMPILER_PLACEHOLDER 0
#endif /* COMPILER_PLACEHOLDER */

#ifndef COMPILER_STACK_ARGUMENT_MAX_COUNT
#	define COMPILER_STACK_ARGUMENT_MAX_COUNT 64
#endif /* COMPILER_STACK_ARGUMENT_MAX_COUNT */

namespace GBBASIC {

/**
 * @brief Enum operators.
 */
template<typename Enum, typename Base = unsigned> inline Enum operator | (Enum left, Enum right) {
	return (Enum)((Base)left | (Base)right);
}
template<typename Enum, typename Base = unsigned> inline Enum operator & (Enum left, Enum right) {
	return (Enum)((Base)left & (Base)right);
}
template<typename Enum> inline Enum &operator |= (Enum &left, Enum right) {
	return left = left | right;
}
template<typename Enum> inline Enum &operator &= (Enum &left, Enum right) {
	return left = left & right;
}
#ifndef IMPLEMENT_ENUM_OPERATORS
#	define IMPLEMENT_ENUM_OPERATORS(E) \
		inline E operator | (E left, E right) { \
			return operator | <E>(left, right); \
		} \
		inline E operator & (E left, E right) { \
			return operator & <E>(left, right); \
		} \
		inline E &operator |= (E &left, E right) { \
			return operator |= <E>(left, right); \
		} \
		inline E &operator &= (E &left, E right) { \
			return operator &= <E>(left, right); \
		}
#endif /* IMPLEMENT_ENUM_OPERATORS */

}

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

namespace GBBASIC {

/**
 * @brief Location for structures from text-based assets.
 */
struct TextLocation {
	typedef std::pair<TextLocation, TextLocation> Range;

	int page = 0;
	int row = 0;
	int column = 0;

	TextLocation();
	TextLocation(int p);
	TextLocation(int p, int r, int c);

	bool operator == (const TextLocation &other) const;
	bool operator != (const TextLocation &other) const;
	bool operator < (const TextLocation &other) const;
	bool operator <= (const TextLocation &other) const;
	bool operator > (const TextLocation &other) const;
	bool operator >= (const TextLocation &other) const;

	int compare(const TextLocation &other) const;

	bool invalid(void) const;

	static TextLocation INVALID(void);
};

/**
 * @brief Location for RAM allocations.
 */
struct RamLocation {
	typedef std::map<std::string, RamLocation> Dictionary;

	enum class Types {
		HEAP
	};

	enum class Usages {
		NONE,
		VARIABLE, // With `LET`.
		ARRAY,    // With `DIM`.
		LOOP,     // With `FOR`.
		READ,     // With `READ`.
		TOUCH,    // With `TOUCH`.
		VIEWPORT, // With `VIEWPORT`.
		COUNT
	};

	Types type = Types::HEAP;
	int address = 0;
	int size = 0;
	Usages usage = Usages::NONE;
	TextLocation textLocation;

	RamLocation();
	RamLocation(Types y, int a, int s, Usages u, const TextLocation &txtLoc);
};

/**
 * @brief Information of macros.
 */
struct Macro { // FEAT: MACRO.
	typedef std::list<Macro> List;

	enum class Types {
		NONE,
		MACRO_AILAS,     // `DEF ... = M`.
		FUNCTION,        // `DEF FN(...) = ...`.
		CONSTANT,        // `DEF ... = N`.
		VARIABLE_ALIAS,  // `DEF ... = var`.
		STACK_REFERENCE, // `DEF ... = STACKN`.
		COUNT
	};

	std::string name;
	Types type = Types::NONE;
	Variant data = nullptr;
	TextLocation::Range scopeLocationRange;

	Macro();
	Macro(const std::string &name_, Types y, const Variant &d);
	Macro(const std::string &name_, Types y, const Variant &d, const TextLocation &begin);

	bool operator == (const Macro &other) const;
	bool operator != (const Macro &other) const;
	bool operator < (const Macro &other) const;
	bool operator <= (const Macro &other) const;
	bool operator > (const Macro &other) const;
	bool operator >= (const Macro &other) const;

	int compare(const Macro &other) const;
};

/**
 * @brief Feature usages.
 */
struct FeatureUsages {
	typedef std::map<std::string, int> Dictionary;

	Dictionary coloredFeatureUsages;
	Dictionary superFeatureUsages;
	Dictionary extensionFeatureUsages;

	FeatureUsages();
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Node interface
*/

namespace GBBASIC {

/**
 * @brief Interface for all AST nodes.
 */
class INode {
public:
	typedef std::shared_ptr<const INode> Ptr;
	typedef std::vector<Ptr> Array;

	typedef std::vector<std::string> Abstract;

	enum class Types : unsigned {
		ANY, // For select query.
		PROGRAM,
		PAGE,
		EXPRESSION,
		MATH,
		ASC,
		DEG,
		LEN,
		RANDOMIZE,
		RND,
		BANKOF,
		ADDRESSOF,
		ARRAY_READ,
		ARRAY_WRITE,
		BLANK,
		REM,
		DO_NOTHING,
		CONST,
		LET,
		DIM,
		IF,
		THEN,
		ELSE,
		ELSE_IF,
		IIF,
		SELECT,
		CASE,
		ON,
		OFF,
		FOR,
		NEXT,
		WHILE,
		REPEAT,
		EXIT,
		DO,
		DESTINATION,
		GOTO,
		GOSUB,
		RETURN,
		END,
		CALL,
		START,
		JOIN,
		KILL,
		WAIT,
		LOCK,
		UNLOCK,
		ARG,
		BEGIN_DO,
		BEGIN_DEF,
		DEF_MACRO_ALIAS,
		DEF_FN,
		FN,
		DEF_CONSTANT,
		DEF_IDENTIFIER_ALIAS,
		DEF_STACK_N,
		LOCATE,
		PRINT,
		CLS,
		PEEK,
		POKE,
		RESERVE,
		PUSH,
		POP,
		TOP,
		STACK,
		STACK_N,
		PACK,
		UNPACK,
		SWAP,
		INC,
		DEC,
		DATA,
		READ,
		RESTORE,
		ROUTINE,
		FUNCTION,
		PALETTE,
		RGB,
		HSV,
		RECT,
		TEXT,
		IMAGE_MANIPULATION,
		TILE_MANIPULATION,
		MAP_MANIPULATION,
		WINDOW_MANIPULATION,
		SPRITE_MANIPULATION,
		PLAY,
		SOUND,
		BEEP,
		TOUCH,
		FILE,
		SERIAL,
		FILLER,
		MEMSET,
		MEMCPY,
		MEMADD,
		NEW,
		WIDTH,
		HEIGHT,
		IS,
		VIEWPORT,
		SCENE_MANIPULATION,
		ACTOR_MANIPULATION,
		EMOTE_MANIPULATION,
		PROJECTILE_MANIPULATION,
		TRIGGER_MANIPULATION,
		WIDGET_MANIPULATION,
		LABEL_MANIPULATION,
		PROGRESSBAR_MANIPULATION,
		MENU_MANIPULATION,
		SCROLL,
		FX,
		HITS,
		UPDATE,
		SCREEN,
		OPTION,
		QUERY,
		AUTO_TOGGLE,
		TOGGLE,
		STREAM,
		SHELL,
		SLEEP,
		RAISE,
		ERROR,
		RESET
	};

public:
	virtual ~INode();

	virtual Types type(void) const = 0;

	virtual /* LAZY */ TextLocation::Range location(void) const = 0;

	virtual /* LAZY */ Array children(void) const = 0;

	virtual /* LAZY */ Abstract abstract(void) const = 0;

	virtual /* LAZY */ std::string dump(int depth) const = 0;
};

/**
 * @brief Where clause of `INode` query.
 */
struct Where {
	friend struct Select;

public:
	typedef std::list<INode::Types> Types;

private:
	INode::Types _type = INode::Types::ANY;
	Types _types;
	bool _failIfNotAllMatch = true;
	bool _recursive = false;
	bool _ignoreMeaningless = true;

public:
	Where();
	Where(INode::Types y, bool failIfNotAllMatch = true, bool recursive = false, bool ignoreMeaningless = true);
	Where(const std::initializer_list<INode::Types> &y, bool failIfNotAllMatch = true, bool recursive = false, bool ignoreMeaningless = true);

	Where &isType(INode::Types y);
	Where &isTypeIn(const std::initializer_list<INode::Types> &y);
	Where &doFailIfNotAllMatch(bool val);
	Where &doRecursive(bool val);
	Where &doIgnoreMeaningless(bool val);
};

/**
 * @brief Select clause of `INode` query.
 */
struct Select {
public:
	typedef std::function<bool(const Select &, const INode::Ptr &, int)> FilterHandler;
	typedef std::function<void(const Select &, const INode::Ptr &, int)> ConstEnumerator;
	typedef std::function<void(const Select &, INode::Ptr &, int)> Enumerator;

private:
	bool _ok = true;
	INode::Array _collection;

public:
	Select();
	Select(INode::Ptr node);

	INode::Ptr operator [] (int idx) const;
	INode::Ptr operator -> (void) const;

	/* LAZY */ Select only(void) const;
	/* LAZY */ Select children(void) const;
	/* LAZY */ Select children(const Where &where) const;
	/* LAZY */ Select firstChild(void) const;
	/* LAZY */ Select firstChild(const Where &where) const;

	bool ok(void) const;
	int count(void) const;
	int count(FilterHandler filter) const;
	Select filter(FilterHandler filter) const;
	void foreach(ConstEnumerator enumerator) const;
	void foreach(Enumerator enumerator);

private:
	Select &fail(void);
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Compiler
*/

namespace GBBASIC {

/**
 * @brief Program input and output.
 */
struct Program {
	/**
	 * @brief Structure of compiled result.
	 */
	struct Compiled {
		/**
		 * @brief The macro functions, constants and stack references.
		 */
		Macro::List macros;
		/**
		 * @brief The RAM allocations on the heap.
		 */
		RamLocation::Dictionary allocations;
		/**
		 * @brief Feature usages. Filled by compiler.
		 */
		FeatureUsages featureUsages;
		/**
		 * @brief The compiled ROM.
		 */
		Bytes::Ptr bytes = nullptr;
		/**
		 * @brief The effective data size.
		 */
		Pipeline::Size effectiveSize;
	};

	/**< Input. */

	/**
	 * @brief The generated ROM.
	 */
	Bytes::Ptr rom = nullptr;
	/**
	 * @brief The read content of the symbols.
	 */
	std::string symbols;
	/**
	 * @brief The read content of the aliases.
	 */
	std::string aliases;
	/**
	 * @brief The parsed content of the assets.
	 */
	AssetsBundle::Ptr assets = nullptr;

	/**< Output. */

	/**
	 * @brief Whetier the compiling program is a plain one.
	 */
	bool isPlain = false;
	/**
	 * @brief The width of the line number.
	 */
	int lineNumberWidth = 0;
	/**
	 * @brief The generated AST text.
	 */
	std::string ast;
	/**
	 * @brief The generated AST nodes.
	 */
	INode::Ptr root;
	/**
	 * @brief The compiled result.
	 */
	Compiled compiled;
};

/**
 * @brief Compiling options.
 */
struct Options {
	/**
	 * @brief Handler to output messages.
	 */
	typedef std::function<void(const std::string &/* msg */)> PrintHandler;
	/**
	 * @brief Handler to output warnings and errors.
	 */
	typedef std::function<void(const std::string &/* msg */, bool /* isWarning */, int /* page */, int /* row */, int /* column */)> ErrorHandler;

	/**
	 * @brief How far to compile.
	 */
	enum class Passes : unsigned {
		PARSE,    // Parsing only.
		GENERATE, // Parsing and perform generating.
		FULL      // Full passes including parsing, generating and posting.
	};

	/**
	 * @brief Compiling strategies.
	 */
	struct Strategies {
		/**
		 * @brief Device compatibilities.
		 */
		enum class Compatibilities : unsigned {
			NONE      = 0,
			CLASSIC   = 1 << 0,
			COLORED   = 1 << 1,
			EXTENSION = 1 << 2
		};

		/**< Cartridge strategies. */

		/**
		 * @brief The cartridge compatibility.
		 */
		Compatibilities compatibility = Compatibilities::CLASSIC | Compatibilities::COLORED;
		/**
		 * @brief The SRAM type.
		 *   SRAM size code:
		 *     0x00 -   0KB
		 *     0x01 -   2KB
		 *     0x02 -   8KB
		 *     0x03 -  32KB
		 *     0x04 - 128KB
		 */
		int sramType = 0x03;
		/**
		 * @brief Whether include an RTC chip.
		 */
		bool cartridgeHasRtc = true;

		/**< Parser and compiler strategies. */

		/**
		 * @brief Indicates whether the parser is case insensitive.
		 */
		bool caseInsensitive = true;
		/**
		 * @brief Whether to complete line number automatically.
		 */
		bool completeLineNumber = true;
		/**
		 * @brief Whether declaration is required before using a variable.
		 */
		bool declarationRequired = true;
		/**
		 * @brief The index base.
		 */
		int indexBase = 0;
		/**
		 * @brief The bootstrap bank.
		 */
		int bootstrapBank = 0;
		/**
		 * @brief The heap size.
		 */
		int heapSize = 1024 * 8;
		/**
		 * @brief The stack size.
		 */
		int stackSize = 1024 * 8;
		/**
		 * @brief Whether to break compiling following code when an error occurs.
		 */
		bool failOnError = true;
		/**
		 * @brief Whether to optimize code.
		 */
		bool optimizeCode = true;
		/**
		 * @brief Whether to optimize assets.
		 */
		bool optimizeAssets = true;
	};
	/**
	 * @brief Pipeline config.
	 */
	struct Piping {
		/**
		 * @brief Whether to use work queue.
		 */
		bool useWorkQueue = false;
		/**
		 * @brief Whether to output less to the console.
		 */
		bool lessConsoleOutput = false;
	};

	/**< Configs. */

	/**
	 * @brief The path of the input file, will use `program.assets` (which must be
	 *   predefined) if this field is empty.
	 */
	std::string input;
	/**
	 * @brief The path of the output file, will output to `program.compiled` only
	 *   if this field is empty.
	 */
	std::string output;
	/**
	 * @brief The path of the VM ROM file.
	 */
	std::string rom;
	/**
	 * @brief The path of the symbol file of the VM ROM.
	 */
	std::string sym;
	/**
	* @brief The path of the symbol aliases file of the VM ROM.
	*/
	std::string aliases;
	/**
	 * @brief The path of the font configuration file.
	 */
	std::string font;
	/**
	 * @brief The path or target of the AST output, can be "none", "stdout" or
	 *   file path.
	 */
	std::string ast = "none";
	/**
	 * @brief The desired compiling passes to be executed.
	 */
	Passes passes = Passes::FULL;
	/**
	 * @brief The icon of the program.
	 */
	Bytes::Ptr icon = nullptr;
	/**
	 * @brief Whether the super features have been enabled for the program.
	 */
	bool superFeaturesEnabled = false;
	/**
	 * @brief The border frame of the program.
	 */
	Image::Ptr border = nullptr;
	/**
	 * @brief The background palettes of the program.
	 */
	Bytes::Ptr backgroundPalettes = nullptr;
	/**
	 * @brief The sprite palettes of the program.
	 */
	Bytes::Ptr spritePalettes = nullptr;
	/**
	 * @brief The title of the program.
	 */
	std::string title;
	/**
	 * @brief The strategies for the cartridge generation.
	 */
	Strategies strategies;
	/**
	 * @brief Pipeline options.
	 */
	Piping piping;

	/**< Handlers. */

	/**
	 * @brief The compiler's message output handler.
	 */
	PrintHandler onPrint;
	/**
	 * @brief The compiler's error output handler.
	 */
	ErrorHandler onError;
	/**
	 * @brief The pipeline's handler to check whether a behaviour is for player.
	 */
	ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour;
	/**
	 * @brief The pipeline's message output handler.
	 */
	Pipeline::PrintHandler onPipelinePrint;
	/**
	 * @brief The pipeline's error output handler.
	 */
	Pipeline::ErrorHandler onPipelineError;
};

IMPLEMENT_ENUM_OPERATORS(Options::Strategies::Compatibilities)

/**
 * @brief Handler to traverse identifiers.
 */
typedef std::function<void(const std::string &/* id */, const std::string &/* type */)> IdentifierHandler;

/**
 * @brief Loads configs, assets, code and kernel.
 */
bool load(Program &program, Options &options);
/**
 * @brief Parses, compiles and pipeline assets.
 */
bool compile(Program &program, const Options &options);
/**
 * @brief Links compiled result.
 */
bool link(Program &program, const Options &options);

/**
 * @brief Traverses all identifiers.
 */
void identifiers(IdentifierHandler handler);

}

/* ===========================================================================} */

#endif /* __COMPILER_H__ */
