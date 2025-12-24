/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "either.h"
#include "sfx.h"
#include "text.h"
#include "../../lib/audio_file/AudioFile.h"
#include "../../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Sfx
*/

class SfxImpl : public Sfx {
private:
	Sound _sound;

public:
	SfxImpl() {
	}
	virtual ~SfxImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Sfx** ptr, bool /* represented */) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		SfxImpl* result = static_cast<SfxImpl*>(Sfx::create());
		result->_sound = _sound;

		*ptr = result;

		return true;
	}
	virtual bool clone(Sfx** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Sfx* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		result = Math::hash(result, _sound.hash(false));

		return result;
	}
	virtual int compare(const Sfx* other) const override {
		const SfxImpl* implOther = static_cast<const SfxImpl*>(other);

		if (this == other)
			return 0;

		if (!other)
			return 1;

		return _sound.compare(implOther->_sound, false);
	}

	virtual const Sound* pointer(void) const override {
		return &_sound;
	}
	virtual Sound* pointer(void) override {
		return &_sound;
	}

	virtual bool load(const Sound &data) override {
		_sound = data;

		return true;
	}
	virtual void unload(void) override {
		_sound.~Sound();
		new (&_sound) Sound(); // To avoid stack overflow.
	}

	virtual bool toBytes(class Bytes* val) const override {
		rapidjson::Document doc;
		if (!toJson(doc, doc))
			return false;

		std::string txt;
		if (!Json::toString(doc, txt, false))
			return false;

		val->clear();
		val->writeString(txt);

		return true;
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		unload();

		std::string txt;
		txt.assign((const char*)val, size);

		rapidjson::Document doc;

		if (!Json::fromString(doc, txt.c_str()))
			return false;

		return fromJson(doc);
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		return _sound.toJson(val, doc);
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		unload();

		return _sound.fromJson(val);
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}
};

Sfx::Cell::Cell() {
}

Sfx::Cell::~Cell() {
}

bool Sfx::Cell::operator == (const Cell &other) const {
	return compare(other) == 0;
}

bool Sfx::Cell::operator != (const Cell &other) const {
	return compare(other) != 0;
}

bool Sfx::Cell::operator < (const Cell &other) const {
	return compare(other) < 0;
}

bool Sfx::Cell::operator > (const Cell &other) const {
	return compare(other) > 0;
}

int Sfx::Cell::compare(const Cell &other) const {
	if (command < other.command)
		return -1;
	else if (command > other.command)
		return 1;

	if (registers.size() < other.registers.size())
		return -1;
	else if (registers.size() > other.registers.size())
		return 1;

	for (int i = 0; i < (int)registers.size(); ++i) {
		const Register rl = registers[i];
		const Register rr = other.registers[i];
		if (rl < rr)
			return -1;
		else if (rl > rr)
			return 1;
	}

	return 0;
}

bool Sfx::Cell::equals(const Cell &other) const {
	return compare(other) == 0;
}

Sfx::Row::Row() {
}

Sfx::Row::~Row() {
}

Sfx::Sound::Sound() {
}

Sfx::Sound::~Sound() {
}

bool Sfx::Sound::operator == (const Sound &other) const {
	return compare(other, true) == 0;
}

bool Sfx::Sound::operator != (const Sound &other) const {
	return compare(other, true) != 0;
}

size_t Sfx::Sound::hash(bool countName) const {
	size_t result = 0;

	if (countName)
		result = Math::hash(result, name);

	result = Math::hash(result, mask);

	for (int j = 0; j < (int)rows.size(); ++j) {
		const Row &row = rows[j];

		result = Math::hash(result, row.delay);

		for (int i = 0; i < (int)row.cells.size(); ++i) {
			const Cell &cell = row.cells[i];

			result = Math::hash(result, cell.command);

			for (int k = 0; k < (int)cell.registers.size(); ++k) {
				const Register reg = cell.registers[k];
				result = Math::hash(result, reg);
			}
		}
	}

	return result;
}

int Sfx::Sound::compare(const Sound &other, bool countName) const {
	if (countName) {
		if (name < other.name)
			return -1;
		else if (name > other.name)
			return 1;
	}

	if (mask < other.mask)
		return -1;
	else if (mask > other.mask)
		return 1;

	if (rows.size() < other.rows.size())
		return -1;
	else if (rows.size() > other.rows.size())
		return 1;

	for (int j = 0; j < (int)rows.size(); ++j) {
		const Row &rowl = rows[j];
		const Row &rowr = other.rows[j];
		if (rowl.delay < rowr.delay)
			return -1;
		else if (rowl.delay > rowr.delay)
			return 1;

		if (rowl.cells.size() < rowr.cells.size())
			return -1;
		else if (rowl.cells.size() > rowr.cells.size())
			return 1;

		for (int i = 0; i < (int)rowl.cells.size(); ++i) {
			const Cell &celll = rowl.cells[i];
			const Cell &cellr = rowr.cells[i];
			if (celll < cellr)
				return -1;
			else if (celll > cellr)
				return 1;
		}
	}

	return 0;
}

bool Sfx::Sound::equals(const Sound &other) const {
	return compare(other, true) == 0;
}

bool Sfx::Sound::empty(void) const {
	if (mask == 0)
		return true;

	if (rows.empty())
		return true;

	return false;
}

bool Sfx::Sound::toString(std::string &val) const {
	val.clear();

	val += "0b" + Text::toBin(mask);
	val += ",";
	val += "\n";
	const int n = (int)rows.size();
	for (int j = 0; j < n; ++j) {
		const Row &row = rows[j];
		const int m = (int)row.cells.size();
		if (row.delay > 0x0f || m > 0x0f)
			return false;

		const int dm = (row.delay << 4) | (m & 0x0f);
		val += "0x" + Text::toHex(dm, 2, '0', false);
		val += ", ";
		for (int i = 0; i < m; ++i) {
			const Cell &cell = row.cells[i];
			const int p = (int)cell.registers.size();
			val += "0b" + Text::toBin(cell.command);
			if (!(i == m - 1 && j == n - 1 && p == 0))
				val += ", ";
			for (int k = 0; k < p; ++k) {
				const Register reg = cell.registers[k];
				val += "0x" + Text::toHex(reg, 2, '0', false);
				if (!(i == m - 1 && j == n - 1 && k == p - 1)) {
					if (i == m - 1 && k == p - 1)
						val += ",";
					else
						val += ", ";
				}
			}
		}
		val += "\n";
	}

	return true;
}

bool Sfx::Sound::fromString(const std::string &val) {
	typedef std::vector<int> Data;

	auto parse = [] (const std::string &txt) -> Data {
		Data result;
		Text::Array parts = Text::split(txt, ",");
		int i = 0;
		while (i < (int)parts.size()) {
			const std::string &part = parts[i];
			const std::string txt_ = Text::trim(part);
			if (txt_.empty()) {
				if (i == (int)parts.size() - 1)
					break;

				return Data();
			}

			int val = 0;
			if (!Text::fromString(txt_, val))
				return Data();

			result.push_back(val);
		}

		return result;
	};

	Mask mask_ = 0;
	Rows rows_;

	Text::Array lines = Text::split(val, "\n");
	int l = 0;
	while (l < (int)lines.size()) {
		const std::string &line = lines[l++];
		const std::string ln = Text::trim(line);
		if (ln.empty())
			continue;

		if (l == 0) {
			Data seq = parse(ln);
			if (seq.size() != 1)
				return false;

			mask_ = (Mask)seq.front();

			continue;
		}

		Text::Array parts = Text::split(val, ",");
		if (parts.empty())
			return false;

		int i = 0;
		const std::string &part = parts[i++];
		const std::string txt = Text::trim(part);
		if (txt.empty())
			return false;

		if (!Text::startsWith(txt, "0x", true))
			return false;

		int dm = 0;
		if (!Text::fromString(txt, dm))
			return false;
		if (dm > 0xff)
			return false;

		Row row;
		row.delay = (dm >> 4) & 0x0f;
		const int m = dm & 0x0f;
		for (int k = 0; k < m; ++k) {
			if (i >= (int)parts.size())
				return false;

			const std::string &part_ = parts[i++];
			const std::string txt_ = Text::trim(part_);
			if (txt_.empty())
				return false;

			if (Text::startsWith(txt_, "0b", true)) {
				int c = 0;
				if (!Text::fromString(txt_, c))
					return false;

				Cell cell;
				cell.command = (Command)c;
				row.cells.push_back(cell);

				continue;
			} else if (Text::startsWith(txt_, "0x", true)) {
				int r = 0;
				if (!Text::fromString(txt_, r))
					return false;

				if (row.cells.empty())
					return false;

				Cell &cell = row.cells.back();
				cell.registers.push_back((Register)r);

				continue;
			} else {
				return false;
			}
		}

		if (m != (int)row.cells.size())
			return false;

		rows_.push_back(row);
	}

	mask = mask_;
	rows = rows_;

	return true;
}

bool Sfx::Sound::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!Jpath::set(doc, val, name, "name"))
		return false;

	if (!Jpath::set(doc, val, mask, "mask"))
		return false;

	if (!Jpath::set(doc, val, Jpath::ANY(), "rows"))
		return false;

	rapidjson::Value* jrows = nullptr;
	if (!Jpath::get(val, jrows, "rows"))
		return false;

	for (int j = 0; j < (int)rows.size(); ++j) {
		const Row &row = rows[j];

		if (row.delay > 0x0f)
			return false;
		if (!Jpath::set(doc, *jrows, row.delay, j, "delay"))
			return false;

		if (row.cells.size() > 0x0f)
			return false;
		for (int i = 0; i < (int)row.cells.size(); ++i) {
			const Cell &cell = row.cells[i];
			if (!Jpath::set(doc, *jrows, cell.command, j, "cells", i, "command"))
				return false;

			if (!Jpath::set(doc, *jrows, Jpath::ANY(), j, "cells", i, "registers"))
				return false;

			rapidjson::Value* jregs = nullptr;
			if (!Jpath::get(*jrows, jregs, j, "cells", i, "registers"))
				return false;

			for (int k = 0; k < (int)cell.registers.size(); ++k) {
				const Register reg = cell.registers[k];
				if (!Jpath::set(doc, *jregs, reg, k))
					return false;
			}
		}
	}

	return true;
}

bool Sfx::Sound::fromJson(const rapidjson::Value &val) {
	rows.clear();

	if (!Jpath::get(val, name, "name"))
		return false;

	if (!Jpath::get(val, mask, "mask"))
		return false;

	const int n = Jpath::count(val, "rows");
	for (int j = 0; j < n; ++j) {
		rapidjson::Value* jrow = nullptr;
		if (!Jpath::get(val, jrow, "rows", j))
			return false;

		if (!jrow)
			return false;

		Row row;
		if (!Jpath::get(*jrow, row.delay, "delay"))
			return false;
		if (row.delay > 0x0f)
			return false;

		const int m = Jpath::count(*jrow, "cells");
		if (m > 0x0f)
			return false;
		for (int i = 0; i < m; ++i) {
			rapidjson::Value* jcell = nullptr;
			if (!Jpath::get(*jrow, jcell, "cells", i))
				return false;

			if (!jcell)
				return false;

			Cell cell;
			if (!Jpath::get(*jcell, cell.command, "command"))
				return false;

			const int p = Jpath::count(*jcell, "registers");
			for (int k = 0; k < p; ++k) {
				Register reg = 0;
				if (!Jpath::get(*jcell, reg, "registers", k))
					return false;

				cell.registers.push_back(reg);
			}

			row.cells.push_back(cell);
		}

		rows.push_back(row);
	}

	return true;
}

Sfx::VgmOptions::VgmOptions() {
}

Sfx::VgmOptions::~VgmOptions() {
}

Sfx::FxHammerOptions::FxHammerOptions() {
}

Sfx::FxHammerOptions::~FxHammerOptions() {
}

int Sfx::countVgm(class Bytes* val) {
	(void)val;

	return 1;
}

int Sfx::fromVgm(class Bytes* val, Sound::Array &sounds, const VgmOptions* options) {
	// See: https://github.com/untoxa/VGM2GBSFX/blob/main/utils/vgm2data.py.

	// Prepare.
	auto parse = [] (Sound &sound, Bytes* val, const VgmOptions &options) -> bool {
		// Prepare.
		typedef int Value;
		typedef std::map<int, Value> Channel;
		typedef std::map<int, Channel> Channels;

		typedef Either<Channel, std::nullptr_t> NullableChannel;
		typedef Either<Value, std::nullptr_t> NullableValue;

		auto pop = [] (Channels &dict, int key, const NullableChannel &default_ = Right<std::nullptr_t>(nullptr)) -> NullableChannel {
			Channels::const_iterator it = dict.find(key);
			if (it == dict.end())
				return default_;

			return Left<Channel>(it->second);
		};
		auto pop_ = [] (Channel &dict, int key, Value default_ = -1) -> Value {
			Channel::const_iterator it = dict.find(key);
			if (it == dict.end())
				return default_;

			return it->second;
		};

		auto range = [] (UInt8 val, UInt8 begin, UInt8 end) -> bool {
			return val >= begin && val < end;
		};
		auto inclusive = [] (UInt8 val, UInt8 begin, UInt8 end) -> bool {
			return val >= begin && val <= end;
		};

		constexpr const UInt8 NR1x = 0, NR2x = 1, NR3x = 2, NR4x = 3, NR5x = 4, PCMDATA = 5;
		constexpr const UInt8 NR10_REG = 0x10, NR11_REG = 0x11, NR12_REG = 0x12, NR13_REG = 0x13, NR14_REG = 0x14; // NR1x.
		constexpr const UInt8 NR20_REG = 0x15, NR21_REG = 0x16, NR22_REG = 0x17, NR23_REG = 0x18, NR24_REG = 0x19; // NR2x.
		constexpr const UInt8 NR30_REG = 0x1a, NR31_REG = 0x1b, NR32_REG = 0x1c, NR33_REG = 0x1d, NR34_REG = 0x1e; // NR3x.
		constexpr const UInt8 NR40_REG = 0x1f, NR41_REG = 0x20, NR42_REG = 0x21, NR43_REG = 0x22, NR44_REG = 0x23; // NR4x.
		constexpr const UInt8 NR50_REG = 0x24, NR51_REG = 0x25, NR52_REG = 0x26;                                   // NR5x.
		constexpr const UInt8 PCM_SAMPLE = 0x30;                                                                   // PCM wave pattern.
		constexpr const UInt8 PCM_LENGTH = 0x10;                                                                   // PCM wave pattern.

		Mask mask_ = 0;
		Rows rows_;

		// Convert.
		val->poke(0x34);
		const size_t cursor = val->peek();
		const UInt32 offset = val->readUInt32();
		val->poke(cursor + offset);

		UInt8 disabledChannels = 0;
		if (options.noNr1x)
			disabledChannels |= 1 << NR1x;
		if (options.noNr2x)
			disabledChannels |= 1 << NR2x;
		if (options.noNr3x)
			disabledChannels |= 1 << NR3x;
		if (options.noNr4x)
			disabledChannels |= 1 << NR4x;

		Channels channels;
		UInt8 data = val->readUInt8();
		while (data) {
			if (data == 0xb3) {
				const UInt8 addr = val->readUInt8() + NR10_REG;
				data = val->readUInt8();

				if (inclusive(addr, NR10_REG, NR14_REG)) {
					channels[NR1x][addr - NR10_REG] = data;
				} else if (inclusive(addr, NR21_REG, NR24_REG)) {
					channels[NR2x][addr - NR20_REG] = data;
				} else if (inclusive(addr, NR30_REG, NR34_REG)) {
					channels[NR3x][addr - NR30_REG] = data;
				} else if (inclusive(addr, NR41_REG, NR44_REG)) {
					channels[NR4x][addr - NR40_REG] = data;
				} else if (inclusive(addr, NR50_REG, NR52_REG)) {
					channels[NR5x][addr - NR50_REG] = data;
				} else if (range(addr, PCM_SAMPLE, PCM_SAMPLE + PCM_LENGTH)) {
					channels[PCMDATA][addr - PCM_SAMPLE] = data;
				} else {
					const std::string cmd = Text::toHex(addr, 2, '0', true);
					fprintf(stderr, "Invalid register address 0x%s.\n", cmd.c_str());

					return false;
				}

				const UInt8 value = data;
				(void)value;
			} else if ((data == 0x66) || (data >= 0x61 && data <= 0x63) || (data >= 0x70 && data <= 0x7f)) {
				if (data == 0x61) {
					const size_t cursor_ = val->peek();
					val->poke(cursor_ + 2);
				}

				Row row;

				// NR5x registers.
				NullableChannel ch = pop(channels, NR5x, Right<std::nullptr_t>(nullptr));
				if (ch.isLeft() && !options.noNr5x) {
					const Value val_ = pop_(ch.left().ref(), 2, -1);
					if (val_ != -1 && !options.noInit) {
						Cell cell;
						cell.command = 0b00100100;
						cell.registers.push_back((Register)val_);
						row.cells.push_back(cell);
					}
					UInt8 mask = NR5x;
					Cell cell;
					for (int i = 0; i < 1; ++i) {
						const Value val__ = pop_(ch.left().ref(), i, -1);
						if (val__ != -1) {
							mask |= 1 << (7 - i);
							cell.registers.push_back((Register)val__);
						}
					}
					if (mask != 4) {
						cell.command = mask;
						row.cells.push_back(cell);
					}
				}

				// Wav.
				ch = pop(channels, PCMDATA, Right<std::nullptr_t>(nullptr));
				if (ch.isLeft() && !options.noWave) {
					const int mask = PCMDATA;
					Cell cell;
					for (int i = 0; i < 16; ++i) {
						const Value val_ = pop_(ch.left().ref(), i, 0);
						cell.registers.push_back((Register)val_);
					}
					cell.command = (Command)mask;
					row.cells.push_back(cell);
				}

				// NR1x, NR2x, NR3x, NR4x registers.
				for (int j = NR1x; j <= NR4x; ++j) {
					ch = pop(channels, j, Right<std::nullptr_t>(nullptr));
					if (ch.isLeft() && ((1 << j) & disabledChannels) == 0) {
						int mask = j;
						Cell cell;
						for (int i = 0; i < 5; ++i) {
							const Value val_ = pop_(ch.left().ref(), i, -1);
							if (val_ != -1) {
								mask |= 0b10000000 >> i;
								cell.registers.push_back((Register)val_);
							}
						}
						if ((mask != j) && ((mask & 0b00001000) != 0)) {
							cell.command = (Command)mask;
							row.cells.push_back(cell);
							mask_ |= 1 << j;
						}
					}
				}

				// Optional delay.
				row.delay = (UInt8)Math::max(options.delay - 1, 0);

				// Output row.
				rows_.push_back(row);

				// Reset channels.
				channels.clear();

				// Write terminate sequence and exit.
				if (data == 0x66) {
					Cell cell;
					cell.command = 0b00000111;
					Row row;
					row.cells.push_back(cell);
					rows_.push_back(row);

					break;
				}
			} else {
				const std::string cmd = Text::toHex(data, 2, '0', true);
				fprintf(stderr, "Unsupported command 0x%s.\n", cmd.c_str());

				return false;
			}
			data = val->readUInt8();
		}

		// Assign.
		sound.mask = mask_;
		sound.rows = rows_;

		// Finish.
		return true;
	};

	const VgmOptions options_ = options ? *options : VgmOptions();

	sounds.clear();

	// Parse the header.
	Byte buf[8];
	if (!val->readBytes(buf, 4) || memcmp(buf, "Vgm ", 4) != 0) {
		fprintf(stderr, "Invalid file format.\n");

		return false;
	}

	val->poke(0x08);
	const UInt32 fileVer = val->readUInt32();
	if (fileVer < 0x161) {
		fprintf(stderr, "VGM version too low: %d.\n", fileVer);

		return false;
	}
	fprintf(stdout, "VGM file version: %d.\n", fileVer);

	val->poke(0x80);
	if (val->readUInt32() == 0) {
		fprintf(stderr, "VGM must contain GB data.\n");

		return false;
	}

	// Parse the data.
	Sound snd;
	if (!parse(snd, val, options_))
		return 0;

	sounds.push_back(snd);

	// Finish.
	return 1;
}

int Sfx::countWav(class Bytes* val) {
	(void)val;

	return 1;
}

int Sfx::fromWav(class Bytes* val, Sound::Array &sounds) {
	// See: https://github.com/untoxa/VGM2GBSFX/blob/main/utils/wav2data.py.

	// Prepare.
	auto parse = [] (Sound &sound, Bytes* val) -> bool {
		// Prepare.
		typedef double Sample;
		typedef std::vector<Sample> Samples;

		Mask mask_ = 0;
		Rows rows_;

		// Prepare the WAV data.
		Bytes::Collection wav;
		do {
			Bytes::Collection* coll = val->collection();
			AudioFile<Sample> audio;
			if (!audio.loadFromMemory(*coll))
				return false;

			const int ch = audio.getNumChannels();
			const uint32_t rate = audio.getSampleRate();
			const int depth = audio.getBitDepth();

			if (ch != 1)
				audio.setNumChannels(1);

			if (rate != 8000)
				audio.setSampleRate(8000);

			if (depth != 8)
				audio.setBitDepth(8);

			const Samples &samples = audio.samples.front();
			for (int i = 0; i < (int)samples.size(); ++i) {
				const UInt8 s = AudioSampleConverter<Sample>::sampleToUnsignedByte(samples[i]);
				wav.push_back(s);
			}
		} while (false);

		// Convert.
		mask_ = 0b00000100;

		const int n = (int)wav.size();
		const int m = n - n % 32;

		UInt8 c = 0;
		bool flag = false;
		int count = 0;

		Row row;
		Cell cell;
		for (int i = 0; i < m; ++i) {
			c = ((c << 4) | (wav[i] >> 4)) & 0xff;
			if (flag) {
				cell.registers.push_back(c);
				++count;
				if (count % 16 == 0) {
					cell.command = 0b00000110;
					row.cells.push_back(cell);
					rows_.push_back(row);

					row = Row();
					cell = Cell();
				}
			}
			flag = !flag;
		}

		row = Row();
		cell = Cell();

		cell.command = 0b00000111;
		row.cells.push_back(cell);
		rows_.push_back(row);

		// Assign.
		sound.mask = mask_;
		sound.rows = rows_;

		// Finish.
		return true;
	};

	sounds.clear();

	// Parse the data.
	Sound snd;
	if (!parse(snd, val))
		return 0;

	sounds.push_back(snd);

	// Finish.
	return 1;
}

int Sfx::countFxHammer(class Bytes* val) {
	int result = 0;

	Byte buf[16];
	if (!val->poke(9) || !val->readBytes(buf, 9) || memcmp(buf, "FX HAMMER", 9) != 0)
		return result;

	for (int i = 0; i < 0x3c; ++i) {
		const size_t cursor = 0x300 + i;
		if (cursor >= val->count())
			continue;

		const UInt8 channels = val->get(cursor);
		if (channels == 0)
			continue;

		++result;
	}

	return result;
}

int Sfx::fromFxHammer(class Bytes* val, Sound::Array &sounds, const FxHammerOptions* options) {
	// See: https://github.com/untoxa/VGM2GBSFX/blob/main/utils/fxhammer2data.py.

	// Prepare.
	auto parse = [] (Sound &sound, Bytes* val, int index, UInt8 channels, const FxHammerOptions &options) -> bool {
		// Prepare.
		typedef std::pair<int, int> Cache;

		auto fillCell = [] (UInt8 ch, UInt8 a, UInt8 b, UInt8 c, UInt8 d, Cache &cache) -> Cell {
			Cell result;
			UInt8 mask = 0b01001000 | ch;
			result.registers.push_back(a);
			if (b != cache.first) {
				mask |= 0b00100000;
				result.registers.push_back(b);
				cache.first = b;
			}
			if (c != cache.second) {
				mask |= 0b00010000;
				result.registers.push_back(c);
				cache.second = c;
			}
			result.registers.push_back(d);
			result.command = mask;

			return result;
		};

		constexpr const UInt16 NOTE_FREQS[] = {
			  44,  156,  262,  363,  457,  547,  631,  710,  786,  854,  923,  986,
			1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517,
			1546, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783,
			1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915,
			1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982,
			1985, 1988, 1992, 1995, 1998, 2001, 2004, 2006, 2009, 2011, 2013, 2015
		};

		const Mask mask_ = ((channels & 0xf0) ? 0x02 : 0x00) | ((channels & 0x0f) ? 0x08 : 0x00);
		Rows rows_;

		// Convert.
		const size_t len = 256;
		const size_t begin = 0x400 + index * len;
		const size_t end = begin + len;

		if (begin >= val->count() || end > val->count()) {
			fprintf(stderr, "Unexpected end of file.\n");

			return false;
		}

		Cache ch2Cache = std::make_pair(-1, -1);
		Cache ch4Cache = std::make_pair(-1, -1);
		UInt8 oldPan = 0xff;

		for (int i = 0; i < 32; ++i) {
			const UInt8 duration = val->get(begin + i * 8 + 0);
			const UInt8 ch2pan   = val->get(begin + i * 8 + 1);
			const UInt8 ch2vol   = val->get(begin + i * 8 + 2);
			const UInt8 ch2duty  = val->get(begin + i * 8 + 3);
			const UInt8 ch2note  = val->get(begin + i * 8 + 4);
			const UInt8 ch4pan   = val->get(begin + i * 8 + 5);
			const UInt8 ch4vol   = val->get(begin + i * 8 + 6);
			const UInt8 ch4freq  = val->get(begin + i * 8 + 7);

			if (duration == 0)
				break;

			Row row;

			if (options.usePan) {
				const UInt8 currentPan = 0b01010101 | ch2pan | ch4pan;
				if (oldPan != currentPan) {
					Cell cell;
					cell.command = 0b01000100;
					cell.registers.push_back(currentPan);
					row.cells.push_back(cell);
					oldPan = currentPan;
				}
			}

			if (ch2pan != 0) {
				const UInt16 freq = NOTE_FREQS[(ch2note - 0x40) >> 1];
				if (options.optimize) {
					const Cell cell = fillCell(1, ch2duty, ch2vol, freq & 0xff, ((freq >> 8) | 0x80) & 0xff, ch2Cache);
					row.cells.push_back(cell);
				} else {
					Cell cell;
					cell.command = 0b01111001;
					cell.registers.push_back(ch2duty);
					cell.registers.push_back(ch2vol);
					cell.registers.push_back(freq & 0xff);
					cell.registers.push_back(((freq >> 8) | 0x80) & 0xff);
					row.cells.push_back(cell);
				}
			}

			if (ch4pan != 0) {
				if (options.optimize) {
					const Cell cell = fillCell(3, 0x2a, ch4vol, ch4freq, 0x80, ch4Cache);
					row.cells.push_back(cell);
				} else {
					Cell cell;
					cell.command = 0b01111011;
					cell.registers.push_back(0x2a);
					cell.registers.push_back(ch4vol);
					cell.registers.push_back(ch4freq);
					cell.registers.push_back(0x80);
					row.cells.push_back(cell);
				}
			}

			int delay = Math::max(0, options.delay * duration - 1);
			int delta = Math::min(15, delay);

			row.delay = (UInt8)delta;
			rows_.push_back(row);

			delay -= delta;
			while (delay > 0) {
				delta = Math::min(15, delay);
				row = Row();
				row.delay = (UInt8)delta;
				rows_.push_back(row);
				delay -= delta;
			}
		}

		Row row;
		if (options.cutSound) {
			if (mask_ & 2) {
				Cell cell;
				cell.command = 0b00101001;
				cell.registers.push_back(0);
				cell.registers.push_back(0xc0);
				row.cells.push_back(cell);
			}

			if (mask_ & 8) {
				Cell cell;
				cell.command = 0b00101011;
				cell.registers.push_back(0);
				cell.registers.push_back(0xc0);
				row.cells.push_back(cell);
			}
		}

		if (options.usePan) {
			Cell cell;
			cell.command = 0b01000100;
			cell.registers.push_back(0xff);
			row.cells.push_back(cell);
		}

		Cell cell;
		cell.command = 0b00000111;
		row.cells.push_back(cell);
		rows_.push_back(row);

		// Assign.
		sound.mask = mask_;
		sound.rows = rows_;

		// Finish.
		return true;
	};

	const FxHammerOptions options_ = options ? *options : FxHammerOptions();

	sounds.clear();

	// Parse the header.
	Byte buf[16];
	if (!val->poke(9) || !val->readBytes(buf, 9) || memcmp(buf, "FX HAMMER", 9) != 0) {
		fprintf(stderr, "Invalid file format.\n");

		return false;
	}

	// Parse the data.
	int result = 0;
	if (options_.index == -1) {
		for (int i = 0; i < 0x3c; ++i) {
			const size_t cursor = 0x300 + i;
			if (cursor >= val->count())
				continue;

			const UInt8 channels = val->get(cursor);
			if (channels == 0)
				continue;

			Sound snd;
			if (!parse(snd, val, i, channels, options_))
				return 0;

			sounds.push_back(snd);

			++result;
		}
	} else {
		const size_t cursor = 0x300 + options_.index;
		if (cursor >= val->count())
			return 0;

		const UInt8 channels = val->get(cursor);
		if (channels == 0)
			return 0;

		Sound snd;
		if (!parse(snd, val, options_.index, channels, options_))
			return 0;

		sounds.push_back(snd);

		++result;
	}

	// Finish.
	return result;
}

Sfx* Sfx::create(void) {
	SfxImpl* result = new SfxImpl();

	return result;
}

void Sfx::destroy(Sfx* ptr) {
	SfxImpl* impl = static_cast<SfxImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
