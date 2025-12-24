/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __CLONEABLE_H__
#define __CLONEABLE_H__

#include "../gbbasic.h"

/*
** {===========================================================================
** Cloneable
*/

/**
 * @brief Cloneable interface.
 */
template<typename T> class Cloneable {
public:
	/**
	 * @param[out] ptr
	 */
	virtual bool clone(T** ptr) const = 0;
};

/* ===========================================================================} */

#endif /* __CLONEABLE_H__ */
