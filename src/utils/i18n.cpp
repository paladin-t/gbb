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
	// TODO

public:
	I18nImpl() {
		// TODO
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
		// TODO

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

	virtual void* pointer(void) override {
		return nullptr;
	}

	// TODO

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		// TODO
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

		// TODO

		return true;
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}

private:
	void clear(void) {
		// TODO
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
