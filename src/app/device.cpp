/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "device.h"
#include "device_binjgb.h"

/*
** {===========================================================================
** Device
*/

Device::KeyboardModifiers::KeyboardModifiers() {
}

Device::KeyboardModifiers::KeyboardModifiers(bool ctrl_, bool shift_, bool alt_, bool super_) :
	ctrl(ctrl_),
	shift(shift_),
	alt(alt_),
	super(super_)
{
}

Device* Device::create(CoreTypes type, Protocol* dbgListener) {
	Device* result = nullptr;
	switch (type) {
	case CoreTypes::BINJGB:
		result = new DeviceBinjgb(dbgListener);

		break;
	default:
		GBBASIC_ASSERT(false && "Not implemented.");

		break;
	}

	return result;
}

void Device::destroy(Device* ptr) {
	Device* impl = ptr;
	delete impl;
}

/* ===========================================================================} */
