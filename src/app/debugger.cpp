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
#include "workspace.h"
#include "../compiler/disassembler.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef DEBUGGER_BANK_SIZE
#	define DEBUGGER_BANK_SIZE 0x4000
#endif /* DEBUGGER_BANK_SIZE */
#ifndef DEBUGGER_START_ADDRESS
#	define DEBUGGER_START_ADDRESS 0x4000
#endif /* DEBUGGER_START_ADDRESS */

/* ===========================================================================} */

/*
** {===========================================================================
** Debugger
*/

class DebuggerImpl : public Debugger {
private:
	struct SrcLocation {
		int page = 0;
		int row = 0;

		SrcLocation() {
		}
		SrcLocation(int pg, int ln) : page(pg), row(ln) {
		}

		bool operator < (const SrcLocation &other) const {
			return compare(other) < 0;
		}

		int compare(const SrcLocation &other) const {
			if (page < other.page)
				return -1;
			else if (page > other.page)
				return 1;

			if (row < other.row)
				return -1;
			else if (row > other.row)
				return 1;

			return 0;
		}
	};
	typedef std::map<SrcLocation, int> SrcToTracePointDictionary;

private:
	bool _opened = false;
	struct {
		float startY = 0;
		int safeHeight = 0;
	} _options;
	Workspace* _workspace = nullptr; // Foreign.
	Device* _device = nullptr; // Foreign.

	const GBBASIC::Program::Compiled* _compiled = nullptr; // Foreign.
	bool _started = false;
	Breakpoint::Array _breakpoints;
	int _vmStepBreakpointCount = 0;
	int _vmStepBreakpointId = -1;
	mutable SrcToTracePointDictionary _srcToTracePoint;

public:
	DebuggerImpl() {
	}
	virtual ~DebuggerImpl() {
		stop();

		close();
	}

	virtual bool open(class Renderer* rnd, class Workspace* ws, class Theme* /* theme */, class Device* device) override {
		if (_opened)
			return true;

		(void)rnd;
		_workspace = ws;
		_device = device;

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		_device->clearBreakpoints();

		_compiled = nullptr;
		_started = false;
		_breakpoints.clear();
		_vmStepBreakpointCount = 0;
		_vmStepBreakpointId = -1;
		_srcToTracePoint.clear();

		_workspace = nullptr;
		_device = nullptr;

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

		_compiled = &_workspace->getCompiledData();

		refreshBreakpoints();

		for (Breakpoint &breakpoint : _breakpoints) {
			if (!breakpoint.enabled)
				continue;

			installBreakpoint(breakpoint);
		}
	}
	virtual void stop(void) override {
		_device->clearBreakpoints();

		_compiled = nullptr;
		_breakpoints.clear();
		_vmStepBreakpointCount = 0;
		_vmStepBreakpointId = -1;
		_srcToTracePoint.clear();

		_started = false;
	}

	virtual void clearBreakpoints(void) override {
		_device->clearBreakpoints();

		_breakpoints.clear();
	}
	virtual void setBreakpoint(int page, int ln, bool brk) override {
		const Breakpoint breakpoint(page, ln, brk);
		Breakpoint::Array::iterator it = std::lower_bound(_breakpoints.begin(), _breakpoints.end(), breakpoint);
		Breakpoint* pointer = nullptr;
		if (it != _breakpoints.end() && !(breakpoint < *it)) {
			it->enabled = brk; // Update the existing breakpoint.
			pointer = &*it;
		} else {
			_breakpoints.push_back(breakpoint); // Add a new breakpoint.
			pointer = &_breakpoints.back();
		}

		if (_started) {
			refreshBreakpoints();

			GBBASIC_ASSERT(pointer && "Impossible.");
			if (pointer) {
				if (brk)
					installBreakpoint(*pointer);
				else
					uninstallBreakpoint(*pointer);
			}
		}
	}
	virtual void removeBreakpoint(int page, int ln) override {
		const Breakpoint breakpoint(page, ln);
		Breakpoint::Array::iterator it = std::lower_bound(_breakpoints.begin(), _breakpoints.end(), breakpoint);
		if (it != _breakpoints.end() && !(breakpoint < *it)) {
			if (it->enabled)
				uninstallBreakpoint(*it);

			_breakpoints.erase(it);
		}
	}

	virtual void breakpointHit(void) override {
		const Device::Registers regs = _device->readRegisters();
		(void)regs;

		// TODO: DBG.
	}

private:
	const GBBASIC::RamLocation::Dictionary* compiledAllocations(void) const {
		if (!_compiled)
			return nullptr;

		return &_compiled->allocations;
	}
	const GBBASIC::SymbolTable* compiledSymbols(void) const {
		if (!_compiled)
			return nullptr;

		return &_compiled->symbols;
	}
	const GBBASIC::TracePoint::Array* compiledTracePoints(void) const {
		if (!_compiled)
			return nullptr;

		return &_compiled->tracePoints;
	}
	Bytes::Ptr compiledBytes(void) const {
		if (!_compiled)
			return nullptr;

		return _compiled->bytes;
	}

	const GBBASIC::RomLocation* getRomLocationBySymbolName(const std::string &symbol) const {
		const GBBASIC::SymbolTable* symbols = compiledSymbols();
		if (!symbols)
			return nullptr;

		const GBBASIC::RomLocation* result = symbols->find(symbol);

		return result;
	}
	const GBBASIC::TracePoint* getTracePointBySourceLocation(int page, int ln) const {
		const GBBASIC::TracePoint::Array* tracePoints = compiledTracePoints();
		if (!tracePoints)
			return nullptr;

		if (_srcToTracePoint.empty()) {
			for (int i = 0; i < (int)tracePoints->size(); ++i) {
				const GBBASIC::TracePoint &tp = (*tracePoints)[i];
				const SrcLocation key(tp.inCode.page, tp.inCode.row);
				_srcToTracePoint[key] = i;
			}
		}

		const SrcLocation key(page, ln);
		SrcToTracePointDictionary::const_iterator it = _srcToTracePoint.find(key);
		if (it == _srcToTracePoint.end())
			return nullptr;

		const int idx = it->second;
		if (idx < 0 || idx >= (int)tracePoints->size())
			return nullptr;

		return &(*tracePoints)[idx];
	}
	GBBASIC::Disassembler::Mnemonic::Array getDisassembledMnemonics(void) const {
		GBBASIC::Disassembler::Mnemonic::Array result;
		GBBASIC::Disassembler::Ptr dasm(GBBASIC::Disassembler::create());

		GBBASIC::Disassembler::DisassemblingOptions options;
		options.bankSize = DEBUGGER_BANK_SIZE;
		options.startAddress = DEBUGGER_START_ADDRESS;
		options.bank = 0;
		options.addressCursor = 0;
		dasm->disassemble(result, compiledBytes(), options);

		return result;
	}

	void refreshBreakpoints(void) {
		// Sort the breakpoints.
		std::sort(_breakpoints.begin(), _breakpoints.end());

		// Re-assign breakpoints' type if needed.
		for (Breakpoint &breakpoint : _breakpoints) {
			if (breakpoint.type != Breakpoint::Types::NONE)
				continue;

			const GBBASIC::TracePoint* tp = getTracePointBySourceLocation(breakpoint.page, breakpoint.line);
			if (!tp)
				continue;

			if (tp->inRom.type == GBBASIC::RomLocation::Types::BASIC)
				breakpoint.type = Breakpoint::Types::BASIC;
			else
				breakpoint.type = Breakpoint::Types::ASM;
		}
	}
	bool installBreakpoint(Breakpoint &breakpoint) {
		if (breakpoint.type == Breakpoint::Types::NONE) {
			return false;
		}

		if (breakpoint.type == Breakpoint::Types::ASM) {
			const GBBASIC::TracePoint* tp = getTracePointBySourceLocation(breakpoint.page, breakpoint.line);
			if (!tp)
				return false;

			const UInt8 bank = (UInt8)tp->inRom.bank;
			const UInt16 address = (UInt16)tp->inRom.address;
			const int id = _device->addBreakpoint(bank, address);
			breakpoint.id = id;

			return true;
		}

		const GBBASIC::RomLocation* vmStepInRom = getRomLocationBySymbolName(COMPILER_VM_STEP_ENTRY_NAME);
		if (!vmStepInRom)
			return false;

		if (_vmStepBreakpointCount++ == 0) {
			const UInt8 bank = (UInt8)vmStepInRom->bank;
			const UInt16 address = (UInt16)vmStepInRom->address;
			const int id = _device->addBreakpoint(bank, address);
			_vmStepBreakpointId = id;
		}
		breakpoint.id = _vmStepBreakpointId;

		return true;
	}
	bool uninstallBreakpoint(Breakpoint &breakpoint) {
		if (breakpoint.type == Breakpoint::Types::NONE) {
			return false;
		}

		if (breakpoint.type == Breakpoint::Types::ASM) {
			_device->removeBreakpoint(breakpoint.id);
			breakpoint.id = -1;

			return true;
		}

		const GBBASIC::RomLocation* vmStepInRom = getRomLocationBySymbolName(COMPILER_VM_STEP_ENTRY_NAME);
		if (!vmStepInRom)
			return false;

		if (--_vmStepBreakpointCount == 0) {
			_device->removeBreakpoint(_vmStepBreakpointId);
			_vmStepBreakpointId = -1;
			breakpoint.id = -1;
		}

		return true;
	}
	void debug(void) {
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

Debugger::Breakpoint::Breakpoint(int pg, int ln) :
	page(pg), line(ln)
{
}

Debugger::Breakpoint::Breakpoint(int pg, int ln, bool enabled_) :
	page(pg), line(ln),
	enabled(enabled_)
{
}

bool Debugger::Breakpoint::operator < (const Breakpoint &other) const {
	return compare(other) < 0;
}

int Debugger::Breakpoint::compare(const Breakpoint &other) const {
	if (page < other.page)
		return -1;
	else if (page > other.page)
		return 1;

	if (line < other.line)
		return -1;
	else if (line > other.line)
		return 1;

	// `enabled` doesn't count.
	// `type` doesn't count.

	return 0;
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
