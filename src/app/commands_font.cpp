/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_font.h"
#include "../../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Font commands
*/

namespace Commands {

namespace Font {

SetContent::SetContent() {
	bytes(0);
}

SetContent::~SetContent() {
}

unsigned SetContent::type(void) const {
	return TYPE();
}

const char* SetContent::toString(void) const {
	return "Set content";
}

Command* SetContent::redo(Object::Ptr obj, int argc, const Variant* argv) {
	auto redo_ = [] (int &bytes, Bytes::Ptr &old, std::string &content, const std::string &newContent) -> void {
		if (!old) {
			old = Bytes::Ptr(Bytes::create());

			Bytes::Ptr tmp(Bytes::create());
			tmp->writeString(content);
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
		content = newContent;
	};

	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	redo_(bytes(), old(), entry->content, content());

	return this;
}

Command* SetContent::undo(Object::Ptr obj, int argc, const Variant* argv) {
	auto undo_ = [] (int bytes, Bytes::Ptr old, std::string &content) -> void {
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
			content = str;
		} else {
			std::string str;
			old->poke(0);
			old->readString(str);
			content = str;
		}
	};

	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	undo_(bytes(), old(), entry->content);

	return this;
}

SetContent* SetContent::with(const std::string &content_) {
	content(content_);

	return this;
}

Command* SetContent::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetContent::create(void) {
	SetContent* result = new SetContent();

	return result;
}

void SetContent::destroy(Command* ptr) {
	SetContent* impl = static_cast<SetContent*>(ptr);
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
	::Font::Ptr ptr = Object::as<::Font::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->name);
	entry->name = name();

	return this;
}

Command* SetName::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr ptr = Object::as<::Font::Ptr>(obj);
	(void)ptr;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

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

ResizeInt::ResizeInt() {
	size(0);

	old(0);
}

ResizeInt::~ResizeInt() {
}

unsigned ResizeInt::type(void) const {
	return TYPE();
}

const char* ResizeInt::toString(void) const {
	return "Resize";
}

Command* ResizeInt::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->size.y);
	entry->size.y = size();

	return this;
}

Command* ResizeInt::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->size.y = old();

	return this;
}

ResizeInt* ResizeInt::with(int size_) {
	size(size_);

	return this;
}

Command* ResizeInt::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ResizeInt::create(void) {
	ResizeInt* result = new ResizeInt();

	return result;
}

void ResizeInt::destroy(Command* ptr) {
	ResizeInt* impl = static_cast<ResizeInt*>(ptr);
	delete impl;
}

ResizeVec2::ResizeVec2() {
	size(Math::Vec2i(-1, -1));

	old(Math::Vec2i(-1, -1));
}

ResizeVec2::~ResizeVec2() {
}

unsigned ResizeVec2::type(void) const {
	return TYPE();
}

const char* ResizeVec2::toString(void) const {
	return "Resize";
}

Command* ResizeVec2::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->size);
	entry->size = size();

	return this;
}

Command* ResizeVec2::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->size = old();

	return this;
}

ResizeVec2* ResizeVec2::with(const Math::Vec2i &size_) {
	size(size_);

	return this;
}

Command* ResizeVec2::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ResizeVec2::create(void) {
	ResizeVec2* result = new ResizeVec2();

	return result;
}

void ResizeVec2::destroy(Command* ptr) {
	ResizeVec2* impl = static_cast<ResizeVec2*>(ptr);
	delete impl;
}

ChangeMaxSizeVec2::ChangeMaxSizeVec2() {
	size(Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE));

	old(Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE));
}

ChangeMaxSizeVec2::~ChangeMaxSizeVec2() {
}

unsigned ChangeMaxSizeVec2::type(void) const {
	return TYPE();
}

const char* ChangeMaxSizeVec2::toString(void) const {
	return "Change max size";
}

Command* ChangeMaxSizeVec2::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->maxSize);
	entry->maxSize = size();

	return this;
}

Command* ChangeMaxSizeVec2::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->maxSize = old();

	return this;
}

ChangeMaxSizeVec2* ChangeMaxSizeVec2::with(const Math::Vec2i &size_) {
	size(size_);

	return this;
}

Command* ChangeMaxSizeVec2::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ChangeMaxSizeVec2::create(void) {
	ChangeMaxSizeVec2* result = new ChangeMaxSizeVec2();

	return result;
}

void ChangeMaxSizeVec2::destroy(Command* ptr) {
	ChangeMaxSizeVec2* impl = static_cast<ChangeMaxSizeVec2*>(ptr);
	delete impl;
}

SetTrim::SetTrim() {
	toTrim(true);

	old(true);
}

SetTrim::~SetTrim() {
}

unsigned SetTrim::type(void) const {
	return TYPE();
}

const char* SetTrim::toString(void) const {
	return "Set trim";
}

Command* SetTrim::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->trim);
	entry->trim = toTrim();

	return this;
}

Command* SetTrim::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->trim = old();

	return this;
}

SetTrim* SetTrim::with(bool trim) {
	toTrim(trim);

	return this;
}

Command* SetTrim::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetTrim::create(void) {
	SetTrim* result = new SetTrim();

	return result;
}

void SetTrim::destroy(Command* ptr) {
	SetTrim* impl = static_cast<SetTrim*>(ptr);
	delete impl;
}

SetOffset::SetOffset() {
	offset(0);

	old(0);
}

SetOffset::~SetOffset() {
}

unsigned SetOffset::type(void) const {
	return TYPE();
}

const char* SetOffset::toString(void) const {
	return "Set offset";
}

Command* SetOffset::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->offset);
	entry->offset = offset();

	return this;
}

Command* SetOffset::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->offset = old();

	return this;
}

SetOffset* SetOffset::with(int offset_) {
	offset(offset_);

	return this;
}

Command* SetOffset::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetOffset::create(void) {
	SetOffset* result = new SetOffset();

	return result;
}

void SetOffset::destroy(Command* ptr) {
	SetOffset* impl = static_cast<SetOffset*>(ptr);
	delete impl;
}

Set2Bpp::Set2Bpp() {
	isTwoBitsPerPixel(false);

	old(false);
}

Set2Bpp::~Set2Bpp() {
}

unsigned Set2Bpp::type(void) const {
	return TYPE();
}

const char* Set2Bpp::toString(void) const {
	return "Set 2BPP";
}

Command* Set2Bpp::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->isTwoBitsPerPixel);
	entry->isTwoBitsPerPixel = isTwoBitsPerPixel();

	return this;
}

Command* Set2Bpp::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->isTwoBitsPerPixel = old();

	return this;
}

Set2Bpp* Set2Bpp::with(bool twoBitsPerPixel_) {
	isTwoBitsPerPixel(twoBitsPerPixel_);

	return this;
}

Command* Set2Bpp::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Set2Bpp::create(void) {
	Set2Bpp* result = new Set2Bpp();

	return result;
}

void Set2Bpp::destroy(Command* ptr) {
	Set2Bpp* impl = static_cast<Set2Bpp*>(ptr);
	delete impl;
}

SetEffect::SetEffect() {
	enabledEffect(0);

	old(0);
}

SetEffect::~SetEffect() {
}

unsigned SetEffect::type(void) const {
	return TYPE();
}

const char* SetEffect::toString(void) const {
	return "Set effect";
}

Command* SetEffect::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->enabledEffect);
	entry->enabledEffect = enabledEffect();

	return this;
}

Command* SetEffect::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->enabledEffect = old();

	return this;
}

SetEffect* SetEffect::with(int enabledEffect_) {
	enabledEffect(enabledEffect_);

	return this;
}

Command* SetEffect::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetEffect::create(void) {
	SetEffect* result = new SetEffect();

	return result;
}

void SetEffect::destroy(Command* ptr) {
	SetEffect* impl = static_cast<SetEffect*>(ptr);
	delete impl;
}

SetShadowParameters::SetShadowParameters() {
	offset(0);
	strength(0.0f);
	direction(0);

	oldOffset(0);
	oldStrength(0.0f);
	oldDirection(0);
}

SetShadowParameters::~SetShadowParameters() {
}

unsigned SetShadowParameters::type(void) const {
	return TYPE();
}

const char* SetShadowParameters::toString(void) const {
	return "Set shadow parameters";
}

Command* SetShadowParameters::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	oldOffset(entry->shadowOffset);
	entry->shadowOffset = offset();
	oldStrength(entry->shadowStrength);
	entry->shadowStrength = strength();
	oldDirection(entry->shadowDirection);
	entry->shadowDirection = direction();

	return this;
}

Command* SetShadowParameters::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->shadowOffset = oldOffset();
	entry->shadowStrength = oldStrength();
	entry->shadowDirection = oldDirection();

	return this;
}

SetShadowParameters* SetShadowParameters::with(int offset_, float strength_, UInt8 direction_) {
	offset(offset_);
	strength(strength_);
	direction(direction_);

	return this;
}

Command* SetShadowParameters::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetShadowParameters::create(void) {
	SetShadowParameters* result = new SetShadowParameters();

	return result;
}

void SetShadowParameters::destroy(Command* ptr) {
	SetShadowParameters* impl = static_cast<SetShadowParameters*>(ptr);
	delete impl;
}

SetOutlineParameters::SetOutlineParameters() {
	offset(0);
	strength(0.0f);

	oldOffset(0);
	oldStrength(0.0f);
}

SetOutlineParameters::~SetOutlineParameters() {
}

unsigned SetOutlineParameters::type(void) const {
	return TYPE();
}

const char* SetOutlineParameters::toString(void) const {
	return "Set outline parameters";
}

Command* SetOutlineParameters::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	oldOffset(entry->outlineOffset);
	entry->outlineOffset = offset();
	oldStrength(entry->outlineStrength);
	entry->outlineStrength = strength();

	return this;
}

Command* SetOutlineParameters::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->outlineOffset = oldOffset();
	entry->outlineStrength = oldStrength();

	return this;
}

SetOutlineParameters* SetOutlineParameters::with(int offset_, float strength_) {
	offset(offset_);
	strength(strength_);

	return this;
}

Command* SetOutlineParameters::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetOutlineParameters::create(void) {
	SetOutlineParameters* result = new SetOutlineParameters();

	return result;
}

void SetOutlineParameters::destroy(Command* ptr) {
	SetOutlineParameters* impl = static_cast<SetOutlineParameters*>(ptr);
	delete impl;
}

SetWordWrap::SetWordWrap() {
	preferFullWord(false);
	preferFullWordForNonAscii(false);

	oldPreferFullWord(false);
	oldPreferFullWordForNonAscii(false);
}

SetWordWrap::~SetWordWrap() {
}

unsigned SetWordWrap::type(void) const {
	return TYPE();
}

const char* SetWordWrap::toString(void) const {
	return "Set word wrap";
}

Command* SetWordWrap::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	oldPreferFullWord(entry->preferFullWord);
	oldPreferFullWordForNonAscii(entry->preferFullWordForNonAscii);
	entry->preferFullWord = preferFullWord();
	entry->preferFullWordForNonAscii = preferFullWordForNonAscii();

	return this;
}

Command* SetWordWrap::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->preferFullWord = oldPreferFullWord();
	entry->preferFullWordForNonAscii = oldPreferFullWordForNonAscii();

	return this;
}

SetWordWrap* SetWordWrap::with(bool preferFullWord_, bool preferFullWordForNonAscii_) {
	preferFullWord(preferFullWord_);
	preferFullWordForNonAscii(preferFullWordForNonAscii_);

	return this;
}

Command* SetWordWrap::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetWordWrap::create(void) {
	SetWordWrap* result = new SetWordWrap();

	return result;
}

void SetWordWrap::destroy(Command* ptr) {
	SetWordWrap* impl = static_cast<SetWordWrap*>(ptr);
	delete impl;
}

SetThresholds::SetThresholds() {
}

SetThresholds::~SetThresholds() {
}

unsigned SetThresholds::type(void) const {
	return TYPE();
}

const char* SetThresholds::toString(void) const {
	return "Set thresholds";
}

Command* SetThresholds::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(
		Math::Vec4i(
			entry->thresholds[0],
			entry->thresholds[1],
			entry->thresholds[2],
			entry->thresholds[3]
		)
	);
	entry->thresholds[0] = (int)thresholds().x;
	entry->thresholds[1] = (int)thresholds().y;
	entry->thresholds[2] = (int)thresholds().z;
	entry->thresholds[3] = (int)thresholds().w;

	return this;
}

Command* SetThresholds::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->thresholds[0] = (int)old().x;
	entry->thresholds[1] = (int)old().y;
	entry->thresholds[2] = (int)old().z;
	entry->thresholds[3] = (int)old().w;

	return this;
}

SetThresholds* SetThresholds::with(Math::Vec4i thresholds_) {
	thresholds(thresholds_);

	return this;
}

Command* SetThresholds::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetThresholds::create(void) {
	SetThresholds* result = new SetThresholds();

	return result;
}

void SetThresholds::destroy(Command* ptr) {
	SetThresholds* impl = static_cast<SetThresholds*>(ptr);
	delete impl;
}

Invert::Invert() {
	inverted(false);

	old(false);
}

Invert::~Invert() {
}

unsigned Invert::type(void) const {
	return TYPE();
}

const char* Invert::toString(void) const {
	return "Invert";
}

Command* Invert::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->inverted);
	entry->inverted = inverted();

	return this;
}

Command* Invert::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->inverted = old();

	return this;
}

Invert* Invert::with(bool inverted_) {
	inverted(inverted_);

	return this;
}

Command* Invert::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Invert::create(void) {
	Invert* result = new Invert();

	return result;
}

void Invert::destroy(Command* ptr) {
	Invert* impl = static_cast<Invert*>(ptr);
	delete impl;
}

SetArbitrary::SetArbitrary() {
}

SetArbitrary::~SetArbitrary() {
}

unsigned SetArbitrary::type(void) const {
	return TYPE();
}

const char* SetArbitrary::toString(void) const {
	return "Set arbitrary";
}

Command* SetArbitrary::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	old(entry->arbitrary);
	entry->arbitrary = arbitrary();

	return this;
}

Command* SetArbitrary::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Font::Ptr font = Object::as<::Font::Ptr>(obj);
	(void)font;
	void* arg0 = unpack<void*>(argc, argv, 0, nullptr);
	FontAssets::Entry* entry = (FontAssets::Entry*)(arg0);

	entry->arbitrary = old();

	return this;
}

SetArbitrary* SetArbitrary::with(const ::Font::Codepoints &arbitrary_) {
	arbitrary(arbitrary_);

	return this;
}

Command* SetArbitrary::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* SetArbitrary::create(void) {
	SetArbitrary* result = new SetArbitrary();

	return result;
}

void SetArbitrary::destroy(Command* ptr) {
	SetArbitrary* impl = static_cast<SetArbitrary*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
