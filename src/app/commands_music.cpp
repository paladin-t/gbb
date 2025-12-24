/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_music.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Music commands
*/

namespace Commands {

namespace Music {

Playable::Playable() {
	sequenceIndex(0);
	channelIndex(0);
	lineIndex(0);
	note(___);
	instrument(0);
	effectCode(0);
	effectParameters(0);
}

Command* Playable::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	if (setView())
		setView()(sequenceIndex(), channelIndex(), lineIndex());

	return this;
}

Command* Playable::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	if (setView())
		setView()(sequenceIndex(), channelIndex(), lineIndex());

	return this;
}

SetNote::SetNote() {
	filled(false);
	old(___);
}

SetNote::~SetNote() {
}

unsigned SetNote::type(void) const {
	return TYPE();
}

const char* SetNote::toString(void) const {
	return "Set note";
}

Command* SetNote::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	if (!filled()) {
		old(cellptr->note);
		filled(true);
	}
	cellptr->note = note();

	return this;
}

Command* SetNote::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	cellptr->note = old();

	return this;
}

SetNote* SetNote::with(Getter get_) {
	get(get_);

	return this;
}

SetNote* SetNote::with(ViewSetter set, int sequence, int channel, int line) {
	setView(set);
	sequenceIndex(sequence);
	channelIndex(channel);
	lineIndex(line);

	return this;
}

SetNote* SetNote::with(int note_) {
	note(note_);

	return this;
}

Command* SetNote::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetNote::create(void) {
	SetNote* result = new SetNote();

	return result;
}

void SetNote::destroy(Command* ptr) {
	SetNote* impl = static_cast<SetNote*>(ptr);
	delete impl;
}

SetInstrument::SetInstrument() {
	filled(false);
	old(0);
}

SetInstrument::~SetInstrument() {
}

unsigned SetInstrument::type(void) const {
	return TYPE();
}

const char* SetInstrument::toString(void) const {
	return "Set instrument";
}

Command* SetInstrument::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	if (!filled()) {
		old(cellptr->instrument);
		filled(true);
	}
	cellptr->instrument = instrument();

	return this;
}

Command* SetInstrument::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	cellptr->instrument = old();

	return this;
}

SetInstrument* SetInstrument::with(Getter get_) {
	get(get_);

	return this;
}

SetInstrument* SetInstrument::with(ViewSetter set, int sequence, int channel, int line) {
	setView(set);
	sequenceIndex(sequence);
	channelIndex(channel);
	lineIndex(line);

	return this;
}

SetInstrument* SetInstrument::with(int inst) {
	instrument(inst);

	return this;
}

Command* SetInstrument::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetInstrument::create(void) {
	SetInstrument* result = new SetInstrument();

	return result;
}

void SetInstrument::destroy(Command* ptr) {
	SetInstrument* impl = static_cast<SetInstrument*>(ptr);
	delete impl;
}

SetEffectCode::SetEffectCode() {
	filled(false);
	old(0);
}

SetEffectCode::~SetEffectCode() {
}

unsigned SetEffectCode::type(void) const {
	return TYPE();
}

const char* SetEffectCode::toString(void) const {
	return "Set effect code";
}

Command* SetEffectCode::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	if (!filled()) {
		old(cellptr->effectCode);
		filled(true);
	}
	cellptr->effectCode = effectCode();

	return this;
}

Command* SetEffectCode::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	cellptr->effectCode = old();

	return this;
}

SetEffectCode* SetEffectCode::with(Getter get_) {
	get(get_);

	return this;
}

SetEffectCode* SetEffectCode::with(ViewSetter set, int sequence, int channel, int line) {
	setView(set);
	sequenceIndex(sequence);
	channelIndex(channel);
	lineIndex(line);

	return this;
}

SetEffectCode* SetEffectCode::with(int code) {
	effectCode(code);

	return this;
}

Command* SetEffectCode::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetEffectCode::create(void) {
	SetEffectCode* result = new SetEffectCode();

	return result;
}

void SetEffectCode::destroy(Command* ptr) {
	SetEffectCode* impl = static_cast<SetEffectCode*>(ptr);
	delete impl;
}

SetEffectParameters::SetEffectParameters() {
	filled(false);
	old(0);
}

SetEffectParameters::~SetEffectParameters() {
}

unsigned SetEffectParameters::type(void) const {
	return TYPE();
}

const char* SetEffectParameters::toString(void) const {
	return "Set effect parameters";
}

Command* SetEffectParameters::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	if (!filled()) {
		old(cellptr->effectParameters);
		filled(true);
	}
	cellptr->effectParameters = effectParameters();

	return this;
}

Command* SetEffectParameters::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);
	(void)ptr;
	::Music::Cell BLANK;
	::Music::Cell* cellptr = get()(sequenceIndex(), channelIndex(), lineIndex());
	if (!cellptr)
		cellptr = &BLANK;

	cellptr->effectParameters = old();

	return this;
}

SetEffectParameters* SetEffectParameters::with(Getter get_) {
	get(get_);

	return this;
}

SetEffectParameters* SetEffectParameters::with(ViewSetter set, int sequence, int channel, int line) {
	setView(set);
	sequenceIndex(sequence);
	channelIndex(channel);
	lineIndex(line);

	return this;
}

SetEffectParameters* SetEffectParameters::with(int params) {
	effectParameters(params);

	return this;
}

Command* SetEffectParameters::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetEffectParameters::create(void) {
	SetEffectParameters* result = new SetEffectParameters();

	return result;
}

void SetEffectParameters::destroy(Command* ptr) {
	SetEffectParameters* impl = static_cast<SetEffectParameters*>(ptr);
	delete impl;
}

ChangeOrder::ChangeOrder() {
	bytes(0);
}

ChangeOrder::~ChangeOrder() {
}

unsigned ChangeOrder::type(void) const {
	return TYPE();
}

const char* ChangeOrder::toString(void) const {
	return "Change order";
}

Command* ChangeOrder::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	if (!old()) {
		old(Bytes::Ptr(Bytes::create()));

		Bytes::Ptr tmp(Bytes::create());
		::Music::toBytes(tmp.get(), ptr->pointer()->channels);
		bytes((int)tmp->count());
		int n = LZ4_compressBound((int)tmp->count());
		if (n < (int)tmp->count()) {
			old()->resize((size_t)n);
			n = LZ4_compress_default(
				(const char*)tmp->pointer(), (char*)old()->pointer(),
				(int)tmp->count(), (int)old()->count()
			);
			GBBASIC_ASSERT(n);
			old()->resize((size_t)n);
		} else {
			bytes(0);
			old()->clear();
			old()->writeBytes(tmp.get());
		}
	}
	ptr->pointer()->channels = channels();

	return this;
}

Command* ChangeOrder::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Playable::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	if (bytes()) {
		Bytes::Ptr tmp(Bytes::create());
		tmp->resize(bytes());
		const int n = LZ4_decompress_safe(
			(const char*)old()->pointer(), (char*)tmp->pointer(),
			(int)old()->count(), (int)tmp->count()
		);
		(void)n;
		GBBASIC_ASSERT(n == (int)bytes());
		::Music::fromBytes(tmp.get(), ptr->pointer()->channels);
	} else {
		::Music::fromBytes(old().get(), ptr->pointer()->channels);
	}

	return this;
}

ChangeOrder* ChangeOrder::with(ViewSetter set, int sequence, int channel, int line) {
	setView(set);
	sequenceIndex(sequence);
	channelIndex(channel);
	lineIndex(line);

	return this;
}

ChangeOrder* ChangeOrder::with(const ::Music::Channels &channels_) {
	channels(channels_);

	return this;
}

Command* ChangeOrder::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ChangeOrder::create(void) {
	ChangeOrder* result = new ChangeOrder();

	return result;
}

void ChangeOrder::destroy(Command* ptr) {
	ChangeOrder* impl = static_cast<ChangeOrder*>(ptr);
	delete impl;
}

BlitForTracker::BlitForTracker() {
}

BlitForTracker::~BlitForTracker() {
}

Command* BlitForTracker::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	int k = 0;
	for (int ln = area().min().line; ln <= area().max().line; ++ln) {
		const Tracker::Cursor a(area().min().sequence, area().min().channel, ln, area().min().data);
		const Tracker::Cursor b(area().max().sequence, area().max().channel, ln, area().max().data);
		for (Tracker::Cursor c = a; c <= b && c.line == ln; c = c.nextData()) {
			const int val = values()[k];
			set()(c, val);

			++k;
		}
	}

	return this;
}

Command* BlitForTracker::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	int k = 0;
	for (int ln = area().min().line; ln <= area().max().line; ++ln) {
		const Tracker::Cursor a(area().min().sequence, area().min().channel, ln, area().min().data);
		const Tracker::Cursor b(area().max().sequence, area().max().channel, ln, area().max().data);
		for (Tracker::Cursor c = a; c <= b && c.line == ln; c = c.nextData()) {
			const int val = old()[k];
			set()(c, val);

			++k;
		}
	}

	return this;
}

BlitForTracker* BlitForTracker::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

BlitForTracker* BlitForTracker::with(const Tracker::Range &area_, const DataArray &values_, const Tracker::Range &srcArea) {
	area().first = area().second = area_.first;
	area().first.data = area().second.data = srcArea.first.data;
	const Math::Vec2i size = srcArea.size();
	for (int j = 0; j < size.y - 1 && area().second.nextLine().valid(); ++j)
		area().second = area().second.nextLine();
	const int ln_ = area().second.line;
	for (int i = 0; i < size.x - 1 && area().second.nextData().line == ln_; ++i)
		area().second = area().second.nextData();

	for (int j = 0; j < size.y; ++j) {
		const int ln = area_.min().line + j;
		if (ln >= GBBASIC_MUSIC_PATTERN_COUNT)
			break;

		Tracker::Cursor c(area_.min().sequence, area_.min().channel, ln, srcArea.min().data);
		for (int i = 0; i < size.x && c.line == ln; ++i, c = c.nextData()) {
			int val = 0;

			get()(c, val);
			old().push_back(val);

			const int k = i + j * size.x;
			val = values_[k];
			values().push_back(val);

			set()(c, val);
		}
		old().shrink_to_fit();
		values().shrink_to_fit();
	}

	return this;
}

BlitForTracker* BlitForTracker::with(const Tracker::Range &area_, const DataArray &values_) {
	area(area_);

	int k = 0;
	for (int ln = area().min().line; ln <= area().max().line; ++ln) {
		const Tracker::Cursor a(area().min().sequence, area().min().channel, ln, area().min().data);
		const Tracker::Cursor b(area().max().sequence, area().max().channel, ln, area().max().data);
		for (Tracker::Cursor c = a; c <= b && c.line == ln; c = c.nextData()) {
			int val = 0;

			get()(c, val);
			old().push_back(val);

			val = values_[k];
			values().push_back(val);

			set()(c, val);

			++k;
		}
		old().shrink_to_fit();
		values().shrink_to_fit();
	}

	return this;
}

BlitForTracker* BlitForTracker::with(const Tracker::Range &area_, Getter getValues) {
	area(area_);

	int k = 0;
	for (int ln = area().min().line; ln <= area().max().line; ++ln) {
		const Tracker::Cursor a(area().min().sequence, area().min().channel, ln, area().min().data);
		const Tracker::Cursor b(area().max().sequence, area().max().channel, ln, area().max().data);
		for (Tracker::Cursor c = a; c <= b && c.line == ln; c = c.nextData()) {
			int val = 0;

			get()(c, val);
			old().push_back(val);

			int val_ = 0;
			getValues(c, val_);

			values().push_back(val_);

			set()(c, val_);

			++k;
		}
		old().shrink_to_fit();
		values().shrink_to_fit();
	}

	return this;
}

BlitForTracker* BlitForTracker::with(int sequence, int channel, int line) {
	sequenceIndex(sequence);
	channelIndex(channel);
	lineIndex(line);

	return this;
}

Command* BlitForTracker::exec(Object::Ptr, int, const Variant*) {
	return this;
}

CutForTracker::CutForTracker() {
}

CutForTracker::~CutForTracker() {
}

unsigned CutForTracker::type(void) const {
	return TYPE();
}

const char* CutForTracker::toString(void) const {
	return "Cut";
}

Command* CutForTracker::create(void) {
	CutForTracker* result = new CutForTracker();

	return result;
}

void CutForTracker::destroy(Command* ptr) {
	CutForTracker* impl = static_cast<CutForTracker*>(ptr);
	delete impl;
}

PasteForTracker::PasteForTracker() {
}

PasteForTracker::~PasteForTracker() {
}

unsigned PasteForTracker::type(void) const {
	return TYPE();
}

const char* PasteForTracker::toString(void) const {
	return "Paste";
}

Command* PasteForTracker::create(void) {
	PasteForTracker* result = new PasteForTracker();

	return result;
}

void PasteForTracker::destroy(Command* ptr) {
	PasteForTracker* impl = static_cast<PasteForTracker*>(ptr);
	delete impl;
}

DeleteForTracker::DeleteForTracker() {
}

DeleteForTracker::~DeleteForTracker() {
}

unsigned DeleteForTracker::type(void) const {
	return TYPE();
}

const char* DeleteForTracker::toString(void) const {
	return "Delete";
}

Command* DeleteForTracker::create(void) {
	DeleteForTracker* result = new DeleteForTracker();

	return result;
}

void DeleteForTracker::destroy(Command* ptr) {
	DeleteForTracker* impl = static_cast<DeleteForTracker*>(ptr);
	delete impl;
}

BlitForInstrument::BlitForInstrument() {
	instrumentGroup(0);
	instrumentIndex(0);
}

BlitForInstrument::~BlitForInstrument() {
}

Command* BlitForInstrument::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);
	if (setInstrumentIndex())
		setInstrumentIndex()(instrumentType(), instrumentGroup(), instrumentIndex());

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	::Music::Song* song = ptr->pointer();
	switch (instrumentType()) {
	case ::Music::Instruments::SQUARE: {
			::Music::DutyInstrument &inst = song->dutyInstruments[instrumentIndex()];
			oldDuty(inst);
			const std::string name = inst.name;
			inst = dutyData();
			inst.name = name;
		}

		break;
	case ::Music::Instruments::WAVE: {
			::Music::WaveInstrument &inst = song->waveInstruments[instrumentIndex()];
			oldWave(inst);
			const std::string name = inst.name;
			inst = waveData();
			inst.name = name;
		}

		break;
	case ::Music::Instruments::NOISE: {
			::Music::NoiseInstrument &inst = song->noiseInstruments[instrumentIndex()];
			oldNoise(inst);
			const std::string name = inst.name;
			inst = noiseData();
			inst.name = name;
		}

		break;
	}

	return this;
}

Command* BlitForInstrument::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);
	if (setInstrumentIndex())
		setInstrumentIndex()(instrumentType(), instrumentGroup(), instrumentIndex());

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	::Music::Song* song = ptr->pointer();
	switch (instrumentType()) {
	case ::Music::Instruments::SQUARE: {
			::Music::DutyInstrument &inst = song->dutyInstruments[instrumentIndex()];
			const std::string name = inst.name;
			inst = oldDuty();
			inst.name = name;
		}

		break;
	case ::Music::Instruments::WAVE: {
			::Music::WaveInstrument &inst = song->waveInstruments[instrumentIndex()];
			const std::string name = inst.name;
			inst = oldWave();
			inst.name = name;
		}

		break;
	case ::Music::Instruments::NOISE: {
			::Music::NoiseInstrument &inst = song->noiseInstruments[instrumentIndex()];
			const std::string name = inst.name;
			inst = oldNoise();
			inst.name = name;
		}

		break;
	}

	return this;
}

BlitForInstrument* BlitForInstrument::with(InstrumentIndexSetter setInstrumentIndex_, int group, int idx) {
	setInstrumentIndex(setInstrumentIndex_);
	instrumentGroup(group);
	instrumentIndex(idx);

	return this;
}

BlitForInstrument* BlitForInstrument::with(const ::Music::DutyInstrument &data) {
	instrumentType(::Music::Instruments::SQUARE);
	dutyData(data);

	return this;
}

BlitForInstrument* BlitForInstrument::with(const ::Music::WaveInstrument &data) {
	instrumentType(::Music::Instruments::WAVE);
	waveData(data);

	return this;
}

BlitForInstrument* BlitForInstrument::with(const ::Music::NoiseInstrument &data) {
	instrumentType(::Music::Instruments::NOISE);
	noiseData(data);

	return this;
}

Command* BlitForInstrument::exec(Object::Ptr obj, int argc, const Variant* argv) {
	redo(obj, argc, argv);

	return this;
}

CutForInstrument::CutForInstrument() {
}

CutForInstrument::~CutForInstrument() {
}

unsigned CutForInstrument::type(void) const {
	return TYPE();
}

const char* CutForInstrument::toString(void) const {
	return "Cut instrument";
}

Command* CutForInstrument::create(void) {
	CutForInstrument* result = new CutForInstrument();

	return result;
}

void CutForInstrument::destroy(Command* ptr) {
	CutForInstrument* impl = static_cast<CutForInstrument*>(ptr);
	delete impl;
}

PasteForInstrument::PasteForInstrument() {
}

PasteForInstrument::~PasteForInstrument() {
}

unsigned PasteForInstrument::type(void) const {
	return TYPE();
}

const char* PasteForInstrument::toString(void) const {
	return "Paste instrument";
}

Command* PasteForInstrument::create(void) {
	PasteForInstrument* result = new PasteForInstrument();

	return result;
}

void PasteForInstrument::destroy(Command* ptr) {
	PasteForInstrument* impl = static_cast<PasteForInstrument*>(ptr);
	delete impl;
}

DeleteForInstrument::DeleteForInstrument() {
}

DeleteForInstrument::~DeleteForInstrument() {
}

unsigned DeleteForInstrument::type(void) const {
	return TYPE();
}

const char* DeleteForInstrument::toString(void) const {
	return "Delete instrument";
}

Command* DeleteForInstrument::create(void) {
	DeleteForInstrument* result = new DeleteForInstrument();

	return result;
}

void DeleteForInstrument::destroy(Command* ptr) {
	DeleteForInstrument* impl = static_cast<DeleteForInstrument*>(ptr);
	delete impl;
}

BlitForWave::BlitForWave() {
	waveIndex(0);

	data().fill(0);
	old().fill(0);
}

BlitForWave::~BlitForWave() {
}

Command* BlitForWave::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);
	if (setWaveIndex())
		setWaveIndex()(waveIndex());

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	::Music::Song* song = ptr->pointer();
	::Music::Wave &wave = song->waves[waveIndex()];
	old(wave);
	wave = data();

	return this;
}

Command* BlitForWave::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);
	if (setWaveIndex())
		setWaveIndex()(waveIndex());

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	::Music::Song* song = ptr->pointer();
	::Music::Wave &wave = song->waves[waveIndex()];
	wave = old();

	return this;
}

BlitForWave* BlitForWave::with(WaveIndexSetter setWaveIndex_, WaveIndexGetter getWaveIndex_) {
	setWaveIndex(setWaveIndex_);
	waveIndex(getWaveIndex_());

	return this;
}

Command* BlitForWave::exec(Object::Ptr obj, int argc, const Variant* argv) {
	redo(obj, argc, argv);

	return this;
}

CutForWave::CutForWave() {
	data().fill(0);
	old().fill(0);
}

CutForWave::~CutForWave() {
}

unsigned CutForWave::type(void) const {
	return TYPE();
}

const char* CutForWave::toString(void) const {
	return "Cut";
}

Command* CutForWave::create(void) {
	CutForWave* result = new CutForWave();

	return result;
}

void CutForWave::destroy(Command* ptr) {
	CutForWave* impl = static_cast<CutForWave*>(ptr);
	delete impl;
}

PasteForWave::PasteForWave() {
	data().fill(0);
	old().fill(0);
}

PasteForWave::~PasteForWave() {
}

unsigned PasteForWave::type(void) const {
	return TYPE();
}

const char* PasteForWave::toString(void) const {
	return "Paste";
}

PasteForWave* PasteForWave::with(const ::Music::Wave &data_) {
	data(data_);

	return this;
}

Command* PasteForWave::create(void) {
	PasteForWave* result = new PasteForWave();

	return result;
}

void PasteForWave::destroy(Command* ptr) {
	PasteForWave* impl = static_cast<PasteForWave*>(ptr);
	delete impl;
}

DeleteForWave::DeleteForWave() {
	data().fill(0);
	old().fill(0);
}

DeleteForWave::~DeleteForWave() {
}

unsigned DeleteForWave::type(void) const {
	return TYPE();
}

const char* DeleteForWave::toString(void) const {
	return "Delete";
}

DeleteForWave* DeleteForWave::with(WaveIndexSetter setWaveIndex_, int index) {
	setWaveIndex(setWaveIndex_);
	waveIndex(index);

	data().fill(0);

	return this;
}

Command* DeleteForWave::create(void) {
	DeleteForWave* result = new DeleteForWave();

	return result;
}

void DeleteForWave::destroy(Command* ptr) {
	DeleteForWave* impl = static_cast<DeleteForWave*>(ptr);
	delete impl;
}

SetName::SetName() {
}

SetName::~SetName() {
}

unsigned SetName::type(void) const {
	return TYPE();
}

const char* SetName::toString(void) const {
	return "Set name";
}

Command* SetName::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->name);
	ptr->pointer()->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->name = old();

	return this;
}

SetName* SetName::with(const std::string &txt) {
	name(txt);

	return this;
}

Command* SetName::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetName::create(void) {
	SetName* result = new SetName();

	return result;
}

void SetName::destroy(Command* ptr) {
	SetName* impl = static_cast<SetName*>(ptr);
	delete impl;
}

SetArtist::SetArtist() {
}

SetArtist::~SetArtist() {
}

unsigned SetArtist::type(void) const {
	return TYPE();
}

const char* SetArtist::toString(void) const {
	return "Set artist";
}

Command* SetArtist::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->artist);
	ptr->pointer()->artist = artist();

	return this;
}

Command* SetArtist::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->artist = old();

	return this;
}

SetArtist* SetArtist::with(const std::string &txt) {
	artist(txt);

	return this;
}

Command* SetArtist::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetArtist::create(void) {
	SetArtist* result = new SetArtist();

	return result;
}

void SetArtist::destroy(Command* ptr) {
	SetArtist* impl = static_cast<SetArtist*>(ptr);
	delete impl;
}

SetComment::SetComment() {
}

SetComment::~SetComment() {
}

unsigned SetComment::type(void) const {
	return TYPE();
}

const char* SetComment::toString(void) const {
	return "Set comment";
}

Command* SetComment::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->comment);
	ptr->pointer()->comment = comment();

	return this;
}

Command* SetComment::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->comment = old();

	return this;
}

SetComment* SetComment::with(const std::string &txt) {
	comment(txt);

	return this;
}

Command* SetComment::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetComment::create(void) {
	SetComment* result = new SetComment();

	return result;
}

void SetComment::destroy(Command* ptr) {
	SetComment* impl = static_cast<SetComment*>(ptr);
	delete impl;
}

SetTicksPerRow::SetTicksPerRow() {
}

SetTicksPerRow::~SetTicksPerRow() {
}

unsigned SetTicksPerRow::type(void) const {
	return TYPE();
}

const char* SetTicksPerRow::toString(void) const {
	return "Set ticks per row";
}

Command* SetTicksPerRow::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->ticksPerRow);
	ptr->pointer()->ticksPerRow = ticksPerRow();

	return this;
}

Command* SetTicksPerRow::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->ticksPerRow = old();

	return this;
}

SetTicksPerRow* SetTicksPerRow::with(int val) {
	ticksPerRow(val);

	return this;
}

Command* SetTicksPerRow::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetTicksPerRow::create(void) {
	SetTicksPerRow* result = new SetTicksPerRow();

	return result;
}

void SetTicksPerRow::destroy(Command* ptr) {
	SetTicksPerRow* impl = static_cast<SetTicksPerRow*>(ptr);
	delete impl;
}

SetDutyInstrument::SetDutyInstrument() {
	index(0);
}

SetDutyInstrument::~SetDutyInstrument() {
}

unsigned SetDutyInstrument::type(void) const {
	return TYPE();
}

const char* SetDutyInstrument::toString(void) const {
	return "Set duty instrument";
}

Command* SetDutyInstrument::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->dutyInstruments[index()]);
	ptr->pointer()->dutyInstruments[index()] = instrument();

	return this;
}

Command* SetDutyInstrument::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->dutyInstruments[index()] = old();

	return this;
}

SetDutyInstrument* SetDutyInstrument::with(int idx, const ::Music::DutyInstrument &inst) {
	index(idx);
	instrument(inst);

	return this;
}

Command* SetDutyInstrument::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetDutyInstrument::create(void) {
	SetDutyInstrument* result = new SetDutyInstrument();

	return result;
}

void SetDutyInstrument::destroy(Command* ptr) {
	SetDutyInstrument* impl = static_cast<SetDutyInstrument*>(ptr);
	delete impl;
}

SetWaveInstrument::SetWaveInstrument() {
	index(0);
}

SetWaveInstrument::~SetWaveInstrument() {
}

unsigned SetWaveInstrument::type(void) const {
	return TYPE();
}

const char* SetWaveInstrument::toString(void) const {
	return "Set wave instrument";
}

Command* SetWaveInstrument::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->waveInstruments[index()]);
	ptr->pointer()->waveInstruments[index()] = instrument();

	return this;
}

Command* SetWaveInstrument::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->waveInstruments[index()] = old();

	return this;
}

SetWaveInstrument* SetWaveInstrument::with(int idx, const ::Music::WaveInstrument &inst) {
	index(idx);
	instrument(inst);

	return this;
}

Command* SetWaveInstrument::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetWaveInstrument::create(void) {
	SetWaveInstrument* result = new SetWaveInstrument();

	return result;
}

void SetWaveInstrument::destroy(Command* ptr) {
	SetWaveInstrument* impl = static_cast<SetWaveInstrument*>(ptr);
	delete impl;
}

SetNoiseInstrument::SetNoiseInstrument() {
	index(0);
}

SetNoiseInstrument::~SetNoiseInstrument() {
}

unsigned SetNoiseInstrument::type(void) const {
	return TYPE();
}

const char* SetNoiseInstrument::toString(void) const {
	return "Set noise instrument";
}

Command* SetNoiseInstrument::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->noiseInstruments[index()]);
	ptr->pointer()->noiseInstruments[index()] = instrument();

	return this;
}

Command* SetNoiseInstrument::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->noiseInstruments[index()] = old();

	return this;
}

SetNoiseInstrument* SetNoiseInstrument::with(int idx, const ::Music::NoiseInstrument &inst) {
	index(idx);
	instrument(inst);

	return this;
}

Command* SetNoiseInstrument::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetNoiseInstrument::create(void) {
	SetNoiseInstrument* result = new SetNoiseInstrument();

	return result;
}

void SetNoiseInstrument::destroy(Command* ptr) {
	SetNoiseInstrument* impl = static_cast<SetNoiseInstrument*>(ptr);
	delete impl;
}

SetWave::SetWave() {
	waveIndex(0);
}

SetWave::~SetWave() {
}

unsigned SetWave::type(void) const {
	return TYPE();
}

const char* SetWave::toString(void) const {
	return "Set wave";
}

Command* SetWave::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);
	if (setWaveIndex())
		setWaveIndex()(waveIndex());

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	old(ptr->pointer()->waves[waveIndex()]);
	ptr->pointer()->waves[waveIndex()] = wave();

	return this;
}

Command* SetWave::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);
	if (setWaveIndex())
		setWaveIndex()(waveIndex());

	::Music::Ptr ptr = Object::as<::Music::Ptr>(obj);

	ptr->pointer()->waves[waveIndex()] = old();

	return this;
}

SetWave* SetWave::with(const ::Music::Wave &wav, WaveIndexSetter setWaveIndex_, WaveIndexGetter getWaveIndex_) {
	setWaveIndex(setWaveIndex_);
	waveIndex(getWaveIndex_());
	wave(wav);

	return this;
}

Command* SetWave::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetWave::create(void) {
	SetWave* result = new SetWave();

	return result;
}

void SetWave::destroy(Command* ptr) {
	SetWave* impl = static_cast<SetWave*>(ptr);
	delete impl;
}

Import::Import() {
	bytes(0);
}

Import::~Import() {
}

unsigned Import::type(void) const {
	return TYPE();
}

const char* Import::toString(void) const {
	return "Import";
}

Command* Import::redo(Object::Ptr obj, int argc, const Variant* argv) {
	auto redo_ = [] (::Music::Ptr music, int &bytes, Bytes::Ptr &old, ::Music::Ptr music_) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			rapidjson::Document doc;
			music_->toJson(doc);
			std::string str;
			Json::toString(doc, str, false);
			tmp->writeString(str);
			bytes = (int)tmp->count();
			int n = LZ4_compressBound((int)tmp->count());
			old->resize((size_t)n);
			n = LZ4_compress_default(
				(const char*)tmp->pointer(), (char*)old->pointer(),
				(int)tmp->count(), (int)old->count()
			);
			GBBASIC_ASSERT(n);
			if (n < (int)tmp->count()) {
				old->resize((size_t)n);
			} else {
				bytes = 0;
				old->clear();
				old->writeBytes(tmp.get());
			}
		}
		rapidjson::Document doc;
		music->toJson(doc);
		music_->fromJson(doc);
	};

	Layered::Layered::redo(obj, argc, argv);

	::Music::Ptr music_ = Object::as<::Music::Ptr>(obj);

	if (music())
		redo_(music(), bytes(), old(), music_);

	return this;
}

Command* Import::undo(Object::Ptr obj, int argc, const Variant* argv) {
	auto undo_ = [] (::Music::Ptr music, int bytes, Bytes::Ptr old, ::Music::Ptr music_) -> void {
		(void)music;

		if (bytes) {
			Bytes::Ptr tmp(Bytes::create());
			tmp->resize(bytes);
			const int n = LZ4_decompress_safe(
				(const char*)old->pointer(), (char*)tmp->pointer(),
				(int)old->count(), (int)tmp->count()
			);
			(void)n;
			GBBASIC_ASSERT(n == (int)bytes);
			std::string str;
			tmp->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			music_->fromJson(doc);
		} else {
			std::string str;
			old->poke(0);
			old->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			music_->fromJson(doc);
		}
	};

	Layered::Layered::undo(obj, argc, argv);

	::Music::Ptr music_ = Object::as<::Music::Ptr>(obj);

	if (music())
		undo_(music(), bytes(), old(), music_);

	return this;
}

Import* Import::with(const ::Music::Ptr &music_) {
	music(music_);

	return this;
}

Command* Import::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Import::create(void) {
	Import* result = new Import();

	return result;
}

void Import::destroy(Command* ptr) {
	Import* impl = static_cast<Import*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
