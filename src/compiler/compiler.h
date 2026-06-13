/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
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
#ifndef COMPILER_STRICT_ON_OPTION_KEY
#	define COMPILER_STRICT_ON_OPTION_KEY "u"
#endif /* COMPILER_STRICT_ON_OPTION_KEY */
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
#ifndef COMPILER_MACROS_OPTION_KEY
#	define COMPILER_MACROS_OPTION_KEY "q"
#endif /* COMPILER_MACROS_OPTION_KEY */
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
#ifndef COMPILER_RUMBLE_OPTION_KEY
#	define COMPILER_RUMBLE_OPTION_KEY "w"
#endif /* COMPILER_RUMBLE_OPTION_KEY */
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
#ifndef COMPILER_I18N_BEGIN
#	define COMPILER_I18N_BEGIN "<i18n page=\"{0}\">"
#endif /* COMPILER_I18N_BEGIN */
#ifndef COMPILER_I18N_END
#	define COMPILER_I18N_END "</i18n>"
#endif /* COMPILER_I18N_END */
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
		NONE,
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
		STRING,          // `DEF ... = "..."`.
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
 * @brief Information of preprocessor branches.
 */
struct PreprocessorBranch {
	typedef std::vector<PreprocessorBranch> Array;

	bool isAlive = true;
	int page = -1;
	int beginLine = -1;
	int endLine = -1;
	int conditionLine = -1;

	PreprocessorBranch();
	PreprocessorBranch(bool alive, int pg, int begin, int end, int cond);

	bool operator == (const PreprocessorBranch &other) const;
	bool operator != (const PreprocessorBranch &other) const;
	bool operator < (const PreprocessorBranch &other) const;
	bool operator <= (const PreprocessorBranch &other) const;
	bool operator > (const PreprocessorBranch &other) const;
	bool operator >= (const PreprocessorBranch &other) const;

	int compare(const PreprocessorBranch &other) const;

	bool valid(void) const;
	void clear(void);
};

/**
 * @brief Assembly block range, used for syntax highlighting.
 */
struct AsmBlock {
	typedef std::vector<AsmBlock> Array;

	int page = -1;
	int row = 0; // Position of `BEGIN ASM`
	int column = 0;
	std::string name; // Optional.
	int beginLine = -1;
	int endLine = -1;
	int bank = -1; // Not involved in comparison.
	int address = -1; // Not involved in comparison.

	AsmBlock();
	AsmBlock(
		int pg, int ln, int col, const std::string &name_,
		int begin, int end,
		int b,
		int addr
	);

	bool operator == (const AsmBlock &other) const;
	bool operator != (const AsmBlock &other) const;
	bool operator < (const AsmBlock &other) const;
	bool operator <= (const AsmBlock &other) const;
	bool operator > (const AsmBlock &other) const;
	bool operator >= (const AsmBlock &other) const;

	int compare(const AsmBlock &other) const;

	bool valid(void) const;
	void clear(void);
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
** Structure of operations
*/

namespace GBBASIC {

struct Op {
	typedef UInt8 Opcode;

	enum class Types : Byte {
		STOP,
		INT8,
		INT16,
		REF,
		EQ,
		LT,
		LE,
		GT,
		GE,
		NE,
		AND,
		OR,
		NOT,
		ADD,
		SUB,
		MUL,
		DIV,
		MOD,
		BITWISE_AND,
		BITWISE_OR,
		BITWISE_XOR,
		BITWISE_LSHIFT,
		BITWISE_RSHIFT,
		BITWISE_NOT,
		NEG,
		SGN,
		ABS,
		SQR,
		SQRT,
		SIN,
		COS,
		ATAN2,
		POWI,
		MIN,
		MAX,

		COUNT
	};
	typedef std::array<Op, (size_t)Types::COUNT> Operators;

	static Operators OPERATORS;

	Opcode opcode = 0x00;
	int size = 0;                // Size of the parameter list in bytes.
	int oprands = 0;             // Count of oprands.
	int associativity = 0;       // -1 for left, 1 for right.
	int precedence = 0;          // The greater, the higher.
	bool isFunctionLike = false; // Whether the operator is function-like;

	Op(Opcode oc, int s, int r, int a, int p, bool f);

	static Types typeOf(const std::string &str);
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Token interface
*/

namespace GBBASIC {

/**
 * @brief Interface of AST token.
 */
class IToken {
public:
	typedef std::shared_ptr<IToken> Ptr;
	typedef std::vector<Ptr> Array;
	typedef std::vector<Array> Matrix;

	enum class Types : unsigned {
		NONE             =  0,
		PAGE             =  1 << 0,
		PREPROCESSOR     =  1 << 1,
		SPACE            =  1 << 2,
		END_OF_LINE      =  1 << 3,
		LINE_CONNECTOR   =  1 << 4,
		OPERATOR         =  1 << 5,
		SYMBOL           = (1 << 6) | (1 << 7),
			KEYWORD      =  1 << 6,
			IDENTIFIER   =  1 << 7,
		LABEL            =  1 << 8,
		NOTHING          =  1 << 9,
		BOOLEAN          =  1 << 10,
		NUMBER           = (1 << 11) | (1 << 12),
			INTEGER      =  1 << 11,
			REAL         =  1 << 12,
		STRING           =  1 << 13,
		COMMENT          =  1 << 14,
		INTERMEDIA       = (1 << 15) | (1 << 16) | (1 << 17) | (1 << 18),
			MATH         =  1 << 15,
			STATEMENT    =  1 << 16,
			ARRAY        =  1 << 17,
			MACRO        =  1 << 18,
		ANY              =  0xffffffff
	};

	enum class IntegerTypes {
		UNSPECIFIED,   // 8-bit signed integer or 8-bit unsigned integer.
		INT            // 16-bit signed integer.
	};

	struct Details {
		IntegerTypes integerType = IntegerTypes::UNSPECIFIED;
	};

public:
	virtual ~IToken();

	virtual Types type(void) const = 0;
	virtual IToken* type(Types y) = 0;
	virtual const Variant &data(void) const = 0;
	virtual IToken* data(const Variant &data_) = 0;
	virtual const std::string &text(void) const = 0;
	virtual IToken* text(const std::string &txt) = 0;
	virtual const std::string &caseSensitiveText(void) const = 0;

	virtual const TextLocation &begin(void) const = 0;
	virtual TextLocation &begin(void) = 0;
	virtual IToken* begin(const TextLocation &loc) = 0;
	virtual const TextLocation &end(void) const = 0;
	virtual TextLocation &end(void) = 0;
	virtual IToken* end(const TextLocation &loc) = 0;

	virtual const Details &details(void) const = 0;
	virtual Details &details(void) = 0;

	virtual IToken* add(const std::string &str) = 0;
	virtual char back(void) const = 0;
	virtual IToken* parse(bool caseInsensitive) = 0;

	virtual bool is(Types y) const = 0;
	virtual bool isNot(Types y) const = 0;

	virtual std::string dump(void) const = 0;

	static IToken* create(void);
	static IToken* create(Types y);
	static IToken* create(Types y, const Variant &data_, const TextLocation* begin_ = nullptr, const TextLocation* end_ = nullptr);
	static IToken* create(Types y, const std::string &txt, bool caseInsensitive = true, const TextLocation* begin_ = nullptr, const TextLocation* end_ = nullptr);
	static void destroy(IToken* ptr);
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Node interface
*/

namespace GBBASIC {

/**
 * @brief Interface of AST nodes.
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
		PREPROCESSOR_IF,
		PREPROCESSOR_MESSAGE,
		PREPROCESSOR_WARN,
		PREPROCESSOR_ERROR,
		EXPRESSION,
		MATH,
		DEG,
		ASC,
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
		DO_NOTHING_WITH,
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
		ASM,
		BEGIN_ASM,
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
		DEF_STRING,
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
		DIALOG_MANIPULATION,
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

	virtual /* LAZY */ IToken::Array tokens(void) const = 0;

	virtual bool get(Variant &ret, const std::string &msg, int argc, const Variant* argv) const = 0;
	bool get(Variant &ret, const std::string &msg) const {
		return get(ret, msg, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> bool get(Variant &ret, const std::string &msg, const Args &...args) const {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return get(ret, msg, (int)n, argv);
	}

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
	 * @brief The read content of the configuration.
	 */
	std::string config;
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
	 * @brief "Super" features.
	 */
	struct SuperFeatures {
		typedef std::vector<Colour> Palettes;

		/**
		 * @brief Whether these features have been enabled.
		 */
		bool enabled = false;
		/**
		 * @brief Border frame.
		 */
		Image::Ptr border = nullptr;
		/**
		 * @brief Extra pelettes.
		 */
		Palettes palettes;
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
		/**
		 * @brief Whether include a rumble motor.
		 */
		bool cartridgeHasRumble = false;

		/**< Parser and compiler strategies. */

		/**
		 * @brief Indicates whether the parser is case insensitive.
		 */
		bool caseInsensitive = true;
		/**
		 * @brief Indicates whether the compiler runs in strict mode.
		 */
		bool strictOn = true;
		/**
		 * @brief Whether to complete line number automatically.
		 */
		bool completeLineNumber = true;
		/**
		 * @brief Whether declaration is required before using a variable.
		 */
		bool declarationRequired = true;
		/**
		 * @brief Whether the kernel has implemented `touch` APIs for non-extension emulation.
		 */
		bool kernelImplementedTouchApi = false;
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
	 * @brief The path of the configuration file of the VM ROM.
	 */
	std::string config;
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
	 * @brief The pre-defined macros.
	 */
	std::string macros;
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
	 * @brief "Super" features.
	 */
	SuperFeatures superFeatures;
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
void identifiers(const char* kernelConfigPath, IdentifierHandler handler);

}

/* ===========================================================================} */

#endif /* __COMPILER_H__ */
