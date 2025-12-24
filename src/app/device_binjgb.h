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

	virtual bool cartridgeHasCgbSupport(void) const override;
	virtual bool cartridgeHasExtSupport(void) const override;
	virtual bool cartridgeHasSgbSupport(void) const override;
	virtual int cartridgeRomSize(int* banks) const override;
	virtual int cartridgeSramSize(int* banks) const override;
	virtual bool cartridgeHasRtc(void) const override;

	virtual DeviceTypes deviceType(void) const override;
	virtual DeviceTypes enabledDeviceType(void) const override;
	virtual bool deviceHasCgbSupport(void) const override;
	virtual bool deviceHasExtSupport(void) const override;
	virtual bool deviceHasSgbSupport(void) const override;

	virtual Colour classicPalette(int index) const override;
	virtual void classicPalette(int index, const Colour &col) override;

	virtual bool isVariableSpeedSupported(void) const override;
	virtual bool isSgbBorderSupported(void) const override;
	virtual void* audioSpecification(void) const override;

	virtual bool inputEnabled(void) const override;
	virtual void inputEnabled(bool val) override;

	virtual void stroke(int key) override;

	virtual unsigned speed(void) const override;
	virtual bool speed(unsigned s) override;

	virtual unsigned fps(void) const override;

	virtual bool canGetDuty(void) const override;
	virtual int getDuty(int* l) const override;

	virtual long long timeoutThreshold(void) const override;
	virtual void timeoutThreshold(long long val) override;

	virtual bool traceless(void) const override;

	virtual bool open(Bytes::Ptr rom, DeviceTypes deviceType, bool preferSgb, class Input* input, Bytes::Ptr sram, bool isEditor, bool useAudioDevice, bool traceless) override;
	virtual bool close(Bytes::Ptr sram) override;

	virtual bool update(
		class Window* wnd, class Renderer* rnd,
		double delta,
		class Texture* texture, class Texture* textureForBorderFrame,
		bool allowInput,
		const KeyboardModifiers* keyMods,
		AudioHandler handleAudio
	) override;

	virtual bool opened(void) const override;
	virtual bool paused(void) const override;
	virtual void pause(void) override;
	virtual void resume(void) override;

	virtual bool readRam(UInt16 address, UInt8* data) override;
	virtual bool readRam(UInt16 address, UInt16* data) override;
	virtual bool writeRam(UInt16 address, UInt8 data) override;
	virtual bool writeRam(UInt16 address, UInt16 data) override;

	virtual bool readSram(const Bytes* bytes) override;
	virtual bool writeSram(Bytes* bytes) override;

private:
	void setBwPalette(PaletteType type, u32 white, u32 light_gray, u32 dark_gray, u32 black);

	void updateVideo(
		class Window* wnd, class Renderer* rnd,
		double delta,
		class Texture* texture, class Texture* textureForBorderFrame,
		bool isSgb
	);
	void updateAudio(
		class Window* wnd, class Renderer* rnd,
		double delta,
		AudioHandler handleAudio
	);

	bool processStreaming(class Window* wnd, class Renderer* rnd);
	bool processShellCommand(class Window* wnd, class Renderer* rnd);

	static void onInput(JoypadButtons* joyp, void* data);
};

/* ===========================================================================} */

#endif /* __DEVICE_BINJGB_H__ */
