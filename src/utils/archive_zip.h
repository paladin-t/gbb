/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ARCHIVE_ZIP_H__
#define __ARCHIVE_ZIP_H__

#include "archive.h"

/*
** {===========================================================================
** ZIP archive
*/

class Archive* archive_create_zip(void);
void archive_destroy_zip(class Archive* ptr);

/* ===========================================================================} */

#endif /* __ARCHIVE_ZIP_H__ */
