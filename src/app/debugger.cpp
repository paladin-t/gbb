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

#ifndef DEBUGGER_WORD_SIZE
#	define DEBUGGER_WORD_SIZE sizeof(UInt16)
#endif /* DEBUGGER_WORD_SIZE */

#ifndef DEBUGGER_ACTOR_MAX_ANIMATIONS
#	define DEBUGGER_ACTOR_MAX_ANIMATIONS 8
#endif /* DEBUGGER_ACTOR_MAX_ANIMATIONS */
#ifndef DEBUGGER_PROJECTILE_MAX_ANIMATIONS
#	define DEBUGGER_PROJECTILE_MAX_ANIMATIONS 4
#endif /* DEBUGGER_PROJECTILE_MAX_ANIMATIONS */

#ifndef DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT
#	define DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT DateTime::fromSeconds(0.333333);
#endif /* DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT */

#ifndef DEBUGGER_TABLE_LEVEL_MAX_COUNT
#	define DEBUGGER_TABLE_LEVEL_MAX_COUNT 16
#endif /* DEBUGGER_TABLE_LEVEL_MAX_COUNT */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

/**< Shared between the VM and the compiler. */

namespace VM {

template<typename T> struct Reference {
	typedef T ValueType;

	int order = 0;
	ValueType data;
	Debugger::FarPtr pointer;

	Reference() {
	}
	Reference(int ord, const ValueType &d, const Debugger::FarPtr &ptr) :
		order(ord),
		data(d),
		pointer(ptr)
	{
	}
};

struct HeapAllocation {
	typedef std::vector<HeapAllocation> Array;

	int order = 0;
	std::string identifier;
	UInt16 address = 0;
	int length = 0; // In words.
	GBBASIC::RamLocation::Usages usage = GBBASIC::RamLocation::Usages::NONE;

	HeapAllocation() {
	}
	HeapAllocation(int ord, const std::string &id, UInt16 addr, int len, GBBASIC::RamLocation::Usages usg) :
		order(ord),
		identifier(id),
		address(addr),
		length(len),
		usage(usg)
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

	Buffer buffer;
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
		VM::HeapAllocation::Array heap;
		VM::SCRIPT_CTX::Array threads; // Active only.
		VM::ThreadStack::Array threadStacks; // Active only.
		VM::actor_t::Array actors; // Active only.
		VM::projectile_def_t::Array projectileDefs;
		VM::projectile_t::Array projectiles;
		VM::trigger_t::Array triggers;
		VM::scene_t scene;

		Snapshot() {
		}

		void reset(void) {
			heap.clear();
			threads.clear();
			threadStacks.clear();
			actors.clear();
			projectileDefs.clear();
			projectiles.clear();
			triggers.clear();
			scene = VM::scene_t();
		}
	};

	struct SortingRule {
		int index = 0;
		bool ascending = true;

		SortingRule() {
		}
		SortingRule(int idx, bool asc) : index(idx), ascending(asc) {
		}
	};

private:
	/**< General. */

	bool _opened = false;
	struct {
		float startY = 0;
		int safeHeight = 0;
		int disassemblerView = 0;
		SortingRule heapSortingRule;
		SortingRule threadSortingRule;
		int ramView = 1;
		int bankIndex = -1;
		std::string bankText;
		int vramIndex = -1;
		std::string vramText;
		int wramIndex = -1;
		std::string wramText;
		std::string echoText;
	} _options;
	Window* _window = nullptr; // Foreign.
	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	Device* _device = nullptr; // Foreign.

	/**< Debugging. */

	// Basic.
	bool _started = false;
	const GBBASIC::Program::Compiled* _compiled = nullptr;    // Foreign.
	// Config.
	RuntimeConfig _runtimeConfig;                             // From kernel config.
	FarPtr _currentBankPointer;                               // Stores the address of symbol `_current_bank`.
	FarPtr _vmStepPointer;                                    // Stores the address of symbol `VM_STEP`.
	mutable SourceRefToTracePointDictionary _srcToTracePoint; // Reversed mapping from trace points.
	// Breakpoints.
	Breakpoint::Array _breakpoints;
	long long _breakTimeout = 0;                              // Timeout to prevent infinite waiting when all threads are dead.
	// Stepping.
	Breakpoint _breakAtNextStep = Breakpoint(-1, -1, false);  // Enabled when a user operated to move a step forward.
	FarPtr _ignoreForBreakingAtNextStep;                      // The address to be ignored when stepping.
	FarPtr _latestStepInstructionAddress;                     // The latest CPU instruction address when operating to step.
	FarPtr _latestVmStepInstructionAddress;                   // The latest VM instruction address when operating to step.
	// `VM_STEP` breakpoint.
	bool _breakAtNextStepInstalledForBasic = false;           // Whether the `VM_STEP` breakpoint has been installed.
	int _vmStepBreakpointRefCount = 0;                        // The count of installed `VM_STEP` breakpoints.
	int _vmStepBreakpointId = -1;                             // The ID of installed `VM_STEP` breakpoints.

	/**< Inspecting. */

	// Basic.
	bool _bringCodeDebuggerToFront = false;
	Categories _bringCategoryToFront = Categories::NONE;
	bool _bringSourceCodeCursorToFront = false;
	bool _bringProgramCounterCursorToFront = false;
	int _activeCodePage = -1;
	bool _inspecting = false;
	// States.
	Snapshot _snapshot;
	Semaphore _mnemonicsIsBeingGenerated;
	GBBASIC::Disassembler::Mnemonic::Queue _mnemonics;
	FarPtr _latestDisassembledMnemonicsAddress;

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

#if defined GBBASIC_OS_WIN32 || defined GBBASIC_OS_HTML || defined GBBASIC_OS_RASPBERRYPI
		_options.disassemblerView = 1;
#elif defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
		_options.disassemblerView = 0;
#else
		_options.disassemblerView = 1;
#endif /* Platform macro. */

		_activeCodePage = -1;
		_inspecting = false;
		_snapshot.reset();
		_mnemonics.clear();
		_latestDisassembledMnemonicsAddress = FarPtr();

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

		_workspace->join();

		_bringCodeDebuggerToFront = false;
		_bringCategoryToFront = Categories::NONE;
		_bringSourceCodeCursorToFront = false;
		_bringProgramCounterCursorToFront = false;
		_activeCodePage = -1;
		_inspecting = false;
		_snapshot.reset();
		_mnemonicsIsBeingGenerated.wait();
		_mnemonics.clear();
		_latestDisassembledMnemonicsAddress = FarPtr();

		_started = false;
		_compiled = nullptr;
		_runtimeConfig = RuntimeConfig();
		_currentBankPointer = FarPtr();
		_vmStepPointer = FarPtr();
		_srcToTracePoint.clear();
		_breakpoints.clear();
		_breakTimeout = 0;
		_breakAtNextStep = Breakpoint(-1, -1, false);
		_ignoreForBreakingAtNextStep = FarPtr();
		_latestStepInstructionAddress = FarPtr();
		_latestVmStepInstructionAddress = FarPtr();
		_breakAtNextStepInstalledForBasic = false;
		_vmStepBreakpointRefCount = 0;
		_vmStepBreakpointId = -1;

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

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt));

		begin(showTitle);
		if (inspecting()) {
			paused();
			ImGui::NewLine(1);
			ImGui::Separator();

			kernelMemory();
			ImGui::NewLine(1);
			ImGui::Separator();

			deviceMemory();
		} else {
			running();
#if !defined GBBASIC_OS_HTML
			ImGui::NewLine(1);
			ImGui::Separator();

			kernelMemory();
			ImGui::NewLine(1);
			ImGui::Separator();

			deviceMemory();
#endif /* GBBASIC_OS_HTML */
		}
		end();

		ImGui::PopStyleColor();
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
		std::string txt;
		if (Jpath::get(doc, txt, "memory", "heap_size")) {
			if (!Text::fromString(txt, _runtimeConfig.heapSize))
				_runtimeConfig.heapSize = 0;
		}
		txt.clear();
		if (Jpath::get(doc, txt, "memory", "stack_size")) {
			if (!Text::fromString(txt, _runtimeConfig.stackSize))
				_runtimeConfig.stackSize = 0;
		}
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

		_workspace->join();

		_bringCodeDebuggerToFront = false;
		_bringCategoryToFront = Categories::NONE;
		_bringSourceCodeCursorToFront = false;
		_bringProgramCounterCursorToFront = false;
		_activeCodePage = -1;
		_inspecting = false;
		_snapshot.reset();
		_mnemonicsIsBeingGenerated.wait();
		_mnemonics.clear();
		_latestDisassembledMnemonicsAddress = FarPtr();

		_compiled = nullptr;
		_runtimeConfig = RuntimeConfig();
		_currentBankPointer = FarPtr();
		_vmStepPointer = FarPtr();
		_srcToTracePoint.clear();
		_breakpoints.clear();
		_breakTimeout = 0;
		_breakAtNextStep = Breakpoint(-1, -1, false);
		_ignoreForBreakingAtNextStep = FarPtr();
		_latestStepInstructionAddress = FarPtr();
		_latestVmStepInstructionAddress = FarPtr();
		_breakAtNextStepInstalledForBasic = false;
		_vmStepBreakpointRefCount = 0;
		_vmStepBreakpointId = -1;

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
		if (!toNextAsmInst && !(isCompiledFromSource()))
			return;

		_inspecting = false;

		_snapshot.reset();

		_bringCategoryToFront = toNextAsmInst ? Categories::ASM : Categories::BASIC;

		if (toNextAsmInst) {
			_breakAtNextStep.enabled = true;
			_breakAtNextStep.type = Categories::ASM;

			_ignoreForBreakingAtNextStep = _latestStepInstructionAddress;

			_workspace->step(_window, _renderer);

			_workspace->resume(_window, _renderer);

			_workspace->skipFrame(2);

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

			_workspace->skipFrame(5);
		}
		_breakTimeout = DateTime::ticks() + DEBUGGER_BREAK_AT_NEXT_STEP_TIMEOUT;
	}

	virtual bool breakpointHit(void) override {
		// Resolve the CPU registers.
		const Device::Registers regs = _device->readRegisters();

		// Resolve the CPU bank and PC.
		FarPtr pc;
		const bool gotPc = probeCurrentProgramCounter(pc);
		if (!gotPc)
			return false;

		// Resolve the VM bank and PC if necessary.
		int hitCount = 0;
		const bool isBasic = _vmStepPointer.equals(pc.bank, pc.address);
		UInt16 ctxPc = 0;
		UInt8 ctxBank = 0;
		if (isBasic) {
			const UInt16 currCtx = regs.DE; // `DE` is the pointer to the current `VM::SCRIPT_CTX`.
			if (!probeThreadProgramCounter(currCtx, ctxBank, ctxPc))
				return false;

			_latestVmStepInstructionAddress = FarPtr(ctxBank, ctxPc);
		}

		_latestStepInstructionAddress = FarPtr(pc.bank, pc.address);

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
					breakpoint.hitPointer = FarPtr(pc.bank, pc.address);
					breakpoint.vmPointer = FarPtr(ctxBank, ctxPc);
					hitBreakpoint(breakpoint);

					uninstallVmStepBreakpoint();

					if (_breakAtNextStepInstalledForBasic)
						_breakAtNextStepInstalledForBasic = false;

					hitCount = 1;
				} else {
					for (int i = 0; i < (int)_breakpoints.size(); ++i) {
						const Breakpoint &breakpoint = _breakpoints[i];
						if (!breakpoint.hitPointer.equals(pc.bank, pc.address))
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
				if (_ignoreForBreakingAtNextStep.equals(pc.bank, pc.address)) {
					_ignoreForBreakingAtNextStep = FarPtr();

					if (hitCount == 0)
						_breakAtNextStep.enabled = true;

					return hitCount > 0;
				}

				if (!_ignoreForBreakingAtNextStep.invalid())
					_ignoreForBreakingAtNextStep = FarPtr();

				const GBBASIC::TracePoint* tp = getTracePointByRomLocation(pc.bank, pc.address, GBBASIC::RomLocation::Types::ASM);
				int page = -1;
				int row = -1;
				if (tp) {
					page = tp->inCode.page;
					row = tp->inCode.row;
				}

				Breakpoint breakpoint(page, row, false);
				breakpoint.type = Categories::ASM;
				breakpoint.id = -1;
				breakpoint.hitPointer = FarPtr(pc.bank, pc.address);
				breakpoint.vmPointer = FarPtr(0, 0);
				hitBreakpoint(breakpoint);

				hitCount = 1;
			}
		}

		// Traverse and check all breakpoints.
		if (hitCount == 0) {
			for (int i = 0; i < (int)_breakpoints.size(); ++i) {
				const Breakpoint &breakpoint = _breakpoints[i];
				if (!breakpoint.hitPointer.equals(pc.bank, pc.address))
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
	bool isCompiledFromSource(void) const {
		return compiledTracePoints() && !compiledTracePoints()->empty();
	}

	const char* getAddressDescription(UInt16 addr, const char* &detail, bool &readonly, bool &prohibited) {
		readonly = false;

		if (addr <= 0x3fff) {
			readonly = true;

			detail = "16KB ROM bank 0";

			return "ROM0 ";
		}
		if (addr <= 0x7fff) {
			readonly = true;

			detail = "16KB ROM bank n";

			FarPtr pc;
			if (!probeCurrentProgramCounter(pc))
				pc.bank = 1;
			if (_options.bankIndex != pc.bank) {
				_options.bankIndex = pc.bank;
				const std::string n = Text::toHex(_options.bankIndex, 2, '0', true);
				if (_options.bankIndex <= 0x0f)
					_options.bankText = "ROM" + n;
				else
					_options.bankText = "RO" + n + "H";
			}

			if (_options.bankIndex == 0)
				return "ROM+ ";

			return _options.bankText.c_str();
		}
		if (addr <= 0x9fff) {
			detail = "8KB VRAM";

			UInt8 vramIdx = 0;
			if (!_device->readRam(0xff4f, &vramIdx))
				vramIdx = 0;
			vramIdx &= 0b00000001;
			if (_options.vramIndex != vramIdx) {
				_options.vramIndex = vramIdx;
				_options.vramText = "VRAM" + Text::toString(_options.vramIndex);
			}

			return _options.vramText.c_str();
		}
		if (addr <= 0xbfff) {
			detail = "8KB SRAM";

			return "SRAM ";
		}
		if (addr <= 0xcfff) {
			detail = "4KB WRAM 0";

			return "WRAM0";
		}
		if (addr <= 0xdfff) {
			detail = "4KB WRAM n";

			UInt8 wramIdx = 0;
			if (!_device->readRam(0xff70, &wramIdx))
				wramIdx = 1;
			wramIdx &= 0b00000111;
			if (wramIdx == 0)
				wramIdx = 1;
			if (_options.wramIndex != wramIdx) {
				_options.wramIndex = wramIdx;
				_options.wramText = "WRAM" + Text::toString(_options.wramIndex);
			}

			return _options.wramText.c_str();
		}
		if (addr <= 0xfdff) {
			readonly = true;
			prohibited = true;

			const int echoAddr = addr - 0xe000 + 0xc000;
			_options.echoText = "Echo RAM (" + Text::toHex(echoAddr, 4, '0', true) + ")";

			detail = _options.echoText.c_str();

			return "ECHO ";
		}
		if (addr <= 0xfe9f) {
			detail = "OAM";

			return "OAM  ";
		}
		if (addr >= 0xfea0 && addr <= 0xfeff) {
			if      (addr == 0xfea0) detail = "Ext. EXTF";
			else if (addr == 0xfea1) detail = "Ext. PLTF";
			else if (addr == 0xfea2) detail = "Ext. LOCF";
			else if (addr == 0xfea3) detail = "Ext. Reserved";
			else if (addr == 0xfea4) detail = "Ext. TCHX";
			else if (addr == 0xfea5) detail = "Ext. TCHY";
			else if (addr == 0xfea6) detail = "Ext. TCHF";
			else if (addr == 0xfea7) detail = "Ext. Reserved";
			else if (addr == 0xfea8) detail = "Ext. KEYM";
			else if (addr == 0xfea9) detail = "Ext. KEYC";
			else if (addr >= 0xfeaa && addr <= 0xfeab) detail = "Ext. Reserved";
			else if (addr == 0xfeac) detail = "Ext. STMF";
			else if (addr == 0xfead) detail = "Ext. STMB";
			else if (addr == 0xfeae) detail = "Ext. Reserved";
			else if (addr == 0xfeaf) detail = "Ext. TRSF";
			else if (addr >= 0xfeb0 && addr <= 0xfeef) detail = "Ext. TRSC";
			else if (addr <= 0xfeff) detail = "Ext. Reserved"; // 0xFEF0 - 0xFEFF

			return "EXT. ";
		}
		if (addr <= 0xff7f) {
			if      (addr == 0xff00) detail = "Joypad input";
			else if (addr <= 0xff02) detail = "Serial transfer"; // FF01 - FF02
			else if (addr <= 0xff07) detail = "Timer and div."; // FF04 - FF07
			else if (addr == 0xff0f) detail = "Interrupts";
			else if (addr <= 0xff26) detail = "Audio"; // FF10 - FF26
			else if (addr <= 0xff3f) detail = "Wave pattern"; // FF30 - FF3F

			else if (addr == 0xff40) detail = "LCDC";
			else if (addr == 0xff41) detail = "STAT";
			else if (addr == 0xff42) detail = "SCY";
			else if (addr == 0xff43) detail = "SCX";
			else if (addr == 0xff44) detail = "LY";
			else if (addr == 0xff45) detail = "LYC";
			else if (addr == 0xff46) detail = "DMA";
			else if (addr == 0xff47) detail = "BGP";
			else if (addr == 0xff48) detail = "OBP0";
			else if (addr == 0xff49) detail = "OBP1";
			else if (addr == 0xff4a) detail = "WY";
			else if (addr == 0xff4b) detail = "WX";
			else if (addr == 0xff4c) detail = "KEY0/SYS";
			else if (addr == 0xff4d) detail = "KEY1/SPD";
			else if (addr == 0xff4f) detail = "VBK";
			else if (addr == 0xff50) detail = "Boot ROM ctrl.";
			else if (addr == 0xff51) detail = "HDMA1";
			else if (addr == 0xff52) detail = "HDMA2";
			else if (addr == 0xff53) detail = "HDMA3";
			else if (addr == 0xff54) detail = "HDMA4";
			else if (addr == 0xff55) detail = "HDMA5";
			else if (addr == 0xff56) detail = "IR port";
			else if (addr == 0xff68) detail = "BCPS/BGPI";
			else if (addr == 0xff69) detail = "BCPD/BGPD";
			else if (addr == 0xff6a) detail = "OCPS/OBPI";
			else if (addr == 0xff6b) detail = "OCPD/OBPD";
			else if (addr == 0xff6c) detail = "OPRI";
			else if (addr == 0xff70) detail = "SVBK/WBK";

			else                     detail = "I/O registers";

			return "I/O  ";
		}
		if (addr <= 0xfffe) {
			detail = "HRAM";

			return "HRAM ";
		}
		if (addr == 0xffff) {
			detail = "IE";

			return "IE   ";
		}

		readonly = true;

		detail = "Unknown";

		return "-----";
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

	const GBBASIC::Disassembler::Mnemonic::Queue* touchMnemonics(void) {
		// Prepare.
		FarPtr pc;
		bool gotPc = false;
		bool toRefreshPc = false;

		// Validate for refreshing.
		const bool compactMode = _options.disassemblerView == 1;
		if (compactMode && !_mnemonics.empty()) {
			gotPc = probeCurrentProgramCounter(pc);
			if (gotPc && _latestDisassembledMnemonicsAddress.bank != pc.bank) { // Bank changed.
#if defined GBBASIC_OS_HTML
				_mnemonicsIsBeingGenerated.wait();
#endif /* GBBASIC_OS_HTML */
				_mnemonicsIsBeingGenerated = Semaphore();
				_mnemonics.clear(); // Invalidate.
			}

			toRefreshPc = true;
		}

		if (!inspecting() && !_mnemonics.empty()) {
			if (!gotPc)
				gotPc = probeCurrentProgramCounter(pc);

			toRefreshPc = true;
		}

		// Generate mnemonics if needed.
		if (_mnemonics.empty() && !_mnemonicsIsBeingGenerated.working()) {
			struct Data {
				bool compactMode = false;
				bool gotBank = false;
				int bank = 0;
				Bytes::Ptr rom = nullptr;
				GBBASIC::Disassembler::Mnemonic::Queue mnemonics;

				Data(bool compact, bool gotBank_, int bank_, const Bytes::Ptr &rom_) :
					compactMode(compact),
					gotBank(gotBank_),
					bank(bank_)
				{
					rom = Bytes::Ptr(Bytes::create());
					rom->writeBytes(rom_.get());
					rom->poke(0);
				}
			};

			if (!gotPc)
				gotPc = probeCurrentProgramCounter(pc);

			Data* data = new Data(compactMode, gotPc, pc.bank, compiledBytes());
			_mnemonicsIsBeingGenerated = _workspace->async(
				std::bind(
					[] (WorkTask* /* task */, Data* data) -> uintptr_t { // On work thread.
						const bool compactMode = data->compactMode;
						const bool gotBank = data->gotBank;
						int bank = data->bank;
						const Bytes::Ptr &rom = data->rom;

						GBBASIC::Disassembler::Ptr dasm(GBBASIC::Disassembler::create());
						try {
							if (compactMode) {
								GBBASIC::Disassembler::DisassemblingOptions options(
									DEBUGGER_BANK_SIZE,
									DEBUGGER_START_ADDRESS,
									0,
									0,
									0,
									1
								);
								dasm->disassemble(data->mnemonics, rom, options);
								if (gotBank) {
									if (bank == 0)
										bank = 1;
									options.bank = bank;
									options.offset = DEBUGGER_BANK_SIZE * bank;
									dasm->disassemble(data->mnemonics, rom, options);
								}
							} else {
								const GBBASIC::Disassembler::DisassemblingOptions options(
									DEBUGGER_BANK_SIZE,
									DEBUGGER_START_ADDRESS,
									0,
									0
								);
								dasm->disassemble(data->mnemonics, rom, options);
							}
						} catch (const std::bad_alloc &e) {
							data->mnemonics.clear();
							data->mnemonics.push_back(GBBASIC::Disassembler::Mnemonic((UInt8)0, (UInt16)0, "cannot disassemble", false, nullptr));
							data->mnemonics.push_back(GBBASIC::Disassembler::Mnemonic((UInt8)0, (UInt16)1, "rom too big", false, nullptr));

							fprintf(stderr, "Cannot allocate memory for disassembling: %s.\n", e.what());
						}

						return (uintptr_t)data;
					},
					std::placeholders::_1, data
				),
				[this] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
					Data* data = (Data*)ptr;
					std::swap(_mnemonics, data->mnemonics);
				},
				[] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
					Data* data = (Data*)ptr;
					delete data;
				}
			);

			toRefreshPc = true;
		}

		// Refresh the program counter.
		if (toRefreshPc) {
			if (!_latestDisassembledMnemonicsAddress.equals(pc.bank, pc.address)) {
				_latestDisassembledMnemonicsAddress = pc;
				_bringProgramCounterCursorToFront = true;
			}
		}

		// Finish.
		return _mnemonics.empty() ? nullptr : &_mnemonics;
	}
	void unloadMnemonics(void) {
#if defined GBBASIC_OS_HTML
		_mnemonicsIsBeingGenerated.wait();
#endif /* GBBASIC_OS_HTML */
		_mnemonicsIsBeingGenerated = Semaphore();
		_mnemonics.clear();
		_latestDisassembledMnemonicsAddress = FarPtr();
	}

	Snapshot* touchSnapshot(void) {
		if (!isCompiledFromSource())
			return nullptr;

		probeHeap(_snapshot.heap);
		probeThreads(_snapshot.threads);
		_snapshot.threadStacks.clear();
		int ord = 0;
		for (const VM::SCRIPT_CTX::Ref &ctx : _snapshot.threads) {
			VM::ThreadStack stk;
			if (probeThreadStack((UInt16)ctx.pointer.address, stk))
				_snapshot.threadStacks.push_back(VM::ThreadStack::Ref(ord++, stk, FarPtr(0, ctx.data.base_addr)));
		}
		probeActors(_snapshot.actors);
		probeProjectileDefs(_snapshot.projectileDefs);
		probeProjectiles(_snapshot.projectiles);
		probeTriggers(_snapshot.triggers);
		probeScene(_snapshot.scene);

		return &_snapshot;
	}
	void unloadSnapshot(void) {
		_snapshot.reset();
	}

	bool probeCurrentProgramCounter(FarPtr &pc_) const {
		bool result = false;
		const Device::Registers regs = _device->readRegisters();
		const UInt16 pc = regs.PC;
		UInt8 bank = 0;
		if (!_currentBankPointer.invalid()) {
			if (_device->readRam((UInt16)_currentBankPointer.address, &bank))
				result = true;
		}
		if (!result) {
			bank = (UInt8)_device->currentBank();
			result = true;
		}

		pc_.bank = bank;
		pc_.address = pc;

		if (!result)
			return false;

		return true;
	}
	bool probeHeap(VM::HeapAllocation::Array &out) const {
		out.clear();

		const GBBASIC::RamLocation::Dictionary* allocations = compiledAllocations();
		if (!allocations)
			return false;

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_SCRIPT_MEMORY_ENTRY_NAME, farPtr))
			return false;
		const UInt16 heapAddr = (UInt16)farPtr.address;
		if (heapAddr == 0)
			return false;

		int ord = 0;
		for (const GBBASIC::RamLocation::Dictionary::value_type kv : *allocations) {
			const std::string &id = kv.first;
			const GBBASIC::RamLocation &ram = kv.second;
			if (ram.usage == GBBASIC::RamLocation::Usages::NONE)
				continue;

			const int len = ram.size / DEBUGGER_WORD_SIZE;
			const VM::HeapAllocation healAlloc(ord++, id, (UInt16)(heapAddr + ram.address * DEBUGGER_WORD_SIZE), len, ram.usage);
			out.push_back(healAlloc);
		}

		const int column = _options.heapSortingRule.index;
		const bool ascending = _options.heapSortingRule.ascending;
		if (column != 0 || !ascending) {
			std::sort(
				out.begin(), out.end(),
				[column, ascending] (const VM::HeapAllocation &l, const VM::HeapAllocation &r) -> bool {
					switch (column) {
					case 1:
						return ascending ?
							l.identifier < r.identifier :
							l.identifier > r.identifier;
					case 2:
						return ascending ?
							l.address < r.address :
							l.address > r.address;
					case 3:
						return ascending ?
							l.usage < r.usage :
							l.usage > r.usage;
					default:
						return ascending ?
							l.order < r.order :
							l.order > r.order;
					}
				}
			);
		}

		out.shrink_to_fit();

		return true;
	}
	bool probeHeap(VM::Buffer &out) const {
		out.clear();

		FarPtr farPtr;
		if (!getFarPointerBySymbolName(COMPILER_SCRIPT_MEMORY_ENTRY_NAME, farPtr))
			return false;
		const UInt16 heapAddr = (UInt16)farPtr.address;
		if (heapAddr == 0)
			return false;

		for (size_t i = 0; i < _runtimeConfig.heapSize * DEBUGGER_WORD_SIZE; ++i) {
			UInt8 data = 0;
			if (!_device->readRam((UInt16)(heapAddr + i), &data))
				data = 0;
			out.push_back(data);
		}

		return true;
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

		int ord = 0;
		do {
			VM::SCRIPT_CTX ctx;
			if (!probeThread(threadAddr, ctx))
				return false;

			out.push_back(VM::SCRIPT_CTX::Ref(ord++, ctx, FarPtr(0, threadAddr)));

			constexpr const int nextOffset = GBBASIC_OFFSETOF(VM::SCRIPT_CTX, next);
			const int nextAddress = threadAddr + nextOffset;
			if (!_device->readRam((UInt16)nextAddress, &threadAddr))
				return false;
		} while (threadAddr != 0);

		out.shrink_to_fit();

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

		for (size_t i = 0; i < _runtimeConfig.stackSize * DEBUGGER_WORD_SIZE; ++i) {
			UInt8 data = 0;
			if (!_device->readRam((UInt16)(baseAddr + i), &data))
				data = 0;
			out.buffer.push_back(data);
		}

		out.count = (stackPtr - baseAddr) / DEBUGGER_WORD_SIZE;

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

		int ord = 0;
		do {
			VM::actor_t actor;
			if (!_device->readRam(actorAddr, (Byte*)&actor, sizeof(VM::actor_t)))
				return false;

			out.push_back(VM::actor_t::Ref(ord++, actor, FarPtr(0, actorAddr)));

			constexpr const int nextOffset = GBBASIC_OFFSETOF(VM::actor_t, next);
			const int nextAddress = actorAddr + nextOffset;
			if (!_device->readRam((UInt16)nextAddress, &actorAddr))
				return false;

			if ((int)out.size() > _runtimeConfig.actorMaxCount)
				break;
		} while (actorAddr != 0);

		out.shrink_to_fit();

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
			out.push_back(VM::projectile_def_t::Ref((int)i, projectileDef, FarPtr(0, projectileDefAddr)));
			projectileDefAddr += sizeof(VM::projectile_def_t);
		}

		out.shrink_to_fit();

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

		int ord = 0;
		do {
			VM::projectile_t projectile;
			if (!_device->readRam(projectileAddr, (Byte*)&projectile, sizeof(VM::projectile_t)))
				return false;

			out.push_back(VM::projectile_t::Ref(ord++, projectile, FarPtr(0, projectileAddr)));

			constexpr const int nextOffset = GBBASIC_OFFSETOF(VM::projectile_t, next);
			const int nextAddress = projectileAddr + nextOffset;
			if (!_device->readRam((UInt16)nextAddress, &projectileAddr))
				return false;

			if ((int)out.size() > _runtimeConfig.projectileMaxCount)
				break;
		} while (projectileAddr != 0);

		out.shrink_to_fit();

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
			out.push_back(VM::trigger_t::Ref((int)i, trigger, FarPtr(0, triggerAddr)));
			triggerAddr += sizeof(VM::trigger_t);

			if ((int)out.size() > _runtimeConfig.triggerMaxCount)
				break;
		}

		out.shrink_to_fit();

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
		_breakpoints.shrink_to_fit();

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

			_bringSourceCodeCursorToFront = true;
		} while (false);

		// Refresh the snapshot.
		touchSnapshot();
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

#if !defined GBBASIC_OS_HTML
		code(regSize);
#endif /* GBBASIC_OS_HTML */
	}
	void paused(void) {
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

		if (isCompiledFromSource()) {
			if (ImGui::ImageButton(_theme->iconStepBasic()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconColor))) {
				step(false);
			}
		} else {
			if (ImGui::ImageButton(_theme->iconStepBasic()->pointer(_renderer), buttonSize, ImGui::ColorConvertU32ToFloat4(_theme->style()->iconDisabledColor))) {
				// Do nothing.
			}
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

		code(regSize);
	}
	void kernelMemory(void) {
		Snapshot* snapshot = touchSnapshot();
		if (!snapshot)
			return;

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

		variables(snapshot);
		threads(snapshot);
		objects(snapshot);
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

		{
			const ImU32 col = _theme->style()->debuggerHeadColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_Registers());
			ImGui::PopStyleColor();

			{
				VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

				registers();
			}
		}

		{
			const ImU32 col = _theme->style()->debuggerHeadColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_RamInfo());
			ImGui::PopStyleColor();

			{
				VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

				const float width = ImGui::GetContentRegionAvail().x;
				const float height = 200.0f - ImGui::GetTextLineHeightWithSpacing();
				const ImGuiWindowFlags flags_ = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav;
				ImGui::BeginChild("@Ram", ImVec2(width, height), false, flags_);
				{
					ram();
				}
				ImGui::EndChild();
			}

			{
				ImGui::NewLine(1);
				ImGui::Dummy(ImVec2(2, 0));
				ImGui::SameLine();
				const float posX = ImGui::GetCursorPosX();
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(_theme->windowEmulator_CodeDebugger_View());
				ImGui::SameLine();
				const float diff = ImGui::GetCursorPosX() - posX;
				const float remain = regSize.x * 0.3f - diff;
				ImGui::Dummy(ImVec2(remain, 0));
				ImGui::SameLine();

				const char* ITEMS[] = {
					_theme->windowEmulator_CodeDebugger_View_Incremental().c_str(),
					_theme->windowEmulator_CodeDebugger_View_Decremental().c_str()
				};

				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(regSize.x * 0.7f);
				if (ImGui::Combo("##RamView", &_options.ramView, ITEMS, GBBASIC_COUNTOF(ITEMS))) {
					// Do nothing.
				}
			}
		}
	}

	void code(const ImVec2 &regSize) {
		ImGuiStyle &style = ImGui::GetStyle();

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

				if (_bringSourceCodeCursorToFront) {
					_bringSourceCodeCursorToFront = false;

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

					ImGui::NewLine(2);
					ImGui::SameLine();
					ImGui::Dummy(ImVec2(2, 0));
#if GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED
					ImGui::Text("%s %02X", _theme->status_Pg().c_str(), _activeCodePage);
#else /* GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED */
					ImGui::Text("%s %d", _theme->status_Pg().c_str(), _activeCodePage);
#endif /* GBBASIC_ASSET_PAGE_SHOW_HEX_ENABLED */
					ImGui::NewLine(3);

					ImGui::EndTabItem();
				}
			} while (false);

			flags = ImGuiTabItemFlags_NoTooltip;
			if (_bringCategoryToFront == Categories::ASM)
				flags |= ImGuiTabItemFlags_SetSelected;
			if (ImGui::BeginTabItem(_theme->windowEmulator_CodeDebugger_Asm(), nullptr, flags)) {
				const GBBASIC::Disassembler::Mnemonic::Queue* mnemonics_ = touchMnemonics();

				const ImU32 col = _theme->style()->debuggerHeadColor;
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				ImGui::Dummy(ImVec2(1, 0));
				ImGui::SameLine();
				ImGui::AlignTextToFramePadding();
				if (mnemonics_)
					ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_DisassembyHeadInfo());
				else
					ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_Disassembling());
				ImGui::PopStyleColor();

				{
					VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

					const float width = ImGui::GetContentRegionAvail().x;
					const float height = 200.0f - ImGui::GetTextLineHeightWithSpacing();
					const ImGuiWindowFlags flags_ = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav;
					ImGui::BeginChild("@Dasm", ImVec2(width, height), false, flags_);
					{
						mnemonics(mnemonics_);
					}
					ImGui::EndChild();
				}

				{
					ImGui::NewLine(1);
					ImGui::Dummy(ImVec2(2, 0));
					ImGui::SameLine();
					const float posX = ImGui::GetCursorPosX();
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(_theme->windowEmulator_CodeDebugger_View());
					ImGui::SameLine();
					const float diff = ImGui::GetCursorPosX() - posX;
					const float remain = regSize.x * 0.3f - diff;
					ImGui::Dummy(ImVec2(remain, 0));
					ImGui::SameLine();

					const char* ITEMS[] = {
						_theme->windowEmulator_CodeDebugger_View_FullRom().c_str(),
						_theme->windowEmulator_CodeDebugger_View_CompactMode().c_str()
					};

					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
					VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

					ImGui::SetNextItemWidth(regSize.x * 0.7f);
					if (ImGui::Combo("##DasmView", &_options.disassemblerView, ITEMS, GBBASIC_COUNTOF(ITEMS))) {
						unloadMnemonics();
					}
				}
				ImGui::SameLine();
				ImGui::NewLine(1);

				ImGui::EndTabItem();
			}

			if (_bringCategoryToFront != Categories::NONE)
				_bringCategoryToFront = Categories::NONE;

			ImGui::EndTabBar();
		}
	}
	void mnemonics(const GBBASIC::Disassembler::Mnemonic::Queue* mnemonics_) {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		if (!mnemonics_)
			return;

		const float lineHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		if (lineHeight <= Math::EPSILON<float>()) return;
		const float panelHeight = ImGui::GetContentRegionAvail().y;
		const int visibleMnemonicCount = (int)std::ceil(panelHeight / lineHeight);
		const int totalMnemonicCount = (int)mnemonics_->size();
		const float totalHeight = lineHeight * totalMnemonicCount;

		const float scrollbarWidth = 12.0f;
		float scrollY = ImGui::GetScrollY();
		if (_bringProgramCounterCursorToFront) {
			_bringProgramCounterCursorToFront = false;

			ImGui::SetScrollHereY();

			FarPtr key = _latestDisassembledMnemonicsAddress;
			if (key.address < DEBUGGER_BANK_SIZE)
				key.bank = 0;
			GBBASIC::Disassembler::Mnemonic::Queue::const_iterator it = std::lower_bound(
				mnemonics_->begin(), mnemonics_->end(),
				key,
				[] (const GBBASIC::Disassembler::Mnemonic &mn, const FarPtr &val) -> bool {
					const FarPtr fp(mn.bank, mn.address);

					auto compare = [&] (void) -> int {
						if (fp.bank < val.bank)
							return -1;
						else if (fp.bank > val.bank)
							return 1;

						if (fp.address < val.address)
							return -1;
						else if (fp.address > val.address)
							return 1;

						return 0;
					};

					return compare() < 0;
				}
			);
			if (it != mnemonics_->end()) {
				const int offset = Math::min((int)(it - mnemonics_->begin()) - 6, (int)mnemonics_->size());
				const double scroll = totalHeight * ((double)offset / totalMnemonicCount);
				scrollY = (float)scroll;
			}
		}

		int startIndex = (int)(scrollY / lineHeight);
		int endIndex = startIndex + (int)std::ceil(panelHeight / lineHeight) + 1;
		startIndex = Math::clamp(startIndex, 0, totalMnemonicCount);
		endIndex = Math::clamp(endIndex, startIndex, totalMnemonicCount);
		if (startIndex > 0) 
			ImGui::Dummy(ImVec2(0.0f, startIndex * lineHeight));

		for (int i = startIndex; i < endIndex; ++i) {
			const GBBASIC::Disassembler::Mnemonic &mnemonic = (*mnemonics_)[i];

			bool indicate = false;
			if (_latestDisassembledMnemonicsAddress.address < DEBUGGER_BANK_SIZE) {
				indicate = 
					0 == mnemonic.bank &&
					_latestDisassembledMnemonicsAddress.address == mnemonic.address;
			} else {
				indicate = 
					_latestDisassembledMnemonicsAddress.bank == mnemonic.bank &&
					_latestDisassembledMnemonicsAddress.address == mnemonic.address;
			}
			if (indicate) {
				const ImVec2 cursorStart = ImGui::GetCursorScreenPos() + ImVec2(1, 0);
				const ImVec2 cursorEnd = cursorStart + ImVec2(ImGui::GetContentRegionAvail().x - scrollbarWidth - 1, lineHeight + 1);
				drawList->AddRectFilled(cursorStart, cursorEnd, 0x40000000);
				drawList->AddRect(cursorStart, cursorEnd, ImGui::GetColorU32(ImGuiCol_NavHighlight));
			}

			ImGui::AlignTextToFramePadding();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
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
	void variables(Snapshot* snapshot) {
		ImGuiStyle &style = ImGui::GetStyle();

		const ImU32 col = _theme->style()->debuggerHeadColor;
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_Heap());
		ImGui::PopStyleColor();

		VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2(style.FramePadding.x, 0));

		const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable |
			ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("##Heap", 5, flags)) {
			constexpr const char* USAGES[] = {
				"-",
				"LET",
				"DIM",
				"FOR",
				"READ",
				"TOUCH",
				"VIEWPORT"
			};

			const float width0 = ImGui::GetFontSize() * 2.0f;
			const float width = ImGui::GetFontSize() * 3.0f;
			const ImU32 col = _theme->style()->debuggerHeadColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Order(), ImGuiTableColumnFlags_WidthFixed, width0);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_ID(), ImGuiTableColumnFlags_WidthFixed, width);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Addr(), ImGuiTableColumnFlags_WidthFixed, width);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Type(), ImGuiTableColumnFlags_WidthFixed, width);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Value(), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
			if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
				const ImGuiTableColumnSortSpecs* spec = &sortSpecs->Specs[0];
				const int column = spec->ColumnIndex;
				const bool ascending = (spec->SortDirection == ImGuiSortDirection_Ascending);
				_options.heapSortingRule = SortingRule(column, ascending);

				sortSpecs->SpecsDirty = false;
			}

			int j = 0;
			for (const VM::HeapAllocation &alloc : snapshot->heap) {
				const int ord = alloc.order;
				const std::string &id = alloc.identifier;
				const UInt16 address = alloc.address;
				const int usage = Math::clamp((int)alloc.usage, 0, (int)GBBASIC_COUNTOF(USAGES));

				ImGui::TableNextRow();
				ImGui::PushID(j);
				{
					ImGui::PushStyleColor(ImGuiCol_Text, col);
					ImGui::TableSetColumnIndex(0); ImGui::Text("%d", ord);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
					ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(id);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerInfoColor);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%04X", address);
					ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(USAGES[usage]);
					ImGui::PopStyleColor();

					if (alloc.usage == GBBASIC::RamLocation::Usages::ARRAY) {
						ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
						ImGui::TableSetColumnIndex(4);
						if (ImGui::TreeNode("[...]")) {
							for (int i = 0; i < alloc.length; ++i) {
								ImGui::PushID(i);
								{
									ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
									ImGui::Text("[%d]=", i);
									ImGui::SameLine();
									ImGui::PopStyleColor();

									UInt16 val = 0;
									if (!_device->readRam((UInt16)(address + i * DEBUGGER_WORD_SIZE), &val))
										val = 0;
									ImGui::Text("%d(%04X)", val, val);
								}
								ImGui::PopID();
							}

							ImGui::TreePop();
						}
						ImGui::PopStyleColor();
					} else {
						ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
						UInt16 val = 0;
						if (!_device->readRam(address, &val))
							val = 0;
						ImGui::TableSetColumnIndex(4); ImGui::Text("%d(%04X)", val, val);
						ImGui::PopStyleColor();
					}
				}
				ImGui::PopID();
				++j;
			}

			ImGui::EndTable();
		}
	}
	void threads(Snapshot* snapshot) {
		ImGuiStyle &style = ImGui::GetStyle();

		const ImU32 col = _theme->style()->debuggerHeadColor;
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_Threads());
		ImGui::PopStyleColor();

		VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

		const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable |
			ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("##Threads", 4, flags)) {
			typedef std::function<void(const VM::SCRIPT_CTX::Array &, const VM::ThreadStack::Array, const VM::SCRIPT_CTX &, int, int, ImU32, ImU32)> Debugger;

			Debugger debugThread = nullptr;
			debugThread = [&debugThread] (const VM::SCRIPT_CTX::Array &all, const VM::ThreadStack::Array &allThreadStacks, const VM::SCRIPT_CTX &thread, int index, int level, ImU32 majCol, ImU32 minCol) -> void {
				if (ImGui::TreeNode("{...}")) {
					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("PC=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%04X", thread.PC);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Bank=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%d", thread.bank);
					ImGui::PopStyleColor();

					if (thread.next == NULL) {
						ImGui::PushStyleColor(ImGuiCol_Text, majCol);
						ImGui::TextUnformatted("Next=");
						ImGui::PopStyleColor();
						ImGui::SameLine();
						ImGui::PushStyleColor(ImGuiCol_Text, minCol);
						ImGui::TextUnformatted("NOTHING");
						ImGui::PopStyleColor();
					} else {
						if (level <= DEBUGGER_TABLE_LEVEL_MAX_COUNT) {
							VM::SCRIPT_CTX::Array::const_iterator it = std::find_if(
								all.begin(), all.end(),
								[thread] (const VM::SCRIPT_CTX::Ref &ref) -> bool {
									return thread.next == ref.pointer.address;
								}
							);
							if (it == all.end()) {
								ImGui::PushStyleColor(ImGuiCol_Text, majCol);
								ImGui::TextUnformatted("Next=");
								ImGui::PopStyleColor();
								ImGui::SameLine();
								ImGui::PushStyleColor(ImGuiCol_Text, minCol);
								ImGui::Text("%04X...", thread.next);
								ImGui::PopStyleColor();
							} else {
								ImGui::PushStyleColor(ImGuiCol_Text, majCol);
								ImGui::TextUnformatted("Next=");
								ImGui::PopStyleColor();
								ImGui::SameLine();

								const VM::SCRIPT_CTX &next = it->data;
								const int idx = (int)(it - all.begin());
								debugThread(all, allThreadStacks, next, idx, level + 1, majCol, minCol);
							}
						} else {
							ImGui::PushStyleColor(ImGuiCol_Text, majCol);
							ImGui::TextUnformatted("Next=");
							ImGui::PopStyleColor();
							ImGui::SameLine();
							ImGui::PushStyleColor(ImGuiCol_Text, minCol);
							ImGui::Text("%04X...", thread.next);
							ImGui::PopStyleColor();
						}
					}
					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("SP=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%04X", thread.stack_ptr);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Stack base=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%04X", thread.base_addr);
					ImGui::PopStyleColor();

					if (index >= 0 && index < (int)allThreadStacks.size()) {
						const VM::ThreadStack::Ref &stkRef = allThreadStacks[index];
						if (!stkRef.data.buffer.empty()) {
							ImGui::PushStyleColor(ImGuiCol_Text, majCol);
							ImGui::TextUnformatted("Stack=");
							ImGui::PopStyleColor();
							ImGui::SameLine();

							const Int16* buffer = (Int16*)&stkRef.data.buffer.front();
							ImGui::PushStyleColor(ImGuiCol_Text, minCol);
							if (ImGui::TreeNode("[...]")) {
								for (int i = 0; i < (int)(stkRef.data.buffer.size() / DEBUGGER_WORD_SIZE); ++i) {
									ImGui::PushID(i);
									{
										ImGui::PushStyleColor(ImGuiCol_Text, majCol);
										ImGui::Text("[%d]=", i);
										ImGui::SameLine();
										ImGui::PopStyleColor();

										const Int16 val = buffer[i];
										ImGui::Text("%d(%04X)", val, val);

										if (stkRef.data.count == i) {
											ImGui::SameLine();
											ImGui::PushStyleColor(ImGuiCol_Text, majCol);
											ImGui::TextUnformatted("<-");
											ImGui::PopStyleColor();
										}
									}
									ImGui::PopID();
								}

								ImGui::TreePop();
							}
							ImGui::PopStyleColor();
						}
					}

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("ID=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%d", thread.ID);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Handle=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%04X", thread.hthread);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Terminated=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%s", thread.terminated ? "true" : "false");
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Waitable=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%s", thread.waitable ? "true" : "false");
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Locks=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%d", thread.lock_count);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Fn=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%04X", thread.update_fn);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, majCol);
					ImGui::TextUnformatted("Fn bank=");
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, minCol);
					ImGui::Text("%d", thread.update_fn_bank);
					ImGui::PopStyleColor();

					ImGui::TreePop();
				}
			};

			const float width0 = ImGui::GetFontSize() * 2.0f;
			const float width = ImGui::GetFontSize() * 3.0f;
			const ImU32 col = _theme->style()->debuggerHeadColor;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Order(), ImGuiTableColumnFlags_WidthFixed, width0);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_ID(), ImGuiTableColumnFlags_WidthFixed, width);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Addr(), ImGuiTableColumnFlags_WidthFixed, width);
			ImGui::TableSetupColumn(_theme->windowEmulator_CodeDebugger_Value(), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
			if (sortSpecs->SpecsDirty && sortSpecs->SpecsCount > 0) {
				const ImGuiTableColumnSortSpecs* spec = &sortSpecs->Specs[0];
				const int column = spec->ColumnIndex;
				const bool ascending = (spec->SortDirection == ImGuiSortDirection_Ascending);
				_options.threadSortingRule = SortingRule(column, ascending);

				sortSpecs->SpecsDirty = false;
			}

			int j = 0;
			for (const VM::SCRIPT_CTX::Ref &thread : snapshot->threads) {
				const int ord = thread.order;
				const UInt8 id = thread.data.ID;
				const UInt16 address = (UInt16)thread.pointer.address;
				const VM::SCRIPT_CTX &val = thread.data;

				ImGui::TableNextRow();
				ImGui::PushID(j);
				{
					ImGui::PushStyleColor(ImGuiCol_Text, col);
					ImGui::TableSetColumnIndex(0); ImGui::Text("%d", ord);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%d", id);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerInfoColor);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%04X", address);
					ImGui::PopStyleColor();

					ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
					ImGui::TableSetColumnIndex(3); debugThread(snapshot->threads, _snapshot.threadStacks, val, j, 1, _theme->style()->debuggerMajorColor, _theme->style()->debuggerMinorColor);
					ImGui::PopStyleColor();
				}
				ImGui::PopID();
				++j;
			}

			ImGui::EndTable();
		}
	}
	void objects(Snapshot* snapshot) {
		ImGuiStyle &style = ImGui::GetStyle();

		/*const ImU32 col = _theme->style()->debuggerHeadColor;
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_theme->tooltipEmulator_CodeDebugger_Objects());
		ImGui::PopStyleColor();*/

		VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

		(void)snapshot;
		// TODO: DBG.
		//_snapshot.actors
		//_snapshot.projectileDefs
		//_snapshot.projectiles
		//_snapshot.triggers
		//_snapshot.scene
	}
	void registers(void) {
		ImVec2 pos = ImGui::GetCursorPos();
		pos.x += ImGui::GetContentRegionAvail().x * 0.5f;

		const Device::Registers regs = _device->readRegisters();

		const UInt16 AF = (regs.A << 8) | ((regs.F.Z << 7) | (regs.F.N << 6) | (regs.F.H << 5) | (regs.F.C << 4));
		const UInt16 BC = regs.BC;
		const UInt16 DE = regs.DE;
		const UInt16 HL = regs.HL;
		const UInt16 SP = regs.SP;
		const UInt16 PC = regs.PC;

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerInfoColor);
		ImGui::Text("%c %c %c %c", regs.F.Z ? 'Z' : '_', regs.F.N ? 'N' : '_', regs.F.H ? 'H' : '_', regs.F.C ? 'C' : '_');
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("AF=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%04X", AF);
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("BC=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%04X", BC);
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("DE=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%04X", DE);
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("HL=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%04X", HL);
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("SP=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%04X", SP);
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("PC=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%04X", PC);
		ImGui::PopStyleColor();

		UInt8 LCDC = 0;
		_device->readRam(0xff40, &LCDC);
		UInt8 STAT = 0;
		_device->readRam(0xff41, &STAT);
		UInt8 LY = 0;
		_device->readRam(0xff44, &LY);
		UInt8 LYC = 0;
		_device->readRam(0xff45, &LYC);
		UInt8 IE = 0;
		_device->readRam(0xffff, &IE);
		UInt8 IF = 0;
		_device->readRam(0xff0f, &IF);
		UInt8 BANK = 0;
		FarPtr pc;
		if (probeCurrentProgramCounter(pc))
			BANK = (UInt8)pc.bank;

		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("LCDC=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", LCDC);
		ImGui::PopStyleColor();

		pos.y = ImGui::GetCursorPosY();
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("STAT=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", STAT);
		ImGui::PopStyleColor();

		pos.y = ImGui::GetCursorPosY();
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("LY  =");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", LY);
		ImGui::PopStyleColor();

		pos.y = ImGui::GetCursorPosY();
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("LYC =");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", LYC);
		ImGui::PopStyleColor();

		pos.y = ImGui::GetCursorPosY();
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("IE  =");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", IE);
		ImGui::PopStyleColor();

		pos.y = ImGui::GetCursorPosY();
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("IF  =");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", IF);
		ImGui::PopStyleColor();

		pos.y = ImGui::GetCursorPosY();
		ImGui::SetCursorPos(pos);
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
		ImGui::Text("BANK=");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
		ImGui::Text("%02X", BANK);
		ImGui::PopStyleColor();
	}
	void ram(void) {
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const float lineHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		if (lineHeight <= Math::EPSILON<float>()) return;
		const float panelHeight = ImGui::GetContentRegionAvail().y;
		const int visibleLineCount = (int)std::ceil(panelHeight / lineHeight);
		const int totalLineCount = std::numeric_limits<UInt16>::max() + 1;
		const float totalHeight = lineHeight * totalLineCount;

		const float scrollbarWidth = 12.0f;
		float scrollY = ImGui::GetScrollY();

		int startIndex = (int)(scrollY / lineHeight);
		int endIndex = startIndex + (int)std::ceil(panelHeight / lineHeight) + 1;
		startIndex = Math::clamp(startIndex, 0, totalLineCount);
		endIndex = Math::clamp(endIndex, startIndex, totalLineCount);
		if (startIndex > 0) 
			ImGui::Dummy(ImVec2(0.0f, startIndex * lineHeight));

		const bool incMode = _options.ramView == 0;
		for (int i = startIndex; i < endIndex; ++i) {
			const int address = incMode ?
				(i) :
				(std::numeric_limits<UInt16>::max() - i);
			UInt8 byte = 0;
			_device->readRam((UInt16)address, &byte);

			const char* desc = nullptr;
			bool readonly = false;
			bool prohibited = false;
			const char* type = getAddressDescription((UInt16)address, desc, readonly, prohibited);

			ImGui::AlignTextToFramePadding();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerHeadColor);
			ImGui::Text("%s  ", type);
			ImGui::SameLine();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMajorColor);
			ImGui::Text("%04X ", address);
			ImGui::SameLine();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerMinorColor);
			ImGui::Text("%02X   ", byte);
			ImGui::SameLine();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Text, _theme->style()->debuggerInfoColor);
			ImGui::Text(
				"%s %s",
				desc,
				prohibited ? "" :
				readonly ? "R" :
					"RW"
			);
			ImGui::PopStyleColor();
		}

		if (endIndex < totalLineCount)
			ImGui::Dummy(ImVec2(0.0f, (totalLineCount - endIndex) * lineHeight));

		if (ImGui::IsWindowHovered()) {
			const float wheel = io.MouseWheel;
			if (wheel != 0.0f)
				scrollY -= wheel * lineHeight * 3.0f;
		}
		const float maxScrollY = Math::max(0.0f, totalHeight - panelHeight);
		scrollY = Math::clamp(scrollY, 0.0f, maxScrollY);
		ImGui::SetScrollY(scrollY);

		if (totalHeight > panelHeight) {
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
			ImGui::InvisibleButton("##RamScrollbar", ImVec2(scrollbarWidth, panelHeight));

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
