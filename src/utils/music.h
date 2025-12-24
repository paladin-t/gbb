/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __MUSIC_H__
#define __MUSIC_H__

#include "../gbbasic.h"
#include "cloneable.h"
#include "either.h"
#include "json.h"
#include "mathematics.h"
#include <array>
#include <set>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef MUSIC_ENCODE_NOTE
#	define MUSIC_ENCODE_NOTE(A, B, C) (unsigned char)(A), (unsigned char)(((B) << 4) | ((C) >> 8)), (unsigned char)((C) & 0xff)
#endif /* MUSIC_ENCODE_NOTE */

#ifndef MUSIC_NODTES
#	define MUSIC_NODTES
#	define C_3       0
#	define Cs3       1
#	define D_3       2
#	define Ds3       3
#	define E_3       4
#	define F_3       5
#	define Fs3       6
#	define G_3       7
#	define Gs3       8
#	define A_3       9
#	define As3       10
#	define B_3       11
#	define C_4       12
#	define Cs4       13
#	define D_4       14
#	define Ds4       15
#	define E_4       16
#	define F_4       17
#	define Fs4       18
#	define G_4       19
#	define Gs4       20
#	define A_4       21
#	define As4       22
#	define B_4       23
#	define C_5       24
#	define Cs5       25
#	define D_5       26
#	define Ds5       27
#	define E_5       28
#	define F_5       29
#	define Fs5       30
#	define G_5       31
#	define Gs5       32
#	define A_5       33
#	define As5       34
#	define B_5       35
#	define C_6       36
#	define Cs6       37
#	define D_6       38
#	define Ds6       39
#	define E_6       40
#	define F_6       41
#	define Fs6       42
#	define G_6       43
#	define Gs6       44
#	define A_6       45
#	define As6       46
#	define B_6       47
#	define C_7       48
#	define Cs7       49
#	define D_7       50
#	define Ds7       51
#	define E_7       52
#	define F_7       53
#	define Fs7       54
#	define G_7       55
#	define Gs7       56
#	define A_7       57
#	define As7       58
#	define B_7       59
#	define C_8       60
#	define Cs8       61
#	define D_8       62
#	define Ds8       63
#	define E_8       64
#	define F_8       65
#	define Fs8       66
#	define G_8       67
#	define Gs8       68
#	define A_8       69
#	define As8       70
#	define B_8       71
#	define LAST_NOTE 72
#	define ___       90
#endif /* MUSIC_NODTES */

#ifndef MUSIC_PATTERN_SIZE
#	define MUSIC_PATTERN_SIZE (3 * GBBASIC_MUSIC_PATTERN_COUNT)
#endif /* MUSIC_PATTERN_SIZE */

#ifndef MUSIC_DUTY_INSTRUMENT_MIN_LENGTH
#	define MUSIC_DUTY_INSTRUMENT_MIN_LENGTH 0
#endif /* MUSIC_DUTY_INSTRUMENT_MIN_LENGTH */
#ifndef MUSIC_DUTY_INSTRUMENT_MAX_LENGTH
#	define MUSIC_DUTY_INSTRUMENT_MAX_LENGTH 64
#endif /* MUSIC_DUTY_INSTRUMENT_MAX_LENGTH */
#ifndef MUSIC_DUTY_INSTRUMENT_MIN_INITIAL_VOLUME
#	define MUSIC_DUTY_INSTRUMENT_MIN_INITIAL_VOLUME 0
#endif /* MUSIC_DUTY_INSTRUMENT_MIN_INITIAL_VOLUME */
#ifndef MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME
#	define MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME 15
#endif /* MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME */
#ifndef MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE
#	define MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE -7
#endif /* MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE */
#ifndef MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE
#	define MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE 7
#endif /* MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE */
#ifndef MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_SHIFT
#	define MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_SHIFT -7
#endif /* MUSIC_DUTY_INSTRUMENT_MIN_VOLUME_SWEEP_SHIFT */
#ifndef MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_SHIFT
#	define MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_SHIFT 7
#endif /* MUSIC_DUTY_INSTRUMENT_MAX_VOLUME_SWEEP_SHIFT */

#ifndef MUSIC_WAVE_INSTRUMENT_MIN_LENGTH
#	define MUSIC_WAVE_INSTRUMENT_MIN_LENGTH 0
#endif /* MUSIC_WAVE_INSTRUMENT_MIN_LENGTH */
#ifndef MUSIC_WAVE_INSTRUMENT_MAX_LENGTH
#	define MUSIC_WAVE_INSTRUMENT_MAX_LENGTH 256
#endif /* MUSIC_WAVE_INSTRUMENT_MAX_LENGTH */

#ifndef MUSIC_NOISE_INSTRUMENT_MIN_LENGTH
#	define MUSIC_NOISE_INSTRUMENT_MIN_LENGTH 0
#endif /* MUSIC_NOISE_INSTRUMENT_MIN_LENGTH */
#ifndef MUSIC_NOISE_INSTRUMENT_MAX_LENGTH
#	define MUSIC_NOISE_INSTRUMENT_MAX_LENGTH 64
#endif /* MUSIC_NOISE_INSTRUMENT_MAX_LENGTH */
#ifndef MUSIC_NOISE_INSTRUMENT_MIN_INITIAL_VOLUME
#	define MUSIC_NOISE_INSTRUMENT_MIN_INITIAL_VOLUME 0
#endif /* MUSIC_NOISE_INSTRUMENT_MIN_INITIAL_VOLUME */
#ifndef MUSIC_NOISE_INSTRUMENT_MAX_INITIAL_VOLUME
#	define MUSIC_NOISE_INSTRUMENT_MAX_INITIAL_VOLUME 15
#endif /* MUSIC_NOISE_INSTRUMENT_MAX_INITIAL_VOLUME */
#ifndef MUSIC_NOISE_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE
#	define MUSIC_NOISE_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE -7
#endif /* MUSIC_NOISE_INSTRUMENT_MIN_VOLUME_SWEEP_CHANGE */
#ifndef MUSIC_NOISE_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE
#	define MUSIC_NOISE_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE 7
#endif /* MUSIC_NOISE_INSTRUMENT_MAX_VOLUME_SWEEP_CHANGE */

/* ===========================================================================} */

/*
** {===========================================================================
** Music
*/

/**
 * @brief Music resource object.
 */
class Music : public Cloneable<Music>, public virtual Object {
public:
	/**< Generic. */

	typedef std::shared_ptr<Music> Ptr;

	typedef Either<int, std::nullptr_t> NullableInt;

	/**< Pattern. */

	struct Cell {
		int note                           = ___;
		int instrument                     = 0;
		int effectCode                     = 0;
		int effectParameters               = 0;

		Cell();
		Cell(int n, int in, int fx, int params);
		~Cell();

		bool operator == (const Cell &other) const;
		bool operator != (const Cell &other) const;
		bool operator < (const Cell &other) const;
		bool operator > (const Cell &other) const;

		int compare(const Cell &other) const;
		bool equals(const Cell &other) const;

		bool empty(void) const;

		bool serialize(Bytes* bytes) const;

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);

		static Cell BLANK(void);
	};
	typedef std::array<Cell, GBBASIC_MUSIC_PATTERN_COUNT> Pattern;
	typedef std::array<Pattern, GBBASIC_MUSIC_ORDER_COUNT> Order;
	typedef std::array<Order, GBBASIC_MUSIC_CHANNEL_COUNT> Orders;

	typedef std::vector<int> Sequence;
	typedef std::array<Sequence, GBBASIC_MUSIC_CHANNEL_COUNT> Channels;

	/**< Instrument. */

	enum class Instruments : UInt8 {
		SQUARE,
		WAVE,
		NOISE
	};
	struct BaseInstrument {
		std::string name;
		NullableInt length                 = Right<std::nullptr_t>(nullptr);

		BaseInstrument();
		~BaseInstrument();
	};
	struct DutyInstrument : public BaseInstrument {
		typedef std::array<DutyInstrument, GBBASIC_MUSIC_INSTRUMENT_COUNT> Array;

		int initialVolume                  = MUSIC_DUTY_INSTRUMENT_MAX_INITIAL_VOLUME;
		int volumeSweepChange              = 0;
		int frequencySweepTime             = 0;
		int frequencySweepShift            = 0;
		int dutyCycle                      = 2;

		DutyInstrument();
		~DutyInstrument();

		bool operator == (const DutyInstrument &other) const;
		bool operator != (const DutyInstrument &other) const;
		bool operator < (const DutyInstrument &other) const;
		bool operator > (const DutyInstrument &other) const;

		int compare(const DutyInstrument &other) const;
		bool equals(const DutyInstrument &other) const;

		bool serialize(Bytes* bytes) const;
		bool parse(const Bytes* bytes);

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);
	};
	struct WaveInstrument : public BaseInstrument {
		typedef std::array<WaveInstrument, GBBASIC_MUSIC_INSTRUMENT_COUNT> Array;

		int volume                         = 0;
		int waveIndex                      = 0;

		WaveInstrument();
		~WaveInstrument();

		bool operator == (const WaveInstrument &other) const;
		bool operator != (const WaveInstrument &other) const;
		bool operator < (const WaveInstrument &other) const;
		bool operator > (const WaveInstrument &other) const;

		int compare(const WaveInstrument &other) const;
		bool equals(const WaveInstrument &other) const;

		bool serialize(Bytes* bytes) const;
		bool parse(const Bytes* bytes);

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);
	};
	struct NoiseInstrument : public BaseInstrument {
		typedef std::array<NoiseInstrument, GBBASIC_MUSIC_INSTRUMENT_COUNT> Array;

		enum class BitCount : int {
			_7,
			_15
		};

		typedef std::vector<int> Macros;

		int initialVolume                  = 0;
		int volumeSweepChange              = 0;
		int shiftClockMask                 = 0;
		int dividingRatio                  = 0;
		BitCount bitCount                  = BitCount::_7;
		Macros noiseMacro;

		NoiseInstrument();
		~NoiseInstrument();

		bool operator == (const NoiseInstrument &other) const;
		bool operator != (const NoiseInstrument &other) const;
		bool operator < (const NoiseInstrument &other) const;
		bool operator > (const NoiseInstrument &other) const;

		int compare(const NoiseInstrument &other) const;
		bool equals(const NoiseInstrument &other) const;

		bool serialize(Bytes* bytes) const;
		bool parse(const Bytes* bytes);

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);
	};

	/**< Wave. */

	typedef std::array<UInt8, GBBASIC_MUSIC_WAVEFORM_SAMPLE_COUNT> Wave;
	typedef std::array<Wave, GBBASIC_MUSIC_WAVEFORM_COUNT> Waves;

	static bool serialize(Bytes* bytes, const Wave &wave);
	static bool parse(Wave &wave, const Bytes* bytes);

	/**< Song. */

	struct Song {
		typedef std::set<int> OrderSet;
		typedef std::array<OrderSet, GBBASIC_MUSIC_CHANNEL_COUNT> OrderMatrix;

		std::string name;
		std::string artist;
		std::string comment;
		DutyInstrument::Array dutyInstruments;
		WaveInstrument::Array waveInstruments;
		NoiseInstrument::Array noiseInstruments;
		Waves waves;
		int ticksPerRow                    = 3;
		Orders orders;
		Channels channels;

		Song();
		~Song();

		bool operator == (const Song &other) const;
		bool operator != (const Song &other) const;

		size_t hash(bool countName) const;
		int compare(const Song &other, bool countName) const;
		bool equals(const Song &other) const;

		int instrumentCount(void) const;
		const BaseInstrument* instrumentAt(int idx, Instruments* type /* nullable */) const;
		BaseInstrument* instrumentAt(int idx, Instruments* type /* nullable */);

		OrderMatrix orderMatrix(void) const;

		bool findStop(int &seq, int &ln, int &ticks) const;

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);
	};

	/**< Handlers. */

	typedef std::function<void(const char*)> ErrorHandler;

public:
	GBBASIC_CLASS_TYPE('A', 'U', 'D', 'A')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Music** ptr, bool represented) const = 0;
	using Cloneable<Music>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const Music* other) const = 0;

	virtual const Song* pointer(void) const = 0;
	virtual Song* pointer(void) = 0;

	virtual bool load(const Song &data) = 0;
	virtual void unload(void) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toC(std::string &val) const = 0;
	virtual bool fromC(const char* filename, const std::string &val, ErrorHandler onError) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toBytes(class Bytes* val) const = 0;
	virtual bool fromBytes(const Byte* val, size_t size) = 0;
	virtual bool fromBytes(const class Bytes* val) = 0;

	/**
	 * @param[out] val
	 * @param[in, out] doc
	 */
	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const = 0;
	/**
	 * @param[in, out] val
	 */
	virtual bool toJson(rapidjson::Document &val) const = 0;
	virtual bool fromJson(const rapidjson::Value &val) = 0;
	virtual bool fromJson(const rapidjson::Document &val) = 0;

	static bool empty(const Pattern &pattern);

	/**
	 * @param[out] val
	 */
	static bool toBytes(class Bytes* val, const Orders &orders);
	static bool fromBytes(const Byte* val, size_t size, Orders &orders);
	static bool fromBytes(const class Bytes* val, Orders &orders);

	/**
	 * @param[out] val
	 */
	static bool toBytes(class Bytes* val, const Channels &channels);
	static bool fromBytes(const Byte* val, size_t size, Channels &channels);
	static bool fromBytes(const class Bytes* val, Channels &channels);

	static int noteOf(const std::string &name);
	static std::string nameOf(int note);

	static Music* create(void);
	static void destroy(Music* ptr);
};

/* ===========================================================================} */

#endif /* __MUSIC_H__ */
