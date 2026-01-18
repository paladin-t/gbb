/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_search_result.h"
#include "theme.h"
#include "workspace.h"
#include "../../lib/imgui_code_editor/imgui_code_editor.h"
#include <SDL.h>

/*
** {===========================================================================
** Search result editor
*/

class EditorSearchResultImpl : public EditorSearchResult, public ImGui::CodeEditor {
private:
	bool _opened = false;

	Workspace* _workspace = nullptr; // Foreign.
	Project* _project = nullptr; // Foreign.

	struct Tools {
		bool focused = false;
		mutable std::string cache;

		void clear(void) {
			focused = false;
			cache.clear();
		}
	} _tools;

	struct Callbacks {
		EditorSearchResult::LineClickedHandler lineClickedHandler = nullptr;
	} _callbacks;

public:
	EditorSearchResultImpl() {
		SetLanguageDefinition(ImGui::CodeEditor::LanguageDefinition::Text());
	}
	virtual ~EditorSearchResultImpl() override {
		close(-1);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* /* wnd */, class Renderer* /* rnd */, class Workspace* ws, class Project* project, int /* index */, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_workspace = ws;
		_project = project;

		SetReadOnly(true);

		SetShowLineNumbers(false);

		SetStickyLineNumbers(true);

		SetHeadClickEnabled(true);

		DisableShortcut(All);

		SetShowWhiteSpaces(false);

		SetErrorTipEnabled(false);

		SetTooltipEnabled(false);

		SetLineClickedHandler(std::bind(&EditorSearchResultImpl::onLineClicked, this, std::placeholders::_1, std::placeholders::_2));

		fprintf(stdout, "Search result opened.\n");
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Search result closed.\n");

		_workspace = nullptr;
		_project = nullptr;

		_tools.clear();
	}

	virtual int index(void) const override {
		return -1;
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

	virtual const std::string &toString(void) const override {
		_tools.cache = GetText();

		return _tools.cache;
	}
	virtual void fromString(const char* txt, size_t /* len */) override {
		SetText(txt);

		ColorRangeMin = std::numeric_limits<int>::max();
		ColorRangeMax = 0;

		const Editing::Tools::SearchResult::Array &found = _workspace->searchResult();
		for (int j = 0; j < (int)found.size(); ++j) {
			const int k = j + 1;
			const Editing::Tools::SearchResult &sr = found[j];
			if (k < 0 || k >= (int)CodeLines.size())
				continue;

			Line &ln = CodeLines[k];
			for (int i = 0; i < (int)sr.head.length(); ++i) {
				if (i >= (int)ln.Glyphs.size())
					continue;

				Glyph &g = ln.Glyphs[i];
				g.ColorIndex = (ImU32)PaletteIndex::Symbol;
			}
			for (int i = sr.min().column; i < sr.max().column; ++i) {
				const int l = i + (int)sr.head.size() + 3 /* for " - " */;
				if (l >= (int)ln.Glyphs.size())
					continue;

				Glyph &g = ln.Glyphs[l];
				g.ColorIndex = (ImU32)PaletteIndex::CharLiteral;
			}
		}
	}

	virtual void setLineClickedHandler(EditorSearchResult::LineClickedHandler handler) override {
		_callbacks.lineClickedHandler = handler;
	}

	virtual bool readonly(void) const override {
		return true;
	}
	virtual void readonly(bool /* ro */) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return false;
	}
	virtual void markChangesSaved(void) override {
		// Do nothing.
	}

	virtual void copy(void) override {
		if (_tools.focused)
			return;

		Copy();
	}
	virtual void cut(void) override {
		copy();
	}
	virtual bool pastable(void) const override {
		return false;
	}
	virtual void paste(void) override {
		// Do nothing.
	}
	virtual void del(bool) override {
		// Do nothing.
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		return nullptr;
	}
	virtual const char* undoable(void) const override {
		return nullptr;
	}

	virtual void redo(BaseAssets::Entry*) override {
		// Do nothing.
	}
	virtual void undo(BaseAssets::Entry*) override {
		// Do nothing.
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
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
				if (col0 < 0 || col0 >= GetColumnsAt(ln0))
					break;
				if (ln1 < 0 || ln1 >= GetTotalLines())
					break;
				if (col1 < 0 || col1 >= GetColumnsAt(ln1))
					break;

				SetSelection(Coordinates(ln0, col0), Coordinates(ln1, col1));
				SetCursorPosition(Coordinates(ln1, col1));
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
		float x, float y, float width, float /* height */,
		double /* delta */
	) override {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

		const float minHeight = Math::min(rnd->height() * 0.3f, 256.0f * io.FontGlobalScale);
		ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_DOCK;
		if (ws->searchResultHeight() <= 0.0f) {
			ws->searchResultHeight(minHeight);
		}

		const float gripPaddingX = 16.0f;
		const float gripMarginY = ImGui::WindowResizingPadding().y;
		if (ws->searchResultResizing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			ws->searchResultResizing(false);

			const Math::Vec2i rndSize(rnd->width(), rnd->height());

			_project->foreach(
				[rnd, rndSize] (AssetsBundle::Categories, int, BaseAssets::Entry*, Editable* editor) -> void {
					if (editor)
						editor->resized(rnd, rndSize, rndSize);
				}
			);
		}
		const bool isHoveringRect = ImGui::IsMouseHoveringRect(
			ImVec2(gripPaddingX, y),
			ImVec2(width - gripPaddingX * 2.0f, y + gripMarginY),
			false
		);
		if (isHoveringRect && !ws->popupBox() && !ws->canvasHovering()) {
			ws->searchResultResizing(true);

			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			ws->searchResultResizing(false);
		}
		if (ws->searchResultResizing()) {
			flags &= ~ImGuiWindowFlags_NoResize;
		}

		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(
			ImVec2(width, ws->searchResultHeight()),
			ImGuiCond_Always
		);
		ImGui::SetNextWindowSizeConstraints(ImVec2(-1, minHeight), ImVec2(-1, rnd->height() * 0.6f));
		if (ImGui::Begin(title, &ws->searchResultVisible(), flags)) {
			const ImVec2 content = ImGui::GetContentRegionAvail();

			Render("@SR", content);

			context(wnd, rnd, ws);

			ws->searchResultHeight(ImGui::GetWindowSize().y);

			ImGui::End();
		}
	}

	virtual void statusInvalidated(void) override {
		// Do nothing.
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
	void shortcuts(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		if (!ws->canUseShortcuts())
			return;

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed()) {
			_tools.clear();
		}
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
			if (ImGui::MenuItem(ws->theme()->menu_Copy())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
				post(SELECT_ALL);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menu_Jump())) {
				onLineClicked(State.CursorPosition.Line, true);
			}

			ImGui::EndPopup();
		}
	}

	void onLineClicked(int line, bool doubleClicked) {
		if (doubleClicked) {
			const Editing::Tools::SearchResult::Array &found = _workspace->searchResult();
			const int ln = line - 1;
			if (ln >= 0 && ln < (int)found.size()) {
				const Editing::Tools::SearchResult &sr = found[ln];
				const Coordinates start(line, (int)sr.head.length() + 3 /* for " - " */);
				const Coordinates end(line, GetColumnsAt(line));
				SetSelection(start, end);
			}
		}

		if (_callbacks.lineClickedHandler)
			_callbacks.lineClickedHandler(line, doubleClicked);
	}
};

EditorSearchResult* EditorSearchResult::create(void) {
	EditorSearchResultImpl* result = new EditorSearchResultImpl();

	return result;
}

void EditorSearchResult::destroy(EditorSearchResult* ptr) {
	EditorSearchResultImpl* impl = static_cast<EditorSearchResultImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
