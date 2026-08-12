/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "../gbbasic.h"
#include "../utils/bytes.h"
#include "../utils/colour.h"
#include <deque>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef DEVICE_CLASSIC_PALETTE_0
#	define DEVICE_CLASSIC_PALETTE_0 0xffd0f8e0
#endif /* DEVICE_CLASSIC_PALETTE_0 */
#ifndef DEVICE_CLASSIC_PALETTE_1
#	define DEVICE_CLASSIC_PALETTE_1 0xff70c088
#endif /* DEVICE_CLASSIC_PALETTE_1 */
#ifndef DEVICE_CLASSIC_PALETTE_2
#	define DEVICE_CLASSIC_PALETTE_2 0xff566834
#endif /* DEVICE_CLASSIC_PALETTE_2 */
#ifndef DEVICE_CLASSIC_PALETTE_3
#	define DEVICE_CLASSIC_PALETTE_3 0xff201808
#endif /* DEVICE_CLASSIC_PALETTE_3 */

#ifndef DEVICE_BASE_SPEED_FACTOR
#	define DEVICE_BASE_SPEED_FACTOR 100
#endif /* DEVICE_BASE_SPEED_FACTOR */
#ifndef DEVICE_DEFAULT_PREFERED_SPEED
#	define DEVICE_DEFAULT_PREFERED_SPEED 8
#endif /* DEVICE_DEFAULT_PREFERED_SPEED */

#ifndef DEVICE_KEY_BUFFER_LENGTH
#	define DEVICE_KEY_BUFFER_LENGTH 1 // 1 slot in pending buffer plus 1 `KEYC` register.
#endif /* DEVICE_KEY_BUFFER_LENGTH */

#ifndef DEVICE_PALETTE_COLOR_COUNT
#	define DEVICE_PALETTE_COLOR_COUNT 4
#endif /* DEVICE_PALETTE_COLOR_COUNT */
#ifndef DEVICE_TILE_BUFFER_WIDTH
#	define DEVICE_TILE_BUFFER_WIDTH 256
#endif /* DEVICE_TILE_BUFFER_WIDTH */
#ifndef DEVICE_TILE_BUFFER_HEIGHT
#	define DEVICE_TILE_BUFFER_HEIGHT 192
#endif /* DEVICE_TILE_BUFFER_HEIGHT */
#ifndef DEVICE_MAP_BUFFER_WIDTH
#	define DEVICE_MAP_BUFFER_WIDTH 32
#endif /* DEVICE_MAP_BUFFER_WIDTH */
#ifndef DEVICE_MAP_BUFFER_HEIGHT
#	define DEVICE_MAP_BUFFER_HEIGHT 32
#endif /* DEVICE_MAP_BUFFER_HEIGHT */
#ifndef DEVICE_MAP_BUFFER_SIZE
#	define DEVICE_MAP_BUFFER_SIZE (DEVICE_MAP_BUFFER_WIDTH * DEVICE_MAP_BUFFER_HEIGHT)
#endif /* DEVICE_MAP_BUFFER_SIZE */
#ifndef DEVICE_OBJ_COUNT
#	define DEVICE_OBJ_COUNT 40
#endif /* DEVICE_OBJ_COUNT */

#ifndef DEVICE_AUDIO_CHANNEL_COUNT
#	define DEVICE_AUDIO_CHANNEL_COUNT 4
#endif /* DEVICE_AUDIO_CHANNEL_COUNT */

/* ===========================================================================} */

/*
** {===========================================================================
** Device
*/

class Device : public virtual Object {
public:
	/**< Common. */

	typedef std::shared_ptr<Device> Ptr;

	/**< Emulation structures. */

	enum class CoreTypes {
		BINJGB,
		COUNT
	};

	enum class DeviceTypes {
		CLASSIC,
		COLORED,
		CLASSIC_EXTENDED,
		COLORED_EXTENDED,
		SUPER,
		SUPER_EXTENDED
	};

	/**< Input structures. */

	enum class CursorTypes {
		NONE,
		POINTER,
		HAND,
		BUSY
	};

	struct KeyboardModifiers {
		bool ctrl = false;
		bool shift = false;
		bool alt = false;
		bool super = false;

		KeyboardModifiers();
		KeyboardModifiers(bool ctrl_, bool shift_, bool alt_, bool super_);
	};

	typedef std::deque<int> KeyBuffer;

	/**< Debugger operations. */

	struct Registers {
		UInt8 A;
		union {
			struct { Byte Z, N, H, C; };
			UInt32 Val;
		} F;
		union {
			struct { UInt8 C; UInt8 B; };
			UInt16 BC;
		};
		union {
			struct { UInt8 E; UInt8 D; };
			UInt16 DE;
		};
		union {
			struct { UInt8 L; UInt8 H; };
			UInt16 HL;
		};
		UInt16 SP;
		UInt16 PC;

		Registers();
	};

	struct Breakpoint {
		bool valid = false;
		UInt8 bank = 0;
		UInt16 address = 0;

		Breakpoint();
		Breakpoint(UInt8 b, UInt16 addr);
	};

	/**< VRAM debugging structures. */

	enum class Colors {
		WHITE = 0,
		LIGHT_GRAY,
		DARK_GRAY,
		BLACK
	};
	enum class PaletteTypes {
		BGP = 0,
		OBP0,
		OBP1,
		COUNT
	};
	enum class CgbPaletteTypes {
		BGCP = 0,
		OBCP,
		COUNT
	};

	enum class ObjPriorities {
		ABOVE_BG,
		BEHIND_BG
	};

	enum class LayerTypes {
		BG,
		WINDOW
	};

	enum class TileSourceTypes {
		INVALID = ~0,
		FROM_8800_TO_97FF = 0,
		FROM_8000_TO_8FFF
	};
	enum class MapSourceTypes {
		INVALID = ~0,
		FROM_9800_TO_9BFF = 0,
		FROM_9C00_TO_9FFF
	};

	struct PaletteGrayscale {
		Colors color[DEVICE_PALETTE_COLOR_COUNT];
	};
	struct PaletteRgba {
		UInt32 color[DEVICE_PALETTE_COLOR_COUNT];
	};

	typedef std::array<UInt8, DEVICE_TILE_BUFFER_WIDTH * DEVICE_TILE_BUFFER_HEIGHT> TileBuffer;
	typedef std::array<UInt8, DEVICE_MAP_BUFFER_SIZE> MapBuffer;
	struct Obj {
		UInt8 y = 0;
		UInt8 x = 0;
		UInt8 tile = 0;
		UInt8 byte3 = 0;
		ObjPriorities priority = ObjPriorities::ABOVE_BG;
		bool yFlip = false;
		bool xFlip = false;
		UInt8 palette = 0;
		UInt8 bank = 0;
		UInt8 cgbPalette = 0;
	};

	/**< Protocols. */

	class Protocol {
	public:
		virtual void streamed(class Window* wnd, class Renderer* rnd, Bytes::Ptr bytes) = 0;

		virtual void sync(class Window* wnd, class Renderer* rnd, const char* module) = 0;

		virtual void debug(const char* msg) = 0;

		virtual void cursor(CursorTypes mode) = 0;

		virtual bool running(void) const = 0;
		virtual void pause(class Window* wnd, class Renderer* rnd) = 0;
		virtual void stop(class Window* wnd, class Renderer* rnd) = 0;

		virtual void breakpointHit(void) = 0;
	};

	/**< Handlers. */

	typedef std::function<bool /* whether has been handled */ (void* /* specification */, Bytes* /* buffer */, UInt32 /* length */)> AudioHandler;

public:
	/**< Common. */

	GBBASIC_CLASS_TYPE('D', 'V', 'C', 'E')

public:
	/**< Cartridge properties. */

	virtual bool isCartridgeCgbCompatible(void) const = 0;
	virtual bool isCartridgeExtCompatible(void) const = 0;
	virtual bool isCartridgeSgbCompatible(void) const = 0;
	virtual int cartridgeRomSize(int* banks /* nullable */) const = 0;
	virtual int cartridgeSramSize(int* banks /* nullable */) const = 0;
	virtual int cartridgeMbcType(void) const = 0;
	virtual const char* cartridgeMbcTypeName(void) const = 0;
	virtual bool cartridgeHasRtc(void) const = 0;
	virtual bool cartridgeHasRumble(void) const = 0;

	/**< Device properties. */

	virtual DeviceTypes deviceType(void) const = 0;
	virtual DeviceTypes enabledDeviceType(void) const = 0;
	virtual bool isDeviceCgbCompatible(void) const = 0;
	virtual bool isDeviceExtCompatible(void) const = 0;
	virtual bool isDeviceSgbCompatible(void) const = 0;

	/**< Emulation properties. */

	virtual bool supportsGettingDuty(void) const = 0;
	virtual bool supportsVariableSpeed(void) const = 0;
	virtual bool supportsSgbBorder(void) const = 0;
	virtual bool supportsBreakpoint(void) const = 0;
	virtual bool supportsVramDebugging(void) const = 0;
	virtual bool supportsMutingAudioChannel(void) const = 0;

	virtual Colour classicPalette(int index) const = 0;
	virtual void classicPalette(int index, const Colour &col) = 0;

	virtual unsigned speed(void) const = 0;
	virtual bool speed(unsigned s) = 0;

	virtual unsigned fps(void) const = 0;

	virtual int getDuty(int* l /* nullable */) const = 0;

	virtual long long timeoutThreshold(void) const = 0;
	virtual void timeoutThreshold(long long val) = 0;

	virtual void* audioSpecification(void) const = 0;
	virtual bool mutedAudioChannel(int ch) const = 0;
	virtual void muteAudioChannel(int ch, bool muted) = 0;

	virtual bool traceless(void) const = 0;

	/**< Input module. */

	virtual bool inputEnabled(void) const = 0;
	virtual void inputEnabled(bool val) = 0;

	virtual void stroke(int key) = 0;

	/**< Debugger operations. */

	virtual int getBreakpointCount(void) const = 0;
	virtual Breakpoint getBreakpoint(int idx) const = 0;
	virtual Breakpoint getBreakpointByAddress(UInt8 bank, UInt16 addr) const = 0;
	virtual int addBreakpoint(UInt8 bank, UInt16 addr) = 0;
	virtual void removeBreakpoint(int idx) = 0;
	virtual void clearBreakpoints(void) = 0;
	virtual void setBreakpointEnabled(int idx, bool enabled) = 0;

	/**< VRAM debugging operations. */

	virtual TileSourceTypes getTileSourceType(void) const = 0;
	virtual MapSourceTypes getMapSourceType(LayerTypes layer) const = 0;
	virtual PaletteGrayscale getPalette(PaletteTypes type) const = 0;
	virtual PaletteRgba getPaletteRgba(PaletteTypes type) const = 0;
	virtual PaletteRgba getCgbPaletteRgba(CgbPaletteTypes type, int index) const = 0;
	virtual PaletteRgba getSgbPaletteRgba(int index) const = 0;
	virtual void getTileBuffer(TileBuffer &buf) const = 0;
	virtual void getMapBuffer(MapSourceTypes type, MapBuffer &buf) const = 0;
	virtual void getMapAttrBuffer(MapSourceTypes type, MapBuffer &buf) const = 0;
	virtual void getSgbMapAttrBuffer(UInt8 map[90]) const = 0;
	virtual void getBgScroll(UInt8* x /* nullable */, UInt8* y /* nullable */) const = 0;
	virtual void getWindowScroll(UInt8* x /* nullable */, UInt8* y /* nullable */) const = 0;

	virtual bool getDisplay(void) const = 0;
	virtual bool getBgDisplay(void) const = 0;
	virtual bool getWindowDisplay(void) const = 0;
	virtual bool getObjDisplay(void) const = 0;

	virtual bool is8x16Obj(void) const = 0;
	virtual Obj getObj(int index) const = 0;
	virtual bool isObjVisible(const Obj* obj) const = 0;

	/**< Opening and closing. */

	virtual bool open(Bytes::Ptr rom, DeviceTypes deviceType, bool preferSgb, class Input* input /* nullable */, Bytes::Ptr sram /* nullable */, bool isEditor, bool useAudioDevice, bool traceless) = 0;
	virtual bool close(Bytes::Ptr sram /* nullable */) = 0;

	/**< Updating. */

	/**
	 * @param[out] isNewFrame
	 */
	virtual bool update(
		class Window* wnd, class Renderer* rnd,
		double delta,
		class Texture* texture /* nullable */, class Texture* textureForBorderFrame /* nullable */, bool* resetBorderFrame,
		bool allowInput, const KeyboardModifiers* keyMods /* nullable */,
		bool* isNewFrame /* nullable */,
		AudioHandler handleAudio /* nullable */
	) = 0;

	/**< Emulation operations. */

	virtual bool opened(void) const = 0;
	virtual bool paused(void) const = 0;
	virtual void pause(void) = 0;
	virtual void resume(void) = 0;

	/**< Memory bus operations. */

	virtual Registers readRegisters(void) = 0;
	virtual void writeRegisters(const Registers &regs) = 0;

	virtual bool readRam(UInt16 address, UInt8* data) = 0;
	virtual bool readRam(UInt16 address, UInt16* data) = 0;
	virtual bool writeRam(UInt16 address, UInt8 data) = 0;
	virtual bool writeRam(UInt16 address, UInt16 data) = 0;

	virtual bool readSram(const Bytes* bytes) = 0;
	virtual bool writeSram(Bytes* bytes) = 0;

	/**< Creating and destroying. */

	static Device* create(CoreTypes type, Protocol* dbgListener /* nullable */);
	static void destroy(Device* ptr);
};

/* ===========================================================================} */

#endif /* __DEVICE_H__ */
