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

/*
** {===========================================================================
** Font editor
*/

class EditorI18nImpl : public EditorI18n {
private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;

public:
	EditorI18nImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			;
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

		// TODO
		(void)ws;

		fprintf(stdout, "Font editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Font editor closed: #%d.\n", _index);

		_project = nullptr;
		_index = -1;

		// TODO
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
	virtual void readonly(bool /* ro */) override {
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
		// TODO
	}
	virtual void cut(void) override {
		// TODO
	}
	virtual bool pastable(void) const override {
		// TODO

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		// TODO
	}
	virtual void del(bool) override {
		// TODO
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

		// TODO

		_project->toPollEditor(true);
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		// TODO

		_project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		(void)msg;
		(void)argc;
		(void)argv;

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		const char* title,
		float /* x */, float y, float width, float height,
		double /* delta */
	) override {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws);

		// TODO
		(void)io;
		(void)style;
		(void)title;
		(void)y;
		(void)width;
		(void)height;
	}

	virtual void statusInvalidated(void) override {
		// TODO
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
		(void)wnd;
		(void)rnd;
		(void)ws;

		// TODO
	}

	void context(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		// TODO
		(void)style;
		(void)ws;
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		// TODO
		(void)ws;
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height, bool actived) {
		ImGuiStyle &style = ImGui::GetStyle();

		// TODO
		(void)wnd;
		(void)rnd;
		(void)ws;
		(void)width;
		(void)height;
		(void)actived;
		(void)style;
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd) {
		// TODO
		(void)ws;
		(void)cmd;
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
