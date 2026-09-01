/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __DEVICE_BINJGB_H__
#define __DEVICE_BINJGB_H__

#include "../gbbasic.h"
#include "device.h"
#include "../../lib/binjgb/src/emulator-debug.h"
#include <SDL.h>

/*
** {===========================================================================
** Device based on the binjgb project
*/

class DeviceBinjgb : public Device, public virtual Object {
private:
	enum class SgbFadeOperations {
		NONE,
		FADEOUT,
		FADEIN,
		INIT
	};

private:
	bool _opened = false;
	bool _traceless = false;
	Protocol* _debugListener = nullptr;
	DeviceTypes _deviceType = DeviceTypes::COLORED; // Determined by device.
	DeviceTypes _enabledDeviceType = DeviceTypes::COLORED; // Determined by both device and ROM.
	int _cartridgeType = 0;
	int _cartridgeSuperType = 0;
	int _cartridgeMbcType = 0;
	int _romType = 0;
	int _sramType = 0;
	int _cartridgeOldLicenseCode = 0;
	Colour _classicPalette[4];
	Emulator* _emulator = nullptr;
	bool _emulatorPaused = false;
	bool _breakAtNextInstruction = false;
	mutable bool _breakAtNextBasicInstruction = false;
	UInt16 _basicStepBreakpointAddress = 0;
	ProgramCounterGetter _basicBreakpointProgramCounterGetter = nullptr;
	UInt16 _basicBreakpointMask[2];
	double _rtcTicks = 0;
	Bytes::Ptr _streamingBuffer = nullptr;
	RGBA _sgbBorderVideoBuffer[SGB_SCREEN_WIDTH * SGB_SCREEN_HEIGHT];
	Colour* _sgbBorderVideoFadeBuffer = nullptr;
	SgbFadeOperations _sgbBorderFade = SgbFadeOperations::INIT;
	double _sgbBorderFadeTicks = 0;
	SDL_AudioDeviceID _audioDeviceId = 0;
	SDL_AudioSpec _audioSpec;
	SDL_AudioCVT _audioCvt;
	size_t _audioCvtBufferLength = 0;
	Bytes::Ptr _audioBuffer = nullptr;
	bool _audioChannelMuted[DEVICE_AUDIO_CHANNEL_COUNT];
	class Input* _input = nullptr; // Foreign.
	bool _inputEnabled = true;
	KeyboardModifiers _keyboardModifiers;
	KeyBuffer _keyBuffer; // FIFO.
	unsigned _speed = DEVICE_BASE_SPEED_FACTOR * 1;
	Ticks _previousTicks = 0;
	unsigned _updatedFrameCount = 0;
	double _updatedSeconds = 0;
	unsigned _fps = 59;
	int _duty = 0;
	long long _timeoutThreshold = 0;
	int _longPeriodDuty = 0;
	int _longPeriodDutyMaxValue = 0;
	int _longPeriodDutyTicks = 0;

public:
	DeviceBinjgb(Protocol* dbgListener);
	virtual ~DeviceBinjgb() override;

	virtual unsigned type(void) const override;

	virtual bool isCartridgeCgbCompatible(void) const override;
	virtual bool isCartridgeExtCompatible(void) const override;
	virtual bool isCartridgeSgbCompatible(void) const override;
	virtual int cartridgeRomSize(int* banks) const override;
	virtual int cartridgeSramSize(int* banks) const override;
	virtual int cartridgeMbcType(void) const override;
	virtual const char* cartridgeMbcTypeName(void) const override;
	virtual bool cartridgeHasRtc(void) const override;
	virtual bool cartridgeHasRumble(void) const override;

	virtual DeviceTypes deviceType(void) const override;
	virtual DeviceTypes enabledDeviceType(void) const override;
	virtual bool isDeviceCgbCompatible(void) const override;
	virtual bool isDeviceExtCompatible(void) const override;
	virtual bool isDeviceSgbCompatible(void) const override;

	virtual bool supportsGettingDuty(void) const override;
	virtual bool supportsVariableSpeed(void) const override;
	virtual bool supportsSgbBorder(void) const override;
	virtual bool supportsBreakpoint(void) const override;
	virtual bool supportsVramDebugging(void) const override;
	virtual bool supportsMutingAudioChannel(void) const override;

	virtual Colour classicPalette(int index) const override;
	virtual void classicPalette(int index, const Colour &col) override;

	virtual unsigned speed(void) const override;
	virtual bool speed(unsigned s) override;

	virtual unsigned fps(void) const override;

	virtual int getDuty(int* l) const override;

	virtual long long timeoutThreshold(void) const override;
	virtual void timeoutThreshold(long long val) override;

	virtual void* audioSpecification(void) const override;
	virtual bool mutedAudioChannel(int ch) const override;
	virtual void muteAudioChannel(int ch, bool muted) override;

	virtual bool traceless(void) const override;

	virtual bool inputEnabled(void) const override;
	virtual void inputEnabled(bool val) override;

	virtual void stroke(int key) override;

	virtual int getBreakpointCount(void) const override;
	virtual Breakpoint getBreakpoint(int idx) const override;
	virtual Breakpoint getBreakpointByAddress(UInt8 bank, UInt16 addr) const override;
	virtual int addBreakpoint(UInt8 bank, UInt16 addr) override;
	virtual void removeBreakpoint(int idx) override;
	virtual void clearBreakpoints(void) override;
	virtual void setBreakpointEnabled(int idx, bool enabled) override;
	virtual void breakAtNextInstruction(void) override;
	virtual void breakAtNextBasicInstruction(void) override;
	virtual void calculateBasicBreakpointMask(int stepAddr, ProgramCounterGetter pc, BreakpointEnumerator en) override;

	virtual TileSourceTypes getTileSourceType(void) const override;
	virtual MapSourceTypes getMapSourceType(LayerTypes layer) const override;
	virtual PaletteGrayscale getPalette(PaletteTypes type) const override;
	virtual PaletteRgba getPaletteRgba(PaletteTypes type) const override;
	virtual PaletteRgba getCgbPaletteRgba(CgbPaletteTypes type, int index) const override;
	virtual PaletteRgba getSgbPaletteRgba(int index) const override;
	virtual void getTileBuffer(TileBuffer &buf) const override;
	virtual void getMapBuffer(MapSourceTypes type, MapBuffer &buf) const override;
	virtual void getMapAttrBuffer(MapSourceTypes type, MapBuffer &buf) const override;
	virtual void getSgbMapAttrBuffer(UInt8 map[90]) const override;
	virtual void getBgScroll(UInt8* x, UInt8* y) const override;
	virtual void getWindowScroll(UInt8* x, UInt8* y) const override;

	virtual bool getDisplay(void) const override;
	virtual bool getBgDisplay(void) const override;
	virtual bool getWindowDisplay(void) const override;
	virtual bool getObjDisplay(void) const override;

	virtual bool is8x16Obj(void) const override;
	virtual Obj getObj(int index) const override;
	virtual bool isObjVisible(const Obj* obj) const override;

	virtual bool open(Bytes::Ptr rom, DeviceTypes deviceType, bool preferSgb, class Input* input, Bytes::Ptr sram, bool isEditor, bool useAudioDevice, bool traceless) override;
	virtual bool close(Bytes::Ptr sram) override;

	virtual bool update(
		class Window* wnd, class Renderer* rnd,
		double delta,
		class Texture* texture, class Texture* textureForBorderFrame, bool* resetBorderFrame,
		bool allowInput, const KeyboardModifiers* keyMods,
		bool* isNewFrame,
		AudioHandler handleAudio
	) override;

	virtual bool opened(void) const override;
	virtual bool paused(void) const override;
	virtual void pause(void) override;
	virtual void resume(void) override;

	virtual Registers readRegisters(void) const override;
	virtual void writeRegisters(const Registers &regs) override;

	virtual bool readRam(UInt16 address, UInt8* data) const override;
	virtual bool readRam(UInt16 address, UInt16* data) const override;
	virtual bool readRam(UInt16 address, Int8* data) const override;
	virtual bool readRam(UInt16 address, Int16* data) const override;
	virtual size_t readRam(UInt16 address, Byte* data, size_t len) const override;
	virtual bool writeRam(UInt16 address, UInt8 data) override;
	virtual bool writeRam(UInt16 address, UInt16 data) override;
	virtual bool writeRam(UInt16 address, Int8 data) override;
	virtual bool writeRam(UInt16 address, Int16 data) override;
	virtual size_t writeRam(UInt16 address, const Byte* data, size_t len) override;

	virtual bool readSram(const Bytes* bytes) const override;
	virtual bool writeSram(Bytes* bytes) override;

	virtual int currentBank(void) const override;

private:
	void setBwPalette(PaletteType type, u32 white, u32 light_gray, u32 dark_gray, u32 black);

	void updateVideo(
		class Window* wnd, class Renderer* rnd,
		double delta,
		class Texture* texture, class Texture* textureForBorderFrame, bool* resetBorderFrame,
		bool isSgb
	);
	void updateAudio(
		class Window* wnd, class Renderer* rnd,
		double delta,
		AudioHandler handleAudio
	);

	bool breakpointShouldHit(void) const;

	bool processStreaming(class Window* wnd, class Renderer* rnd);
	bool processShellCommand(class Window* wnd, class Renderer* rnd);

	static void onInput(JoypadButtons* joyp, void* data);
};

/* ===========================================================================} */

#endif /* __DEVICE_BINJGB_H__ */
