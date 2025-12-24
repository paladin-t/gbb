/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "archive.h"
#include "archive_zip.h"
#include "file_handle.h"

/*
** {===========================================================================
** Archive
*/

Archive::Formats Archive::formatOf(const char*) {
	Formats result = ZIP;

	return result;
}

Archive* Archive::create(Formats type) {
	switch (type) {
	case ZIP:
		return archive_create_zip();
	default:
		GBBASIC_ASSERT(false && "Unknown.");

		return nullptr;
	}
}

void Archive::destroy(Archive* ptr) {
	const Formats type = ptr->format();
	switch (type) {
	case ZIP:
		archive_destroy_zip(ptr);

		break;
	default:
		GBBASIC_ASSERT(false && "Unknown.");

		break;
	}
}

/* ===========================================================================} */
