/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __FILE_H__
#define __FILE_H__

#include "../gbbasic.h"
#include "object.h"
#include "stream.h"

/*
** {===========================================================================
** File
*/

/**
 * @brief File streaming object.
 */
class File : public Stream, public virtual Object {
public:
	typedef std::shared_ptr<File> Ptr;

public:
	GBBASIC_CLASS_TYPE('F', 'I', 'L', 'E')

	virtual const FILE* pointer(void) const = 0;
	virtual FILE* pointer(void) = 0;

	virtual bool open(const char* path, Accesses access) = 0;
	virtual bool close(void) = 0;

	static File* create(void);
	static void destroy(File* ptr);
};

/* ===========================================================================} */

#endif /* __FILE_H__ */
