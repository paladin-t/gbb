/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __UPDATABLE_H__
#define __UPDATABLE_H__

#include "../gbbasic.h"

/*
** {===========================================================================
** Updatable
*/

/**
 * @brief Updatable interface.
 */
class Updatable {
public:
	virtual bool update(double delta) = 0;
};

/* ===========================================================================} */

#endif /* __UPDATABLE_H__ */
