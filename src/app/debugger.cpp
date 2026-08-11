/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "debugger.h"
#include "theme.h"
#include "widgets.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"

/*
** {===========================================================================
** Debugger
*/

class DebuggerImpl : public Debugger {
private:
	bool _opened = false;
	struct {
		float startY = 0;
		int safeHeight = 0;
	} _options;
	Device* _device = nullptr; // Foreign.

	bool _started = false;
	Breakpoint::Array _breakpoints;

public:
	DebuggerImpl() {
	}
	virtual ~DebuggerImpl() {
		stop();

		close();
	}

	virtual bool open(class Renderer* rnd, class Theme* /* theme */, class Device* device) override {
		if (_opened)
			return true;

		(void)rnd;
		_device = device;

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		_started = false;
		_breakpoints.clear();

		_opened = false;

		return true;
	}

	virtual int safeHeight(void) const override {
		return _options.safeHeight;
	}

	virtual void update(
		class Renderer* rnd, class Theme* theme,
		bool visible
	) override {
		debug();

		if (!visible)
			return;

		begin(rnd, theme);
		{
			ImGui::TextUnformatted("DBG");

			// TODO: DBG.
		}
		end(rnd);
	}

	virtual void start(void) override {
		_started = true;
	}
	virtual void stop(void) override {
		_started = false;
	}

	virtual void clearBreakpoints(void) override {
		_breakpoints.clear();
	}
	virtual void setBreakpoint(int page, int ln, bool brk) override {
		_breakpoints.push_back(Breakpoint(brk, page, ln));

		if (brk)
			installBreakpoint(page, ln);
		else
			uninstallBreakpoint(page, ln);
	}

private:
	void debug(void) {
		// TODO: DBG.
	}
	void installBreakpoint(int page, int ln) {
		// TODO: DBG.
	}
	void uninstallBreakpoint(int page, int ln) {
		// TODO: DBG.
	}

	void begin(Renderer* /* rnd */, Theme* theme) {
		_options.startY = ImGui::GetCursorPosY();

		ImGui::AlignTextToFramePadding();
		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::TextUnformatted(theme->windowEmulator_CodeDebugger());
	}
	void end(Renderer* /* rnd */) {
		_options.safeHeight = (int)(ImGui::GetCursorPosY() - _options.startY + 48);
	}
};

Debugger::Breakpoint::Breakpoint() {
}

Debugger::Breakpoint::Breakpoint(bool enabled_, int pg, int ln) :
	enabled(enabled_), page(pg), line(ln)
{
}

Debugger* Debugger::create(void) {
	DebuggerImpl* result = new DebuggerImpl();

	return result;
}

void Debugger::destroy(Debugger* ptr) {
	DebuggerImpl* impl = static_cast<DebuggerImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
