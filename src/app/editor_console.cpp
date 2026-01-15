/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_console.h"
#include "theme.h"
#include "workspace.h"
#include "../../lib/imgui_code_editor/imgui_code_editor.h"
#include <SDL.h>

/*
** {===========================================================================
** Console editor
*/

class EditorConsoleImpl : public EditorConsole, public ImGui::CodeEditor {
private:
	bool _opened = false;

	struct Tools {
		bool focused = false;
		mutable std::string cache;

		void clear(void) {
			focused = false;
			cache.clear();
		}
	} _tools;

	struct Callbacks {
		EditorConsole::LineClickedHandler lineClickedHandler = nullptr;
	} _callbacks;

public:
	EditorConsoleImpl() {
		SetLanguageDefinition(ImGui::CodeEditor::LanguageDefinition::Text());
	}
	virtual ~EditorConsoleImpl() override {
		close(-1);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* wnd, class Renderer* rnd, class Workspace*, class Project* /* project */, int /* index */, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		SetStickyLineNumbers(true);

		DisableShortcut((ImGui::CodeEditor::ShortcutType)((unsigned)ImGui::CodeEditor::UndoRedo | (unsigned)ImGui::CodeEditor::IndentUnindent));

		SetReadOnly(true);

		SetShowLineNumbers(false);

		SetStickyLineNumbers(true);

		SetShowWhiteSpaces(false);

		SetTooltipEnabled(false);

		const int scale = rnd->scale() / wnd->scale();
		SetImePositionUpdatedHandler(
			[scale] (const ImVec2 &pos) -> ImVec2 {
				return ImVec2(pos.x * scale, pos.y * scale);
			}
		);

		fprintf(stdout, "Console opened.\n");
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Console closed.\n");

		_tools.clear();
	}

	virtual int index(void) const override {
		return -1;
	}

	virtual void enter(class Workspace* ws) override {
		do {
			LockGuard<decltype(ws->consoleLock())> guard(ws->consoleLock());

			if (ws->consoleHasWarning())
				ws->consoleHasWarning(false);
			if (ws->consoleHasError())
				ws->consoleHasError(false);
		} while (false);
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
	}

	virtual void appendText(const char* txt, unsigned col) override {
		AppendText(txt, (ImU32)col);
	}
	virtual void moveBottom(bool select) override {
		MoveBottom(select);
	}

	virtual void setLineClickedHandler(EditorConsole::LineClickedHandler handler) override {
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
		float x, float y, float width, float height,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

		const ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(
			ImVec2(width, height),
			ImGuiCond_Always
		);
		if (ImGui::Begin(title, nullptr, flags)) {
			const ImVec2 content = ImGui::GetContentRegionAvail();

			do {
				LockGuard<decltype(ws->consoleLock())> guard(ws->consoleLock());

				Render("@Cnsl", content);
			} while (false);

			context(wnd, rnd, ws);

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

			ImGui::EndPopup();
		}
	}

	void onLineClicked(int line, bool doubleClicked) const {
		if (_callbacks.lineClickedHandler)
			_callbacks.lineClickedHandler(line, doubleClicked);
	}
};

EditorConsole* EditorConsole::create(void) {
	EditorConsoleImpl* result = new EditorConsoleImpl();

	return result;
}

void EditorConsole::destroy(EditorConsole* ptr) {
	EditorConsoleImpl* impl = static_cast<EditorConsoleImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
