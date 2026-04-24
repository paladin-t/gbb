/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "entry.h"
#include "generic.h"

/*
** {===========================================================================
** Asset
*/

Entry::Stub::Stub(const Text::Array &data) : parts(data) {
}

Entry::Stub::Stub(const Entry &data) : parts(data.parts()) {
}

Entry::Entry() {
}

Entry::Entry(const char* name) {
	_name = name;

	_parts = Text::split(_name, "/");
}

Entry::Entry(const std::string &name) {
	_name = name;

	_parts = Text::split(_name, "/");
}

Entry::Entry(const char* name, int order) {
	_order = order;

	_name = name;

	_parts = Text::split(_name, "/");
}

Entry::Entry(const std::string &name, int order) {
	_order = order;

	_name = name;

	_parts = Text::split(_name, "/");
}

bool Entry::operator < (const Entry &other) const {
	if (_order < other._order)
		return -1;
	else if (_order > other._order)
		return 1;

	return compare(*this, other, nullptr) < 0;
}

bool Entry::empty(void) const {
	return _name.empty();
}

void Entry::clear(void) {
	_name.clear();
	_parts.clear();
}

const std::string &Entry::name(void) const {
	return _name;
}

const Text::Array &Entry::parts(void) const {
	return _parts;
}

const char* Entry::c_str(void) const {
	return _name.c_str();
}

int Entry::compare(const Stub &left_, const Stub &right_, const std::string* priority) {
	const Text::Array &left = left_.parts;
	const Text::Array &right = right_.parts;
	if (priority) {
		if ((left.size() == 1 && left.front() == *priority) && (right.size() == 1 && right.front() == *priority))
			return 0;
		if (left.size() == 1 && left.front() == *priority)
			return -1;
		else if (right.size() == 1 && right.front() == *priority)
			return 1;
	}

	return Compare::doc(
		left.begin(), left.end(), right.begin(), right.end(),
		[] (std::string lstr, std::string rstr) -> int {
			Text::toLowerCase(lstr); // Case-insensitive comparison.
			Text::toLowerCase(rstr);

			return Compare::lex(
				lstr.begin(), lstr.end(), rstr.begin(), rstr.end(),
				[] (char lch, char rch) -> int {
					if (lch < rch)
						return -1;
					else if (lch > rch)
						return 1;

					return 0;
				}
			);
		}
	);
}

EntryWithPath::EntryWithPath() {
}

EntryWithPath::EntryWithPath(const std::string &entry, const std::string &path, const std::string &tips) :
	Entry(entry),
	_path(path),
	_tooltips(tips)
{
}

const std::string &EntryWithPath::path(void) const {
	return _path;
}

const std::string &EntryWithPath::tooltips(void) const {
	return _tooltips;
}

/* ===========================================================================} */
