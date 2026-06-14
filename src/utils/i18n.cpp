/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "i18n.h"

/*
** {===========================================================================
** I18n
*/

class I18nImpl : public I18n {
private:
	// TODO: i18n.

public:
	I18nImpl() {
		// TODO: i18n.
	}
	virtual ~I18nImpl() override {
		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(I18n** ptr, bool /* represented */) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		I18nImpl* result = static_cast<I18nImpl*>(I18n::create());
		// TODO: i18n.

		*ptr = result;

		return true;
	}
	virtual bool clone(I18n** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		I18n* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		// TODO: i18n.

		return result;
	}
	virtual int compare(const I18n* other) const override {
		if (this == other)
			return 0;

		if (!other)
			return 1;

		// TODO: i18n.

		return 0;
	}

	virtual void* pointer(void) override {
		return nullptr;
	}

	virtual int languageCount(void) const override {
		// TODO: i18n.

		return 0;
	}
	virtual int itemCount(void) const override {
		// TODO: i18n.

		return 0;
	}

	virtual bool addLanguage(int index) override {
		// TODO: i18n.
		(void)index;

		return false;
	}
	virtual bool deleteLanguage(int index) override {
		// TODO: i18n.
		(void)index;

		return false;
	}
	virtual bool addItem(int index) override {
		// TODO: i18n.
		(void)index;

		return false;
	}
	virtual bool deleteItem(int index) override {
		// TODO: i18n.
		(void)index;

		return false;
	}

	virtual const char* get(int lang, int item) const override {
		// TODO: i18n.
		(void)lang;
		(void)item;

		return nullptr;
	}
	virtual bool set(int lang, int item, const std::string &val) override {
		// TODO: i18n.
		(void)lang;
		(void)item;
		(void)val;

		return false;
	}

	virtual bool fromBlank(void) override {
		// TODO: i18n.

		return false;
	}

	virtual bool toCsv(std::string &val) const override {
		val.clear();

		// TODO: i18n.

		return true;
	}
	virtual bool fromCsv(const std::string &val) override {
		clear();

		if (val.empty())
			return false;

		// TODO: i18n.

		return true;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		// TODO: i18n.
		(void)doc;

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		clear();

		if (!val.IsObject())
			return false;

		// TODO: i18n.

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	void clear(void) {
		// TODO: i18n.
	}
};

I18n* I18n::create(void) {
	I18nImpl* result = new I18nImpl();

	return result;
}

void I18n::destroy(I18n* ptr) {
	I18nImpl* impl = static_cast<I18nImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
