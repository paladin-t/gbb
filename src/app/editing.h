/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITING_H__
#define __EDITING_H__

#include "../gbbasic.h"
#include "../utils/assets.h"
#include "../utils/plus.h"
#include "../utils/text.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITING_ITEM_COUNT_PER_LINE
#	define EDITING_ITEM_COUNT_PER_LINE 5
#endif /* EDITING_ITEM_COUNT_PER_LINE */

#ifndef EDITING_MUSIC_NO_INPUT_VALUE
#	define EDITING_MUSIC_NO_INPUT_VALUE -1
#endif /* EDITING_MUSIC_NO_INPUT_VALUE */
#ifndef EDITING_MUSIC_NEWLINE_INPUT_VALUE
#	define EDITING_MUSIC_NEWLINE_INPUT_VALUE -2
#endif /* EDITING_MUSIC_NEWLINE_INPUT_VALUE */

#ifndef EDITING_SFX_SHAPE_IMAGE_WIDTH
#	define EDITING_SFX_SHAPE_IMAGE_WIDTH (72 - 2)
#endif /* EDITING_SFX_SHAPE_IMAGE_WIDTH */
#ifndef EDITING_SFX_SHAPE_IMAGE_HEIGHT
#	define EDITING_SFX_SHAPE_IMAGE_HEIGHT 24
#endif /* EDITING_SFX_SHAPE_IMAGE_HEIGHT */

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

class Project;
class Workspace;

/* ===========================================================================} */

/*
** {===========================================================================
** Editing
*/

/**
 * @brief Utilities for editing and preview area.
 */
namespace Editing {

/**< Structures. */

enum class Flips : unsigned {
	NONE,
	HORIZONTAL,
	VERTICAL,
	HORIZONTAL_VERTICAL
};

enum class Operations : unsigned {
	COPY,
	CUT,
	PASTE,
	RESET,
	PLAY,
	STOP,
	IMPORT,
	EXPORT
};

union Dots {
	Colour* colored;
	int* indexed;

	Dots(Colour* col);
	Dots(int* idx);
};

union Dot {
	typedef std::vector<Dot> Array;

	Colour colored;
	int indexed;

	Dot();
	Dot(const Colour &col);
	Dot(int idx);
	Dot(const Dot &other);

	Dot &operator = (const Dot &other);
	bool operator == (const Dot &other) const;
	bool operator != (const Dot &other) const;
};

struct Point {
	typedef std::set<Point> Set;

	Math::Vec2i position = Math::Vec2i(-1, -1);
	Dot dot;

	Point();
	Point(const Math::Vec2i &pos);
	Point(const Math::Vec2i &pos, const Dot &d);
	Point(const Math::Vec2i &pos, const Colour &col);
	Point(const Math::Vec2i &pos, int idx);

	bool operator < (const Point &other) const;
};

struct Painting : public NonCopyable {
private:
	bool _lastValue = false;
	bool _value = false;
	Math::Vec2f _lastPosition = Math::Vec2f(-1, -1);
	Math::Vec2f _position = Math::Vec2f(-1, -1);

public:
	Painting();

	operator bool (void) const;

	Painting &set(bool val);
	bool moved(void) const;
	bool down(void) const;
	bool up(void) const;

	void clear(void);
};

struct Brush {
	Math::Vec2i position = Math::Vec2i(-1, -1); // Position in pixels.
	Math::Vec2i size = Math::Vec2i(1, 1);       // Size in pixels.

	Brush();

	bool operator == (const Brush &other) const;
	bool operator != (const Brush &other) const;
	bool operator == (const Math::Recti &other) const;
	bool operator != (const Math::Recti &other) const;

	bool empty(void) const;

	Math::Vec2i unit(void) const;

	Math::Recti toRect(void) const;
	Brush &fromRect(const Math::Recti &other);
};

struct Tiler {
	Math::Vec2i position = Math::Vec2i(-1, -1); // Position in tiles.
	Math::Vec2i size = Math::Vec2i(1, 1);       // Size in tiles.

	Tiler();

	bool operator == (const Tiler &other) const;
	bool operator != (const Tiler &other) const;

	bool empty(void) const;
};

struct Shortcut {
	int key = 0;
	bool ctrl = false;
	bool shift = false;
	bool alt = false;
	bool numLock = false;
	bool capsLock = false;
	bool super = false;

	Shortcut(
		int key,
		bool ctrl = false, bool shift = false, bool alt = false,
		bool numLock = false, bool capsLock = false,
		bool super = false
	);

	bool pressed(bool repeat = true) const;
	bool released(void) const;
};

struct NumberStroke {
	enum class Bases : int {
		DEC = 10,
		HEX = 16
	};

	Bases base = Bases::DEC;
	int value = 0;
	int minValue = 0;
	int maxValue = 255;

	NumberStroke();
	NumberStroke(Bases base_, int val, int min, int max);

	bool filled(int* val /* nullable */, bool repeat = true);
	void clear(void);
};

struct SymbolLocation {
	int bank = 0;
	int address = 0;
	int size = 0;

	SymbolLocation();
	SymbolLocation(int b, int a);
	SymbolLocation(int b, int a, int s);
};

struct SymbolTable {
public:
	typedef std::map<std::string, SymbolLocation> Dictionary;

	typedef std::function<void(const char*)> ErrorHandler;

private:
	Dictionary _dictionary;

public:
	SymbolTable();

	/**
	 * @param[in, out] dict
	 * @param[out] symTxt
	 * @param[out] aliasesTxt
	 */
	bool load(Dictionary &dict, const std::string &symPath, std::string &symTxt, const std::string &aliasesPath, std::string &aliasesTxt, ErrorHandler onError);

	bool parseSymbols(const std::string &symbols);
	bool parseAliases(const std::string &aliases);
	const SymbolLocation* find(const std::string &key) const;
};

struct ActorIndex {
	typedef std::vector<ActorIndex> Array;

	Byte cel = Scene::INVALID_ACTOR();
	Math::Vec2i offset;

	ActorIndex();
	ActorIndex(Byte cel, const Math::Vec2i &off);

	bool invalid(void) const;
	void clear(void);
};

struct TriggerIndex {
	typedef std::vector<TriggerIndex> Array;

	Byte cel = Scene::INVALID_TRIGGER();

	TriggerIndex();
	TriggerIndex(Byte cel);

	bool invalid(void) const;
	void clear(void);
};

typedef std::function<int(const Math::Vec2i &)> MapCelGetter;

typedef std::function<int(const Math::Vec2i &, TriggerIndex::Array &)> SceneTriggerQuerier;

typedef std::function<void(int, Operations)> IndexedOperationHandler;

typedef std::function<void(void)> PostHandler;

/**< Utilities for editing area. */

/**
 * @param[out] col
 * @param[in, out] cursor
 */
bool palette(
	Renderer* rnd, Workspace* ws,
	const Indexed* ptr,
	float width = -1.0f,
	Colour* col = nullptr,
	int* cursor = nullptr, bool showCursor = true,
	Texture* transparent = nullptr
);

/**
 * @param[in, out] cursor
 * @param[in, out] cursorStamp
 */
bool tiles(
	Renderer* rnd, Workspace* ws,
	const Image* ptr, Texture* tex,
	float width = -1.0f,
	Brush* cursor = nullptr, bool showCursor = true, Brush &&cursorStamp = Brush(),
	const Brush* selection = nullptr,
	Texture* overlay = nullptr,
	const Math::Vec2i* gridSize = nullptr, bool showGrids = false,
	bool showTransparentBackbround = true,
	PostHandler post = nullptr
);

/**
 * @param[in, out] cursor
 */
bool tiles(
	Renderer* rnd, Workspace* ws,
	const Image* ptr, Texture* tex,
	float width = -1.0f,
	Brush* cursor = nullptr, bool showCursor = true,
	const Brush* selection = nullptr,
	Texture* overlay = nullptr,
	const Math::Vec2i* gridSize = nullptr, bool showGrids = false,
	bool showTransparentBackbround = true,
	PostHandler post = nullptr
);

/**
 * @param[in, out] cursor
 */
bool map(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const char* id,
	const Map* ptr, const Math::Vec2i &tileSize,
	bool paletted = false,
	float width = -1.0f, 
	Tiler* cursor = nullptr, bool showCursor = true,
	const Tiler* selection = nullptr,
	const Math::Vec2i* showGrids = nullptr,
	bool showTransparentBackbround = true,
	int ignoreCel = -1,
	MapCelGetter getCel = nullptr,
	MapCelGetter getPlt = nullptr,
	MapCelGetter getFlip = nullptr
);

/**
 * @param[in, out] cursor
 */
bool map(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const Map* ptr, const Math::Vec2i &tileSize,
	bool paletted = false,
	float width = -1.0f, 
	Tiler* cursor = nullptr, bool showCursor = true,
	const Tiler* selection = nullptr,
	const Math::Vec2i* showGrids = nullptr,
	bool showTransparentBackbround = true,
	int ignoreCel = -1,
	MapCelGetter getCel = nullptr,
	MapCelGetter getPlt = nullptr,
	MapCelGetter getFlip = nullptr
);

/**
 * @param[in, out] cursor
 * @param[out] hovering
 */
bool frames(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const Text::Array &names,
	float width = -1.0f,
	int* cursor = nullptr, int* hovering = nullptr,
	bool* appended = nullptr,
	bool allowShortcuts = true,
	bool showAnchor = true
);

/**
 * @param[in, out] cursor
 */
bool frame(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const Image* img, Texture* tex,
	float width = -1.0f,
	int magnification = 1,
	Brush* cursor = nullptr, bool showCursor = true,
	const Brush* selection = nullptr,
	Texture* overlay = nullptr,
	const Math::Vec2i* gridSize = nullptr, bool showGrids = false,
	bool showTransparentBackbround = true,
	const Math::Vec2i* anchor = nullptr, bool showAnchor = true,
	const Math::Recti* boundingBox = nullptr, bool showBoundingBox = false
);

/**
 * @param[in, out] cursor
 * @param[in, out] cursorStamp
 */
bool frame(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const Image* img, Texture* tex,
	float width = -1.0f,
	int magnification = 1,
	Brush* cursor = nullptr, bool showCursor = true, Brush &&cursorStamp = Brush(),
	const Brush* selection = nullptr,
	Texture* overlay = nullptr,
	const Math::Vec2i* gridSize = nullptr, bool showGrids = false,
	bool showTransparentBackbround = true,
	const Math::Vec2i* anchor = nullptr, bool showAnchor = true,
	const Math::Recti* boundingBox = nullptr, bool showBoundingBox = false
);

/**
 * @param[in, out] cursor
 * @param[in, out] rawCursor
 * @param[in, out] hovering
 */
bool actors(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const char* id,
	const Scene* scene,
	const Map* map, const Math::Vec2i &tileSize,
	float width = -1.0f,
	bool actorsOnly = false, Byte actorAlpha = 255,
	Tiler* cursor = nullptr, bool showCursor = true,
	const Tiler* selection = nullptr,
	Math::Vec2i* rawCursor = nullptr,
	ActorIndex::Array* hovering = nullptr,
	const Math::Vec2i* showGrids = nullptr,
	bool showTransparentBackbround = true,
	MapCelGetter getCel = nullptr
);

/**
 * @param[in, out] cursor
 * @param[in, out] rawCursor
 * @param[in, out] hovering
 */
bool actors(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const Scene* scene,
	const Map* map, const Math::Vec2i &tileSize,
	float width = -1.0f,
	bool actorsOnly = false, Byte actorAlpha = 255,
	Tiler* cursor = nullptr, bool showCursor = true,
	const Tiler* selection = nullptr,
	Math::Vec2i* rawCursor = nullptr,
	ActorIndex::Array* hovering = nullptr,
	const Math::Vec2i* showGrids = nullptr,
	bool showTransparentBackbround = true,
	MapCelGetter getCel = nullptr
);

/**
 * @param[in, out] cursor
 * @param[in, out] rawCursor
 * @param[in, out] hovering
 */
bool triggers(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const Trigger::Array* triggers,
	const Map* map, const Math::Vec2i &tileSize,
	float width = -1.0f,
	float alpha = 1.0f,
	Tiler* cursor = nullptr, const Tiler* selection = nullptr,
	Math::Vec2i* rawCursor = nullptr,
	TriggerIndex::Array* hovering = nullptr,
	SceneTriggerQuerier queryTrigger = nullptr
);

bool camera(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const Map* map, const Math::Vec2i &tileSize,
	const Math::Vec2i &pos,
	float width = -1.0f,
	int magnification = 1
);

/**
 * @param[in, out] ptr
 * @param[in, out] cursor
 * @param[in, out] contentSize
 */
bool sfx(
	Renderer* rnd, Workspace* ws,
	SfxAssets* ptr,
	float width = -1.0f, 
	int* cursor = nullptr,
	Math::Vec2i* contentSize = nullptr, int* locateToIndex = nullptr,
	const int* playingIndex = nullptr,
	bool showShape = true,
	const char* const* byteNumbers = nullptr,
	IndexedOperationHandler operate = nullptr
);

/**< Utilities for tools area. */

namespace Tools {

enum PaintableTools {
	HAND,
	EYEDROPPER,
	PENCIL,
	PAINTBUCKET,
	LASSO,
	LINE,
	BOX,
	BOX_FILL,
	ELLIPSE,
	ELLIPSE_FILL,
	STAMP,
	SMUDGE,
	ERASER,
	MOVE,
	RESIZE,
	REF,
	COUNT
};

enum BitwiseOperations {
	SET,
	AND,
	OR,
	XOR
};

enum RotationsAndFlippings {
	ROTATE_CLOCKWISE,
	ROTATE_ANTICLOCKWISE,
	HALF_TURN,
	FLIP_VERTICALLY,
	FLIP_HORIZONTALLY,
	INVALID
};

struct CodeIndex {
	std::string destination;
	int page = -1;
	Either<int, std::string> line = Either<int, std::string>(Left<int>(0));

	CodeIndex();

	bool set(const std::string &idx);
	void setPage(int pg);
	void setLine(int line_);
	void setLabel(const std::string &lbl);
	bool empty(void) const;
	void clear(void);
};

struct TextPage {
	typedef std::vector<TextPage> Array;

	const std::string* text = nullptr;
	AssetsBundle::Categories category = AssetsBundle::Categories::CODE;

	TextPage();
	TextPage(const std::string* txt, AssetsBundle::Categories cat);
};

struct Marker {
	struct Coordinates {
		typedef std::vector<Coordinates> Array;

		AssetsBundle::Categories category = AssetsBundle::Categories::CODE;
		int page = -1;
		int line = -1;
		int column = -1;

		Coordinates();
		Coordinates(AssetsBundle::Categories cat, int pg, int ln, int col);

		bool operator == (const Coordinates &other) const;
		bool operator < (const Coordinates &other) const;
		bool operator > (const Coordinates &other) const;

		int compare(const Coordinates &other) const;

		bool empty(void) const;
		void clear(void);
	};

	Coordinates begin;
	Coordinates end;

	Marker();
	Marker(const Coordinates &begin, const Coordinates &end);

	const Coordinates &min(void) const;
	const Coordinates &max(void) const;

	bool empty(void) const;
	void clear(void);
};

struct SearchResult : public Marker {
	typedef std::vector<SearchResult> Array;

	std::string head;
	std::string body;

	SearchResult();
	SearchResult(const Coordinates &begin, const Coordinates &end, const std::string &head_, const std::string &body_);
};

struct Objects {
	typedef Either<Math::Vec2i, int> Index;

	typedef std::function<void(AssetsBundle::Categories, const Index &, const Math::Vec2i &, const std::string &, const std::string &)> ObjectGetter;
	typedef std::function<void(ObjectGetter)> ObjectsGetter;

	struct Entry {
		typedef std::vector<Entry> Array;

		AssetsBundle::Categories category = AssetsBundle::Categories::NONE;
		Index index = Right<int>(-1);
		Math::Vec2i size;
		std::string name;
		std::string shortName;

		Entry();
		Entry(AssetsBundle::Categories cat, const Index &idx, const Math::Vec2i &size, const std::string &n, const std::string &sn);
	};

	ObjectsGetter getObjects = nullptr;

	Entry::Array indices;

	Objects();
	Objects(ObjectsGetter getObjs);

	void clear(void);
	void fill(void);
};

struct Album {
	std::string artist;
	std::string comment;

	Album();
	~Album();
};

typedef std::initializer_list<const std::string*> InvokableList;

typedef std::function<void(int)> RefDataChangedHandler;

typedef std::function<void(void)> RefButtonClickedHandler;

typedef std::function<void(int)> RefButtonWithIndexClickedHandler;

typedef std::function<bool(float, float)> ChildrenHandler;

typedef std::function<void(Operations)> OperationHandler;

typedef std::function<std::string(const Marker::Coordinates &, Marker &)> TextWordGetter;

typedef std::function<std::string(const Marker::Coordinates &)> TextLineGetter;

typedef std::function<const char* (UInt8)> ByteTooltipGetter;

typedef std::function<void(Workspace*, const std::string &, bool)> WarningHandler;

float unitSize(
	Renderer* rnd, Workspace* ws,
	float width
);

void separate(void);
void separate(
	Renderer* rnd, Workspace* ws,
	float width
);

/**
 * @param[in, out] cursor
 * @param[in, out] initialized
 * @param[in, out] focused
 */
bool jump(
	Renderer* rnd, Workspace* ws,
	int* cursor = nullptr,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	int min = -1, int max = -1
);

/**
 * @param[in, out] cursor
 * @param[in, out] initialized
 * @param[in, out] focused
 * @param[in, out] what
 * @param[in, out] direction
 * @param[in, out] caseSensitive
 * @param[in, out] wholeWord
 * @param[in, out] globalSearch
 */
bool find(
	Renderer* rnd, Workspace* ws,
	Marker* cursor = nullptr,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	const TextPage::Array* textPages = nullptr, std::string* what = nullptr,
	const Marker::Coordinates::Array* max = nullptr,
	int* direction = nullptr,
	bool* caseSensitive = nullptr, bool* wholeWord = nullptr, bool* globalSearch = nullptr,
	bool visible = true,
	TextWordGetter getWord = nullptr
);

/**
 * @param[out] found
 */
void search(
	Renderer* rnd, Workspace* ws,
	SearchResult::Array &found,
	const TextPage::Array &textPages = TextPage::Array(), const std::string &what = "",
	bool caseSensitive = false, bool wholeWord = false, bool globalSearch = false,
	int index = -1,
	TextWordGetter getWord = nullptr, TextLineGetter getLine = nullptr
);

/**
 * @param[out] xOffset
 * @param[out] iconSize
 */
float measure(
	Renderer* rnd, Workspace* ws,
	float* xOffset = nullptr,
	float* unitSize = nullptr,
	float width = -1.0f
);

/**
 * @param[in, out] group
 * @param[in, out] colors
 * @param[in, out] cursor
 */
bool indexable(
	Renderer* rnd, Workspace* ws,
	Colour colors[GBBASIC_PALETTE_PER_GROUP_COUNT],
	int* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true,
	const char* tooltip = nullptr,
	RefDataChangedHandler refDataChanged = nullptr, RefButtonClickedHandler refButtonClicked = nullptr
);

/**
 * @param[in, out] colors
 * @param[in, out] cursor
 */
bool colorable(
	Renderer* rnd, Workspace* ws,
	Colour colors[EDITING_ITEM_COUNT_PER_LINE * 2],
	int* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] cursor
 */
bool paintable(
	Renderer* rnd, Workspace* ws,
	PaintableTools* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true, bool withTilewise = false,
	unsigned mask = 0xffffffff
);

/**
 * @param[in, out] cursor
 */
bool actor(
	Renderer* rnd, Workspace* ws,
	PaintableTools* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true,
	unsigned mask = 0xffffffff
);

/**
 * @param[in, out] cursor
 */
bool trigger(
	Renderer* rnd, Workspace* ws,
	PaintableTools* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true,
	unsigned mask = 0xffffffff
);

/**
 * @param[in, out] newVal
 */
bool byte(
	Renderer* rnd, Workspace* ws,
	UInt8 oldVal,
	UInt8* newVal,
	float width = -1.0f,
	bool* focused = nullptr,
	bool disabled = false,
	const char* prompt = nullptr,
	ByteTooltipGetter getTooltip = nullptr
);

/**
 * @param[in, out] cursor
 */
bool bitwise(
	Renderer* rnd, Workspace* ws,
	BitwiseOperations* cursor /* nullable */,
	float width = -1.0f,
	bool disabled = false,
	const char* tooltip = nullptr, const char* tooltipOps[4] = nullptr
);

/**
 * @param[in, out] cursor
 */
bool magnifiable(
	Renderer* rnd, Workspace* ws,
	int* cursor = nullptr,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] cursor
 */
bool weighable(
	Renderer* rnd, Workspace* ws,
	int* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] cursor
 */
bool flippable(
	Renderer* rnd, Workspace* ws,
	RotationsAndFlippings* cursor /* nullable */,
	float width = -1.0f,
	bool allowShortcuts = true,
	unsigned mask = 0xffffffff
);

/**
 * @param[in, out] data
 * @param[in, out] set
 * @param[in, out] bit
 */
bool maskable(
	Renderer* rnd, Workspace* ws,
	Byte* data, bool* set /* nullable */, int* bit /* nullable */,
	Byte editable,
	float width = -1.0f,
	bool disabled = false,
	const char* prompt = nullptr,
	const char* tooltipLnHigh = nullptr, const char* tooltipLnLow = nullptr,
	const char* tooltipBits[8] = nullptr
);
/**
 * @param[in, out] data
 */
bool maskable(
	Renderer* rnd, Workspace* ws,
	Byte* data,
	Byte editable,
	float width = -1.0f,
	bool disabled = false,
	const char* prompt = nullptr,
	const char* tooltipLnHigh = nullptr, const char* tooltipLnLow = nullptr,
	const char* tooltipBits[8] = nullptr
);

bool clickable(
	Renderer* rnd, Workspace* ws,
	float width = -1.0f,
	const char* prompt = nullptr
);

/**
 * @param[in, out] value
 */
bool togglable(
	Renderer* rnd, Workspace* ws,
	bool* value,
	float width = -1.0f,
	const char* prompt = nullptr,
	const char* tooltip = nullptr
);

/**
 * @param[in, out] newVal
 * @param[in, out] focused
 */
bool limited(
	Renderer* rnd, Workspace* ws,
	int oldVal[4],
	int* newVal,
	const int minVal[4], const int maxVal[4],
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newWidth
 * @param[in, out] newHeight
 * @param[in, out] focused
 */
bool resizable(
	Renderer* rnd, Workspace* ws,
	int oldWidth, int oldHeight,
	int* newWidth, int* newHeight,
	int maxWidth, int maxHeight, int maxTotal,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newWidth
 * @param[in, out] newHeight
 * @param[in, out] focused
 */
bool resizable(
	Renderer* rnd, Workspace* ws,
	int oldWidth, int oldHeight,
	int* newWidth, int* newHeight,
	int minWidth, int maxWidth, int minHeight, int maxHeight,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newSize
 * @param[in, out] focused
 */
bool resizable(
	Renderer* rnd, Workspace* ws,
	int oldSize,
	int* newSize,
	int minSize, int maxSize,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newX
 * @param[in, out] newY
 * @param[in, out] focused
 */
bool anchorable(
	Renderer* rnd, Workspace* ws,
	int oldX, int oldY,
	int* newX, int* newY,
	int minX, int minY, int maxX, int maxY,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	const char* tooltipApplyAll = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newPitch
 * @param[in, out] focused
 */
bool pitchable(
	Renderer* rnd, Workspace* ws,
	int width_, int height_, bool sizeChanged,
	int oldPitch,
	int* newPitch,
	int maxPitch,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newOffset
 * @param[in, out] newStrength
 * @param[in, out] focused
 */
bool offsetable(
	Renderer* rnd, Workspace* ws,
	int oldOffset, int* newOffset, int minOffset, int maxOffset, const int* maxOffsetLimit /* nullable */,
	float oldStrength, float* newStrength, float minStrength, float maxStrength,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newOffset
 * @param[in, out] newStrength
 * @param[in, out] newDirection
 * @param[in, out] focused
 */
bool offsetable(
	Renderer* rnd, Workspace* ws,
	int oldOffset, int* newOffset, int minOffset, int maxOffset, const int* maxOffsetLimit /* nullable */,
	float oldStrength, float* newStrength, float minStrength, float maxStrength,
	UInt8 oldDirection, UInt8* newDirection,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* txtOutOfBounds = nullptr,
	WarningHandler warn = nullptr
);

/**
 * @param[in, out] newValue
 * @param[in, out] focused
 */
bool colorable(
	Renderer* rnd, Workspace* ws,
	const Colour &oldValue,
	Colour &newValue,
	bool hasAlpha = false,
	float width = -1.0f,
	bool* focused = nullptr,
	const char* prompt = nullptr
);

/**
 * @param[in, out] newName
 * @param[in, out] initialized
 * @param[in, out] focused
 */
bool namable(
	Renderer* rnd, Workspace* ws,
	const std::string &oldName, std::string &newName,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	const char* prompt = nullptr
);

/**
 * @param[in, out] cursor
 */
bool actable(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	float width = -1.0f,
	Byte* cursor = nullptr
);

/**
 * @param[in, out] newRoutines
 * @param[in, out] unfolded
 * @param[in, out] height
 */
bool invokable(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const InvokableList* oldRoutines, Text::Array* newRoutines,
	const InvokableList* types, const InvokableList* tooltips,
	bool* unfolded = nullptr,
	float width = -1.0f, float* height = nullptr,
	const char* prompt = nullptr,
	ChildrenHandler prevChildren = nullptr,
	RefButtonWithIndexClickedHandler refButtonClicked = nullptr
);

/**
 * @param[out] save
 */
bool compacted(
	Renderer* rnd, Workspace* ws,
	Texture* tex,
	bool filled,
	bool* save = nullptr,
	float width = -1.0f,
	const char* prompt = nullptr
);

/**
 * @param[out] selected
 */
bool listable(
	Renderer* rnd, Workspace* ws,
	const Objects &objs,
	int* selected,
	float width = -1.0f,
	const char* prompt = nullptr
);

/**
 * @param[in, out] newChannels
 * @param[in, out] channelIndex
 * @param[in, out] initialized
 * @param[in, out] focused
 * @param[in, out] lineIndex
 * @param[in, out] editing
 */
bool orderable(
	Renderer* rnd, Workspace* ws,
	const Music::Channels &oldChannels, Music::Channels &newChannels,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	const Math::Vec2i &charSize = Math::Vec2i(),
	int* channelIndex = nullptr, int* lineIndex = nullptr,
	bool* editing = nullptr,
	const char* prompt = nullptr,
	const char* const* byteNumbers = nullptr
);

/**
 * @param[in, out] cursor
 */
bool channels(
	Renderer* rnd, Workspace* ws,
	int* cursor = nullptr,
	float width = -1.0f,
	bool allowShortcuts = true
);

/**
 * @param[in, out] newDutyInst
 * @param[in, out] newWaveInst
 * @param[in, out] newNoiseInst
 * @param[in, out] instWaveform
 * @param[in, out] instWaveformMouseDown
 * @param[in, out] instMacros
 * @param[in, out] instMacrosMouseDown
 * @param[in, out] unfolded
 * @param[in, out] cursor
 * @param[in, out] height
 */
bool instruments(
	Renderer* rnd, Workspace* ws,
	Music* ptr,
	Music::DutyInstrument* newDutyInst /* nullable */,
	Music::WaveInstrument* newWaveInst /* nullable */,
	Music::NoiseInstrument* newNoiseInst /* nullable */,
	std::array<int, GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT>* instWaveform, bool* instWaveformMouseDown,
	Music::NoiseInstrument::Macros* instMacros, bool* instMacrosMouseDown,
	bool changedInst,
	bool* unfolded = nullptr,
	int* cursor = nullptr,
	float width = -1.0f, float* height = nullptr,
	const char* prompt = nullptr,
	OperationHandler operate = nullptr
);

/**
 * @param[in, out] newWave
 * @param[in, out] initialized
 * @param[in, out] focused
 */
bool waveform(
	Renderer* rnd, Workspace* ws,
	const std::string &oldWave, std::string &newWave,
	float width = -1.0f,
	bool* initialized = nullptr, bool* focused = nullptr,
	const char* prompt = nullptr
);

/**
 * @param[in, out] newSound
 * @param[in, out] buffer
 * @param[in, out] initialized
 * @param[in, out] focused
 */
bool sound(
	Renderer* rnd, Workspace* ws,
	const std::string &oldSound, std::string &newSound,
	float width = -1.0f,
	char* buffer = nullptr, size_t bufferSize = 0,
	bool* initialized = nullptr, bool* focused = nullptr,
	const char* prompt = nullptr
);

/**
 * @param[in, out] newDef
 * @param[in, out] unfolded
 * @param[in, out] viewAsActor
 * @param[in, out] height
 * @param[in, out] focused
 */
bool definable(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const ::active_t* oldDef /* nullable */, ::active_t* newDef,
	bool* unfolded = nullptr,
	bool* viewAsActor = nullptr,
	float width = -1.0f, float* height = nullptr,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	bool simplified = true,
	OperationHandler operate = nullptr
);

/**
 * @param[in, out] newDef
 * @param[in, out] unfolded
 * @param[in, out] height
 * @param[in, out] focused
 */
bool definable(
	Renderer* rnd, Workspace* ws,
	const Scene* ptr,
	const ::scene_t* oldDef /* nullable */, ::scene_t* newDef,
	bool* unfolded = nullptr,
	float width = -1.0f, float* height = nullptr,
	bool* focused = nullptr,
	const char* prompt = nullptr,
	OperationHandler operate = nullptr
);

/**
 * @param[in, out] newDef
 * @param[in, out] unfolded
 * @param[in, out] height
 */
bool definable(
	Renderer* rnd, Workspace* ws,
	const Music* ptr,
	Album* newDef,
	bool* unfolded = nullptr,
	float width = -1.0f, float* height = nullptr,
	const char* prompt = nullptr
);

}

}

/* ===========================================================================} */

#endif /* __EDITING_H__ */
