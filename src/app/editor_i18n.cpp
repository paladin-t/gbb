/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_i18n.h"
#include "editor_i18n.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

constexpr const char* EDITOR_I18N_TABLE_HEADER[256] = {
	/*   1- 10 */  "A",  "B",  "C",  "D",  "E",  "F",  "G",  "H",  "I",  "J",
	/*  11- 20 */  "K",  "L",  "M",  "N",  "O",  "P",  "Q",  "R",  "S",  "T",
	/*  21- 30 */  "U",  "V",  "W",  "X",  "Y",  "Z", "AA", "AB", "AC", "AD",
	/*  31- 40 */ "AE", "AF", "AG", "AH", "AI", "AJ", "AK", "AL", "AM", "AN",
	/*  41- 50 */ "AO", "AP", "AQ", "AR", "AS", "AT", "AU", "AV", "AW", "AX",
	/*  51- 60 */ "AY", "AZ", "BA", "BB", "BC", "BD", "BE", "BF", "BG", "BH",
	/*  61- 70 */ "BI", "BJ", "BK", "BL", "BM", "BN", "BO", "BP", "BQ", "BR",
	/*  71- 80 */ "BS", "BT", "BU", "BV", "BW", "BX", "BY", "BZ", "CA", "CB",
	/*  81- 90 */ "CC", "CD", "CE", "CF", "CG", "CH", "CI", "CJ", "CK", "CL",
	/*  91-100 */ "CM", "CN", "CO", "CP", "CQ", "CR", "CS", "CT", "CU", "CV",
	/* 101-110 */ "CW", "CX", "CY", "CZ", "DA", "DB", "DC", "DD", "DE", "DF",
	/* 111-120 */ "DG", "DH", "DI", "DJ", "DK", "DL", "DM", "DN", "DO", "DP",
	/* 121-130 */ "DQ", "DR", "DS", "DT", "DU", "DV", "DW", "DX", "DY", "DZ",
	/* 131-140 */ "EA", "EB", "EC", "ED", "EE", "EF", "EG", "EH", "EI", "EJ",
	/* 141-150 */ "EK", "EL", "EM", "EN", "EO", "EP", "EQ", "ER", "ES", "ET",
	/* 151-160 */ "EU", "EV", "EW", "EX", "EY", "EZ", "FA", "FB", "FC", "FD",
	/* 161-170 */ "FE", "FF", "FG", "FH", "FI", "FJ", "FK", "FL", "FM", "FN",
	/* 171-180 */ "FO", "FP", "FQ", "FR", "FS", "FT", "FU", "FV", "FW", "FX",
	/* 181-190 */ "FY", "FZ", "GA", "GB", "GC", "GD", "GE", "GF", "GG", "GH",
	/* 191-200 */ "GI", "GJ", "GK", "GL", "GM", "GN", "GO", "GP", "GQ", "GR",
	/* 201-210 */ "GS", "GT", "GU", "GV", "GW", "GX", "GY", "GZ", "HA", "HB",
	/* 211-220 */ "HC", "HD", "HE", "HF", "HG", "HH", "HI", "HJ", "HK", "HL",
	/* 221-230 */ "HM", "HN", "HO", "HP", "HQ", "HR", "HS", "HT", "HU", "HV",
	/* 231-240 */ "HW", "HX", "HY", "HZ", "IA", "IB", "IC", "ID", "IE", "IF",
	/* 241-250 */ "IG", "IH", "II", "IJ", "IK", "IL", "IM", "IN", "IO", "IP",
	/* 251-256 */ "IQ", "IR", "IS", "IT", "IU", "IV"
};
static_assert(GBBASIC_COUNTOF(EDITOR_I18N_TABLE_HEADER) >= I18N_MAX_COLUMN_COUNT, "Wrong data.");
static_assert(IMGUI_TABLE_MAX_COLUMNS >= I18N_MAX_COLUMN_COUNT + 1, "Wrong data.");

/* ===========================================================================} */

/*
** {===========================================================================
** I18n editor
*/

class EditorI18nImpl : public EditorI18n {
private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;

	struct Cursor {
		bool activated = false;
		int row = -1;
		int column = -1;
		char buffer[I18N_MAX_BUFFER_SIZE];

		int wasEditing = 0;
		int lastActiveRow = -1;
		int lastActiveColumn = -1;
		bool inputFieldFocused = false;

		Cursor() {
			memset(buffer, 0, sizeof(buffer));
		}

		void clear(void) {
			row = -1;
			column = -1;
			memset(buffer, 0, sizeof(buffer));
			activated = false;

			wasEditing = 0;
			lastActiveRow = -1;
			lastActiveColumn = -1;
			inputFieldFocused = false;
		}
	} _cursor;
	struct {
		bool selecting = false;
		int mode = 0; // 0 = cell, 1 = row, 2 = column, 3 = all.
		Table::Range brush;
		ImVec2 min = ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		ImVec2 max = ImVec2(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

		void clear(void) {
			selecting = false;
			mode = 0;
			brush = Table::Range();
			min = ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			max = ImVec2(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
		}
		void fill(const ImVec2 &rectMin, const ImVec2 &rectMax) {
			min.x = Math::min(min.x, rectMin.x);
			min.y = Math::min(min.y, rectMin.y);
			max.x = Math::max(max.x, rectMax.x);
			max.y = Math::max(max.y, rectMax.y);
		}
	} _selection;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _page;
	struct {
		std::string info;

		void clear(void) {
			info.clear();
		}
	} _status;
	float _statusWidth = 0.0f;
	struct {
		Text::Array warnings;
		std::string text;

		bool empty(void) const {
			return warnings.empty();
		}
		void clear(void) {
			warnings.clear();
			text.clear();
		}
		bool add(const std::string &txt) {
			if (std::find(warnings.begin(), warnings.end(), txt) != warnings.end())
				return false;

			warnings.push_back(txt);

			flush();

			return true;
		}
		bool remove(const std::string &txt) {
			Text::Array::iterator it = std::find(warnings.begin(), warnings.end(), txt);
			if (it == warnings.end())
				return false;

			warnings.erase(it);

			flush();

			return true;
		}
		void flush(void) {
			text.clear();
			for (int i = 0; i < (int)warnings.size(); ++i) {
				const std::string &w = warnings[i];
				text += "* ";
				text += w;
				if (i != (int)warnings.size() - 1)
					text += "\n";
			}
		}
	} _warnings;
	std::function<void(const Command*)> _refresh = nullptr;
	std::function<void(void)> _checker = nullptr;

	struct Ref : public Editor::Ref {
		void clear(void) {
			// Do nothing.
		}
	} _ref;
	struct Tools {
		int magnification = -1;
		bool magnificationChanged = false;
		std::string namableText;

		bool focused = false;
		bool inputFieldFocused = false;
		bool fineZooming = false;
		bool isActingOnTable = false;
		bool isActingLanguages = false;
		int actingIndex = -1;

		void clear(void) {
			magnification = -1;
			magnificationChanged = false;
			namableText.clear();

			focused = false;
			inputFieldFocused = false;
			fineZooming = false;
			isActingOnTable = false;
			isActingLanguages = false;
			actingIndex = -1;
		}
	} _tools;

public:
	EditorI18nImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::I18n::AddItem>()
			->reg<Commands::I18n::DeleteItem>()
			->reg<Commands::I18n::SwapItems>()
			->reg<Commands::I18n::AddLanguage>()
			->reg<Commands::I18n::DeleteLanguage>()
			->reg<Commands::I18n::SwapLanguages>()
			->reg<Commands::I18n::RenameLanguage>()
			->reg<Commands::I18n::ChangeContent>()
			->reg<Commands::I18n::Cut>()
			->reg<Commands::I18n::Paste>()
			->reg<Commands::I18n::Delete>()
			->reg<Commands::I18n::SetName>()
			->reg<Commands::I18n::Import>();
	}
	virtual ~EditorI18nImpl() override {
		close(_index);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* /* wnd */, class Renderer* /* rnd */, class Workspace* ws, class Project* project, int index, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		int another = -1;
		if (entry()->name.empty() || !_project->canRenameI18n(_index, entry()->name, &another)) {
			if (entry()->name.empty()) {
				const std::string msg = Text::format(ws->theme()->warning_I18nI18nNameIsEmptyAtPage(), { Text::toString(_index) });
				warn(ws, msg, true);
			} else {
				const std::string msg = Text::format(ws->theme()->warning_I18nDuplicateI18nNameAtPages(), { entry()->name, Text::toString(_index), Text::toString(another) });
				warn(ws, msg, true);
			}
			entry()->name = _project->getUsableI18nName(_index); // Unique name.
			_project->hasDirtyAsset(true);
		}

		_refresh = std::bind(&EditorI18nImpl::refresh, this, ws, std::placeholders::_1);

		_checker = [ws, this] (void) -> void {
			// Prepare.
			typedef std::set<int> IndexInfo;
			typedef std::map<std::string, IndexInfo> I18nInfo;

			_warnings.clear();

			if (!object())
				return;

			// Check for too few (zero) columns.
			if (object()->columnCount() < 1) {
				const std::string msg = ws->theme()->warning_I18nTooFewColumns();
				warn(ws, msg, true);

				object()->addLanguage(0, I18N_KEY_COLUMN_NAME); // Add the "key" column.

				_project->hasDirtyAsset(true);
			}

			// Check for missing "key" column.
			const int keyCol = object()->getLanguageIndex(I18N_KEY_COLUMN_NAME);
			if (keyCol == -1) {
				const std::string msg = ws->theme()->warning_I18nMissingKeyColumn();
				warn(ws, msg, true);
			}

			// Check for whether the "key" column is at the beginning.
			if (keyCol != 0) {
				const std::string msg = ws->theme()->warning_I18nTheKeyColumnWasNotAtTheBeginning();
				warn(ws, msg, true);

				for (int i = keyCol; i > 0; --i)
					object()->swapLanguages(i, i - 1);

				_project->hasDirtyAsset(true);
			}

			// Check for column/row count out of bounds.
			if (object()->columnCount() > I18N_MAX_COLUMN_COUNT) {
				const std::string msg = ws->theme()->warning_I18nColumnCountOutOfBounds();
				warn(ws, msg, true);

				while (object()->columnCount() > I18N_MAX_COLUMN_COUNT) {
					if (!object()->deleteLanguage(object()->columnCount() - 1))
						break;
				}

				_project->hasDirtyAsset(true);
			}
			if (object()->rowCount() > I18N_MAX_ROW_COUNT) {
				const std::string msg = ws->theme()->warning_I18nRowCountOutOfBounds();
				warn(ws, msg, true);

				while (object()->rowCount() > I18N_MAX_ROW_COUNT) {
					if (!object()->deleteItem(object()->rowCount() - 1 - 1))
						break;
				}

				_project->hasDirtyAsset(true);
			}

			// Check for duplicate languages.
			I18nInfo i18nInfo;
			IndexInfo processed;
			for (int j = 0; j < object()->columnCount() - 1; ++j) {
				if (processed.find(j) != processed.end())
					continue;

				const char* txt_ = object()->get(j, 0);
				if (!txt_)
					continue;
				const std::string str_ = txt_;

				for (int i = j; i < object()->columnCount(); ++i) {
					if (processed.find(i) != processed.end())
						continue;

					const char* txt = object()->get(i, 0);
					if (!txt)
						continue;

					i18nInfo[txt].insert(i);
				}
				const IndexInfo &idxInfo = i18nInfo[str_];
				if (idxInfo.size() > 1) {
					std::string msg = Text::format(ws->theme()->warning_I18nDuplicateLanguages(), { str_ });
					for (int idx : idxInfo) {
						const std::string detail = idx >= 0 && idx < GBBASIC_COUNTOF(EDITOR_I18N_TABLE_HEADER) ?
							EDITOR_I18N_TABLE_HEADER[idx] :
							Text::toString(idx);
						msg += "\n    ";
						msg += Text::format(ws->theme()->warning_I18nDuplicateLanguages_Detail(), { detail });

						processed.insert(idx);
					}
					warn(ws, msg, true);
				}
			}

			// Check for duplicate "key" names.
			i18nInfo.clear();
			processed.clear();
			for (int j = 1; j < object()->rowCount() - 1; ++j) {
				if (processed.find(j) != processed.end())
					continue;

				const char* txt_ = object()->get(keyCol, j);
				if (!txt_)
					continue;
				const std::string str_ = txt_;

				for (int i = j; i < object()->rowCount(); ++i) {
					if (processed.find(i) != processed.end())
						continue;

					const char* txt = object()->get(keyCol, i);
					if (!txt)
						continue;

					i18nInfo[txt].insert(i);
				}
				const IndexInfo &idxInfo = i18nInfo[str_];
				if (idxInfo.size() > 1) {
					std::string msg = Text::format(ws->theme()->warning_I18nDuplicateKeyNames(), { str_ });
					for (int idx : idxInfo) {
						msg += "\n    ";
						msg += Text::format(ws->theme()->warning_I18nDuplicateKeyNames_Detail(), { Text::toString(idx) });

						processed.insert(idx);
					}
					warn(ws, msg, true);
				}
			}
		};
		_checker();

		_tools.magnification = entry()->magnification;
		_tools.namableText = entry()->name;
		_tools.fineZooming = _project->preferencesFineZooming();

		fprintf(stdout, "I18n editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "I18n editor closed: #%d.\n", _index);

		_project = nullptr;
		_index = -1;

		_cursor.clear();
		_selection.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_checker = nullptr;

		_ref.clear();
		_tools.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		// Do nothing.
	}
	virtual void leave(class Workspace*) override {
		// Do nothing.
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return false;
	}
	virtual void readonly(bool) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return _commands->hasUnsavedChanges();
	}
	virtual void markChangesSaved(void) override {
		_commands->markChangesSaved();
	}
	virtual void prepareForSaving(class Workspace*) override {
		// Do nothing.
	}

	virtual void copy(void) override {
		if (_tools.focused || _tools.inputFieldFocused || _cursor.inputFieldFocused)
			return;

		if (_selection.brush.invalid())
			return;

		auto toString = [] (const I18n::Ptr &i18n, const Table::Range &range) -> std::string {
			const Table::Cursor minc = range.min();
			const Table::Cursor maxc = range.max();

			rapidjson::Document doc;
			Jpath::set(doc, doc, maxc.column - minc.column + 1, "width");
			Jpath::set(doc, doc, maxc.row - minc.row + 1, "height");
			int k = 0;
			for (int r = minc.row; r <= maxc.row; ++r) {
				for (int c = minc.column; c <= maxc.column; ++c) {
					const char* txt = i18n->get(c, r);
					const std::string str = txt ? txt : "";
					Jpath::set(doc, doc, str, "data", k);

					++k;
				}
			}

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		const std::string buf = toString(object(), _selection.brush);
		const std::string osstr = Unicode::toOs(buf);

		Platform::setClipboardText(osstr.c_str());
	}
	virtual void cut(void) override {
		if (_tools.focused || _tools.inputFieldFocused || _cursor.inputFieldFocused)
			return;

		if (_selection.brush.invalid())
			return;

		copy();

		Command* cmd = enqueue<Commands::I18n::Cut>()
			->with(
				[this] (int col, int row) -> const char* {
					return object()->get(col, row);
				},
				[this] (int col, int row, const std::string &val) -> bool {
					return object()->set(col, row, val);
				}
			)
			->with(_selection.brush)
			->exec(object());

		_refresh(cmd);

		_selection.clear();
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (_tools.focused || _tools.inputFieldFocused || _cursor.inputFieldFocused)
			return;

		auto fromString = [] (const I18n::Ptr &/* i18n */, const std::string &buf, int &width_, int &height_, Text::Array &content) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int width = -1, height = -1;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;

			width_ = width;
			height_ = height;

			int k = 0;
			for (int c = 0; c < width; ++c) {
				for (int r = 0; r < height; ++r) {
					std::string str;
					if (!Jpath::get(doc, str, "data", k))
						str = "";
					content.push_back(str);

					++k;
				}
			}

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		int width = 0;
		int height = 0;
		Text::Array content;
		if (!fromString(object(), buf, width, height, content) || width == 0 || height == 0)
			return;

		Table::Range brush;
		if (!_selection.brush.invalid() || (_cursor.lastActiveRow == -1 && _cursor.lastActiveColumn == -1)) {
			int startRow = _selection.brush.first.row;
			int startCol = _selection.brush.first.column;
			if (startRow < 0)
				startRow = 0;
			if (startCol < 0)
				startCol = 0;
			brush.start(_selection.brush.first);
			brush.end(startRow + height - 1, startCol + width - 1);
		} else {
			int startRow = _cursor.lastActiveRow;
			int startCol = _cursor.lastActiveColumn;
			if (startRow < 0)
				startRow = 0;
			if (startCol < 0)
				startCol = 0;
			brush.start(startRow, startCol);
			brush.end(startRow + height - 1, startCol + width - 1);
		}

		Command* cmd = enqueue<Commands::I18n::Paste>()
			->with(
				[this] (int col, int row) -> const char* {
					return object()->get(col, row);
				},
				[this] (int col, int row, const std::string &val) -> bool {
					return object()->set(col, row, val);
				}
			)
			->with(content)
			->with(brush)
			->exec(object());

		_refresh(cmd);
	}
	virtual void del(bool) override {
		if (_tools.focused || _tools.inputFieldFocused || _cursor.inputFieldFocused)
			return;

		if (_selection.brush.invalid())
			return;

		Command* cmd = enqueue<Commands::I18n::Delete>()
			->with(
				[this] (int col, int row) -> const char* {
					return object()->get(col, row);
				},
				[this] (int col, int row, const std::string &val) -> bool {
					return object()->set(col, row, val);
				}
			)
			->with(_selection.brush)
			->exec(object());

		_refresh(cmd);

		_selection.clear();
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}
	virtual const char* undoable(void) const override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}

	virtual void redo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return;

		_commands->redo(object());

		_refresh(cmd);

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		_commands->undo(object());

		_refresh(cmd);

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case SELECT_ALL: {
				const int cols = object()->columnCount();
				const int rows = object()->rowCount();
				if (rows == 1)
					return Variant(true);

				if (cols > 0 && rows > 0) {
					_selection.brush.start(1, 0);
					_selection.brush.end(rows - 1, cols - 1);
				}
			}

			return Variant(true);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		if (!entry() || !object()) {
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			ImGui::EndChild();
			refreshStatus(wnd, rnd, ws);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}

		constexpr const int MAGNIFICATIONS[] = {
			1, 2, 3, 4
		};
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
		{
			constexpr const float SUGGESTED_CELL_WIDTH = 70.0f;
			if (_tools.magnification == -1) {
				const float PADDING = 32.0f;
				const float s = (splitter.first + PADDING) / (object()->languageCount() + 1);
				int m = 0;
				for (int i = 0; i < GBBASIC_COUNTOF(MAGNIFICATIONS); ++i) {
					if (s > SUGGESTED_CELL_WIDTH * MAGNIFICATIONS[i])
						m = i;
				}
				_tools.magnification = m;
			}
			if (_tools.fineZooming) {
				_tools.magnification = Math::clamp(_tools.magnification, MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1]);
			} else {
				_tools.magnification = Math::clamp(_tools.magnification, 0, (int)GBBASIC_COUNTOF(MAGNIFICATIONS) - 1);
			}

			const float lineNoWidth = SUGGESTED_CELL_WIDTH;
			float cellWidth = SUGGESTED_CELL_WIDTH;
			if (_tools.fineZooming) {
				cellWidth *= (float)_tools.magnification;
			} else {
				cellWidth *= (float)(MAGNIFICATIONS[_tools.magnification]);
			}

			/*const float tblWidth = (lineNoWidth + style.FramePadding.x * 2) + (cellWidth + style.FramePadding.x * 2) * cols;
			const float tblHeight = (ImGui::GetTextLineHeight() + style.CellPadding.y * 2) * (rows + 1);*/
			const float tblWidth = splitter.first;
			const float tblHeight = height - statusBarHeight;
			updateTable(rnd, ws, tblWidth, tblHeight, lineNoWidth, cellWidth);

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			bool inputFieldFocused = false;
			bool inputFieldFocused_ = false;
			auto canUseShortcuts = [ws, this] (void) -> bool {
				return !_tools.inputFieldFocused && !_cursor.inputFieldFocused && ws->canUseShortcuts() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
			};

			const float spwidth = _ref.windowWidth(splitter.second);

			ImGui::NewLine(1);
			if (_tools.fineZooming) {
				if (
					Editing::Tools::magnifiable(
						rnd, ws,
						&_tools.magnification,
						MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1],
						spwidth, canUseShortcuts(),
						&inputFieldFocused_
					)
				) {
					_tools.magnificationChanged = true;
				}
			} else {
				if (Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, spwidth, canUseShortcuts()))
					_tools.magnificationChanged = true;
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::namable(
					rnd, ws,
					entry()->name, _tools.namableText,
					spwidth,
					nullptr, &inputFieldFocused_,
					ws->theme()->dialogPrompt_Name().c_str()
				)
			) {
				if (_project->canRenameTiles(_index, _tools.namableText, nullptr)) {
					Command* cmd = enqueue<Commands::I18n::SetName>()
						->with(_tools.namableText)
						->exec(object());

					_refresh(cmd);
				} else {
					_tools.namableText = entry()->name;

					warn(ws, ws->theme()->warning_TilesNameIsAlreadyInUse(), true);
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			_tools.inputFieldFocused = inputFieldFocused;

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		refreshStatus(wnd, rnd, ws);
		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;
	}

	virtual void added(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}
	virtual void removed(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}

	virtual void played(class Renderer*, class Workspace*) override {
		// Do nothing.
	}
	virtual void stopped(class Renderer*, class Workspace*) override {
		// Do nothing.
	}

	virtual void resized(class Renderer*, const Math::Vec2i &, const Math::Vec2i &) override {
		// Do nothing.
	}

	virtual void focusLost(class Renderer*) override {
		// Do nothing.
	}
	virtual void focusGained(class Renderer*) override {
		// Do nothing.
	}

private:
	void updateTable(Renderer* rnd, Workspace* ws, float tblWidth, float tblHeight, float lineNoWidth, float cellWidth) {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

		const ImVec4 tblHeaderBg = ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, tblHeaderBg);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, tblHeaderBg);

		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
		if (_tools.magnificationChanged)
			flags |= ImGuiTableFlags_NoHostExtendX;
		else
			flags |= ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
		const int cols = Math::min(object()->columnCount(), I18N_MAX_COLUMN_COUNT);
		const int rows = Math::min(object()->rowCount(), I18N_MAX_ROW_COUNT);
		const int finalCols = Math::min(cols + 1 /* row number */ + 1 /* extra */, IMGUI_TABLE_MAX_COLUMNS);
		const ImVec2 btnSize(ImGui::GetTextLineHeight() + style.CellPadding.y * 2, ImGui::GetTextLineHeight() + style.CellPadding.y * 2 - 1);
		bool inputFieldFocused = false;
		if (ImGui::BeginTable("@Tbl", finalCols, flags, ImVec2(tblWidth, tblHeight))) {
			const Editing::Shortcut tab(SDL_SCANCODE_TAB);
			bool tabbed = false;
			const Editing::Shortcut enter(SDL_SCANCODE_RETURN);
			bool entered = false;
			const ImGui::Rect &tblRect = ImGui::GetTableInnerClipRect();

			ImGui::TableSetupScrollFreeze(1, 2);
			const ImU32 col = ws->theme()->style()->i18nHeadColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			if (_tools.magnificationChanged) {
				_tools.magnificationChanged = false;
				for (int i = 0; i < finalCols; ++i) {
					if (i == 0)
						ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, lineNoWidth);
					/*else if (i == finalCols - 1)
						ImGui::TableSetupColumn(EDITOR_I18N_TABLE_HEADER[i - 1], ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, cellWidth);*/
					else
						ImGui::TableSetupColumn(EDITOR_I18N_TABLE_HEADER[i - 1], ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, cellWidth);
				}
			} else {
				for (int i = 0; i < finalCols; ++i) {
					if (i == 0)
						ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, lineNoWidth);
					/*else if (i == finalCols - 1)
						ImGui::TableSetupColumn(EDITOR_I18N_TABLE_HEADER[i - 1], ImGuiTableColumnFlags_WidthFixed, cellWidth);*/
					else
						ImGui::TableSetupColumn(EDITOR_I18N_TABLE_HEADER[i - 1], ImGuiTableColumnFlags_WidthFixed, cellWidth);
				}
			}
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			// Column selection via header row.
			if (rows >= 2) {
				const float headerTop = ImGui::GetTableRowPosY1();
				const float headerBottom = ImGui::GetTableRowPosY2();
				const ImVec2 mousePos = ImGui::GetMousePos();
				if (mousePos.y >= headerTop && mousePos.y <= headerBottom) {
					for (int col = 0; col < finalCols; ++col) {
						const float colMinX = ImGui::GetTableColumnMinX(col);
						const float colMaxX = ImGui::GetTableColumnMaxX(col);
						if (mousePos.x >= colMinX && mousePos.x <= colMaxX) {
							const ImVec2 rectMin(colMinX, headerTop);
							const ImVec2 rectMax(colMaxX, headerBottom);
							if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::HasPopup()) {
								if (_cursor.row != -1)
									commitCell(ws);
								if (col == 0) { // First cell, select all.
									_selection.selecting = true;
									_selection.mode = 3; // All.
									const int cols = object()->columnCount();
									const int rows = object()->rowCount();
									if (cols > 0 && rows > 1) {
										_selection.brush.start(1, 0);
										_selection.brush.end(rows - 1, cols - 1);
									}
									_selection.fill(rectMin, rectMax);
								} else {
									_selection.selecting = true;
									_selection.mode = 2; // Column.
									const int dataCol = col - 1;
									_selection.brush.start(1, dataCol);
									_selection.brush.end(rows - 1, dataCol);
									_selection.fill(rectMin, rectMax);
								}
							}
							if (_selection.selecting && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
								if (_selection.mode == 2) {
									const int dataCol = col - 1;
									if (dataCol != _selection.brush.second.column) {
										_selection.brush.end(rows - 1, dataCol);
										_selection.fill(rectMin, rectMax);
									}
								} else if (_selection.mode == 3) {
									// Do nothing.
								}
							}

							break;
						}
					}
				}
				if (_selection.selecting && _selection.mode == 2 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					_selection.selecting = false;
				}
			}

			for (int row = 0; row < rows; ++row) {
				ImGui::PushID(row);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				const ImVec2 rowCellMin = ImGui::GetCursorScreenPos();
				if (row == 0) {
					// "Languages" for row 0.
					const ImU32 col = ws->theme()->style()->i18nHeadColor;
					ImGui::PushStyleColor(ImGuiCol_Text, col);
					{
						ImGui::TextUnformatted(ws->theme()->dialogPrompt_Languages());
					}
					ImGui::PopStyleColor();
				} else {
					// Row number.
					const ImU32 col = ws->theme()->style()->i18nHeadColor;
					ImGui::PushStyleColor(ImGuiCol_Text, col);
					{
						ImGui::Text("%d", row);
					}
					ImGui::PopStyleColor();

					ImGui::SameLine();
					const float x = ImGui::GetCursorScreenPos().x
						+ ImGui::GetColumnWidth(0) - btnSize.x + style.CellPadding.x;
					const float y = ImGui::GetCursorScreenPos().y
						- style.CellPadding.y + 1;
					if (ImGui::CompactButton(ws->theme()->iconTableDropdown()->pointer(rnd), ImVec2(x, y), btnSize, ImVec4(1, 1, 1, 1))) {
						_tools.isActingOnTable = true;
						_tools.isActingLanguages = false;
						_tools.actingIndex = row;
					}

					// Row selection via row number column.
					const ImVec2 btnRectMin(x, y);
					const ImVec2 btnRectMax(x + btnSize.x, y + btnSize.y);
					if (!ImGui::IsMouseHoveringRect(btnRectMin, btnRectMax)) {
						const float rowColWidth = ImGui::GetColumnWidth(0);
						const ImVec2 rowRectMin(rowCellMin.x - style.CellPadding.x, rowCellMin.y - style.CellPadding.y);
						const ImVec2 rowRectMax(rowCellMin.x + rowColWidth + style.CellPadding.x, rowCellMin.y + ImGui::GetTextLineHeight() + style.CellPadding.y);
						const bool rowHovered = ImGui::IsMouseHoveringRect(rowRectMin, rowRectMax);
						if (rowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::HasPopup()) {
							if (_cursor.row != -1)
								commitCell(ws);
							_selection.selecting = true;
							_selection.mode = 1; // Row.
							_selection.brush.start(row, 0);
							_selection.brush.end(row, cols - 1);
							_selection.fill(rowRectMin, rowRectMax);
						}
						if (_selection.selecting && _selection.mode == 1 && rowHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
							if (row != _selection.brush.second.row) {
								_selection.brush.end(row, cols - 1);
								_selection.fill(rowRectMin, rowRectMax);
							}
						}
					}
				}

				for (int col = 0; col < cols; ++col) {
					ImGui::PushID(col);

					ImGui::TableSetColumnIndex(col + 1);
					const float colWidth = ImGui::GetColumnWidth(col);
					const char* txt = object()->get(col, row);
					if (_cursor.row == row && _cursor.column == col) {
						VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, style.CellPadding);

						ImGui::SetNextItemWidth(colWidth + style.CellPadding.x * 2);
						const float x = ImGui::GetCursorScreenPos().x
							- style.CellPadding.x;
						const float y = ImGui::GetCursorScreenPos().y
							- style.CellPadding.y;
						ImGui::SetCursorScreenPos(ImVec2(x, y));
						if (!_cursor.activated) {
							ImGui::SetKeyboardFocusHere(0);
							_cursor.activated = true;
						}
						const ImGui::ItemSizeData data = ImGui::ReserveItemSizeData();
						{
							// Edit content cells.
							if (ImGui::InputText("##Ed", _cursor.buffer, sizeof(_cursor.buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
								if (enter.pressed() && !entered) {
									entered = true;
									if (commitCell(ws)) {
										if (row + 1 <= rows)
											editCell(ws, row + 1, col);
									}
								} else {
									commitCell(ws);
								}
							} else if (_cursor.activated && ImGui::IsItemDeactivated()) {
								commitCell(ws);
							} else if (tab.pressed() && !tabbed) {
								tabbed = true;
								commitCell(ws);
								if (col + 1 < cols)
									editCell(ws, row, col + 1);
							}

							if (ImGui::GetActiveID() == ImGui::GetID("##Ed"))
								inputFieldFocused |= true;
						}
						ImGui::RestoreItemSizeData(data);
					} else {
						// Cell content.
						const ImVec2 cellMin = ImGui::GetCursorScreenPos();
						if (row == 0) {
							const ImU32 col = ws->theme()->style()->i18nHeadColor;
							ImGui::PushStyleColor(ImGuiCol_Text, col);
							{
								ImGui::TextUnformatted(txt ? txt : "");
							}
							ImGui::PopStyleColor();
						} else {
							ImGui::TextUnformatted(txt ? txt : "");
						}
						bool dropdownHovered = false;
						if (row == 0 && col >= 1) {
							ImGui::SameLine();
							const float x = ImGui::GetCursorScreenPos().x
								+ ImGui::GetColumnWidth(0) - btnSize.x + style.CellPadding.x;
							const float y = ImGui::GetCursorScreenPos().y
								- style.CellPadding.y + 1;
							if (ImGui::CompactButton(ws->theme()->iconTableDropdown()->pointer(rnd), ImVec2(x, y), btnSize, ImVec4(1, 1, 1, 1))) {
								_tools.isActingOnTable = true;
								_tools.isActingLanguages = true;
								_tools.actingIndex = col;
								dropdownHovered = true;
							}
							const ImVec2 rectMin(x, y);
							const ImVec2 rectMax(x + btnSize.x, y + btnSize.y);
							if (ImGui::IsMouseHoveringRect(rectMin, rectMax)) {
								dropdownHovered = true;
							}
						}
						if (!dropdownHovered) {
							const ImVec2 rectMin(cellMin.x - style.CellPadding.x, cellMin.y - style.CellPadding.y);
							const ImVec2 rectMax(cellMin.x + colWidth + style.CellPadding.x, cellMin.y + ImGui::GetTextLineHeight() + style.CellPadding.y);
							const bool hovered = ImGui::IsMouseHoveringRect(rectMin, rectMax - ImVec2(4, 0));
							if (
								hovered && (
									(ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::HasPopup()) ||
									(ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
										(_selection.brush.invalid() || _selection.brush.first == _selection.brush.second))
								)
							) {
								if (_cursor.row != -1)
									commitCell(ws);

								if (row == 0)
									editCell(ws, row, col);

								// Begin selecting.
								if (row != 0) {
									_selection.selecting = true;
									_selection.mode = 0; // Cell.
									_selection.brush.start(row, col);
									_selection.brush.end(row, col);
									_selection.fill(rectMin, rectMax);
								}
							}
							if (_selection.selecting && _selection.mode == 0 && hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
								// Drag selection.
								if (row != 0 && (row != _selection.brush.second.row || col != _selection.brush.second.column)) {
									_selection.brush.end(row, col);
									_selection.fill(rectMin, rectMax);
								}
							}
							if (_selection.selecting && _selection.mode == 0 && hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
								// End selecting.
								_selection.selecting = false;
								if (_selection.brush.single()) {
									//_selection.clear();

									if (_cursor.row != -1 || _cursor.wasEditing || (_cursor.lastActiveRow == row && _cursor.lastActiveColumn == col)) {
										editCell(ws, row, col);
									} else {
										_cursor.lastActiveRow = row;
										_cursor.lastActiveColumn = col;
									}
								}
							}
							if (!_selection.brush.invalid()) {
								const Table::Cursor minic = _selection.brush.min();
								const Table::Cursor maxc = _selection.brush.max();
								if (row >= minic.row && row <= maxc.row && col >= minic.column && col <= maxc.column) {
									ImDrawList* drawList = ImGui::GetWindowDrawList();

									drawList->AddRectFilled(rectMin + ImVec2(0, 1), rectMax, 0x40808080);
									drawList->AddRect(rectMin + ImVec2(0, 1), rectMax, ImGui::GetColorU32(ImGuiCol_NavHighlight));
								}
							}
						}
					}

					ImGui::PopID();
				}

				ImGui::TableSetColumnIndex(finalCols - 1);
				const float colWidth = ImGui::GetColumnWidth(_cursor.column);
				if (_cursor.row == row && _cursor.column == cols) {
					VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, style.CellPadding);

					ImGui::SetNextItemWidth(colWidth + style.CellPadding.x * 2);
					const float x = ImGui::GetCursorScreenPos().x
						- style.CellPadding.x;
					const float y = ImGui::GetCursorScreenPos().y
						- style.CellPadding.y;
					ImGui::SetCursorScreenPos(ImVec2(x, y));
					if (!_cursor.activated) {
						ImGui::SetKeyboardFocusHere(0);
						_cursor.activated = true;
					}
					const ImGui::ItemSizeData data = ImGui::ReserveItemSizeData();
					{
						// Edit new language column.
						if (ImGui::InputText("##Ed", _cursor.buffer, sizeof(_cursor.buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
							if (enter.pressed() && !entered) {
								entered = true;
								if (commitCell(ws)) {
									if (row + 1 < rows)
										editCell(ws, row + 1, col);
								}
							} else {
								commitCell(ws);
							}
						} else if (_cursor.activated && ImGui::IsItemDeactivated()) {
							commitCell(ws);
						} else if (tab.pressed() && !tabbed) {
							tabbed = true;
							const int col_ = _cursor.column;
							commitCell(ws);
							if (col_ + 1 < cols)
								editCell(ws, row, col_ + 1);
						}

						if (ImGui::GetActiveID() == ImGui::GetID("##Ed"))
							inputFieldFocused |= true;
					}
					ImGui::RestoreItemSizeData(data);
				} else {
					if (cols < I18N_MAX_COLUMN_COUNT) {
						// Placeholder for new language.
						const ImVec2 cellMin = ImGui::GetCursorScreenPos();
						if (row == 0) {
							const ImU32 col = ws->theme()->style()->i18nHeadColor;
							ImVec4 col_ = ImGui::ColorConvertU32ToFloat4(col);
							col_.w *= 0.55f;
							ImGui::PushStyleColor(ImGuiCol_Text, col_);
							{
								ImGui::TextUnformatted(ws->theme()->dialogPrompt_ClickToEdit());
							}
							ImGui::PopStyleColor();
						} else {
							ImVec4 col_ = ImGui::GetStyleColorVec4(ImGuiCol_Text);
							col_.w *= 0.55f;
							ImGui::PushStyleColor(ImGuiCol_Text, col_);
							{
								ImGui::TextUnformatted("-");
							}
							ImGui::PopStyleColor();
						}
						const ImVec2 rectMin(cellMin.x - style.CellPadding.x, cellMin.y - style.CellPadding.y);
						const ImVec2 rectMax(cellMin.x + colWidth + style.CellPadding.x, cellMin.y + ImGui::GetTextLineHeight() + style.CellPadding.y);
						if (ImGui::IsMouseHoveringRect(rectMin, rectMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
							if (_cursor.row != -1)
								commitCell(ws);
							editCell(ws, row, cols);
						}
					}
				}

				ImGui::PopID();
			}
			if (_selection.selecting && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				_selection.selecting = false;
			}
			if (!_selection.selecting && ImGui::IsMouseHoveringRect(tblRect.first, tblRect.second, false) && !ImGui::IsMouseHoveringRect(_selection.min, _selection.max) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::HasPopup()) {
				_selection.clear();
			}
			if (!_selection.selecting && _selection.brush.invalid() && ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE, false) && !ImGui::HasPopup()) {
				_selection.clear();
			}

			if (rows < I18N_MAX_ROW_COUNT) {
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				{
					// Row number for the last line.
					const ImU32 col = ws->theme()->style()->i18nHeadColor;
					ImGui::PushStyleColor(ImGuiCol_Text, col);
					{
						ImGui::Text("%d", rows);
					}
					ImGui::PopStyleColor();
				}

				for (int col = 0; col < cols; ++col) {
					ImGui::TableSetColumnIndex(col + 1);
					const float colWidth = ImGui::GetColumnWidth(col);
					if (_cursor.row == rows && _cursor.column == col) {
						VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, style.CellPadding);

						ImGui::SetNextItemWidth(colWidth + style.CellPadding.x * 2);
						const float x = ImGui::GetCursorScreenPos().x
							- style.CellPadding.x;
						const float y = ImGui::GetCursorScreenPos().y
							- style.CellPadding.y + 1;
						ImGui::SetCursorScreenPos(ImVec2(x, y));
						if (!_cursor.activated) {
							ImGui::SetKeyboardFocusHere(0);
							_cursor.activated = true;
						}
						const ImGui::ItemSizeData data = ImGui::ReserveItemSizeData();
						{
							// Edit new item row.
							if (ImGui::InputText("##Ed", _cursor.buffer, sizeof(_cursor.buffer), ImGuiInputTextFlags_EnterReturnsTrue))
								commitCell(ws);
							else if (_cursor.activated && ImGui::IsItemDeactivated())
								commitCell(ws);

							if (ImGui::GetActiveID() == ImGui::GetID("##Ed"))
								inputFieldFocused |= true;
						}
						ImGui::RestoreItemSizeData(data);
					} else {
						// Placeholder for new item.
						if (rows < I18N_MAX_ROW_COUNT) {
							const ImVec2 cellMin = ImGui::GetCursorScreenPos();
							ImVec4 col_ = ImGui::GetStyleColorVec4(ImGuiCol_Text);
							col_.w *= 0.55f;
							ImGui::PushStyleColor(ImGuiCol_Text, col_);
							{
								if (col == 0)
									ImGui::TextUnformatted(ws->theme()->dialogPrompt_ClickToEdit());
								else
									ImGui::TextUnformatted("-");
							}
							ImGui::PopStyleColor();
							const ImVec2 rectMin(cellMin.x - style.CellPadding.x, cellMin.y - style.CellPadding.y);
							const ImVec2 rectMax(cellMin.x + colWidth + style.CellPadding.x, cellMin.y + ImGui::GetTextLineHeight() + style.CellPadding.y);
							if (ImGui::IsMouseHoveringRect(rectMin, rectMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
								if (_cursor.row != -1)
									commitCell(ws);
								editCell(ws, rows, col);
							}
						}
					}
				}
			}

			if (inputFieldFocused)
				_cursor.wasEditing = 5;
			else if (_cursor.wasEditing > 0)
				--_cursor.wasEditing;

			ImGui::EndTable();
		}

		ImGui::PopStyleColor(2);

		_cursor.inputFieldFocused = inputFieldFocused;
	}
	bool editCell(Workspace* ws, int row, int col) {
		if (ws->popupBox() || ImGui::HasPopup())
			return false;

		if (row != 0 && col == object()->columnCount()) // Clicked the last column, but not the languages row.
			return false;
		if (col != 0 && row == object()->rowCount()) // Clicked the last row, but not the key column.
			return false;

		_cursor.activated = false;
		_cursor.row = row;
		_cursor.column = col;
		memset(_cursor.buffer, 0, sizeof(_cursor.buffer));

		if (row >= 0 && row < object()->rowCount() && col >= 0 && col < object()->columnCount()) {
			const char* txt = object()->get(col, row);
			if (txt) {
				size_t n = strlen(txt);
				if (n >= sizeof(_cursor.buffer))
					n = sizeof(_cursor.buffer) - 1;
				memcpy(_cursor.buffer, txt, n);
				_cursor.buffer[n] = '\0';
			}
		}

		return true;
	}
	bool commitCell(Workspace* ws) {
		const int row = _cursor.row;
		const int col = _cursor.column;
		std::string txt(_cursor.buffer);

		_cursor.activated = false;
		_cursor.row = -1;
		_cursor.column = -1;

		const int rows = object()->rowCount();
		const int cols = object()->columnCount();

		if (row < 0 || col < 0)
			return false;

		const int keyCol = object()->getLanguageIndex(I18N_KEY_COLUMN_NAME);
		if (row == rows || col == cols || row == 0 || col == keyCol)
			txt = Text::trim(txt);

		if (row == rows) {
			if (txt.empty())
				return false;

			if (rows >= I18N_MAX_ROW_COUNT) {
				ws->bubble(ws->theme()->dialogPrompt_CannotAddMoreItems(), nullptr);

				return false;
			}
			if (object()->getItemIndex(txt) != -1) {
				ws->bubble(ws->theme()->dialogPrompt_ItemAlreadyExists(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::I18n::AddItem>()
				->with(rows - 1, txt)
				->exec(object());

			_refresh(cmd);
		} else if (col == cols) {
			if (txt.empty())
				return false;

			if (cols >= I18N_MAX_COLUMN_COUNT) {
				ws->bubble(ws->theme()->dialogPrompt_CannotAddMoreLanguages(), nullptr);

				return false;
			}
			if (object()->getLanguageIndex(txt) != -1) {
				ws->bubble(ws->theme()->dialogPrompt_LanguageAlreadyExists(), nullptr);

				return false;
			}

			Command* cmd = enqueue<Commands::I18n::AddLanguage>()
				->with(cols, txt)
				->exec(object());

			_refresh(cmd);
		} else if (row == 0) {
			if (txt.empty())
				return false;

			const char* txt_ = object()->get(col, row);
			if (txt_ && txt == txt_) // Not changed.
				return true; // Ok.

			Command* cmd = enqueue<Commands::I18n::RenameLanguage>()
				->with(col, txt)
				->exec(object());

			_refresh(cmd);
		} else {
			if (txt.empty())
				return false;

			const char* txt_ = object()->get(col, row);
			if (txt_ && txt == txt_) // Not changed.
				return true; // Ok.

			Command* cmd = enqueue<Commands::I18n::ChangeContent>()
				->with(col, row - 1, txt)
				->exec(object());

			_refresh(cmd);
		}

		return true; // Ok.
	}

	bool shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
		if (_tools.inputFieldFocused || _cursor.inputFieldFocused || !ws->canUseShortcuts()) {
			return true;
		}

#if defined GBBASIC_OS_APPLE
		const Editing::Shortcut pgUp(SDL_SCANCODE_PAGEUP, false, false, false, false, false, true);
		const Editing::Shortcut pgDown(SDL_SCANCODE_PAGEDOWN, false, false, false, false, false, true);
#else /* GBBASIC_OS_APPLE */
		const Editing::Shortcut pgUp(SDL_SCANCODE_PAGEUP, true);
		const Editing::Shortcut pgDown(SDL_SCANCODE_PAGEDOWN, true);
#endif /* GBBASIC_OS_APPLE */
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (pgUp.pressed()) {
				int index = _index - 1;
				if (index < 0)
					index = _project->i18nPageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->i18nPageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, index);
			}
		}

		return true;
	}

	bool context(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (!(_tools.inputFieldFocused || _cursor.inputFieldFocused) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		bool result = false;
		if (ImGui::BeginPopup("@Ctx")) {
			if (ImGui::MenuItem(ws->theme()->menu_Cut())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Copy())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
				del(false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
				post(Editable::SELECT_ALL);
			}

			ImGui::EndPopup();

			result = true;
		}

		if (_tools.isActingOnTable) {
			_tools.isActingOnTable = false;

			ImGui::OpenPopup("@Act");

			ws->bubble(nullptr);
		}

		if (ImGui::BeginPopup("@Act")) {
			const int idx = _tools.actingIndex;

			if (_tools.isActingLanguages) {
				const int cols = object()->columnCount();

				if (ImGui::MenuItem(ws->theme()->menu_Add())) {
					if (cols >= I18N_MAX_COLUMN_COUNT) {
						ws->bubble(ws->theme()->dialogPrompt_CannotAddMoreLanguages(), nullptr);
					} else {
						ImGui::InputPopupBox::ConfirmedHandler confirm(
							[ws, this, idx] (const char* txt) -> void {
								if (!txt || !*txt) {
									ws->popupBox(nullptr);

									return;
								}
								if (object()->getLanguageIndex(txt) != -1) {
									ws->bubble(ws->theme()->dialogPrompt_LanguageAlreadyExists(), nullptr);

									ws->popupBox(nullptr);

									return;
								}

								Command* cmd = enqueue<Commands::I18n::AddLanguage>()
									->with(idx, txt)
									->exec(object());

								_refresh(cmd);

								ws->popupBox(nullptr);
							},
							nullptr
						);
						ImGui::InputPopupBox::CanceledHandler cancel(
							[ws] (void) -> void {
								ws->popupBox(nullptr);
							},
							nullptr
						);
						ws->inputPopupBox(
							ws->theme()->dialogInput_Input(),
							"", 0,
							confirm,
							cancel
						);
					}
				}
				if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
					Command* cmd = enqueue<Commands::I18n::DeleteLanguage>()
						->with(idx)
						->exec(object());

					_refresh(cmd);
				}
				ImGui::Separator();
				if (ImGui::MenuItem(ws->theme()->menu_MoveLeft(), nullptr, nullptr, idx > 1)) {
					Command* cmd = enqueue<Commands::I18n::SwapLanguages>()
						->with(idx, idx - 1)
						->exec(object());

					_refresh(cmd);
				}
				if (ImGui::MenuItem(ws->theme()->menu_MoveRight(), nullptr, nullptr, idx < cols - 1)) {
					Command* cmd = enqueue<Commands::I18n::SwapLanguages>()
						->with(idx, idx + 1)
						->exec(object());

					_refresh(cmd);
				}
			} else {
				const int rows = object()->rowCount();

				if (ImGui::MenuItem(ws->theme()->menu_Add())) {
					if (rows >= I18N_MAX_ROW_COUNT) {
						ws->bubble(ws->theme()->dialogPrompt_CannotAddMoreItems(), nullptr);
					} else {
						ImGui::InputPopupBox::ConfirmedHandler confirm(
							[ws, this, idx] (const char* txt) -> void {
								if (!txt || !*txt) {
									ws->popupBox(nullptr);

									return;
								}
								if (object()->getItemIndex(txt) != -1) {
									ws->bubble(ws->theme()->dialogPrompt_ItemAlreadyExists(), nullptr);

									ws->popupBox(nullptr);

									return;
								}

								Command* cmd = enqueue<Commands::I18n::AddItem>()
									->with(idx - 1, txt)
									->exec(object());

								_refresh(cmd);

								ws->popupBox(nullptr);
							},
							nullptr
						);
						ImGui::InputPopupBox::CanceledHandler cancel(
							[ws] (void) -> void {
								ws->popupBox(nullptr);
							},
							nullptr
						);
						ws->inputPopupBox(
							ws->theme()->dialogInput_Input(),
							"", 0,
							confirm,
							cancel
						);
					}
				}
				if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
					Command* cmd = enqueue<Commands::I18n::DeleteItem>()
						->with(idx - 1)
						->exec(object());

					_refresh(cmd);
				}
				ImGui::Separator();
				if (ImGui::MenuItem(ws->theme()->menu_MoveUp(), nullptr, nullptr, idx > 1)) {
					Command* cmd = enqueue<Commands::I18n::SwapItems>()
						->with(idx, idx - 1)
						->exec(object());

					_refresh(cmd);
				}
				if (ImGui::MenuItem(ws->theme()->menu_MoveDown(), nullptr, nullptr, idx < rows - 1)) {
					Command* cmd = enqueue<Commands::I18n::SwapItems>()
						->with(idx, idx + 1)
						->exec(object());

					_refresh(cmd);
				}
			}

			ImGui::EndPopup();
		}

		return result;
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height, bool actived) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (actived || EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			const ImVec2 pos = ImGui::GetCursorPos();
			ImGui::Dummy(
				ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize),
				ImGui::GetStyleColorVec4(ImGuiCol_Button)
			);
			ImGui::SetCursorPos(pos);
		}

		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		}
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		if (ImGui::Button("<", ImVec2(0, height))) {
			int index = _index - 1;
			if (index < 0)
				index = _project->i18nPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_PreviousPage());
		}
		ImGui::SameLine();
		if (ImGui::Button(_page.text.c_str(), ImVec2(0, height))) {
			ImGui::OpenPopup("@Pg");
		}
		ImGui::SameLine();
		if (ImGui::Button(">", ImVec2(0, height))) {
			int index = _index + 1;
			if (index >= _project->i18nPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		do {
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (wndWidth >= 430) {
				if (_status.info.empty()) {
					const int ln = object()->itemCount();
					const int col = object()->languageCount();
					_status.info = Text::format(
						ws->theme()->tooltipI18n_Info(),
						{
							Text::toString(ln),
							Text::toString(col)
						}
					);
				}

				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::ImageButton(ws->theme()->iconInfo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, _status.info.empty() ? nullptr : _status.info.c_str());
				ImGui::PopStyleColor(2);
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
			}
			if (!_warnings.empty()) {
				const ImVec4 col = ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->warningColor);
				ImGui::PushStyleColor(ImGuiCol_Text, col);

				if (ImGui::ImageButton(ws->theme()->iconWarning()->pointer(rnd), ImVec2(13, 13), col, false, ws->theme()->tooltip_Warning().c_str())) {
					ImGui::OpenPopupTooltip("Wrn");
				}
				if (ImGui::PopupTooltip("Wrn", _warnings.text, ws->theme()->generic_Dismiss().c_str())) {
					_warnings.clear();
				}
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();

				ImGui::PopStyleColor();
			}
			if (ImGui::ImageButton(ws->theme()->iconImport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Import().c_str())) {
				ImGui::OpenPopup("@Imp");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconExport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Export().c_str())) {
				ImGui::OpenPopup("@Xpt");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconViews()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_View().c_str())) {
				ImGui::OpenPopup("@Views");
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(ws->theme()->iconFont()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipFontAndI18n_Font().c_str())) {
				ws->category(Workspace::Categories::FONT);
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			do {
				WIDGETS_SELECTION_GUARD(ws->theme());

				if (ImGui::ImageButton(ws->theme()->iconI18n()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipFontAndI18n_I18n().c_str())) {
					// Do nothing.
				}
			} while (false);
			width_ += ImGui::GetItemRectSize().x;
			width_ += style.FramePadding.x;
			_statusWidth = width_;
		} while (false);
		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImGui::PopStyleColor(3);
		}

		statusBarMenu(wnd, rnd, ws);
	}
	void statusBarMenu(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		// Paging.
		if (ImGui::BeginPopup("@Pg")) {
			const Text::Array &assetNames = ws->getI18nPageNames();
			const int n = _project->i18nPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::I18N, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_JsonFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_JSON_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					std::string txt;
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_Csv())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseCsv(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CsvFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_CSV_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					std::string txt;
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					I18n::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseCsv(newObj, txt, status)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::I18n::Import>()
						->with(newObj)
						->exec(object());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					std::string txt;
					if (!entry()->serializeJson(txt, true))
						break;

					const std::string osstr = Unicode::toOs(txt);

					Platform::setClipboardText(osstr.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_JsonFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-i18n.json" : Text::sanitizeFilename(entry()->name) + ".json",
						GBBASIC_JSON_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "json")
						path += ".json";

					std::string txt;
					if (!entry()->serializeJson(txt, true))
						break;

					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeString(txt);
					file->close();

#if !defined GBBASIC_OS_HTML
					FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
					std::string path_ = fileInfo->parentPath();
					path_ = Unicode::toOs(path_);
					Platform::browse(path_.c_str());
#endif /* Platform macro. */

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_Csv())) {
				do {
					std::string txt;
					if (!entry()->serializeCsv(txt))
						break;

					const std::string osstr = Unicode::toOs(txt);

					Platform::setClipboardText(osstr.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CsvFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-i18n.csv" : Text::sanitizeFilename(entry()->name) + ".csv",
						GBBASIC_CSV_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "csv")
						path += ".csv";

					std::string txt;
					if (!entry()->serializeCsv(txt))
						break;

					const std::string osstr = Unicode::toOs(txt);
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeString(osstr);
					file->close();

#if !defined GBBASIC_OS_HTML
					FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
					std::string path_ = fileInfo->parentPath();
					path_ = Unicode::toOs(path_);
					Platform::browse(path_.c_str());
#endif /* Platform macro. */

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}

		// Views.
		if (ImGui::BeginPopup("@Views")) {
			if (ImGui::MenuItem(ws->theme()->menu_FineZooming(), nullptr, &_tools.fineZooming)) {
				_tools.magnification = -1;

				_project->preferencesFineZooming(_tools.fineZooming);

				_project->hasDirtyEditor(true);
			}

			ImGui::EndPopup();
		}
	}

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "I18n editor: ";
				msg_ += msg;
				if (msg.back() != '.')
					msg_ += '.';
				ws->warn(msg_.c_str());
			}
		} else {
			_warnings.remove(msg);
		}
	}

	void modified(void) {
		_warnings.clear();
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd) {
		const bool refillName =
			Command::is<Commands::I18n::SetName>(cmd);
		const bool toCheck =
			Command::is<Commands::I18n::AddItem>(cmd) ||
			Command::is<Commands::I18n::AddLanguage>(cmd) ||
			Command::is<Commands::I18n::RenameLanguage>(cmd) ||
			Command::is<Commands::I18n::ChangeContent>(cmd) ||
			Command::is<Commands::I18n::Cut>(cmd) ||
			Command::is<Commands::I18n::Paste>(cmd) ||
			Command::is<Commands::I18n::Delete>(cmd) ||
			Command::is<Commands::I18n::Import>(cmd);

		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearTilesPageNames();
		}
		if (toCheck) {
			_checker();
		}
	}

	I18nAssets::Entry* entry(void) const {
		I18nAssets::Entry* entry = _project->getI18n(_index);

		return entry;
	}
	I18n::Ptr &object(void) const {
		return entry()->data;
	}
};

EditorI18n* EditorI18n::create(void) {
	EditorI18nImpl* result = new EditorI18nImpl();

	return result;
}

void EditorI18n::destroy(EditorI18n* ptr) {
	EditorI18nImpl* impl = static_cast<EditorI18nImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
