/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editing.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/encoding.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Utilities
*/

namespace Editing {

static const char* editingTextOffset(const char* code, int ln, int col) {
	// To line.
	if (ln > 0) {
		while (*code) {
			const char ch = *code;
			if (ch == '\n') {
				if (--ln == 0) {
					++code;

					break;
				}
			}
			++code;
		}
	}

	// To column.
	for (int i = 0; i < col; ++i) {
		if (*code == '\0')
			break;

		const int n = Unicode::expectUtf8(code);
		code += n;
	}

	return code;
}
static const char* editingTextFindForward(const char* txt, const char* const what, int ln, int col, int* lnoff, int* coloff) {
	if (lnoff)
		*lnoff = 0;
	if (coloff)
		*coloff = 0;
	const char* off = editingTextOffset(txt, ln, col);

	const char* mat = strstr(off, what);
	if (mat) { // Found.
		const char* str = off;
		while (str < mat) {
			if (*str == '\0')
				break;
			if (*str == '\n') {
				++(*lnoff);
				*coloff = 0;
				++str;
			} else {
				const int n = Unicode::expectUtf8(str);
				++(*coloff);
				str += n;
			}
		}
	}

	return mat;
}
static const char* editingTextFindBackward(const char* txt, const char* const what, int ln, int col, int* lnoff, int* coloff) {
	if (lnoff)
		*lnoff = 0;
	if (coloff)
		*coloff = 0;
	const char* off = editingTextOffset(txt, ln, col);

	std::string stdcode;
	stdcode.assign(txt, off);
	size_t mat = stdcode.rfind(what, stdcode.length() - 1);

	if (mat != std::string::npos) { // Found.
		const char* str = txt;
		while (str < txt + mat) {
			if (*str == '\0')
				break;
			if (*str == '\n') {
				++(*lnoff);
				*coloff = 0;
				++str;
			} else {
				const int n = Unicode::expectUtf8(str);
				++(*coloff);
				str += n;
			}
		}

		return txt + mat;
	}

	return nullptr;
}

static const std::string* editingTextAtPage(const Tools::TextPage::Array &textPages, bool caseSensitive, int page, std::string &cache) {
	const std::string* txt = textPages[page].text;
	if (!caseSensitive) {
		cache = *txt;
		Text::toLowerCase(cache);
		txt = &cache;
	}

	return txt;
}
static bool editingTextFill(Tools::Marker &cursor, const Tools::Marker::Coordinates &nbegin, const Tools::Marker::Coordinates &nend, Tools::TextWordGetter getWord) {
	if (getWord) {
		Tools::Marker src;
		getWord(nbegin, src);
		if ((src.begin == nbegin && src.end == nend) || (src.begin == nend && src.end == nbegin)) {
			cursor.begin = nbegin;
			cursor.end = nend;

			return true;
		}
	} else {
		cursor.begin = nbegin;
		cursor.end = nend;

		return true;
	}

	return false;
}

static void editingBeginAppliable(Theme* theme) {
	ImGui::PushStyleColor(ImGuiCol_Button, theme->style()->highlightButtonColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme->style()->highlightButtonHoveredColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme->style()->highlightButtonActiveColor);
}
static void editingEndAppliable(void) {
	ImGui::PopStyleColor(3);
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Editing
*/

namespace Editing {

Dots::Dots(Colour* col) : colored(col) {
}

Dots::Dots(int* idx) : indexed(idx) {
}

Dot::Dot() {
	colored = Colour(0, 0, 0, 0);
}

Dot::Dot(const Colour &col) : colored(col) {
}

Dot::Dot(int idx) : indexed(idx) {
}

Dot::Dot(const Dot &other) {
	memcpy(this, &other, sizeof(Dot));
}

Dot &Dot::operator = (const Dot &other) {
	memcpy(this, &other, sizeof(Dot));

	return *this;
}

bool Dot::operator == (const Dot &other) const {
	return colored == other.colored && indexed == other.indexed;
}

bool Dot::operator != (const Dot &other) const {
	return colored != other.colored || indexed != other.indexed;
}

Point::Point() {
	dot.colored = Colour(0, 0, 0, 0);
}

Point::Point(const Math::Vec2i &pos) : position(pos) {
}

Point::Point(const Math::Vec2i &pos, const Dot &d) : position(pos), dot(d) {
}

Point::Point(const Math::Vec2i &pos, const Colour &col) : position(pos) {
	dot.colored = col;
}

Point::Point(const Math::Vec2i &pos, int idx) : position(pos) {
	dot.indexed = idx;
}

bool Point::operator < (const Point &other) const {
	return position < other.position;
}

Painting::Painting() {
}

Painting::operator bool (void) const {
	return _value;
}

Painting &Painting::set(bool val) {
	_lastValue = _value;
	_value = val;

	_lastPosition = _position;
	const ImVec2 pos = ImGui::GetMousePos();
	_position = Math::Vec2f(pos.x, pos.y);

	return *this;
}

bool Painting::moved(void) const {
	return _lastPosition != _position;
}

bool Painting::down(void) const {
	return !_lastValue && _value;
}

bool Painting::up(void) const {
	return _lastValue && !_value;
}

void Painting::clear(void) {
	_lastValue = false;
	_value = false;
	_lastPosition = Math::Vec2f(-1, -1);
	_position = Math::Vec2f(-1, -1);
}

Brush::Brush() {
}

bool Brush::operator == (const Brush &other) const {
	return
		position == other.position &&
		size == other.size;
}

bool Brush::operator != (const Brush &other) const {
	return
		position != other.position ||
		size != other.size;
}

bool Brush::operator == (const Math::Recti &other) const {
	return
		position.x == other.xMin() &&
		position.y == other.yMin() &&
		size.x == other.width() &&
		size.y == other.height();
}

bool Brush::operator != (const Math::Recti &other) const {
	return
		position.x != other.xMin() ||
		position.y != other.yMin() ||
		size.x != other.width() ||
		size.y != other.height();
}

bool Brush::empty(void) const {
	return position == Math::Vec2i(-1, -1);
}

Math::Vec2i Brush::unit(void) const {
	if (size.x <= 0 || size.y <= 0)
		return position;

	return Math::Vec2i(position.x / size.x, position.y / size.y);
}

Math::Recti Brush::toRect(void) const {
	return Math::Recti::byXYWH(position.x, position.y, size.x, size.y);
}

Brush &Brush::fromRect(const Math::Recti &other) {
	position = Math::Vec2i(other.xMin(), other.yMin());
	size = Math::Vec2i(other.width(), other.height());

	return *this;
}

Tiler::Tiler() {
}

bool Tiler::operator == (const Tiler &other) const {
	return
		position == other.position &&
		size == other.size;
}

bool Tiler::operator != (const Tiler &other) const {
	return
		position != other.position ||
		size != other.size;
}

bool Tiler::empty(void) const {
	return position == Math::Vec2i(-1, -1);
}

Shortcut::Shortcut(
	int key_,
	bool ctrl_, bool shift_, bool alt_,
	bool numLock_, bool capsLock_,
	bool super_
) : key(key_),
	ctrl(ctrl_), shift(shift_), alt(alt_),
	numLock(numLock_), capsLock(capsLock_),
	super(super_)
{
}

bool Shortcut::pressed(bool repeat) const {
	ImGuiIO &io = ImGui::GetIO();

	const SDL_Keymod mod = SDL_GetModState();
	const bool num = !!(mod & KMOD_NUM);
	const bool caps = !!(mod & KMOD_CAPS);

	if (!key && !ctrl && !shift && !alt && !numLock && !capsLock && !super)
		return false;

	if (key && !ImGui::IsKeyPressed(key, repeat))
		return false;

	if (ctrl && !io.KeyCtrl)
		return false;
	else if (!ctrl && io.KeyCtrl)
		return false;

	if (shift && !io.KeyShift)
		return false;
	else if (!shift && io.KeyShift)
		return false;

	if (alt && !io.KeyAlt)
		return false;
	else if (!alt && io.KeyAlt)
		return false;

	if (numLock && !num)
		return false;

	if (capsLock && !caps)
		return false;

	if (super && !io.KeySuper)
		return false;
	else if (!super && io.KeySuper)
		return false;

	return true;
}

bool Shortcut::released(void) const {
	ImGuiIO &io = ImGui::GetIO();

	const SDL_Keymod mod = SDL_GetModState();
	const bool num = !!(mod & KMOD_NUM);
	const bool caps = !!(mod & KMOD_CAPS);

	if (!key && !ctrl && !shift && !alt && !numLock && !capsLock && !super)
		return false;

	if (key && ImGui::IsKeyReleased(key))
		return true;

	if (ctrl && !io.KeyCtrl)
		return true;
	else if (!ctrl && io.KeyCtrl)
		return true;

	if (shift && !io.KeyShift)
		return true;
	else if (!shift && io.KeyShift)
		return true;

	if (alt && !io.KeyAlt)
		return true;
	else if (!alt && io.KeyAlt)
		return true;

	if (numLock && !num)
		return true;

	if (capsLock && !caps)
		return true;

	if (super && !io.KeySuper)
		return true;
	else if (!super && io.KeySuper)
		return true;

	return false;
}

NumberStroke::NumberStroke() {
}

NumberStroke::NumberStroke(Bases base_, int val, int min, int max) : base(base_), value(val), minValue(min), maxValue(max) {
}

bool NumberStroke::filled(int* val, bool repeat) {
	ImGuiIO &io = ImGui::GetIO();

	bool result = false;
	if (io.KeyCtrl || io.KeyAlt || io.KeySuper)
		return result;

	int append = -1;
	int changeTo = -1;
	switch (base) {
	case Bases::HEX:
		if (ImGui::IsKeyPressed(SDL_SCANCODE_A, repeat)) {
			append = 0xa;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_B, repeat)) {
			append = 0xb;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_C, repeat)) {
			append = 0xc;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_D, repeat)) {
			append = 0xd;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_E, repeat)) {
			append = 0xe;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_F, repeat)) {
			append = 0xf;

			break;
		}
		// Fall through.
	case Bases::DEC:
		if (ImGui::IsKeyPressed(SDL_SCANCODE_0, repeat)) {
			append = 0;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_1, repeat)) {
			append = 1;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_2, repeat)) {
			append = 2;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_3, repeat)) {
			append = 3;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_4, repeat)) {
			append = 4;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_5, repeat)) {
			append = 5;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_6, repeat)) {
			append = 6;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_7, repeat)) {
			append = 7;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_8, repeat)) {
			append = 8;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_9, repeat)) {
			append = 9;

			break;
		}
		// Fall through.
	default:
		if (ImGui::IsKeyPressed(SDL_SCANCODE_MINUS, repeat)) {
			changeTo = value - 1;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_EQUALS, repeat)) {
			changeTo = value + 1;

			break;
		} else if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE, repeat) || ImGui::IsKeyPressed(SDL_SCANCODE_BACKSPACE, repeat)) {
			changeTo = minValue;

			break;
		}

		break;
	}

	if (append != -1) {
		const int oldValue = value;
		value *= (int)base;
		value += append;
		if (value > maxValue)
			value = append;
		value = Math::max(value, minValue);
		if (val)
			*val = value;

		result = value != oldValue;
	} else if (changeTo != -1) {
		const int oldValue = value;
		value = Math::clamp(changeTo, minValue, maxValue);
		if (val)
			*val = value;

		result = value != oldValue;
	}

	return result;
}

void NumberStroke::clear(void) {
	value = 0;
}

SymbolLocation::SymbolLocation() {
}

SymbolLocation::SymbolLocation(int b, int a) : bank(b), address(a) {
}

SymbolLocation::SymbolLocation(int b, int a, int s) : bank(b), address(a), size(s) {
}

SymbolTable::SymbolTable() {
}

bool SymbolTable::load(
	Dictionary &dict,
	const std::string &symPath, std::string &symTxt,
	const std::string &aliasesPath, std::string &aliasesTxt,
	ErrorHandler onError
) {
	symTxt.clear();
	aliasesTxt.clear();

	File::Ptr file(File::create());
	if (!file->open(symPath.c_str(), Stream::READ)) {
		onError("Invalid player symbols");

		return false;
	}
	if (!file->readString(symTxt)) {
		file->close();

		onError("Invalid player symbols");

		return false;
	}
	file->close();
	SymbolTable symbols;
	if (!symbols.parseSymbols(symTxt)) {
		onError("Invalid player symbols");

		return false;
	}

	if (!file->open(aliasesPath.c_str(), Stream::READ)) {
		fprintf(stderr, "Invalid player aliases.\n");
	}
	if (!file->readString(aliasesTxt)) {
		fprintf(stderr, "Invalid player aliases.\n");
	}
	file->close();
	if (!symbols.parseAliases(aliasesTxt)) {
		fprintf(stderr, "Failed to parse the aliases.\n");
	}

	for (Dictionary::iterator it = dict.begin(); it != dict.end(); ++it) {
		const std::string &key = it->first;
		const SymbolLocation* symLoc = symbols.find(key);
		if (!symLoc) {
			onError("Invalid player symbols");

			return false;
		}

		it->second = *symLoc;
	}

	return true;
}

bool SymbolTable::parseSymbols(const std::string &symbols) {
	// Prepare.
	std::string symbols_ = symbols;

	// Uniform new line characters to '\n'.
	symbols_ = Text::replace(symbols_, "\r\n", "\n");
	symbols_ = Text::replace(symbols_, "\r", "\n");

	// Split the symbol data into lines.
	Text::Array lines;
	const Text::Array lines_ = Text::split(symbols_, "\n");
	for (std::string ln : lines_) {
		const size_t idx = Text::indexOf(ln, ";");
		if (idx != std::string::npos)
			ln = ln.substr(0, idx);

		ln = Text::trim(ln);

		if (ln.empty())
			continue;

		lines.push_back(ln);
	}

	// Parse the symbol lines.
	for (const std::string &ln : lines) {
		const Text::Array parts = Text::split(ln, " ");
		if (parts.size() != 2)
			continue;

		const std::string &address_ = parts.front();
		std::string name = parts.back();
		const Text::Array addressParts = Text::split(address_, ":");
		if (addressParts.size() != 2)
			continue;

		const std::string &bank = addressParts.front();
		const std::string &address = addressParts.back();

		if (Text::startsWith(name, "_", true))
			name = name.substr(1);
		const int nbank = std::stoi(bank, 0, 16);
		const int naddress = std::stoi(address, 0, 16);
		const SymbolLocation location(nbank, naddress);
		_dictionary.insert(std::make_pair(name, location));
	}

	// Finish.
	return true;
}

bool SymbolTable::parseAliases(const std::string &aliases) {
	rapidjson::Document doc;
	if (!Json::fromString(doc, aliases.c_str()))
		return false;
	if (!doc.IsObject())
		return false;

	for (rapidjson::Value::ConstMemberIterator it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
		const rapidjson::Value &jkey = it->name;
		const rapidjson::Value &jval = it->value;

		const std::string name = jkey.GetString();
		if (!jval.IsObject()) {
			fprintf(stderr, "Object expected for the alias of the \"%s\" symbol.\n", name.c_str());

			continue;
		}

		std::string ref;
		int offset = 0;
		if (!Jpath::get(jval, ref, "ref")) {
			fprintf(stderr, "Cannot read the \"ref\" field of the alias of the \"%s\" symbol.\n", name.c_str());

			continue;
		}
		if (!Jpath::get(jval, offset, "offset")) {
			fprintf(stderr, "Cannot read the \"offset\" field of the alias of the \"%s\" symbol.\n", name.c_str());

			continue;
		}

		const SymbolLocation* refLocation = find(ref);
		if (!refLocation) {
			fprintf(stderr, "Cannot find the \"%s\" ref alias of the \"%s\" symbol.\n", ref.c_str(), name.c_str());

			continue;
		}

		const int nbank = refLocation->bank;
		const int naddress = refLocation->address + offset;
		const SymbolLocation romLocation(nbank, naddress);
		_dictionary.insert(std::make_pair(name, romLocation));
	}

	return true;
}

const SymbolLocation* SymbolTable::find(const std::string &key) const {
	Dictionary::const_iterator it = _dictionary.find(key);
	if (it == _dictionary.end())
		return nullptr;

	return &it->second;
}

ActorIndex::ActorIndex() {
}

ActorIndex::ActorIndex(Byte cel_, const Math::Vec2i &off) : cel(cel_), offset(off) {
}

bool ActorIndex::invalid(void) const {
	return cel == Scene::INVALID_ACTOR();
}

void ActorIndex::clear(void) {
	cel = Scene::INVALID_ACTOR();
	offset = Math::Vec2i();
}

TriggerIndex::TriggerIndex() {
}

TriggerIndex::TriggerIndex(Byte cel_) : cel(cel_) {
}

bool TriggerIndex::invalid(void) const {
	return cel == Scene::INVALID_TRIGGER();
}

void TriggerIndex::clear(void) {
	cel = Scene::INVALID_TRIGGER();
}

bool palette(
	Renderer* rnd, Workspace*,
	const Indexed* ptr,
	float width,
	Colour* col,
	int* cursor, bool showCursor,
	Texture* transparent
) {
	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	const int X_COUNT = (int)std::sqrt(IMAGE_PALETTE_COLOR_COUNT);
	const float size = Math::min(width / X_COUNT, ImGui::GetFontSize() * 4);

	const int count = ptr->count();
	GBBASIC_ASSERT(count == IMAGE_PALETTE_COLOR_COUNT);
	for (int i = 0; i < count; ++i) {
		Colour color;
		ptr->get(i, color);
		const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

		ImGui::PushID(i);

		const ImVec2 pos = ImGui::GetCursorScreenPos();
		bool pressed = false;
		if (color.a < 255 && transparent) {
			ImGui::Image(transparent->pointer(rnd), ImVec2(size, size));
			ImGui::SetCursorScreenPos(pos);
		}
		pressed = ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size));
		if (pressed) {
			result = true;

			if (cursor)
				*cursor = i;
			if (col)
				*col = color;
		}
		if (showCursor) {
			const bool active = cursor && *cursor == i;
			if (active)
				ImGui::Indicator(pos, pos + ImVec2(size, size));
		}
		if ((i + 1) % X_COUNT != 0)
			ImGui::SameLine();

		ImGui::PopID();
	}

	return result;
}

bool tiles(
	Renderer* rnd, Workspace* ws,
	const Image* ptr, Texture* tex,
	float width,
	Brush* cursor, bool showCursor, Brush &&cursorStamp,
	const Brush* selection,
	Texture* overlay,
	const Math::Vec2i* gridSize, bool showGrids,
	bool showTransparentBackbround,
	int mouseActionButton,
	PostHandler post
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	float widgetWidth = (float)ptr->width();
	float widgetHeight = (float)ptr->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth - width) >= Math::EPSILON<float>()) {
		scale = width / widgetWidth;
		widgetHeight = widgetHeight / widgetWidth * width;
		widgetWidth = width;
	}
	const ImVec2 widgetSize(widgetWidth, widgetHeight);

	const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
	do {
		float transparentWidth = 0;
		float transparentHeight = 0;
		if (gridSize && (gridSize->x > GBBASIC_TILE_SIZE || gridSize->y > GBBASIC_TILE_SIZE)) {
			transparentWidth = (float)gridSize->x * 2 * scale;
			transparentHeight = (float)gridSize->y * 2 * scale;
		} else {
			transparentWidth = (float)theme->iconBackground()->width() * scale;
			transparentHeight = (float)theme->iconBackground()->height() * scale;
		}
		ImGui::PushClipRect(curPos, curPos + widgetSize, true);
		for (float j = 0; j < widgetHeight; j += transparentHeight) {
			for (float i = 0; i < widgetWidth; i += transparentWidth) {
				const ImVec2 pos = curPos + ImVec2(i, j);
				ImGui::SetCursorScreenPos(pos);
				if (showTransparentBackbround) {
					ImGui::Image(
						(void*)theme->iconBackground()->pointer(rnd),
						ImVec2(transparentWidth, transparentHeight),
						ImVec2(0, 0), ImVec2(1, 1),
						ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.0f)
					);
				} else {
					ImGui::Image(
						(void*)theme->iconBackground()->pointer(rnd),
						ImVec2(transparentWidth, transparentHeight),
						ImVec2(0, 0), ImVec2(1, 1),
						ImVec4(1, 1, 1, 0.0f), ImVec4(1, 1, 1, 0.0f)
					);
				}
			}
		}
		ImGui::PopClipRect();
	} while (false);
	ImGui::Dummy(ImVec2(0, 1));

	ImGui::SetCursorScreenPos(curPos);
	ImGui::Image(
		(void*)tex->pointer(rnd),
		widgetSize,
		ImVec2(0, 0), ImVec2(1, 1),
		ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.0f)
	);
	if (overlay) {
		ImGui::SetCursorScreenPos(curPos);
		ImGui::Image(
			(void*)overlay->pointer(rnd),
			widgetSize,
			ImVec2(0, 0), ImVec2(1, 1),
			ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.0f)
		);
	}

	if (showGrids && gridSize) {
		const float w = gridSize->x * scale;
		const float h = gridSize->y * scale;
		for (float i = w; i < widgetWidth; i += w) {
			drawList->AddLine(
				curPos + ImVec2(i, 0),
				curPos + ImVec2(i, widgetHeight),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
			);
		}
		for (float j = h; j < widgetHeight; j += h) {
			drawList->AddLine(
				curPos + ImVec2(0, j),
				curPos + ImVec2(widgetWidth, j),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
			);
		}
	}
	drawList->AddRect(
		curPos + ImVec2(-1, -1),
		curPos + ImVec2(widgetWidth, widgetHeight) + ImVec2(1, 1),
		ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
	);

	if (showCursor && cursor) {
		const Math::Recti rect = Math::Recti::byXYWH(cursor->position.x, cursor->position.y, cursor->size.x, cursor->size.y);
		const Int cursorX = rect.xMin();
		const Int cursorY = rect.yMin();
		const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
		if (active) {
			ImVec2 blinkPos((float)cursorX, (float)cursorY);
			blinkPos *= scale;
			blinkPos += curPos;
			ImVec2 blinkSize((float)rect.width(), (float)rect.height());
			blinkSize *= scale;
			float blinkThick = 3;
			if (blinkSize.x <= 3) {
				blinkPos.x -= 1;
				blinkSize.x += 2;
				blinkThick = 1;
			}
			if (blinkSize.y <= 3) {
				blinkPos.y -= 1;
				blinkSize.y += 2;
				blinkThick = 1;
			}
			ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
		}
	}

	if (selection) {
		const Math::Recti rect = Math::Recti::byXYWH(selection->position.x, selection->position.y, selection->size.x, selection->size.y);
		const Int selX = selection->position.x;
		const Int selY = selection->position.y;
		const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
		if (active) {
			ImVec2 blinkPos((float)selX, (float)selY);
			blinkPos *= scale;
			blinkPos += curPos;
			ImVec2 blinkSize((float)selection->size.x, (float)selection->size.y);
			blinkSize *= scale;
			float blinkThick = 3;
			if (blinkSize.x <= 3) {
				blinkPos.x -= 1;
				blinkSize.x += 2;
				blinkThick = 1;
			}
			if (blinkSize.y <= 3) {
				blinkPos.y -= 1;
				blinkSize.y += 2;
				blinkThick = 1;
			}
			ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
		}
	}

	if (ImGui::IsItemHovered()) {
		ImVec2 imgPos = ImGui::GetMousePosOnCurrentItem(&curPos);
		imgPos /= scale;

		const Int x = Math::clamp((int)imgPos.x, 0, ptr->width() - 1);
		const Int y = Math::clamp((int)imgPos.y, 0, ptr->height() - 1);

		if (cursorStamp.empty()) {
			if (ImGui::IsMouseDown(mouseActionButton))
				result = true;

			if (cursor) {
				cursor->position = Math::Vec2i(
					x / cursor->size.x * cursor->size.x,
					y / cursor->size.y * cursor->size.y
				);
			}
		} else if (cursor) {
			if (ImGui::IsMouseClicked(mouseActionButton)) {
				cursorStamp.position = Math::Vec2i(
					x / cursorStamp.size.x,
					y / cursorStamp.size.y
				);
				cursor->position = Math::Vec2i(
					x / cursorStamp.size.x * cursorStamp.size.x,
					y / cursorStamp.size.y * cursorStamp.size.y
				);
				cursor->size = cursorStamp.size;
			} else if (ImGui::IsMouseDown(mouseActionButton)) {
				const Math::Vec2i diff = Math::Vec2i(x / cursorStamp.size.x, y / cursorStamp.size.y) -
					cursorStamp.position +
					Math::Vec2i(1, 1);
				const Math::Recti rect = Math::Recti::byXYWH(cursorStamp.position.x, cursorStamp.position.y, diff.x, diff.y);
				cursor->position = Math::Vec2i(rect.xMin(), rect.yMin()) * cursorStamp.size;
				cursor->size = Math::Vec2i(rect.width(), rect.height()) * cursorStamp.size;
			} else if (ImGui::IsMouseReleased(mouseActionButton)) {
				result = true;
			}
		}
	}

	if (post)
		post();

	return result;
}

bool tiles(
	Renderer* rnd, Workspace* ws,
	const Image* ptr, Texture* tex,
	float width,
	Brush* cursor, bool showCursor,
	const Brush* selection,
	Texture* overlay,
	const Math::Vec2i* gridSize, bool showGrids,
	bool showTransparentBackbround,
	int mouseActionButton,
	PostHandler post
) {
	return tiles(
		rnd, ws,
		ptr, tex,
		width,
		cursor, showCursor, Brush(),
		selection,
		overlay,
		gridSize, showGrids,
		showTransparentBackbround,
		mouseActionButton,
		post
	);
}

bool map(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const char* id,
	const Map* ptr, const Math::Vec2i &tileSize,
	bool paletted,
	float width,
	Tiler* cursor, bool showCursor,
	const Tiler* selection,
	const Math::Vec2i* showGrids,
	bool showTransparentBackbround,
	int ignoreCel,
	MapCelGetter getCel,
	MapCelGetter getPlt,
	MapCelGetter getFlip,
	int mouseActionButton
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	float widgetWidth = (float)ptr->width();
	float widgetHeight = (float)ptr->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth * tileSize.x - width) >= Math::EPSILON<float>()) {
		scale = width / (widgetWidth * tileSize.x);
		widgetHeight = widgetHeight / (widgetWidth * tileSize.x) * width;
		widgetWidth = width / tileSize.x;
	}
	const ImVec2 widgetSize(widgetWidth * tileSize.x, widgetHeight * tileSize.y);

	const float scrollX = ImGui::GetScrollX() / scale;
	const float scrollY = ImGui::GetScrollY() / scale;

	const ImVec2 content = ImGui::GetContentRegionAvail();
	const int beginX = Math::clamp((int)(scrollX / tileSize.x), 0, ptr->width());
	const int beginY = Math::clamp((int)(scrollY / tileSize.y), 0, ptr->height());
	const int endX = Math::clamp((int)std::ceil((scrollX + content.x) / tileSize.x), 0, ptr->width());
	const int endY = Math::clamp((int)std::ceil((scrollY + content.y) / tileSize.y), 0, ptr->height());

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::BeginChildFrame(
		ImGui::GetID(id),
		widgetSize + ImVec2(style.WindowBorderSize * 2 + 2, style.WindowBorderSize * 2 + 2),
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav
	);

	do {
		const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
		const bool hoveringWnd = ImGui::IsMouseHoveringRect(curPos, curPos + widgetSize);

		for (int j = beginY; j < endY; ++j) {
			for (int i = beginX; i < endX; ++i) {
				Math::Recti area;
				Texture::Ptr tex = nullptr;
				int idx = Map::INVALID();
				bool isOverlay = false;
				if (paletted) {
					idx = ptr->get(i, j);
					if (getCel) {
						idx = getCel(Math::Vec2i(i, j));
						if (idx != Map::INVALID()) {
							tex = ptr->at(idx, &area);
							isOverlay = true;
						}
					}
					if (!tex) {
						int plt = 0;
						if (getPlt)
							plt = getPlt(Math::Vec2i(i, j));
						const PaletteAssets::Entry* entry = prj->getPalette(plt);
						if (entry)
							tex = ptr->sub(rnd, i, j, 1, 1, entry->data, plt);
						else
							tex = ptr->sub(rnd, i, j, 1, 1, nullptr, 0);
						area = Math::Recti::byXYWH(0, 0, tileSize.x, tileSize.y);
					}
				} else {
					if (getCel) {
						idx = getCel(Math::Vec2i(i, j));
						if (idx != Map::INVALID()) {
							tex = ptr->at(idx, &area);
							isOverlay = true;
						}
					}
					if (!tex)
						tex = ptr->at(i, j, &area);
				}
				if (!tex)
					continue;

				const ImVec2 pos = ImVec2(i * (float)area.width() * scale, j * (float)area.height() * scale) + curPos;
				const ImVec2 sz = ImVec2((float)area.width(), (float)area.height()) * scale;
				const ImRect rect(pos, pos + sz);
				if (idx != ignoreCel || !getCel) {
					if (showTransparentBackbround) {
						const ImVec2 uv0_(i % 2 * 0.5f, j % 2 * 0.5f);
						const ImVec2 uv1_ = uv0_ + ImVec2(0.5f, 0.5f);
						drawList->AddImage(
							theme->iconBackground()->pointer(rnd),
							rect.Min, rect.Max,
							uv0_, uv1_
						);
					}
					ImVec2 uv0((float)area.xMin() / tex->width(), (float)(area.yMin()) / tex->height());
					ImVec2 uv1((float)(area.xMax() + 1) / tex->width(), (float)(area.yMax() + 1) / tex->height());
					ImU32 col = isOverlay ? (ImU32)Colour(224, 244, 164, 255).toRGBA() : 0xffffffff;
					if (getFlip) {
						const Flips flip = (Flips)getFlip(Math::Vec2i(i, j));
						switch (flip) {
						case Flips::NONE:
							// Do nothing.

							break;
						case Flips::HORIZONTAL:
							std::swap(uv0.x, uv1.x);

							break;
						case Flips::VERTICAL:
							std::swap(uv0.y, uv1.y);

							break;
						case Flips::HORIZONTAL_VERTICAL:
							std::swap(uv0.x, uv1.x);
							std::swap(uv0.y, uv1.y);

							break;
						}
					}
					drawList->AddImage(
						tex->pointer(rnd),
						rect.Min, rect.Max,
						uv0, uv1,
						col
					);
				}

				if (hoveringWnd && focusingWnd && !result) {
					const ImVec2 mousePos = ImGui::GetMousePos();
					if (rect.Contains(mousePos)) {
						if (ImGui::IsMouseDown(mouseActionButton))
							result = true;

						if (cursor)
							cursor->position = Math::Vec2i(i, j);
					}
				}
			}
		}

		if (showGrids) {
			for (int i = beginX + 1; i < endX; ++i) {
				drawList->AddLine(
					curPos + ImVec2(i * (float)tileSize.x * scale, 0),
					curPos + ImVec2(i * (float)tileSize.x * scale, widgetHeight * (float)tileSize.y),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
				);
			}
			for (int j = beginY + 1; j < endY; ++j) {
				drawList->AddLine(
					curPos + ImVec2(0, j * (float)tileSize.y * scale),
					curPos + ImVec2(widgetWidth * (float)tileSize.x, j * (float)tileSize.y * scale),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
				);
			}
		}
		drawList->AddRect(
			curPos + ImVec2(-1, -1),
			curPos + widgetSize + ImVec2(1, 1),
			ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
		);

		if (showCursor && cursor) {
			const Math::Recti rect = Math::Recti::byXYWH(cursor->position.x, cursor->position.y, cursor->size.x, cursor->size.y);
			const Int cursorX = rect.xMin();
			const Int cursorY = rect.yMin();
			const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
			if (active) {
				ImVec2 blinkPos((float)cursorX, (float)cursorY);
				blinkPos *= scale;
				blinkPos.x *= tileSize.x;
				blinkPos.y *= tileSize.y;
				blinkPos += curPos;
				ImVec2 blinkSize((float)rect.width(), (float)rect.height());
				blinkSize *= scale;
				blinkSize.x *= tileSize.x;
				blinkSize.y *= tileSize.y;
				float blinkThick = 3;
				if (blinkSize.x <= 3) {
					blinkPos.x -= 1;
					blinkSize.x += 2;
					blinkThick = 1;
				}
				if (blinkSize.y <= 3) {
					blinkPos.y -= 1;
					blinkSize.y += 2;
					blinkThick = 1;
				}
				ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
			}
		}

		if (selection) {
			const Math::Recti rect = Math::Recti::byXYWH(selection->position.x, selection->position.y, selection->size.x, selection->size.y);
			const Int selX = selection->position.x;
			const Int selY = selection->position.y;
			const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
			if (active) {
				ImVec2 blinkPos((float)selX, (float)selY);
				blinkPos *= scale;
				blinkPos.x *= tileSize.x;
				blinkPos.y *= tileSize.y;
				blinkPos += curPos;
				ImVec2 blinkSize((float)selection->size.x, (float)selection->size.y);
				blinkSize *= scale;
				blinkSize.x *= tileSize.x;
				blinkSize.y *= tileSize.y;
				float blinkThick = 3;
				if (blinkSize.x <= 3) {
					blinkPos.x -= 1;
					blinkSize.x += 2;
					blinkThick = 1;
				}
				if (blinkSize.y <= 3) {
					blinkPos.y -= 1;
					blinkSize.y += 2;
					blinkThick = 1;
				}
				ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
			}
		}
	} while (false);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndChildFrame();

	return result;
}

bool map(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const Map* ptr, const Math::Vec2i &tileSize,
	bool paletted,
	float width,
	Tiler* cursor, bool showCursor,
	const Tiler* selection,
	const Math::Vec2i* showGrids,
	bool showTransparentBackbround,
	int ignoreCel,
	MapCelGetter getCel,
	MapCelGetter getPlt,
	MapCelGetter getFlip,
	int mouseActionButton
) {
	return map(
		rnd, ws,
		prj,
		"@Bg",
		ptr, tileSize,
		paletted,
		width,
		cursor, showCursor,
		selection,
		showGrids,
		showTransparentBackbround,
		ignoreCel,
		getCel,
		getPlt,
		getFlip,
		mouseActionButton
	);
}

bool frames(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const Text::Array &names,
	float width,
	int* cursor, int* hovering,
	bool* appended,
	bool allowShortcuts,
	bool showAnchor
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	if (hovering)
		*hovering = -1;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (appended)
		*appended = false;

	constexpr const char* const TOOLTIPS[] = {
		"(Ctrl+Shift+`)",
		"(Ctrl+Shift+1)",
		"(Ctrl+Shift+2)",
		"(Ctrl+Shift+3)",
		"(Ctrl+Shift+4)",
		"(Ctrl+Shift+5)",
		"(Ctrl+Shift+6)",
		"(Ctrl+Shift+7)",
		"(Ctrl+Shift+8)",
		"(Ctrl+Shift+9)"
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_GRAVE, true, true),
		Shortcut(SDL_SCANCODE_1, true, true),
		Shortcut(SDL_SCANCODE_2, true, true),
		Shortcut(SDL_SCANCODE_3, true, true),
		Shortcut(SDL_SCANCODE_4, true, true),
		Shortcut(SDL_SCANCODE_5, true, true),
		Shortcut(SDL_SCANCODE_6, true, true),
		Shortcut(SDL_SCANCODE_7, true, true),
		Shortcut(SDL_SCANCODE_8, true, true),
		Shortcut(SDL_SCANCODE_9, true, true)
	};

	const ImVec2 curPos = ImGui::GetCursorScreenPos();
	for (int i = 0; i < ptr->count(); ++i) {
		const Actor::Frame* frame = ptr->get(i);
		const bool selected = cursor ? i == *cursor : false;
		const Texture::Ptr &tex = frame->texture;
		Texture* tex_ = nullptr;
		if (tex && tex->width() > 0 && tex->height() > 0)
			tex_ = tex.get();
		else
			tex_ = theme->iconUnknown();

		const int texw = tex_->width();
		const int texh = tex_->height();
		const float w = width;
		const float h = w * ((float)texh / texw);
		const float h_ = h + ImGui::GetTextLineHeightWithSpacing();

		const ImVec2 pos = ImGui::GetCursorPos();
		const std::string &ntitle = names[i];
		ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0, 0));
		{
			if (ImGui::Selectable(ntitle.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(0, h_)) || (allowShortcuts && focusingWnd && i < GBBASIC_COUNTOF(shortcuts) && shortcuts[i].pressed())) {
				result = true;

				if (cursor)
					*cursor = i;
			}
			if (hovering && ImGui::IsItemHovered()) {
				*hovering = i;
			}
		}
		ImGui::PopStyleColor();
		if (i < GBBASIC_COUNTOF(TOOLTIPS) && ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(TOOLTIPS[i]);
		}

		ImGui::PushClipRect(curPos + pos, curPos + pos + ImVec2(w, h), true);
		{
			ImGui::SetCursorPos(pos);
			const ImVec2 gsize((float)texw / (GBBASIC_TILE_SIZE * 2), (float)texh / (GBBASIC_TILE_SIZE * 2));
			const ImVec2 size(std::floor(w / gsize.x), std::floor(h / gsize.y));
			for (int y = 0; y < gsize.y; ++y) {
				for (int x = 0; x < gsize.x; ++x) {
					ImGui::Image(theme->iconTransparent()->pointer(rnd), size);
					ImGui::SameLine();
				}
				ImGui::NewLine();
			}

			const ImVec2 size_(w, h);
			ImGui::SetCursorPos(pos);
			ImGui::Image(tex_->pointer(rnd), size_);

			do {
				if (!showAnchor)
					break;

				const Math::Vec2i &anchor = frame->anchor;
				const float x = frame->width() > 1 ? (float)anchor.x / (frame->width() - 0.9999f) * w : 0;
				const float y = frame->height() > 1 ? (float)anchor.y / (frame->height() - 0.9999f) * h : 0;
				const ImVec2 pos_ = curPos + ImVec2(1, 1) + pos + ImVec2(x, y) - ImVec2(3, 3);
				drawList->AddImage(
					theme->iconAnchor()->pointer(rnd),
					pos_, pos_ + ImVec2(5, 5),
					ImVec2(0, 0), ImVec2(1, 1),
					0xff0000ff
				);
			} while (false);
		}
		ImGui::PopClipRect();

		ImGui::TextUnformatted(ntitle);
	}

	ImGui::NewLine(3);
	if (ImGui::Button("+", ImVec2(width, 0))) {
		if (appended)
			*appended = true;
	}
	if (ImGui::IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(theme->tooltipActor_AddFrame());
	}
	ImGui::NewLine(1);

	return result;
}

bool frame(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const Image* img, Texture* tex,
	float width,
	int magnification,
	Brush* cursor, bool showCursor,
	const Brush* selection,
	Texture* overlay,
	const Math::Vec2i* gridSize, bool showGrids,
	bool showTransparentBackbround,
	const Math::Vec2i* anchor, bool showAnchor,
	const Math::Recti* boundingBox, bool showBoundingBox,
	int mouseActionButton
) {
	return frame(
		rnd, ws,
		ptr,
		img, tex,
		width,
		magnification,
		cursor, showCursor, Brush(),
		selection,
		overlay,
		gridSize, showGrids,
		showTransparentBackbround,
		anchor, showAnchor,
		boundingBox, showBoundingBox,
		mouseActionButton
	);
}

bool frame(
	Renderer* rnd, Workspace* ws,
	const Actor* /* ptr */,
	const Image* img, Texture* tex,
	float width,
	int /* magnification */,
	Brush* cursor, bool showCursor, Brush &&cursorStamp,
	const Brush* selection,
	Texture* overlay,
	const Math::Vec2i* gridSize, bool showGrids,
	bool showTransparentBackbround,
	const Math::Vec2i* anchor_, bool showAnchor,
	const Math::Recti* boundingBox_, bool showBoundingBox,
	int mouseActionButton
) {
	Theme* theme = ws->theme();

	const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
	const ImVec2 pos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);

	const bool result = tiles(
		rnd, ws,
		img, tex,
		width,
		cursor, showCursor, std::move(cursorStamp),
		selection,
		overlay,
		gridSize, showGrids,
		showTransparentBackbround,
		mouseActionButton,
		[&] (void) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			if ((!anchor_ || !showAnchor) && (!boundingBox_ || !showBoundingBox))
				return;

			float scale = 1;
			if (width > 0 && std::abs((float)img->width() - width) >= Math::EPSILON<float>())
				scale = width / (float)img->width();
			const float w = img->width() * scale;
			const float h = img->height() * scale;

			ImGui::PushClipRect(curPos, curPos + ImVec2(w, h), true);
			{
				if (anchor_ && showAnchor) {
					const Math::Vec2i &anchor = *anchor_;
					const float x = img->width() > 1 ? (float)anchor.x / (img->width() - 0.9999f) * w : 0;
					const float y = img->height() > 1 ? (float)anchor.y / (img->height() - 0.9999f) * h : 0;
					const ImVec2 pos_ = pos + ImVec2(1, 1) + ImVec2(x, y) - ImVec2(3, 3);
					drawList->AddImage(
						theme->iconAnchor()->pointer(rnd),
						pos_, pos_ + ImVec2(5, 5),
						ImVec2(0, 0), ImVec2(1, 1),
						0xff0000ff
					);
				}

				if (boundingBox_ && showBoundingBox) {
					Math::Recti boundingBox = *boundingBox_;
					if (anchor_) {
						boundingBox.x0 += anchor_->x;
						boundingBox.y0 += anchor_->y;
						boundingBox.x1 += anchor_->x;
						boundingBox.y1 += anchor_->y;
					}
					drawList->AddRect(
						curPos + ImVec2((float)boundingBox.xMin() * scale, (float)boundingBox.yMin() * scale),
						curPos + ImVec2((float)(boundingBox.xMax() + 1) * scale, (float)(boundingBox.yMax() + 1) * scale),
						0xff0000ff
					);
				}
			}
			ImGui::PopClipRect();
		}
	);

	return result;
}

bool actors(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const char* id,
	const Scene* scene,
	const Map* map, const Math::Vec2i &tileSize,
	float width,
	bool actorsOnly, Byte actorAlpha,
	Tiler* cursor, bool showCursor,
	const Tiler* selection,
	Math::Vec2i* rawCursor,
	ActorIndex::Array* hovering,
	const Math::Vec2i* showGrids,
	bool showTransparentBackbround,
	MapCelGetter getCel,
	int mouseActionButton
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	float widgetWidth = (float)map->width();
	float widgetHeight = (float)map->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth * tileSize.x - width) >= Math::EPSILON<float>()) {
		scale = width / (widgetWidth * tileSize.x);
		widgetHeight = widgetHeight / (widgetWidth * tileSize.x) * width;
		widgetWidth = width / tileSize.x;
	}
	const ImVec2 widgetSize(widgetWidth * tileSize.x, widgetHeight * tileSize.y);

	const float scrollX = ImGui::GetScrollX() / scale;
	const float scrollY = ImGui::GetScrollY() / scale;

	const ImVec2 editPos = ImGui::GetWindowPos();
	const ImVec2 content = ImGui::GetContentRegionAvail();
	const int beginX = Math::clamp((int)(scrollX / tileSize.x), 0, map->width());
	const int beginY = Math::clamp((int)(scrollY / tileSize.y), 0, map->height());
	const int endX = Math::clamp((int)std::ceil((scrollX + content.x) / tileSize.x), 0, map->width());
	const int endY = Math::clamp((int)std::ceil((scrollY + content.y) / tileSize.y), 0, map->height());

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::BeginChildFrame(
		ImGui::GetID(id),
		widgetSize + ImVec2(style.WindowBorderSize * 2 + 2, style.WindowBorderSize * 2 + 2),
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav
	);

	do {
		const ImVec2 mousePos = ImGui::GetMousePos();
		Math::Vec2i rawCursor_(-1, -1);
		if (rawCursor)
			*rawCursor = rawCursor_;
		if (hovering)
			hovering->clear();
		const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
		const bool hoveringWnd = ImGui::IsMouseHoveringRect(curPos, curPos + widgetSize);

		const Colour DEFAULT_COLORS_ARRAY[] = INDEXED_DEFAULT_COLORS;
		drawList->PushClipRect(curPos, curPos + widgetSize, true);
		const ImRect editRect(editPos, editPos + content);
		const bool hoveringEditArea = editRect.Contains(mousePos);
		for (int j = beginY; j < endY; ++j) {
			for (int i = beginX; i < endX; ++i) {
				const Byte cel = (Byte)scene->actorLayer()->get(i, j);
				if (cel == Scene::INVALID_ACTOR())
					continue;

				const ActorAssets::Entry* entry = prj->getActor(cel);
				if (!entry)
					continue;
				Math::Vec2i anchor;
				ActorAssets::Entry::PaletteResolver resolver = std::bind(&ActorAssets::Entry::colorAt, entry, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
				const Texture::Ptr &tex = entry->figureTexture(&anchor, resolver);
				if (!tex)
					continue;

				const ImVec2 objPos = ImVec2((i * (float)tileSize.x - anchor.x) * scale, (j * (float)tileSize.y - anchor.y) * scale) + curPos;
				const ImVec2 objSize = ImVec2((float)tex->width(), (float)tex->height()) * scale;
				const ImRect objRect(objPos, objPos + objSize);
				const ImVec2 uv0(0, 0);
				const ImVec2 uv1(1, 1);
				drawList->AddImage(
					tex->pointer(rnd),
					objRect.Min, objRect.Max,
					uv0, uv1,
					(ImU32)Colour(255, 255, 255, actorAlpha).toRGBA()
				);
				if (!actorsOnly) {
					if (rawCursor) {
						const ImVec2 exactPos = ImVec2((i * (float)tileSize.x) * scale, (j * (float)tileSize.y) * scale) + curPos;
						const ImVec2 exactSize = ImVec2((float)tileSize.x, (float)tileSize.y) * scale;
						const ImRect exactRect(exactPos, exactPos + exactSize);
						if (exactRect.Contains(mousePos))
							*rawCursor = Math::Vec2i(i, j);
					}
					const float margin = 0.0f; // 8.0f;
					const ImVec2 nearPos = ImVec2((i * (float)tileSize.x - margin) * scale, (j * (float)tileSize.y - margin) * scale) + curPos;
					const ImVec2 nearSize = ImVec2((float)tileSize.x + margin * 2, (float)tileSize.y + margin * 2) * scale;
					const ImRect nearRect(nearPos, nearPos + nearSize);
					if (hoveringEditArea && (objRect.Contains(mousePos) || nearRect.Contains(mousePos))) {
						rawCursor_ = Math::Vec2i(i, j);
						if (hovering) {
							const ImVec2 pos_ = mousePos - curPos;
							const Math::Vec2i pos((Int)(pos_.x / scale / tileSize.x), (Int)(pos_.y / scale / tileSize.y));
							const Math::Vec2i off = Math::Vec2i(i, j) - pos;
							hovering->push_back(ActorIndex(cel, off));
						}

						if (!ws->popupBox()) {
							const Colour col(
								255 - DEFAULT_COLORS_ARRAY[cel].r,
								255 - DEFAULT_COLORS_ARRAY[cel].g,
								255 - DEFAULT_COLORS_ARRAY[cel].b,
								255
							);
							drawList->AddRect(
								objRect.Min, objRect.Max,
								(ImU32)col.toRGBA()
							);
						}
					}
				}
			}
		}
		drawList->PopClipRect();

		if (actorsOnly)
			break;

		for (int j = beginY; j < endY; ++j) {
			for (int i = beginX; i < endX; ++i) {
				Byte cel = Scene::INVALID_ACTOR();
				Math::Recti area;
				Texture::Ptr tex = nullptr;
				if (getCel) {
					const int idx = getCel(Math::Vec2i(i, j));
					if (idx != Map::INVALID()) {
						tex = map->at(idx, &area);
						cel = (Byte)idx;
					}
				} else {
					cel = (Byte)scene->actorLayer()->get(i, j);
				}
				if (!tex)
					tex = map->at(i, j, &area);
				if (!tex)
					continue;

				const ImVec2 pos = ImVec2(i * (float)area.width() * scale, j * (float)area.height() * scale) + curPos;
				const ImVec2 sz = ImVec2((float)area.width(), (float)area.height()) * scale;
				const ImRect rect(pos, pos + sz);
				const ImVec2 uv0((float)area.xMin() / tex->width(), (float)(area.yMin()) / tex->height());
				const ImVec2 uv1((float)(area.xMax() + 1) / tex->width(), (float)(area.yMax() + 1) / tex->height());
				if (showTransparentBackbround) {
					const ImVec2 uv0(i % 2 * 0.5f, j % 2 * 0.5f);
					const ImVec2 uv1 = uv0 + ImVec2(0.5f, 0.5f);
					drawList->AddImage(
						theme->iconBackground()->pointer(rnd),
						rect.Min, rect.Max,
						uv0, uv1
					);
				}
				if (cel == Scene::INVALID_ACTOR()) {
					drawList->AddImage(
						tex->pointer(rnd),
						rect.Min, rect.Max,
						uv0, uv1
					);
				} else {
					const Colour col(
						255 - DEFAULT_COLORS_ARRAY[cel].r,
						255 - DEFAULT_COLORS_ARRAY[cel].g,
						255 - DEFAULT_COLORS_ARRAY[cel].b,
						255
					);
					if (rawCursor_ == Math::Vec2i(i, j)) {
						drawList->AddRect(
							rect.Min, rect.Max,
							(ImU32)col.toRGBA()
						);
					}
					drawList->AddImage(
						tex->pointer(rnd),
						rect.Min, rect.Max,
						uv0, uv1,
						(ImU32)col.toRGBA()
					);
				}

				if (hoveringWnd && focusingWnd && !result) {
					if (rect.Contains(mousePos)) {
						if (ImGui::IsMouseDown(mouseActionButton))
							result = true;

						if (cursor)
							cursor->position = Math::Vec2i(i, j);
					}
				}
			}
		}

		if (showGrids) {
			for (int i = beginX + 1; i < endX; ++i) {
				drawList->AddLine(
					curPos + ImVec2(i * (float)tileSize.x * scale, 0),
					curPos + ImVec2(i * (float)tileSize.x * scale, widgetHeight * (float)tileSize.y),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
				);
			}
			for (int j = beginY + 1; j < endY; ++j) {
				drawList->AddLine(
					curPos + ImVec2(0, j * (float)tileSize.y * scale),
					curPos + ImVec2(widgetWidth * (float)tileSize.x, j * (float)tileSize.y * scale),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
				);
			}
		}
		drawList->AddRect(
			curPos + ImVec2(-1, -1),
			curPos + widgetSize + ImVec2(1, 1),
			ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
		);

		if (showCursor && cursor) {
			const Math::Recti rect = Math::Recti::byXYWH(cursor->position.x, cursor->position.y, cursor->size.x, cursor->size.y);
			const Int cursorX = rect.xMin();
			const Int cursorY = rect.yMin();
			const bool active = Math::intersects(Math::Recti(0, 0, map->width(), map->height()), rect);
			if (active) {
				ImVec2 blinkPos((float)cursorX, (float)cursorY);
				blinkPos *= scale;
				blinkPos.x *= tileSize.x;
				blinkPos.y *= tileSize.y;
				blinkPos += curPos;
				ImVec2 blinkSize((float)rect.width(), (float)rect.height());
				blinkSize *= scale;
				blinkSize.x *= tileSize.x;
				blinkSize.y *= tileSize.y;
				float blinkThick = 3;
				if (blinkSize.x <= 3) {
					blinkPos.x -= 1;
					blinkSize.x += 2;
					blinkThick = 1;
				}
				if (blinkSize.y <= 3) {
					blinkPos.y -= 1;
					blinkSize.y += 2;
					blinkThick = 1;
				}
				ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
			}
		}

		if (selection) {
			const Math::Recti rect = Math::Recti::byXYWH(selection->position.x, selection->position.y, selection->size.x, selection->size.y);
			const Int selX = selection->position.x;
			const Int selY = selection->position.y;
			const bool active = Math::intersects(Math::Recti(0, 0, map->width(), map->height()), rect);
			if (active) {
				ImVec2 blinkPos((float)selX, (float)selY);
				blinkPos *= scale;
				blinkPos.x *= tileSize.x;
				blinkPos.y *= tileSize.y;
				blinkPos += curPos;
				ImVec2 blinkSize((float)selection->size.x, (float)selection->size.y);
				blinkSize *= scale;
				blinkSize.x *= tileSize.x;
				blinkSize.y *= tileSize.y;
				float blinkThick = 3;
				if (blinkSize.x <= 3) {
					blinkPos.x -= 1;
					blinkSize.x += 2;
					blinkThick = 1;
				}
				if (blinkSize.y <= 3) {
					blinkPos.y -= 1;
					blinkSize.y += 2;
					blinkThick = 1;
				}
				ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
			}
		}
	} while (false);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndChildFrame();

	return result;
}

bool actors(
	Renderer* rnd, Workspace* ws,
	const Project* prj,
	const Scene* scene,
	const Map* map, const Math::Vec2i &tileSize,
	float width,
	bool actorsOnly, Byte actorAlpha,
	Tiler* cursor, bool showCursor,
	const Tiler* selection,
	Math::Vec2i* rawCursor,
	ActorIndex::Array* hovering,
	const Math::Vec2i* showGrids,
	bool showTransparentBackbround,
	MapCelGetter getCel,
	int mouseActionButton
) {
	return actors(
		rnd, ws,
		prj,
		"@Bg",
		scene,
		map, tileSize,
		width,
		actorsOnly, actorAlpha,
		cursor, showCursor,
		selection,
		rawCursor,
		hovering,
		showGrids,
		showTransparentBackbround,
		getCel,
		mouseActionButton
	);
}

bool triggers(
	Renderer*, Workspace*,
	const Project*,
	const Trigger::Array* triggers,
	const Map* map, const Math::Vec2i &tileSize,
	float width,
	float alpha,
	Tiler* cursor, const Tiler* selection,
	Math::Vec2i* rawCursor,
	TriggerIndex::Array* hovering,
	SceneTriggerQuerier queryTrigger,
	int mouseActionButton
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	bool result = false;

	float widgetWidth = (float)map->width();
	float widgetHeight = (float)map->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth * tileSize.x - width) >= Math::EPSILON<float>()) {
		scale = width / (widgetWidth * tileSize.x);
		widgetHeight = widgetHeight / (widgetWidth * tileSize.x) * width;
		widgetWidth = width / tileSize.x;
	}
	const ImVec2 widgetSize(widgetWidth * tileSize.x, widgetHeight * tileSize.y);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::BeginChildFrame(
		ImGui::GetID("@Tri"),
		widgetSize + ImVec2(style.WindowBorderSize * 2 + 2, style.WindowBorderSize * 2 + 2),
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav
	);

	const ImVec2 oldPos = ImGui::GetCursorScreenPos();
	ImGui::Dummy(widgetSize);
	ImGui::SetCursorScreenPos(oldPos);

	const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
	do {
		for (int i = 0; i < (int)triggers->size(); ++i) {
			const Trigger &trigger = (*triggers)[i];
			ImVec2 p0((float)trigger.xMin() * tileSize.x, (float)trigger.yMin() * tileSize.x);
			ImVec2 p1((float)(trigger.xMax() + 1) * tileSize.x, (float)(trigger.yMax() + 1) * tileSize.x);
			p0 *= scale;
			p1 *= scale;
			drawList->AddRectFilled(
				curPos + p0,
				curPos + p1,
				ImGui::GetColorU32(ImVec4(trigger.color.r / 255.0f, trigger.color.g / 255.0f, trigger.color.b / 255.0f, 0.3f * alpha))
			);
			drawList->AddRect(
				curPos + p0,
				curPos + p1,
				ImGui::GetColorU32(ImVec4(trigger.color.r / 255.0f, trigger.color.g / 255.0f, trigger.color.b / 255.0f, 0.5f * alpha))
			);
		}
	} while (false);

	if (hovering)
		hovering->clear();
	if (ImGui::IsItemHovered()) {
		ImVec2 imgPos = ImGui::GetMousePosOnCurrentItem(&curPos);
		imgPos /= scale;

		const Int x = Math::clamp((int)imgPos.x, 0, (Int)widgetSize.x - 1);
		const Int y = Math::clamp((int)imgPos.y, 0, (Int)widgetSize.y - 1);

		if (ImGui::IsMouseDown(mouseActionButton))
			result = true;

		if (cursor) {
			cursor->position = Math::Vec2i(
				x / tileSize.x,
				y / tileSize.y
			);
			cursor->size = Math::Vec2i(1, 1);

			if (rawCursor)
				*rawCursor = cursor->position;

			if (queryTrigger) {
				TriggerIndex::Array triggers_;
				if (queryTrigger(cursor->position, triggers_) > 0) {
					if (hovering)
						*hovering = triggers_;

					const Byte idx = triggers_.front().cel;
					const Trigger &trigger = (*triggers)[idx];
					cursor->position = trigger.position();
					cursor->size = trigger.size();
				}
			}
		}
	}

	if (cursor && !selection) {
		const Math::Recti rect = Math::Recti::byXYWH(
			cursor->position.x * tileSize.x, cursor->position.y * tileSize.y,
			cursor->size.x * tileSize.x, cursor->size.y * tileSize.y
		);
		const Int cursorX = rect.xMin();
		const Int cursorY = rect.yMin();
		const bool active = Math::intersects(Math::Recti(0, 0, (Int)widgetSize.x, (Int)widgetSize.y), rect);
		if (active) {
			ImVec2 blinkPos((float)cursorX, (float)cursorY);
			blinkPos *= scale;
			blinkPos += curPos;
			ImVec2 blinkSize((float)rect.width(), (float)rect.height());
			blinkSize *= scale;
			float blinkThick = 3;
			if (blinkSize.x <= 3) {
				blinkPos.x -= 1;
				blinkSize.x += 2;
				blinkThick = 1;
			}
			if (blinkSize.y <= 3) {
				blinkPos.y -= 1;
				blinkSize.y += 2;
				blinkThick = 1;
			}
			ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
		}
	}

	if (selection) {
		const Math::Recti rect = Math::Recti::byXYWH(
			selection->position.x * tileSize.x, selection->position.y * tileSize.y,
			selection->size.x * tileSize.x, selection->size.y * tileSize.y
		);
		const Int selX = rect.xMin();
		const Int selY = rect.yMin();
		const bool active = Math::intersects(Math::Recti(0, 0, (Int)widgetSize.x, (Int)widgetSize.y), rect);
		if (active) {
			ImVec2 blinkPos((float)selX, (float)selY);
			blinkPos *= scale;
			blinkPos += curPos;
			ImVec2 blinkSize((float)rect.width(), (float)rect.height());
			blinkSize *= scale;
			float blinkThick = 3;
			if (blinkSize.x <= 3) {
				blinkPos.x -= 1;
				blinkSize.x += 2;
				blinkThick = 1;
			}
			if (blinkSize.y <= 3) {
				blinkPos.y -= 1;
				blinkSize.y += 2;
				blinkThick = 1;
			}
			ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
		}
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndChildFrame();

	return result;
}

bool camera(
	Renderer*,
	Workspace*,
	const Project*,
	const Map* map, const Math::Vec2i &tileSize,
	const Math::Vec2i &pos,
	float width,
	int magnification
) {
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;

	float widgetWidth = (float)map->width();
	float widgetHeight = (float)map->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth * tileSize.x - width) >= Math::EPSILON<float>()) {
		scale = width / (widgetWidth * tileSize.x) / magnification;
		widgetHeight = widgetHeight / (widgetWidth * tileSize.x) * width;
		widgetWidth = width / tileSize.x;
	}
	const ImVec2 widgetSize(widgetWidth * tileSize.x, widgetHeight * tileSize.y);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::BeginChildFrame(
		ImGui::GetID("@Cam"),
		widgetSize + ImVec2(style.WindowBorderSize * 2 + 2, style.WindowBorderSize * 2 + 2),
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav
	);

	do {
		const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);

		const ImVec2 pos_ = (magnification == -1 ?
				ImVec2((float)pos.x, (float)pos.y) :
				(ImVec2((float)pos.x, (float)pos.y) * (float)magnification)) *
			scale;
		const ImVec2 size = (magnification == -1 ?
				ImVec2(GBBASIC_SCREEN_WIDTH, GBBASIC_SCREEN_HEIGHT) :
				(ImVec2(GBBASIC_SCREEN_WIDTH, GBBASIC_SCREEN_HEIGHT) * (float)magnification)) *
			scale;
		const ImVec2 start(curPos + ImVec2(-1, -1) + pos_);
		const ImVec2 end(start + ImVec2(1, 1) + size);

		ImGui::Indicator(start, end, 1);
	} while (false);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndChildFrame();

	return result;
}

bool sfx(
	Renderer* rnd, Workspace* ws,
	SfxAssets* ptr,
	float width,
	int* cursor,
	Math::Vec2i* contentSize, int* locateToIndex,
	const int* playingIndex,
	bool showShape,
	const char* const* byteNumbers,
	IndexedOperationHandler operate
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	ImVec4 selCol = ImGui::GetStyleColorVec4(ImGuiCol_Header);
	selCol.w *= 0.7f;
	ImVec4 unselBtnCol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	unselBtnCol.w *= 0.3f;

	constexpr const int BORDER_X = 1;
	constexpr const int BORDER_Y = 1;
	constexpr const float PADDING = 1.0f;
	const float availableWidth = width - style.ScrollbarSize;
	const bool toRecalcSize = contentSize->x <= 0 || contentSize->y <= 0;
	const float itemWidth = toRecalcSize ? 100.0f : contentSize->x;
	const int selectableXCount = (int)std::floor(availableWidth / (itemWidth + PADDING * 2));
	const float selectableWidth = std::floor(availableWidth / selectableXCount) - PADDING * 2;
	float selectableMarginX = (selectableWidth - itemWidth) * 0.5f;
	float selectableMarginY = Math::min(selectableMarginX, 8.0f);
	const ImVec2 selectableSize(contentSize->x + selectableMarginX * 2, contentSize->y + selectableMarginY * 2);
	selectableMarginX += BORDER_X;
	selectableMarginY += BORDER_Y;

	const ImVec2 startPos = ImGui::GetCursorPos();
	const int n = ptr->count();
	for (int i = 0; i < n; ++i) {
		ImGui::PushID(i);

		const SfxAssets::Entry* entry = ptr->get(i);
		const Sfx::Ptr &data = entry->data;
		const Sfx::Sound* sound = data->pointer();

		const bool selected = i == *cursor;

		const std::div_t div = std::div(i, selectableXCount);
		const ImVec2 pos = startPos + ImVec2(div.rem * (selectableSize.x + PADDING * 2) + PADDING, div.quot * (selectableSize.y + PADDING * 2) + PADDING);
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Header, selCol);
		ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0, 0));
		{
			if (ImGui::Selectable(byteNumbers[i], selected, ImGuiSelectableFlags_AllowItemOverlap, selectableSize)) {
				*cursor = i;
				result = true;
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !result) {
				*cursor = i;
				result = true;
			}
			if (locateToIndex && *locateToIndex == i) {
				ImGui::ScrollToItem(ImGuiScrollFlags_KeepVisibleEdgeY);
				*locateToIndex = -1;
			}
		}
		ImGui::PopStyleColor(2);
		ImGui::SetCursorPos(pos + ImVec2(selectableMarginX, selectableMarginY));

		const ImVec2 contentStartPos = ImGui::GetCursorPos();

		const std::string &num = byteNumbers[i];
		const char* numptr = num.c_str();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(numptr);
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();

		const float numWidth = ImGui::GetCursorPosX() - contentStartPos.x;

		ImGui::PushID("@Nm");
		do {
			ImGui::SetNextItemWidth(72.0f);
			char buf[ASSETS_NAME_STRING_MAX_LENGTH]; // Fixed size.
			const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, sound->name.length());
			if (n > 0)
				memcpy(buf, sound->name.c_str(), n);
			buf[n] = '\0';
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
			ImGui::PopStyleColor();

			if (ImGui::IsItemClicked() && !result) {
				*cursor = i;
				result = true;
			}
		} while (false);
		ImGui::PopID();

		ImGui::NewLine(1);
		ImGui::SetCursorPosX(pos.x);
		ImGui::Dummy(ImVec2(numWidth + selectableMarginX, 0));
		ImGui::SameLine();

		ImGui::PushID("@Msk");
		{
			constexpr const char* CHANNELS_INITIALS[] = { "D", "D", "W", "N" };
			const char* CHANNELS_TOOLTIPS[] = {
				theme->tooltipAudio_SfxChannelDuty1().c_str(),
				theme->tooltipAudio_SfxChannelDuty2().c_str(),
				theme->tooltipAudio_SfxChannelWave().c_str(),
				theme->tooltipAudio_SfxChannelNoise().c_str()
			};

			const float width_ = (12.0f + style.FramePadding.y * 2) * GBBASIC_COUNTOF(CHANNELS_INITIALS);
			const float height_ = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
			UInt32 mask = sound->mask;
			const ImU32 col = ImGui::ColorConvertFloat4ToU32(theme->style()->builtin[ImGuiCol_Button]);
			const bool clicked = ImGui::BulletButtons(
				width_, height_,
				GBBASIC_COUNTOF(CHANNELS_INITIALS), nullptr, mask, nullptr,
				(const char**)CHANNELS_INITIALS, col,
				(const char**)CHANNELS_TOOLTIPS, ImGui::ColorConvertFloat4ToU32(theme->style()->builtin[ImGuiCol_Text])
			);
			if (clicked && !result) {
				*cursor = i;
				result = true;
			}
		}
		ImGui::PopID();

		if (showShape) {
			ImGui::NewLine(1);
			ImGui::SetCursorPosX(pos.x);
			ImGui::Dummy(ImVec2(numWidth + selectableMarginX, 0));
			ImGui::SameLine();

			Texture::Ptr shapeTex = entry->shape;
			const ImVec2 sz(EDITING_SFX_SHAPE_IMAGE_WIDTH, EDITING_SFX_SHAPE_IMAGE_HEIGHT);
			ImGui::PushID("@Prv");
			if (shapeTex) {
				ImGui::Image(
					shapeTex->pointer(rnd),
					sz,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1),
					ImVec4(0.5f, 0.5f, 0.5f, 1.0f)
				);
			} else {
				ImGui::Image(
					theme->textureWaitingForSoundShape()->pointer(rnd),
					sz,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1),
					ImVec4(0.5f, 0.5f, 0.5f, 1.0f)
				);
			}
			ImGui::PopID();
		}

		ImGui::NewLine(1);
		ImGui::SetCursorPosX(pos.x);
		ImGui::Dummy(ImVec2(numWidth + selectableMarginX, 0));
		ImGui::SameLine();

		if (!selected) {
			ImGui::PushStyleColor(ImGuiCol_Button, unselBtnCol);
		}
		if (playingIndex == nullptr || *playingIndex != i) {
			if (ws->running()) {
				ImGui::BeginDisabled();
				ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipAudio_PlaySfx().c_str());
				ImGui::EndDisabled();
			} else {
				if (ImGui::ImageButton(theme->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme->tooltipAudio_PlaySfx().c_str())) {
					if (operate)
						operate(i, Operations::PLAY);
				}
			}
		} else {
			if (ImGui::ImageButton(theme->iconStopPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme->tooltipAudio_StopSfx().c_str())) {
				if (operate)
					operate(i, Operations::STOP);
			}
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(9, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconImport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme->tooltip_Import().c_str())) {
			if (operate)
				operate(i, Operations::IMPORT);
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconExport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme->tooltip_Export().c_str())) {
			if (operate)
				operate(i, Operations::EXPORT);
		}
		if (!selected) {
			ImGui::PopStyleColor();
		}

		if (i == n - 1) {
			ImGui::SameLine();
			if (toRecalcSize) {
				const float contentWidth = ImGui::GetCursorPosX() - contentStartPos.x;
				contentSize->x = (Int)contentWidth + BORDER_X * 2;
			}
			ImGui::NewLine();
			if (toRecalcSize) {
				const float contentHeight = ImGui::GetCursorPosY() - contentStartPos.y;
				contentSize->y = (Int)contentHeight + BORDER_Y * 2;
			}
		}

		ImGui::PopID();
	}

	return result;
}

namespace Tools {

CodeIndex::CodeIndex() {
}

bool CodeIndex::set(const std::string &idx) {
	destination = idx;

	if (destination.empty())
		destination = "#0";

	Text::Array parts = Text::split(destination, ":");
	if (parts.size() == 1) {
		if (parts.front().length() < 2 || !Text::startsWith(parts.front(), "#", false))
			return false;

		if (!Text::fromString(parts.front().substr(1), page))
			return false;

		line = Either<int, std::string>(Left<int>(0));
	} else if (parts.size() == 2) {
		for (std::string &part : parts)
			part = Text::trim(part);

		if (parts.front().length() < 2 || !Text::startsWith(parts.front(), "#", false))
			return false;

		if (!Text::fromString(parts.front().substr(1), page))
			return false;

		int ln = -1;
		if (Text::fromString(parts.back(), ln) && ln >= 1)
			line = Either<int, std::string>(Left<int>(ln - 1));
		else
			line = Either<int, std::string>(Right<std::string>(parts.back()));
	} else {
		return false;
	}

	return true;
}

void CodeIndex::setPage(int pg) {
	page = pg;

	if (line.isLeft()) {
		const int ln = line.left().get();
		if (ln == 0)
			destination = "#" + Text::toString(page);
		else
			destination = "#" + Text::toString(page) + ":" + Text::toString(ln + 1);
	} else {
		destination = "#" + Text::toString(page) + ":" + line.right().get();
	}
}

void CodeIndex::setLine(int line_) {
	line = Either<int, std::string>(Left<int>(line_));

	const int ln = line.left().get();
	if (ln == 0)
		destination = "#" + Text::toString(page);
	else
		destination = "#" + Text::toString(page) + ":" + Text::toString(ln + 1);
}

void CodeIndex::setLabel(const std::string &lbl) {
	line = Either<int, std::string>(Right<std::string>(lbl));

	destination = "#" + Text::toString(page) + ":" + line.right().get();
}

bool CodeIndex::empty(void) const {
	return destination.empty();
}

void CodeIndex::clear(void) {
	destination.clear();
	page = -1;
	line = Either<int, std::string>(Left<int>(0));
}

TextPage::TextPage() {
}

TextPage::TextPage(const std::string* txt, AssetsBundle::Categories cat) :
	text(txt), category(cat)
{
}

Marker::Coordinates::Coordinates() {
}

Marker::Coordinates::Coordinates(AssetsBundle::Categories cat, int pg, int ln, int col) :
	category(cat), page(pg), line(ln), column(col)
{
}

bool Marker::Coordinates::operator == (const Coordinates &other) const {
	return compare(other) == 0;
}

bool Marker::Coordinates::operator < (const Coordinates &other) const {
	return compare(other) < 0;
}

bool Marker::Coordinates::operator > (const Coordinates &other) const {
	return compare(other) > 0;
}

int Marker::Coordinates::compare(const Coordinates &other) const {
	if ((unsigned)category < (unsigned)other.category)
		return -1;
	else if ((unsigned)category > (unsigned)other.category)
		return 1;

	if (page < other.page)
		return -1;
	else if (page > other.page)
		return 1;

	if (line < other.line)
		return -1;
	else if (line > other.line)
		return 1;

	if (column < other.column)
		return -1;
	else if (column > other.column)
		return 1;

	return 0;
}

bool Marker::Coordinates::empty(void) const {
	return page == -1 && line == -1 && column == -1;
}

void Marker::Coordinates::clear(void) {
	page = -1;
	line = -1;
	column = -1;
}

Marker::Marker() {
}

Marker::Marker(const Coordinates &begin_, const Coordinates &end_) : begin(begin_), end(end_) {
}

const Marker::Coordinates &Marker::min(void) const {
	if (begin < end)
		return begin;

	return end;
}

const Marker::Coordinates &Marker::max(void) const {
	if (begin > end)
		return begin;

	return end;
}

bool Marker::empty(void) const {
	return begin.empty() || end.empty();
}

void Marker::clear(void) {
	begin.clear();
	end.clear();
}

SearchResult::SearchResult() {
}

SearchResult::SearchResult(const Coordinates &begin, const Coordinates &end, const std::string &head_, const std::string &body_) :
	Marker(begin, end), head(head_), body(body_)
{
}

Objects::Entry::Entry() {
}

Objects::Entry::Entry(AssetsBundle::Categories cat, const Index &idx, const Math::Vec2i &size_, const std::string &n, const std::string &sn) :
	category(cat),
	index(idx),
	size(size_),
	name(n),
	shortName(sn)
{
}

Objects::Objects() {
}

Objects::Objects(ObjectsGetter getObjs) : getObjects(getObjs) {
}

void Objects::clear(void) {
	indices.clear();
}

void Objects::fill(void) {
	indices.clear();
	getObjects(
		[&] (AssetsBundle::Categories cat, const Index &idx, const Math::Vec2i &size, const std::string &n, const std::string &sn) -> void {
			indices.push_back(Entry(cat, idx, size, n, sn));
		}
	);
}

Album::Album() {
}

Album::~Album() {
}

float unitSize(
	Renderer*, Workspace* ws,
	float width
) {
	Theme* theme = ws->theme();

	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE - 1;
	float size = width / (X_COUNT + 1);
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
	}

	return size;
}

void separate(void) {
	ImGui::NewLine(1);
	ImGui::Separator();
	ImGui::NewLine(2);
}

void separate(
	Renderer*,
	Workspace* ws,
	float width
) {
	Theme* theme = ws->theme();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::NewLine(1);
	const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(xOffset + 1, 0);
	drawList->AddLine(
		curPos,
		curPos + ImVec2(size * X_COUNT - 2, 0),
		ImGui::GetColorU32(ImGuiCol_Separator)
	);
	ImGui::NewLine(2);
}

bool jump(
	Renderer* rnd, Workspace* ws,
	int* cursor,
	float width,
	bool* initialized, bool* focused,
	int min, int max
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (!cursor)
		return result;

	const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

	const float x = ImGui::GetCursorPosX();
	ImGui::Dummy(ImVec2(8, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(theme->dialogPrompt_Goto());

	ImGui::PushID("@Jmp");
	do {
		ImGui::SameLine();
		if (!*initialized) {
			ImGui::SetKeyboardFocusHere();
			*initialized = true;
		}
		ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 2);
		char buf[16];
		snprintf(buf, GBBASIC_COUNTOF(buf), "%d", *cursor + 1);
		const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		if (!edited)
			break;
		int ln = 0;
		if (!Text::fromString(buf, ln))
			break;
		--ln;
		if (min >= 0 && max >= 0 && (ln < min || ln > max))
			break;

		result = true;

		*cursor = ln;
	} while (false);
	ImGui::PopID();

	ImGui::SameLine();
	if (ImGui::ImageButton(theme->iconLeft()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor))) {
		result = true;

		--*cursor;
		if (min >= 0 && *cursor < min)
			*cursor = min;
	}

	ImGui::SameLine();
	if (ImGui::ImageButton(theme->iconRight()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor))) {
		result = true;

		++*cursor;
		if (max >= 0 && *cursor > max)
			*cursor = max;
	}

	return result;
}

bool find(
	Renderer* rnd, Workspace* ws,
	Marker* cursor,
	float width,
	bool* initialized, bool* focused,
	const TextPage::Array* textPages, std::string* what,
	const Marker::Coordinates::Array* max,
	int* direction,
	bool* caseSensitive, bool* wholeWord, bool* globalSearch,
	bool visible,
	TextWordGetter getWord
) {
	typedef std::function<void(int &)> FindViewHandler;

	auto findOne = [] (
		Renderer*, Workspace*,
		Marker* cursor,
		bool* focused,
		const TextPage::Array* textPages, std::string* what,
		const Marker::Coordinates::Array* max,
		int* direction,
		bool* caseSensitive, bool* wholeWord, bool* globalSearch,
		FindViewHandler findView,
		TextWordGetter getWord
	) -> bool {
		bool result = false;

		if (focused)
			*focused = false;

		if (!cursor || !what)
			return result;

		int step = 0;
		if (direction) {
			step = *direction;
			*direction = 0;
		}

		findView(step);

		if (step && what->empty())
			step = 0;
		if (step == 1) { // Forward.
			std::string pat = *what;
			if (!caseSensitive || !*caseSensitive) {
				Text::toLowerCase(pat);
			}
			const std::wstring widepat = Unicode::toWide(what->c_str());
			AssetsBundle::Categories category = cursor->begin.category;
			int page = cursor->begin.page;
			std::string tmp;
			const std::string* text = editingTextAtPage(*textPages, caseSensitive && *caseSensitive, page, tmp);
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindForward(text->c_str(), pat.c_str(), cursor->max().line, cursor->max().column, &lnoff, &coloff);
			if (mat) { // Found.
				Marker::Coordinates nbegin(category, page, cursor->begin.line + lnoff, coloff);
				Marker::Coordinates nend(category, page, cursor->begin.line + lnoff, coloff + (int)widepat.length());
				if (lnoff == 0) {
					nbegin.column += cursor->max().column;
					nend.column += cursor->max().column;
				}

				result = editingTextFill(*cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			} else { // Not found.
				// Open next page for global search.
				if (*globalSearch && textPages->size() > 1) {
					if (++page >= (int)textPages->size())
						page = 0;
					text = editingTextAtPage(*textPages, caseSensitive && *caseSensitive, page, tmp);
				}

				// Find again from the beginning.
			_forwardAgain:
				mat = editingTextFindForward(text->c_str(), pat.c_str(), 0, 0, &lnoff, &coloff);
				if (mat) { // Found.
					const Marker::Coordinates nbegin(category, page, lnoff, coloff);
					const Marker::Coordinates nend(category, page, lnoff, coloff + (int)widepat.length());

					result = editingTextFill(*cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
				} else { // Not found.
					if (++page >= (int)textPages->size())
						page = 0;
					if (page != cursor->begin.page) {
						text = editingTextAtPage(*textPages, caseSensitive && *caseSensitive, page, tmp);

						goto _forwardAgain;
					}
				}
			}
		} else if (step == -1) { // Backward.
			Marker::Coordinates pos = cursor->min();
			std::string pat = *what;
			if (!caseSensitive || !*caseSensitive) {
				Text::toLowerCase(pat);
			}
			std::wstring widepat = Unicode::toWide(what->c_str());
			AssetsBundle::Categories category = cursor->begin.category;
			int page = cursor->begin.page;
			std::string tmp;
			const std::string* text = editingTextAtPage(*textPages, caseSensitive && *caseSensitive, page, tmp);
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindBackward(text->c_str(), pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(category, page, lnoff, coloff);
				const Marker::Coordinates nend(category, page, lnoff, coloff + (int)widepat.length());

				result = editingTextFill(*cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			} else { // Not found.
				// Open previous page for global search.
				if (*globalSearch && textPages->size() > 1) {
					if (--page < 0)
						page = (int)textPages->size() - 1;
					text = editingTextAtPage(*textPages, caseSensitive && *caseSensitive, page, tmp);
				}

				// Find again from the end.
			_backwardAgain:
				pos = (*max)[page];
				mat = editingTextFindBackward(text->c_str(), pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
				if (mat) { // Found.
					const Marker::Coordinates nbegin(category, page, lnoff, coloff);
					const Marker::Coordinates nend(category, page, lnoff, coloff + (int)widepat.length());

					result = editingTextFill(*cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
				} else { // Not found.
					if (--page < 0)
						page = (int)textPages->size() - 1;
					if (page != cursor->begin.page) {
						text = editingTextAtPage(*textPages, caseSensitive && *caseSensitive, page, tmp);

						goto _backwardAgain;
					}
				}
			}
		} else /* if (step == 0) */ { // Error.
			// Do nothing.
		}

		return result;
	};

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	auto findView = [&] (int &step) -> void {
		if (!visible)
			return;

		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

		const float x = ImGui::GetCursorPosX();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->dialogPrompt_Find());

		ImGui::PushID("@Fnd");
		do {
			ImGui::SameLine();
			if (!*initialized) {
				ImGui::SetKeyboardFocusHere();
				*initialized = true;
			}
			ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 5);
			char buf[1024]; // Fixed size.
			const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, what->length());
			if (n > 0)
				memcpy(buf, what->c_str(), n);
			buf[n] = '\0';
			const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AllowTabInput;
			const bool edited = ImGui::InputText(
				"", buf, sizeof(buf),
				flags,
				[] (ImGuiInputTextCallbackData* data) -> int {
					std::string* what = (std::string*)data->UserData;
					what->assign(data->Buf, data->BufTextLen);

					return 0;
				},
				what
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			if (!edited)
				break;

			step = 1;

			*what = buf;
		} while (false);
		ImGui::PopID();

		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconCaseSensitive()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor), caseSensitive ? *caseSensitive : false)) {
			if (caseSensitive)
				*caseSensitive = !*caseSensitive;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->tooltip_CaseSensitive());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconWholeWord()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor), wholeWord ? *wholeWord : false)) {
			if (wholeWord)
				*wholeWord = !*wholeWord;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->tooltip_MatchWholeWords());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconGlobal()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor), globalSearch ? *globalSearch : false)) {
			if (globalSearch)
				*globalSearch = !*globalSearch;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->tooltip_GlobalSearch());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconLeft()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor))) {
			if (!what->empty())
				step = -1;
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconRight()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme->style()->iconColor))) {
			if (!what->empty())
				step = 1;
		}
	};

	const bool result = findOne(
		rnd, ws,
		cursor,
		focused,
		textPages, what,
		max,
		direction,
		caseSensitive, wholeWord, globalSearch,
		findView,
		getWord
	);

	return result;
}

void search(
	Renderer*, Workspace*,
	SearchResult::Array &found,
	const TextPage::Array &textPages, const std::string &what,
	bool caseSensitive, bool wholeWord, bool globalSearch,
	int index,
	TextWordGetter getWord, TextLineGetter getLine
) {
	auto searchForOne = [&] (AssetsBundle::Categories category, int page, const std::string &pat, const std::wstring &widepat, int &maxHeadLen) -> void {
		Marker cursor(Marker::Coordinates(category, page, 0, 0), Marker::Coordinates(category, page, 0, 0));
		std::string tmp;
		const std::string* text = editingTextAtPage(textPages, caseSensitive, page, tmp);
		for (; ; ) {
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindForward(text->c_str(), pat.c_str(), cursor.max().line, cursor.max().column, &lnoff, &coloff);
			if (!mat)
				break;

			Marker::Coordinates nbegin(category, page, cursor.begin.line + lnoff, coloff);
			Marker::Coordinates nend(category, page, cursor.begin.line + lnoff, coloff + (int)widepat.length());
			if (lnoff == 0) {
				nbegin.column += cursor.max().column;
				nend.column += cursor.max().column;
			}

			const bool ret = editingTextFill(cursor, nbegin, nend, wholeWord ? getWord : nullptr);
			if (ret) {
				const std::string head = Text::format("#{0}:{1}", { Text::toString(cursor.begin.page), Text::toString(cursor.begin.line + 1) });
				if ((int)head.length() > maxHeadLen)
					maxHeadLen = (int)head.length();
				const std::string body = getLine(cursor.begin);
				const SearchResult item(cursor.begin, cursor.end, head, body);
				found.push_back(item);
			} else {
				cursor.begin = nbegin;
				cursor.end = nend;
			}
		}
	};

	found.clear();

	AssetsBundle::Categories category = AssetsBundle::Categories::CODE;
	std::string pat = what;
	if (!caseSensitive) {
		Text::toLowerCase(pat);
	}
	const std::wstring widepat = Unicode::toWide(what.c_str());
	int maxHeadLen = 0;
	if (globalSearch) {
		for (int page = 0; page < (int)textPages.size(); ++page) {
			searchForOne(category, page, pat, widepat, maxHeadLen);
		}
	} else {
		searchForOne(category, index, pat, widepat, maxHeadLen);
	}

	for (SearchResult &sr : found) {
		while ((int)sr.head.length() < maxHeadLen)
			sr.head.push_back(' ');
	}
}

float measure(
	Renderer*,
	Workspace* ws,
	float* xOffset_,
	float* unitSize,
	float width
) {
	Theme* theme = ws->theme();

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE - 1;
	float xOffset = 0;
	float size = width / (X_COUNT + 1);
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * (X_COUNT + 1)) * 0.5f;
	}

	if (xOffset_)
		*xOffset_ = xOffset;
	if (unitSize)
		*unitSize = size;

	return size * (X_COUNT + 1);
}

bool indexable(
	Renderer* rnd, Workspace* ws,
	Colour colors[GBBASIC_PALETTE_PER_GROUP_COUNT],
	int* cursor,
	float width,
	bool allowShortcuts,
	const char* tooltip,
	RefDataChangedHandler refDataChanged, RefButtonClickedHandler refButtonClicked
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE - 1;
	float xOffset = 0;
	float size = width / (X_COUNT + 1);
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * (X_COUNT + 1)) * 0.5f;
	}

	const char* const TOOLTIPS[] = {
		theme->tooltipPalette_Index0().c_str(),
		theme->tooltipPalette_Index1().c_str(),
		theme->tooltipPalette_Index2().c_str(),
		theme->tooltipPalette_Index3().c_str()
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_1),
		Shortcut(SDL_SCANCODE_2),
		Shortcut(SDL_SCANCODE_3),
		Shortcut(SDL_SCANCODE_4)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
	VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

	do {
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconRef()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, tooltip)) {
			if (refButtonClicked)
				refButtonClicked();
		}
		ImGui::SameLine();
	} while (false);
	for (int i = 0; i < GBBASIC_PALETTE_PER_GROUP_COUNT; ++i) {
		const Colour &color = colors[i];
		const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

		ImGui::PushID(i);

		const ImVec2 pos = ImGui::GetCursorScreenPos();

		if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size), TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = i;
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Pkr");
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

		if (ImGui::BeginPopup("@Pkr")) {
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			const Colour col = colors[i];
			float col4f[] = {
				Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
			};

			if (ImGui::ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
				const Colour value(
					(Byte)(col4f[0] * 255),
					(Byte)(col4f[1] * 255),
					(Byte)(col4f[2] * 255),
					(Byte)(col4f[3] * 255)
				);
				colors[i] = value;

				if (refDataChanged != nullptr)
					refDataChanged(i);
			}

			ImGui::EndPopup();
		}

		if (cursor && *cursor == i)
			ImGui::Indicator(pos, pos + ImVec2(size, size));

		if (i != EDITING_ITEM_COUNT_PER_LINE - 2)
			ImGui::SameLine();

		ImGui::PopID();
	}

	return result;
}

bool colorable(
	Renderer*,
	Workspace* ws,
	Colour colors[EDITING_ITEM_COUNT_PER_LINE * 2],
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	constexpr const char* const TOOLTIPS[] = {
		"(1)",
		"(2)",
		"(3)",
		"(4)",
		"(5)",
		"(6)",
		"(7)",
		"(8)",
		"(9)",
		"(0)"
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_1),
		Shortcut(SDL_SCANCODE_2),
		Shortcut(SDL_SCANCODE_3),
		Shortcut(SDL_SCANCODE_4),
		Shortcut(SDL_SCANCODE_5),
		Shortcut(SDL_SCANCODE_6),
		Shortcut(SDL_SCANCODE_7),
		Shortcut(SDL_SCANCODE_8),
		Shortcut(SDL_SCANCODE_9),
		Shortcut(SDL_SCANCODE_0)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
	VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

	for (int i = 0; i < EDITING_ITEM_COUNT_PER_LINE * 2; ++i) {
		if (xOffset > 0 && (i % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		const Colour &color = colors[i];
		const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

		ImGui::PushID(i);

		const ImVec2 pos = ImGui::GetCursorScreenPos();

		if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size), TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = i;
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Pkr");
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

		if (ImGui::BeginPopup("@Pkr")) {
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			const Colour col = colors[i];
			float col4f[] = {
				Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
			};

			if (ImGui::ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
				const Colour value(
					(Byte)(col4f[0] * 255),
					(Byte)(col4f[1] * 255),
					(Byte)(col4f[2] * 255),
					(Byte)(col4f[3] * 255)
				);
				colors[i] = value;
			}

			ImGui::EndPopup();
		}

		if (cursor && *cursor == i)
			ImGui::Indicator(pos, pos + ImVec2(size, size));

		if (((i + 1) % X_COUNT) != 0 && i != EDITING_ITEM_COUNT_PER_LINE * 2 - 1)
			ImGui::SameLine();

		ImGui::PopID();
	}

	return result;
}

bool paintable(
	Renderer* rnd, Workspace* ws,
	PaintableTools* cursor,
	float width,
	bool allowShortcuts, bool withTilewise,
	unsigned mask
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconHand()->pointer(rnd),
		theme->iconEyedropper()->pointer(rnd),
		theme->iconPencil()->pointer(rnd),
		theme->iconPaintbucket()->pointer(rnd),
		theme->iconLasso()->pointer(rnd),
		theme->iconLine()->pointer(rnd),
		theme->iconBox()->pointer(rnd),
		theme->iconBoxFill()->pointer(rnd),
		theme->iconEllipse()->pointer(rnd),
		theme->iconEllipseFill()->pointer(rnd),
		theme->iconStamp()->pointer(rnd)
	};
	const char* const TOOLTIPS[] = {
		theme->tooltipEdit_ToolHand().c_str(),
		theme->tooltipEdit_ToolEyedropper().c_str(),
		theme->tooltipEdit_ToolPencil().c_str(),
		theme->tooltipEdit_ToolPaintbucket().c_str(),
		withTilewise ? theme->tooltipEdit_ToolLassoWithTilewise().c_str() : theme->tooltipEdit_ToolLasso().c_str(),
		theme->tooltipEdit_ToolLine().c_str(),
		theme->tooltipEdit_ToolBox().c_str(),
		theme->tooltipEdit_ToolBoxFill().c_str(),
		theme->tooltipEdit_ToolEllipse().c_str(),
		theme->tooltipEdit_ToolEllipseFill().c_str(),
		theme->tooltipEdit_ToolStamp().c_str()
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_H),
		Shortcut(SDL_SCANCODE_I),
		Shortcut(SDL_SCANCODE_B),
		Shortcut(SDL_SCANCODE_G),
		Shortcut(SDL_SCANCODE_M),
		Shortcut(SDL_SCANCODE_L),
		Shortcut(SDL_SCANCODE_X),
		Shortcut(SDL_SCANCODE_X, false, true),
		Shortcut(SDL_SCANCODE_E),
		Shortcut(SDL_SCANCODE_E, false, true),
		Shortcut(SDL_SCANCODE_S)
	};
	const ImU32 colors[] = {
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		theme->style()->iconColor,
		theme->style()->iconColor,
		theme->style()->iconColor,
		theme->style()->iconColor,
		theme->style()->iconColor,
		IM_COL32_WHITE
	};
	constexpr const PaintableTools VALUES[] = {
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
		STAMP
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	for (int i = 0, j = 0; i < GBBASIC_COUNTOF(icons); ++i) {
		if ((mask & (1 << i)) == 0)
			continue;

		if (xOffset > 0 && (j % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		const bool selected = cursor && *cursor == VALUES[i];
		if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), selected, TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = VALUES[i];
		}

		if (((j + 1) % X_COUNT) != 0 && i != GBBASIC_COUNTOF(icons) - 1)
			ImGui::SameLine();

		++j;
	}

	return result;
}

bool actor(
	Renderer* rnd, Workspace* ws,
	PaintableTools* cursor,
	float width,
	bool allowShortcuts,
	unsigned mask
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconHand()->pointer(rnd),
		theme->iconSmudge()->pointer(rnd),
		theme->iconEraser()->pointer(rnd),
		theme->iconMove()->pointer(rnd),
		theme->iconRef()->pointer(rnd)
	};
	const char* const TOOLTIPS[] = {
		theme->tooltipEdit_ToolHand().c_str(),
		theme->tooltipEdit_ToolSmudge().c_str(),
		theme->tooltipEdit_ToolEraser().c_str(),
		theme->tooltipEdit_ToolMove().c_str(),
		theme->tooltipEdit_ToolJumpToRef().c_str()
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_H),
		Shortcut(SDL_SCANCODE_U),
		Shortcut(SDL_SCANCODE_D),
		Shortcut(SDL_SCANCODE_P),
		Shortcut(SDL_SCANCODE_R)
	};
	const ImU32 colors[] = {
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE
	};
	constexpr const PaintableTools VALUES[] = {
		HAND,
		SMUDGE,
		ERASER,
		MOVE,
		REF
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	for (int i = 0, j = 0; i < GBBASIC_COUNTOF(icons); ++i) {
		if ((mask & (1 << i)) == 0)
			continue;

		if (xOffset > 0 && (j % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		const bool selected = cursor && *cursor == VALUES[i];
		if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), selected, TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = VALUES[i];
		}

		if (((j + 1) % X_COUNT) != 0 && i != GBBASIC_COUNTOF(icons) - 1)
			ImGui::SameLine();

		++j;
	}

	return result;
}

bool trigger(
	Renderer* rnd, Workspace* ws,
	PaintableTools* cursor,
	float width,
	bool allowShortcuts,
	unsigned mask
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconHand()->pointer(rnd),
		theme->iconSmudge()->pointer(rnd),
		theme->iconEraser()->pointer(rnd),
		theme->iconMove()->pointer(rnd),
		theme->iconResize()->pointer(rnd)
	};
	const char* const TOOLTIPS[] = {
		theme->tooltipEdit_ToolHand().c_str(),
		theme->tooltipEdit_ToolSmudge().c_str(),
		theme->tooltipEdit_ToolEraser().c_str(),
		theme->tooltipEdit_ToolMove().c_str(),
		theme->tooltipEdit_ToolResize().c_str()
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_H),
		Shortcut(SDL_SCANCODE_U),
		Shortcut(SDL_SCANCODE_D),
		Shortcut(SDL_SCANCODE_P),
		Shortcut(SDL_SCANCODE_Z)
	};
	const ImU32 colors[] = {
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE
	};
	constexpr const PaintableTools VALUES[] = {
		HAND,
		SMUDGE,
		ERASER,
		MOVE,
		RESIZE
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	for (int i = 0, j = 0; i < GBBASIC_COUNTOF(icons); ++i) {
		if ((mask & (1 << i)) == 0)
			continue;

		if (xOffset > 0 && (j % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		const bool selected = cursor && *cursor == VALUES[i];
		if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), selected, TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = VALUES[i];
		}

		if (((j + 1) % X_COUNT) != 0 && i != GBBASIC_COUNTOF(icons) - 1)
			ImGui::SameLine();

		++j;
	}

	return result;
}

bool byte(
	Renderer* rnd, Workspace* ws,
	UInt8 oldVal,
	UInt8* newVal,
	float width,
	bool* focused,
	bool disabled,
	const char* prompt,
	ByteTooltipGetter getTooltip
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;

	ImGui::PushID("@Byte");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);
	if (disabled) {
		ImGui::BeginDisabled();

		int val = *newVal;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		ImGui::DragInt("", &val, 1.0f, 0, 255, "%02X", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
		ImGui::SameLine();
		ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE));

		ImGui::EndDisabled();
	} else {
		bool hovered = false;

		int val = *newVal;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newVal = (UInt8)Math::max(val - 1, 0);
			if (oldVal != *newVal)
				result = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &val, 1.0f, 0, 255, "%02X", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput)) {
			*newVal = (UInt8)val;
			if (oldVal != *newVal)
				result = true;
		}
		hovered |= ImGui::IsItemHovered();
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newVal = (UInt8)Math::min(val + 1, 255);
			if (oldVal != *newVal)
				result = true;
		}

		const char* tooltip = nullptr;
		if (getTooltip)
			tooltip = getTooltip(*newVal);
		if (hovered && tooltip) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(tooltip);
		}
	}

	ImGui::PopID();

	return result;
}

bool bitwise(
	Renderer* rnd, Workspace* ws,
	BitwiseOperations* cursor,
	float width,
	bool disabled,
	const char* tooltip, const char* tooltipOps[4]
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconBitwiseSet()->pointer(rnd),
		theme->iconBitwiseAnd()->pointer(rnd),
		theme->iconBitwiseOr()->pointer(rnd),
		theme->iconBitwiseXor()->pointer(rnd)
	};
	const ImU32 colors[] = {
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE
	};
	constexpr const BitwiseOperations VALUES[] = {
		SET,
		AND,
		OR,
		XOR
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

	if (ImGui::ImageButton(theme->iconBitwise()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, tooltip) && focusingWnd && !disabled) {
		result = true;

		if (cursor) {
			unsigned cursor_ = (unsigned)(*cursor);
			if (++cursor_ >= GBBASIC_COUNTOF(VALUES))
				cursor_ = 0;
			*cursor = (BitwiseOperations)cursor_;
		}
	}
	ImGui::SameLine();

	ImGui::PopStyleColor(3);

	if (disabled) {
		ImGui::BeginDisabled();
		for (int i = 0, j = 0; i < GBBASIC_COUNTOF(icons); ++i) {
			const bool selected = cursor && *cursor == VALUES[i];
			ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), selected, tooltipOps[i]);

			ImGui::SameLine();

			++j;
		}
		ImGui::EndDisabled();
	} else {
		for (int i = 0, j = 0; i < GBBASIC_COUNTOF(icons); ++i) {
			const bool selected = cursor && *cursor == VALUES[i];
			if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), selected, tooltipOps[i])) {
				result = true;

				if (cursor)
					*cursor = VALUES[i];
			}

			ImGui::SameLine();

			++j;
		}
	}

	return result;
}

bool magnifiable(
	Renderer* rnd, Workspace* ws,
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconMagnify()->width(); // Borrowed.
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const char* const TOOLTIP = theme->tooltipEdit_Magnifiable().c_str();
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_MINUS),
		Shortcut(SDL_SCANCODE_EQUALS)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

	if (ImGui::ImageButton(theme->iconMagnify()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIP) && focusingWnd) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}
	constexpr const int MIN_ZOOM = 0;
	constexpr const int MAX_ZOOM = 3;
#if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
	const bool modifier = io.KeyCtrl;
#elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
	const bool modifier = io.KeySuper;
#endif /* GBBASIC_MODIFIER_KEY */
	const bool zoomOut = shortcuts[0].pressed() || (*cursor > MIN_ZOOM && modifier && io.MouseWheel < 0);
	const bool zoomIn = shortcuts[1].pressed() || (*cursor < MAX_ZOOM && modifier && io.MouseWheel > 0);
	if (allowShortcuts && focusingWnd && zoomOut) {
		result = true;

		if (cursor) {
			if (--*cursor < MIN_ZOOM)
				*cursor = MAX_ZOOM;
		}
	} else if (allowShortcuts && focusingWnd && zoomIn) {
		result = true;

		if (cursor) {
			if (++*cursor >= MAX_ZOOM + 1)
				*cursor = MIN_ZOOM;
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	drawList->PathLineTo(pos + ImVec2(1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, size - 1));
	drawList->PathLineTo(pos + ImVec2(1, size - 1));
	drawList->PathStroke(col, true);

	if (cursor && *cursor >= 0 && *cursor <= 3) {
		drawList->PathLineTo(pos + ImVec2(size * *cursor, 1));
		drawList->PathLineTo(pos + ImVec2(size * (*cursor + 1), 1));
		drawList->PathLineTo(pos + ImVec2(size * (*cursor + 1), size - 1));
		drawList->PathLineTo(pos + ImVec2(size * *cursor, size - 1));
		drawList->PathFillConvex(col);

		Texture* texs[] = {
			theme->iconNumber1(),
			theme->iconNumber2(),
			theme->iconNumber3(),
			theme->iconNumber4()
		};
		Texture* tex = texs[*cursor];
		drawList->AddImage(
			tex->pointer(rnd),
			ImVec2(pos + ImVec2(size * *cursor, 1)), ImVec2(pos + ImVec2(size * (*cursor + 1), size - 1)),
			ImVec2(0, 0), ImVec2(1, 1),
			ImGui::GetColorU32(ImGuiCol_FrameBg)
		);

		const ImRect rects[] = {
			ImRect(
				ImVec2(pos + ImVec2(0, 1)),
				ImVec2(pos + ImVec2(size, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size, 1)),
				ImVec2(pos + ImVec2(size * 2, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 2, 1)),
				ImVec2(pos + ImVec2(size * 3, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 3, 1)),
				ImVec2(pos + ImVec2(size * 4, size - 1))
			)
		};
		for (int i = 0; i < GBBASIC_COUNTOF(rects); ++i) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(rects[i].Min, rects[i].Max, false) && focusingWnd) {
				result = true;

				*cursor = i;

				break;
			}
		}
	}

	ImGui::NewLine();

	return result;
}

bool weighable(
	Renderer* rnd, Workspace* ws,
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconPencils()->width(); // Borrowed.
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	constexpr const char* const TOOLTIP = "([/])";
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_LEFTBRACKET),
		Shortcut(SDL_SCANCODE_RIGHTBRACKET)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

	if (ImGui::ImageButton(theme->iconPencils()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIP) && focusingWnd) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}
	if (allowShortcuts && focusingWnd && shortcuts[0].pressed()) {
		result = true;

		if (cursor) {
			if (--*cursor < 0)
				*cursor = 3;
		}
	} else if (allowShortcuts && focusingWnd && shortcuts[1].pressed()) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float xOffsets[] = { 1, size, size * 2, size * 3, size * 4 };
	const float yOffsets[] = { size * (3.0f / 4.0f), size * (2.0f / 4.0f), size * (1.0f / 4.0f), 1, 1 };
	{
		drawList->PathLineTo(pos + ImVec2(xOffsets[0], yOffsets[0]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[1], yOffsets[0]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[1], yOffsets[1]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[2], yOffsets[1]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[2], yOffsets[2]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[3], yOffsets[2]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[3], yOffsets[3]));
	}
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, size - 1));
	drawList->PathLineTo(pos + ImVec2(1, size - 1));
	drawList->PathStroke(col, true);

	if (cursor && *cursor >= 0 && *cursor <= 3) {
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor] - 1, yOffsets[*cursor]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor + 1], yOffsets[*cursor]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor + 1], size - 1));
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor] - 1, size - 1));
		drawList->PathFillConvex(col);

		const ImRect rects[] = {
			ImRect(
				ImVec2(pos + ImVec2(0, 1)),
				ImVec2(pos + ImVec2(size, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size, 1)),
				ImVec2(pos + ImVec2(size * 2, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 2, 1)),
				ImVec2(pos + ImVec2(size * 3, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 3, 1)),
				ImVec2(pos + ImVec2(size * 4, size - 1))
			)
		};
		for (int i = 0; i < GBBASIC_COUNTOF(rects); ++i) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(rects[i].Min, rects[i].Max, false) && focusingWnd) {
				result = true;

				*cursor = i;

				break;
			}
		}
	}

	ImGui::NewLine();

	return result;
}

bool flippable(
	Renderer* rnd, Workspace* ws,
	RotationsAndFlippings* cursor,
	float width,
	bool allowShortcuts,
	unsigned mask
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (cursor)
		*cursor = INVALID;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconRotateClockwise()->width(); // Borrowed.
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconRotateClockwise()->pointer(rnd),
		theme->iconRotateAnticlockwise()->pointer(rnd),
		theme->iconRotateHalfTurn()->pointer(rnd),
		theme->iconFlipVertically()->pointer(rnd),
		theme->iconFlipHorizontally()->pointer(rnd)
	};
	constexpr const char* const TOOLTIPS[] = {
		"(,)",
		"(.)",
		"(/)",
		"(;)",
		"(')"
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_COMMA),
		Shortcut(SDL_SCANCODE_PERIOD),
		Shortcut(SDL_SCANCODE_SLASH),
		Shortcut(SDL_SCANCODE_SEMICOLON),
		Shortcut(SDL_SCANCODE_APOSTROPHE)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	for (int i = 0; i < GBBASIC_COUNTOF(icons); ++i) {
		if ((mask & (1 << i)) == 0) {
			const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

			ImGui::ImageButton(icons[i], ImVec2(size, size), ImColor(IM_COL32(128, 128, 128, 255)), false, TOOLTIPS[i]);

			ImGui::PopStyleColor(3);
		} else {
			if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
				result = true;

				if (cursor)
					*cursor = (RotationsAndFlippings)i;
			}
		}

		if (i != GBBASIC_COUNTOF(icons) - 1)
			ImGui::SameLine();
	}

	return result;
}

bool maskable(
	Renderer* rnd, Workspace* ws,
	Byte* data, bool* set, int* bit,
	Byte editable,
	float width,
	bool disabled,
	const char* prompt,
	const char* tooltipLnHigh, const char* tooltipLnLow,
	const char* tooltipBits[8]
) {
	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	if (disabled) {
		ImGui::BeginDisabled();
		ImGui::ByteMatrice(rnd, theme, data, set, bit, editable, size * X_COUNT, tooltipLnHigh, tooltipLnLow, tooltipBits);
		ImGui::EndDisabled();
	} else {
		result = ImGui::ByteMatrice(rnd, theme, data, set, bit, editable, size * X_COUNT, tooltipLnHigh, tooltipLnLow, tooltipBits);
	}

	return result;
}

bool maskable(
	Renderer* rnd, Workspace* ws,
	Byte* data,
	Byte editable,
	float width,
	bool disabled,
	const char* prompt,
	const char* tooltipLnHigh, const char* tooltipLnLow,
	const char* tooltipBits[8]
) {
	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	if (disabled) {
		ImGui::BeginDisabled();
		ImGui::ByteMatrice(rnd, theme, data, editable, size * X_COUNT, tooltipLnHigh, tooltipLnLow, tooltipBits);
		ImGui::EndDisabled();
	} else {
		result = ImGui::ByteMatrice(rnd, theme, data, editable, size * X_COUNT, tooltipLnHigh, tooltipLnLow, tooltipBits);
	}

	return result;
}

bool clickable(
	Renderer*,
	Workspace* ws,
	float width,
	const char* prompt
) {
	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImGui::GetStyleColorVec4(ImGuiCol_Text));
	if (ImGui::Button(prompt, ImVec2(iconSize * 5, 0))) {
		result = true;
	}
	ImGui::PopStyleColor(2);

	return result;
}

bool togglable(
	Renderer*,
	Workspace* ws,
	bool* value,
	float width,
	const char* prompt,
	const char* tooltip
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImGui::GetStyleColorVec4(ImGuiCol_Text));
	if (ImGui::Checkbox(prompt, value)) {
		result = true;
	}
	ImGui::PopStyleColor(2);

	if (ImGui::IsItemHovered() && tooltip) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(tooltip);
	}

	return result;
}

bool limited(
	Renderer* rnd, Workspace* ws,
	int oldVal[4],
	int* newVal,
	const int minVal[4], const int maxVal[4],
	float width,
	bool* focused,
	const char* prompt,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Thr");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	for (int i = 0; i < 4; ++i) {
		int thrVal = newVal[i];
		ImGui::PushID(i);
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal[i] = Math::max(thrVal - 1, minVal[i]);
				changed = true;
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &thrVal, 1.0f, minVal[i], maxVal[i], "%d")) {
				newVal[i] = thrVal;
				changed = true;
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal[i] = Math::min(thrVal + 1, maxVal[i]);
				changed = true;
			}
		}
		ImGui::PopID();
	}

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (newVal[0] == oldVal[0] && newVal[1] == oldVal[1] && newVal[2] == oldVal[2] && newVal[3] == oldVal[3]) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			for (int i = 0; i < 4; ++i)
				newVal[i] = oldVal[i];
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool resizable(
	Renderer* rnd, Workspace* ws,
	int oldWidth, int oldHeight,
	int* newWidth, int* newHeight,
	int maxWidth, int maxHeight, int maxTotal,
	float width,
	bool* focused,
	const char* prompt,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Sz");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	int xVal = *newWidth;
	ImGui::PushID("@X");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newWidth = Math::max(xVal - 1, 1);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &xVal, 1.0f, 1, maxWidth, "W: %d")) {
			*newWidth = xVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newWidth = Math::min(xVal + 1, maxWidth);
			changed = true;
		}
	}
	ImGui::PopID();

	int yVal = *newHeight;
	ImGui::PushID("@Y");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newHeight = Math::max(yVal - 1, 1);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &yVal, 1.0f, 1, maxHeight, "H: %d")) {
			*newHeight = yVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newHeight = Math::min(yVal + 1, maxHeight);
			changed = true;
		}
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	const bool bad = maxTotal > 0 && (*newWidth) * (*newHeight) > maxTotal;
	if (*newWidth == oldWidth && *newHeight == oldHeight) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else if (bad) {
		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, true);

		{
			VariableGuard<decltype(style.Alpha)> guardAlpha(&style.Alpha, style.Alpha, style.DisabledAlpha);

			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();
		}
		if (ImGui::IsItemHovered() && txtOutOfBounds) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(txtOutOfBounds);
		}

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newWidth = oldWidth;
			*newHeight = oldHeight;

			if (warn && txtOutOfBounds)
				warn(ws, txtOutOfBounds, false);
		}
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newWidth = oldWidth;
			*newHeight = oldHeight;
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool resizable(
	Renderer* rnd, Workspace* ws,
	int oldWidth, int oldHeight,
	int* newWidth, int* newHeight,
	int minWidth, int maxWidth, int minHeight, int maxHeight,
	float width,
	bool* focused,
	const char* prompt,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Sz");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	int xVal = *newWidth;
	ImGui::PushID("@X");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newWidth = Math::max(xVal - 1, minWidth);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &xVal, 1.0f, minWidth, maxWidth, "W: %d")) {
			*newWidth = xVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newWidth = Math::min(xVal + 1, maxWidth);
			changed = true;
		}
	}
	ImGui::PopID();

	int yVal = *newHeight;
	ImGui::PushID("@Y");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newHeight = Math::max(yVal - 1, minHeight);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &yVal, 1.0f, minHeight, maxHeight, "H: %d")) {
			*newHeight = yVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newHeight = Math::min(yVal + 1, maxHeight);
			changed = true;
		}
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (*newWidth == oldWidth && *newHeight == oldHeight) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newWidth = oldWidth;
			*newHeight = oldHeight;
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool resizable(
	Renderer* rnd, Workspace* ws,
	int oldSize,
	int* newSize,
	int minSize, int maxSize,
	float width,
	bool* focused,
	const char* prompt,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Sz");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	int szVal = *newSize;
	ImGui::PushID("@Sz");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newSize = Math::max(szVal - 1, minSize);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &szVal, 1.0f, minSize, maxSize, "%d")) {
			*newSize = szVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newSize = Math::min(szVal + 1, maxSize);
			changed = true;
		}
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (*newSize == oldSize) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newSize = oldSize;
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool anchorable(
	Renderer* rnd, Workspace* ws,
	int oldX, int oldY,
	int* newX, int* newY,
	int minX, int minY, int maxX, int maxY,
	float width,
	bool* focused,
	const char* prompt,
	const char* tooltipNoLimit, const char* tooltipApplyAll,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@An");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	auto showTooltipForNoLimit = [&style] (const char* tooltipNoLimit) -> void {
		if (ImGui::IsItemHovered() && tooltipNoLimit) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(tooltipNoLimit);
		}
	};

	bool changed = false;
	int xVal = *newX;
	ImGui::PushID("@X");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newX = Math::max(xVal - 1, minX);
			changed = true;
		}
		showTooltipForNoLimit(tooltipNoLimit);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &xVal, 1.0f, minX, maxX, "X: %d")) {
			*newX = xVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		showTooltipForNoLimit(tooltipNoLimit);
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newX = Math::min(xVal + 1, maxX);
			changed = true;
		}
		showTooltipForNoLimit(tooltipNoLimit);
	}
	ImGui::PopID();

	int yVal = *newY;
	ImGui::PushID("@Y");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newY = Math::max(yVal - 1, minY);
			changed = true;
		}
		showTooltipForNoLimit(tooltipNoLimit);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &yVal, 1.0f, minY, maxY, "Y: %d")) {
			*newY = yVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		showTooltipForNoLimit(tooltipNoLimit);
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newY = Math::min(yVal + 1, maxY);
			changed = true;
		}
		showTooltipForNoLimit(tooltipNoLimit);
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (*newX == oldX && *newY == oldY) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
		}
		editingEndAppliable();
		if (ImGui::IsItemHovered() && tooltipApplyAll) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(tooltipApplyAll);
		}
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newX = oldX;
			*newY = oldY;
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool pitchable(
	Renderer* rnd, Workspace* ws,
	int width_, int height_, bool sizeChanged,
	int oldPitch,
	int* newPitch,
	int maxPitch,
	float width,
	bool* focused,
	const char* prompt,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Ptch");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = sizeChanged;
	int pitch_ = *newPitch;
	ImGui::PushID("@P");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newPitch = Math::max(pitch_ - 1, 1);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &pitch_, 1.0f, 1, maxPitch, "%d")) {
			*newPitch = pitch_;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newPitch = Math::min(pitch_ + 1, maxPitch);
			changed = true;
		}
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	const bool bad = (*newPitch) > width_ * height_;
	if (*newPitch == oldPitch) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();
	} else if (bad) {
		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, true);

		{
			VariableGuard<decltype(style.Alpha)> guardAlpha(&style.Alpha, style.Alpha, style.DisabledAlpha);

			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();
		}
		if (ImGui::IsItemHovered() && txtOutOfBounds) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(txtOutOfBounds);
		}

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newPitch = oldPitch;

			if (warn && txtOutOfBounds)
				warn(ws, txtOutOfBounds, false);
		}
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newPitch = oldPitch;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool offsetable(
	Renderer* rnd, Workspace* ws,
	int oldOffset, int* newOffset, int minOffset, int maxOffset, const int* maxOffsetLimit,
	float oldStrength, float* newStrength, float minStrength, float maxStrength,
	float width,
	bool* focused,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Sd");

	bool changed = false;
	int offVal = *newOffset;
	ImGui::PushID("@Off");
	{
		bool showTooltip = false;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newOffset = Math::max(offVal - 1, minOffset);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &offVal, 1.0f, minOffset, maxOffset, "%dpx")) {
			*newOffset = offVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newOffset = Math::min(offVal + 1, maxOffset);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		if (showTooltip) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->dialogPrompt_Offset());
		}
	}
	ImGui::PopID();

	float strVal = *newStrength;
	ImGui::PushID("@Str");
	{
		bool showTooltip = false;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newStrength = Math::max(strVal - 0.1f, minStrength);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		int intVal = (int)(strVal * 100);
		if (ImGui::DragInt("", &intVal, 5, (int)(minStrength * 100), (int)(maxStrength * 100), "%d%%")) {
			strVal = intVal / 100.0f;
			*newStrength = strVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newStrength = Math::min(strVal + 0.1f, maxStrength);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		if (showTooltip) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->dialogPrompt_Strength());
		}
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	const bool bad = maxOffsetLimit && *newOffset > *maxOffsetLimit;
	if (*newOffset == oldOffset && *newStrength == oldStrength) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else if (bad) {
		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, true);

		{
			VariableGuard<decltype(style.Alpha)> guardAlpha(&style.Alpha, style.Alpha, style.DisabledAlpha);

			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();
		}
		if (ImGui::IsItemHovered() && txtOutOfBounds) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(txtOutOfBounds);
		}

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newOffset = oldOffset;
			*newStrength = oldStrength;

			if (warn && txtOutOfBounds)
				warn(ws, txtOutOfBounds, false);
		}
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newOffset = oldOffset;
			*newStrength = oldStrength;
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool offsetable(
	Renderer* rnd, Workspace* ws,
	int oldOffset, int* newOffset, int minOffset, int maxOffset, const int* maxOffsetLimit,
	float oldStrength, float* newStrength, float minStrength, float maxStrength,
	UInt8 oldDirection, UInt8* newDirection,
	float width,
	bool* focused,
	const char* txtOutOfBounds,
	WarningHandler warn
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Sd");

	bool changed = false;
	int offVal = *newOffset;
	ImGui::PushID("@Off");
	{
		bool showTooltip = false;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newOffset = Math::max(offVal - 1, minOffset);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &offVal, 1.0f, minOffset, maxOffset, "%dpx")) {
			*newOffset = offVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newOffset = Math::min(offVal + 1, maxOffset);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		if (showTooltip) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->dialogPrompt_Offset());
		}
	}
	ImGui::PopID();

	float strVal = *newStrength;
	ImGui::PushID("@Str");
	{
		bool showTooltip = false;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newStrength = Math::max(strVal - 0.1f, minStrength);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		int intVal = (int)(strVal * 100);
		if (ImGui::DragInt("", &intVal, 5, (int)(minStrength * 100), (int)(maxStrength * 100), "%d%%")) {
			strVal = intVal / 100.0f;
			*newStrength = strVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			*newStrength = Math::min(strVal + 0.1f, maxStrength);
			changed = true;
		}
		showTooltip |= ImGui::IsItemHovered();
		if (showTooltip) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->dialogPrompt_Strength());
		}
	}
	ImGui::PopID();

	int dirVal = (int)*newDirection;
	ImGui::PushID("@Dir");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();

		const char* items[] = {
			theme->dialogPrompt_Direction_S().c_str(),
			theme->dialogPrompt_Direction_E().c_str(),
			theme->dialogPrompt_Direction_N().c_str(),
			theme->dialogPrompt_Direction_W().c_str(),
			theme->dialogPrompt_Direction_SE().c_str(),
			theme->dialogPrompt_Direction_NE().c_str(),
			theme->dialogPrompt_Direction_NW().c_str(),
			theme->dialogPrompt_Direction_SW().c_str()
		};

		bool showTooltip = false;
		do {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			ImGui::SetNextItemWidth(size * X_COUNT);
			if (ImGui::Combo("", &dirVal, items, GBBASIC_COUNTOF(items))) {
				*newDirection = (UInt8)dirVal;
			}
			showTooltip |= ImGui::IsItemHovered();
		} while (false);
		if (showTooltip) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme->dialogPrompt_Direction());
		}
		ImGui::SameLine();
		ImGui::NewLine(1);
	}
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	const bool bad = maxOffsetLimit && *newOffset > *maxOffsetLimit;
	if (*newOffset == oldOffset && *newStrength == oldStrength && *newDirection == oldDirection) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	} else if (bad) {
		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, true);

		{
			VariableGuard<decltype(style.Alpha)> guardAlpha(&style.Alpha, style.Alpha, style.DisabledAlpha);

			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();
		}
		if (ImGui::IsItemHovered() && txtOutOfBounds) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(txtOutOfBounds);
		}

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newOffset = oldOffset;
			*newStrength = oldStrength;
			*newDirection = oldDirection;

			if (warn && txtOutOfBounds)
				warn(ws, txtOutOfBounds, false);
		}
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			*newOffset = oldOffset;
			*newStrength = oldStrength;
			*newDirection = oldDirection;
			changed = true;
		}

		if (changed && warn && txtOutOfBounds)
			warn(ws, txtOutOfBounds, false);
	}

	ImGui::PopID();

	return result;
}

bool colorable(
	Renderer* rnd, Workspace* ws,
	const Colour &oldValue,
	Colour &newValue,
	bool hasAlpha,
	float width,
	bool* focused,
	const char* prompt
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::PushID("@Col");

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	int colVal = newValue.r;
	ImGui::PushID("@R");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			newValue.r = (Byte)Math::max(colVal - 1, 0);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &colVal, 1.0f, 0, std::numeric_limits<Byte>::max(), "%d")) {
			newValue.r = (Byte)colVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			newValue.r = (Byte)Math::min(colVal + 1, (int)std::numeric_limits<Byte>::max());
			changed = true;
		}
	}
	ImGui::PopID();

	colVal = newValue.g;
	ImGui::PushID("@G");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			newValue.g = (Byte)Math::max(colVal - 1, 0);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &colVal, 1.0f, 0, std::numeric_limits<Byte>::max(), "%d")) {
			newValue.g = (Byte)colVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			newValue.g = (Byte)Math::min(colVal + 1, (int)std::numeric_limits<Byte>::max());
			changed = true;
		}
	}
	ImGui::PopID();

	colVal = newValue.b;
	ImGui::PushID("@B");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			newValue.b = (Byte)Math::max(colVal - 1, 0);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(dragWidth);
		if (ImGui::DragInt("", &colVal, 1.0f, 0, std::numeric_limits<Byte>::max(), "%d")) {
			newValue.b = (Byte)colVal;
			changed = true;
		}
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
			newValue.b = (Byte)Math::min(colVal + 1, (int)std::numeric_limits<Byte>::max());
			changed = true;
		}
	}
	ImGui::PopID();

	if (hasAlpha) {
		colVal = newValue.a;
		ImGui::PushID("@A");
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newValue.a = (Byte)Math::max(colVal - 1, 0);
				changed = true;
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &colVal, 1.0f, 0, std::numeric_limits<Byte>::max(), "%d")) {
				newValue.a = (Byte)colVal;
				changed = true;
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newValue.a = (Byte)Math::min(colVal + 1, (int)std::numeric_limits<Byte>::max());
				changed = true;
			}
		}
		ImGui::PopID();
	}

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (newValue == oldValue) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			newValue = oldValue;
			changed = true;
		}
	}

	ImGui::PopID();

	return result;
}

bool palette(
	Renderer*, Workspace* ws,
	const PaletteAssets::Array &palette,
	Colour &newValue,
	int* editingColorGroup, int* editingColorIndex, bool* openColorPicker,
	float width
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE - 1;
	float xOffset = 0;
	float size = width / (X_COUNT + 1);
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * (X_COUNT + 1)) * 0.5f;
	}

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
	VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

	constexpr const char* const GRP[] = {
		"BG0", "BG1", "BG2", "BG3", "BG4", "BG5", "BG6", "BG7"
	};

	const int n = Math::min((int)GBBASIC_COUNTOF(GRP), (int)palette.size());
	for (int j = 0; j < n; ++j) {
		const PaletteAssets::Entry &entry = palette[j];
		const Indexed::Ptr &palette_ = entry.data;

		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		const float xPos = ImGui::GetCursorPosX();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(GRP[j]);
		ImGui::SameLine();
		ImGui::SetCursorPosX(xPos + iconSize);

		for (int i = 0; i < palette_->count(); ++i) {
			Colour color;
			palette_->get(i, color);
			const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

			ImGui::PushID(i);

			const float size = ImGui::GetTextLineHeightWithSpacing();
			if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview, ImVec2(iconSize, size))) {
				// Do nothing.
			}
			ImGui::SameLine();

			ImGui::PopID();

			if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
				*editingColorGroup = j;
				*editingColorIndex = i;
				*openColorPicker = true;
			}
		}
		ImGui::NewLine();
		ImGui::NewLine(1);
	}
	ImGui::SameLine();

	if (*openColorPicker) {
		*openColorPicker = false;
		ImGui::OpenPopup("@Pkr");
	}
	do {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

		if (ImGui::BeginPopup("@Pkr")) {
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			const PaletteAssets::Entry &entry = palette[*editingColorGroup];
			const Indexed::Ptr &palette_ = entry.data;

			Colour col;
			palette_->get(*editingColorIndex, col);
			float col4f[] = {
				Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
			};

			if (ImGui::ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
				const Colour value(
					(Byte)(col4f[0] * 255),
					(Byte)(col4f[1] * 255),
					(Byte)(col4f[2] * 255),
					(Byte)(col4f[3] * 255)
				);
				newValue = value;
				result = true;
			}

			ImGui::EndPopup();
		} else {
			*editingColorGroup = -1;
			*editingColorIndex = -1;
		}
	} while (false);

	return result;
}

bool namable(
	Renderer*,
	Workspace* ws,
	const std::string &oldName, std::string &newName,
	float width,
	bool* initialized, bool* focused,
	const char* prompt
) {
	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::PushID("@Txt");
	do {
		if (initialized && !*initialized) {
			ImGui::SetKeyboardFocusHere();
			*initialized = true;
		}
		ImGui::SetNextItemWidth(size * X_COUNT);
		char buf[ASSETS_NAME_STRING_MAX_LENGTH]; // Fixed size.
		const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, oldName.length());
		if (n > 0)
			memcpy(buf, oldName.c_str(), n);
		buf[n] = '\0';
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
		const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_AutoSelectAll);
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::PopStyleColor();
		if (!edited)
			break;
		newName = buf;

		changed = true;
	} while (false);
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (newName == oldName) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			newName = oldName;
			changed = true;
		}
	}

	return result;
}

bool actable(
	Renderer*,
	Workspace* ws,
	const Project* prj,
	float width,
	Byte* cursor
) {
	struct Data {
		Theme* theme = nullptr;
		const Project* project = nullptr;

		Data() {
		}
		Data(Theme* t, const Project* p) : theme(t), project(p) {
		}
	};

	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	int cursor_ = *cursor;
	if (cursor_ == Scene::INVALID_ACTOR())
		cursor_ = 0;
	else
		++cursor_;
	ImGui::PushID("@Act");
	{
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		Data data(theme, prj);
		ImGui::SetNextItemWidth(size * X_COUNT);
		const bool changed_ = ImGui::Combo(
			"",
			&cursor_,
			[] (void* data_, int idx, const char** outText) -> bool {
				Data* data = (Data*)data_;
				Theme* theme = data->theme;
				const Project* prj = (const Project*)data->project;

				if (idx == 0) {
					*outText = theme->generic_None().c_str();

					return true;
				}

				const ActorAssets::Entry* entry = prj->getActor(idx - 1);
				const char* entry_ = entry->name.c_str();
				*outText = entry_ ? entry_ : "Unknown";

				return true;
			},
			(void*)&data, (int)(prj->actorPageCount() + 1)
		);
		if (changed_) {
			result = true;

			if (cursor_ == 0)
				cursor_ = Scene::INVALID_ACTOR();
			else
				--cursor_;
			*cursor = (Byte)cursor_;
		}
	}
	ImGui::PopID();

	return result;
}

bool invokable(
	Renderer* rnd, Workspace* ws,
	const Project*,
	const InvokableList* oldRoutines, Text::Array* newRoutines,
	const InvokableList* types, const InvokableList* tooltips,
	bool* unfolded,
	float width, float* height,
	const char* prompt,
	ChildrenHandler prevChildren,
	RefButtonWithIndexClickedHandler refButtonClicked
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float buttonWidth = width - iconSize;

	auto equals = [] (const InvokableList* oldRoutines, const Text::Array* newRoutines) -> bool {
		auto oldRoutinesIterator = oldRoutines->begin();
		auto newRoutinesIterator = newRoutines->begin();
		for (; oldRoutinesIterator != oldRoutines->end() && newRoutinesIterator != newRoutines->end(); ++oldRoutinesIterator, ++newRoutinesIterator) {
			const std::string* oldRoutine = *oldRoutinesIterator;
			const std::string &newRoutine = *newRoutinesIterator;
			if (*oldRoutine != newRoutine)
				return false;
		}

		return true;
	};

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	const float childWidth = size * X_COUNT;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (unfolded && *unfolded)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::PushID("@Inv");
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		const float oldY = ImGui::GetCursorPosY();
		if (height) {
			const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
			drawList->AddRectFilled(
				curPos + ImVec2(xOffset - 1, 0),
				curPos + ImVec2(xOffset - 1 + size * X_COUNT, *height),
				theme->style()->screenClearingColor
			);
		}

		if (prevChildren) {
			if (prevChildren(xOffset, childWidth))
				result = true;
		}

		GBBASIC_ASSERT(oldRoutines->size() == types->size() && newRoutines->size() == types->size() && types->size() == tooltips->size() && "Count doesn't match.");
		InvokableList::iterator oldRoutineIterator = oldRoutines->begin();
		int newRoutineIndex = 0;
		InvokableList::iterator typeIterator = types->begin();
		InvokableList::iterator tooltipIterator = tooltips->begin();
		for (
			;
			oldRoutineIterator != oldRoutines->end() && newRoutineIndex < (int)newRoutines->size() && typeIterator != types->end() && tooltipIterator != tooltips->end();
			++oldRoutineIterator, ++newRoutineIndex, ++typeIterator, ++tooltipIterator
		) {
			const std::string* oldRoutine = *oldRoutineIterator;
			std::string &newRoutine = (*newRoutines)[newRoutineIndex];
			const std::string* type = *typeIterator;
			const std::string* tooltip = *tooltipIterator;
			ImGui::PushID(type);
			{
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(*type);
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(*tooltip);
				}

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				const char* btnTxt = oldRoutine->empty() ? "-" : oldRoutine->c_str();
				if (ImGui::Button(btnTxt, ImVec2(buttonWidth - 1, 0.0f))) {
					if (refButtonClicked)
						refButtonClicked(newRoutineIndex);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltip_SetRoutine());
				}
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconReset()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE), false, theme->tooltip_Clear().c_str())) {
					newRoutine.clear();

					result |= !equals(oldRoutines, newRoutines);
				}
			}
			ImGui::PopID();
		}

		const float h = ImGui::GetCursorPosY() - oldY;
		if (height)
			*height = h;

		if (unfolded)
			*unfolded = true;
	} else {
		if (unfolded)
			*unfolded = false;
	}
	ImGui::PopID();

	return result;
}

bool compacted(
	Renderer* rnd, Workspace* ws,
	Texture* tex,
	bool filled,
	bool* save,
	float width,
	const char* prompt
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	if (save)
		*save = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(3, 0));
	ImGui::SameLine();
	if (filled) {
		if (ImGui::ImageButton(theme->iconExport()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE), false, theme->tooltipActor_Export().c_str())) {
			if (save)
				*save = true;
		}
	} else {
		if (ImGui::ImageButton(theme->iconRefresh()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE), false, theme->tooltipActor_Refresh().c_str())) {
			result = true;
		}
	}
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	const float childWidth = size * X_COUNT;
	ImGui::BeginChild("@Cpct", ImVec2(childWidth, childWidth), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav);
	if (tex) {
		const int texw = tex->width();
		const int texh = tex->height();
		const int scale = (texw / GBBASIC_TILE_SIZE) <= 4 ?
			Math::clamp((int)std::floor((childWidth - 3) / texw), 1, 16) :
			Math::clamp((int)std::floor(childWidth / texw), 1, 16);

		VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

		const float widgetWidth = (float)texw * scale;
		const float widgetHeight = (float)texh * scale;
		const ImVec2 widgetSize(widgetWidth, widgetHeight);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		const ImVec2 frameSize = widgetSize + ImVec2(style.WindowBorderSize * 2 + 2, style.WindowBorderSize * 2 + 2);
		ImGui::BeginChildFrame(
			ImGui::GetID("@Tex"),
			frameSize,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav
		);
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			const ImVec2 pos = ImGui::GetCursorPos();
			const ImVec2 gsize((float)texw / (GBBASIC_TILE_SIZE * 2), (float)texh / (GBBASIC_TILE_SIZE * 2));
			const ImVec2 size(std::ceil(widgetSize.x / gsize.x), std::ceil(widgetSize.y / gsize.y));
			for (int y = 0; y < gsize.y; ++y) {
				for (int x = 0; x < gsize.x; ++x) {
					ImGui::SetCursorPos(pos + ImVec2(size.x * x, size.y * y + 1));
					ImGui::Image(theme->iconTransparent()->pointer(rnd), size);
				}
			}

			ImGui::SetCursorPos(pos);
			ImGui::Image(
				(void*)tex->pointer(rnd),
				ImVec2(widgetWidth, widgetHeight),
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.0f)
			);
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::EndChildFrame();
	} else {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("...");
	}
	ImGui::EndChild();

	return result;
}

bool listable(
	Renderer*, Workspace* ws,
	const Objects &objs,
	int* selected,
	float width,
	const char* prompt
) {
	ImGuiStyle &style = ImGui::GetStyle();
	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	if (selected)
		*selected = -1;

	const float childWidth = size * X_COUNT;
	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		if (objs.indices.empty()) {
			ImGui::NewLine(1);

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();

			ImGui::TextUnformatted(theme->generic_None());
		} else {
			for (int i = 0; i < (int)objs.indices.size(); ++i) {
				ImGui::PushID(i + 1);

				ImGui::NewLine(1);

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::Button(objs.indices[i].shortName, ImVec2(iconSize * 5, 0))) {
					result = true;

					if (selected)
						*selected = i;
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(objs.indices[i].name);
				}

				ImGui::PopID();
			}
		}
	}

	return result;
}

bool orderable(
	Renderer* rnd, Workspace* ws,
	const Music::Channels &oldChannels, Music::Channels &newChannels,
	float width,
	bool* initialized, bool* focused,
	const Math::Vec2i &charSize,
	int* channelIndex, int* lineIndex,
	bool* editing,
	const char* prompt,
	const char* const* byteNumbers
) {
	ImGuiStyle &style = ImGui::GetStyle();
	Theme* theme = ws->theme();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconInsert_24x24()->pointer(rnd),
		theme->iconMinus_24x24()->pointer(rnd),
		theme->iconAppend_24x24()->pointer(rnd),
		theme->iconUp_24x24()->pointer(rnd),
		theme->iconDown_24x24()->pointer(rnd)
	};
	const char* const tooltips[] = {
		theme->tooltipAudio_InsertRowOfOrders().c_str(),
		theme->tooltipAudio_DeleteRowOfOrders().c_str(),
		theme->tooltipAudio_AppendRowOfOrders().c_str(),
		theme->tooltipAudio_MoveRowOfOrdersUp().c_str(),
		theme->tooltipAudio_MoveRowOfOrdersDown().c_str()
	};
	const ImU32 colors[] = {
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE
	};
	auto getMaxOrderIndex = [] (const Music::Sequence &seq) -> int {
		int val = 0;
		for (int j = 0; j < (int)seq.size(); ++j) {
			if (seq[j] > val)
				val = seq[j];
		}

		return val;
	};

	if (focused)
		*focused = false;

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	bool changed = false;
	int op = -1;
	const bool canAdd = newChannels.front().size() < GBBASIC_MUSIC_SEQUENCE_MAX_COUNT;
	const bool canDel = newChannels.front().size() > 1;
	const bool canMove = newChannels.front().size() > 1;
	for (int i = 0, j = 0; i < GBBASIC_COUNTOF(icons); ++i) {
		if (xOffset > 0 && (j % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		bool enabled = true;
		if (i == 0 || i == 2) // Insert, append.
			enabled = canAdd;
		else if (i == 1) // Delete.
			enabled = canDel;
		else if (i == 3 || i == 4) // Move up, move down.
			enabled = canMove;

		if (enabled) {
			if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), false, tooltips[i])) {
				op = i;
			}
		} else {
			ImGui::BeginDisabled();
			{
				ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), false, tooltips[i]);
			}
			ImGui::EndDisabled();
		}

		if (((j + 1) % X_COUNT) != 0 && i != GBBASIC_COUNTOF(icons) - 1)
			ImGui::SameLine();

		++j;
	}
	ImGui::NewLine(1);
	if (op != -1) {
		switch (op) {
		case 0: { // Insert.
				for (int i = 0; i < (int)newChannels.size(); ++i) {
					Music::Sequence &seq = newChannels[i];
					const int val = getMaxOrderIndex(seq) + 1;
					seq.insert(seq.begin() + *lineIndex, val);
				}
				changed = true;
			}

			break;
		case 1: { // Delete.
				int affected = 0;
				for (int i = 0; i < (int)newChannels.size(); ++i) {
					Music::Sequence &seq = newChannels[i];
					if (seq.empty())
						continue;

					seq.erase(seq.begin() + *lineIndex);
					++affected;
				}
				if (affected) {
					*lineIndex = Math::clamp(*lineIndex, 0, (int)newChannels.front().size() - 1);
					changed = true;
				}
			}

			break;
		case 2: { // Append.
				for (int i = 0; i < (int)newChannels.size(); ++i) {
					Music::Sequence &seq = newChannels[i];
					const int val = getMaxOrderIndex(seq) + 1;
					seq.push_back(val);
				}
				*lineIndex = (int)newChannels.front().size() - 1;
				changed = true;
			}

			break;
		case 3: { // Move up.
				int affected = 0;
				for (int i = 0; i < (int)newChannels.size(); ++i) {
					Music::Sequence &seq = newChannels[i];
					if (seq.size() <= 1)
						continue;
					if (*lineIndex <= 0)
						break;

					std::swap(seq[*lineIndex], seq[*lineIndex - 1]);
					++affected;
				}
				if (affected) {
					--*lineIndex;
					changed = true;
				}
			}

			break;
		case 4: { // Move down.
				int affected = 0;
				for (int i = 0; i < (int)newChannels.size(); ++i) {
					Music::Sequence &seq = newChannels[i];
					if (seq.size() <= 1)
						continue;
					if (*lineIndex >= (int)seq.size() - 1)
						break;

					std::swap(seq[*lineIndex], seq[*lineIndex + 1]);
					++affected;
				}
				if (affected) {
					++*lineIndex;
					changed = true;
				}
			}

			break;
		}
	}

	do {
		if (newChannels.front().empty())
			break;

		bool showMenu = false;
		const ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TableBorderStrong));
		const ImU32 rectCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Button));
		const float paddingHeader = 2;
		const float paddingButton = 0;
		ImGui::Dummy(ImVec2(xOffset, 0));
		ImGui::SameLine();
		const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg); //ImVec4(0.0f, 1.0f, 0.5f, 1.0f)
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		{
			const ImVec2 colStartPos = ImGui::GetCursorScreenPos();
			float colEndPosY = 0;
			ImVec2 rowStartPos = colStartPos;

			const ImU32 col = theme->style()->musicSideColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			{
				ImGui::Dummy(ImVec2(size, 0));
				ImGui::SameLine();
				ImVec2 oldPos = ImGui::GetCursorPos();
				ImGui::Dummy(ImVec2(paddingHeader + 2, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("D1");

				ImGui::SetCursorPos(oldPos);
				ImGui::Dummy(ImVec2(size, 0));
				ImGui::SameLine();
				oldPos = ImGui::GetCursorPos();
				ImGui::Dummy(ImVec2(paddingHeader + 2, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("D2");

				ImGui::SetCursorPos(oldPos);
				ImGui::Dummy(ImVec2(size, 0));
				ImGui::SameLine();
				oldPos = ImGui::GetCursorPos();
				ImGui::Dummy(ImVec2(paddingHeader + 2, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("W1");

				ImGui::SetCursorPos(oldPos);
				ImGui::Dummy(ImVec2(size, 0));
				ImGui::SameLine();
				oldPos = ImGui::GetCursorPos();
				ImGui::Dummy(ImVec2(paddingHeader + 2, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("N1");
			}
			ImGui::PopStyleColor();

			const float rowEndPosX = rowStartPos.x + size * X_COUNT - 1;
			float rowEndPosY = ImGui::GetCursorScreenPos().y;
			drawList->AddLine( // Horizontal lines.
				ImVec2(rowStartPos.x, rowStartPos.y),
				ImVec2(rowEndPosX, rowStartPos.y),
				lineCol
			);
			drawList->AddLine(
				ImVec2(rowStartPos.x, rowEndPosY),
				ImVec2(rowEndPosX, rowEndPosY),
				lineCol
			);

			ImGuiMouseButton mouseButton = ImGuiMouseButton_Left;
			bool doubleClicked = false;
			const ImVec2 charSize_((float)charSize.x, (float)charSize.y);
			const ImVec2 btnSize(size - 1, charSize_.y);
			for (int j = 0; j < (int)newChannels.front().size(); ++j) {
				ImGui::PushID(j + 1);

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();

				rowStartPos = ImGui::GetCursorScreenPos();

				ImVec2 oldPos = ImGui::GetCursorPos();
				if (paddingButton > 0) {
					ImGui::Dummy(ImVec2(paddingButton, 0));
					ImGui::SameLine();
				}
				const ImU32 col = theme->style()->musicSideColor;
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				if (ImGui::AdjusterButton(byteNumbers[j], &btnSize, charSize_, nullptr, nullptr, false)) {
					*lineIndex = j;
				}
				ImGui::PopStyleColor();
				ImGui::SetCursorPos(oldPos);
				ImGui::Dummy(ImVec2(size, 0));
				ImGui::SameLine();

				for (int i = 0; i < GBBASIC_MUSIC_CHANNEL_COUNT; ++i) {
					ImGui::PushID(i + 1);

					ImGui::SetCursorPos(oldPos);
					ImGui::Dummy(ImVec2(size, 0));
					ImGui::SameLine();
					oldPos = ImGui::GetCursorPos();
					if (paddingButton > 0) {
						ImGui::Dummy(ImVec2(paddingButton, 0));
						ImGui::SameLine();
					}

					const int idx = newChannels[i][j];
					if (*editing && *channelIndex == i && *lineIndex == j) {
						ImGui::PushID("@Ord");
						do {
							bool first = false;
							ImGui::SameLine();
							if (!*initialized) {
								ImGui::SetKeyboardFocusHere();
								*initialized = true;
								first = true;
							}
							ImGui::SetNextItemWidth(btnSize.x);
							const ImVec2 startPos = ImGui::GetCursorScreenPos();
							char buf[4];
							snprintf(buf, GBBASIC_COUNTOF(buf), "%02X", idx);
							const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
							if (ImGui::GetActiveID() == ImGui::GetID("")) {
								if (focused)
									*focused = true;
							}
							if (first) {
								break;
							}
							if (edited) {
								// Do nothing.
							} else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
								if (ImGui::IsMouseHoveringRect(startPos, startPos + btnSize))
									break;
							} else if (ImGui::IsItemFocused()) {
								if (!edited)
									break;
							}
							*initialized = false;
							*focused = false;
							*editing = false;
							const std::string hex = std::string("0x") + buf;
							int val = 0;
							if (!Text::fromString(hex, val))
								break;
							if (val < 0 || val >= GBBASIC_MUSIC_ORDER_COUNT)
								break;

							changed = true;

							newChannels[i][j] = val;
						} while (false);
						ImGui::PopID();
					} else {
						if (ImGui::AdjusterButton(byteNumbers[idx], &btnSize, charSize_, &mouseButton, &doubleClicked, false)) {
							const bool sameLine = *lineIndex == j;

							*channelIndex = i;
							*lineIndex = j;

							*initialized = false;
							*focused = false;

							if (mouseButton == ImGuiMouseButton_Left) {
								*editing = sameLine;
							} else if (mouseButton == ImGuiMouseButton_Right) {
								showMenu = true;
							}
						}
					}

					ImGui::PopID();
				}

				rowEndPosY = ImGui::GetCursorScreenPos().y;
				colEndPosY = rowEndPosY;
				drawList->AddLine( // Horizontal lines.
					ImVec2(rowStartPos.x, rowStartPos.y),
					ImVec2(rowEndPosX, rowStartPos.y),
					lineCol
				);
				drawList->AddLine(
					ImVec2(rowStartPos.x, rowEndPosY),
					ImVec2(rowEndPosX, rowEndPosY),
					lineCol
				);

				ImGui::PopID();
			}
			for (int i = 0; i <= GBBASIC_MUSIC_CHANNEL_COUNT + 1; ++i) {
				const float offset = i == 0 ? 0.0f : 1.0f;
				drawList->AddLine( // Vertical lines.
					ImVec2(colStartPos.x + size * i - offset, colStartPos.y),
					ImVec2(colStartPos.x + size * i - offset, colEndPosY),
					lineCol
				);
			}
			const ImVec2 rectStartPos = colStartPos + ImVec2(size - 1, charSize_.y * (*lineIndex + 1));
			drawList->AddRect( // Curcor rectangle.
				rectStartPos,
				rectStartPos + ImVec2(size * (X_COUNT - 1) + 1, charSize_.y + 1),
				rectCol
			);
		}
		ImGui::PopStyleColor(3);

		if (showMenu) {
			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}
		do {
			static constexpr const char* const BYTE_GROUPS[] = {
				"00 - 0F", "10 - 1F", "20 - 2F", "30 - 3F", "40 - 4F", "50 - 5F", "60 - 6F", "70 - 7F",
				"80 - 8F", "90 - 9F", "A0 - AF", "B0 - BF", "C0 - CF", "D0 - DF", "E0 - EF", "F0 - FF",
			};

			ImGuiStyle &style = ImGui::GetStyle();

			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			if (ImGui::BeginPopup("@Ctx")) {
				int code = EDITING_MUSIC_NO_INPUT_VALUE;
				constexpr const int M = GBBASIC_COUNTOF(BYTE_GROUPS);
				constexpr const int N = (std::numeric_limits<Byte>::max() + 1) / M;
				for (int i = 0; i < M; ++i) {
					if (ImGui::BeginMenu(BYTE_GROUPS[i])) {
						for (int j = 0; j < N; ++j) {
							if (ImGui::MenuItem(byteNumbers[j + M * i])) {
								code = j + M * i;
							}
						}

						ImGui::EndMenu();
					}
				}

				ImGui::EndPopup();

				if (code != EDITING_MUSIC_NO_INPUT_VALUE) {
					changed = true;

					newChannels[*channelIndex][*lineIndex] = code;
				}
			}
		} while (false);
	} while (false);

	const bool result = changed ?
		oldChannels != newChannels :
		false;

	return result;
}

bool channels(
	Renderer* rnd, Workspace* ws,
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconMagnify()->width(); // Borrowed.
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	constexpr const char* const TOOLTIP = "(,/.)";
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_COMMA),
		Shortcut(SDL_SCANCODE_PERIOD)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

	if (ImGui::ImageButton(theme->iconChannels()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIP) && focusingWnd) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}
	if (allowShortcuts && focusingWnd && shortcuts[0].pressed()) {
		result = true;

		if (cursor) {
			if (--*cursor < 0)
				*cursor = 3;
		}
	} else if (allowShortcuts && focusingWnd && shortcuts[1].pressed()) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	drawList->PathLineTo(pos + ImVec2(1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, size - 1));
	drawList->PathLineTo(pos + ImVec2(1, size - 1));
	drawList->PathStroke(col, true);

	if (cursor && *cursor >= 0 && *cursor <= 3) {
		drawList->PathLineTo(pos + ImVec2(size * *cursor, 1));
		drawList->PathLineTo(pos + ImVec2(size * (*cursor + 1), 1));
		drawList->PathLineTo(pos + ImVec2(size * (*cursor + 1), size - 1));
		drawList->PathLineTo(pos + ImVec2(size * *cursor, size - 1));
		drawList->PathFillConvex(col);

		Texture* texs[] = {
			theme->iconNumber0(),
			theme->iconNumber1(),
			theme->iconNumber2(),
			theme->iconNumber3()
		};
		Texture* tex = texs[*cursor];
		drawList->AddImage(
			tex->pointer(rnd),
			ImVec2(pos + ImVec2(size * *cursor, 1)), ImVec2(pos + ImVec2(size * (*cursor + 1), size - 1)),
			ImVec2(0, 0), ImVec2(1, 1),
			theme->style()->musicSideColor
		);

		const ImRect rects[] = {
			ImRect(
				ImVec2(pos + ImVec2(0, 1)),
				ImVec2(pos + ImVec2(size, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size, 1)),
				ImVec2(pos + ImVec2(size * 2, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 2, 1)),
				ImVec2(pos + ImVec2(size * 3, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 3, 1)),
				ImVec2(pos + ImVec2(size * 4, size - 1))
			)
		};
		for (int i = 0; i < GBBASIC_COUNTOF(rects); ++i) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(rects[i].Min, rects[i].Max, false) && focusingWnd) {
				result = true;

				*cursor = i;

				break;
			}
		}
	}

	ImGui::NewLine();

	return result;
}

bool instruments(
	Renderer* rnd, Workspace* ws,
	Music* ptr,
	Music::DutyInstrument* newDutyInst,
	Music::WaveInstrument* newWaveInst,
	Music::NoiseInstrument* newNoiseInst,
	std::array<int, GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT>* instWaveform, bool* instWaveformMouseDown,
	Music::NoiseInstrument::Macros* instMacros, bool* instMacrosMouseDown,
	bool changedInst,
	bool* unfolded,
	int* cursor,
	float width, float* height,
	const char* prompt,
	OperationHandler operate
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	const float childWidth = size * X_COUNT;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (unfolded && *unfolded)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::PushID("@Inst");
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		const float oldY = ImGui::GetCursorPosY();
		if (height) {
			const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
			drawList->AddRectFilled(
				curPos + ImVec2(xOffset - 1, 0),
				curPos + ImVec2(xOffset - 1 + size * X_COUNT, *height),
				theme->style()->screenClearingColor
			);
		}

		typedef std::function<const Music::BaseInstrument*(const Music::Song*, int, Music::Instruments*)> InstrumentGetter;
		InstrumentGetter getInstrument = [newDutyInst, newWaveInst, newNoiseInst] (const Music::Song* song, int idx, Music::Instruments* type) -> const Music::BaseInstrument* {
			const Music::BaseInstrument* inst = nullptr;
			if (newDutyInst && newWaveInst && newNoiseInst) {
				inst = song->instrumentAt(idx, type);
			} else if (newDutyInst) {
				inst = (const Music::BaseInstrument*)(&song->dutyInstruments[idx]);
				if (type)
					*type = Music::Instruments::SQUARE;
			} else if (newWaveInst) {
				inst = (const Music::BaseInstrument*)(&song->waveInstruments[idx]);
				if (type)
					*type = Music::Instruments::WAVE;
			} else if (newNoiseInst) {
				inst = (const Music::BaseInstrument*)(&song->noiseInstruments[idx]);
				if (type)
					*type = Music::Instruments::NOISE;
			}

			return inst;
		};
		struct Context {
			const Music::Song* song = nullptr;
			Music::DutyInstrument* newDutyInst = nullptr;
			Music::WaveInstrument* newWaveInst = nullptr;
			Music::NoiseInstrument* newNoiseInst = nullptr;
			InstrumentGetter instrumentGetter = nullptr;
		};

		const Music::Song* song = ptr->pointer();
		int totalCount = 0;
		if (newDutyInst)
			totalCount += (int)song->dutyInstruments.size();
		if (newWaveInst)
			totalCount += (int)song->waveInstruments.size();
		if (newNoiseInst)
			totalCount += (int)song->noiseInstruments.size();

		if (operate) {
			ImGui::PushID("@Opt");
			{
				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();

				const float btnWidth = (childWidth - 3) / 4.0f;

				if (ImGui::Button(theme->dialogPrompt_Cpy(), ImVec2(btnWidth + 1, 0.0f))) {
					operate(Operations::COPY);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipAudio_CopyInstrument());
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();

				if (ImGui::Button(theme->dialogPrompt_Cut(), ImVec2(btnWidth, 0.0f))) {
					operate(Operations::CUT);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipAudio_CutInstrument());
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();

				if (ImGui::Button(theme->dialogPrompt_Pst(), ImVec2(btnWidth, 0.0f))) {
					operate(Operations::PASTE);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipAudio_PasteInstrument());
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();

				if (ImGui::Button(theme->dialogPrompt_Rst(), ImVec2(btnWidth, 0.0f))) {
					operate(Operations::RESET);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipAudio_ResetInstrument());
				}
			}
			ImGui::PopID();
		}

		ImGui::PushID("@Cmb");
		{
			if (*cursor == -1) {
				changedInst |= true;
				*cursor = 0;
			}

			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();

			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			Context data;
			data.song = song;
			data.newDutyInst = newDutyInst;
			data.newWaveInst = newWaveInst;
			data.newNoiseInst = newNoiseInst;
			data.instrumentGetter = getInstrument;
			ImGui::SetNextItemWidth(size * X_COUNT);
			changedInst |= ImGui::Combo(
				"",
				cursor,
				[] (void* data, int idx, const char** outText) -> bool {
					Context* data_ = (Context*)data;
					const Music::Song* song = data_->song;
					const Music::BaseInstrument* inst = data_->instrumentGetter(song, idx, nullptr);
					*outText = inst->name.c_str();

					return true;
				},
				(void*)&data, totalCount
			);
		}
		ImGui::PopID();

		Music::Instruments type;
		const Music::BaseInstrument* inst = getInstrument(song, *cursor, &type);
		bool changed = false;
		Music::BaseInstrument* newInst = nullptr;
		int minLength = 0;
		int maxLength = 0;
		if (type == Music::Instruments::SQUARE) {
			newInst = newDutyInst;
			minLength = MUSIC_DUTY_INSTRUMENT_MIN_LENGTH;
			maxLength = MUSIC_DUTY_INSTRUMENT_MAX_LENGTH;
			if (changedInst) {
				Music::DutyInstrument* instImpl = (Music::DutyInstrument*)inst;
				*newDutyInst = *instImpl;
			}
		} else if (type == Music::Instruments::WAVE) {
			newInst = newWaveInst;
			minLength = MUSIC_WAVE_INSTRUMENT_MIN_LENGTH;
			maxLength = MUSIC_WAVE_INSTRUMENT_MAX_LENGTH;
			if (changedInst) {
				Music::WaveInstrument* instImpl = (Music::WaveInstrument*)inst;
				*newWaveInst = *instImpl;
			}
		} else if (type == Music::Instruments::NOISE) {
			newInst = newNoiseInst;
			minLength = MUSIC_NOISE_INSTRUMENT_MIN_LENGTH;
			maxLength = MUSIC_NOISE_INSTRUMENT_MAX_LENGTH;
			if (changedInst) {
				Music::NoiseInstrument* instImpl = (Music::NoiseInstrument*)inst;
				*newNoiseInst = *instImpl;
			}
		}

		ImGui::PushID("@Len");
		{
			int val = newInst->length.isLeft() ? newInst->length.left().get() : minLength - 1;

			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Length());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				const int val_ = Math::max(val - 1, minLength - 1);
				if (val != val_) {
					if (val_ == minLength - 1)
						newInst->length = Right<std::nullptr_t>(nullptr);
					else
						newInst->length = val_;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (newInst->length.isLeft()) {
				int val_ = val;
				if (ImGui::DragInt("", &val_, 1.0f, minLength - 1, maxLength, "%d")) {
					if (val != val_) {
						if (val_ == minLength - 1)
							newInst->length = Right<std::nullptr_t>(nullptr);
						else
							newInst->length = val_;
					}
				}
			} else {
				int val_ = val;
				if (ImGui::DragInt("", &val_, 1.0f, minLength - 1, maxLength, "None")) {
					if (val != val_) {
						if (val_ == minLength - 1)
							newInst->length = Right<std::nullptr_t>(nullptr);
						else
							newInst->length = val_;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newInst->length = Math::min(val + 1, maxLength);
				const int val_ = Math::min(val + 1, maxLength);
				if (val != val_) {
					if (val_ == minLength - 1)
						newInst->length = Right<std::nullptr_t>(nullptr);
					else
						newInst->length = val_;
				}
			}
			changed |= newInst->length != inst->length;
		}
		ImGui::PopID();

		std::function<void(void)> revert = nullptr;
		if (type == Music::Instruments::SQUARE) {
			Music::DutyInstrument* instImpl = (Music::DutyInstrument*)inst;
			revert = [newDutyInst, instImpl] (void) -> void {
				*newDutyInst = *instImpl;
			};

			ImGui::PushID("@IVol");
			{
				int val = newDutyInst->initialVolume;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_InitialVolume());

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newDutyInst->initialVolume = Math::max(val - 1, MUSIC_DUTY_INSTRUMENT_MIN_INITIAL_VOLUME);
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &val, 1.0f, MUSIC_DUTY_INSTRUMENT_MIN_INITIAL_VOLUME, MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME, "%d")) {
					if ((int)newDutyInst->initialVolume != val) {
						newDutyInst->initialVolume = val;
					}
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newDutyInst->initialVolume = Math::min(val + 1, MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME);
				}
				changed |= newDutyInst->initialVolume != instImpl->initialVolume;
			}
			ImGui::PopID();

			ImGui::PushID("@SChg");
			{
				int val = newDutyInst->volumeSweepChange;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_SweepChange());

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newDutyInst->volumeSweepChange = Math::max(val - 1, MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE);
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &val, 1.0f, MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE, MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE, "%d")) {
					if ((int)newDutyInst->volumeSweepChange != val) {
						newDutyInst->volumeSweepChange = val;
					}
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newDutyInst->volumeSweepChange = Math::min(val + 1, MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE);
				}
				changed |= newDutyInst->volumeSweepChange != instImpl->volumeSweepChange;
			}
			ImGui::PopID();

			ImGui::PushID("@STim");
			{
				int val = newDutyInst->frequencySweepTime;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_SweepTime());

				const char* items[] = {
					theme->dialogPrompt_SweepTime_Off().c_str(),
					theme->dialogPrompt_SweepTime_1().c_str(),
					theme->dialogPrompt_SweepTime_2().c_str(),
					theme->dialogPrompt_SweepTime_3().c_str(),
					theme->dialogPrompt_SweepTime_4().c_str(),
					theme->dialogPrompt_SweepTime_5().c_str(),
					theme->dialogPrompt_SweepTime_6().c_str(),
					theme->dialogPrompt_SweepTime_7().c_str()
				};

				bool showTooltip = false;
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &val, items, GBBASIC_COUNTOF(items))) {
						newDutyInst->frequencySweepTime = val;
					}
					changed |= newDutyInst->frequencySweepTime != instImpl->frequencySweepTime;
					showTooltip |= ImGui::IsItemHovered();
				} while (false);
			}
			ImGui::PopID();

			ImGui::PushID("@SSft");
			{
				int val = newDutyInst->frequencySweepShift;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_SweepShift());

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newDutyInst->frequencySweepShift = Math::max(val - 1, MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_SHIFT);
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &val, 1.0f, MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_SHIFT, MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_SHIFT, "%d")) {
					if ((int)newDutyInst->frequencySweepShift != val) {
						newDutyInst->frequencySweepShift = val;
					}
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newDutyInst->frequencySweepShift = Math::min(val + 1, MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_SHIFT);
				}
				changed |= newDutyInst->frequencySweepShift != instImpl->frequencySweepShift;
			}
			ImGui::PopID();

			ImGui::PushID("@Duty");
			{
				int val = newDutyInst->dutyCycle;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_Duty());

				const char* items[] = {
					theme->dialogPrompt_Duty_12_5().c_str(),
					theme->dialogPrompt_Duty_25_0().c_str(),
					theme->dialogPrompt_Duty_50_0().c_str(),
					theme->dialogPrompt_Duty_75_0().c_str()
				};

				bool showTooltip = false;
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &val, items, GBBASIC_COUNTOF(items))) {
						newDutyInst->dutyCycle = val;
					}
					changed |= newDutyInst->dutyCycle != instImpl->dutyCycle;
					showTooltip |= ImGui::IsItemHovered();
				} while (false);
			}
			ImGui::PopID();
		} else if (type == Music::Instruments::WAVE) {
			Music::WaveInstrument* instImpl = (Music::WaveInstrument*)inst;
			revert = [newWaveInst, instImpl] (void) -> void {
				*newWaveInst = *instImpl;
			};

			ImGui::PushID("@Vol");
			{
				int val = newWaveInst->volume;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_Volume());

				const char* items[] = {
					theme->dialogPrompt_Volume_Mute().c_str(),
					theme->dialogPrompt_Volume_100().c_str(),
					theme->dialogPrompt_Volume_50().c_str(),
					theme->dialogPrompt_Volume_25().c_str()
				};

				bool showTooltip = false;
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &val, items, GBBASIC_COUNTOF(items))) {
						newWaveInst->volume = val;
					}
					changed |= newWaveInst->volume != instImpl->volume;
					showTooltip |= ImGui::IsItemHovered();
				} while (false);
			}
			ImGui::PopID();

			ImGui::PushID("@Wfm");
			{
				int val = newWaveInst->waveIndex;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_Waveform());

				const char* items[] = {
					theme->dialogPrompt_Waveform_0().c_str(),
					theme->dialogPrompt_Waveform_1().c_str(),
					theme->dialogPrompt_Waveform_2().c_str(),
					theme->dialogPrompt_Waveform_3().c_str(),
					theme->dialogPrompt_Waveform_4().c_str(),
					theme->dialogPrompt_Waveform_5().c_str(),
					theme->dialogPrompt_Waveform_6().c_str(),
					theme->dialogPrompt_Waveform_7().c_str(),
					theme->dialogPrompt_Waveform_8().c_str(),
					theme->dialogPrompt_Waveform_9().c_str(),
					theme->dialogPrompt_Waveform_10().c_str(),
					theme->dialogPrompt_Waveform_11().c_str(),
					theme->dialogPrompt_Waveform_12().c_str(),
					theme->dialogPrompt_Waveform_13().c_str(),
					theme->dialogPrompt_Waveform_14().c_str(),
					theme->dialogPrompt_Waveform_15().c_str()
				};

				bool showTooltip = false;
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &val, items, GBBASIC_COUNTOF(items))) {
						newWaveInst->waveIndex = val;
					}
					changed |= newWaveInst->waveIndex != instImpl->waveIndex;
					showTooltip |= ImGui::IsItemHovered();
				} while (false);

				if (instWaveform) {
					const Music::Wave &wave = song->waves[val];
					for (int i = 0; i < (int)wave.size(); ++i)
						(*instWaveform)[i] = wave[i];

					ImGui::NewLine(1);
					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();
					ImGui::LineGraph(
						ImVec2(size * X_COUNT, 130),
						ImGui::GetColorU32(ImGuiCol_Text), ImGui::GetColorU32(ImGuiCol_TextDisabled), theme->style()->graphColor, theme->style()->graphDrawingColor,
						Math::Vec2i(1, 1),
						&instWaveform->front(), nullptr,
						0x0, 0xf,
						(int)instWaveform->size(),
						false,
						nullptr,
						instWaveformMouseDown,
						"%d: %d"
					);
				}
			}
			ImGui::PopID();
		} else if (type == Music::Instruments::NOISE) {
			Music::NoiseInstrument* instImpl = (Music::NoiseInstrument*)inst;
			revert = [newNoiseInst, instImpl] (void) -> void {
				*newNoiseInst = *instImpl;
			};

			ImGui::PushID("@IVol");
			{
				int val = newNoiseInst->initialVolume;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_InitialVolume());

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newNoiseInst->initialVolume = Math::max(val - 1, MUSIC_NOISE_INSTRUMENT_MIN_INITIAL_VOLUME);
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &val, 1.0f, MUSIC_NOISE_INSTRUMENT_MIN_INITIAL_VOLUME, MUSIC_NOISE_INSTRUMENT_MAX_INITIAL_VOLUME, "%d")) {
					if ((int)newNoiseInst->initialVolume != val) {
						newNoiseInst->initialVolume = val;
					}
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newNoiseInst->initialVolume = Math::min(val + 1, MUSIC_NOISE_INSTRUMENT_MAX_INITIAL_VOLUME);
				}
				changed |= newNoiseInst->initialVolume != instImpl->initialVolume;
			}
			ImGui::PopID();

			ImGui::PushID("@SChg");
			{
				int val = newNoiseInst->volumeSweepChange;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_SweepChange());

				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newNoiseInst->volumeSweepChange = Math::max(val - 1, MUSIC_NOISE_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE);
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &val, 1.0f, MUSIC_NOISE_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE, MUSIC_NOISE_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE, "%d")) {
					if ((int)newNoiseInst->volumeSweepChange != val) {
						newNoiseInst->volumeSweepChange = val;
					}
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newNoiseInst->volumeSweepChange = Math::min(val + 1, MUSIC_NOISE_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE);
				}
				changed |= newNoiseInst->volumeSweepChange != instImpl->volumeSweepChange;
			}
			ImGui::PopID();

			ImGui::PushID("@BCnt");
			{
				int val = (int)newNoiseInst->bitCount;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(theme->dialogPrompt_BitCount());

				const char* items[] = {
					"7",
					"15"
				};

				bool showTooltip = false;
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &val, items, GBBASIC_COUNTOF(items))) {
						newNoiseInst->bitCount = (Music::NoiseInstrument::BitCount)val;
					}
					changed |= newNoiseInst->bitCount != instImpl->bitCount;
					showTooltip |= ImGui::IsItemHovered();
				} while (false);
			}
			ImGui::PopID();

			ImGui::PushID("@Mcr");
			{
				const Music::NoiseInstrument::Macros &values = newNoiseInst->noiseMacro;

				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				const int n = Math::min((newNoiseInst->bitCount == Music::NoiseInstrument::BitCount::_7 ? 7 : 15), (int)values.size());
				if (
					ImGui::BarGraph(
						ImVec2(size * X_COUNT, 130),
						ImGui::GetColorU32(ImGuiCol_Text), ImGui::GetColorU32(ImGuiCol_TextDisabled), theme->style()->graphColor, theme->style()->graphDrawingColor,
						Math::Vec2i(1, 2),
						values.empty() ? nullptr : &values.front(), (!instMacros || instMacros->empty()) ? nullptr : &instMacros->front(),
						-32, 32,
						n,
						0.5f,
						nullptr,
						instMacrosMouseDown,
						"%d: %d"
					)
				) {
					if (instMacros) {
						newNoiseInst->noiseMacro = *instMacros;
					}
				}
				changed |= newNoiseInst->noiseMacro != instImpl->noiseMacro;
			}
			ImGui::PopID();
		}

		ImGui::SameLine();
		ImGui::NewLine(1);
		ImGui::NewLine(1);
		ImGui::Dummy(ImVec2(xOffset, 0));
		if (changed) {
			editingBeginAppliable(ws->theme());
			{
				ImGui::SameLine();
				if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
					result = true;
				}
				ImGui::SameLine();
			}
			editingEndAppliable();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
				revert();
				changed = true;
			}
		} else {
			ImGui::BeginDisabled();
			{
				ImGui::SameLine();
				ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
				ImGui::SameLine();

				ImGui::Dummy(ImVec2(2, 0));

				ImGui::SameLine();
				ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
			}
			ImGui::EndDisabled();
		}

		const float h = ImGui::GetCursorPosY() - oldY;
		if (height)
			*height = h;

		if (unfolded)
			*unfolded = true;
	} else {
		if (unfolded)
			*unfolded = false;
	}
	ImGui::PopID();

	return result;
}

bool waveform(
	Renderer*,
	Workspace* ws,
	const std::string &oldWave, std::string &newWave,
	float width,
	bool* initialized, bool* focused,
	const char* prompt
) {
	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::PushID("@Txt");
	do {
		if (initialized && !*initialized) {
			ImGui::SetKeyboardFocusHere();
			*initialized = true;
		}
		ImGui::SetNextItemWidth(size * X_COUNT);
		char buf[ASSETS_NAME_STRING_MAX_LENGTH]; // Fixed size.
		const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, oldWave.length());
		if (n > 0)
			memcpy(buf, oldWave.c_str(), n);
		buf[n] = '\0';
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
		const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll);
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		ImGui::PopStyleColor();
		if (!edited)
			break;
		newWave = buf;

		changed = true;
	} while (false);
	ImGui::PopID();

	ImGui::NewLine(1);
	ImGui::Dummy(ImVec2(xOffset, 0));
	if (newWave == oldWave) {
		ImGui::BeginDisabled();
		{
			ImGui::SameLine();
			ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
			ImGui::SameLine();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
		}
		ImGui::EndDisabled();
	} else {
		editingBeginAppliable(ws->theme());
		{
			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
				result = true;
			}
			ImGui::SameLine();
		}
		editingEndAppliable();

		ImGui::Dummy(ImVec2(2, 0));

		ImGui::SameLine();
		if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			newWave = oldWave;
			changed = true;
		}
	}

	return result;
}

bool sound(
	Renderer*,
	Workspace* ws,
	const std::string &oldSound, std::string &newSound,
	float width,
	char* buffer, size_t bufferSize,
	bool* initialized, bool* focused,
	const char* prompt
) {
	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(prompt);

	bool changed = false;
	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();
	ImGui::PushID("@Txt");
	do {
		if (initialized && !*initialized) {
			ImGui::SetKeyboardFocusHere();
			*initialized = true;
		}
		ImGui::SetNextItemWidth(size * X_COUNT);
		if (buffer) {
			const size_t n = Math::min(bufferSize - 1, oldSound.length());
			if (n > 0)
				memcpy(buffer, oldSound.c_str(), n);
			buffer[n] = '\0';
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
			const bool edited = ImGui::InputTextMultiline(
				"##Ipt",
				buffer, bufferSize,
				ImVec2(size * X_COUNT, size * X_COUNT),
				ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::PopStyleColor();
			if (!edited)
				break;
			newSound = buffer;
		} else {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
			const bool edited = ImGui::InputTextMultiline(
				"##Ipt",
				(char*)oldSound.c_str(), oldSound.length(),
				ImVec2(size * X_COUNT, size * X_COUNT),
				ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::PopStyleColor();
			if (!edited)
				break;
		}

		changed = true;
	} while (false);
	ImGui::PopID();

	if (buffer) {
		ImGui::NewLine(1);
		ImGui::Dummy(ImVec2(xOffset, 0));
		if (newSound == oldSound) {
			ImGui::BeginDisabled();
			{
				ImGui::SameLine();
				ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
				ImGui::SameLine();

				ImGui::Dummy(ImVec2(2, 0));

				ImGui::SameLine();
				ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
			}
			ImGui::EndDisabled();
		} else {
			editingBeginAppliable(ws->theme());
			{
				ImGui::SameLine();
				if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
					result = true;
				}
				ImGui::SameLine();
			}
			editingEndAppliable();

			ImGui::Dummy(ImVec2(2, 0));

			ImGui::SameLine();
			if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
				newSound = oldSound;
				changed = true;
			}
		}
	}

	return result;
}

static bool definableViewAsActor(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const ::active_t* oldDef, ::active_t* newDef,
	bool* unfolded,
	bool* viewAsActor,
	float width, float* height,
	bool* focused,
	const char* prompt,
	bool simplified,
	OperationHandler operate
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	const float childWidth = size * X_COUNT;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (unfolded && *unfolded)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::PushID("@Act");
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		bool changed = false;

		const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
		const GBBASIC::Kernel::Behaviour::Array* bhvrs = nullptr;
		int bhvrIdx = 0;
		if (krnl) {
			bhvrs = &krnl->behaviours();
			for (int i = 0; i < (int)bhvrs->size(); ++i) {
				const GBBASIC::Kernel::Behaviour &bhvr = (*bhvrs)[i];
				if (newDef->behaviour == (UInt8)bhvr.value) {
					bhvrIdx = i;

					break;
				}
			}
		}

		const float oldY = ImGui::GetCursorPosY();
		if (height) {
			const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
			drawList->AddRectFilled(
				curPos + ImVec2(xOffset - 1, 0),
				curPos + ImVec2(xOffset - 1 + size * X_COUNT, *height),
				theme->style()->screenClearingColor
			);
		}

		int viewAsActor_ = *viewAsActor ? 0 : 1;
		ImGui::PushID("@View");
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_ViewAs());

			const char* items[] = {
				theme->dialogPrompt_Actor().c_str(),
				theme->dialogPrompt_Projectile().c_str()
			};

			do {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(size * X_COUNT);
				if (ImGui::Combo("", &viewAsActor_, items, GBBASIC_COUNTOF(items))) {
					*viewAsActor = viewAsActor_ == 0;
				}
			} while (false);
			ImGui::SameLine();
			ImGui::NewLine(1);
		}
		ImGui::PopID();

		separate(rnd, ws, width);

		if (operate) {
			ImGui::PushID("@Opt");
			{
				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();

				const float btnWidth = (childWidth - 2) / 2.0f;

				if (ImGui::Button(theme->dialogPrompt_Copy(), ImVec2(btnWidth + 1, 0.0f))) {
					operate(Operations::COPY);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_CopyDefinition());
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();

				if (ImGui::Button(theme->dialogPrompt_Pst(), ImVec2(btnWidth, 0.0f))) {
					operate(Operations::PASTE);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_PasteDefinition());
				}
			}
			ImGui::PopID();

			separate(rnd, ws, width);
		}

		bool hidden = newDef->hidden;
		ImGui::PushID("@Hid");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Checkbox(theme->dialogPrompt_Hidden(), &hidden)) {
				if (newDef->hidden != hidden) {
					newDef->hidden = hidden;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		bool pinned = newDef->pinned;
		ImGui::PushID("@Pin");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Checkbox(theme->dialogPrompt_Pinned(), &pinned)) {
				if (newDef->pinned != pinned) {
					newDef->pinned = pinned;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		bool persistent = newDef->persistent;
		ImGui::PushID("@Pst");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Checkbox(theme->dialogPrompt_Persistent(), &persistent)) {
				if (newDef->persistent != persistent) {
					newDef->persistent = persistent;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		bool following = newDef->following;
		ImGui::PushID("@Flw");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Checkbox(theme->dialogPrompt_Following(), &following)) {
				if (newDef->following != following) {
					newDef->following = following;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		int dirVal = (int)newDef->direction;
		ImGui::PushID("@Dir");
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Direction());

			const char* items[] = {
				theme->dialogPrompt_Direction_S().c_str(),
				theme->dialogPrompt_Direction_E().c_str(),
				theme->dialogPrompt_Direction_N().c_str(),
				theme->dialogPrompt_Direction_W().c_str()
			};

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			do {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(size * X_COUNT);
				if (ImGui::Combo("", &dirVal, items, GBBASIC_COUNTOF(items))) {
					if ((int)newDef->direction != dirVal) {
						newDef->direction = (UInt8)dirVal;
						changed = true;
					}
				}
			} while (false);
		}
		ImGui::PopID();

		ImGui::PushID("@Bnd");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Bounds());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@L");
			{
				int newVal = (int)newDef->bounds.left;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.left != newVal) {
						newDef->bounds.left = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "L: %d")) {
					if ((int)newDef->bounds.left != newVal) {
						newDef->bounds.left = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.left != newVal) {
						newDef->bounds.left = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@R");
			{
				int newVal = (int)newDef->bounds.right;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.right != newVal) {
						newDef->bounds.right = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "R: %d")) {
					if ((int)newDef->bounds.right != newVal) {
						newDef->bounds.right = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.right != newVal) {
						newDef->bounds.right = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@T");
			{
				int newVal = (int)newDef->bounds.top;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.top != newVal) {
						newDef->bounds.top = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "T: %d")) {
					if ((int)newDef->bounds.top != newVal) {
						newDef->bounds.top = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.top != newVal) {
						newDef->bounds.top = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@B");
			{
				int newVal = (int)newDef->bounds.bottom;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.bottom != newVal) {
						newDef->bounds.bottom = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "B: %d")) {
					if ((int)newDef->bounds.bottom != newVal) {
						newDef->bounds.bottom = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.bottom != newVal) {
						newDef->bounds.bottom = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();
		}
		ImGui::PopID();

		ImGui::PushID("@Int");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Interval());
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme->tooltipActor_ForAnimationTicking());
			}

			if (simplified) {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				const char* items[] = {
					theme->tooltipActor_IntervalTooltips()[0].c_str(),
					theme->tooltipActor_IntervalTooltips()[1].c_str(),
					theme->tooltipActor_IntervalTooltips()[2].c_str(),
					theme->tooltipActor_IntervalTooltips()[3].c_str(),
					theme->tooltipActor_IntervalTooltips()[4].c_str(),
					theme->tooltipActor_IntervalTooltips()[5].c_str(),
					theme->tooltipActor_IntervalTooltips()[6].c_str(),
					theme->tooltipActor_IntervalTooltips()[7].c_str(),
					theme->tooltipActor_IntervalTooltips()[8].c_str()
				};
				constexpr const int values[] = {
					255, // Paused.
					127,
					63,
					31,
					15,
					7,
					3,
					1,
					0
				};
				auto indexOf = [&values] (int val) -> int {
					int closestIndex = 0;
					int smallestDiff = std::abs(values[0] - val);
					for (int i = 1; i < GBBASIC_COUNTOF(values); ++i) {
						const int currentDiff = std::abs(values[i] - val);
						if (currentDiff < smallestDiff) {
							smallestDiff = currentDiff;
							closestIndex = i;
						}
					}

					return closestIndex;
				};
				int idx = indexOf(newDef->animation_interval);
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &idx, items, GBBASIC_COUNTOF(items))) {
						if ((int)newDef->animation_interval != values[idx]) {
							newDef->animation_interval = (UInt8)values[idx];
							changed = true;
						}
					}
				} while (false);

				ImGui::SameLine();
				ImGui::NewLine(1);
			} else {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				int newVal = (int)newDef->animation_interval;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, 0);
					if ((int)newDef->animation_interval != newVal) {
						newDef->animation_interval = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_IntervalInfo());
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%dF", ImGuiSliderFlags_NoInput)) {
					if ((int)newDef->animation_interval != newVal) {
						newDef->animation_interval = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_IntervalInfo());
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
					if ((int)newDef->animation_interval != newVal) {
						newDef->animation_interval = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_IntervalInfo());
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Anim");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Animations());

			constexpr const char* const ANIMATION_NAMES[] = {
				"0       ", "1       ", "2       ", "3       ", "4       ", "5       ", "6       ", "7       "
			};
			static_assert(GBBASIC_COUNTOF(ANIMATION_NAMES) == ASSETS_ACTOR_MAX_ANIMATIONS, "Wrong data.");
			for (int i = 0; i < ASSETS_ACTOR_MAX_ANIMATIONS; ++i) {
				ImGui::PushID(i);
				{
					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(ANIMATION_NAMES[i]);
					if (ImGui::IsItemHovered()) {
						do {
							if (!krnl || !bhvrs)
								break;
							if (bhvrIdx < 0 || bhvrIdx >= (int)bhvrs->size())
								break;
							const GBBASIC::Kernel::Behaviour &bhvr = (*bhvrs)[bhvrIdx];

							const int anim = bhvr.animation;
							if (anim < 0 || anim >= (int)krnl->animations().size())
								break;

							const GBBASIC::Kernel::Animations &anims = krnl->animations()[anim];
							if (i >= (int)anims.names.size())
								break;

							const Localization::Dictionary &name = anims.names[i];
							if (name.empty())
								break;

							const char* tooltip = Localization::get(name);
							if (!tooltip)
								break;

							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							ImGui::SetTooltip(tooltip);
						} while (false);
					}

					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();

					ImGui::PushID("@Beg");
					{
						int newVal = (int)newDef->animations[i].begin;
						if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::max(newVal - 1, 0);
							if ((int)newDef->animations[i].begin != newVal) {
								newDef->animations[i].begin = (UInt8)newVal;
								changed = true;
							}
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(dragWidth);
						if (ptr->count() == 1) {
							ImGui::BeginDisabled();
							ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "Begin: %d");
							ImGui::EndDisabled();
						} else {
							if (ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "Begin: %d")) {
								if ((int)newDef->animations[i].begin != newVal) {
									newDef->animations[i].begin = (UInt8)newVal;
									changed = true;
								}
							}
						}
						if (ImGui::GetActiveID() == ImGui::GetID("")) {
							if (focused)
								*focused = true;
						}
						ImGui::SameLine();
						if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::min(newVal + 1, ptr->count() - 1);
							if ((int)newDef->animations[i].begin != newVal) {
								newDef->animations[i].begin = (UInt8)newVal;
								changed = true;
							}
						}
					}
					ImGui::PopID();

					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();
					ImGui::PushID("@End");
					{
						int newVal = (int)newDef->animations[i].end;
						if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::max(newVal - 1, 0);
							if ((int)newDef->animations[i].end != newVal) {
								newDef->animations[i].end = (UInt8)newVal;
								changed = true;
							}
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(dragWidth);
						if (ptr->count() == 1) {
							ImGui::BeginDisabled();
							ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "End: %d");
							ImGui::EndDisabled();
						} else {
							if (ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "End: %d")) {
								if ((int)newDef->animations[i].end != newVal) {
									newDef->animations[i].end = (UInt8)newVal;
									changed = true;
								}
							}
						}
						if (ImGui::GetActiveID() == ImGui::GetID("")) {
							if (focused)
								*focused = true;
						}
						ImGui::SameLine();
						if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::min(newVal + 1, ptr->count() - 1);
							if ((int)newDef->animations[i].end != newVal) {
								newDef->animations[i].end = (UInt8)newVal;
								changed = true;
							}
						}
					}
					ImGui::PopID();
				}
				ImGui::PopID();
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Spd");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_MoveSpeed());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->move_speed;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->move_speed != newVal) {
					newDef->move_speed = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			char buf[16];
			if (newVal == ASSETS_ACTOR_MOVE_SPEED) {
				buf[0] = 'x';
				buf[1] = '1';
				buf[2] = '\0';
			} else if ((newVal % ASSETS_ACTOR_MOVE_SPEED) == 0) {
				const int v = newVal / ASSETS_ACTOR_MOVE_SPEED;
				buf[0] = 'x';
				if (v <= 9) {
					buf[1] = (char)('0' + v);
					buf[2] = '\0';
				} else {
					buf[1] = (char)('0' + v / 10);
					buf[2] = (char)('0' + v % 10);
					buf[3] = '\0';
				}
			} else {
				snprintf(buf, GBBASIC_COUNTOF(buf), "x%.2f", (double)newVal / ASSETS_ACTOR_MOVE_SPEED);
			}
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), buf, ImGuiSliderFlags_NoInput)) {
				if ((int)newDef->move_speed != newVal) {
					newDef->move_speed = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->move_speed != newVal) {
					newDef->move_speed = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Bhvr");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Behaviour());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();

			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			ImGui::SetNextItemWidth(size * X_COUNT);
			const bool changed_ = ImGui::Combo(
				"",
				&bhvrIdx,
				[] (void* data, int idx, const char** outText) -> bool {
					const GBBASIC::Kernel::Behaviour::Array* bhvrs = (const GBBASIC::Kernel::Behaviour::Array*)data;
					const GBBASIC::Kernel::Behaviour &bhvr = (*bhvrs)[idx];
					const char* entry_ = Localization::get(bhvr.name);
					*outText = entry_ ? entry_ : "Unknown";

					return true;
				},
				(void*)bhvrs, (int)bhvrs->size()
			);
			if (changed_) {
				const GBBASIC::Kernel::Behaviour &bhvr = (*bhvrs)[bhvrIdx];
				if ((int)newDef->behaviour != bhvr.value) {
					newDef->behaviour = (UInt8)bhvr.value;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Clg");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_CollisionGroup());
			Byte val = newDef->collision_group;
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				if (io.KeyCtrl)
					ImGui::SetTooltip(theme->generic_ByteHex()[val]);
				else
					ImGui::SetTooltip(theme->generic_ByteBin()[val]);
			}

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::ByteMatrice(rnd, theme, &val, 0b11111111, size * X_COUNT, nullptr, nullptr, nullptr)) {
				if (newDef->collision_group != val) {
					newDef->collision_group = val;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		if (oldDef) {
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			if (*newDef == *oldDef) {
				ImGui::BeginDisabled();
				{
					ImGui::SameLine();
					ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
					ImGui::SameLine();

					ImGui::Dummy(ImVec2(2, 0));

					ImGui::SameLine();
					ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
				}
				ImGui::EndDisabled();
			} else {
				editingBeginAppliable(ws->theme());
				{
					ImGui::SameLine();
					if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
						result = true;
					}
					ImGui::SameLine();
				}
				editingEndAppliable();

				ImGui::Dummy(ImVec2(2, 0));

				ImGui::SameLine();
				if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
					*newDef = *oldDef;
					changed = true;
				}
			}
		} else {
			result = changed;
		}

		const float h = ImGui::GetCursorPosY() - oldY;
		if (height)
			*height = h;

		if (unfolded)
			*unfolded = true;
	} else {
		if (unfolded)
			*unfolded = false;
	}
	ImGui::PopID();

	return result;
}

static bool definableViewAsProjectile(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const ::active_t* oldDef, ::active_t* newDef,
	bool* unfolded,
	bool* viewAsActor,
	float width, float* height,
	bool* focused,
	const char* prompt,
	bool simplified,
	OperationHandler operate
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	const float childWidth = size * X_COUNT;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (unfolded && *unfolded)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::PushID("@Prj");
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		bool changed = false;

		const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();

		const float oldY = ImGui::GetCursorPosY();
		if (height) {
			const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
			drawList->AddRectFilled(
				curPos + ImVec2(xOffset - 1, 0),
				curPos + ImVec2(xOffset - 1 + size * X_COUNT, *height),
				theme->style()->screenClearingColor
			);
		}

		int viewAsActor_ = *viewAsActor ? 0 : 1;
		ImGui::PushID("@View");
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_ViewAs());

			const char* items[] = {
				theme->dialogPrompt_Actor().c_str(),
				theme->dialogPrompt_Projectile().c_str()
			};

			do {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(size * X_COUNT);
				if (ImGui::Combo("", &viewAsActor_, items, GBBASIC_COUNTOF(items))) {
					*viewAsActor = viewAsActor_ == 0;
				}
			} while (false);
			ImGui::SameLine();
			ImGui::NewLine(1);
		}
		ImGui::PopID();

		separate(rnd, ws, width);

		if (operate) {
			ImGui::PushID("@Opt");
			{
				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();

				const float btnWidth = (childWidth - 2) / 2.0f;

				if (ImGui::Button(theme->dialogPrompt_Copy(), ImVec2(btnWidth + 1, 0.0f))) {
					operate(Operations::COPY);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_CopyDefinition());
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();

				if (ImGui::Button(theme->dialogPrompt_Pst(), ImVec2(btnWidth, 0.0f))) {
					operate(Operations::PASTE);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_PasteDefinition());
				}
			}
			ImGui::PopID();

			separate(rnd, ws, width);
		}

		ImGui::PushID("@Bnd");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Bounds());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@L");
			{
				int newVal = (int)newDef->bounds.left;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.left != newVal) {
						newDef->bounds.left = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "L: %d")) {
					if ((int)newDef->bounds.left != newVal) {
						newDef->bounds.left = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.left != newVal) {
						newDef->bounds.left = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@R");
			{
				int newVal = (int)newDef->bounds.right;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.right != newVal) {
						newDef->bounds.right = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "R: %d")) {
					if ((int)newDef->bounds.right != newVal) {
						newDef->bounds.right = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.right != newVal) {
						newDef->bounds.right = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@T");
			{
				int newVal = (int)newDef->bounds.top;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.top != newVal) {
						newDef->bounds.top = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "T: %d")) {
					if ((int)newDef->bounds.top != newVal) {
						newDef->bounds.top = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.top != newVal) {
						newDef->bounds.top = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@B");
			{
				int newVal = (int)newDef->bounds.bottom;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, (int)std::numeric_limits<Int8>::min());
					if ((int)newDef->bounds.bottom != newVal) {
						newDef->bounds.bottom = (Int8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, (int)std::numeric_limits<Int8>::min(), (int)std::numeric_limits<Int8>::max(), "B: %d")) {
					if ((int)newDef->bounds.bottom != newVal) {
						newDef->bounds.bottom = (Int8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<Int8>::max());
					if ((int)newDef->bounds.bottom != newVal) {
						newDef->bounds.bottom = (Int8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();
		}
		ImGui::PopID();

		ImGui::PushID("@Int");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Interval());
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme->tooltipActor_ForAnimationTicking());
			}

			if (simplified) {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				const char* items[] = {
					theme->tooltipActor_IntervalTooltips()[0].c_str(),
					theme->tooltipActor_IntervalTooltips()[1].c_str(),
					theme->tooltipActor_IntervalTooltips()[2].c_str(),
					theme->tooltipActor_IntervalTooltips()[3].c_str(),
					theme->tooltipActor_IntervalTooltips()[4].c_str(),
					theme->tooltipActor_IntervalTooltips()[5].c_str(),
					theme->tooltipActor_IntervalTooltips()[6].c_str(),
					theme->tooltipActor_IntervalTooltips()[7].c_str(),
					theme->tooltipActor_IntervalTooltips()[8].c_str()
				};
				constexpr const int values[] = {
					255, // Paused.
					127,
					63,
					31,
					15,
					7,
					3,
					1,
					0
				};
				auto indexOf = [&values] (int val) -> int {
					int closestIndex = 0;
					int smallestDiff = std::abs(values[0] - val);
					for (int i = 1; i < GBBASIC_COUNTOF(values); ++i) {
						const int currentDiff = std::abs(values[i] - val);
						if (currentDiff < smallestDiff) {
							smallestDiff = currentDiff;
							closestIndex = i;
						}
					}

					return closestIndex;
				};
				int idx = indexOf(newDef->animation_interval);
				do {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(size * X_COUNT);
					if (ImGui::Combo("", &idx, items, GBBASIC_COUNTOF(items))) {
						if ((int)newDef->animation_interval != values[idx]) {
							newDef->animation_interval = (UInt8)values[idx];
							changed = true;
						}
					}
				} while (false);
			} else {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				int newVal = (int)newDef->animation_interval;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, 0);
					if ((int)newDef->animation_interval != newVal) {
						newDef->animation_interval = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_IntervalInfo());
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%dF", ImGuiSliderFlags_NoInput)) {
					if ((int)newDef->animation_interval != newVal) {
						newDef->animation_interval = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_IntervalInfo());
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
					if ((int)newDef->animation_interval != newVal) {
						newDef->animation_interval = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipActor_IntervalInfo());
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Anim");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Animations());

			constexpr const char* const ANIMATION_NAMES[] = {
				"0       ", "1       ", "2       ", "3       "
			};
			static_assert(GBBASIC_COUNTOF(ANIMATION_NAMES) == ASSETS_PROJECTILE_MAX_ANIMATIONS, "Wrong data.");
			for (int i = 0; i < ASSETS_PROJECTILE_MAX_ANIMATIONS; ++i) {
				ImGui::PushID(i);
				{
					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(ANIMATION_NAMES[i]);
					if (ImGui::IsItemHovered()) {
						do {
							if (!krnl)
								break;

							const int anim = krnl->projectileAnimationIndex();
							if (anim < 0 || anim >= (int)krnl->animations().size())
								break;

							const GBBASIC::Kernel::Animations &anims = krnl->animations()[anim];
							if (i >= (int)anims.names.size())
								break;

							const Localization::Dictionary &name = anims.names[i];
							if (name.empty())
								break;

							const char* tooltip = Localization::get(name);
							if (!tooltip)
								break;

							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							ImGui::SetTooltip(tooltip);
						} while (false);
					}

					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();

					ImGui::PushID("@Beg");
					{
						int newVal = (int)newDef->animations[i].begin;
						if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::max(newVal - 1, 0);
							if ((int)newDef->animations[i].begin != newVal) {
								newDef->animations[i].begin = (UInt8)newVal;
								changed = true;
							}
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(dragWidth);
						if (ptr->count() == 1) {
							ImGui::BeginDisabled();
							ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "Begin: %d");
							ImGui::EndDisabled();
						} else {
							if (ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "Begin: %d")) {
								if ((int)newDef->animations[i].begin != newVal) {
									newDef->animations[i].begin = (UInt8)newVal;
									changed = true;
								}
							}
						}
						if (ImGui::GetActiveID() == ImGui::GetID("")) {
							if (focused)
								*focused = true;
						}
						ImGui::SameLine();
						if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::min(newVal + 1, ptr->count() - 1);
							if ((int)newDef->animations[i].begin != newVal) {
								newDef->animations[i].begin = (UInt8)newVal;
								changed = true;
							}
						}
					}
					ImGui::PopID();

					ImGui::Dummy(ImVec2(xOffset, 0));
					ImGui::SameLine();
					ImGui::PushID("@End");
					{
						int newVal = (int)newDef->animations[i].end;
						if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::max(newVal - 1, 0);
							if ((int)newDef->animations[i].end != newVal) {
								newDef->animations[i].end = (UInt8)newVal;
								changed = true;
							}
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(dragWidth);
						if (ptr->count() == 1) {
							ImGui::BeginDisabled();
							ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "End: %d");
							ImGui::EndDisabled();
						} else {
							if (ImGui::DragInt("", &newVal, 1.0f, 0, ptr->count() - 1, "End: %d")) {
								if ((int)newDef->animations[i].end != newVal) {
									newDef->animations[i].end = (UInt8)newVal;
									changed = true;
								}
							}
						}
						if (ImGui::GetActiveID() == ImGui::GetID("")) {
							if (focused)
								*focused = true;
						}
						ImGui::SameLine();
						if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
							newVal = Math::min(newVal + 1, ptr->count() - 1);
							if ((int)newDef->animations[i].end != newVal) {
								newDef->animations[i].end = (UInt8)newVal;
								changed = true;
							}
						}
					}
					ImGui::PopID();
				}
				ImGui::PopID();
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Lift");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_LifeTime());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->life_time;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->life_time != newVal) {
					newDef->life_time = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%d")) {
				if ((int)newDef->life_time != newVal) {
					newDef->life_time = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->life_time != newVal) {
					newDef->life_time = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Spd");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_MoveSpeed());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->move_speed;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->move_speed != newVal) {
					newDef->move_speed = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			char buf[16];
			if (newVal == ASSETS_ACTOR_MOVE_SPEED) {
				buf[0] = 'x';
				buf[1] = '1';
				buf[2] = '\0';
			} else if ((newVal % ASSETS_ACTOR_MOVE_SPEED) == 0) {
				const int v = newVal / ASSETS_ACTOR_MOVE_SPEED;
				buf[0] = 'x';
				if (v <= 9) {
					buf[1] = (char)('0' + v);
					buf[2] = '\0';
				} else {
					buf[1] = (char)('0' + v / 10);
					buf[2] = (char)('0' + v % 10);
					buf[3] = '\0';
				}
			} else {
				snprintf(buf, GBBASIC_COUNTOF(buf), "x%.2f", (double)newVal / ASSETS_ACTOR_MOVE_SPEED);
			}
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), buf, ImGuiSliderFlags_NoInput)) {
				if ((int)newDef->move_speed != newVal) {
					newDef->move_speed = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->move_speed != newVal) {
					newDef->move_speed = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Off");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_InitialOffset());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->initial_offset;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->initial_offset != newVal) {
					newDef->initial_offset = (UInt16)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt16>::max(), "%d")) {
				if ((int)newDef->initial_offset != newVal) {
					newDef->initial_offset = (UInt16)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt16>::max());
				if ((int)newDef->initial_offset != newVal) {
					newDef->initial_offset = (UInt16)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@Clg");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_CollisionGroup());
			Byte val = newDef->collision_group;
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				if (io.KeyCtrl)
					ImGui::SetTooltip(theme->generic_ByteHex()[val]);
				else
					ImGui::SetTooltip(theme->generic_ByteBin()[val]);
			}

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::ByteMatrice(rnd, theme, &val, 0b11111111, size * X_COUNT, nullptr, nullptr, nullptr)) {
				if (newDef->collision_group != val) {
					newDef->collision_group = val;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		if (oldDef) {
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			if (*newDef == *oldDef) {
				ImGui::BeginDisabled();
				{
					ImGui::SameLine();
					ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
					ImGui::SameLine();

					ImGui::Dummy(ImVec2(2, 0));

					ImGui::SameLine();
					ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
				}
				ImGui::EndDisabled();
			} else {
				editingBeginAppliable(ws->theme());
				{
					ImGui::SameLine();
					if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
						result = true;
					}
					ImGui::SameLine();
				}
				editingEndAppliable();

				ImGui::Dummy(ImVec2(2, 0));

				ImGui::SameLine();
				if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
					*newDef = *oldDef;
					changed = true;
				}
			}
		} else {
			result = changed;
		}

		const float h = ImGui::GetCursorPosY() - oldY;
		if (height)
			*height = h;

		if (unfolded)
			*unfolded = true;
	} else {
		if (unfolded)
			*unfolded = false;
	}
	ImGui::PopID();

	return result;
}

bool definable(
	Renderer* rnd, Workspace* ws,
	const Actor* ptr,
	const ::active_t* oldDef, ::active_t* newDef,
	bool* unfolded,
	bool* viewAsActor,
	float width, float* height,
	bool* focused,
	const char* prompt,
	bool simplified,
	OperationHandler operate
) {
	if (!viewAsActor || *viewAsActor)
		return definableViewAsActor(rnd, ws, ptr, oldDef, newDef, unfolded, viewAsActor, width, height, focused, prompt, simplified, operate);

	return definableViewAsProjectile(rnd, ws, ptr, oldDef, newDef, unfolded, viewAsActor, width, height, focused, prompt, simplified, operate);
}

bool definable(
	Renderer* rnd, Workspace* ws,
	const Scene* ptr,
	const ::scene_t* oldDef, ::scene_t* newDef,
	bool* unfolded,
	float width, float* height,
	bool* focused,
	const char* prompt,
	OperationHandler operate
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	if (focused)
		*focused = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float dragWidth = size * EDITING_ITEM_COUNT_PER_LINE - (13 + style.FramePadding.x * 2) * 2;
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	const float childWidth = size * X_COUNT;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (unfolded && *unfolded)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::PushID("@Scn");
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		bool changed = false;

		const float oldY = ImGui::GetCursorPosY();
		if (height) {
			const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
			drawList->AddRectFilled(
				curPos + ImVec2(xOffset - 1, 0),
				curPos + ImVec2(xOffset - 1 + size * X_COUNT, *height),
				theme->style()->screenClearingColor
			);
		}

		if (operate) {
			ImGui::PushID("@Opt");
			{
				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();

				const float btnWidth = (childWidth - 2) / 2.0f;

				if (ImGui::Button(theme->dialogPrompt_Copy(), ImVec2(btnWidth + 1, 0.0f))) {
					operate(Operations::COPY);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipScene_CopyDefinition());
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();

				if (ImGui::Button(theme->dialogPrompt_Pst(), ImVec2(btnWidth, 0.0f))) {
					operate(Operations::PASTE);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(theme->tooltipScene_PasteDefinition());
				}
			}
			ImGui::PopID();

			separate(rnd, ws, width);
		}

		ImGui::PushID("@Grv");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Gravity());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->gravity;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->gravity != newVal) {
					newDef->gravity = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%d")) {
				if ((int)newDef->gravity != newVal) {
					newDef->gravity = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->gravity != newVal) {
					newDef->gravity = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@JGr");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_JumpGravity());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->jump_gravity;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->jump_gravity != newVal) {
					newDef->jump_gravity = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%d")) {
				if ((int)newDef->jump_gravity != newVal) {
					newDef->jump_gravity = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->jump_gravity != newVal) {
					newDef->jump_gravity = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@JMC");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_MaxJumpCount());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->jump_max_count;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->jump_max_count != newVal) {
					newDef->jump_max_count = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%d")) {
				if ((int)newDef->jump_max_count != newVal) {
					newDef->jump_max_count = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->jump_max_count != newVal) {
					newDef->jump_max_count = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@JMT");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_MaxJumpTicks());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->jump_max_ticks;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->jump_max_ticks != newVal) {
					newDef->jump_max_ticks = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%d")) {
				if ((int)newDef->jump_max_ticks != newVal) {
					newDef->jump_max_ticks = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->jump_max_ticks != newVal) {
					newDef->jump_max_ticks = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@CVl");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_ClimbVelocity());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			int newVal = (int)newDef->climb_velocity;
			if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::max(newVal - 1, 0);
				if ((int)newDef->climb_velocity != newVal) {
					newDef->climb_velocity = (UInt8)newVal;
					changed = true;
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(dragWidth);
			if (ImGui::DragInt("", &newVal, 1.0f, 0, (int)std::numeric_limits<UInt8>::max(), "%d")) {
				if ((int)newDef->climb_velocity != newVal) {
					newDef->climb_velocity = (UInt8)newVal;
					changed = true;
				}
			}
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
				newVal = Math::min(newVal + 1, (int)std::numeric_limits<UInt8>::max());
				if ((int)newDef->climb_velocity != newVal) {
					newDef->climb_velocity = (UInt8)newVal;
					changed = true;
				}
			}
		}
		ImGui::PopID();

		ImGui::PushID("@CmP");
		{
			const int maxX = ptr->width() * GBBASIC_TILE_SIZE - GBBASIC_SCREEN_WIDTH;
			const int maxY = ptr->height() * GBBASIC_TILE_SIZE - GBBASIC_SCREEN_HEIGHT;

			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_CameraPosition());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@X");
			{
				int newVal = (int)newDef->camera_position.x;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, 0);
					if ((int)newDef->camera_position.x != newVal) {
						newDef->camera_position.x = (UInt16)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, 0, maxX, "X: %d")) {
					if ((int)newDef->camera_position.x != newVal) {
						newDef->camera_position.x = (UInt16)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, maxX);
					if ((int)newDef->camera_position.x != newVal) {
						newDef->camera_position.x = (UInt16)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@Y");
			{
				int newVal = (int)newDef->camera_position.y;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, 0);
					if ((int)newDef->camera_position.y != newVal) {
						newDef->camera_position.y = (UInt16)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, 0, maxY, "Y: %d")) {
					if ((int)newDef->camera_position.y != newVal) {
						newDef->camera_position.y = (UInt16)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, maxY);
					if ((int)newDef->camera_position.y != newVal) {
						newDef->camera_position.y = (UInt16)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();
		}
		ImGui::PopID();

		ImGui::PushID("@CDZ");
		{
			const int maxW = (int)std::numeric_limits<UInt8>::max();
			const int maxH = (int)std::numeric_limits<UInt8>::max();

			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_CameraDeadZone());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@W");
			{
				int newVal = (int)newDef->camera_deadzone.x;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, 0);
					if ((int)newDef->camera_deadzone.x != newVal) {
						newDef->camera_deadzone.x = (UInt8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, 0, maxW, "W: %d")) {
					if ((int)newDef->camera_deadzone.x != newVal) {
						newDef->camera_deadzone.x = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, maxW);
					if ((int)newDef->camera_deadzone.x != newVal) {
						newDef->camera_deadzone.x = (UInt8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushID("@H");
			{
				int newVal = (int)newDef->camera_deadzone.y;
				if (ImGui::ImageButton(theme->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::max(newVal - 1, 0);
					if ((int)newDef->camera_deadzone.y != newVal) {
						newDef->camera_deadzone.y = (UInt8)newVal;
						changed = true;
					}
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(dragWidth);
				if (ImGui::DragInt("", &newVal, 1.0f, 0, maxH, "H: %d")) {
					if ((int)newDef->camera_deadzone.y != newVal) {
						newDef->camera_deadzone.y = (UInt8)newVal;
						changed = true;
					}
				}
				if (ImGui::GetActiveID() == ImGui::GetID("")) {
					if (focused)
						*focused = true;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(theme->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					newVal = Math::min(newVal + 1, maxH);
					if ((int)newDef->camera_deadzone.y != newVal) {
						newDef->camera_deadzone.y = (UInt8)newVal;
						changed = true;
					}
				}
			}
			ImGui::PopID();
		}
		ImGui::PopID();

		bool is16x16grid = newDef->is_16x16_grid;
		ImGui::PushID("@16x16");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Checkbox(theme->dialogPrompt_16x16Grid(), &is16x16grid)) {
				if (newDef->is_16x16_grid != is16x16grid) {
					newDef->is_16x16_grid = is16x16grid;
					changed = true;
				}
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme->tooltip_ForAlignToTileMovementForControllers());
			}
		}
		ImGui::PopID();

		bool is16x16Player = newDef->is_16x16_player;
		ImGui::PushID("@16x16");
		{
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			{
				ImGui::Checkbox(theme->dialogPrompt_16x16Player(), &is16x16Player);
			}
			ImGui::PopStyleColor(2);
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme->tooltip_ForAutoAlignToTileMovementForControllers());
			}
		}
		ImGui::PopID();

		if (oldDef) {
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			if (*newDef == *oldDef) {
				ImGui::BeginDisabled();
				{
					ImGui::SameLine();
					ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
					ImGui::SameLine();

					ImGui::Dummy(ImVec2(2, 0));

					ImGui::SameLine();
					ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
				}
				ImGui::EndDisabled();
			} else {
				editingBeginAppliable(ws->theme());
				{
					ImGui::SameLine();
					if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
						result = true;
					}
					ImGui::SameLine();
				}
				editingEndAppliable();

				ImGui::Dummy(ImVec2(2, 0));

				ImGui::SameLine();
				if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
					*newDef = *oldDef;
					changed = true;
				}
			}
		} else {
			result = changed;
		}

		const float h = ImGui::GetCursorPosY() - oldY;
		if (height)
			*height = h;

		if (unfolded)
			*unfolded = true;
	} else {
		if (unfolded)
			*unfolded = false;
	}
	ImGui::PopID();

	return result;
}

bool definable(
	Renderer*,
	Workspace* ws,
	const Music* ptr,
	Album* newDef,
	bool* unfolded,
	float width, float* height,
	const char* prompt
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconSize();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}
	const float buttonWidth = size * EDITING_ITEM_COUNT_PER_LINE * 0.5f - 1;

	ImGui::Dummy(ImVec2(xOffset - 1, 0));
	ImGui::SameLine();

	const Music::Song* song = ptr->pointer();
	const float childWidth = size * X_COUNT;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (unfolded && *unfolded)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::PushID("@Mus");
	if (ImGui::CollapsingHeader(prompt, childWidth, flags)) {
		bool changed = false;

		const float oldY = ImGui::GetCursorPosY();
		if (height) {
			const ImVec2 curPos = ImGui::GetCursorScreenPos() + ImVec2(1, 1);
			drawList->AddRectFilled(
				curPos + ImVec2(xOffset - 1, 0),
				curPos + ImVec2(xOffset - 1 + size * X_COUNT, *height),
				theme->style()->screenClearingColor
			);
		}

		ImGui::PushID("@Art");
		do {
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Artist());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(size * X_COUNT);
			char buf[ASSETS_NAME_STRING_MAX_LENGTH]; // Fixed size.
			const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, song->artist.length());
			if (n > 0)
				memcpy(buf, song->artist.c_str(), n);
			buf[n] = '\0';
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
			const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_AutoSelectAll);
			ImGui::PopStyleColor();
			if (!edited)
				break;
			newDef->artist = buf;

			changed = true;
		} while (false);
		ImGui::PopID();

		ImGui::PushID("@Cmt");
		do {
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(theme->dialogPrompt_Comment());

			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(size * X_COUNT);
			char buf[ASSETS_NAME_STRING_MAX_LENGTH]; // Fixed size.
			const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, song->comment.length());
			if (n > 0)
				memcpy(buf, song->comment.c_str(), n);
			buf[n] = '\0';
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
			const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_AutoSelectAll);
			ImGui::PopStyleColor();
			if (!edited)
				break;
			newDef->comment = buf;

			changed = true;
		} while (false);
		ImGui::PopID();

		do {
			ImGui::NewLine(1);
			ImGui::Dummy(ImVec2(xOffset, 0));
			if (newDef->artist == song->artist && newDef->comment == song->comment) {
				ImGui::BeginDisabled();
				{
					ImGui::SameLine();
					ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0));
					ImGui::SameLine();

					ImGui::Dummy(ImVec2(2, 0));

					ImGui::SameLine();
					ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0));
				}
				ImGui::EndDisabled();
			} else {
				editingBeginAppliable(ws->theme());
				{
					ImGui::SameLine();
					if (ImGui::Button(theme->generic_Apply(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
						result = true;
					}
					ImGui::SameLine();
				}
				editingEndAppliable();

				ImGui::Dummy(ImVec2(2, 0));

				ImGui::SameLine();
				if (ImGui::Button(theme->generic_Cancel(), ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
					newDef->artist = song->artist;
					newDef->comment = song->comment;
					changed = true;
				}
			}
		} while (false);

		const float h = ImGui::GetCursorPosY() - oldY;
		if (height)
			*height = h;

		if (unfolded)
			*unfolded = true;
	} else {
		if (unfolded)
			*unfolded = false;
	}
	ImGui::PopID();

	return result;
}

}

}

/* ===========================================================================} */
