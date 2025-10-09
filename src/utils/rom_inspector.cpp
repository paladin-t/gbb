/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "rom_inspector.h"
#include "text.h"

/*
** {===========================================================================
** ROM Inspector
*/

RomInspector::RomInspector() {
}

void RomInspector::load(Bytes::Ptr bytes) {
	_bytes = bytes;
}

void RomInspector::unload(void) {
	_bytes = nullptr;
}

std::string RomInspector::getRomEntryPoint(void) const {
	return readHexString(0x100, 0x04);
}

bool RomInspector::setRomEntryPoint(const std::string &romEntryPoint) {
	return writeBytes(0x100, stringToBytes(padHex(romEntryPoint, 0x08, '0')));
}

std::string RomInspector::getLogo(void) const {
	return readHexString(0x104, 0x2f);
}

bool RomInspector::setLogo(const std::string &logo) {
	return writeBytes(0x104, stringToBytes(padHex(logo, 0x5e, '0')));
}

std::string RomInspector::getTitle(bool includeManufactureCode, bool includeColorSupport) const {
	if (!includeManufactureCode)
		return convertHexToAscii(readHexString(0x134, 0x0b));
	if (!includeColorSupport)
		return convertHexToAscii(readHexString(0x134, 0x0f));

	return convertHexToAscii(readHexString(0x134, 0x10));
}

bool RomInspector::setTitle(const std::string &gameTitle, bool includeManufactureCode, bool includeColorSupport) {
	if (!includeManufactureCode) {
		const std::string hexValue = convertAsciiToHex(Text::limitRight(Text::padRight(gameTitle, 0x0b), 0x0b));

		return writeBytes(0x134, stringToBytes(hexValue));
	}
	if (!includeColorSupport) {
		const std::string hexValue = convertAsciiToHex(Text::limitRight(Text::padRight(gameTitle, 0x0f), 0x0f));

		return writeBytes(0x134, stringToBytes(hexValue));
	}

	const std::string hexValue = convertAsciiToHex(Text::limitRight(Text::padRight(gameTitle, 0x10), 0x10));

	return writeBytes(0x134, stringToBytes(hexValue));
}

std::string RomInspector::getManufacturerCode(void) const {
	return convertHexToAscii(readHexString(0x13f, 0x04));
}

bool RomInspector::setManufacturerCode(const std::string &manufacturerCode) {
	const std::string hexValue = convertAsciiToHex(Text::limitRight(Text::padRight(manufacturerCode, 0x04), 0x04));

	return writeBytes(0x13f, stringToBytes(hexValue));
}

std::string RomInspector::getCartridgeCompatibilityCode(void) const {
	const std::string hexString = readHexString(0x143, 0x01);

	return hexString;
}

bool RomInspector::setCartridgeCompatibilityCode(const std::string &newCompatibilityCode) {
	return writeBytes(0x143, stringToBytes(newCompatibilityCode));
}

bool RomInspector::hasClassicSupport(void) const {
	const std::string hexString = readHexString(0x143, 0x01);

	return
		hexString == "00" || hexString == "80" ||
		hexString == "20" || hexString == "A0";
}

bool RomInspector::hasColoredSupport(void) const {
	const std::string hexString = readHexString(0x143, 0x01);

	return
		hexString == "80" || hexString == "C0" ||
		hexString == "A0" || hexString == "E0";
}

bool RomInspector::setClassicSupportOnly(bool ext) {
	if (ext)
		return writeBytes(0x143, stringToBytes("20"));

	return writeBytes(0x143, stringToBytes("00"));
}

bool RomInspector::setColoredSupportOnly(bool ext) {
	if (ext)
		return writeBytes(0x143, stringToBytes("E0"));

	return writeBytes(0x143, stringToBytes("C0"));
}

bool RomInspector::setClassicAndColoredSupport(bool ext) {
	if (ext)
		return writeBytes(0x143, stringToBytes("A0"));

	return writeBytes(0x143, stringToBytes("80"));
}

std::string RomInspector::getNewLicenseCode(void) const {
	return convertHexToAscii(readHexString(0x144, 0x02));
}

bool RomInspector::setNewLicenseCode(const std::string &newLicenseCode) {
	std::string hexValue = convertAsciiToHex(Text::limitRight(Text::padRight(newLicenseCode, 0x02), 0x02));

	return writeBytes(0x144, stringToBytes(hexValue));
}

bool RomInspector::hasSuperSupport(void) const {
	const std::string hexString = readHexString(0x146, 0x01);

	return hexString == "03";
}

bool RomInspector::setSuperSupport(bool hasSGBSupport) {
	bool result = false;
	if (hasSGBSupport)
		result = writeBytes(0x146, stringToBytes("03"));
	else
		result = writeBytes(0x146, stringToBytes("00"));

	return result;
}

std::string RomInspector::getCartridgeTypeCode(void) const {
	return readHexString(0x147, 0x01);
}

bool RomInspector::setCartridgeTypeCode(const std::string &cartridgeTypeCode) {
	return writeBytes(0x147, stringToBytes(padHex(cartridgeTypeCode, 0x02, '0')));
}

std::string RomInspector::getRomSizeCode(void) const {
	return readHexString(0x148, 0x01);
}

bool RomInspector::setRomSizeCode(const std::string &romSizeCode) {
	return writeBytes(0x148, stringToBytes(padHex(romSizeCode, 0x02, '0')));
}

std::string RomInspector::getRamSizeCode(void) const {
	return readHexString(0x149, 0x01);
}

bool RomInspector::setRamSizeCode(const std::string &ramSizeCode) {
	return writeBytes(0x149, stringToBytes(padHex(ramSizeCode, 0x02, '0')));
}

std::string RomInspector::getDestinationCode(void) const {
	return readHexString(0x14a, 0x01);
}

bool RomInspector::setDestinationCode(const std::string &destinationCode) {
	return writeBytes(0x14a, stringToBytes(padHex(destinationCode, 0x02, '0')));
}

std::string RomInspector::getOldLicenseCode(void) const {
	return readHexString(0x14b, 0x01);
}

bool RomInspector::setOldLicenseCode(const std::string &oldLicenseCode) {
	return writeBytes(0x14b, stringToBytes(padHex(oldLicenseCode, 0x02, '0')));
}

std::string RomInspector::getVersionNumber(void) const {
	return readHexString(0x14c, 0x01);
}

bool RomInspector::setVersionNumber(const std::string &versionNumber) {
	return writeBytes(0x14c, stringToBytes(padHex(versionNumber, 0x02, '0')));
}

std::string RomInspector::getHeaderChecksum(void) const {
	return readHexString(0x14d, 0x01);
}

bool RomInspector::setHeaderChecksum(const std::string &headerChecksum) {
	return writeBytes(0x14d, stringToBytes(padHex(headerChecksum, 0x02, '0')));
}

std::string RomInspector::getGlobalChecksum(void) const {
	return readHexString(0x14e, 0x02);
}

bool RomInspector::setGlobalChecksum(const std::string &globalChecksum) {
	return writeBytes(0x14e, stringToBytes(padHex(globalChecksum, 0x04, '0')));
}

std::string RomInspector::getCorrectHeaderChecksum(void) const {
	// DOC: CARTRIDGE SCHEMA.
	// To calculate the header checksum, start at the address 0x0134.
	// For each byte until address 0x014c, the calculated checksum (starting at 0) equals the calculated checksum - the byte you are currently at - 1.
	// The checksum is the last byte of the result.
	const std::string checksumData = readHexString(0x134, 0x19);
	int data1  = std::stoi(checksumData.substr(0,  2), 0, 16);
	int data2  = std::stoi(checksumData.substr(2,  2), 0, 16);
	int data3  = std::stoi(checksumData.substr(4,  2), 0, 16);
	int data4  = std::stoi(checksumData.substr(6,  2), 0, 16);
	int data5  = std::stoi(checksumData.substr(8,  2), 0, 16);
	int data6  = std::stoi(checksumData.substr(10, 2), 0, 16);
	int data7  = std::stoi(checksumData.substr(12, 2), 0, 16);
	int data8  = std::stoi(checksumData.substr(14, 2), 0, 16);
	int data9  = std::stoi(checksumData.substr(16, 2), 0, 16);
	int data10 = std::stoi(checksumData.substr(18, 2), 0, 16);
	int data11 = std::stoi(checksumData.substr(20, 2), 0, 16);
	int data12 = std::stoi(checksumData.substr(22, 2), 0, 16);
	int data13 = std::stoi(checksumData.substr(24, 2), 0, 16);
	int data14 = std::stoi(checksumData.substr(26, 2), 0, 16);
	int data15 = std::stoi(checksumData.substr(28, 2), 0, 16);
	int data16 = std::stoi(checksumData.substr(30, 2), 0, 16);
	int data17 = std::stoi(checksumData.substr(32, 2), 0, 16);
	int data18 = std::stoi(checksumData.substr(34, 2), 0, 16);
	int data19 = std::stoi(checksumData.substr(36, 2), 0, 16);
	int data20 = std::stoi(checksumData.substr(38, 2), 0, 16);
	int data21 = std::stoi(checksumData.substr(40, 2), 0, 16);
	int data22 = std::stoi(checksumData.substr(42, 2), 0, 16);
	int data23 = std::stoi(checksumData.substr(44, 2), 0, 16);
	int data24 = std::stoi(checksumData.substr(46, 2), 0, 16);
	int data25 = std::stoi(checksumData.substr(48, 2), 0, 16);

	const int checksum =
		0 -
		data1 - data2 - data3 - data4 - data5 - data6 - data7 - data8 - data9 - data10 - data11 - data12 - data13 -
		data14 - data15 - data16 - data17 - data18 - data19 - data20 - data21 - data22 - data23 - data24 - data25 -
		0x19;
	std::string checksumString = Text::toHex(checksum, 2, '0', true);
	checksumString = checksumString.substr(Math::max(0, (int)checksumString.length() - 2));

	return checksumString;
}

std::string RomInspector::getCorrectGlobalChecksum(void) const {
	unsigned long long checksum = 0;
	for (int i = 0; i < (int)_bytes->count(); ++i) {
		if (i == 0x014e || i == 0x014f)
			continue;

		checksum += _bytes->get((size_t)i);
	}
	const UInt16 u = (UInt16)(checksum & 0xffff);
	const std::string result = Text::toHex(u, 4, '0', true);

	return result;
}

std::string RomInspector::readHexString(int startPoint, int length) const {
	std::string hexString;
	_bytes->poke(startPoint);
	for (int x = 0; x < length; x++)
		hexString += Text::toHex(_bytes->readByte(), 2, '0', true);

	return hexString;
}

bool RomInspector::writeBytes(int startPoint, Bytes::Ptr byteArray) {
	if (!_bytes->poke(startPoint))
		return false;
	if (!_bytes->writeBytes(byteArray.get()))
		return false;

	return true;
}

std::string RomInspector::padHex(const std::string &txt, int width, char fill) {
	std::string result;
	if (width < 1) {
		// Do nothing.
	} else if (width == 1) {
		if (txt.empty())
			result = fill;
		else
			result = txt.back();
	} else {
		result = Text::padLeft(txt, 2, fill);
		result = Text::padRight(result, width, fill);
		result = Text::limitLeft(result, width);
	}

	return result;
}

Bytes::Ptr RomInspector::stringToBytes(const std::string &hex) {
	Bytes::Ptr _bytes(Bytes::create());
	for (int i = 0; i < (int)hex.length(); i += 2) {
		const std::string sub = hex.substr((size_t)i, 2);
		UInt8 byte = 0;
		try {
			byte = (UInt8)std::stoi(sub, 0, 16);
		} catch (...) {
			byte = 0;
		}
		_bytes->writeUInt8(byte);
	}

	return _bytes;
}

std::string RomInspector::convertAsciiToHex(const std::string &asciiString) {
	std::string result;
	for (char ch : asciiString) {
		union {
			UInt8 byte;
			std::string::value_type char_;
		} u;
		u.char_ = ch;
		result += Text::toHex(u.byte, 2, '0', true);
	}

	return result;
}

std::string RomInspector::convertHexToAscii(const std::string &hexString) {
	std::string result;
	for (int i = 0; i < (int)hexString.length(); i += 2) {
		std::string hs = hexString.substr(i, 2);
		union {
			UInt8 byte;
			std::string::value_type char_;
		} u;
		try {
			u.byte = (UInt8)std::stoi(hs, 0, 16);
		} catch (...) {
			u.byte = 0;
		}
		result += u.char_;

	}

	return result;
}

/* ===========================================================================} */
