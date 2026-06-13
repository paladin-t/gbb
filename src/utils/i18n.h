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

	/**
	 * @return `nullptr`.
	 */
	virtual void* pointer(void) = 0;

	// TODO: i18n.

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
