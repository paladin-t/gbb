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
** Utilities
*/

/**< Shared between the VM and the compiler. */

#pragma pack(push, 1)

typedef UInt8 Boolean;
typedef UInt16 Pointer;
typedef Pointer UInt8Ptr;
typedef Pointer UInt16Ptr;
typedef Pointer CtxPtr;

struct SCRIPT_CTX {
	// Program pointer.
	const UInt8Ptr PC = NULL;
	UInt8 bank = 0;
	// Linked list of contexts for cooperative multitasking.
	CtxPtr next = NULL;
	// VM stack pointer.
	UInt16Ptr stack_ptr = NULL;
	UInt16Ptr base_addr = NULL;
	// Thread control.
	UInt8 ID = 0;
	UInt16Ptr hthread = NULL;
	Boolean terminated = 0;
	// Waitable state.
	Boolean waitable = 0;
	// Lock state.
	UInt8 lock_count = 0;
	// Update function.
	Pointer update_fn = NULL;
	UInt8 update_fn_bank = 0;

	SCRIPT_CTX() {
	}
};

#pragma pack(pop)

/* ===========================================================================} */

/*
** {===========================================================================
** Debugger
*/

class DebuggerImpl : public Debugger {
private:
	struct SourceRef {
		int page = 0;
		int row = 0;

		SourceRef() {
		}
		SourceRef(int pg, int ln) : page(pg), row(ln) {
		}

		bool operator < (const SourceRef &other) const {
			return compare(other) < 0;
		}

		int compare(const SourceRef &other) const {
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
	typedef std::map<SourceRef, int> SourceRefToTracePointDictionary;

private:
	bool _opened = false;
	struct {
		float startY = 0;
		int safeHeight = 0;
	} _options;
	Workspace* _workspace = nullptr; // Foreign.
	Device* _device = nullptr; // Foreign.

	bool _started = false;
	const GBBASIC::Program::Compiled* _compiled = nullptr; // Foreign.
	Breakpoint::Array _breakpoints;
	mutable SourceRefToTracePointDictionary _srcToTracePoint; // Reversed mapping from trace points.
	FarPtr _rRom0Pointer;
	FarPtr _currentBankPointer;
	int _vmStepBreakpointRefCount = 0;
	int _vmStepBreakpointId = -1;
	FarPtr _vmStepPointer;

	bool _bringCodeDebuggerToFront = false;

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

		_bringCodeDebuggerToFront = false;

		_started = false;
		_compiled = nullptr;
		_breakpoints.clear();
		_srcToTracePoint.clear();
		_rRom0Pointer = FarPtr();
		_currentBankPointer = FarPtr();
		_vmStepBreakpointRefCount = 0;
		_vmStepBreakpointId = -1;
		_vmStepPointer = FarPtr();

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
		debug(visible);

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
		// Prepare.
		if (_started)
			return;

		_started = true;

		_compiled = &_workspace->getCompiledData();

		// Resolve the ROM entries.
		const GBBASIC::RomLocation* rRom0InRom = getRomLocationBySymbolName(COMPILERFREE_RROM0_ENTRY_NAME);
		if (rRom0InRom) {
			_rRom0Pointer.bank = rRom0InRom->bank;
			_rRom0Pointer.address = rRom0InRom->address;
		}

		const GBBASIC::RomLocation* currentBankInRom = getRomLocationBySymbolName(COMPILERFREE_CURRENT_BANK_ENTRY_NAME);
		if (currentBankInRom) {
			_currentBankPointer.bank = currentBankInRom->bank;
			_currentBankPointer.address = currentBankInRom->address;
		}

		const GBBASIC::RomLocation* vmStepInRom = getRomLocationBySymbolName(COMPILER_VM_STEP_ENTRY_NAME);
		if (vmStepInRom) {
			_vmStepPointer.bank = vmStepInRom->bank;
			_vmStepPointer.address = vmStepInRom->address;
		}

		// Refresh the breakpoints.
		refreshBreakpoints();

		// Install all enabled breakpoints.
		for (Breakpoint &breakpoint : _breakpoints) {
			if (!breakpoint.enabled)
				continue;

			installBreakpoint(breakpoint);
		}
	}
	virtual void stop(void) override {
		if (!_started)
			return;

		if (_device)
			_device->clearBreakpoints();

		_bringCodeDebuggerToFront = false;

		_compiled = nullptr;
		_breakpoints.clear();
		_srcToTracePoint.clear();
		_rRom0Pointer = FarPtr();
		_currentBankPointer = FarPtr();
		_vmStepBreakpointRefCount = 0;
		_vmStepBreakpointId = -1;
		_vmStepPointer = FarPtr();

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

	virtual bool breakpointHit(void) override {
		const Device::Registers regs = _device->readRegisters();
		const UInt16 pc = regs.PC;
		UInt8 bank = 0;
		bool gotBank = false;
		if (!_currentBankPointer.invalid()) {
			if (_device->readRam((UInt16)_currentBankPointer.address, &bank))
				gotBank = true;
		}
		if (!gotBank && !_rRom0Pointer.invalid()) {
			if (_device->readRam((UInt16)_rRom0Pointer.address, &bank))
				gotBank = true;
		}
		if (!gotBank)
			return false;

		const bool isBasic = _vmStepPointer.equals(bank, pc);
		UInt16 ctxPc = 0;
		UInt8 ctxBank = 0;
		if (isBasic) {
			const UInt16 currCtx = regs.DE; // `DE` is the pointer to the current `SCRIPT_CTX`.
			constexpr const int pcOffset = GBBASIC_OFFSETOF(SCRIPT_CTX, PC);
			constexpr const int bankOffset = GBBASIC_OFFSETOF(SCRIPT_CTX, bank);
			const int currCtxPcAddress = currCtx + pcOffset;
			const int currCtxBankAddress = currCtx + bankOffset;
			if (!_device->readRam((UInt16)currCtxPcAddress, &ctxPc))
				return false;
			if (!_device->readRam((UInt16)currCtxBankAddress, &ctxBank))
				return false;
		}

		int hitCount = 0;
		for (int i = 0; i < (int)_breakpoints.size(); ++i) {
			const Breakpoint &breakpoint = _breakpoints[i];
			if (!breakpoint.hitPointer.equals(bank, pc))
				continue;

			if (isBasic) {
				if (breakpoint.vmPointer.equals(ctxBank, ctxPc)) {
					triggerBreakpoint(breakpoint);
					++hitCount;
				}
			} else {
				triggerBreakpoint(breakpoint);
				++hitCount;
			}
		}

		return hitCount > 0;
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
				const SourceRef key(tp.inCode.page, tp.inCode.row);
				_srcToTracePoint[key] = i;
			}
		}

		const SourceRef key(page, ln);
		SourceRefToTracePointDictionary::const_iterator it = _srcToTracePoint.find(key);
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

			const GBBASIC::TracePoint* tp = getTracePointBySourceLocation(breakpoint.page, breakpoint.row);
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

		const GBBASIC::TracePoint* tp = getTracePointBySourceLocation(breakpoint.page, breakpoint.row);
		if (!tp)
			return false;

		if (breakpoint.type == Breakpoint::Types::ASM) {
			const UInt8 bank = (UInt8)tp->inRom.bank;
			const UInt16 address = (UInt16)tp->inRom.address;
			const int id = _device->addBreakpoint(bank, address);
			breakpoint.id = id;
			breakpoint.hitPointer = FarPtr(bank, address);
			breakpoint.vmPointer = FarPtr();

			return true;
		}

		if (_vmStepPointer.invalid())
			return false;

		if (_vmStepBreakpointRefCount++ == 0) {
			const UInt8 bank = (UInt8)_vmStepPointer.bank;
			const UInt16 address = (UInt16)_vmStepPointer.address;
			const int id = _device->addBreakpoint(bank, address);
			_vmStepBreakpointId = id;
		}
		breakpoint.id = _vmStepBreakpointId;
		breakpoint.hitPointer = _vmStepPointer;
		breakpoint.vmPointer = FarPtr(tp->inRom.bank, tp->inRom.address);

		return true;
	}
	bool uninstallBreakpoint(Breakpoint &breakpoint) {
		if (breakpoint.type == Breakpoint::Types::NONE) {
			return false;
		}

		if (breakpoint.type == Breakpoint::Types::ASM) {
			_device->removeBreakpoint(breakpoint.id);
			breakpoint.id = -1;
			breakpoint.hitPointer = FarPtr();
			breakpoint.vmPointer = FarPtr();

			return true;
		}

		if (_vmStepPointer.invalid())
			return false;

		if (--_vmStepBreakpointRefCount == 0) {
			_device->removeBreakpoint(_vmStepBreakpointId);
			_vmStepBreakpointId = -1;
			breakpoint.id = -1;
			breakpoint.hitPointer = FarPtr();
			breakpoint.vmPointer = FarPtr();
		}

		return true;
	}
	void triggerBreakpoint(const Breakpoint &breakpoint) {
		_bringCodeDebuggerToFront = true;

		// TODO: DBG.
		(void)breakpoint;
	}
	void debug(bool visible) {
		if (_bringCodeDebuggerToFront) {
			_bringCodeDebuggerToFront = false;
			if (!visible)
				_workspace->bringCodeDebuggerToFront(true);
		}

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

Debugger::FarPtr::FarPtr() {
}

Debugger::FarPtr::FarPtr(int b, int addr) : bank(b), address(addr) {
}

bool Debugger::FarPtr::equals(int b, int addr) const {
	return (bank == 0 || bank == b) && address == addr;
}

bool Debugger::FarPtr::invalid(void) const {
	return bank == -1 || address == -1;
}

Debugger::Breakpoint::Breakpoint() {
}

Debugger::Breakpoint::Breakpoint(int pg, int ln) :
	page(pg), row(ln)
{
}

Debugger::Breakpoint::Breakpoint(int pg, int ln, bool enabled_) :
	page(pg), row(ln),
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

	if (row < other.row)
		return -1;
	else if (row > other.row)
		return 1;

	// `enabled` doesn't count.
	// `type` doesn't count.
	// `id` doesn't count.
	// `bank` doesn't count.
	// `address` doesn't count.

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
