/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ROM_INSPECTOR_H__
#define __ROM_INSPECTOR_H__

#include "../gbbasic.h"
#include "bytes.h"

/*
** {===========================================================================
** ROM Inspector
*/

class RomInspector {
private:
	Bytes::Ptr _bytes = nullptr;

public:
	RomInspector();

	void load(Bytes::Ptr bytes);
	void unload(void);

	std::string getRomEntryPoint(void) const;
	bool setRomEntryPoint(const std::string &romEntryPoint);

	std::string getLogo(void) const;
	bool setLogo(const std::string &logo);

	std::string getTitle(bool includeManufactureCode = false, bool includeColorSupport = false) const;
	bool setTitle(const std::string &gameTitle, bool includeManufactureCode = false, bool includeColorSupport = false);

	std::string getManufacturerCode(void) const;
	bool setManufacturerCode(const std::string &manufacturerCode);

	std::string getCartridgeCompatibilityCode(void) const;
	bool setCartridgeCompatibilityCode(const std::string &newCompatibilityCode);
	// DOC: CARTRIDGE SCHEMA.
	// This is only for the color device:
	//   0x80 - Game supports CGB functions, but works on DMG also
	//   0xc0 - Game works on CGB only (physically the same as 0x80)
	bool hasClassicSupport(void) const;
	// DOC: CARTRIDGE SCHEMA.
	// This is only for the color device:
	//   0x80 - Game supports CGB functions, but works on DMG also
	//   0xc0 - Game works on CGB only (physically the same as 0x80)
	bool hasColoredSupport(void) const;
	bool setClassicSupportOnly(bool ext);
	// This is only for the color device.
	bool setColoredSupportOnly(bool ext);
	// This is only for the color device.
	bool setClassicAndColoredSupport(bool ext);

	std::string getNewLicenseCode(void) const;
	bool setNewLicenseCode(const std::string &newLicenseCode);

	// DOC: CARTRIDGE SCHEMA.
	// 0x00 for no SGB functions (normal DMG or CGB only game).
	// 0x03 for SGB functions.
	bool hasSuperSupport(void) const;
	bool setSuperSupport(bool hasSGBSupport);

	std::string getCartridgeTypeCode(void) const;
	bool setCartridgeTypeCode(const std::string &cartridgeTypeCode);

	std::string getRomSizeCode(void) const;
	bool setRomSizeCode(const std::string &romSizeCode);

	std::string getRamSizeCode(void) const;
	bool setRamSizeCode(const std::string &ramSizeCode);

	std::string getDestinationCode(void) const;
	bool setDestinationCode(const std::string &destinationCode);

	std::string getOldLicenseCode(void) const;
	bool setOldLicenseCode(const std::string &oldLicenseCode);

	std::string getVersionNumber(void) const;
	bool setVersionNumber(const std::string &versionNumber);

	std::string getHeaderChecksum(void) const;
	bool setHeaderChecksum(const std::string &headerChecksum);

	std::string getGlobalChecksum(void) const;
	bool setGlobalChecksum(const std::string &globalChecksum);

	// X = 0
	// FOR I = 0x0134 TO 0x014c
	//   X = X - ROM[I] - 1
	// NEXT
	std::string getCorrectHeaderChecksum(void) const;
	std::string getCorrectGlobalChecksum(void) const;

private:
	std::string readHexString(int startPoint, int length) const;
	bool writeBytes(int startPoint, Bytes::Ptr byteArray);

	static std::string padHex(const std::string &txt, int width, char fill = ' ');
	static Bytes::Ptr stringToBytes(const std::string &hex);
	static std::string convertAsciiToHex(const std::string &asciiString);
	static std::string convertHexToAscii(const std::string &hexString);
};

/* ===========================================================================} */

#endif /* __ROM_INSPECTOR_H__ */
