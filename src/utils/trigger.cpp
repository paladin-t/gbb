/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "trigger.h"

/*
** {===========================================================================
** Trigger
*/

Trigger::Trigger() {
}

Trigger::Trigger(ValueType x0_, ValueType y0_, ValueType x1_, ValueType y1_) {
	x0 = x0_;
	y0 = y0_;
	x1 = x1_;
	y1 = y1_;
}

Trigger::Trigger(const Rect &other) {
	x0 = other.x0;
	y0 = other.y0;
	x1 = other.x1;
	y1 = other.y1;
}

Trigger Trigger::operator + (const Math::Vec2i &diff) const {
	Trigger result = *this;
	result.x0 += diff.x;
	result.y0 += diff.y;
	result.x1 += diff.x;
	result.y1 += diff.y;

	return result;
}

Trigger &Trigger::operator += (const Math::Vec2i &diff) {
	x0 += diff.x;
	y0 += diff.y;
	x1 += diff.x;
	y1 += diff.y;

	return *this;
}

Math::Vec2i Trigger::position(void) const {
	return Math::Vec2i(xMin(), yMin());
}

Math::Vec2i Trigger::size(void) const {
	return Math::Vec2i(width(), height());
}

Trigger &Trigger::setRect(const Rect &rect) {
	x0 = rect.x0;
	y0 = rect.y0;
	x1 = rect.x1;
	y1 = rect.y1;

	return *this;
}

Math::Recti Trigger::toRect(void) const {
	return Math::Recti(x0, y0, x1, y1);
}

/* ===========================================================================} */
