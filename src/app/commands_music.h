/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMANDS_MUSIC_H__
#define __COMMANDS_MUSIC_H__

#include "commands_layered.h"
#include "../utils/assets.h"

/*
** {===========================================================================
** Music commands
*/

namespace Commands {

namespace Music {

class Playable : public Layered::Layered {
public:
	typedef std::function<::Music::Cell*(int, int, int)> Getter;

	typedef std::function<void(int, int, int)> ViewSetter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(ViewSetter, setView)
	GBBASIC_PROPERTY(int, sequenceIndex)
	GBBASIC_PROPERTY(int, channelIndex)
	GBBASIC_PROPERTY(int, lineIndex)
	GBBASIC_PROPERTY(int, note)
	GBBASIC_PROPERTY(int, instrument)
	GBBASIC_PROPERTY(int, effectCode)
	GBBASIC_PROPERTY(int, effectParameters)

public:
	Playable();

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
};

class SetNote : public Playable {
public:
	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	SetNote();
	virtual ~SetNote() override;

	GBBASIC_CLASS_TYPE('S', 'N', 'T', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetNote* with(Getter get);
	virtual SetNote* with(ViewSetter set, int sequence, int channel, int line);
	virtual SetNote* with(int note_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetInstrument : public Playable {
public:
	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	SetInstrument();
	virtual ~SetInstrument() override;

	GBBASIC_CLASS_TYPE('S', 'I', 'N', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetInstrument* with(Getter get);
	virtual SetInstrument* with(ViewSetter set, int sequence, int channel, int line);
	virtual SetInstrument* with(int inst);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetEffectCode : public Playable {
public:
	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	SetEffectCode();
	virtual ~SetEffectCode() override;

	GBBASIC_CLASS_TYPE('S', 'E', 'C', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetEffectCode* with(Getter get);
	virtual SetEffectCode* with(ViewSetter set, int sequence, int channel, int line);
	virtual SetEffectCode* with(int code);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetEffectParameters : public Playable {
public:
	GBBASIC_PROPERTY(bool, filled)
	GBBASIC_PROPERTY(int, old)

public:
	SetEffectParameters();
	virtual ~SetEffectParameters() override;

	GBBASIC_CLASS_TYPE('S', 'E', 'P', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetEffectParameters* with(Getter get);
	virtual SetEffectParameters* with(ViewSetter set, int sequence, int channel, int line);
	virtual SetEffectParameters* with(int params);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ChangeOrder : public Playable {
public:
	GBBASIC_PROPERTY(::Music::Channels, channels)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	ChangeOrder();
	virtual ~ChangeOrder() override;

	GBBASIC_CLASS_TYPE('C', 'O', 'R', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual ChangeOrder* with(ViewSetter set, int sequence, int channel, int line);
	virtual ChangeOrder* with(const ::Music::Channels &channels);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class BlitForTracker : public Layered::Layered {
public:
	typedef std::vector<int> DataArray;

	typedef std::function<bool(const Tracker::Cursor &, int &)> Getter;
	typedef std::function<bool(const Tracker::Cursor &, const int &)> Setter;

public:
	GBBASIC_PROPERTY_READONLY(Getter, get)
	GBBASIC_PROPERTY_READONLY(Setter, set)
	GBBASIC_PROPERTY(Tracker::Range, area)
	GBBASIC_PROPERTY(DataArray, values)
	GBBASIC_PROPERTY(int, sequenceIndex)
	GBBASIC_PROPERTY(int, channelIndex)
	GBBASIC_PROPERTY(int, lineIndex)

	GBBASIC_PROPERTY(DataArray, old)

public:
	BlitForTracker();
	virtual ~BlitForTracker() override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual BlitForTracker* with(Getter get, Setter set);
	virtual BlitForTracker* with(const Tracker::Range &area, const DataArray &values, const Tracker::Range &srcArea);
	virtual BlitForTracker* with(const Tracker::Range &area, const DataArray &values);
	virtual BlitForTracker* with(const Tracker::Range &area, Getter getValues);
	virtual BlitForTracker* with(int sequence, int channel, int line);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class CutForTracker : public BlitForTracker {
public:
	CutForTracker();
	virtual ~CutForTracker() override;

	GBBASIC_CLASS_TYPE('T', 'C', 'U', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class PasteForTracker : public BlitForTracker {
public:
	PasteForTracker();
	virtual ~PasteForTracker() override;

	GBBASIC_CLASS_TYPE('T', 'P', 'S', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteForTracker : public BlitForTracker {
public:
	DeleteForTracker();
	virtual ~DeleteForTracker() override;

	GBBASIC_CLASS_TYPE('T', 'D', 'E', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class BlitForInstrument : public Layered::Layered {
public:
	typedef std::function<void(::Music::Instruments, int, int)> InstrumentIndexSetter;

public:
	GBBASIC_PROPERTY_READONLY(InstrumentIndexSetter, setInstrumentIndex)
	GBBASIC_PROPERTY_READONLY(::Music::Instruments, instrumentType)
	GBBASIC_PROPERTY_READONLY(int, instrumentGroup)
	GBBASIC_PROPERTY_READONLY(int, instrumentIndex)

	GBBASIC_PROPERTY(::Music::DutyInstrument, dutyData)
	GBBASIC_PROPERTY(::Music::WaveInstrument, waveData)
	GBBASIC_PROPERTY(::Music::NoiseInstrument, noiseData)
	GBBASIC_PROPERTY(::Music::DutyInstrument, oldDuty)
	GBBASIC_PROPERTY(::Music::WaveInstrument, oldWave)
	GBBASIC_PROPERTY(::Music::NoiseInstrument, oldNoise)

public:
	BlitForInstrument();
	virtual ~BlitForInstrument() override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual BlitForInstrument* with(InstrumentIndexSetter setInstrumentIndex_, int group, int idx);
	virtual BlitForInstrument* with(const ::Music::DutyInstrument &data);
	virtual BlitForInstrument* with(const ::Music::WaveInstrument &data);
	virtual BlitForInstrument* with(const ::Music::NoiseInstrument &data);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class CutForInstrument : public BlitForInstrument {
public:
	CutForInstrument();
	virtual ~CutForInstrument() override;

	GBBASIC_CLASS_TYPE('I', 'C', 'U', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class PasteForInstrument : public BlitForInstrument {
public:
	PasteForInstrument();
	virtual ~PasteForInstrument() override;

	GBBASIC_CLASS_TYPE('I', 'P', 'S', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteForInstrument : public BlitForInstrument {
public:
	DeleteForInstrument();
	virtual ~DeleteForInstrument() override;

	GBBASIC_CLASS_TYPE('I', 'D', 'E', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class BlitForWave : public Layered::Layered {
public:
	typedef std::function<void(int)> WaveIndexSetter;
	typedef std::function<int(void)> WaveIndexGetter;

public:
	GBBASIC_PROPERTY_READONLY(WaveIndexSetter, setWaveIndex)
	GBBASIC_PROPERTY_READONLY(int, waveIndex)

	GBBASIC_PROPERTY(::Music::Wave, data)
	GBBASIC_PROPERTY(::Music::Wave, old)

public:
	BlitForWave();
	virtual ~BlitForWave() override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual BlitForWave* with(WaveIndexSetter setWaveIndex_, WaveIndexGetter getWaveIndex_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class CutForWave : public BlitForWave {
public:
	CutForWave();
	virtual ~CutForWave() override;

	GBBASIC_CLASS_TYPE('W', 'C', 'U', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class PasteForWave : public BlitForWave {
public:
	PasteForWave();
	virtual ~PasteForWave() override;

	GBBASIC_CLASS_TYPE('W', 'P', 'S', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual PasteForWave* with(const ::Music::Wave &data_);
	using BlitForWave::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteForWave : public BlitForWave {
public:
	DeleteForWave();
	virtual ~DeleteForWave() override;

	GBBASIC_CLASS_TYPE('W', 'D', 'E', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual DeleteForWave* with(WaveIndexSetter setWaveIndex_, int index) override;
	using BlitForWave::with;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetName : public Layered::Layered {
public:
	GBBASIC_PROPERTY(std::string, name)

	GBBASIC_PROPERTY(std::string, old)

public:
	SetName();
	virtual ~SetName() override;

	GBBASIC_CLASS_TYPE('S', 'N', 'M', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetName* with(const std::string &txt);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetArtist : public Layered::Layered {
public:
	GBBASIC_PROPERTY(std::string, artist)

	GBBASIC_PROPERTY(std::string, old)

public:
	SetArtist();
	virtual ~SetArtist() override;

	GBBASIC_CLASS_TYPE('S', 'A', 'T', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetArtist* with(const std::string &txt);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetComment : public Layered::Layered {
public:
	GBBASIC_PROPERTY(std::string, comment)

	GBBASIC_PROPERTY(std::string, old)

public:
	SetComment();
	virtual ~SetComment() override;

	GBBASIC_CLASS_TYPE('S', 'C', 'M', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetComment* with(const std::string &txt);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetTicksPerRow : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, ticksPerRow)

	GBBASIC_PROPERTY(int, old)

public:
	SetTicksPerRow();
	virtual ~SetTicksPerRow() override;

	GBBASIC_CLASS_TYPE('S', 'T', 'R', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetTicksPerRow* with(int val);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetDutyInstrument : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(::Music::DutyInstrument, instrument)

	GBBASIC_PROPERTY(::Music::DutyInstrument, old)

public:
	SetDutyInstrument();
	virtual ~SetDutyInstrument() override;

	GBBASIC_CLASS_TYPE('S', 'D', 'I', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetDutyInstrument* with(int idx, const ::Music::DutyInstrument &inst);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetWaveInstrument : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(::Music::WaveInstrument, instrument)

	GBBASIC_PROPERTY(::Music::WaveInstrument, old)

public:
	SetWaveInstrument();
	virtual ~SetWaveInstrument() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'I', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetWaveInstrument* with(int idx, const ::Music::WaveInstrument &inst);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetNoiseInstrument : public Layered::Layered {
public:
	GBBASIC_PROPERTY(int, index)
	GBBASIC_PROPERTY(::Music::NoiseInstrument, instrument)

	GBBASIC_PROPERTY(::Music::NoiseInstrument, old)

public:
	SetNoiseInstrument();
	virtual ~SetNoiseInstrument() override;

	GBBASIC_CLASS_TYPE('S', 'N', 'I', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetNoiseInstrument* with(int idx, const ::Music::NoiseInstrument &inst);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class SetWave : public Layered::Layered {
public:
	typedef std::function<void(int)> WaveIndexSetter;
	typedef std::function<int(void)> WaveIndexGetter;

public:
	GBBASIC_PROPERTY_READONLY(WaveIndexSetter, setWaveIndex)
	GBBASIC_PROPERTY(int, waveIndex)
	GBBASIC_PROPERTY(::Music::Wave, wave)

	GBBASIC_PROPERTY(::Music::Wave, old)

public:
	SetWave();
	virtual ~SetWave() override;

	GBBASIC_CLASS_TYPE('S', 'W', 'V', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual SetWave* with(const ::Music::Wave &wav, WaveIndexSetter setWaveIndex_, WaveIndexGetter getWaveIndex_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Import : public Layered::Layered {
public:
	GBBASIC_PROPERTY(::Music::Ptr, music)

	GBBASIC_PROPERTY(int, bytes)
	GBBASIC_PROPERTY(Bytes::Ptr, old)

public:
	Import();
	virtual ~Import() override;

	GBBASIC_CLASS_TYPE('I', 'M', 'P', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Import* with(const ::Music::Ptr &music_);
	using Layered::Layered::with;

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_MUSIC_H__ */
