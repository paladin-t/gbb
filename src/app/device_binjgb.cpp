/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "device_binjgb.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/input.h"
#include "../utils/platform.h"
#include "../utils/renderer.h"
#include "../utils/rom_inspector.h"
#include "../utils/text.h"
#include "../utils/texture.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS
#	define DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS 0x0143
#endif /* DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS */
#ifndef DEVICE_BINJGB_CARTRIDGE_SUPER_TYPE_ADDRESS
#	define DEVICE_BINJGB_CARTRIDGE_SUPER_TYPE_ADDRESS 0x0146
#endif /* DEVICE_BINJGB_CARTRIDGE_SUPER_TYPE_ADDRESS */
#ifndef DEVICE_BINJGB_CARTRIDGE_MBC_TYPE_ADDRESS
#	define DEVICE_BINJGB_CARTRIDGE_MBC_TYPE_ADDRESS 0x0147
#endif /* DEVICE_BINJGB_CARTRIDGE_MBC_TYPE_ADDRESS */
#ifndef DEVICE_BINJGB_ROM_SIZE_ADDRESS
#	define DEVICE_BINJGB_ROM_SIZE_ADDRESS 0x0148
#endif /* DEVICE_BINJGB_ROM_SIZE_ADDRESS */
#ifndef DEVICE_BINJGB_SRAM_SIZE_ADDRESS
#	define DEVICE_BINJGB_SRAM_SIZE_ADDRESS 0x0149
#endif /* DEVICE_BINJGB_SRAM_SIZE_ADDRESS */
#ifndef DEVICE_BINJGB_CARTRIDGE_OLD_LICENSE_CODE_ADDRESS
#	define DEVICE_BINJGB_CARTRIDGE_OLD_LICENSE_CODE_ADDRESS 0x14b
#endif /* DEVICE_BINJGB_CARTRIDGE_OLD_LICENSE_CODE_ADDRESS */

// For `DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS`.
#ifndef DEVICE_BINJGB_CARTRIDGE_GB_ONLY_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_GB_ONLY_TYPE 0x00
#endif /* DEVICE_BINJGB_CARTRIDGE_GB_ONLY_TYPE */
#ifndef DEVICE_BINJGB_CARTRIDGE_CGB_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_CGB_TYPE 0x80 // The cartridge supports colored functions, but works on classic also.
#endif /* DEVICE_BINJGB_CARTRIDGE_CGB_TYPE */
#ifndef DEVICE_BINJGB_CARTRIDGE_CGB_ONLY_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_CGB_ONLY_TYPE 0xc0 // The cartridge works on colored device only (physically the same as 0x80).
#endif /* DEVICE_BINJGB_CARTRIDGE_CGB_ONLY_TYPE */
#ifndef DEVICE_BINJGB_CARTRIDGE_EXTENSION_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_EXTENSION_TYPE 0x20 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_CARTRIDGE_EXTENSION_TYPE */

// For `DEVICE_BINJGB_CARTRIDGE_SUPER_TYPE_ADDRESS`.
#ifndef DEVICE_BINJGB_CARTRIDGE_WITHOUT_SGB_SUPPORT_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_WITHOUT_SGB_SUPPORT_TYPE 0x00
#endif /* DEVICE_BINJGB_CARTRIDGE_WITHOUT_SGB_SUPPORT_TYPE */
#ifndef DEVICE_BINJGB_CARTRIDGE_WITH_SGB_SUPPORT_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_WITH_SGB_SUPPORT_TYPE 0x03 // The cartridge supports super functions.
#endif /* DEVICE_BINJGB_CARTRIDGE_WITH_SGB_SUPPORT_TYPE */

// For `DEVICE_BINJGB_CARTRIDGE_OLD_LICENSE_CODE_ADDRESS`.
#ifndef DEVICE_BINJGB_CARTRIDGE_WITH_NEW_LICENSE_CODE_TYPE
#	define DEVICE_BINJGB_CARTRIDGE_WITH_NEW_LICENSE_CODE_TYPE 0x33 // The cartridge supports super functions.
#endif /* DEVICE_BINJGB_CARTRIDGE_WITH_NEW_LICENSE_CODE_TYPE */

#ifndef DEVICE_BINJGB_EXTENSION_AREA_SIZE
#	define DEVICE_BINJGB_EXTENSION_AREA_SIZE 0x60 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_EXTENSION_AREA_SIZE */
#ifndef DEVICE_BINJGB_EXTENSION_START_ADDRESS
#	define DEVICE_BINJGB_EXTENSION_START_ADDRESS 0xfea0 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_EXTENSION_START_ADDRESS */
#ifndef DEVICE_BINJGB_EXTENSION_STATUS_REG
#	define DEVICE_BINJGB_EXTENSION_STATUS_REG 0xfea0 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_EXTENSION_STATUS_REG */
#ifndef DEVICE_BINJGB_PLATFORM_FLAGS_REG
#	define DEVICE_BINJGB_PLATFORM_FLAGS_REG 0xfea1 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_PLATFORM_FLAGS_REG */
#ifndef DEVICE_BINJGB_LOCALIZATION_FLAGS_REG
#	define DEVICE_BINJGB_LOCALIZATION_FLAGS_REG 0xfea2 // GBB EXTENSION. Not documented.
#endif /* DEVICE_BINJGB_LOCALIZATION_FLAGS_REG */
#ifndef DEVICE_BINJGB_TOUCH_X_REG
#	define DEVICE_BINJGB_TOUCH_X_REG 0xfea4 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TOUCH_X_REG */
#ifndef DEVICE_BINJGB_TOUCH_Y_REG
#	define DEVICE_BINJGB_TOUCH_Y_REG 0xfea5 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TOUCH_Y_REG */
#ifndef DEVICE_BINJGB_TOUCH_PRESSED_REG
#	define DEVICE_BINJGB_TOUCH_PRESSED_REG 0xfea6 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TOUCH_PRESSED_REG */
#ifndef DEVICE_BINJGB_KEY_MODIFIERS_REG
#	define DEVICE_BINJGB_KEY_MODIFIERS_REG 0xfea8 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_KEY_MODIFIERS_REG */
#ifndef DEVICE_BINJGB_KEYBOARD_PRESSED_REG
#	define DEVICE_BINJGB_KEYBOARD_PRESSED_REG 0xfea9 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_KEYBOARD_PRESSED_REG */
#ifndef DEVICE_BINJGB_STREAMING_STATUS_REG
#	define DEVICE_BINJGB_STREAMING_STATUS_REG 0xfeac // GBB EXTENSION.
#endif /* DEVICE_BINJGB_STREAMING_STATUS_REG */
#ifndef DEVICE_BINJGB_STREAMING_ADDRESS
#	define DEVICE_BINJGB_STREAMING_ADDRESS 0xfead // GBB EXTENSION.
#endif /* DEVICE_BINJGB_STREAMING_ADDRESS */
#ifndef DEVICE_BINJGB_TRANSFER_STATUS_REG
#	define DEVICE_BINJGB_TRANSFER_STATUS_REG 0xfeaf // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TRANSFER_STATUS_REG */
#ifndef DEVICE_BINJGB_TRANSFER_ADDRESS
#	define DEVICE_BINJGB_TRANSFER_ADDRESS 0xfeb0 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TRANSFER_ADDRESS */
#ifndef DEVICE_BINJGB_TRANSFER_MAX_SIZE
#	define DEVICE_BINJGB_TRANSFER_MAX_SIZE 0x40 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TRANSFER_MAX_SIZE */

// For `DEVICE_BINJGB_EXTENSION_STATUS_REG`.
#ifndef DEVICE_BINJGB_CPU_ANY_EXT_TYPE
#	define DEVICE_BINJGB_CPU_ANY_EXT_TYPE 0x01 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_CPU_ANY_EXT_TYPE */
#ifndef DEVICE_BINJGB_CPU_GB_EXT_TYPE
#	define DEVICE_BINJGB_CPU_GB_EXT_TYPE (0x10 | DEVICE_BINJGB_CPU_ANY_EXT_TYPE) // GBB EXTENSION.
#endif /* DEVICE_BINJGB_CPU_GB_EXT_TYPE */
#ifndef DEVICE_BINJGB_CPU_CGB_EXT_TYPE
#	define DEVICE_BINJGB_CPU_CGB_EXT_TYPE (0x20 | DEVICE_BINJGB_CPU_ANY_EXT_TYPE) // GBB EXTENSION.
#endif /* DEVICE_BINJGB_CPU_CGB_EXT_TYPE */
#ifndef DEVICE_BINJGB_CPU_SGB_EXT_TYPE
#	define DEVICE_BINJGB_CPU_SGB_EXT_TYPE (0x50 | DEVICE_BINJGB_CPU_ANY_EXT_TYPE) // GBB EXTENSION.
#endif /* DEVICE_BINJGB_CPU_SGB_EXT_TYPE */

// For `DEVICE_BINJGB_PLATFORM_FLAGS_REG`.
#ifndef DEVICE_BINJGB_PLATFORM_EDITOR_FLAG
#	define DEVICE_BINJGB_PLATFORM_EDITOR_FLAG 0b10000000 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_PLATFORM_EDITOR_FLAG */
#ifndef DEVICE_BINJGB_PLATFORM_WINDOWS_FLAG
#	define DEVICE_BINJGB_PLATFORM_WINDOWS_FLAG 0b00000001 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_PLATFORM_WINDOWS_FLAG */
#ifndef DEVICE_BINJGB_PLATFORM_LINUX_FLAG
#	define DEVICE_BINJGB_PLATFORM_LINUX_FLAG 0b00000010 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_PLATFORM_LINUX_FLAG */
#ifndef DEVICE_BINJGB_PLATFORM_MACOS_FLAG
#	define DEVICE_BINJGB_PLATFORM_MACOS_FLAG 0b00000100 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_PLATFORM_MACOS_FLAG */
#ifndef DEVICE_BINJGB_PLATFORM_HTML_FLAG
#	define DEVICE_BINJGB_PLATFORM_HTML_FLAG 0b00001000 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_PLATFORM_HTML_FLAG */

// For `DEVICE_BINJGB_TOUCH_PRESSED_REG`.
#ifndef DEVICE_BINJGB_MOUSE_BUTTON_0
#	define DEVICE_BINJGB_MOUSE_BUTTON_0 0x01 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_MOUSE_BUTTON_0 */
#ifndef DEVICE_BINJGB_MOUSE_BUTTON_1
#	define DEVICE_BINJGB_MOUSE_BUTTON_1 0x02 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_MOUSE_BUTTON_1 */

// For `DEVICE_BINJGB_STREAMING_STATUS_REG`.
#ifndef DEVICE_BINJGB_STREAMING_STATUS_READY
#	define DEVICE_BINJGB_STREAMING_STATUS_READY 0x00 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_STREAMING_STATUS_READY */
#ifndef DEVICE_BINJGB_STREAMING_STATUS_BUSY
#	define DEVICE_BINJGB_STREAMING_STATUS_BUSY 0x01 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_STREAMING_STATUS_BUSY */
#ifndef DEVICE_BINJGB_STREAMING_STATUS_FILLED
#	define DEVICE_BINJGB_STREAMING_STATUS_FILLED 0x02 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_STREAMING_STATUS_FILLED */
#ifndef DEVICE_BINJGB_STREAMING_STATUS_EOS
#	define DEVICE_BINJGB_STREAMING_STATUS_EOS 0x80 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_STREAMING_STATUS_EOS */

// For `DEVICE_BINJGB_TRANSFER_STATUS_REG`.
#ifndef DEVICE_BINJGB_TRANSFER_STATUS_READY
#	define DEVICE_BINJGB_TRANSFER_STATUS_READY 0x00 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TRANSFER_STATUS_READY */
#ifndef DEVICE_BINJGB_TRANSFER_STATUS_BUSY
#	define DEVICE_BINJGB_TRANSFER_STATUS_BUSY 0x01 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TRANSFER_STATUS_BUSY */
#ifndef DEVICE_BINJGB_TRANSFER_STATUS_FILLED
#	define DEVICE_BINJGB_TRANSFER_STATUS_FILLED 0x02 // GBB EXTENSION.
#endif /* DEVICE_BINJGB_TRANSFER_STATUS_FILLED */

#ifndef DEVICE_BINJGB_SRAM_MAX_SIZE
#	define DEVICE_BINJGB_SRAM_MAX_SIZE (1024 * 128)
#endif /* DEVICE_BINJGB_SRAM_MAX_SIZE */

#ifndef DEVICE_BINJGB_AUDIO_SPEC_FORMAT
#	define DEVICE_BINJGB_AUDIO_SPEC_FORMAT AUDIO_F32
#endif /* DEVICE_BINJGB_AUDIO_SPEC_FORMAT */
#ifndef DEVICE_BINJGB_AUDIO_SPEC_FREQUENCE
#	define DEVICE_BINJGB_AUDIO_SPEC_FREQUENCE 48000
#endif /* DEVICE_BINJGB_AUDIO_SPEC_FREQUENCE */
#ifndef DEVICE_BINJGB_AUDIO_SPEC_CHANNELS
#	define DEVICE_BINJGB_AUDIO_SPEC_CHANNELS 2
#endif /* DEVICE_BINJGB_AUDIO_SPEC_CHANNELS */
#ifndef DEVICE_BINJGB_AUDIO_SPEC_FRAMES
#	define DEVICE_BINJGB_AUDIO_SPEC_FRAMES 2048
#endif /* DEVICE_BINJGB_AUDIO_SPEC_FRAMES */
#ifndef DEVICE_BINJGB_AUDIO_CONVERT_FROM_U8
#	define DEVICE_BINJGB_AUDIO_CONVERT_FROM_U8(VAL, VOL) ((VOL) * (VAL) * (1 / 255.0f))
#endif /* DEVICE_BINJGB_AUDIO_CONVERT_FROM_U8 */

#ifndef DEVICE_BINJGB_DELTA_TIME_MIN_SECONDS
#	define DEVICE_BINJGB_DELTA_TIME_MIN_SECONDS 0.024
#endif /* DEVICE_BINJGB_DELTA_TIME_MIN_SECONDS */

#ifndef DEVICE_BINJGB_FADE_SECONDS
#	define DEVICE_BINJGB_FADE_SECONDS 0.35
#endif /* DEVICE_BINJGB_FADE_SECONDS */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

#define RAM_BANKS_ONLY     0x0f
#define RAM_BANK_REG       0x4000

#define RTC_TIMER_STOP     0b01000000
#define RTC_VALUE_SEC      0x08
#define RTC_VALUE_MIN      0x09
#define RTC_VALUE_HOUR     0x0a
#define RTC_VALUE_DAY      0x0b
#define RTC_VALUE_FLAGS    0x0c
#define RTC_SELECT_REG     0x4000
#define RTC_LATCH_REG      0x6000
#define RTC_VALUE_REG      0xa000

#define SWITCH_RAM(E, B)   emulator_write_u8_raw((E), RAM_BANK_REG, (B))
#define ENABLED_RAM(E)     (emulator_read_u8_raw((E), 0x0000) == 0x0a)
#define ENABLE_RAM(E)      emulator_write_u8_raw((E), 0x0000, 0x0a)
#define DISABLE_RAM(E)     emulator_write_u8_raw((E), 0x0000, 0x00)

inline void deviceBinjgbRtcSelect(Emulator* emu, UInt8 what) {
	SWITCH_RAM(emu, what & RAM_BANKS_ONLY);
}

inline void deviceBinjgbRtcStart(Emulator* emu, UInt8 start) {
	deviceBinjgbRtcSelect(emu, RTC_VALUE_FLAGS);
	if (start) {
		u8 val = emulator_read_u8_raw(emu, RTC_VALUE_REG);
		val &= ~RTC_TIMER_STOP;
		emulator_write_u8_raw(emu, RTC_VALUE_REG, val);
	} else {
		u8 val = emulator_read_u8_raw(emu, RTC_VALUE_REG);
		val |= RTC_TIMER_STOP;
		emulator_write_u8_raw(emu, RTC_VALUE_REG, val);
	}
}
inline void deviceBinjgbRtcLatch(Emulator* emu) {
	emulator_write_u8_raw(emu, RTC_LATCH_REG, 0);
	emulator_write_u8_raw(emu, RTC_LATCH_REG, 1);
}

inline UInt16 deviceBinjgbRtcGet(Emulator* emu, UInt8 part) {
	UInt16 ret;
	deviceBinjgbRtcSelect(emu, part);
	ret = RTC_VALUE_REG;
	if (part == RTC_VALUE_DAY) {
		deviceBinjgbRtcSelect(emu, RTC_VALUE_FLAGS);
		if (RTC_VALUE_REG & 0x01)
			ret |= 0x0100u;
	}

	return ret;
}
inline void deviceBinjgbRtcSet(Emulator* emu, UInt8 part, UInt16 val) {
	deviceBinjgbRtcSelect(emu, part);
	emulator_write_u8_raw(emu, RTC_VALUE_REG, (u8)val);
	if (part == RTC_VALUE_DAY) {
		deviceBinjgbRtcSelect(emu, RTC_VALUE_FLAGS);
		const u8 old = emulator_read_u8_raw(emu, RTC_VALUE_REG);
		const u8 new_ = (old & 0x0e) | (UInt8)((val >> 8) & 0x01);
		emulator_write_u8_raw(emu, RTC_VALUE_REG, new_);
	}
}

/* ===========================================================================} */

/*
** {===========================================================================
** Device based on the binjgb project
*/

DeviceBinjgb::DeviceBinjgb(Protocol* dbgListener)
	: _debugListener(dbgListener)
{
	_classicPalette[0] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_0);
	_classicPalette[1] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_1);
	_classicPalette[2] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_2);
	_classicPalette[3] = Colour::byRGBA8888(DEVICE_CLASSIC_PALETTE_3);

	memset(_basicBreakpointMask, 0, sizeof(_basicBreakpointMask));

	SDL_AudioSpec wantSpec;
	memset(&wantSpec, 0, sizeof(SDL_AudioSpec));
	wantSpec.freq     = DEVICE_BINJGB_AUDIO_SPEC_FREQUENCE;
	wantSpec.format   = DEVICE_BINJGB_AUDIO_SPEC_FORMAT;
	wantSpec.channels = DEVICE_BINJGB_AUDIO_SPEC_CHANNELS;
	wantSpec.samples  = (Uint16)(DEVICE_BINJGB_AUDIO_SPEC_FRAMES * DEVICE_BINJGB_AUDIO_SPEC_CHANNELS);
	wantSpec.callback = nullptr;
	wantSpec.userdata = nullptr;

	const int sampleSize = SDL_AUDIO_BITSIZE(wantSpec.format) / 8;
	wantSpec.size = wantSpec.samples * wantSpec.channels * sampleSize;

	_audioSpec = wantSpec;

	memset(_audioChannelMuted, 0, sizeof(_audioChannelMuted));
}

DeviceBinjgb::~DeviceBinjgb() {
	close(nullptr);
}

unsigned DeviceBinjgb::type(void) const {
	return TYPE();
}

bool DeviceBinjgb::isCartridgeCgbCompatible(void) const {
	const int y = (_cartridgeType & ~DEVICE_BINJGB_CARTRIDGE_EXTENSION_TYPE);

	return
		y == DEVICE_BINJGB_CARTRIDGE_CGB_TYPE ||
		y == DEVICE_BINJGB_CARTRIDGE_CGB_ONLY_TYPE;
}

bool DeviceBinjgb::isCartridgeExtCompatible(void) const {
	return !!(_cartridgeType & DEVICE_BINJGB_CARTRIDGE_EXTENSION_TYPE);
}

bool DeviceBinjgb::isCartridgeSgbCompatible(void) const {
	const int y = _cartridgeSuperType;
	const int l = _cartridgeOldLicenseCode;

	return
		y == DEVICE_BINJGB_CARTRIDGE_WITH_SGB_SUPPORT_TYPE &&
		l == DEVICE_BINJGB_CARTRIDGE_WITH_NEW_LICENSE_CODE_TYPE;
}

int DeviceBinjgb::cartridgeRomSize(int* banks) const {
	if (banks)
		*banks = 0;

	switch (_romType) {
	case 0x00:
		if (banks)
			*banks = 0;

		return 1024 * 32;
	case 0x01:
		if (banks)
			*banks = 4;

		return 1024 * 64;
	case 0x02:
		if (banks)
			*banks = 8;

		return 1024 * 128;
	case 0x03:
		if (banks)
			*banks = 16;

		return 1024 * 256;
	case 0x04:
		if (banks)
			*banks = 32;

		return 1024 * 512;
	case 0x05:
		if (banks)
			*banks = 64;

		return 1024 * 1024 * 1;
	case 0x06:
		if (banks)
			*banks = 128;

		return 1024 * 1024 * 2;
	case 0x07:
		if (banks)
			*banks = 256;

		return 1024 * 1024 * 4;
	case 0x08:
		if (banks)
			*banks = 512;

		return 1024 * 1024 * 8;
	case 0x52:
		if (banks)
			*banks = 72;

		return 1024 * 16 * 72;
	case 0x53:
		if (banks)
			*banks = 80;

		return 1024 * 16 * 80;
	case 0x54:
		if (banks)
			*banks = 96;

		return 1024 * 16 * 96;
	default:
		return 0;
	}
}

int DeviceBinjgb::cartridgeSramSize(int* banks) const {
	if (banks)
		*banks = 0;

	switch (_sramType) {
	case 0x00:
		return 0;
	case 0x01:
		if (banks)
			*banks = 1;

		return 1024 * 2;
	case 0x02:
		if (banks)
			*banks = 1;

		return 1024 * 8;
	case 0x03:
		if (banks)
			*banks = 4;

		return 1024 * 32;
	case 0x04:
		if (banks)
			*banks = 16;

		return 1024 * 128;
	default:
		return 0;
	}
}

int DeviceBinjgb::cartridgeMbcType(void) const {
	return _cartridgeMbcType;
}

const char* DeviceBinjgb::cartridgeMbcTypeName(void) const {
	switch (_cartridgeMbcType) {
	case 0x00: return "ROM";
	case 0x01: return "MBC1";
	case 0x02: return "MBC1+RAM";
	case 0x03: return "MBC1+RAM+BATTERY";
	case 0x05: return "MBC2";
	case 0x06: return "MBC2+BATTERY";
	case 0x08: return "ROM+RAM";
	case 0x09: return "ROM+RAM+BATTERY";
	case 0x0b: return "MMM01";
	case 0x0c: return "MMM01+RAM";
	case 0x0d: return "MMM01+RAM+BATTERY";
	case 0x0f: return "MBC3+TIMER+BATTERY";
	case 0x10: return "MBC3+TIMER+RAM+BATTERY";
	case 0x11: return "MBC3";
	case 0x12: return "MBC3+RAM";
	case 0x13: return "MBC3+RAM+BATTERY";
	case 0x15: return "MBC4";
	case 0x16: return "MBC4+RAM";
	case 0x17: return "MBC4+RAM+BATTERY";
	case 0x19: return "MBC5";
	case 0x1a: return "MBC5+RAM";
	case 0x1b: return "MBC5+RAM+BATTERY";
	case 0x1c: return "MBC5+RUMBLE";
	case 0x1d: return "MBC5+RUMBLE+RAM";
	case 0x1e: return "MBC5+RUMBLE+RAM+BATTERY";
	case 0xfc: return "POCKET CAMERA";
	case 0xfd: return "Bandai TAMA5";
	case 0xfe: return "HuC3";
	case 0xff: return "HuC1+RAM+BATTERY";
	default:
		return "Unknown";
	}
}

bool DeviceBinjgb::cartridgeHasRtc(void) const {
	return
		_cartridgeMbcType == 0x0f || // MBC3 + TIMER + BATTERY.
		_cartridgeMbcType == 0x10;   // MBC3 + TIMER + RAM + BATTERY.
}

bool DeviceBinjgb::cartridgeHasRumble(void) const {
	return
		_cartridgeMbcType == 0x1c || // MBC5 + RUMBLE.
		_cartridgeMbcType == 0x1e;   // MBC5 + RUMBLE + RAM + BATTERY.
}

Device::DeviceTypes DeviceBinjgb::deviceType(void) const {
	return _deviceType;
}

Device::DeviceTypes DeviceBinjgb::enabledDeviceType(void) const {
	return _enabledDeviceType;
}

bool DeviceBinjgb::isDeviceCgbCompatible(void) const {
	return _enabledDeviceType == DeviceTypes::COLORED || _enabledDeviceType == DeviceTypes::COLORED_EXTENDED;
}

bool DeviceBinjgb::isDeviceExtCompatible(void) const {
	return _enabledDeviceType == DeviceTypes::CLASSIC_EXTENDED || _enabledDeviceType == DeviceTypes::COLORED_EXTENDED || _enabledDeviceType == DeviceTypes::SUPER_EXTENDED;
}

bool DeviceBinjgb::isDeviceSgbCompatible(void) const {
	return _enabledDeviceType == DeviceTypes::SUPER || _enabledDeviceType == DeviceTypes::SUPER_EXTENDED;
}

bool DeviceBinjgb::supportsGettingDuty(void) const {
	return true;
}

bool DeviceBinjgb::supportsVariableSpeed(void) const {
	return true;
}

bool DeviceBinjgb::supportsSgbBorder(void) const {
	return true;
}

bool DeviceBinjgb::supportsBreakpoint(void) const {
	return true;
}

bool DeviceBinjgb::supportsVramDebugging(void) const {
	return true;
}

bool DeviceBinjgb::supportsMutingAudioChannel(void) const {
	return true;
}

Colour DeviceBinjgb::classicPalette(int index) const {
	if (index < 0 || index >= GBBASIC_COUNTOF(_classicPalette))
		return Colour();

	return _classicPalette[index];
}

void DeviceBinjgb::classicPalette(int index, const Colour &col) {
	if (index < 0 || index >= GBBASIC_COUNTOF(_classicPalette))
		return;

	_classicPalette[index] = col;

	setBwPalette(PALETTE_TYPE_BGP,  _classicPalette[0].toRGBA(), _classicPalette[1].toRGBA(), _classicPalette[2].toRGBA(), _classicPalette[3].toRGBA());
	setBwPalette(PALETTE_TYPE_OBP0, _classicPalette[0].toRGBA(), _classicPalette[1].toRGBA(), _classicPalette[2].toRGBA(), _classicPalette[3].toRGBA());
	setBwPalette(PALETTE_TYPE_OBP1, _classicPalette[0].toRGBA(), _classicPalette[1].toRGBA(), _classicPalette[2].toRGBA(), _classicPalette[3].toRGBA());
}

unsigned DeviceBinjgb::speed(void) const {
	return _speed;
}

bool DeviceBinjgb::speed(unsigned s) {
	const bool valid = (s >= DEVICE_BASE_SPEED_FACTOR ?
		(s % DEVICE_BASE_SPEED_FACTOR) == 0 :
		(DEVICE_BASE_SPEED_FACTOR % s) == 0);

	if (!valid) {
		GBBASIC_ASSERT(false && "Wrong data.");

		return false;
	}

	_speed = s;

	return true;
}

unsigned DeviceBinjgb::fps(void) const {
	return _fps;
}

int DeviceBinjgb::getDuty(int* l) const {
	if (l)
		*l = _longPeriodDuty;

	return _duty;
}

long long DeviceBinjgb::timeoutThreshold(void) const {
	return _timeoutThreshold;
}

void DeviceBinjgb::timeoutThreshold(long long val) {
	_timeoutThreshold = val;
}

void* DeviceBinjgb::audioSpecification(void) const {
	return (void*)&_audioSpec;
}

bool DeviceBinjgb::mutedAudioChannel(int ch) const {
	if (ch < 0 || ch >= DEVICE_AUDIO_CHANNEL_COUNT)
		return false;

	if (!_emulator)
		return false;

	return _audioChannelMuted[ch];
}

void DeviceBinjgb::muteAudioChannel(int ch, bool muted) {
	if (ch < 0 || ch >= DEVICE_AUDIO_CHANNEL_COUNT)
		return;

	if (!_emulator)
		return;

	_audioChannelMuted[ch] = muted;

	set_audio_channel_mute(_emulator, ch, muted ? TRUE : FALSE);
}

bool DeviceBinjgb::traceless(void) const {
	return _traceless;
}

bool DeviceBinjgb::inputEnabled(void) const {
	return _inputEnabled;
}

void DeviceBinjgb::inputEnabled(bool val) {
	_inputEnabled = val;
}

void DeviceBinjgb::stroke(int key) {
	if (_keyBuffer.size() >= DEVICE_KEY_BUFFER_LENGTH)
		return;

	_keyBuffer.push_back(key);
}

int DeviceBinjgb::getBreakpointCount(void) const {
	return emulator_get_max_breakpoint_id();
}

Device::Breakpoint DeviceBinjgb::getBreakpoint(int idx) const {
	const ::Breakpoint bp = emulator_get_breakpoint(idx);
	Breakpoint result(bp.bank, bp.addr);
	result.valid = bp.valid;

	return result;
}

Device::Breakpoint DeviceBinjgb::getBreakpointByAddress(UInt8 bank, UInt16 addr) const {
	const int n = getBreakpointCount();
	for (int i = 0; i < n; ++i) {
		const ::Breakpoint bp = emulator_get_breakpoint(i);
		if (bp.bank == bank && bp.addr == addr) {
			Breakpoint result(bp.bank, bp.addr);
			result.valid = bp.valid;

			return result;
		}
	}

	return Breakpoint();
}

int DeviceBinjgb::addBreakpoint(UInt8 bank, UInt16 addr) {
	const int idx = emulator_add_empty_breakpoint();
	emulator_set_breakpoint_address_and_bank(_emulator, idx, addr, bank);
	emulator_enable_breakpoint(idx, TRUE);

	return idx;
}

void DeviceBinjgb::removeBreakpoint(int idx) {
	emulator_remove_breakpoint(idx);
}

void DeviceBinjgb::clearBreakpoints(void) {
	const int n = emulator_get_max_breakpoint_id();
	for (int i = n - 1; i >= 0; --i)
		emulator_remove_breakpoint(i);
}

void DeviceBinjgb::setBreakpointEnabled(int idx, bool enabled) {
	emulator_enable_breakpoint(idx, enabled ? TRUE : FALSE);
}

void DeviceBinjgb::breakAtNextInstruction(void) {
	_breakAtNextInstruction = true;
}

void DeviceBinjgb::calculateBasicBreakpointMask(int stepAddr, ProgramCounterGetter pc, BreakpointEnumerator en) {
	_basicStepBreakpointAddress = (UInt16)stepAddr;

	_basicBreakpointProgramCounterGetter = pc;

	_basicBreakpointMask[0] = 0xffff;
	_basicBreakpointMask[1] = 0xffff;

	if (en) {
		int bank = 0;
		int addr = 0;
		while (en(bank, addr)) {
			_basicBreakpointMask[0] &= ~addr;
			_basicBreakpointMask[1] &= addr;
		}
	}
}

Device::TileSourceTypes DeviceBinjgb::getTileSourceType(void) const {
	if (!_emulator)
		return TileSourceTypes::INVALID;

	const TileDataSelect val = emulator_get_tile_data_select(_emulator);

	return val == TILE_DATA_8800_97FF ?
		TileSourceTypes::FROM_8800_TO_97FF :
		TileSourceTypes::FROM_8000_TO_8FFF;
}

Device::MapSourceTypes DeviceBinjgb::getMapSourceType(LayerTypes layer) const {
	if (!_emulator)
		return MapSourceTypes::INVALID;

	const TileMapSelect val = emulator_get_tile_map_select(_emulator, (LayerType)layer);

	return val == TILE_MAP_9800_9BFF ?
		MapSourceTypes::FROM_9800_TO_9BFF :
		MapSourceTypes::FROM_9C00_TO_9FFF;
}

Device::PaletteGrayscale DeviceBinjgb::getPalette(PaletteTypes type) const {
	if (!_emulator)
		return PaletteGrayscale();

	const Palette val = emulator_get_palette(_emulator, (PaletteType)type);
	Device::PaletteGrayscale result;
	memcpy(result.color, val.color, sizeof(Colors) * DEVICE_PALETTE_COLOR_COUNT);

	return result;
}

Device::PaletteRgba DeviceBinjgb::getPaletteRgba(PaletteTypes type) const {
	if (!_emulator)
		return PaletteRgba();

	const PaletteRGBA val = emulator_get_palette_rgba(_emulator, (PaletteType)type);
	Device::PaletteRgba result;
	memcpy(result.color, val.color, sizeof(UInt32) * DEVICE_PALETTE_COLOR_COUNT);

	return result;
}

Device::PaletteRgba DeviceBinjgb::getCgbPaletteRgba(CgbPaletteTypes type, int index) const {
	if (!_emulator)
		return PaletteRgba();

	const PaletteRGBA val = emulator_get_cgb_palette_rgba(_emulator, (CgbPaletteType)type, index);
	Device::PaletteRgba result;
	memcpy(result.color, val.color, sizeof(UInt32) * DEVICE_PALETTE_COLOR_COUNT);

	return result;
}

Device::PaletteRgba DeviceBinjgb::getSgbPaletteRgba(int index) const {
	if (!_emulator)
		return PaletteRgba();

	const PaletteRGBA val = emulator_get_sgb_palette_rgba(_emulator, index);
	Device::PaletteRgba result;
	memcpy(result.color, val.color, sizeof(UInt32) * DEVICE_PALETTE_COLOR_COUNT);

	return result;
}

void DeviceBinjgb::getTileBuffer(TileBuffer &buf) const {
	if (!_emulator)
		return;

	buf.fill(0);

	TileData val;
	emulator_get_tile_data(_emulator, val);
	memcpy(buf.data(), val, sizeof(UInt8) * buf.size());
}

void DeviceBinjgb::getMapBuffer(MapSourceTypes type, MapBuffer &buf) const {
	if (!_emulator)
		return;

	buf.fill(0);

	TileMap val;
	emulator_get_tile_map(_emulator, type == MapSourceTypes::FROM_9800_TO_9BFF ? TILE_MAP_9800_9BFF : TILE_MAP_9C00_9FFF, val);
	memcpy(buf.data(), val, sizeof(UInt8) * buf.size());
}

void DeviceBinjgb::getMapAttrBuffer(MapSourceTypes type, MapBuffer &buf) const {
	if (!_emulator)
		return;

	buf.fill(0);

	TileMap val;
	emulator_get_tile_map_attr(_emulator, type == MapSourceTypes::FROM_9800_TO_9BFF ? TILE_MAP_9800_9BFF : TILE_MAP_9C00_9FFF, val);
	memcpy(buf.data(), val, sizeof(UInt8) * buf.size());
}

void DeviceBinjgb::getSgbMapAttrBuffer(UInt8 map[90]) const {
	if (!_emulator)
		return;

	u8 val[90];
	emulator_get_sgb_attr_map(_emulator, val);
	memcpy(map, val, sizeof(val));
}

void DeviceBinjgb::getBgScroll(UInt8* x, UInt8* y) const {
	if (!_emulator)
		return;

	u8 x_ = 0, y_ = 0;
	emulator_get_bg_scroll(_emulator, &x_, &y_);
	if (x)
		*x = x_;
	if (y)
		*y = y_;
}

void DeviceBinjgb::getWindowScroll(UInt8* x, UInt8* y) const {
	if (!_emulator)
		return;

	u8 x_ = 0, y_ = 0;
	emulator_get_window_scroll(_emulator, &x_, &y_);
	if (x)
		*x = x_;
	if (y)
		*y = y_;
}

bool DeviceBinjgb::getDisplay(void) const {
	if (!_emulator)
		return false;

	return !!emulator_get_display(_emulator);
}

bool DeviceBinjgb::getBgDisplay(void) const {
	if (!_emulator)
		return false;

	return !!emulator_get_bg_display(_emulator);
}

bool DeviceBinjgb::getWindowDisplay(void) const {
	if (!_emulator)
		return false;

	return !!emulator_get_window_display(_emulator);
}

bool DeviceBinjgb::getObjDisplay(void) const {
	if (!_emulator)
		return false;

	return !!emulator_get_obj_display(_emulator);
}

bool DeviceBinjgb::is8x16Obj(void) const {
	if (!_emulator)
		return false;

	return emulator_get_obj_size(_emulator) == OBJ_SIZE_8X16;
}

Device::Obj DeviceBinjgb::getObj(int index) const {
	if (!_emulator)
		return Obj();

	const ::Obj obj = emulator_get_obj(_emulator, index);
	Obj result;
	result.y = (UInt8)obj.y;
	result.x = (UInt8)obj.x;
	result.tile = (UInt8)obj.tile;
	result.byte3 = (UInt8)obj.byte3;
	result.priority = obj.priority == OBJ_PRIORITY_ABOVE_BG ? ObjPriorities::ABOVE_BG : ObjPriorities::BEHIND_BG;
	result.yFlip = !!obj.yflip;
	result.xFlip = !!obj.xflip;
	result.palette = (UInt8)Math::clamp(obj.palette, (UInt8)0, (UInt8)1);
	result.bank = (UInt8)Math::clamp(obj.bank, (UInt8)0, (UInt8)1);
	result.cgbPalette = (UInt8)Math::clamp(obj.cgb_palette, (UInt8)0, (UInt8)7);

	return result;
}

bool DeviceBinjgb::isObjVisible(const Obj* obj) const {
	if (!obj)
		return false;

	::Obj obj_;
	obj_.y = (u8)obj->y;
	obj_.x = (u8)obj->x;
	obj_.tile = (u8)obj->tile;
	obj_.byte3 = (u8)obj->byte3;
	obj_.priority = obj->priority == ObjPriorities::ABOVE_BG ? OBJ_PRIORITY_ABOVE_BG : OBJ_PRIORITY_BEHIND_BG;
	obj_.yflip = obj->yFlip ? TRUE : FALSE;
	obj_.xflip = obj->xFlip ? TRUE : FALSE;
	obj_.palette = (u8)obj->palette;
	obj_.bank = (u8)obj->bank;
	obj_.cgb_palette = (u8)obj->cgbPalette;

	return !!obj_is_visible(&obj_);
}

bool DeviceBinjgb::open(Bytes::Ptr rom, DeviceTypes deviceType, bool preferSgb, class Input* input, Bytes::Ptr sram, bool isEditor, bool useAudioDevice, bool traceless) {
	// Prepare.
	if (_opened)
		return false;
	_opened = true;

	_traceless = traceless;

	// Initialize the device type.
	_enabledDeviceType = _deviceType = deviceType;

	// Retain the ROM.
	if (DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS < rom->count())
		_cartridgeType = rom->get(DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS);
	if (DEVICE_BINJGB_CARTRIDGE_SUPER_TYPE_ADDRESS < rom->count())
		_cartridgeSuperType = rom->get(DEVICE_BINJGB_CARTRIDGE_SUPER_TYPE_ADDRESS);
	if (DEVICE_BINJGB_CARTRIDGE_MBC_TYPE_ADDRESS < rom->count())
		_cartridgeMbcType = rom->get(DEVICE_BINJGB_CARTRIDGE_MBC_TYPE_ADDRESS);
	if (DEVICE_BINJGB_ROM_SIZE_ADDRESS < rom->count())
		_romType = rom->get(DEVICE_BINJGB_ROM_SIZE_ADDRESS);
	if (DEVICE_BINJGB_SRAM_SIZE_ADDRESS < rom->count())
		_sramType = rom->get(DEVICE_BINJGB_SRAM_SIZE_ADDRESS);
	if (DEVICE_BINJGB_CARTRIDGE_OLD_LICENSE_CODE_ADDRESS < rom->count())
		_cartridgeOldLicenseCode = rom->get(DEVICE_BINJGB_CARTRIDGE_OLD_LICENSE_CODE_ADDRESS);

	if (isCartridgeSgbCompatible() && isDeviceCgbCompatible() && (!isCartridgeCgbCompatible() || preferSgb)) {
		// The device's SGB features are only available if the cartridge prefers it and the user does as well.
		if (_enabledDeviceType == DeviceTypes::COLORED)
			_enabledDeviceType = DeviceTypes::SUPER;
		else /* if (_enabledDeviceType == DeviceTypes::COLORED_EXTENDED) */
			_enabledDeviceType = DeviceTypes::SUPER_EXTENDED;

		// Interpolate the ROM to remove CGB flag.
		if (isCartridgeCgbCompatible()) {
			RomInspector header;
			header.load(rom);
			{
				const int newCartType = _cartridgeType & ~DEVICE_BINJGB_CARTRIDGE_CGB_ONLY_TYPE;
				rom->set(DEVICE_BINJGB_CARTRIDGE_TYPE_ADDRESS, (Byte)newCartType);

				const std::string headerChecksum = header.getCorrectHeaderChecksum();
				header.setHeaderChecksum(headerChecksum);
				const std::string globalChecksum = header.getCorrectGlobalChecksum();
				header.setGlobalChecksum(globalChecksum);
			}
			header.unload();
		}
	}

	if (isDeviceExtCompatible()) { // GBB EXTENSION.
		// The device's extension is only turned on if the cartridge prefers it.
		if (!isCartridgeExtCompatible()) {
			// Otherwise turn it off.
			if (_enabledDeviceType == DeviceTypes::CLASSIC_EXTENDED)
				_enabledDeviceType = DeviceTypes::CLASSIC;
			else if (_enabledDeviceType == DeviceTypes::COLORED_EXTENDED)
				_enabledDeviceType = DeviceTypes::COLORED;
			else if (_enabledDeviceType == DeviceTypes::SUPER_EXTENDED)
				_enabledDeviceType = DeviceTypes::SUPER;
		}
	}

	// Initialize the emulator.
	EmulatorInit init;
	memset(&init, 0, sizeof(EmulatorInit));
	const size_t sz = rom->count();
	u8* data = (u8*)xmalloc(sz);
	memcpy(data, rom->pointer(), rom->count());
	init.rom             = { data, sz };
	init.audio_frequency = DEVICE_BINJGB_AUDIO_SPEC_FREQUENCE;
	init.audio_frames    = DEVICE_BINJGB_AUDIO_SPEC_FRAMES;
	init.random_seed     = 0xcabba6e5;
	init.builtin_palette = 0;
	init.force_dmg       = (_enabledDeviceType == DeviceTypes::CLASSIC || _enabledDeviceType == DeviceTypes::CLASSIC_EXTENDED) ? TRUE : FALSE;
	init.cgb_color_curve = CGB_COLOR_CURVE_GAMBATTE;
	_emulator = emulator_new(&init);
	if (!_emulator) {
		xfree(data);

		return false;
	}

	setBwPalette(PALETTE_TYPE_BGP,  _classicPalette[0].toRGBA(), _classicPalette[1].toRGBA(), _classicPalette[2].toRGBA(), _classicPalette[3].toRGBA());
	setBwPalette(PALETTE_TYPE_OBP0, _classicPalette[0].toRGBA(), _classicPalette[1].toRGBA(), _classicPalette[2].toRGBA(), _classicPalette[3].toRGBA());
	setBwPalette(PALETTE_TYPE_OBP1, _classicPalette[0].toRGBA(), _classicPalette[1].toRGBA(), _classicPalette[2].toRGBA(), _classicPalette[3].toRGBA());

	if (isDeviceExtCompatible()) { // GBB EXTENSION.
		// Initialize the extension RAM to zero.
		for (int i = 0; i < DEVICE_BINJGB_EXTENSION_AREA_SIZE; ++i)
			emulator_write_u8_raw(_emulator, (Address)(DEVICE_BINJGB_EXTENSION_START_ADDRESS + i), 0);

		// Turn on the extension features.
		if (_enabledDeviceType == DeviceTypes::CLASSIC_EXTENDED)
			emulator_write_u8_raw(_emulator, DEVICE_BINJGB_EXTENSION_STATUS_REG, DEVICE_BINJGB_CPU_GB_EXT_TYPE);
		else if (_enabledDeviceType == DeviceTypes::COLORED_EXTENDED)
			emulator_write_u8_raw(_emulator, DEVICE_BINJGB_EXTENSION_STATUS_REG, DEVICE_BINJGB_CPU_CGB_EXT_TYPE);
		else if (_enabledDeviceType == DeviceTypes::SUPER_EXTENDED)
			emulator_write_u8_raw(_emulator, DEVICE_BINJGB_EXTENSION_STATUS_REG, DEVICE_BINJGB_CPU_SGB_EXT_TYPE);

		// Set the platform flags.
		u8 flags = 0;
		if (isEditor)
			flags |= DEVICE_BINJGB_PLATFORM_EDITOR_FLAG;
#if defined GBBASIC_OS_WIN
		flags |= DEVICE_BINJGB_PLATFORM_WINDOWS_FLAG;
#elif defined GBBASIC_OS_MAC
		flags |= DEVICE_BINJGB_PLATFORM_MACOS_FLAG;
#elif defined GBBASIC_OS_LINUX
		flags |= DEVICE_BINJGB_PLATFORM_LINUX_FLAG;
#elif defined GBBASIC_OS_HTML
		flags |= DEVICE_BINJGB_PLATFORM_HTML_FLAG;
#endif /* Platform macro. */
		emulator_write_u8_raw(_emulator, DEVICE_BINJGB_PLATFORM_FLAGS_REG, flags);

		// Set the localization flags.
		const Platform::Languages lang = Platform::locale();
		static_assert((unsigned)Platform::Languages::COUNT <= (unsigned)std::numeric_limits<u8>::max(), "Wrong type.");
		emulator_write_u8_raw(_emulator, DEVICE_BINJGB_LOCALIZATION_FLAGS_REG, (u8)lang);
	}

	// Initialize the RTC.
	if (cartridgeHasRtc()) {
		_rtcTicks = 0;
		int day = 0, hr = 0, mi = 0, sec = 0;
		DateTime::now(&sec, &mi, &hr, nullptr, nullptr, nullptr, nullptr, &day, nullptr);
		ENABLE_RAM(_emulator);
		deviceBinjgbRtcStart(_emulator, 1);
		deviceBinjgbRtcLatch(_emulator);
		deviceBinjgbRtcSet(_emulator, (UInt8)RTC_VALUE_DAY,  (UInt16)day);
		deviceBinjgbRtcSet(_emulator, (UInt8)RTC_VALUE_HOUR, (UInt16)hr);
		deviceBinjgbRtcSet(_emulator, (UInt8)RTC_VALUE_MIN,  (UInt16)mi);
		deviceBinjgbRtcSet(_emulator, (UInt8)RTC_VALUE_SEC,  (UInt16)sec);
		DISABLE_RAM(_emulator);
	}

	// Initialize the streaming buffer.
	_streamingBuffer = nullptr;

	// Initialize the frame buffer.
	memset(_sgbBorderVideoBuffer, 0, sizeof(_sgbBorderVideoBuffer));

	// Initialize the audio.
	memset(&_audioCvt, 0, sizeof(SDL_AudioCVT));

	if (useAudioDevice) {
		const int n = SDL_GetNumAudioDrivers();
		if (!n)
			fprintf(stderr, "No audio device.\n");
	}

	SDL_AudioSpec obtdSpec;
	memset(&obtdSpec, 0, sizeof(SDL_AudioSpec));
	if (useAudioDevice) {
		_audioDeviceId = SDL_OpenAudioDevice(
			nullptr, SDL_FALSE,
			&_audioSpec, &obtdSpec,
			SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE
		);
	}
	if (useAudioDevice && _audioDeviceId == 0)
		fprintf(stderr, "Cannot open the audio hardware or the desired audio output format.\n");

	if (useAudioDevice) {
		SDL_BuildAudioCVT(
			&_audioCvt,
			_audioSpec.format, _audioSpec.channels, _audioSpec.freq,
			obtdSpec.format, obtdSpec.channels, obtdSpec.freq
		);
		if (_audioCvt.needed) {
			fprintf(
				stdout,
				"Need audio conversion.\n"
				"  Source format: %d, channels: %d, frequency: %d\n"
				"  Target format: %d, channels: %d, frequency: %d\n",
				_audioSpec.format, _audioSpec.channels, _audioSpec.freq,
				obtdSpec.format, obtdSpec.channels, obtdSpec.freq
			);
			_audioCvt.len = obtdSpec.size;
			_audioCvt.buf = (Uint8*)SDL_malloc(_audioCvt.len * _audioCvt.len_mult);
			_audioCvtBufferLength = _audioCvt.len * _audioCvt.len_mult;
		}
		_audioBuffer = Bytes::Ptr(Bytes::create());
		_audioBuffer->resize(_audioSpec.size);

		SDL_PauseAudioDevice(_audioDeviceId, SDL_FALSE);
	} else {
		_audioBuffer = Bytes::Ptr(Bytes::create());
		_audioBuffer->resize(_audioSpec.size);
	}

	// Initialize the input.
	_input = input;
	_inputEnabled = !!_input;
	if (_inputEnabled)
		emulator_set_joypad_callback(_emulator, onInput, this);

	_keyboardModifiers = KeyboardModifiers();
	_keyBuffer.clear();

	// Initialize the SRAM.
	if (sram)
		readSram(sram.get());

	// Finish.
	return true;
}

bool DeviceBinjgb::close(Bytes::Ptr sram) {
	// Prepare.
	if (!_opened)
		return false;
	_opened = false;

	// Dispose the SRAM.
	if (sram)
		writeSram(sram.get());

	// Dispose the counters.
	_speed = 1 * DEVICE_BASE_SPEED_FACTOR;
	_previousTicks = 0;
	_updatedFrameCount = 0;
	_updatedSeconds = 0;
	_fps = 59;
	_duty = 0;
	_timeoutThreshold = 0;
	_longPeriodDuty = 0;
	_longPeriodDutyMaxValue = 0;
	_longPeriodDutyTicks = 0;

	// Dispose the input.
	if (_emulator)
		emulator_set_joypad_callback(_emulator, nullptr, nullptr);
	_input = nullptr;
	_inputEnabled = true;
	_keyboardModifiers = KeyboardModifiers();
	_keyBuffer.clear();

	// Dispose the video.
	if (_sgbBorderVideoFadeBuffer) {
		delete [] _sgbBorderVideoFadeBuffer;
		_sgbBorderVideoFadeBuffer = nullptr;
	}

	// Dispose the audio.
	_audioCvtBufferLength = 0;
	_audioBuffer = nullptr;

	if (_audioCvt.buf)
		SDL_free(_audioCvt.buf);
	memset(&_audioCvt, 0, sizeof(SDL_AudioCVT));

	if (_audioDeviceId) {
		SDL_CloseAudioDevice(_audioDeviceId);
		_audioDeviceId = 0;
	}

	// Dispose the frame buffer.
	memset(_sgbBorderVideoBuffer, 0, sizeof(_sgbBorderVideoBuffer));

	// Dispose the streaming buffer.
	_streamingBuffer = nullptr;

	// Dispose the RTC state.
	_rtcTicks = 0;

	// Dispose the emulator.
	if (_emulator) {
		emulator_delete(_emulator);
		_emulator = nullptr;
	}
	_emulatorPaused = false;

	// Release the ROM.
	_cartridgeType = DEVICE_BINJGB_CARTRIDGE_GB_ONLY_TYPE;
	_cartridgeSuperType = DEVICE_BINJGB_CARTRIDGE_WITHOUT_SGB_SUPPORT_TYPE;
	_cartridgeMbcType = 0;
	_romType = 0;
	_sramType = 0;
	_cartridgeOldLicenseCode = 0;

	// Dispose the device type.
	_deviceType = DeviceTypes::COLORED;
	_enabledDeviceType = DeviceTypes::COLORED;

	// Finish.
	return true;
}

bool DeviceBinjgb::update(
	class Window* wnd, class Renderer* rnd,
	double delta,
	class Texture* texture, class Texture* textureForBorderFrame, bool* resetBorderFrame,
	bool allowInput, const KeyboardModifiers* keyMods,
	bool* isNewFrame,
	AudioHandler handleAudio
) {
	// Prepare.
	if (isNewFrame)
		*isNewFrame = false;

	if (!_emulator)
		return false;

	const bool isSgb = !!textureForBorderFrame;
	Texture* oldRndTarget = nullptr;
	int oldRndScale = 0;

	auto beginRendering = [&] (void) -> void {
		oldRndTarget = rnd->target();
		oldRndScale = rnd->scale();
	};
	auto endRendering = [&] (void) -> void {
		rnd->target(oldRndTarget);
		rnd->scale(oldRndScale);
	};

	// Fill the keyboard modifiers.
	if (keyMods)
		_keyboardModifiers = *keyMods;

	// Set the renderer states.
	if (texture)
		beginRendering();

	// Update with paused content.
	if (_emulatorPaused) {
		if (texture) {
			updateVideo(wnd, rnd, delta, texture, textureForBorderFrame, resetBorderFrame, isSgb);

			endRendering();
		}

		return true;
	}

	// Tick.
	bool timeout = false;
	long long start = 0;
	if (_timeoutThreshold > 0)
		start = DateTime::ticks();

	delta = Math::min(delta, DEVICE_BINJGB_DELTA_TIME_MIN_SECONDS);
	const Ticks deltaTicks = (Ticks)(delta * CPU_TICKS_PER_SECOND);
	const Ticks scaleTicks = deltaTicks * _speed / DEVICE_BASE_SPEED_FACTOR;
	const Ticks untilTicks = emulator_get_ticks(_emulator) + scaleTicks;

	EmulatorEvent event = 0;
	bool disabledInput = false;
	if (_inputEnabled && !allowInput) {
		_inputEnabled = false;
		disabledInput = true;
	}
	if (_previousTicks == 0) {
		_previousTicks = emulator_get_ticks(_emulator);
	}
	do {
		// Tick.
		if (_breakAtNextInstruction) {
			event = emulator_step(_emulator); // Tick one instruction.
		} else {
			event = emulator_run_until(_emulator, untilTicks); // Tick for the specific interval.

			if (event & EMULATOR_EVENT_BREAKPOINT) {
				if (!breakpointShouldHit())
					event &= ~EMULATOR_EVENT_BREAKPOINT;
			}
		}

		// Update the video system.
		if (event & EMULATOR_EVENT_NEW_FRAME) {
			const Ticks nowTicks = emulator_get_ticks(_emulator);
			const Ticks diffTicks = nowTicks - _previousTicks;
			_previousTicks = nowTicks;
			const double deltaSecs = (double)diffTicks / CPU_TICKS_PER_SECOND;
			_updatedSeconds += deltaSecs;
			if (++_updatedFrameCount >= (120 * _speed / DEVICE_BASE_SPEED_FACTOR)) { // ~2 seconds.
				if (_updatedSeconds > 0)
					_fps = (unsigned)((_updatedFrameCount * _speed / DEVICE_BASE_SPEED_FACTOR) / _updatedSeconds);
				else
					_fps = 0;
				_updatedFrameCount = 0;
				_updatedSeconds = 0;
			}

			if (texture) {
				updateVideo(wnd, rnd, delta, texture, textureForBorderFrame, resetBorderFrame, isSgb);
			}

			const Ticks ticks = emulator_get_duty_ticks(_emulator);
			constexpr const double dlt = ((double)CPU_TICKS_PER_SECOND / 120);
			_duty = Math::clamp((int)((double)ticks / dlt * 100), 0, 100);
			emulator_set_duty_ticks(_emulator, 0);

			if (_duty > _longPeriodDutyMaxValue)
				_longPeriodDutyMaxValue = _duty;
			if (++_longPeriodDutyTicks >= (int)(30 * _speed / DEVICE_BASE_SPEED_FACTOR)) { // ~0.5 second.
				_longPeriodDuty = Math::clamp(_longPeriodDutyMaxValue, 0, 100);
				_longPeriodDutyMaxValue = 0;
				_longPeriodDutyTicks = 0;
			}

			if (isNewFrame)
				*isNewFrame = true;
		}

		// Update the audio system.
		if (event & EMULATOR_EVENT_AUDIO_BUFFER_FULL) {
			updateAudio(wnd, rnd, delta, handleAudio);
		}

		// Break if timeout.
		if (_timeoutThreshold > 0) {
			const long long now = DateTime::ticks();
			const long long diff = now - start;
			if (diff > _timeoutThreshold) {
				timeout = true;

				break;
			}
		}
	} while (!(event & (EMULATOR_EVENT_UNTIL_TICKS | EMULATOR_EVENT_BREAKPOINT | EMULATOR_EVENT_INVALID_OPCODE)) && !_breakAtNextInstruction);

	if (disabledInput) {
		_inputEnabled = true;
	}

	if (event & EMULATOR_EVENT_BREAKPOINT || _breakAtNextInstruction) {
		if (_debugListener) {
			if (_debugListener->breakpointHit())
				pause();
		}
	}

	if (_breakAtNextInstruction)
		_breakAtNextInstruction = false;

	// Tick the RTC.
	if (cartridgeHasRtc()) {
		_rtcTicks += delta;
		if (_rtcTicks > 0.5) { // Ticks every half second.
			_rtcTicks -= 0.5;
			deviceBinjgbRtcLatch(_emulator);
		}
	}

	// Process the rumble.
	if (cartridgeHasRumble()) {
		// Not supported yet.
	}

	// Process the streaming and shell command area.
	processStreaming(wnd, rnd);
	processShellCommand(wnd, rnd);

	// Restore the renderer states.
	if (texture)
		endRendering();

	// Finish.
	return !timeout;
}

bool DeviceBinjgb::opened(void) const {
	return _opened;
}

bool DeviceBinjgb::paused(void) const {
	return _emulatorPaused;
}

void DeviceBinjgb::pause(void) {
	_emulatorPaused = true;
}

void DeviceBinjgb::resume(void) {
	_emulatorPaused = false;
}

Device::Registers DeviceBinjgb::readRegisters(void) const {
	const ::Registers regs = emulator_get_registers(_emulator);
	Registers result;
	result.A = regs.A;
	result.F.Z = regs.F.Z ? 1 : 0;
	result.F.N = regs.F.N ? 1 : 0;
	result.F.H = regs.F.H ? 1 : 0;
	result.F.C = regs.F.C ? 1 : 0;
	result.BC = regs.BC;
	result.DE = regs.DE;
	result.HL = regs.HL;
	result.SP = regs.SP;
	result.PC = regs.PC;

	return result;
}

void DeviceBinjgb::writeRegisters(const Registers &regs) {
	::Registers regs_;
	regs_.A = regs.A;
	regs_.F.Z = regs.F.Z ? TRUE : FALSE;
	regs_.F.N = regs.F.N ? TRUE : FALSE;
	regs_.F.H = regs.F.H ? TRUE : FALSE;
	regs_.F.C = regs.F.C ? TRUE : FALSE;
	regs_.BC = regs.BC;
	regs_.DE = regs.DE;
	regs_.HL = regs.HL;
	regs_.SP = regs.SP;
	regs_.PC = regs.PC;

	emulator_set_registers(_emulator, &regs_);
}

bool DeviceBinjgb::readRam(UInt16 address, UInt8* data) const {
	if (!_emulator)
		return false;

	const u8 ret = emulator_read_u8_raw(_emulator, address);
	*data = ret;

	return true;
}

bool DeviceBinjgb::readRam(UInt16 address, UInt16* data) const {
	if (!_emulator)
		return false;

	union {
		UInt16 data;
		UInt8 bytes[2];
	} u;
	u.bytes[0] = emulator_read_u8_raw(_emulator, address);
	u.bytes[1] = emulator_read_u8_raw(_emulator, address + 1);
	if (!Platform::isLittleEndian())
		std::swap(u.bytes[0], u.bytes[1]);
	*data = u.data;

	return true;
}

bool DeviceBinjgb::readRam(UInt16 address, Int8* data) const {
	if (!_emulator)
		return false;

	union {
		UInt8 o;
		Int8 r;
	} u;
	u.o = emulator_read_u8_raw(_emulator, address);

	*data = u.r;

	return true;
}

bool DeviceBinjgb::readRam(UInt16 address, Int16* data) const {
	if (!_emulator)
		return false;

	union {
		Int16 data;
		UInt8 bytes[2];
	} u;
	u.bytes[0] = emulator_read_u8_raw(_emulator, address);
	u.bytes[1] = emulator_read_u8_raw(_emulator, address + 1);
	if (!Platform::isLittleEndian())
		std::swap(u.bytes[0], u.bytes[1]);
	*data = u.data;

	return true;
}

size_t DeviceBinjgb::readRam(UInt16 address, Byte* data, size_t len) const {
	size_t result = 0;
	if (!_emulator)
		return result;

	for (size_t i = 0; i < len; ++i) {
		const u8 ret = emulator_read_u8_raw(_emulator, (Address)(address + i));
		data[i] = ret;
		++result;
	}

	return result;
}

bool DeviceBinjgb::writeRam(UInt16 address, UInt8 data) {
	if (!_emulator)
		return false;

	emulator_write_u8_raw(_emulator, address, data);

	return true;
}

bool DeviceBinjgb::writeRam(UInt16 address, UInt16 data) {
	if (!_emulator)
		return false;

	union {
		UInt16 data;
		UInt8 bytes[2];
	} u;
	u.data = data;
	if (!Platform::isLittleEndian())
		std::swap(u.bytes[0], u.bytes[1]);
	emulator_write_u8_raw(_emulator, address,     u.bytes[0]);
	emulator_write_u8_raw(_emulator, address + 1, u.bytes[1]);

	return true;
}

bool DeviceBinjgb::writeRam(UInt16 address, Int8 data) {
	if (!_emulator)
		return false;

	union {
		Int8 i;
		UInt8 r;
	} u;
	u.i = data;
	emulator_write_u8_raw(_emulator, address, u.r);

	return true;
}

bool DeviceBinjgb::writeRam(UInt16 address, Int16 data) {
	if (!_emulator)
		return false;

	union {
		Int16 data;
		UInt8 bytes[2];
	} u;
	u.data = data;
	if (!Platform::isLittleEndian())
		std::swap(u.bytes[0], u.bytes[1]);
	emulator_write_u8_raw(_emulator, address,     u.bytes[0]);
	emulator_write_u8_raw(_emulator, address + 1, u.bytes[1]);

	return true;
}

size_t DeviceBinjgb::writeRam(UInt16 address, const Byte* data, size_t len) {
	size_t result = 0;
	if (!_emulator)
		return result;

	for (size_t i = 0; i < len; ++i) {
		emulator_write_u8_raw(_emulator, (Address)(address + i), data[i]);
		++result;
	}

	return result;
}

bool DeviceBinjgb::readSram(const Bytes* bytes) const {
	if (!bytes)
		return false;
	if (bytes->empty())
		return false;

	Bytes::Ptr tmp(Bytes::create());
	tmp->writeBytes(bytes);
	const FileData fd = { (u8*)tmp->pointer(), tmp->count() };
	const Result ret = emulator_read_ext_ram(_emulator, &fd);

	return ret == OK;
}

bool DeviceBinjgb::writeSram(Bytes* bytes) {
	if (!bytes)
		return false;

	bytes->resize(DEVICE_BINJGB_SRAM_MAX_SIZE);
	FileData fd = { bytes->pointer(), bytes->count() };
	const Result ret = emulator_write_ext_ram(_emulator, &fd);
	const int size = cartridgeSramSize(nullptr);
	bytes->resize(size);

	return ret == OK;
}

int DeviceBinjgb::currentBank(void) const {
	if (!_emulator)
		return 0;

	return emulator_get_current_rom_bank(_emulator);
}

void DeviceBinjgb::setBwPalette(PaletteType type, u32 white, u32 light_gray, u32 dark_gray, u32 black) {
	if (!_emulator)
		return;

	GBBASIC_ASSERT(type < PALETTE_TYPE_COUNT);
	PaletteRGBA palette = { { white, light_gray, dark_gray, black } };
	emulator_set_bw_palette(_emulator, type, &palette);
}

void DeviceBinjgb::updateVideo(
	class Window*, class Renderer* rnd,
	double delta,
	class Texture* texture, class Texture* textureForBorderFrame, bool* resetBorderFrame,
	bool isSgb
) {
	constexpr const int BYTES = SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ABGR8888);
	if (isSgb) {
		auto blend = [this, BYTES] (SDL_Texture* tex, double alpha) -> void {
			if (!_sgbBorderVideoFadeBuffer)
				_sgbBorderVideoFadeBuffer = new Colour [SGB_SCREEN_WIDTH * SGB_SCREEN_HEIGHT];

			for (int j = 0; j < SGB_SCREEN_HEIGHT; ++j) {
				for (int i = 0; i < SGB_SCREEN_WIDTH; ++i) {
					const Colour* pixels = (Colour*)_sgbBorderVideoBuffer;
					const Colour &srccol = pixels[i + j * SGB_SCREEN_WIDTH];
					Colour &dstcol = _sgbBorderVideoFadeBuffer[i + j * SGB_SCREEN_WIDTH];
					dstcol.r = (UInt8)(srccol.r * alpha);
					dstcol.g = (UInt8)(srccol.g * alpha);
					dstcol.b = (UInt8)(srccol.b * alpha);
					dstcol.a = srccol.a;
				}
			}
			SDL_UpdateTexture(tex, nullptr, (void*)_sgbBorderVideoFadeBuffer, SGB_SCREEN_WIDTH * BYTES);
		};

		rnd->target(textureForBorderFrame);
		rnd->scale(1);
		SDL_Texture* tex = (SDL_Texture*)textureForBorderFrame->pointer(rnd);
		const SgbFrameBuffer* sframe = emulator_get_sgb_frame_buffer(_emulator);
		switch (_sgbBorderFade) {
		case SgbFadeOperations::NONE:
			if (memcmp(_sgbBorderVideoBuffer, sframe, sizeof(_sgbBorderVideoBuffer)) != 0) {
				_sgbBorderFade = SgbFadeOperations::FADEOUT;
			} else if (resetBorderFrame && *resetBorderFrame) {
				_sgbBorderFade = SgbFadeOperations::NONE;
				_sgbBorderFadeTicks = 0;
				blend(tex, 1);
				*resetBorderFrame = false;
			}

			break;
		case SgbFadeOperations::FADEOUT:
			_sgbBorderFadeTicks += delta;
			if (_sgbBorderFadeTicks < DEVICE_BINJGB_FADE_SECONDS) {
				const double alpha = 1.0 - Math::clamp(_sgbBorderFadeTicks / DEVICE_BINJGB_FADE_SECONDS, 0.0, 1.0);
				blend(tex, alpha);
			} else {
				_sgbBorderFade = SgbFadeOperations::FADEIN;
				_sgbBorderFadeTicks = 0;
				blend(tex, 0);
				memcpy(_sgbBorderVideoBuffer, sframe, sizeof(_sgbBorderVideoBuffer));
				SDL_UpdateTexture(tex, nullptr, (void*)_sgbBorderVideoBuffer, SGB_SCREEN_WIDTH * BYTES);
			}

			break;
		case SgbFadeOperations::FADEIN:
			_sgbBorderFadeTicks += delta;
			if (_sgbBorderFadeTicks < DEVICE_BINJGB_FADE_SECONDS) {
				const double alpha = Math::clamp(_sgbBorderFadeTicks / DEVICE_BINJGB_FADE_SECONDS, 0.0, 1.0);
				blend(tex, alpha);
			} else {
				_sgbBorderFade = SgbFadeOperations::NONE;
				_sgbBorderFadeTicks = 0;
				blend(tex, 1);
				if (_sgbBorderVideoFadeBuffer) {
					delete [] _sgbBorderVideoFadeBuffer;
					_sgbBorderVideoFadeBuffer = nullptr;
				}
			}

			break;
		case SgbFadeOperations::INIT:
			if (memcmp(_sgbBorderVideoBuffer, sframe, sizeof(_sgbBorderVideoBuffer)) != 0) {
				_sgbBorderFade = SgbFadeOperations::FADEIN;
				memcpy(_sgbBorderVideoBuffer, sframe, sizeof(_sgbBorderVideoBuffer));
			}

			break;
		}
	}
	rnd->target(texture);
	rnd->scale(1);
	const FrameBuffer* frame = emulator_get_frame_buffer(_emulator);
	SDL_UpdateTexture((SDL_Texture*)texture->pointer(rnd), nullptr, (void*)frame, SCREEN_WIDTH * BYTES);
}

void DeviceBinjgb::updateAudio(
	class Window*, class Renderer*,
	double /* delta */,
	AudioHandler handleAudio
) {
	typedef std::array<float, SOUND_OUTPUT_COUNT> AudioValues;

	AudioBuffer* srcBuffer = emulator_get_audio_buffer(_emulator);
	const u32 srcFrames = audio_buffer_get_frames(srcBuffer);
	const u8* srcData = srcBuffer->data;
	const int sampleSize = SDL_AUDIO_BITSIZE(_audioSpec.format) / 8;
	const u32 maxDstFrames = _audioSpec.size / (_audioSpec.channels * sampleSize);
	const u32 frames = Math::min(srcFrames, maxDstFrames);

	_audioBuffer->poke(0);
	if (_speed == DEVICE_BASE_SPEED_FACTOR * 1) {
		for (u32 i = 0; i < frames; ++i) {
			AudioValues values;
			for (int j = 0; j < (int)values.size(); ++j) {
				const float val = DEVICE_BINJGB_AUDIO_CONVERT_FROM_U8(*srcData, 1.0f);
				values[j] = val;
				++srcData;
			}
			for (int c = 0; c < _audioSpec.channels; ++c) {
				const int u = c % (int)values.size();
				const float val = values[u];
				_audioBuffer->writeSingle(val);
			}
		}
	} else if (_speed >= DEVICE_BASE_SPEED_FACTOR * 1) {
		for (u32 i = 0; i < frames; i += _speed / DEVICE_BASE_SPEED_FACTOR) {
			AudioValues values;
			for (int j = 0; j < (int)values.size(); ++j) {
				const float val = DEVICE_BINJGB_AUDIO_CONVERT_FROM_U8(*srcData, 1.0f);
				values[j] = val;
				srcData += _speed / DEVICE_BASE_SPEED_FACTOR;
			}
			for (int c = 0; c < _audioSpec.channels; ++c) {
				const int u = c % (int)values.size();
				const float val = values[u];
				_audioBuffer->writeSingle(val);
			}
		}
	} else /* if (_speed < DEVICE_BASE_SPEED_FACTOR * 1) */ {
		for (u32 i = 0; i < frames; ++i) {
			AudioValues values;
			for (int j = 0; j < (int)values.size(); ++j) {
				const float val = DEVICE_BINJGB_AUDIO_CONVERT_FROM_U8(*srcData, 1.0f);
				values[j] = val;
				++srcData;
			}
			for (int j = 0; j < (int)(DEVICE_BASE_SPEED_FACTOR / _speed); ++j) {
				for (int c = 0; c < _audioSpec.channels; ++c) {
					const int u = c % (int)values.size();
					const float val = values[u];
					_audioBuffer->writeSingle(val);
				}
			}
		}
	}
	const Uint32 len = Math::min(
		(frames * DEVICE_BASE_SPEED_FACTOR / _speed) * _audioSpec.channels * sampleSize,
		(u32)_audioBuffer->count()
	);

	bool handledAudio = false;
	if (handleAudio) {
		handledAudio = handleAudio((void*)&_audioSpec, _audioBuffer.get(), len);
	}
	if (!handledAudio && _audioDeviceId) {
		const Uint32 inq = SDL_GetQueuedAudioSize(_audioDeviceId);
		if (inq < (len << 4)) { // Skip the audio frame if the queue is too long.
			if (_audioCvt.needed) {
				const size_t newLen = len * _audioCvt.len_mult;
				if (_audioCvtBufferLength < newLen) {
					_audioCvtBufferLength = newLen;
					_audioCvt.buf = (Uint8*)SDL_realloc(_audioCvt.buf, newLen);
				}
				_audioCvt.len = (int)len;
				SDL_memcpy(_audioCvt.buf, _audioBuffer->pointer(), len);
				SDL_ConvertAudio(&_audioCvt);
				SDL_QueueAudio(_audioDeviceId, _audioCvt.buf, _audioCvt.len_cvt);
			} else {
				SDL_QueueAudio(_audioDeviceId, _audioBuffer->pointer(), len);
			}
		}
	}
}

bool DeviceBinjgb::breakpointShouldHit(void) const {
	if (_basicStepBreakpointAddress == 0)
		return true;

	const Registers regs = readRegisters();
	if (regs.PC != _basicStepBreakpointAddress) // Is not a BASIC breakpoint, hits.
		return true;

	int bank = 0;
	int pc = 0;
	if (!_basicBreakpointProgramCounterGetter(regs, bank, pc))
		return false;

	const bool ret =
		(pc & _basicBreakpointMask[0]) == 0 &&
		(pc & _basicBreakpointMask[1]) == _basicBreakpointMask[1];

	return ret;
}

bool DeviceBinjgb::processStreaming(class Window* wnd, class Renderer* rnd) {
	// Extension feature: stream transferring.
	if (!isDeviceExtCompatible()) // GBB EXTENSION.
		return false;

	const u8 statusByte = emulator_read_u8_raw(_emulator, DEVICE_BINJGB_STREAMING_STATUS_REG);
	if (statusByte == DEVICE_BINJGB_STREAMING_STATUS_READY || statusByte == DEVICE_BINJGB_STREAMING_STATUS_BUSY)
		return false;

	if (statusByte != DEVICE_BINJGB_STREAMING_STATUS_EOS) {
		if (!_streamingBuffer)
			_streamingBuffer = Bytes::Ptr(Bytes::create());

		const u8 byte = emulator_read_u8_raw(_emulator, DEVICE_BINJGB_STREAMING_ADDRESS);
		_streamingBuffer->writeByte(byte);
	}

	emulator_write_u8_raw(_emulator, DEVICE_BINJGB_STREAMING_ADDRESS, 0);

	emulator_write_u8_raw(_emulator, DEVICE_BINJGB_STREAMING_STATUS_REG, DEVICE_BINJGB_STREAMING_STATUS_READY);

	if (statusByte == DEVICE_BINJGB_STREAMING_STATUS_EOS) {
		if (_debugListener)
			_debugListener->streamed(wnd, rnd, _streamingBuffer);

		if (_streamingBuffer)
			_streamingBuffer = nullptr;

		fprintf(stdout, "Got stream data, bytes: %d.\n", _streamingBuffer ? (int)_streamingBuffer->count() : 0);
	}

	return true;
}

bool DeviceBinjgb::processShellCommand(class Window* wnd, class Renderer* rnd) {
	// Extension feature: shell command.
	if (!isDeviceExtCompatible()) // GBB EXTENSION.
		return false;

	const u8 statusByte = emulator_read_u8_raw(_emulator, DEVICE_BINJGB_TRANSFER_STATUS_REG);
	if (statusByte == DEVICE_BINJGB_TRANSFER_STATUS_READY || statusByte == DEVICE_BINJGB_TRANSFER_STATUS_BUSY)
		return false;

	std::string cmd;
	for (int i = 0; i < DEVICE_BINJGB_TRANSFER_MAX_SIZE; ++i) {
		const Address address = (Address)(DEVICE_BINJGB_TRANSFER_ADDRESS + i);
		const u8 byte = emulator_read_u8_raw(_emulator, address);
		emulator_write_u8_raw(_emulator, address, 0);
		cmd.push_back(byte);
	}

	emulator_write_u8_raw(_emulator, DEVICE_BINJGB_TRANSFER_STATUS_REG, DEVICE_BINJGB_TRANSFER_STATUS_READY);

	const bool isSurf =
		Text::startsWith(cmd, "http://", false) ||
		Text::startsWith(cmd, "https://", false);
	const bool isBrowse =
		Text::startsWith(cmd, "file://", false);
	const bool isDummy =
		Text::startsWith(cmd, "$", false);
	const bool isSync =
		Text::startsWith(cmd, "@", false);
	const bool isDebug =
		Text::startsWith(cmd, ">", false);
	const bool isCursor =
		Text::startsWith(cmd, "^", false);
	const bool isPause =
		Text::startsWith(cmd, "||", false);
	const bool isStop =
		Text::startsWith(cmd, "[]", false);

	if (isSurf) {
		const std::string osstr = Unicode::toOs(cmd);

		Platform::surf(osstr.c_str());

		const std::string &cmd_ = osstr;
		fprintf(stdout, "Executed shell command (surf): \"%s\".\n", cmd_.c_str());
	} else if (isBrowse) {
		if (cmd.length() >= 7) { // "file://".
			const std::string path = cmd.substr(7);
			if (!path.empty()) {
				const std::string osstr = Unicode::toOs(path);

				Platform::browse(osstr.c_str());
			}
		}

		const std::string cmd_ = Unicode::toOs(cmd);
		fprintf(stdout, "Executed shell command (browse): \"%s\".\n", cmd_.c_str());
	} else if (isDummy) {
		// Note: this type of command does nothing here, however
		// you could make your own handler by modifying the code.

		const std::string cmd_ = Unicode::toOs(cmd);
		fprintf(stdout, "Executed shell command (dummy): \"%s\".\n", cmd_.c_str());
	} else if (isSync) {
		if (_debugListener)
			_debugListener->sync(wnd, rnd, cmd.c_str() + 1);

		const std::string cmd_ = Unicode::toOs(cmd);
		fprintf(stdout, "Executed shell command (sync): \"%s\".\n", cmd_.c_str());
	} else if (isDebug) {
		if (_debugListener)
			_debugListener->debug(cmd.c_str() + 1);

		const std::string cmd_ = Unicode::toOs(cmd);
		fprintf(stdout, "Executed shell command (debug): \"%s\".\n", cmd_.c_str());
	} else if (isCursor) {
		if (cmd.length() >= 1) { // "^".
			const std::string mode = cmd.substr(1);
			CursorTypes y = CursorTypes::POINTER;
			if (!mode.empty()) {
				std::string mode_ = mode.c_str();
				Text::toLowerCase(mode_);
				if (mode_ == "none")
					y = CursorTypes::NONE;
				else if (mode_ == "pointer")
					y = CursorTypes::POINTER;
				else if (mode_ == "hand")
					y = CursorTypes::HAND;
				else if (mode_ == "busy")
					y = CursorTypes::BUSY;
				else
					y = CursorTypes::POINTER;
			}
			if (_debugListener)
				_debugListener->cursor(y);
		}
	} else if (isPause) {
		if (_debugListener)
			_debugListener->pause(wnd, rnd);

		const std::string cmd_ = Unicode::toOs(cmd);
		fprintf(stdout, "Executed shell command (pause): \"%s\".\n", cmd_.c_str());
	} else if (isStop) {
		if (_debugListener)
			_debugListener->stop(wnd, rnd);

		const std::string cmd_ = Unicode::toOs(cmd);
		fprintf(stdout, "Executed shell command (stop): \"%s\".\n", cmd_.c_str());
	} else {
		const std::string osstr = Unicode::toOs(cmd);

		Platform::execute(osstr.c_str());

		const std::string &cmd_ = osstr;
		fprintf(stdout, "Executed shell command: \"%s\".\n", cmd_.c_str());
	}

	return true;
}

void DeviceBinjgb::onInput(JoypadButtons* joyp, void* data) {
	// Prepare.
	static_assert(INPUT_GAMEPAD_COUNT == 2, "Wrong data.");

	DeviceBinjgb* self = (DeviceBinjgb*)data;
	Input* input = (Input*)self->_input;

	if (!self->_inputEnabled) {
		joyp->up     = FALSE;
		joyp->down   = FALSE;
		joyp->left   = FALSE;
		joyp->right  = FALSE;
		joyp->select = FALSE;
		joyp->start  = FALSE;
		joyp->A      = FALSE;
		joyp->B      = FALSE;

		emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_X_REG,          (u8)0xff);
		emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_Y_REG,          (u8)0xff);
		emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_PRESSED_REG,    (u8)0x00);
		emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_KEY_MODIFIERS_REG,    (u8)0x00);
		emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_KEYBOARD_PRESSED_REG, (u8)0x00);

		return;
	}

	// Update the joypad.
	joyp->up = (Bool)(
		input->buttonDown((int)Input::Buttons::UP,     0) ||
		input->buttonDown((int)Input::Buttons::UP,     1)
	);
	joyp->down = (Bool)(
		input->buttonDown((int)Input::Buttons::DOWN,   0) ||
		input->buttonDown((int)Input::Buttons::DOWN,   1)
	);
	joyp->left = (Bool)(
		input->buttonDown((int)Input::Buttons::LEFT,   0) ||
		input->buttonDown((int)Input::Buttons::LEFT,   1)
	);
	joyp->right = (Bool)(
		input->buttonDown((int)Input::Buttons::RIGHT,  0) ||
		input->buttonDown((int)Input::Buttons::RIGHT,  1)
	);
	joyp->select = (Bool)(
		input->buttonDown((int)Input::Buttons::SELECT, 0) ||
		input->buttonDown((int)Input::Buttons::SELECT, 1)
	);
	joyp->start = (Bool)(
		input->buttonDown((int)Input::Buttons::START,  0) ||
		input->buttonDown((int)Input::Buttons::START,  1)
	);
	joyp->A = (Bool)(
		input->buttonDown((int)Input::Buttons::A,      0) ||
		input->buttonDown((int)Input::Buttons::A,      1)
	);
	joyp->B = (Bool)(
		input->buttonDown((int)Input::Buttons::B,      0) ||
		input->buttonDown((int)Input::Buttons::B,      1)
	);

	// Update the touch states.
	if (self->isDeviceExtCompatible()) { // GBB EXTENSION.
		// Extension feature: touch input.
		int x = 0;
		int y = 0;
		bool p0 = false;
		bool p1 = false;
		if (input->touch(0, &x, &y, &p0, &p1, nullptr, nullptr, nullptr)) {
			UInt8 prs = 0x00;
			if (p0) prs |= DEVICE_BINJGB_MOUSE_BUTTON_0;
			if (p1) prs |= DEVICE_BINJGB_MOUSE_BUTTON_1;
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_X_REG,       (u8)x);
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_Y_REG,       (u8)y);
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_PRESSED_REG, (u8)prs);
		} else {
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_X_REG,       (u8)0xff);
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_Y_REG,       (u8)0xff);
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_TOUCH_PRESSED_REG, (u8)0x00);
		}
	}

	// Update the keyboard states.
	UInt8 mods = 0x00;
	if (self->_keyboardModifiers.ctrl)
		mods |= 0x01;
	if (self->_keyboardModifiers.shift)
		mods |= 0x02;
	if (self->_keyboardModifiers.alt)
		mods |= 0x04;
	if (self->_keyboardModifiers.super)
		mods |= 0x08;
	emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_KEY_MODIFIERS_REG, (u8)mods);
	if (!self->_keyBuffer.empty() && emulator_read_u8_raw(self->_emulator, DEVICE_BINJGB_KEYBOARD_PRESSED_REG) == 0x00) {
		const int key = self->_keyBuffer.front(); // FIFO.
		self->_keyBuffer.pop_front();
		if (key)
			emulator_write_u8_raw(self->_emulator, DEVICE_BINJGB_KEYBOARD_PRESSED_REG, (u8)key);
	}
}

/* ===========================================================================} */
