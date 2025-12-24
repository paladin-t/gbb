/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_sfx.h"
#include "editor_sfx.h"
#include "project.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Sfx commands
*/

namespace Commands {

namespace Sfx {

AddPage::AddPage() {
	index(0);

	filled(false);
}

AddPage::~AddPage() {
}

unsigned AddPage::type(void) const {
	return TYPE();
}

const char* AddPage::toString(void) const {
	return "Add page";
}

Command* AddPage::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	if (filled()) {
		SfxAssets &assets_ = prj->assets()->sfx;
		const bool ret = assets_.insert(SfxAssets::Entry(sound()), index());
		(void)ret;

		if (prj->activeSfxIndex() == -1)
			prj->activeSfxIndex(index());

		SfxAssets::Entry* entry = assets_.get(index());
		if (entry) {
			if (!entry->editor) {
				entry->editor = EditorSfx::create(window(), renderer(), workspace(), prj);
				entry->editor->open(window(), renderer(), workspace(), prj, index(), (unsigned)(~0), -1);
			}
		}

		for (int i = 0; i < assets_.count(); ++i) {
			SfxAssets::Entry* entry = assets_.get(i);
			if (entry->editor)
				entry->editor->statusInvalidated();
		}
	} else {
		filled(true);
	}

	return this;
}

Command* AddPage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	SfxAssets &assets_ = prj->assets()->sfx;

	SfxAssets::Entry* entry = assets_.get(index());
	if (entry->editor) {
		entry->editor->close(index());
		EditorSfx::destroy((EditorSfx*)entry->editor);
		entry->editor = nullptr;
	}

	const bool ret = assets_.remove(index());
	(void)ret;
	prj->activeSfxIndex(Math::clamp(prj->activeSfxIndex(), 0, assets_.count() - 1));

	return this;
}

AddPage* AddPage::with(Window* wnd, Renderer* rnd, Workspace* ws, int idx, const ::Sfx::Sound &snd) {
	window(wnd);
	renderer(rnd);
	workspace(ws);
	index(idx);
	sound(snd);

	return this;
}

Command* AddPage::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddPage::create(void) {
	AddPage* result = new AddPage();

	return result;
}

void AddPage::destroy(Command* ptr) {
	AddPage* impl = static_cast<AddPage*>(ptr);
	delete impl;
}

DeletePage::DeletePage() {
	index(0);

	filled(false);
}

DeletePage::~DeletePage() {
}

unsigned DeletePage::type(void) const {
	return TYPE();
}

const char* DeletePage::toString(void) const {
	return "Delete page";
}

Command* DeletePage::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	if (filled()) {
		SfxAssets &assets_ = prj->assets()->sfx;

		SfxAssets::Entry* entry = assets_.get(index());
		if (entry->editor) {
			entry->editor->close(index());
			EditorSfx::destroy((EditorSfx*)entry->editor);
			entry->editor = nullptr;
		}

		const bool ret = assets_.remove(index());
		(void)ret;
		prj->activeSfxIndex(Math::clamp(prj->activeSfxIndex(), 0, assets_.count() - 1));
	} else {
		filled(true);
	}

	return this;
}

Command* DeletePage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	SfxAssets &assets_ = prj->assets()->sfx;
	const bool ret = assets_.insert(SfxAssets::Entry(sound()), index());
	(void)ret;

	if (prj->activeSfxIndex() == -1)
		prj->activeSfxIndex(index());

	SfxAssets::Entry* entry = assets_.get(index());
	if (entry) {
		if (!entry->editor) {
			entry->editor = EditorSfx::create(window(), renderer(), workspace(), prj);
			entry->editor->open(window(), renderer(), workspace(), prj, index(), (unsigned)(~0), -1);
		}
	}

	for (int i = 0; i < assets_.count(); ++i) {
		SfxAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return this;
}

DeletePage* DeletePage::with(Window* wnd, Renderer* rnd, Workspace* ws, int idx, const ::Sfx::Sound &snd, bool filled_) {
	window(wnd);
	renderer(rnd);
	workspace(ws);
	index(idx);
	sound(snd);
	filled(filled_);

	return this;
}

Command* DeletePage::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* DeletePage::create(void) {
	DeletePage* result = new DeletePage();

	return result;
}

void DeletePage::destroy(Command* ptr) {
	DeletePage* impl = static_cast<DeletePage*>(ptr);
	delete impl;
}

DeleteAllPages::DeleteAllPages() {
}

DeleteAllPages::~DeleteAllPages() {
}

unsigned DeleteAllPages::type(void) const {
	return TYPE();
}

const char* DeleteAllPages::toString(void) const {
	return "Delete all pages";
}

Command* DeleteAllPages::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	SfxAssets &assets_ = prj->assets()->sfx;

	for (int i = 0; i < (int)sounds().size(); ++i) {
		SfxAssets::Entry* entry = assets_.get(i);
		if (entry->editor) {
			entry->editor->close(i);
			EditorSfx::destroy((EditorSfx*)entry->editor);
			entry->editor = nullptr;
		}
	}

	assets_.clear();
	prj->activeSfxIndex(-1);

	return this;
}

Command* DeleteAllPages::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	SfxAssets &assets_ = prj->assets()->sfx;
	for (int i = 0; i < (int)sounds().size(); ++i) {
		const bool ret = assets_.add(SfxAssets::Entry(sounds()[i]));
		(void)ret;

		SfxAssets::Entry* entry = assets_.get(i);
		if (entry) {
			if (!entry->editor) {
				entry->editor = EditorSfx::create(window(), renderer(), workspace(), prj);
				entry->editor->open(window(), renderer(), workspace(), prj, i, (unsigned)(~0), -1);
			}
		}
	}
	prj->activeSfxIndex(sub());

	for (int i = 0; i < assets_.count(); ++i) {
		SfxAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return this;
}

DeleteAllPages* DeleteAllPages::with(Window* wnd, Renderer* rnd, Workspace* ws, const ::Sfx::Sound::Array &snds) {
	window(wnd);
	renderer(rnd);
	workspace(ws);
	sounds(snds);

	return this;
}

Command* DeleteAllPages::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* DeleteAllPages::create(void) {
	DeleteAllPages* result = new DeleteAllPages();

	return result;
}

void DeleteAllPages::destroy(Command* ptr) {
	DeleteAllPages* impl = static_cast<DeleteAllPages*>(ptr);
	delete impl;
}

MoveBackward::MoveBackward() {
	index(0);
}

MoveBackward::~MoveBackward() {
}

unsigned MoveBackward::type(void) const {
	return TYPE();
}

const char* MoveBackward::toString(void) const {
	return "Move backward";
}

Command* MoveBackward::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	GBBASIC_ASSERT(index() >= 1 && index() < prj->sfxPageCount() && "Wrong data.");

	const bool ret = prj->swapSfxPages(index(), index() - 1);
	(void)ret;
	GBBASIC_ASSERT(ret && "Impossible.");

	if (setSub())
		setSub()(index() - 1);

	return this;
}

Command* MoveBackward::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	GBBASIC_ASSERT(index() >= 1 && index() < prj->sfxPageCount() && "Wrong data.");

	const bool ret = prj->swapSfxPages(index() - 1, index());
	(void)ret;
	GBBASIC_ASSERT(ret && "Impossible.");

	if (setSub())
		setSub()(index());

	return this;
}

MoveBackward* MoveBackward::with(int index_) {
	index(index_);

	return this;
}

Command* MoveBackward::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* MoveBackward::create(void) {
	MoveBackward* result = new MoveBackward();

	return result;
}

void MoveBackward::destroy(Command* ptr) {
	MoveBackward* impl = static_cast<MoveBackward*>(ptr);
	delete impl;
}

MoveForward::MoveForward() {
	index(0);
}

MoveForward::~MoveForward() {
}

unsigned MoveForward::type(void) const {
	return TYPE();
}

const char* MoveForward::toString(void) const {
	return "Move forward";
}

Command* MoveForward::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	GBBASIC_ASSERT(index() >= 0 && index() < prj->sfxPageCount() - 1 && "Wrong data.");

	const bool ret = prj->swapSfxPages(index(), index() + 1);
	(void)ret;
	GBBASIC_ASSERT(ret && "Impossible.");

	if (setSub())
		setSub()(index() + 1);

	return this;
}

Command* MoveForward::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	GBBASIC_ASSERT(index() >= 0 && index() < prj->sfxPageCount() - 1 && "Wrong data.");

	const bool ret = prj->swapSfxPages(index() + 1, index());
	(void)ret;
	GBBASIC_ASSERT(ret && "Impossible.");

	if (setSub())
		setSub()(index());

	return this;
}

MoveForward* MoveForward::with(int index_) {
	index(index_);

	return this;
}

Command* MoveForward::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* MoveForward::create(void) {
	MoveForward* result = new MoveForward();

	return result;
}

void MoveForward::destroy(Command* ptr) {
	MoveForward* impl = static_cast<MoveForward*>(ptr);
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

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);

	old(ptr->pointer()->name);
	ptr->pointer()->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);

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

SetSound::SetSound() {
}

SetSound::~SetSound() {
}

unsigned SetSound::type(void) const {
	return TYPE();
}

const char* SetSound::toString(void) const {
	return "Set sound";
}

Command* SetSound::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);

	old(*ptr->pointer());
	*ptr->pointer() = wave();

	return this;
}

Command* SetSound::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);

	*ptr->pointer() = old();

	return this;
}

SetSound* SetSound::with(const ::Sfx::Sound &wav) {
	wave(wav);

	return this;
}

Command* SetSound::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetSound::create(void) {
	SetSound* result = new SetSound();

	return result;
}

void SetSound::destroy(Command* ptr) {
	SetSound* impl = static_cast<SetSound*>(ptr);
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
	auto redo_ = [] (::Sfx::Ptr sfx, int &bytes, Bytes::Ptr &old, ::Sfx::Ptr sfx_) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			rapidjson::Document doc;
			sfx_->toJson(doc);
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
		sfx->toJson(doc);
		sfx_->fromJson(doc);
	};

	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr sfx_ = Object::as<::Sfx::Ptr>(obj);

	if (sfx())
		redo_(sfx(), bytes(), old(), sfx_);

	return this;
}

Command* Import::undo(Object::Ptr obj, int argc, const Variant* argv) {
	auto undo_ = [] (::Sfx::Ptr sfx, int bytes, Bytes::Ptr old, ::Sfx::Ptr sfx_) -> void {
		(void)sfx;

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
			sfx_->fromJson(doc);
		} else {
			std::string str;
			old->poke(0);
			old->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			sfx_->fromJson(doc);
		}
	};

	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr sfx_ = Object::as<::Sfx::Ptr>(obj);

	if (sfx())
		undo_(sfx(), bytes(), old(), sfx_);

	return this;
}

Import* Import::with(const ::Sfx::Ptr &sfx_) {
	sfx(sfx_);

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

ImportAsNew::ImportAsNew() {
	index(0);
}

ImportAsNew::~ImportAsNew() {
}

unsigned ImportAsNew::type(void) const {
	return TYPE();
}

const char* ImportAsNew::toString(void) const {
	return "Import as new";
}

Command* ImportAsNew::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	if (sfx()) {
		const ::Sfx::Sound* snd = sfx()->pointer();

		SfxAssets &assets_ = prj->assets()->sfx;
		const bool ret = assets_.add(SfxAssets::Entry(*snd));
		(void)ret;

		SfxAssets::Entry* entry = assets_.get(index());
		if (entry) {
			if (!entry->editor) {
				entry->editor = EditorSfx::create(window(), renderer(), workspace(), prj);
				entry->editor->open(window(), renderer(), workspace(), prj, index(), (unsigned)(~0), -1);
			}
		}

		for (int i = 0; i < assets_.count(); ++i) {
			SfxAssets::Entry* entry = assets_.get(i);
			if (entry->editor)
				entry->editor->statusInvalidated();
		}
	}

	return this;
}

Command* ImportAsNew::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::Sfx::Ptr ptr = Object::as<::Sfx::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	Project* prj = (Project*)(arg0);

	SfxAssets &assets_ = prj->assets()->sfx;

	SfxAssets::Entry* entry = assets_.get(index());
	if (entry->editor) {
		entry->editor->close(index());
		EditorSfx::destroy((EditorSfx*)entry->editor);
		entry->editor = nullptr;
	}

	const bool ret = assets_.remove(index());
	(void)ret;
	prj->activeSfxIndex(Math::clamp(prj->activeSfxIndex(), 0, assets_.count() - 1));

	return this;
}

ImportAsNew* ImportAsNew::with(Window* wnd, Renderer* rnd, Workspace* ws, int idx, const ::Sfx::Ptr &sfx_) {
	window(wnd);
	renderer(rnd);
	workspace(ws);
	index(idx);
	sfx(sfx_);

	return this;
}

Command* ImportAsNew::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ImportAsNew::create(void) {
	ImportAsNew* result = new ImportAsNew();

	return result;
}

void ImportAsNew::destroy(Command* ptr) {
	ImportAsNew* impl = static_cast<ImportAsNew*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
