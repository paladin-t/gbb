/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __ENTRY_H__
#define __ENTRY_H__

#include "../gbbasic.h"
#include "text.h"

/*
** {===========================================================================
** Entry
*/

/**
 * @brief Entry.
 */
class Entry {
public:
	typedef std::map<Entry, std::string> Dictionary;

	struct Stub {
		const Text::Array &parts;

		Stub(const Text::Array &data);
		Stub(const Entry &data);
	};

protected:
	int _order = 100;
	std::string _name;
	Text::Array _parts;

public:
	Entry();
	Entry(const char* name);
	Entry(const std::string &name);
	Entry(const char* name, int order);
	Entry(const std::string &name, int order);

	bool operator < (const Entry &other) const;

	bool empty(void) const;
	void clear(void);

	const std::string &name(void) const;
	const Text::Array &parts(void) const;

	const char* c_str(void) const;

	static int compare(const Stub &left, const Stub &right, const std::string* priority = nullptr);
};

/**
 * @brief Entry with path.
 */
class EntryWithPath : public Entry {
public:
	typedef std::vector<EntryWithPath> Array;
	typedef std::list<EntryWithPath> List;

protected:
	std::string _path;
	std::string _tooltips;

public:
	EntryWithPath();
	EntryWithPath(const std::string &entry, const std::string &path, const std::string &tips);

	const std::string &path(void) const;
	const std::string &tooltips(void) const;
};

/**
 * @brief Entry with visibility.
 */
class EntryWithVisibility : public Entry {
public:
	typedef std::map<EntryWithVisibility, std::string> Dictionary;

protected:
	bool _hidden = false;

public:
	EntryWithVisibility();
	EntryWithVisibility(const char* name);
	EntryWithVisibility(const std::string &name);
	EntryWithVisibility(const char* name, bool hidden_);
	EntryWithVisibility(const std::string &name, bool hidden_);
	EntryWithVisibility(const char* name, int order, bool hidden_);
	EntryWithVisibility(const std::string &name, int order, bool hidden_);

	bool operator < (const EntryWithVisibility &other) const;

	bool hidden(void) const;
};

/* ===========================================================================} */

#endif /* __ENTRY_H__ */
