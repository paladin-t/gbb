/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "debugger.h"
#include "editor_code.h"
#include "theme.h"
#include "widgets.h"
#include "workspace.h"
#include "../compiler/disassembler.h"
#include "../utils/datetime.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"

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

#ifndef DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT
#	define DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT DateTime::fromSeconds(0.333333);
#endif /* DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

/**< Shared between the VM and the compiler. */

namespace VM {

template<typename T> struct Reference {
	typedef T ValueType;

	ValueType data;
	Debugger::FarPtr pointer;

	Reference() {
	}
	Reference(const ValueType &d, const Debugger::FarPtr &ptr) :
		data(d),
		pointer(ptr)
	{
	}
};

}

#pragma pack(push, 1)

namespace VM {

typedef UInt8 Boolean;
typedef UInt16 Pointer;
typedef Pointer UInt8Ptr;
typedef Pointer UInt16Ptr;
typedef Pointer CtxPtr;
typedef Pointer MetaSpriteRef;

typedef std::vector<Byte> Buffer;

struct ThreadStack {
	typedef Reference<ThreadStack> Ref;
	typedef std::vector<Ref> Array;

	VM::Buffer buffer;
	int count = 0;

	ThreadStack() {
	}
};

struct SCRIPT_CTX {
	typedef Reference<SCRIPT_CTX> Ref;
	typedef std::vector<Ref> Array;
	typedef Pointer Ptr;

	UInt8Ptr PC = NULL;
	UInt8 bank = 0;
	Ptr next = NULL;
	UInt16Ptr stack_ptr = NULL;
	UInt16Ptr base_addr = NULL;
	UInt8 ID = 0;
	UInt16Ptr hthread = NULL;
	Boolean terminated = 0;
	Boolean waitable = 0;
	UInt8 lock_count = 0;
	Pointer update_fn = NULL;
	UInt8 update_fn_bank = 0;

	SCRIPT_CTX() {
	}
};

struct actor_t {
	typedef Reference<actor_t> Ref;
	typedef std::vector<Ref> Array;
	typedef Pointer Ptr;

	Boolean instantiated         : 1;
	Boolean active               : 1;
	Boolean enabled              : 1;
	Boolean hidden               : 1;
	Boolean pinned               : 1;
	Boolean persistent           : 1;
	Boolean animation_loop       : 1;
	Boolean movement_interrupt   : 1;
	UInt8 template_ = 0;
	upoint16_t position;
	UInt8 direction = 0;
	boundingbox_t bounds;
	UInt8 base_tile = 0;
	UInt8 sprite_bank = 0;
	MetaSpriteRef sprite_frames = NULL;
	UInt8 animation = 0;
	UInt8 animation_interval = 0;
	animation_t animations[DEBUGGER_ACTOR_MAX_ANIMATIONS];
	UInt8 frame = 0;
	UInt8 motion = 0;
	UInt8 move_speed = 0;
	union {
		UInt32 movement;
		upoint16_t absolute_movement;
		struct {
			point8_t relative_movement;
			UInt8 original_move_speed;
			UInt8 max_move_speed;
		};
	};
	UInt8 behaviour = 0;
	UInt8 collision_group = 0;
	UInt16 behave_thread_id = 0;
	UInt8 behave_handler_bank = 0;
	UInt8Ptr behave_handler_address = NULL;
	UInt16 hit_thread_id = 0;
	UInt8 hit_handler_bank = 0;
	UInt8Ptr hit_handler_address = NULL;
	Ptr next = NULL;
	Ptr prev = NULL;

	actor_t() :
		instantiated(0),
		active(0),
		enabled(0),
		hidden(0),
		pinned(0),
		persistent(0),
		animation_loop(0),
		movement_interrupt(0),
		movement(0)
	{
	}
};

struct projectile_def_t {
	typedef Reference<projectile_def_t> Ref;
	typedef std::vector<Ref> Array;

	boundingbox_t bounds;
	UInt8 base_tile = 0;
	UInt8 sprite_bank = 0;
	MetaSpriteRef sprite_frames = NULL;
	UInt8 animation_interval = 0;
	animation_t animations[DEBUGGER_PROJECTILE_MAX_ANIMATIONS];
	UInt8 life_time = 0;
	UInt8 move_speed = 0;
	UInt16 initial_offset = 0;
	UInt8 collision_group = 0;

	projectile_def_t() {
	}
};

struct projectile_t {
	typedef Reference<projectile_t> Ref;
	typedef std::vector<Ref> Array;
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
	UInt8 frame = 0;
	UInt8 animation = 0;
	projectile_def_t def;
	Ptr next = NULL;

	projectile_t() :
		animation_no_loop(0),
		strong(0),
		reserved1(0),
		reserved2(0),
		reserved3(0),
		reserved4(0),
		reserved5(0),
		reserved6(0)
	{
	}
};

struct trigger_t {
	typedef Reference<trigger_t> Ref;
	typedef std::vector<Ref> Array;

	UInt8 x = 0;
	UInt8 y = 0;
	UInt8 width = 0;
	UInt8 height = 0;
	UInt8 hit_handler_flags = 0;
	UInt8 hit_handler_bank = 0;
	UInt8Ptr hit_handler_address = NULL;

	trigger_t() {
	}
};

struct scene_t {
	typedef Reference<scene_t> Ref;

	Boolean is_16x16_grid      : 1;
	Boolean is_16x16_player    : 1;
	Boolean clamp_camera       : 1;
	Boolean player_on_ladder   : 1;
	Boolean reserved1          : 1;
	Boolean reserved2          : 1;
	Boolean reserved3          : 1;
	Boolean reserved4          : 1;
	UInt8 gravity = 0;
	UInt8 jump_gravity = 0;
	UInt8 jump_max_count = 0;
	UInt8 jump_max_ticks = 0;
	UInt8 climb_velocity = 0;
	UInt8 player_can_jump = 0;
	UInt8 player_jump_ticks = 0;
	Int16 player_velocity_y = 0;
	UInt8 width = 0;
	UInt8 height = 0;
	UInt8 base_tile = 0;
	UInt8 map_bank = 0;
	UInt8Ptr map_address = NULL;
	UInt8 attr_bank = 0;
	UInt8Ptr attr_address = NULL;
	UInt8 prop_bank = 0;
	UInt8Ptr prop_address = NULL;
	UInt8 actor_bank = 0;
	UInt8Ptr actor_address = NULL;
	UInt8 trigger_bank = 0;
	UInt8Ptr trigger_address = NULL;

	scene_t() :
		is_16x16_grid(0),
		is_16x16_player(0),
		clamp_camera(0),
		player_on_ladder(0),
		reserved1(0),
		reserved2(0),
		reserved3(0),
		reserved4(0)
	{
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

	struct RuntimeConfig {
		int heapSize = 0; // In words.
		int stackSize = 0; // In words.

		int actorMaxCount = 0;
		int projectileDefMaxCount = 0;
		int projectileMaxCount = 0;
		int triggerMaxCount = 0;

		RuntimeConfig() {
		}
	};

	struct Snapshot {
		VM::SCRIPT_CTX::Array threads; // Active only.
		VM::ThreadStack::Array threadStacks; // Active only.
		VM::Buffer heap;
		VM::actor_t::Array actors; // Active only.
		VM::projectile_def_t::Array projectileDefs;
		VM::projectile_t::Array projectiles;
		VM::trigger_t::Array triggers;
		VM::scene_t scene;

		Snapshot() {
		}

		void reset(void) {
			threads.clear();
			threadStacks.clear();
			heap.clear();
			actors.clear();
			projectileDefs.clear();
			projectiles.clear();
			triggers.clear();
			scene = VM::scene_t();
		}
	};

private:
	bool _opened = false;
	struct {
		float startY = 0;
		int safeHeight = 0;
	} _options;
	Window* _window = nullptr; // Foreign.
	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	Device* _device = nullptr; // Foreign.

	bool _started = false;
	const GBBASIC::Program::Compiled* _compiled = nullptr; // Foreign.
	RuntimeConfig _runtimeConfig;
	Breakpoint::Array _breakpoints;
	Breakpoint _breakAtNextStep = Breakpoint(-1, -1, false);
	bool _breakAtNextStepInstalledForBasic = false;
	FarPtr _ignoreForBreakingAtNextStep;
	long long _breakTimeout = 0;
	mutable SourceRefToTracePointDictionary _srcToTracePoint; // Reversed mapping from trace points.
	FarPtr _currentBankPointer;
	FarPtr _vmStepPointer;
	int _vmStepBreakpointRefCount = 0;
	int _vmStepBreakpointId = -1;
	FarPtr _latestVmStepInstructionAddress;

	bool _bringCodeDebuggerToFront = false;
	Categories _bringCategoryToFront = Categories::NONE;
	bool _bringProgramCursorToFront = false;
	bool _inspecting = false;
	int _activeCodePage = -1;
	Snapshot _snapshot;
	Device::Registers _registers;
	GBBASIC::Disassembler::Mnemonic::Array _mnemonics;
	int _mnemonicsInit = 0;

public:
	DebuggerImpl() {
	}
	virtual ~DebuggerImpl() {
		stop();

		close();
	}

	virtual bool open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Theme* theme, class Device* device) override {
		if (_opened)
			return true;

		_inspecting = false;
		_activeCodePage = -1;
		_snapshot.reset();
		_registers = Device::Registers();
		_mnemonics.clear();
		_mnemonicsInit = 0;

		_window = wnd;
		_renderer = rnd;
		_workspace = ws;
		_theme = theme;
		_device = device;

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		_device->clearBreakpoints();

		_bringCodeDebuggerToFront = false;
		_bringCategoryToFront = Categories::NONE;
		_bringProgramCursorToFront = false;
		_inspecting = false;
		_activeCodePage = -1;
		_snapshot.reset();
		_registers = Device::Registers();
		_mnemonics.clear();
		_mnemonicsInit = 0;

		_started = false;
		_compiled = nullptr;
		_runtimeConfig = RuntimeConfig();
		_breakpoints.clear();
		_breakAtNextStep = Breakpoint(-1, -1, false);
		_breakAtNextStepInstalledForBasic = false;
		_ignoreForBreakingAtNextStep = FarPtr();
		_breakTimeout = 0;
		_srcToTracePoint.clear();
		_currentBankPointer = FarPtr();
		_vmStepPointer = FarPtr();
		_vmStepBreakpointRefCount = 0;
		_vmStepBreakpointId = -1;
		_latestVmStepInstructionAddress = FarPtr();

		_window = nullptr;
		_renderer = nullptr;
		_workspace = nullptr;
		_theme = nullptr;
		_device = nullptr;

		_opened = false;

		return true;
	}

	virtual int safeHeight(void) const override {
		return _options.safeHeight;
	}

	virtual void update(bool visible, bool showTitle) override {
		debug(visible);

		if (!visible)
			return;

		begin(showTitle);
		if (inspecting()) {
			code();
			ImGui::NewLine(1);
			ImGui::Separator();

			kernelMemory();
			ImGui::NewLine(1);
			ImGui::Separator();

			deviceMemory();
		} else {
			running();
		}
		end();
	}

	virtual void start(void) override {
		// Prepare.
		if (_started)
			return;

		_started = true;

		std::string config;
		_compiled = &_workspace->getCompiledData(&config);
		rapidjson::Document doc;
		Json::fromString(doc, config.c_str());
		Jpath::get(doc, _runtimeConfig.heapSize, "memory", "heap_size");
		Jpath::get(doc, _runtimeConfig.stackSize, "memory", "stack_size");
		Jpath::get(doc, _runtimeConfig.actorMaxCount, "objects", "max_actor_count");
		Jpath::get(doc, _runtimeConfig.projectileDefMaxCount, "objects", "max_projectile_def_count");
		Jpath::get(doc, _runtimeConfig.projectileMaxCount, "objects", "max_projectile_count");
		Jpath::get(doc, _runtimeConfig.triggerMaxCount, "objects", "max_trigger_count");

		// Resolve the ROM entries.
		getFarPointerBySymbolName(COMPILERFREE_CURRENT_BANK_ENTRY_NAME, _currentBankPointer);
		getFarPointerBySymbolName(COMPILER_VM_STEP_ENTRY_NAME, _vmStepPointer);

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
		_bringCategoryToFront = Categories::NONE;
		_bringProgramCursorToFront = false;
		_inspecting = false;
		_activeCodePage = -1;
		_snapshot.reset();
		_registers = Device::Registers();
		_mnemonics.clear();
		_mnemonicsInit = 0;

		_compiled = nullptr;
		_runtimeConfig = RuntimeConfig();
		_breakpoints.clear();
		_breakAtNextStep = Breakpoint(-1, -1, false);
		_breakAtNextStepInstalledForBasic = false;
		_ignoreForBreakingAtNextStep = FarPtr();
		_breakTimeout = 0;
		_srcToTracePoint.clear();
		_currentBankPointer = FarPtr();
		_vmStepPointer = FarPtr();
		_vmStepBreakpointRefCount = 0;
		_vmStepBreakpointId = -1;
		_latestVmStepInstructionAddress = FarPtr();

		_started = false;
	}

	virtual void pause(void) override {
		_bringCodeDebuggerToFront = true;

		_inspecting = true;

		inspect(nullptr);
	}
	virtual void resume(void) override {
		_inspecting = false;

		_snapshot.reset();
		_registers = Device::Registers();
	}
	bool inspecting(void) const {
		return _inspecting;
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

	virtual void step(bool toNextAsmInst) override {
		_inspecting = false;

		_snapshot.reset();
		_registers = Device::Registers();

		_bringCategoryToFront = toNextAsmInst ? Categories::ASM : Categories::BASIC;

		if (toNextAsmInst) {
			_breakAtNextStep.enabled = true;
			_breakAtNextStep.type = Categories::ASM;

			_workspace->step(_window, _renderer);

			_workspace->resume(_window, _renderer);

			return;
		}

		if (!_breakAtNextStep.enabled) {
			_breakAtNextStep.enabled = true;
			_breakAtNextStep.type = Categories::BASIC;

			if (!_breakAtNextStepInstalledForBasic) {
				_breakAtNextStepInstalledForBasic = true;

				_ignoreForBreakingAtNextStep = _latestVmStepInstructionAddress;

				installVmStepBreakpoint();
			}

			_workspace->resume(_window, _renderer);
		}
		_breakTimeout = DateTime::ticks() + DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT;
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
		int hitCount = 0;
		const bool isBasic = _vmStepPointer.equals(bank, pc);
		UInt16 ctxPc = 0;
		UInt8 ctxBank = 0;
		if (isBasic) {
			const UInt16 currCtx = regs.DE; // `DE` is the pointer to the current `VM::SCRIPT_CTX`.
			if (!probeThreadProgramCounter(currCtx, ctxBank, ctxPc))
				return false;

			_latestVmStepInstructionAddress = FarPtr(ctxBank, ctxPc);
		}

		// Handle stepping.
		if (_breakAtNextStep.enabled && hitCount == 0) {
			_breakAtNextStep.enabled = false;

			if (_breakAtNextStep.type == Categories::BASIC) {
				const GBBASIC::TracePoint* tp = getTracePointByRomLocation(ctxBank, ctxPc, GBBASIC::RomLocation::Types::BASIC);
				if (tp) {
					if (_ignoreForBreakingAtNextStep.equals(ctxBank, ctxPc)) {
						_ignoreForBreakingAtNextStep = FarPtr();

						if (hitCount == 0)
							_breakAtNextStep.enabled = true;

						return hitCount > 0;
					}

					if (!_ignoreForBreakingAtNextStep.invalid())
						_ignoreForBreakingAtNextStep = FarPtr();

					_breakTimeout = 0;

					const int page = tp->inCode.page;
					const int row = tp->inCode.row;

					Breakpoint breakpoint(page, row, false);
					breakpoint.type = Categories::BASIC;
					breakpoint.id = _vmStepBreakpointId;
					breakpoint.hitPointer = FarPtr(bank, pc);
					breakpoint.vmPointer = FarPtr(ctxBank, ctxPc);
					hitBreakpoint(breakpoint);

					uninstallVmStepBreakpoint();

					if (_breakAtNextStepInstalledForBasic)
						_breakAtNextStepInstalledForBasic = false;

					hitCount = 1;
				} else {
					for (int i = 0; i < (int)_breakpoints.size(); ++i) {
						const Breakpoint &breakpoint = _breakpoints[i];
						if (!breakpoint.hitPointer.equals(bank, pc))
							continue;

						if (!isBasic) {
							hitBreakpoint(breakpoint);
							++hitCount;
						}
					}

					if (hitCount == 0)
						_breakAtNextStep.enabled = true;

					return hitCount > 0;
				}
			} else if (_breakAtNextStep.type == Categories::ASM) {
				const GBBASIC::TracePoint* tp = getTracePointByRomLocation(bank, pc, GBBASIC::RomLocation::Types::ASM);
				int page = -1;
				int row = -1;
				if (tp) {
					page = tp->inCode.page;
					row = tp->inCode.row;
				}

				Breakpoint breakpoint(page, row, false);
				breakpoint.type = Categories::ASM;
				breakpoint.id = -1;
				breakpoint.hitPointer = FarPtr(bank, pc);
				breakpoint.vmPointer = FarPtr(0, 0);
				hitBreakpoint(breakpoint);

				hitCount = 1;
			}
		}

		// Traverse and check all breakpoints.
		if (hitCount == 0) {
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
		}

		// Finish.
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
	bool getFarPointerBySymbolName(const char* name, FarPtr &out) const {
		out = FarPtr();

		if (!name)
			return false;

		const GBBASIC::RomLocation* romLocation = getRomLocationBySymbolName(name);
		if (!romLocation)
			return false;

		out.bank = romLocation->bank;
		out.address = romLocation->address;

		return true;
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
	const GBBASIC::TracePoint* getTracePointByRomLocation(int bank, int address, GBBASIC::RomLocation::Types type) const {
		const GBBASIC::TracePoint::Array* tracePoints = compiledTracePoints();
		if (!tracePoints)
			return nullptr;

		const GBBASIC::RomLocation key(bank, address, 0, type);
		const GBBASIC::TracePoint::Array::const_iterator it = std::lower_bound(
			tracePoints->begin(), tracePoints->end(),
			key,
			[] (const GBBASIC::TracePoint &tp, const GBBASIC::RomLocation &val) -> bool {
				return tp.compare(val) < 0;
			}
		);
		if (it != tracePoints->end()) {
			const GBBASIC::TracePoint &tp = *it;
			if (tp.compare(key) <= 0)
				return &tp;
		}

		return nullptr;
	}
	GBBASIC::Disassembler::Mnemonic::Array getDisassembledMnemonics(void) const {
		GBBASIC::Disassembler::Mnemonic::Array result;
		GBBASIC::Disassembler::Ptr dasm(GBBASIC::Disassembler::create());

		GBBASIC::Disassembler::DisassemblingOptions options;
		options.bankSize = DEBUGGER_BANK_SIZE;
		options.startAddress = DEBUGGER_START_ADDRESS;
		options.bank = 0;
		options.addressCursor = 0;
		try {
			dasm->disassemble(result, compiledBytes(), options);
		} catch (const std::bad_alloc &e) {
			result.clear();
			result.push_back(GBBASIC::Disassembler::Mnemonic((UInt8)0, (UInt16)0, "cannot disassemble", false, nullptr));
			result.push_back(GBBASIC::Disassembler::Mnemonic((UInt8)0, (UInt16)1, "rom too big", false, nullptr));

			fprintf(stderr, "Cannot allocate memory for disassembling: %s.\n", e.what());
		}

		return result;
	}

	const GBBASIC::Disassembler::Mnemonic::Array &touchMnemonics(void) {
		if (_mnemonics.empty())
			_mnemonics = getDisassembledMnemonics();

		return _mnemonics;
	}

	bool probeThreadProgramCounter(UInt16 threadAddr, UInt8 &bank, UInt16 &pc) const {
		bank = 0;
		pc = 0;

		UInt16 ctxPc = 0;
		UInt8 ctxBank = 0;
		constexpr const int pcOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, PC);
		constexpr const int bankOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, bank);
		const int ctxPcAddress = threadAddr + pcOffset;
		const int ctxBankAddress = threadAddr + bankOffset;
		if (!_device->readRam((UInt16)ctxPcAddress, &ctxPc))
			return false;
		if (!_device->readRam((UInt16)ctxBankAddress, &ctxBank))
			return false;

		bank = ctxBank;
		pc = ctxPc;

		return true;
	}
	bool probeFirstThreadAddress(UInt16 &out) const {
		out = 0;

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_FIRST_CONTEXT_ENTRY_NAME, farPtr))
			return false;
		UInt16 threadAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &threadAddr) || threadAddr == 0)
			return false;

		out = threadAddr;

		return true;
	}
	bool probeThreads(VM::SCRIPT_CTX::Array &out) const {
		out.clear();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_FIRST_CONTEXT_ENTRY_NAME, farPtr))
			return false;
		UInt16 threadAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &threadAddr) || threadAddr == 0)
			return false;

		do {
			VM::SCRIPT_CTX ctx;
			if (!probeThread(threadAddr, ctx))
				return false;

			out.push_back(VM::SCRIPT_CTX::Ref(ctx, FarPtr(0, threadAddr)));

			constexpr const int nextOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, next);
			const int nextAddress = threadAddr + nextOffset;
			if (!_device->readRam((UInt16)nextAddress, &threadAddr))
				return false;
		} while (threadAddr != 0);

		return true;
	}
	bool probeThread(UInt16 threadAddr, VM::SCRIPT_CTX &out) const {
		out = VM::SCRIPT_CTX();

		if (!_device->readRam(threadAddr, (Byte*)&out, sizeof(VM::SCRIPT_CTX)))
			return false;

		return true;
	}
	bool probeThreadStack(UInt16 threadAddr, VM::ThreadStack &out) const {
		out = VM::ThreadStack();

		UInt16 baseAddr = 0;
		constexpr const int baseAddrOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, base_addr);
		const int baseAddrAddress = threadAddr + baseAddrOffset;
		if (!_device->readRam((UInt16)baseAddrAddress, &baseAddr))
			return false;

		UInt16 stackPtr = 0;
		constexpr const int stackPtrOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, stack_ptr);
		const int stackPtrAddress = threadAddr + stackPtrOffset;
		if (!_device->readRam((UInt16)stackPtrAddress, &stackPtr))
			return false;

		for (size_t i = 0; i < _runtimeConfig.stackSize * sizeof(UInt16); ++i) {
			UInt8 data = 0;
			if (!_device->readRam((UInt16)(baseAddr + i), &data))
				data = 0;
			out.buffer.push_back(data);
		}

		out.count = (stackPtr - baseAddr) / sizeof(UInt16);

		return true;
	}
	bool probeHeap(VM::Buffer &out) const {
		out.clear();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_SCRIPT_MEMORY_ENTRY_NAME, farPtr))
			return false;
		UInt16 heapAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &heapAddr) || heapAddr == 0)
			return false;

		for (size_t i = 0; i < _runtimeConfig.heapSize * sizeof(UInt16); ++i) {
			UInt8 data = 0;
			if (!_device->readRam((UInt16)(heapAddr + i), &data))
				data = 0;
			out.push_back(data);
		}

		return true;
	}
	bool probeActors(VM::actor_t::Array &out) const {
		out.clear();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_ACTOR_ACTIVE_HEAD_ENTRY_NAME, farPtr))
			return false;
		UInt16 actorAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &actorAddr) || actorAddr == 0)
			return false;

		do {
			VM::actor_t actor;
			if (!_device->readRam(actorAddr, (Byte*)&actor, sizeof(VM::actor_t)))
				return false;

			out.push_back(VM::actor_t::Ref(actor, FarPtr(0, actorAddr)));

			constexpr const int nextOffset = GBBASIC_OFFSETOF(VM::actor_t, next);
			const int nextAddress = actorAddr + nextOffset;
			if (!_device->readRam((UInt16)nextAddress, &actorAddr))
				return false;
		} while (actorAddr != 0);

		GBBASIC_ASSERT((int)out.size() <= _runtimeConfig.actorMaxCount && "Wrong data.");

		return true;
	}
	bool probeProjectileDefs(VM::projectile_def_t::Array &out) const {
		out.clear();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_PROJECTILE_DEFS_ENTRY_NAME, farPtr))
			return false;
		UInt16 projectileDefAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &projectileDefAddr) || projectileDefAddr == 0)
			return false;

		for (size_t i = 0; i < (size_t)_runtimeConfig.projectileDefMaxCount; ++i) {
			VM::projectile_def_t projectileDef;
			if (!_device->readRam((UInt16)projectileDefAddr, (Byte*)&projectileDef, sizeof(VM::projectile_def_t)))
				projectileDef = VM::projectile_def_t();
			out.push_back(VM::projectile_def_t::Ref(projectileDef, FarPtr(0, projectileDefAddr)));
			projectileDefAddr += sizeof(VM::projectile_def_t);
		}

		return true;
	}
	bool probeProjectiles(VM::projectile_t::Array &out) const {
		out.clear();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_PROJECTILE_ACTIVE_HEAD_ENTRY_NAME, farPtr))
			return false;
		UInt16 projectileAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &projectileAddr) || projectileAddr == 0)
			return false;

		do {
			VM::projectile_t projectile;
			if (!_device->readRam(projectileAddr, (Byte*)&projectile, sizeof(VM::projectile_t)))
				return false;

			out.push_back(VM::projectile_t::Ref(projectile, FarPtr(0, projectileAddr)));

			constexpr const int nextOffset = GBBASIC_OFFSETOF(VM::projectile_t, next);
			const int nextAddress = projectileAddr + nextOffset;
			if (!_device->readRam((UInt16)nextAddress, &projectileAddr))
				return false;
		} while (projectileAddr != 0);

		GBBASIC_ASSERT((int)out.size() <= _runtimeConfig.projectileMaxCount && "Wrong data.");

		return true;
	}
	bool probeTriggers(VM::trigger_t::Array &out) const {
		out.clear();

		FarPtr farPtr_;
		if (!getFarPointerBySymbolName(COMPILER_TRIGGER_COUNT_ENTRY_NAME, farPtr_))
			return false;
		UInt16 triggerCountAddr = 0;
		if (!_device->readRam((UInt16)farPtr_.address, &triggerCountAddr) || triggerCountAddr == 0)
			return false;

		UInt8 triggerCount = 0;
		if (!_device->readRam((UInt16)triggerCountAddr, &triggerCount))
			return false;

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_PROJECTILE_DEFS_ENTRY_NAME, farPtr))
			return false;
		UInt16 triggerAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &triggerAddr) || triggerAddr == 0)
			return false;

		for (size_t i = 0; i < triggerCount; ++i) {
			VM::trigger_t trigger;
			if (!_device->readRam((UInt16)triggerAddr, (Byte*)&trigger, sizeof(VM::trigger_t)))
				trigger = VM::trigger_t();
			out.push_back(VM::trigger_t::Ref(trigger, FarPtr(0, triggerAddr)));
			triggerAddr += sizeof(VM::trigger_t);
		}

		GBBASIC_ASSERT((int)out.size() <= _runtimeConfig.triggerMaxCount && "Wrong data.");

		return true;
	}
	bool probeScene(VM::scene_t &out) const {
		out = VM::scene_t();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_SCENE_ENTRY_NAME, farPtr))
			return false;
		UInt16 sceneAddr = 0;
		if (!_device->readRam((UInt16)farPtr.address, &sceneAddr) || sceneAddr == 0)
			return false;

		VM::scene_t scene;
		if (!_device->readRam((UInt16)sceneAddr, (Byte*)&scene, sizeof(VM::scene_t)))
			scene = VM::scene_t();
		out = scene;

		return true;
	}

	void refreshBreakpoints(void) {
		// Sort the breakpoints.
		std::sort(_breakpoints.begin(), _breakpoints.end());

		// Re-assign breakpoints' type if needed.
		for (Breakpoint &breakpoint : _breakpoints) {
			if (breakpoint.type != Categories::NONE)
				continue;

			const GBBASIC::TracePoint* tp = getTracePointBySourceLocation(breakpoint.page, breakpoint.row);
			if (!tp)
				continue;

			if (tp->inRom.type == GBBASIC::RomLocation::Types::BASIC)
				breakpoint.type = Categories::BASIC;
			else
				breakpoint.type = Categories::ASM;
		}
	}
	bool installBreakpoint(Breakpoint &breakpoint) {
		if (breakpoint.type == Categories::NONE) {
			return false;
		}

		const GBBASIC::TracePoint* tp = getTracePointBySourceLocation(breakpoint.page, breakpoint.row);
		if (!tp)
			return false;

		if (breakpoint.type == Categories::ASM) {
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

		installVmStepBreakpoint();
		breakpoint.id = _vmStepBreakpointId;
		breakpoint.hitPointer = _vmStepPointer;
		breakpoint.vmPointer = FarPtr(tp->inRom.bank, tp->inRom.address);

		return true;
	}
	bool uninstallBreakpoint(Breakpoint &breakpoint) {
		if (breakpoint.type == Categories::NONE) {
			return false;
		}

		if (breakpoint.type == Categories::ASM) {
			_device->removeBreakpoint(breakpoint.id);
			breakpoint.id = -1;
			breakpoint.hitPointer = FarPtr();
			breakpoint.vmPointer = FarPtr();

			return true;
		}

		if (_vmStepPointer.invalid())
			return false;

		uninstallVmStepBreakpoint();
		breakpoint.id = -1;
		breakpoint.hitPointer = FarPtr();
		breakpoint.vmPointer = FarPtr();

		return true;
	}
	int installVmStepBreakpoint(void) {
		if (_vmStepBreakpointRefCount++ == 0) {
			const UInt8 bank = (UInt8)_vmStepPointer.bank;
			const UInt16 address = (UInt16)_vmStepPointer.address;
			const int id = _device->addBreakpoint(bank, address);
			_vmStepBreakpointId = id;
		}

		return _vmStepBreakpointRefCount;
	}
	int uninstallVmStepBreakpoint(void) {
		GBBASIC_ASSERT(_vmStepBreakpointRefCount > 0);

		if (_vmStepBreakpointRefCount == 0)
			return 0;

		if (--_vmStepBreakpointRefCount == 0) {
			_device->removeBreakpoint(_vmStepBreakpointId);
			_vmStepBreakpointId = -1;
		}

		return _vmStepBreakpointRefCount;
	}
	void hitBreakpoint(const Breakpoint &breakpoint) {
		_bringCodeDebuggerToFront = true;

		_inspecting = true;

		inspect(&breakpoint);
	}
	void debug(bool visible) {
		// Handle stepping timeout.
		if (_breakTimeout != 0 && DateTime::ticks() >= _breakTimeout) {
			_breakTimeout = 0;
			UInt16 firstThread = 0;
			if (!probeFirstThreadAddress(firstThread) || firstThread == 0) {
				// No point to step to if all threads are ended.
				_workspace->resume(_window, _renderer);
			}
		}

		// Handle debugger focusing.
		if (_bringCodeDebuggerToFront) {
			_bringCodeDebuggerToFront = false;
			if (!visible)
				_workspace->bringCodeDebuggerToFront(true);
		}
	}
	void inspect(const Breakpoint* breakpoint /* nullable */) {
		// Assign the active code page.
		if (breakpoint)
			_activeCodePage = breakpoint->page;

		// Set a program pointer.
		do {
			if (!breakpoint)
				break;

			Project::Ptr &prj = _workspace->currentProject();
			if (!prj)
				break;

			CodeAssets::Entry* entry = prj->getCode(_activeCodePage);
			if (!entry)
				break;

			EditorCode* editor = (EditorCode*)entry->editor;
			if (!editor)
				editor = _workspace->touchCodeEditor(_window, _renderer, prj.get(), _activeCodePage, true, entry);

			if (!editor)
				break;

			editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)(breakpoint->row - 1)); // 1-based.

			_bringCategoryToFront = Categories::BASIC;

			_bringProgramCursorToFront = true;
		} while (false);

		// TODO: DBG.
	}

	void begin(bool showTitle) {
		_options.startY = ImGui::GetCursorPosY();

		if (showTitle) {
			ImGui::AlignTextToFramePadding();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::TextUnformatted(_theme->windowEmulator_CodeDebugger());
		}
	}
	void end(void) {
		_options.safeHeight = (int)(ImGui::GetCursorPosY() - _options.startY + 48);
	}

	void running(void) {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_CodeDebugger_Running().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);
		if (ImGui::ImageButton(_theme->iconPause()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->pause(_window, _renderer);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_Pause());
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconStepBasic()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconDisabledColor))) {
			// Do nothing.
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_StepBasic());
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(_theme->iconStepAsm()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconDisabledColor))) {
			// Do nothing.
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_StepAsm());
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconBreakDisable()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->disableBreakpoints();
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_DisableBreakpoints());
		}
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconBreakEnable()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->enableBreakpoints();
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_EnableBreakpoints());
		}
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconBreakClear()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->clearBreakpoints();
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_ClearBreakpoints());
		}
	}
	void code(void) {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_CodeDebugger_Inspector().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);
		if (ImGui::ImageButton(_theme->iconPlay()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->resume(_window, _renderer);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_Resume());
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconStepBasic()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			step(false);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_StepBasic());
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(_theme->iconStepAsm()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			step(true);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_StepAsm());
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconBreakDisable()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->disableBreakpoints();
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_DisableBreakpoints());
		}
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconBreakEnable()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->enableBreakpoints();
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_EnableBreakpoints());
		}
		ImGui::SameLine();

		if (ImGui::ImageButton(_theme->iconBreakClear()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
			_workspace->clearBreakpoints();
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(_theme->tooltipEmulator_CodeDebugger_ClearBreakpoints());
		}

		if (ImGui::BeginTabBar("@Code")) {
			ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoTooltip;
			do {
				Project::Ptr &prj = _workspace->currentProject();
				if (!prj)
					break;
				CodeAssets::Entry* entry = prj->getCode(_activeCodePage);
				if (!entry)
					break;
				EditorCode* editor = (EditorCode*)entry->editor;
				if (!editor)
					break;

				if (_bringProgramCursorToFront) {
					_bringProgramCursorToFront = false;

					editor->ensureCursorVisible();
				}

				if (_bringCategoryToFront == Categories::BASIC)
					flags |= ImGuiTabItemFlags_SetSelected;
				if (ImGui::BeginTabItem(_theme->windowEmulator_CodeDebugger_Basic(), nullptr, flags)) {
					const float width = ImGui::GetContentRegionAvail().x;
					const float height = 200.0f;
					const bool ro = editor->readonly();
					editor->readonly(true);
					editor->update(
						_window, _renderer,
						_workspace,
						prj->title().c_str(),
						0, 0, width, height,
						0.0
					);
					editor->readonly(ro);

					ImGui::EndTabItem();
				}
			} while (false);

			flags = ImGuiTabItemFlags_NoTooltip;
			if (_bringCategoryToFront == Categories::ASM)
				flags |= ImGuiTabItemFlags_SetSelected;
			if (ImGui::BeginTabItem(_theme->windowEmulator_CodeDebugger_Asm(), nullptr, flags)) {
				const GBBASIC::Disassembler::Mnemonic::Array* mnemonics_ = nullptr;
				if (_mnemonicsInit == 0) { // A simple FSM for lazy mnemonics initialization.
					++_mnemonicsInit;
				} else if (_mnemonicsInit == 1) {
					++_mnemonicsInit;
				} else if (_mnemonicsInit == 2) {
					++_mnemonicsInit;

					touchMnemonics();
				} else {
					mnemonics_ = &touchMnemonics();
				}

				const ImU32 col = _theme->style()->debuggerHeadColor;
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				ImGui::AlignTextToFramePadding();
				if (mnemonics_)
					ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_DisassembyHeadInfo());
				else
					ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_Disassembling());
				ImGui::PopStyleColor();

				const float width = ImGui::GetContentRegionAvail().x;
				const float height = 200.0f - ImGui::GetTextLineHeightWithSpacing();
				const ImGuiWindowFlags flags_ = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav;
				ImGui::BeginChild("@Dasm", ImVec2(width, height), false, flags_);
				{
					mnemonics(mnemonics_);
				}
				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			if (_bringCategoryToFront != Categories::NONE)
				_bringCategoryToFront = Categories::NONE;

			ImGui::EndTabBar();
		}
	}
	void kernelMemory(void) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_CodeDebugger_KernelMemory().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		// TODO: DBG.
	}
	void deviceMemory(void) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_CodeDebugger_DeviceMemory().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		// TODO: DBG.
	}

	void mnemonics(const GBBASIC::Disassembler::Mnemonic::Array* mnemonics_) {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		if (!mnemonics_)
			return;

		VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

		const float lineHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		if (lineHeight <= Math::EPSILON<float>()) return;
		const float panelHeight = ImGui::GetContentRegionAvail().y;
		const int visibleMnemonicCount = (int)std::ceil(panelHeight / lineHeight);
		const int totalMnemonicCount = (int)mnemonics_->size();
		const float totalHeight = lineHeight * totalMnemonicCount;

		float scrollY = ImGui::GetScrollY();

		int startIndex = (int)(scrollY / lineHeight);
		int endIndex = startIndex + (int)std::ceil(panelHeight / lineHeight) + 1;
		startIndex = Math::clamp(startIndex, 0, totalMnemonicCount);
		endIndex = Math::clamp(endIndex, startIndex, totalMnemonicCount);
		if (startIndex > 0) 
			ImGui::Dummy(ImVec2(0.0f, startIndex * lineHeight));

		for (int i = startIndex; i < endIndex; ++i) {
			const GBBASIC::Disassembler::Mnemonic &mnemonic = (*mnemonics_)[i];

			ImGui::AlignTextToFramePadding();
			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerHeadColor);
			ImGui::Text("%02X:%04X ", mnemonic.bank, mnemonic.address);
			ImGui::SameLine();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerInfoColor);
			if (mnemonic.bytes.count == 1)
				ImGui::Text("%02X       ", mnemonic.bytes.data[0]);
			else if (mnemonic.bytes.count == 2)
				ImGui::Text("%02X %02X    ", mnemonic.bytes.data[0], mnemonic.bytes.data[1]);
			else if (mnemonic.bytes.count == 3)
				ImGui::Text("%02X %02X %02X ", mnemonic.bytes.data[0], mnemonic.bytes.data[1], mnemonic.bytes.data[2]);
			else
				ImGui::Text("         ");
			ImGui::SameLine();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
			ImGui::Text("%s ", mnemonic.opcode.c_str());
			ImGui::PopStyleColor();
			if (!mnemonic.operands.empty()) {
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
				ImGui::Text("%s ", mnemonic.operands.c_str());
				ImGui::PopStyleColor();
			}
		}

		if (endIndex < totalMnemonicCount)
			ImGui::Dummy(ImVec2(0.0f, (totalMnemonicCount - endIndex) * lineHeight));

		if (ImGui::IsWindowHovered()) {
			const float wheel = io.MouseWheel;
			if (wheel != 0.0f)
				scrollY -= wheel * lineHeight * 3.0f;
		}
		const float maxScrollY = Math::max(0.0f, totalHeight - panelHeight);
		scrollY = Math::clamp(scrollY, 0.0f, maxScrollY);
		ImGui::SetScrollY(scrollY);

		if (totalHeight > panelHeight) {
			const float scrollbarWidth = 12.0f;
			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 windowSize = ImGui::GetWindowSize();
        
			const ImVec2 barTrackMin = ImVec2(windowPos.x + windowSize.x - scrollbarWidth, windowPos.y);
			const ImVec2 barTrackMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + panelHeight);

			const float grabHeight = Math::max(20.0f, panelHeight * (panelHeight / totalHeight));
			const float scrollRatio = scrollY / maxScrollY;
			const float grabMinY = barTrackMin.y + scrollRatio * (panelHeight - grabHeight);

			const ImVec2 barGrabMin = ImVec2(barTrackMin.x + 2.0f, grabMinY + 1);
			const ImVec2 barGrabMax = ImVec2(barTrackMax.x - 2.0f, grabMinY + grabHeight);

			ImGui::SetCursorScreenPos(barTrackMin);
			ImGui::InvisibleButton("##DasmScrollbar", ImVec2(scrollbarWidth, panelHeight));

			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();

			if (isActive) {
				const float mouseLocalY = io.MousePos.y - barTrackMin.y;
				float newScrollRatio = (mouseLocalY - grabHeight * 0.5f) / (panelHeight - grabHeight);
				newScrollRatio = Math::clamp(newScrollRatio, 0.0f, 1.0f);
				scrollY = newScrollRatio * maxScrollY;
				ImGui::SetScrollY(scrollY);
			}

			const ImU32 trackColor = ImGui::GetColorU32(ImGuiCol_ScrollbarBg);
			drawList->AddRectFilled(barTrackMin, barTrackMax, trackColor);

			const ImGuiCol grabCol = isActive ? ImGuiCol_ScrollbarGrabActive : (isHovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab);
			const ImU32 grabColor = ImGui::GetColorU32(grabCol);
			drawList->AddRectFilled(barGrabMin, barGrabMax, grabColor);
		}
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
