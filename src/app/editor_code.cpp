/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor.h"
#include "editor_code.h"
#include "editor_code_language_definition.h"
#include "theme.h"
#include "workspace.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/filesystem.h"
#include "../utils/marker.h"
#include <SDL.h>

/*
** {===========================================================================
** Utilities
*/

class DeadCodeRange {
private:
	typedef std::pair<int, int> CodeRange;
	typedef std::vector<CodeRange> CodeRanges;

private:
	CodeRanges _mergedRanges;

public:
	DeadCodeRange() {
	}
	explicit DeadCodeRange(const GBBASIC::PreprocessorBranch::Array &branches) {
		CodeRanges ranges;
		for (const GBBASIC::PreprocessorBranch &branch : branches) {
			if (!branch.isAlive && branch.valid()) {
				const int begin = branch.beginLine;
				const int end = Math::max(branch.beginLine, branch.endLine - 1);
				ranges.emplace_back(begin, end);
			}
		}
		if (ranges.empty())
			return;

		std::sort(ranges.begin(), ranges.end());
		_mergedRanges.push_back(ranges[0]);
		for (int i = 1; i < (int)ranges.size(); ++i) {
			CodeRange &last = _mergedRanges.back();
			if (ranges[i].first <= last.second + 1)
				last.second = std::max(last.second, ranges[i].second);
			else
				_mergedRanges.push_back(ranges[i]);
		}
	}

	bool isDeadLine(int ln) const {
		CodeRanges::const_iterator it = std::upper_bound(
			_mergedRanges.begin(), _mergedRanges.end(), ln,
			[] (int value, const CodeRange &range) {
				return value < range.first;
			}
		);
		if (it == _mergedRanges.begin())
			return false;

		--it;

		return ln <= it->second;
	}

	void clear(void) {
		_mergedRanges.clear();
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Code editor
*/

class EditorCodeImpl : public EditorCode, public ImGui::CodeEditor {
private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;

	bool _isMajor = true;
	bool _acquireFocus = false;
	float _scrollY = 0.0f;
	bool _actived = false;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _page;
	struct {
		int line = -1;
		std::string text;
		bool filled = false;

		void clear(void) {
			line = -1;
			text.clear();
			filled = false;
		}
	} _line;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _tokens;
	struct {
		std::string text;
		bool filled = false;
		Marker<bool> analyzing = Marker<bool>(false);

		void clear(void) {
			text.clear();
			filled = false;
			analyzing = Marker<bool>(false);
		}
	} _status;
	float _statusWidth = 0.0f;
	mutable struct {
		std::string text;
		bool overdue = true;

		void clear(void) {
			text.clear();
			overdue = true;
		}
	} _cache;
	struct {
		DeadCodeRange deadCodeRange;
		unsigned revision = 0;
		bool filled = false;

		void clear(bool clearRevision) {
			deadCodeRange.clear();
			if (clearRevision)
				revision = 0;
			filled = false;
		}
	} _analyzedCodeInformations;

	struct Tools {
		typedef std::function<void(void)> GlobalSearchingHandler;

		bool initialized = false;
		bool focused = false;

		int jumpingTo = -1;

		Editing::Tools::Marker marker;
		int direction = 0;
		GlobalSearchingHandler search = nullptr;

		void clear(void) {
			initialized = false;
			focused = false;

			jumpingTo = -1;
		}
	} _tools;

	static Debounce _debounce;
	static struct Shared {
		bool jumping = false;
		bool finding = false;
		mutable std::string* wordPtr = nullptr;
		mutable Editing::Tools::TextPage::Array* allPagesPtr = nullptr;
		mutable Editing::Tools::Marker::Coordinates::Array* boundariesPtr = nullptr;

		Shared() {
		}
		~Shared() {
			clear();
		}

		const std::string &word(void) const {
			if (wordPtr == nullptr)
				wordPtr = new std::string();

			return *wordPtr;
		}
		std::string &word(void) {
			if (wordPtr == nullptr)
				wordPtr = new std::string();

			return *wordPtr;
		}

		const Editing::Tools::TextPage::Array &allPages(void) const {
			if (allPagesPtr == nullptr)
				allPagesPtr = new Editing::Tools::TextPage::Array();

			return *allPagesPtr;
		}
		Editing::Tools::TextPage::Array &allPages(void) {
			if (allPagesPtr == nullptr)
				allPagesPtr = new Editing::Tools::TextPage::Array();

			return *allPagesPtr;
		}

		const Editing::Tools::Marker::Coordinates::Array &boundaries(void) const {
			if (boundariesPtr == nullptr)
				boundariesPtr = new Editing::Tools::Marker::Coordinates::Array();

			return *boundariesPtr;
		}
		Editing::Tools::Marker::Coordinates::Array &boundaries(void) {
			if (boundariesPtr == nullptr)
				boundariesPtr = new Editing::Tools::Marker::Coordinates::Array();

			return *boundariesPtr;
		}

		void clear(void) {
			jumping = false;
			finding = false;
			if (wordPtr) {
				delete wordPtr;
				wordPtr = nullptr;
			}
			if (allPagesPtr) {
				delete allPagesPtr;
				allPagesPtr = nullptr;
			}
			if (boundariesPtr) {
				delete boundariesPtr;
				boundariesPtr = nullptr;
			}
		}
	} _shared;

public:
	static int refCount;

public:
	EditorCodeImpl(class Workspace* ws, bool isMajor) :
		_isMajor(isMajor)
	{
		const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
		SetLanguageDefinition(EditorCodeLanguageDefinition::languageDefinition(krnl ? krnl->path().c_str() : nullptr));
	}
	virtual ~EditorCodeImpl() override {
		close(_index);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	void initialize(int refCount_) {
		if (refCount_ == 0) {
			_debounce.trigger();
		}
	}
	void dispose(int refCount_) {
		if (refCount_ == 0) {
			_debounce.clear();
			_shared.clear();
		}
	}

	virtual void open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* project, int index, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		CodeAssets::Entry* entry = _project->getCode(_index);
		if (entry) {
			std::string txt;
			entry->toString(txt, nullptr);
			fromString(txt.c_str(), txt.length());
		}

		SetStickyLineNumbers(true);

		SetHeadClickEnabled(true);

		DisableShortcut(All);

		SetErrorTipEnabled(true);

		SetTooltipEnabled(false);

		const Settings &settings = ws->settings();
		post(Editable::SET_INDENT_RULE, (Int)settings.mainIndentRule);
		post(Editable::SET_COLUMN_INDICATOR, (Int)settings.mainColumnIndicator);
		post(Editable::SET_SHOW_SPACES, (Int)settings.mainShowWhiteSpaces);

		SetIsDeadLineHandler(std::bind(&EditorCodeImpl::isDeadLine, this, std::placeholders::_1));

		SetColorizedHandler(std::bind(&EditorCodeImpl::colorized, this, std::placeholders::_1));

		SetModifiedHandler(std::bind(&EditorCodeImpl::modified, this, wnd, rnd, ws, std::placeholders::_1));

		SetHeadClickedHandler(std::bind(&EditorCodeImpl::headClicked, this, ws, std::placeholders::_1, std::placeholders::_2));

		_tools.search = std::bind(&EditorCodeImpl::searchGlobally, this, wnd, rnd, ws);

		fprintf(stdout, "Code editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Code editor closed: #%d.\n", _index);

		SetIsDeadLineHandler(nullptr);

		SetColorizedHandler(nullptr);

		SetModifiedHandler(nullptr);

		SetHeadClickedHandler(nullptr);

		_project = nullptr;
		_index = -1;

		_page.clear();
		_line.clear();
		_tokens.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_cache.clear();
		_analyzedCodeInformations.clear(true);

		_tools.clear();
		_tools.search = nullptr;
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		_acquireFocus = true;

		SetScrollPositionY(_scrollY);
	}
	virtual void leave(class Workspace*) override {
		_scrollY = GetScrollPositionY();
	}

	virtual void flush(void) const override {
		CodeAssets::Entry* entry = _project->getCode(_index);
		if (!entry)
			return;

		const std::string &txt = toString();
		if (!txt.empty())
			entry->fromString(txt.c_str(), txt.length(), nullptr);
		else
			entry->fromString("", nullptr);
	}

	virtual void addKeyword(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		def.Keys.insert(str);
	}
	virtual void addSymbol(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		def.Symbols.insert(str);
	}
	virtual void addIdentifier(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "GB BASIC function";
		def.Ids.insert(std::make_pair(std::string(str), id));
	}
	virtual void addPreprocessor(const char* str) override {
		LanguageDefinition &def = GetLanguageDefinition();
		Identifier id;
		id.Declaration = "GB BASIC preprocessor";
		def.PreprocIds.insert(std::make_pair(std::string(str), id));
	}

	virtual const std::string &toString(void) const override {
		if (_cache.overdue) {
			_cache.text = GetText("\n");
			_cache.overdue = false;
		}

		return _cache.text;
	}
	virtual void fromString(const char* txt, size_t /* len */) override {
		SetText(txt);
	}

	virtual bool readonly(void) const override {
		return IsReadOnly();
	}
	virtual void readonly(bool ro) override {
		SetReadOnly(ro);

		_status.clear();
	}

	virtual bool hasUnsavedChanges(void) const override {
		return !IsChangesSaved();
	}
	virtual void markChangesSaved(void) override {
		SetChangesSaved();
	}
	virtual void prepareForSaving(class Workspace*) override {
		// Do nothing.
	}

	virtual void copy(void) override {
		if (_tools.focused)
			return;

		Copy();
	}
	virtual void cut(void) override {
		if (ReadOnly) {
			copy();

			return;
		}

		if (_tools.focused)
			return;

		Cut();
	}
	virtual bool pastable(void) const override {
		if (ReadOnly)
			return false;

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (ReadOnly)
			return;

		if (_tools.focused)
			return;

		Paste();
	}
	virtual void del(bool wholeLine) override {
		if (ReadOnly)
			return;

		if (_tools.focused)
			return;

		if (wholeLine)
			DeleteLine();
		else
			Delete();
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		return CanRedo() ? "" : nullptr;
	}
	virtual const char* undoable(void) const override {
		return CanUndo() ? "" : nullptr;
	}

	virtual void redo(BaseAssets::Entry*) override {
		if (ReadOnly)
			return;

		Redo();

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		if (ReadOnly)
			return;

		Undo();

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case SET_INDENT_RULE: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Settings::SPACE_2);
				switch ((Settings::IndentRules)rule) {
				case Settings::SPACE_2:
					SetIndentWithTab(false);
					SetTabSize(2);

					break;
				case Settings::SPACE_4:
					SetIndentWithTab(false);
					SetTabSize(4);

					break;
				case Settings::SPACE_8:
					SetIndentWithTab(false);
					SetTabSize(8);

					break;
				case Settings::TAB_2:
					SetIndentWithTab(true);
					SetTabSize(2);

					break;
				case Settings::TAB_4:
					SetIndentWithTab(true);
					SetTabSize(4);

					break;
				case Settings::TAB_8:
					SetIndentWithTab(true);
					SetTabSize(8);

					break;
				}
			}

			return Variant(true);
		case SET_COLUMN_INDICATOR: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Settings::COL_80);
				switch ((Settings::ColumnIndicator)rule) {
				case Settings::COL_NONE:
					SetSafeColumnIndicatorOffset(0);

					break;
				case Settings::COL_40:
					SetSafeColumnIndicatorOffset(40);

					break;
				case Settings::COL_60:
					SetSafeColumnIndicatorOffset(60);

					break;
				case Settings::COL_80:
					SetSafeColumnIndicatorOffset(80);

					break;
				case Settings::COL_100:
					SetSafeColumnIndicatorOffset(100);

					break;
				case Settings::COL_120:
					SetSafeColumnIndicatorOffset(120);

					break;
				}
			}

			return Variant(true);
		case SET_SHOW_SPACES: {
				const bool show = unpack<bool>(argc, argv, 0, true);
				SetShowWhiteSpaces(show);
			}

			return Variant(true);
		case SET_CONTENT: {
				const void* ptr = unpack<void*>(argc, argv, 0, nullptr);
				if (!ptr)
					return Variant(false);

				const char* txt = (const char*)ptr;
				SetText(txt);
			}

			return Variant(true);
		case SELECT_ALL:
			if (_tools.focused)
				return Variant(false);

			SelectAll();

			return Variant(true);
		case SELECT_WORD:
			if (_tools.focused)
				return Variant(false);

			SelectWordUnderCursor();

			return Variant(true);
		case DELETE_LINE:
			DeleteLine();

			return Variant(true);
		case BACK_SPACE:
			BackSpace();

			return Variant(true);
		case BACK_SPACE_WORDWISE:
			BackSpaceWordwise();

			return Variant(true);
		case INDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				if (_tools.focused)
					return Variant(false);

				Indent(byKey);
			}

			return Variant(true);
		case UNINDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				if (_tools.focused)
					return Variant(false);

				Unindent(byKey);
			}

			return Variant(true);
		case TO_LOWER_CASE:
			ToLowerCase();

			return Variant(true);
		case TO_UPPER_CASE:
			ToUpperCase();

			return Variant(true);
		case TOGGLE_COMMENT:
			if (_tools.focused)
				return Variant(false);

			if (HasSelection()) {
				if (GetCommentLines() == GetNonEmptySelectionLines())
					Uncomment();
				else
					Comment();
			} else {
				if (GetCommentLines() > 0)
					Uncomment();
				else
					Comment();
			}

			return Variant(true);
		case MOVE_UP:
			if (_tools.focused)
				return Variant(false);

			MoveLineUp();

			return Variant(true);
		case MOVE_DOWN:
			if (_tools.focused)
				return Variant(false);

			MoveLineDown();

			return Variant(true);
		case FIND: {
				const bool withPopup = unpack<bool>(argc, argv, 0, true);

				if (withPopup) {
					_tools.search();

					return Variant(true);
				}

				_tools.initialized = false;

				_shared.jumping = false;
				_tools.jumpingTo = -1;

				_shared.finding = true;

				Coordinates begin, end;
				GetSelection(begin, end);
				if (begin == end)
					_shared.word() = GetWordUnderCursor(&begin, &end);
				else
					_shared.word() = GetSelectionText();
				SetSelection(begin, end);

				_tools.direction = 0;
			}

			return Variant(true);
		case FIND_NEXT:
			_shared.jumping = false;
			_tools.jumpingTo = -1;

			if (_shared.word().empty()) {
				_shared.finding = true;

				_shared.word() = GetWordUnderCursor();
			}

			_tools.direction = 1;

			return Variant(true);
		case FIND_PREVIOUS:
			_shared.jumping = false;
			_tools.jumpingTo = -1;

			if (_shared.word().empty()) {
				_shared.finding = true;

				_shared.word() = GetWordUnderCursor();
			}

			_tools.direction = -1;

			return Variant(true);
		case GOTO: {
				_tools.initialized = false;

				_shared.finding = false;

				const Coordinates coord = GetCursorPosition();
				_shared.jumping = true;
				_tools.jumpingTo = coord.Line;
			}

			return Variant(true);
		case GET_CURSOR:
			return Variant((Variant::Int)GetCursorPosition().Line);
		case SET_CURSOR: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				SetCursorPosition(Coordinates(ln, 0));
			}

			return Variant(true);
		case SET_SELECTION: {
				const Variant::Int ln0 = unpack<Variant::Int>(argc, argv, 0, -1);
				const Variant::Int col0 = unpack<Variant::Int>(argc, argv, 1, -1);
				const Variant::Int ln1 = unpack<Variant::Int>(argc, argv, 2, -1);
				const Variant::Int col1 = unpack<Variant::Int>(argc, argv, 3, -1);
				if (ln0 < 0 || ln0 >= GetTotalLines())
					break;
				if (col0 < 0 || col0 > GetColumnsAt(ln0))
					break;
				if (ln1 < 0 || ln1 >= GetTotalLines())
					break;
				if (col1 < 0 || col1 > GetColumnsAt(ln1))
					break;

				SetSelection(Coordinates(ln0, col0), Coordinates(ln1, col1));
				SetCursorPosition(Coordinates(ln1, col1));
			}

			return Variant(true);
		case GET_PROGRAM_POINTER:
			return Variant((Variant::Int)GetProgramPointer());
		case SET_PROGRAM_POINTER: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines()) {
					SetProgramPointer(-1);

					break;
				}

				SetProgramPointer(ln);
			}

			return Variant(true);
		case SET_ERRORS: {
				bool cursorSet = false;
				const Object::Ptr obj = unpack<Object::Ptr>(argc, argv, 0, Object::Ptr(nullptr));
				const Workspace::CompilingErrors::Ptr errors = Object::as<Workspace::CompilingErrors::Ptr>(obj);
				ErrorMarkers errs;
				for (int i = 0; i < errors->count(); ++i) {
					const Workspace::CompilingErrors::Entry* err = errors->get(i);
					if (err->page != _index)
						continue;

					ErrorMarkers::iterator it = errs.find(err->row);
					if (it == errs.end()) {
						errs.insert(std::make_pair(err->row, Error(err->message, err->isWarning, false)));
					} else {
						Error &existing = it->second;
						if (!existing.isWarning && err->isWarning) {
							// Do nothing.
						} else if (existing.isWarning && !err->isWarning) {
							existing = Error(err->message, err->isWarning, false);
						} else {
							existing.message += "\n       " + err->message;
						}
					}
					if (!cursorSet) {
						SetCursorPosition(Coordinates(err->row, err->column));
						EnsureCursorVisible();
						cursorSet = true;
					}
				}
				if (!errs.empty())
					SetErrorMarkers(errs);
			}

			return Variant(true);
		case CLEAR_ANALYZED_CODE_INFORMATIONS: {
				const bool clearRevision = unpack<bool>(argc, argv, 0, false);

				_analyzedCodeInformations.clear(clearRevision);
			}

			return Variant(true);
		case CLEAR_ERRORS:
			ClearErrorMarkers();

			return Variant(true);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		case SYNC_DATA: {
				const Variant::Int minor = unpack<Variant::Int>(argc, argv, 0, 0);
				const void* ptr = unpack<void*>(argc, argv, 1, nullptr);
				const Variant::Int idx = unpack<Variant::Int>(argc, argv, 2, -1);
				const bool majorToMinor = !!minor;
				EditorCode* dstCodeEditor = (EditorCode*)ptr;
				const int dstCodeIndex = (int)idx;
				(void)majorToMinor;
				(void)dstCodeIndex;
				EditorCodeImpl* dstCodeEditorImpl = static_cast<EditorCodeImpl*>(dstCodeEditor);
				SyncTo(dstCodeEditorImpl);
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
		const char* title,
		float /* x */, float /* y */, float width, float height,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		touchAnalyzedCodeInformations(ws);

		shortcuts(wnd, rnd, ws);

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		const bool matched = (_isMajor && _project->isMajorCodeEditorActive()) || (!_isMajor && !_project->isMajorCodeEditorActive());
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		constexpr const bool matched = true;
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		float toolBarHeight = 0;
		bool constrainMinWidth = false;
		if (matched && _shared.jumping && _tools.jumpingTo >= 0) {
			const float posY = ImGui::GetCursorPosY();
			if (Editing::Tools::jump(rnd, ws, &_tools.jumpingTo, width, &_tools.initialized, &_tools.focused, 0, GetTotalLines() - 1)) {
				if (HasSelection())
					ClearSelection();
				SetCursorPosition(Coordinates(_tools.jumpingTo, 0));
			}
			toolBarHeight += ImGui::GetCursorPosY() - posY;
			constrainMinWidth = true;
		}
		if (matched && (_shared.finding || _tools.direction != 0)) {
			Coordinates srcBegin, srcEnd;
			GetSelection(srcBegin, srcEnd);
			_tools.marker = Editing::Tools::Marker(
				Editing::Tools::Marker::Coordinates(AssetsBundle::Categories::CODE, _index, srcBegin.Line, srcBegin.Column),
				Editing::Tools::Marker::Coordinates(AssetsBundle::Categories::CODE, _index, srcEnd.Line, srcEnd.Column)
			);

			Editing::Tools::TextPage::Array const * allPages = nullptr;
			Editing::Tools::Marker::Coordinates::Array const * boundaries = nullptr;
			getAllPagesGlobally(wnd, rnd, ws, &allPages, &boundaries);
			const float y_ = ImGui::GetCursorPosY();
			const bool stepped = Editing::Tools::find(
				rnd, ws,
				&_tools.marker,
				width,
				&_tools.initialized, &_tools.focused,
				allPages, &_shared.word(),
				boundaries,
				&_tools.direction,
				&ws->settings().mainCaseSensitive, &ws->settings().mainMatchWholeWords, &ws->settings().mainGlobalSearch,
				_shared.finding,
				std::bind(&EditorCodeImpl::getWordGlobally, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2)
			);
			if (stepped && !_tools.marker.empty()) {
				if (_tools.marker.begin.page == _index) {
					const Coordinates begin(_tools.marker.begin.line, _tools.marker.begin.column);
					const Coordinates end(_tools.marker.end.line, _tools.marker.end.column);

					SetCursorPosition(begin);
					SetSelection(begin, end);
				} else {
					const int index = _tools.marker.begin.page;
					EditorCodeImpl* editor = openCodeEditorGlobally(wnd, rnd, ws, _project, index, false);

					ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, index);

					const Coordinates begin(_tools.marker.begin.line, _tools.marker.begin.column);
					const Coordinates end(_tools.marker.end.line, _tools.marker.end.column);

					editor->SetCursorPosition(begin);
					editor->SetSelection(begin, end);
				}
			}
			toolBarHeight += ImGui::GetCursorPosY() - y_;
			constrainMinWidth = true;
		}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (constrainMinWidth && _project->isMinorCodeEditorEnabled()) {
			if (_isMajor) {
				const float currentWidth = rnd->width() - ws->codeEditorMinorWidth();
				if (currentWidth < EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH) {
					const float w = (float)rnd->width() - EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH;
					ws->codeEditorMinorWidth(w);
					_project->minorCodeEditorWidth(w);
				}
			} else {
				const float currentWidth = ws->codeEditorMinorWidth();
				if (currentWidth < EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH) {
					const float w = EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH;
					ws->codeEditorMinorWidth(w);
					_project->minorCodeEditorWidth(w);
				}
			}
		}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		if (_acquireFocus) {
			_acquireFocus = false;
			ImGui::SetNextWindowFocus();
		}

		ImFont* fontCode = ws->theme()->fontCode();
		if (fontCode && fontCode->IsLoaded()) {
			ImGui::PushFont(fontCode);
			SetFont(fontCode);
		}
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		const float height_ = height - toolBarHeight;
		(void)statusBarHeight;
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		const float height_ = height - statusBarHeight - toolBarHeight;
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		Render(title, ImVec2(width, height_));
		if (fontCode && fontCode->IsLoaded()) {
			SetFont(nullptr);
			ImGui::PopFont();
		}

		context(wnd, rnd, ws);

		if (!ws->analyzing() && _debounce.fire()) {
			if (ws->needAnalyzing()) {
				if (!ws->analyze(false)) // Analyzing not started.
					_debounce.modified(); // Will try again later.
			}
		}

		_actived = IsEditorFocused() || ImGui::IsWindowFocused();
		refreshStatus(wnd, rnd, ws);
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (_actived) {
			_project->isMajorCodeEditorActive(_isMajor);
		}
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		renderStatus(wnd, rnd, ws, width, statusBarHeight);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	}

	virtual bool isToolBarVisible(void) const override {
		return _shared.jumping || _shared.finding;
	}

	virtual void renderStatus(class Window* wnd, class Renderer* rnd, class Workspace* ws, float x, float y, float width, float height) override {
		(void)x;
		(void)y;

		renderStatus(wnd, rnd, ws, width, height);
	}
	virtual void statusInvalidated(void) override {
		_page.filled = false;
		_line.filled = false;
		_tokens.filled = false;
		_status.filled = false;
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
	void shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		const bool matched = (_isMajor && _project->isMajorCodeEditorActive()) || (!_isMajor && !_project->isMajorCodeEditorActive());
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		constexpr const bool matched = true;
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		if (!matched)
			return;

		if (!ws->canUseShortcuts())
			return;

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed()) {
			_tools.clear();

			_shared.clear();
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
					index = _project->codePageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->codePageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, index);
			}
		}

#if defined GBBASIC_OS_APPLE
		const Editing::Shortcut f(SDL_SCANCODE_F, false, true, false, false, false, true);
#else /* GBBASIC_OS_APPLE */
		const Editing::Shortcut f(SDL_SCANCODE_F, true, true);
#endif /* GBBASIC_OS_APPLE */
		if (f.pressed() && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			searchGlobally(wnd, rnd, ws);
	}

	void context(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			if (!HasSelection())
				SelectWordUnderMouse();

			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Ctx")) {
			const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
			const GBBASIC::Kernel::Snippet::Array* snippets = krnl ? &krnl->snippets() : nullptr;

			if (!snippets->empty()) {
				if (ImGui::BeginMenu(ws->theme()->menu_InsertSnippet())) {
					for (int i = 0; i < (int)snippets->size(); ++i) {
						const GBBASIC::Kernel::Snippet &snippet = (*snippets)[i];

						if (snippet.isSeparator) {
							ImGui::Separator();

							continue;
						}

						const char* name = Localization::get(snippet.name);
						if (!name)
							continue;

						if (ImGui::MenuItem(name)) {
							const std::string &code = snippet.content;
							Paste(code.c_str());
						}
					}

					ImGui::EndMenu();
				}
			}
			if (ImGui::MenuItem(ws->theme()->menu_CopyJumpCode())) {
				const int ln = GetCursorPosition().Line;

				std::string val;
				val += "goto ";
				val += "#" + Text::toString(_index);
				val += ":" + Text::toString(ln + 1);

				Platform::setClipboardText(val.c_str());

				ws->bubble(ws->theme()->dialogPrompt_CopiedLineNumberToClipboard(), nullptr);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menu_Cut(), nullptr, nullptr, !readonly() && HasSelection())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Copy(), nullptr, nullptr, HasSelection())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Delete(), nullptr, nullptr, !readonly() && HasSelection())) {
				del(false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
				post(SELECT_ALL);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menu_IncreaseIndent(), nullptr, nullptr, !readonly())) {
				post(INDENT, false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_DecreaseIndent(), nullptr, nullptr, !readonly())) {
				post(UNINDENT, false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_ToLowerCase(), nullptr, nullptr, !readonly())) {
				post(TO_LOWER_CASE);
			}
			if (ImGui::MenuItem(ws->theme()->menu_ToUpperCase(), nullptr, nullptr, !readonly())) {
				post(TO_UPPER_CASE);
			}
			if (ImGui::MenuItem(ws->theme()->menu_ToggleComment(), nullptr, nullptr, !readonly())) {
				post(TOGGLE_COMMENT);
			}

			ImGui::EndPopup();
		}
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		const Coordinates coord = GetCursorPosition();
		if (!_line.filled || _line.line != coord.Line) {
			_line.line = coord.Line;
			_line.text = ws->theme()->status_Ln() + " " + Text::toString(_line.line + 1) + "/" + Text::toString(GetTotalLines());
			_line.filled = true;
		}
		if (!_tokens.filled) {
			const int tokens = GetTotalTokens();
			_tokens.text = ws->theme()->status_Tokens() + " " + Text::toString(tokens);
			_tokens.filled = true;
		}
		if (!_status.filled) {
			if (readonly()) {
				_status.text = ws->theme()->status_Readonly();
			}
			_status.filled = true;
		}
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height) {
		ImGuiStyle &style = ImGui::GetStyle();

		const bool actived = _actived || IsEditorFocused() || ImGui::IsWindowFocused();
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
		const Coordinates coord = GetCursorPosition();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		if (ImGui::Button("<", ImVec2(0, height))) {
			int index = _index - 1;
			if (index < 0)
				index = _project->codePageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, index);
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
			if (index >= _project->codePageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		if (ImGui::Button(_line.text.c_str(), ImVec2(0, height))) {
			if (_tools.jumpingTo != -1) {
				_tools.clear();

				_shared.clear();
			} else {
				post(GOTO);
			}
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		if (_status.text.empty()) {
			ImGui::Text(
				" %s %d ",
				ws->theme()->status_Col().c_str(),
				coord.Column + 1
			);
		} else {
			ImGui::Text(
				" %s %d %s ",
				ws->theme()->status_Col().c_str(),
				coord.Column + 1,
				_status.text.c_str()
			);
		}
		ImGui::SameLine();
		if (ImGui::GetWindowWidth() >= 370) {
			if (ImGui::Button(_tokens.text.c_str(), ImVec2(0, height))) {
				if (_shared.finding) {
					_tools.clear();

					_shared.clear();
				} else {
					post(FIND);
				}
			}
			ImGui::SameLine();
		}
		do {
			_status.analyzing = ws->analyzing();
			const Marker<bool>::Values analyzing = _status.analyzing.values();
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (analyzing.first) {
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::ImageButton(ws->theme()->iconWorking()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Analyzing().c_str());
				ImGui::PopStyleColor(2);
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
			} else {
				const std::string &info = ws->getAnalyzedCodeInformation();
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::ImageButton(ws->theme()->iconInfo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, info.empty() ? nullptr : info.c_str());
				ImGui::PopStyleColor(2);
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
			}
			if (wndWidth >= 430) {
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
			}
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			if (_project->isMinorCodeEditorEnabled()) {
				if (_project->isMajorCodeEditorActive()) {
					if (ImGui::ImageButton(ws->theme()->iconMajor()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_MonoEditor().c_str())) {
						_project->isMinorCodeEditorEnabled(false);
					}
				} else {
					if (ImGui::ImageButton(ws->theme()->iconMinor()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_MonoEditor().c_str())) {
						const int idx = _project->activeMinorCodeIndex();
						_project->isMajorCodeEditorActive(true);
						_project->isMinorCodeEditorEnabled(false);
						ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, idx);
					}
				}
			} else {
				if (ImGui::ImageButton(ws->theme()->iconDual()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_SplitEditor().c_str())) {
					_project->isMajorCodeEditorActive(false);
					_project->isMinorCodeEditorEnabled(true);
					if (_project->activeMinorCodeIndex() == -1) {
						_project->activeMinorCodeIndex(_index);
					} else {
						const int minorIdx = Math::clamp(_project->activeMinorCodeIndex(), 0, _project->codePageCount() - 1);
						if (minorIdx != _project->activeMinorCodeIndex())
							_project->activeMinorCodeIndex(minorIdx);
					}
				}
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			if (ImGui::ImageButton(ws->theme()->iconProperty()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipProjectProperty_ProjectProperty().c_str())) {
				ws->showProjectProperty(wnd, rnd, _project, true);
			}
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
			const Text::Array &assetNames = ws->getCodePageNames();
			const int n = _project->codePageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
			const GBBASIC::Kernel::Snippet::Array* snippets = krnl ? &krnl->snippets() : nullptr;

			if (!snippets->empty()) {
				if (ImGui::BeginMenu(ws->theme()->menu_InsertSnippet())) {
					for (int i = 0; i < (int)snippets->size(); ++i) {
						const GBBASIC::Kernel::Snippet &snippet = (*snippets)[i];

						if (snippet.isSeparator) {
							ImGui::Separator();

							continue;
						}

						const char* name = Localization::get(snippet.name);
						if (!name)
							continue;

						if (ImGui::MenuItem(name)) {
							const std::string &code = snippet.content;
							Paste(code.c_str());
						}
					}

					ImGui::EndMenu();
				}
				ImGui::Separator();
			}
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					SelectAll();
					Paste(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CodeFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_CODE_FILE_FILTER,
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

					SelectAll();
					Paste(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				do {
					const std::string &txt = toString();

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CodeFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						"gbbasic-code.bas",
						GBBASIC_CODE_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					const Text::Array exts = { "bas", "gbb", "txt" };
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || std::find(exts.begin(), exts.end(), ext) == exts.end())
						path += ".bas";

					const std::string &txt = toString();

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

					ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}
	}

	bool isDeadLine(int ln) {
		return _analyzedCodeInformations.deadCodeRange.isDeadLine(ln);
	}
	void colorized(bool) {
		_tokens.filled = false;
	}
	void modified(Window* wnd, Renderer* rnd, Workspace* ws, bool newLine) {
		_cache.overdue = true;

		_project->toPollEditor(true);

		ClearErrorMarkers();

		_debounce.modified();

		ws->needAnalyzing(true);

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (_isMajor)
			ws->syncMajorCodeEditorDataToMinor(wnd, rnd);
		else
			ws->syncMinorCodeEditorDataToMajor(wnd, rnd);
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		(void)wnd;
		(void)rnd;
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		if (newLine)
			++ws->activities().wroteCodeLines;
	}
	void headClicked(Workspace* ws, int ln, bool /* doubleClicked */) {
		(void)ws;
		(void)ln;

		// Do nothing.
	}

	void searchGlobally(Window* wnd, Renderer* rnd, Workspace* ws) const {
		ImGui::SearchPopupBox::ConfirmedHandler confirm(
			[wnd, rnd, ws, this] (const char* name) -> void {
				WORKSPACE_AUTO_CLOSE_POPUP(ws)

				if (!name || !*name)
					return;

				Editing::Tools::TextPage::Array const * allPages = nullptr;
				getAllPagesGlobally(wnd, rnd, ws, &allPages, nullptr);
				Editing::Tools::SearchResult::Array found;
				Editing::Tools::search(
					rnd, ws,
					found,
					*allPages, name,
					ws->settings().mainCaseSensitive, ws->settings().mainMatchWholeWords, ws->settings().mainGlobalSearch,
					_index,
					std::bind(&EditorCodeImpl::getWordGlobally, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2),
					std::bind(&EditorCodeImpl::getLineGlobally, this, wnd, rnd, ws, std::placeholders::_1)
				);
				ws->showSearchResult(name, found);
			},
			nullptr
		);
		ImGui::SearchPopupBox::CanceledHandler cancel(
			[ws] (void) -> void {
				WORKSPACE_AUTO_CLOSE_POPUP(ws)
			},
			nullptr
		);
		std::string default_ = _shared.finding ? _shared.word() : "";
		if (default_.empty())
			default_ = GetSelectionText("\n");
		if (default_.empty())
			default_ = GetWordUnderCursor();
		const size_t eol = Text::indexOf(default_, '\n');
		if (eol != std::string::npos)
			default_ = default_.substr(0, eol);
		ws->searchPopupBox(
			ws->theme()->windowCode_SearchFor(),
			default_,
			ImGuiInputTextFlags_None,
			confirm,
			cancel
		);
	}
	void getAllPagesGlobally(Window* wnd, Renderer* rnd, Workspace* ws, Editing::Tools::TextPage::Array const ** const allPages_ /* nullable */, Editing::Tools::Marker::Coordinates::Array const ** const boundaries_ /* nullable */) const {
		Editing::Tools::TextPage::Array &allPages = _shared.allPages();
		Editing::Tools::Marker::Coordinates::Array &boundaries = _shared.boundaries();
		allPages.clear();
		boundaries.clear();
		for (int i = 0; i < _project->codePageCount(); ++i) {
			Editing::Tools::Marker::Coordinates max;
			const std::string* str = toString(wnd, rnd, ws, _project, i, max);
			allPages.push_back(Editing::Tools::TextPage(str, AssetsBundle::Categories::CODE));
			boundaries.push_back(max);
		}

		if (allPages_)
			*allPages_ = &allPages;
		if (boundaries_)
			*boundaries_ = &boundaries;
	}
	std::string getLineGlobally(Window* wnd, Renderer* rnd, Workspace* ws, const Editing::Tools::Marker::Coordinates &pos) const {
		Coordinates srcBegin, srcEnd;
		std::string result;
		if (pos.page == _index) {
			result = GetTextLine(pos.line);
		} else {
			const int index = pos.page;
			EditorCodeImpl* editor = openCodeEditorGlobally(wnd, rnd, ws, _project, index, false);

			result = editor->GetTextLine(pos.line);
		}

		return result;
	}
	std::string getWordGlobally(Window* wnd, Renderer* rnd, Workspace* ws, const Editing::Tools::Marker::Coordinates &pos, Editing::Tools::Marker &src) const {
		Coordinates srcBegin, srcEnd;
		std::string result;
		if (pos.page == _index) {
			result = GetWordAt(Coordinates(pos.line, pos.column), &srcBegin, &srcEnd);
		} else {
			const int index = pos.page;
			EditorCodeImpl* editor = openCodeEditorGlobally(wnd, rnd, ws, _project, index, true);

			result = editor->GetWordAt(Coordinates(pos.line, pos.column), &srcBegin, &srcEnd);
		}
		src.begin = Editing::Tools::Marker::Coordinates(pos.category, pos.page, srcBegin.Line, srcBegin.Column);
		src.end = Editing::Tools::Marker::Coordinates(pos.category, pos.page, srcEnd.Line, srcEnd.Column);

		return result;
	}

	void touchAnalyzedCodeInformations(Workspace* ws) {
		// Prepare.
		if (_analyzedCodeInformations.filled)
			return;

		// Ignore if not changed.
		const unsigned revision = ws->getLanguageDefinitionRevision();
		if (_analyzedCodeInformations.revision == revision) {
			_analyzedCodeInformations.filled = true;

			return;
		}
		_analyzedCodeInformations.revision = revision;

		// Initialize with default definition.
		const GBBASIC::Kernel::Ptr &krnl = ws->activeKernel();
		SetLanguageDefinition(EditorCodeLanguageDefinition::languageDefinition(krnl ? krnl->path().c_str() : nullptr));

		// Initialize with conditional compilation branches.
		const GBBASIC::PreprocessorBranch::Array* preprocessorBranches = ws->getPreprocessorBranches(_index);
		if (preprocessorBranches && !preprocessorBranches->empty()) {
			_analyzedCodeInformations.deadCodeRange = DeadCodeRange(*preprocessorBranches);

			Colorize();
		}

		// Initialize with macros.
		Text::Set added;
		const GBBASIC::Macro::List* macros = ws->getMacroDefinitions();
		if (macros && !macros->empty()) {
			for (const GBBASIC::Macro &macro : *macros) {
				const std::string &name = macro.name;
				if (added.find(name) != added.end())
					continue;

				added.insert(name);

				addPreprocessor(name.c_str());
			}

			Colorize();
		}

		// Initialize with destinitions.
		added.clear();
		const Text::Array* destinitions = ws->getDestinitions();
		if (destinitions && !destinitions->empty()) {
			for (const std::string &dest : *destinitions) {
				if (added.find(dest) != added.end())
					continue;

				added.insert(dest);

				addSymbol(dest.c_str());
			}

			Colorize();
		}

		// Finish.
		_analyzedCodeInformations.filled = true;
	}

	static EditorCodeImpl* openCodeEditorGlobally(Window* wnd, Renderer* rnd, Workspace* ws, Project* prj, int index, bool colorize) {
		EditorCodeImpl* editor = nullptr;
		CodeAssets::Entry* entry = prj->getCode(index);
		bool toColorize = false;
		if (entry->editor) {
			editor = (EditorCodeImpl*)entry->editor;

			if (editor->ColorRangeMin < editor->ColorRangeMax) {
				toColorize = true;
				editor->ColorRangeMin = std::numeric_limits<int>::max();
				editor->ColorRangeMax = 0;
			}
		} else {
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			ws->touchCodeEditor(wnd, rnd, prj, index, prj->isMajorCodeEditorActive(), entry);
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			ws->touchCodeEditor(wnd, rnd, prj, index, true, entry);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

			editor = (EditorCodeImpl*)entry->editor;

			toColorize = true;
		}

		if (colorize && toColorize)
			editor->ColorizeRange(0, editor->GetTotalLines());

		return editor;
	}

	static const std::string* toString(Window* wnd, Renderer* rnd, Workspace* ws, Project* prj, int index, Editing::Tools::Marker::Coordinates &max) {
		CodeAssets::Entry* entry = prj->getCode(index);
		if (!entry)
			return nullptr;

		EditorCodeImpl* editor = openCodeEditorGlobally(wnd, rnd, ws, prj, index, true);
		if (!editor)
			return nullptr;

		max = Editing::Tools::Marker::Coordinates(AssetsBundle::Categories::CODE, index, editor->GetTotalLines(), editor->GetColumnsAt(editor->GetTotalLines()));

		return &editor->toString();
	}
};

EditorCodeImpl::Shared EditorCodeImpl::_shared;
EditorCodeImpl::Debounce EditorCodeImpl::_debounce = EditorCodeImpl::Debounce(STATIC_ANALYZER_INTERVAL);

int EditorCodeImpl::refCount = 0;

EditorCode* EditorCode::create(class Workspace* ws, bool isMajor) {
	EditorCodeImpl* result = new EditorCodeImpl(ws, isMajor);
	result->initialize(EditorCodeImpl::refCount++);

	return result;
}

void EditorCode::destroy(EditorCode* ptr) {
	EditorCodeImpl* impl = static_cast<EditorCodeImpl*>(ptr);
	impl->dispose(--EditorCodeImpl::refCount);
	delete impl;
}

/* ===========================================================================} */
