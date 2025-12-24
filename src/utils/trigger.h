/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include "../gbbasic.h"
#include "colour.h"
#include <string>
#include <vector>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef TRIGGER_HAS_NONE
#	define TRIGGER_HAS_NONE           0x00
#endif /* TRIGGER_HAS_NONE */
#ifndef TRIGGER_HAS_ENTER_SCRIPT
#	define TRIGGER_HAS_ENTER_SCRIPT   0x01
#endif /* TRIGGER_HAS_ENTER_SCRIPT */
#ifndef TRIGGER_HAS_LEAVE_SCRIPT
#	define TRIGGER_HAS_LEAVE_SCRIPT   0x02
#endif /* TRIGGER_HAS_LEAVE_SCRIPT */

/* ===========================================================================} */

/*
** {===========================================================================
** Trigger
*/

/**
 * @brief Trigger resource structure.
 */
struct Trigger : public Math::Recti {
	typedef std::vector<Trigger> Array;

	Colour color;
	int eventType = TRIGGER_HAS_NONE;
	std::string eventRoutine;

	Trigger();
	Trigger(ValueType x0_, ValueType y0_, ValueType x1_, ValueType y1_);
	Trigger(const Rect &other);

	Trigger &operator = (const Rect &other);

	Trigger operator + (const Math::Vec2i &diff) const;
	Trigger &operator += (const Math::Vec2i &diff);

	Math::Vec2i position(void) const;
	Math::Vec2i size(void) const;

	Math::Recti toRect(void) const;
};

/* ===========================================================================} */

#endif /* __TRIGGER_H__ */
