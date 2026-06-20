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
#ifndef I18N_DEFAULT_COLUMN_NAME
#	define I18N_DEFAULT_COLUMN_NAME "default"
#endif /* I18N_DEFAULT_COLUMN_NAME */
#ifndef I18N_ENGLISH_COLUMN_NAME
#	define I18N_ENGLISH_COLUMN_NAME "english"
#endif /* I18N_ENGLISH_COLUMN_NAME */

#ifndef I18N_MAX_BUFFER_SIZE
#	define I18N_MAX_BUFFER_SIZE 1024
#endif /* I18N_MAX_BUFFER_SIZE */
#ifndef I18N_MAX_ROW_COUNT
#	define I18N_MAX_ROW_COUNT 2000
#endif /* I18N_MAX_ROW_COUNT */
#ifndef I18N_MAX_COLUMN_COUNT
#	define I18N_MAX_COLUMN_COUNT 256
#endif /* I18N_MAX_COLUMN_COUNT */

/* ===========================================================================} */

/*
** {===========================================================================
** I18n
*/

/**
 * @brief I18n resource object.
 *
 * Stores internationalization dictionary as a 2D table. The key and languages
 * are stored as table header (row 0), followed with item content row by row.
 * I.e.
 *   |     | 0      | 1           | 2           | ... |
 *   |-----|--------|-------------|-------------|-----|
 *   | 0   | key    | language 1  | language 2  | ... |
 *   | 1   | item 1 | content 1-1 | content 1-2 | ... |
 *   | 2   | item 2 | content 2-1 | content 2-2 | ... |
 *   | ... | ...    | ...         | ...         | ... |
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

	/**
	 * @brief Gets the count of all columns, including "key" and languages.
	 */
	virtual int languageCount(void) const = 0;
	virtual bool addLanguage(int col, const std::string &lang) = 0;
	virtual bool deleteLanguage(int col) = 0;
	virtual int getLanguageIndex(const std::string &lang) const = 0;
	/**
	 * @param[out] lang
	 */
	virtual bool getLanguage(int col, std::string &lang) const = 0;
	virtual bool setLanguage(int col, const std::string &lang) = 0;

	/**
	 * @brief Gets the count of all items, not including the No. 0 row for languages.
	 */
	virtual int itemCount(void) const = 0;
	/**
	 * @param[in] row Starts from 0, already skipped the No. 0 row for languages.
	 */
	virtual bool addItem(int row, const char* item /* nullable */) = 0;
	/**
	 * @param[in] row Starts from 0, already skipped the No. 0 row for languages.
	 */
	virtual bool deleteItem(int row) = 0;
	virtual int getItemIndex(const std::string &item) const = 0;

	virtual bool swapLanguages(int l, int r) = 0;
	/**
	 * @param[in] l Starts from 0, already skipped the No. 0 row for languages.
	 * @param[in] r Starts from 0, already skipped the No. 0 row for languages.
	 */
	virtual bool swapItems(int l, int r) = 0;

	/**
	 * @param[out] lang_
	 * @param[out] item Starts from 0, already skipped the No. 0 row for languages.
	 */
	virtual const char* getContent(const std::string &lang, const std::string &key, int* lang_ /* nullable */, int* item /* nullable */) const = 0;
	/**
	 * @param[in] item Starts from 0, already skipped the No. 0 row for languages.
	 */
	virtual const char* getContent(int lang, int item) const = 0;
	/**
	 * @param[in] item Starts from 0, already skipped the No. 0 row for languages.
	 */
	virtual bool setContent(int lang, int item, const std::string &val) = 0;

	virtual int columnCount(void) const = 0;
	virtual int rowCount(void) const = 0;
	virtual const char* get(int col, int row) const = 0;
	virtual bool set(int col, int row, const std::string &val) = 0;

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
