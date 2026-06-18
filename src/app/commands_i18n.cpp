/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_i18n.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** I18n commands
*/

namespace Commands {

namespace I18n {

AddItem::AddItem() {
	index(0);
}

AddItem::~AddItem() {
}

unsigned AddItem::type(void) const {
	return TYPE();
}

const char* AddItem::toString(void) const {
	return "Add item";
}

Command* AddItem::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->addItem(index(), item().empty() ? nullptr : item().c_str());

	return this;
}

Command* AddItem::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->deleteItem(index());

	return this;
}

AddItem* AddItem::with(int index_) {
	index(index_);

	return this;
}

AddItem* AddItem::with(int index_, const std::string &item_) {
	index(index_);
	item(item_);

	return this;
}

Command* AddItem::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddItem::create(void) {
	AddItem* result = new AddItem();

	return result;
}

void AddItem::destroy(Command* ptr) {
	AddItem* impl = static_cast<AddItem*>(ptr);
	delete impl;
}

DeleteItem::DeleteItem() {
	index(0);

	filled(false);
}

DeleteItem::~DeleteItem() {
}

unsigned DeleteItem::type(void) const {
	return TYPE();
}

const char* DeleteItem::toString(void) const {
	return "Delete item";
}

Command* DeleteItem::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	if (filled()) {
		for (int l = 0; l < ptr->languageCount(); ++l) {
			const char* val = ptr->getContent(l, index());
			old().push_back(val ? val : "");
		}

		filled(true);
	}
	ptr->deleteItem(index());

	return this;
}

Command* DeleteItem::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->addItem(index(), nullptr);
	for (int l = 0; l < (int)old().size(); ++l)
		ptr->setContent(l, index(), old()[l]);

	return this;
}

DeleteItem* DeleteItem::with(int index_) {
	index(index_);

	return this;
}

Command* DeleteItem::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* DeleteItem::create(void) {
	DeleteItem* result = new DeleteItem();

	return result;
}

void DeleteItem::destroy(Command* ptr) {
	DeleteItem* impl = static_cast<DeleteItem*>(ptr);
	delete impl;
}

SwapItems::SwapItems() {
	index0(0);
	index1(0);
}

SwapItems::~SwapItems() {
}

unsigned SwapItems::type(void) const {
	return TYPE();
}

const char* SwapItems::toString(void) const {
	return "Swap items";
}

Command* SwapItems::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->swapItems(index0(), index1());

	return this;
}

Command* SwapItems::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->swapItems(index0(), index1());

	return this;
}

SwapItems* SwapItems::with(int index0_, int index1_) {
	index0(index0_);
	index1(index1_);

	return this;
}

Command* SwapItems::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SwapItems::create(void) {
	SwapItems* result = new SwapItems();

	return result;
}

void SwapItems::destroy(Command* ptr) {
	SwapItems* impl = static_cast<SwapItems*>(ptr);
	delete impl;
}

AddLanguage::AddLanguage() {
	index(0);
}

AddLanguage::~AddLanguage() {
}

unsigned AddLanguage::type(void) const {
	return TYPE();
}

const char* AddLanguage::toString(void) const {
	return "Add language";
}

Command* AddLanguage::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->addLanguage(index(), language());

	return this;
}

Command* AddLanguage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->deleteLanguage(index());

	return this;
}

AddLanguage* AddLanguage::with(int index_, const std::string &lang) {
	index(index_);
	language(lang);

	return this;
}

Command* AddLanguage::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddLanguage::create(void) {
	AddLanguage* result = new AddLanguage();

	return result;
}

void AddLanguage::destroy(Command* ptr) {
	AddLanguage* impl = static_cast<AddLanguage*>(ptr);
	delete impl;
}

DeleteLanguage::DeleteLanguage() {
	index(0);

	filled(false);
}

DeleteLanguage::~DeleteLanguage() {
}

unsigned DeleteLanguage::type(void) const {
	return TYPE();
}

const char* DeleteLanguage::toString(void) const {
	return "Delete language";
}

Command* DeleteLanguage::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	if (!filled()) {
		std::string lang;
		if (ptr->getLanguage(index(), lang)) {
			oldLanguage(lang);
		}

		oldColumn().clear();
		for (int i = 0; i < ptr->itemCount(); ++i) {
			const char* val = ptr->getContent(index(), i);
			oldColumn().push_back(val ? val : "");
		}

		filled(true);
	}
	ptr->deleteLanguage(index());

	return this;
}

Command* DeleteLanguage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->addLanguage(index(), oldLanguage());
	for (int i = 0; i < (int)oldColumn().size(); ++i)
		ptr->setContent(index(), i, oldColumn()[i]);

	return this;
}

DeleteLanguage* DeleteLanguage::with(int index_) {
	index(index_);

	return this;
}

Command* DeleteLanguage::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* DeleteLanguage::create(void) {
	DeleteLanguage* result = new DeleteLanguage();

	return result;
}

void DeleteLanguage::destroy(Command* ptr) {
	DeleteLanguage* impl = static_cast<DeleteLanguage*>(ptr);
	delete impl;
}

SwapLanguages::SwapLanguages() {
	index0(0);
	index1(0);
}

SwapLanguages::~SwapLanguages() {
}

unsigned SwapLanguages::type(void) const {
	return TYPE();
}

const char* SwapLanguages::toString(void) const {
	return "Swap languages";
}

Command* SwapLanguages::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->swapLanguages(index0(), index1());

	return this;
}

Command* SwapLanguages::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->swapLanguages(index0(), index1());

	return this;
}

SwapLanguages* SwapLanguages::with(int index0_, int index1_) {
	index0(index0_);
	index1(index1_);

	return this;
}

Command* SwapLanguages::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SwapLanguages::create(void) {
	SwapLanguages* result = new SwapLanguages();

	return result;
}

void SwapLanguages::destroy(Command* ptr) {
	SwapLanguages* impl = static_cast<SwapLanguages*>(ptr);
	delete impl;
}

RenameLanguage::RenameLanguage() {
	index(0);

	filled(false);
}

RenameLanguage::~RenameLanguage() {
}

unsigned RenameLanguage::type(void) const {
	return TYPE();
}

const char* RenameLanguage::toString(void) const {
	return "Rename language";
}

Command* RenameLanguage::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	if (!filled()) {
		std::string lang;
		if (ptr->getLanguage(index(), lang))
			old(lang);
		filled(true);
	}
	ptr->setLanguage(index(), language());

	return this;
}

Command* RenameLanguage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->setLanguage(index(), old());

	return this;
}

RenameLanguage* RenameLanguage::with(int index_, const std::string &lang) {
	index(index_);
	language(lang);

	return this;
}

Command* RenameLanguage::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* RenameLanguage::create(void) {
	RenameLanguage* result = new RenameLanguage();

	return result;
}

void RenameLanguage::destroy(Command* ptr) {
	RenameLanguage* impl = static_cast<RenameLanguage*>(ptr);
	delete impl;
}

ChangeContent::ChangeContent() {
	index(0);
	language(0);

	filled(false);
}

ChangeContent::~ChangeContent() {
}

unsigned ChangeContent::type(void) const {
	return TYPE();
}

const char* ChangeContent::toString(void) const {
	return "Change content";
}

Command* ChangeContent::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	if (!filled()) {
		const char* val = ptr->getContent(language(), index());
		old(val ? val : "");
		filled(true);
	}
	ptr->setContent(language(), index(), content());

	return this;
}

Command* ChangeContent::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	ptr->setContent(language(), index(), old());

	return this;
}

ChangeContent* ChangeContent::with(int lang, int index_, const std::string &newVal_) {
	language(lang);
	index(index_);
	content(newVal_);

	return this;
}

Command* ChangeContent::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ChangeContent::create(void) {
	ChangeContent* result = new ChangeContent();

	return result;
}

void ChangeContent::destroy(Command* ptr) {
	ChangeContent* impl = static_cast<ChangeContent*>(ptr);
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
	::Image::Ptr ptr = Object::as<::Image::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	TilesAssets::Entry* entry = (TilesAssets::Entry*)(arg0);

	old(entry->name);
	entry->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Image::Ptr ptr = Object::as<::Image::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	TilesAssets::Entry* entry = (TilesAssets::Entry*)(arg0);

	entry->name = old();

	return this;
}

SetName* SetName::with(const std::string &n) {
	name(n);

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
	Layered::Layered::redo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	if (!old()) {
		old(Bytes::Ptr(Bytes::create()));

		Bytes::Ptr tmp(Bytes::create());
		rapidjson::Document doc;
		ptr->toJson(doc);
		std::string str;
		Json::toString(doc, str, true);
		tmp->writeString(str);
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
	rapidjson::Document doc;
	i18n()->toJson(doc);
	ptr->fromJson(doc);

	return this;
}

Command* Import::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	::I18n::Ptr ptr = Object::as<::I18n::Ptr>(obj);

	if (bytes()) {
		Bytes::Ptr tmp(Bytes::create());
		tmp->resize(bytes());
		const int n = LZ4_decompress_safe(
			(const char*)old()->pointer(), (char*)tmp->pointer(),
			(int)old()->count(), (int)tmp->count()
		);
		(void)n;
		GBBASIC_ASSERT(n == (int)bytes());
		std::string str;
		tmp->readString(str);
		rapidjson::Document doc;
		Json::fromString(doc, str.c_str());
		ptr->fromJson(doc);
	} else {
		std::string str;
		old()->poke(0);
		old()->readString(str);
		rapidjson::Document doc;
		Json::fromString(doc, str.c_str());
		ptr->fromJson(doc);
	}

	return this;
}

Import* Import::with(const ::I18n::Ptr &i18n_) {
	i18n(i18n_);

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
