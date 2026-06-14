/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __I18N_H__
#define __I18N_H__

#include "../gbbasic.h"
#include "cloneable.h"
#include "json.h"
#include <map>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef I18N_KEY_COLUMN_NAME
#	define I18N_KEY_COLUMN_NAME "key"
#endif /* I18N_KEY_COLUMN_NAME */
#ifndef I18N_ENGLISH_COLUMN_NAME
#	define I18N_ENGLISH_COLUMN_NAME "english"
#endif /* I18N_ENGLISH_COLUMN_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** I18n
*/

/**
 * @brief I18n resource object.
 */
class I18n : public Cloneable<I18n>, public virtual Object {
public:
	typedef std::shared_ptr<I18n> Ptr;

public:
	GBBASIC_CLASS_TYPE('I', '1', '8', 'N')

	/**
	 * @param[out] ptr
	 */
	virtual bool clone(I18n** ptr, bool represented) const = 0;
	using Cloneable<I18n>::clone;
	using Object::clone;

	virtual size_t hash(void) const = 0;
	virtual int compare(const I18n* other) const = 0;

	/**
	 * @return Items.
	 */
	virtual void* pointer(void) = 0;

	virtual int languageCount(void) const = 0;
	virtual bool addLanguage(int index, const std::string &lang) = 0;
	virtual bool deleteLanguage(int index) = 0;
	virtual int getLanguageIndex(const std::string &lang) const = 0;

	virtual int itemCount(void) const = 0;
	virtual bool addItem(int index) = 0;
	virtual bool deleteItem(int index) = 0;

	virtual bool swapLanguages(int l, int r) = 0;
	virtual bool swapItems(int l, int r) = 0;

	/**
	 * @param[out] lang_
	 * @param[out] item
	 */
	virtual const char* get(const std::string &lang, const std::string &key, int* lang_ /* nullable */, int* item /* nullable */) const = 0;
	virtual const char* get(int lang, int item) const = 0;
	virtual bool set(int lang, int item, const std::string &val) = 0;

	virtual bool fromBlank(void) = 0;

	/**
	 * @param[out] val
	 */
	virtual bool toCsv(std::string &val) const = 0;
	virtual bool fromCsv(const std::string &val) = 0;

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

	static I18n* create(void);
	static void destroy(I18n* ptr);
};

/* ===========================================================================} */

#endif /* __I18N_H__ */
