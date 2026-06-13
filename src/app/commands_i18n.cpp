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

	// TODO: i18n.

	return this;
}

Command* AddItem::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	// TODO: i18n.

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

	// TODO: i18n.

	return this;
}

Command* DeleteItem::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	// TODO: i18n.

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

AddLanguage::AddLanguage() {
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

	// TODO: i18n.

	return this;
}

Command* AddLanguage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	// TODO: i18n.

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

	// TODO: i18n.

	return this;
}

Command* DeleteLanguage::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	// TODO: i18n.

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

ChangeContent::ChangeContent() {
}

ChangeContent::~ChangeContent() {
}

unsigned ChangeContent::type(void) const {
	return TYPE();
}

const char* ChangeContent::toString(void) const {
	return "Delete language";
}

Command* ChangeContent::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	// TODO: i18n.

	return this;
}

Command* ChangeContent::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	// TODO: i18n.

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
	auto redo_ = [] (::I18n::Ptr i18n, int &bytes, Bytes::Ptr &old, ::I18n::Ptr i18n_) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			rapidjson::Document doc;
			i18n_->toJson(doc);
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
		i18n->toJson(doc);
		i18n_->fromJson(doc);
	};

	Layered::Layered::redo(obj, argc, argv);

	// TODO: i18n.

	return this;
}

Command* Import::undo(Object::Ptr obj, int argc, const Variant* argv) {
	auto undo_ = [] (::I18n::Ptr i18n, int bytes, Bytes::Ptr old, ::I18n::Ptr i18n_) -> void {
		(void)i18n;

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
			i18n_->fromJson(doc);
		} else {
			std::string str;
			old->poke(0);
			old->readString(str);
			rapidjson::Document doc;
			Json::fromString(doc, str.c_str());
			i18n_->fromJson(doc);
		}
	};

	Layered::Layered::undo(obj, argc, argv);

	// TODO: i18n.

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
