/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
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

/* ===========================================================================} */

/*
** {===========================================================================
** Device
*/

class Device : public virtual Object {
public:
	typedef std::shared_ptr<Device> Ptr;

	enum class CoreTypes {
		BINJGB,
		COUNT
	};

	enum class DeviceTypes {
		CLASSIC,
		COLORED,
		CLASSIC_EXTENDED,
		COLORED_EXTENDED
	};

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

	class Protocol {
	public:
		virtual void streamed(class Window* wnd, class Renderer* rnd, Bytes::Ptr bytes) = 0;

		virtual void sync(class Window* wnd, class Renderer* rnd, const char* module) = 0;

		virtual void debug(const char* msg) = 0;

		virtual void cursor(CursorTypes mode) = 0;

		virtual bool running(void) const = 0;
		virtual void pause(class Window* wnd, class Renderer* rnd) = 0;
		virtual void stop(class Window* wnd, class Renderer* rnd) = 0;
	};

	typedef std::function<bool /* whether has been handled */ (void* /* specification */, Bytes* /* buffer */, UInt32 /* length */)> AudioHandler;

public:
	GBBASIC_CLASS_TYPE('D', 'V', 'C', 'E')

public:
	virtual bool cartridgeHasCgbSupport(void) const = 0;
	virtual bool cartridgeHasExtensionSupport(void) const = 0;
	virtual bool cartridgeHasSgbSupport(void) const = 0;
	virtual int cartridgeRomSize(int* banks /* nullable */) const = 0;
	virtual int cartridgeSramSize(int* banks /* nullable */) const = 0;
	virtual bool cartridgeHasRtc(void) const = 0;
	virtual DeviceTypes deviceType(void) const = 0;
	virtual DeviceTypes enabledDeviceType(void) const = 0;

	virtual Colour classicPalette(int index) const = 0;
	virtual void classicPalette(int index, const Colour &col) = 0;

	virtual bool isVariableSpeedSupported(void) const = 0;
	virtual bool isSgbBorderSupported(void) const = 0;
	virtual void* audioSpecification(void) const = 0;

	virtual bool inputEnabled(void) const = 0;
	virtual void inputEnabled(bool val) = 0;

	virtual void stroke(int key) = 0;

	virtual unsigned speed(void) const = 0;
	virtual bool speed(unsigned s) = 0;

	virtual unsigned fps(void) const = 0;

	virtual bool canGetDuty(void) const = 0;
	virtual int getDuty(int* l /* nullable */) const = 0;

	virtual long long timeoutThreshold(void) const = 0;
	virtual void timeoutThreshold(long long val) = 0;

	virtual bool traceless(void) const = 0;

	virtual bool open(Bytes::Ptr rom, DeviceTypes deviceType, class Input* input /* nullable */, Bytes::Ptr sram /* nullable */, bool isEditor, bool useAudioDevice, bool traceless) = 0;
	virtual bool close(Bytes::Ptr sram /* nullable */) = 0;

	virtual bool update(
		class Window* wnd, class Renderer* rnd,
		double delta,
		class Texture* texture /* nullable */, class Texture* textureForBorderFrame /* nullable */,
		bool allowInput,
		const KeyboardModifiers* keyMods /* nullable */,
		AudioHandler handleAudio /* nullable */
	) = 0;

	virtual bool opened(void) const = 0;
	virtual bool paused(void) const = 0;
	virtual void pause(void) = 0;
	virtual void resume(void) = 0;

	virtual bool readRam(UInt16 address, UInt8* data) = 0;
	virtual bool readRam(UInt16 address, UInt16* data) = 0;
	virtual bool writeRam(UInt16 address, UInt8 data) = 0;
	virtual bool writeRam(UInt16 address, UInt16 data) = 0;

	virtual bool readSram(const Bytes* bytes) = 0;
	virtual bool writeSram(Bytes* bytes) = 0;

	static Device* create(CoreTypes type, Protocol* dbgListener /* nullable */);
	static void destroy(Device* ptr);
};

/* ===========================================================================} */

#endif /* __DEVICE_H__ */
