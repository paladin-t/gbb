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

#ifndef DEBUGGER_ACTOR_MAX_ANIMATIONS
#	define DEBUGGER_ACTOR_MAX_ANIMATIONS 8
#endif /* DEBUGGER_ACTOR_MAX_ANIMATIONS */
#ifndef DEBUGGER_PROJECTILE_MAX_ANIMATIONS
#	define DEBUGGER_PROJECTILE_MAX_ANIMATIONS 4
#endif /* DEBUGGER_PROJECTILE_MAX_ANIMATIONS */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

/**< Shared between the VM and the compiler. */

#pragma pack(push, 1)

namespace VM {

typedef UInt8 Boolean;
typedef UInt16 Pointer;
typedef Pointer UInt8Ptr;
typedef Pointer UInt16Ptr;
typedef Pointer CtxPtr;
typedef Pointer MetaSpriteRef;

struct SCRIPT_CTX {
	typedef Pointer Ptr;

	UInt8Ptr PC;
	UInt8 bank;
	Ptr next;
	UInt16Ptr stack_ptr;
	UInt16Ptr base_addr;
	UInt8 ID;
	UInt16Ptr hthread;
	Boolean terminated;
	Boolean waitable;
	UInt8 lock_count;
	Pointer update_fn;
	UInt8 update_fn_bank;

	SCRIPT_CTX() {
		memset(this, 0, sizeof(SCRIPT_CTX));
	}
};

struct actor_t {
	typedef Pointer Ptr;

	Boolean instantiated         : 1;
	Boolean active               : 1;
	Boolean enabled              : 1;
	Boolean hidden               : 1;
	Boolean pinned               : 1;
	Boolean persistent           : 1;
	Boolean animation_loop       : 1;
	Boolean movement_interrupt   : 1;
	UInt8 template_;
	upoint16_t position;
	UInt8 direction;
	boundingbox_t bounds;
	UInt8 base_tile;
	UInt8 sprite_bank;
	MetaSpriteRef sprite_frames;
	UInt8 animation;
	UInt8 animation_interval;
	animation_t animations[DEBUGGER_ACTOR_MAX_ANIMATIONS];
	UInt8 frame;
	UInt8 motion;
	UInt8 move_speed;
	union {
		UINT32 movement;
		upoint16_t absolute_movement;
		struct {
			point8_t relative_movement;
			UInt8 original_move_speed;
			UInt8 max_move_speed;
		};
	};
	UInt8 behaviour;
	UInt8 collision_group;
	UInt16 behave_thread_id;
	UInt8 behave_handler_bank;
	UInt8Ptr behave_handler_address;
	UInt16 hit_thread_id;
	UInt8 hit_handler_bank;
	UInt8Ptr hit_handler_address;
	Ptr next;
	Ptr prev;

	actor_t() {
		memset(this, 0, sizeof(actor_t));
	}
};

struct projectile_def_t {
	boundingbox_t bounds;
	UInt8 base_tile;
	UInt8 sprite_bank;
	MetaSpriteRef sprite_frames;
	UInt8 animation_interval;
	animation_t animations[DEBUGGER_PROJECTILE_MAX_ANIMATIONS];
	UInt8 life_time;
	UInt8 move_speed;
	UInt16 initial_offset;
	UInt8 collision_group;

	projectile_def_t() {
	}
};

struct projectile_t {
	typedef Pointer Ptr;

	Boolean animation_no_loop   : 1;
	Boolean strong              : 1;
	Boolean reserved1           : 1;
	Boolean reserved2           : 1;
	Boolean reserved3           : 1;
	Boolean reserved4           : 1;
	Boolean reserved5           : 1;
	Boolean reserved6           : 1;
	upoint16_t position;
	point16_t movement;
	UInt8 frame;
	UInt8 animation;
	projectile_def_t def;
	Ptr next;

	projectile_t() {
		memset(this, 0, sizeof(projectile_t));
	}
};

struct trigger_t {
	UInt8 x;
	UInt8 y;
	UInt8 width;
	UInt8 height;
	UInt8 hit_handler_flags;
	UInt8 hit_handler_bank;
	UInt8Ptr hit_handler_address;

	trigger_t() {
		memset(this, 0, sizeof(trigger_t));
	}
};

struct scene_t {
	Boolean is_16x16_grid         : 1;
	Boolean is_16x16_player       : 1;
	Boolean clamp_camera          : 1;
	Boolean player_on_ladder      : 1;
	Boolean reserved1             : 1;
	Boolean reserved2             : 1;
	Boolean reserved3             : 1;
	Boolean reserved4             : 1;
	UInt8 gravity;
	UInt8 jump_gravity;
	UInt8 jump_max_count;
	UInt8 jump_max_ticks;
	UInt8 climb_velocity;
	UInt8 player_can_jump;
	UInt8 player_jump_ticks;
	Int16 player_velocity_y;
	UInt8 width;
	UInt8 height;
	UInt8 base_tile;
	UInt8 map_bank;
	UInt8Ptr map_address;
	UInt8 attr_bank;
	UInt8Ptr attr_address;
	UInt8 prop_bank;
	UInt8Ptr prop_address;
	UInt8 actor_bank;
	UInt8Ptr actor_address;
	UInt8 trigger_bank;
	UInt8Ptr trigger_address;

	scene_t() {
		memset(this, 0, sizeof(scene_t));
	}
};

}

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
		// Resolve the CPU bank and PC.
		const Device::Registers regs = _device->readRegisters();
		const UInt16 pc = regs.PC;
		UInt8 bank = 0;
		bool gotBank = false;
		if (!_currentBankPointer.invalid()) {
			if (_device->readRam((UInt16)_currentBankPointer.address, &bank))
				gotBank = true;
		}
		if (!gotBank) {
			bank = (UInt8)_device->currentBank();
			gotBank = true;
		}
		if (!gotBank)
			return false;

		// Resolve the VM bank and PC if necessary.
		const bool isBasic = _vmStepPointer.equals(bank, pc);
		UInt16 ctxPc = 0;
		UInt8 ctxBank = 0;
		if (isBasic) {
			const UInt16 currCtx = regs.DE; // `DE` is the pointer to the current `VM::SCRIPT_CTX`.
			if (!probeThreadProgramCounter(currCtx, ctxBank, ctxPc))
				return false;
		}

		// Traverse and check all breakpoints.
		int hitCount = 0;
		for (int i = 0; i < (int)_breakpoints.size(); ++i) {
			const Breakpoint &breakpoint = _breakpoints[i];
			if (!breakpoint.hitPointer.equals(bank, pc))
				continue;

			if (isBasic) {
				if (breakpoint.vmPointer.equals(ctxBank, ctxPc)) {
					hitBreakpoint(breakpoint);
					++hitCount;
				}
			} else {
				hitBreakpoint(breakpoint);
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

	bool probeThreadProgramCounter(UInt16 address, UInt8 &bank, UInt16 &pc) const {
		bank = 0;
		pc = 0;

		UInt16 ctxPc = 0;
		UInt8 ctxBank = 0;
		constexpr const int pcOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, PC);
		constexpr const int bankOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, bank);
		const int ctxPcAddress = address + pcOffset;
		const int ctxBankAddress = address + bankOffset;
		if (!_device->readRam((UInt16)ctxPcAddress, &ctxPc))
			return false;
		if (!_device->readRam((UInt16)ctxBankAddress, &ctxBank))
			return false;

		bank = ctxBank;
		pc = ctxPc;

		return true;
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
	void hitBreakpoint(const Breakpoint &breakpoint) {
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
