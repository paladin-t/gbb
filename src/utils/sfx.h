/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __SFX_H__
#define __SFX_H__

#include "../gbbasic.h"
#include "cloneable.h"
#include "json.h"

/*
** {===========================================================================
** Sfx
*/

/**
 * @brief Sfx resource object.
 */
class Sfx : public Cloneable<Sfx>, public virtual Object {
public:
	typedef std::shared_ptr<Sfx> Ptr;

	typedef UInt8 Mask;

	typedef UInt8 Command;

	typedef UInt8 Register;
	typedef std::vector<Register> Registers;

	struct Cell {
		Command command = 0;
		Registers registers;

		Cell();
		~Cell();

		bool operator == (const Cell &other) const;
		bool operator != (const Cell &other) const;
		bool operator < (const Cell &other) const;
		bool operator > (const Cell &other) const;

		int compare(const Cell &other) const;
		bool equals(const Cell &other) const;
	};
	typedef std::vector<Cell> Cells;
	struct Row {
		UInt8 delay = 0;
		Cells cells;

		Row();
		~Row();
	};
	typedef std::vector<Row> Rows;

	struct Sound {
		typedef std::vector<Sound> Array;

		std::string name;
		Mask mask = 0;
		Rows rows;

		Sound();
		~Sound();

		bool operator == (const Sound &other) const;
		bool operator != (const Sound &other) const;

		size_t hash(bool countName) const;
		int compare(const Sound &other, bool countName) const;
		bool equals(const Sound &other) const;

		bool empty(void) const;

		bool toString(std::string &val) const;
		bool fromString(const std::string &val);

		bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const;
		bool fromJson(const rapidjson::Value &val);
	};

	struct VgmOptions {
		bool noNr1x = false;
		bool noNr2x = false;
		bool noNr3x = false;
		bool noNr4x = false;
		bool noNr5x = false;
		bool noInit = false;
		bool noWave = false;
		int delay = 1; // With range of values from 1 to 16.

		VgmOptions();
		~VgmOptions();
	};
	struct FxHammerOptions {
		int index = -1; // -1 for all.
		bool cutSound = true; //false;
		bool usePan = true;
		bool optimize = true;
		int delay = 1; // With range of values from 1 to 16.

		FxHammerOptions();
		~FxHammerOptions();
	};

public:
	GBBASIC_CLASS_TYPE('S', 'F', 'X', 'A')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(Sfx** ptr, bool represented) const = 0;
	using Cloneable<Sfx>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const Sfx* other) const = 0;

	virtual const Sound* pointer(void) const = 0;
	virtual Sound* pointer(void) = 0;

	virtual bool load(const Sound &data) = 0;
	virtual void unload(void) = 0;

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

	static int countVgm(class Bytes* val);
	/**
	 * @param[out] sounds
	 */
	static int fromVgm(class Bytes* val, Sound::Array &sounds, const VgmOptions* options /* nullable */);

	static int countWav(class Bytes* val);
	/**
	 * @param[out] sounds
	 */
	static int fromWav(class Bytes* val, Sound::Array &sounds);

	static int countFxHammer(class Bytes* val);
	/**
	 * @param[out] sounds
	 */
	static int fromFxHammer(class Bytes* val, Sound::Array &sounds, const FxHammerOptions* options /* nullable */);

	static Sfx* create(void);
	static void destroy(Sfx* ptr);
};

/* ===========================================================================} */

#endif /* __SFX_H__ */
