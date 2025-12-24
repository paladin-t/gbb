/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "command.h"

/*
** {===========================================================================
** Macros and constants
*/

#if GBBASIC_EDITOR_MAX_COMMAND_COUNT > 0
#	pragma message("Editor command queue threshold is specified.")
#endif /* GBBASIC_EDITOR_MAX_COMMAND_COUNT */

/* ===========================================================================} */

/*
** {===========================================================================
** Command
*/

Command::Command() {
}

Command::~Command() {
}

unsigned Command::id(void) const {
	return _id;
}

bool Command::isSimilarTo(const Command* /* other */) const {
	return false;
}

bool Command::mergeWith(const Command* /* other */) {
	return false;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Command queue
*/

CommandQueue::Factory::Factory(Command::Creator create_, Command::Destroyer destroy_) : create(create_), destroy(destroy_) {
}

CommandQueue::CommandQueue(int threshold) : _threshold(threshold) {
	_cursor = _collection.end();
}

CommandQueue::~CommandQueue() {
	clear();
}

int CommandQueue::cursor(void) const {
	return (int)(_cursor - _collection.begin());
}

int CommandQueue::offset(void) const {
	const int c = cursor();

	if (_savepoint == -1)
		return c;

	return c - _savepoint;
}

Command* CommandQueue::enqueue(unsigned type) {
	if (_cursor != _collection.end()) {
		for (Collection::iterator it = _cursor; it != _collection.end(); ++it) {
			Command* obsolete = *it;
			destroyCommand(obsolete);
		}
		_collection.erase(_cursor, _collection.end());
		_cursor = _collection.end();
	}

	Command* current = createCommand(type);
	_collection.push_back(current);
	_cursor = _collection.end();

	if (_threshold > 0) {
		while ((int)_collection.size() > _threshold) {
			if (_savepoint > 0)
				--_savepoint;
			const int offset = cursor();
			Command* overflow = _collection.front();
			destroyCommand(overflow);
			_collection.pop_front();
			_cursor = _collection.begin() + offset - 1;
		}
	}

	if (_savepoint >= cursor())
		_savepoint = -1;

	return _collection.back();
}

bool CommandQueue::discard(void) {
	if (_collection.empty())
		return false;

	if (_cursor != _collection.end())
		return false;

	Command* obsolete = _collection.back();
	destroyCommand(obsolete);
	_collection.pop_back();

	_cursor = _collection.end();

	if (_savepoint >= cursor())
		_savepoint = -1;

	return true;
}

bool CommandQueue::empty(void) const {
	return _collection.empty() || _cursor == _collection.begin();
}

Command* CommandQueue::back(void) {
	return at(-1);
}

Command* CommandQueue::at(int index) {
	if (index >= 0 && index < (int)_collection.size())
		return _collection[index];

	if (index < 0 && -index <= (int)_collection.size())
		return _collection[(int)_collection.size() + index];

	return nullptr;
}

int CommandQueue::clear(void) {
	const int result = (int)_collection.size();
	for (Command* c : _collection)
		destroyCommand(c);
	_collection.clear();

	_cursor = _collection.end();
	_savepoint = -1;

	return result;
}

void CommandQueue::foreach(CommandHandler handler) {
	for (Collection::iterator it = _collection.begin(); it != _collection.end(); ++it) {
		Command* cmd = *it;
		handler(cmd);
	}
}

const Command* CommandQueue::redoable(void) const {
	if (_cursor == _collection.end())
		return nullptr;

	return *_cursor;
}

const Command* CommandQueue::undoable(void) const {
	if (_cursor == _collection.begin())
		return nullptr;

	Collection::iterator it = _cursor;
	--it;

	return *it;
}

const Command* CommandQueue::current(unsigned* id) const {
	if (id)
		*id = 0;

	if (_cursor == _collection.begin())
		return nullptr;

	Collection::iterator it = _cursor;
	--it;

	const Command* cmd = *it;
	if (id)
		*id = cmd->id();

	return cmd;
}

const Command* CommandQueue::seek(unsigned id, int dir, int* count) const {
	if (count)
		*count = 0;

	if (dir == -1) { // Backward.
		Collection::iterator it = _cursor;
		if (it == _collection.begin())
			return nullptr;

		--it;

		while (it != _collection.begin()) {
			if ((*it)->id() == id)
				return *it;

			--it;
			if (count)
				++(*count);
		}

		if ((*it)->id() == id)
			return *it;
	} else if (dir == 1) { // Forward.
		Collection::iterator it = _cursor;
		while (it != _collection.end()) {
			if ((*it)->id() == id)
				return *it;

			++it;
			if (count)
				++(*count);
		}
	} else {
		GBBASIC_ASSERT(false && "Wrong data.");
	}

	return nullptr;
}

Command* CommandQueue::redo(Object::Ptr obj, int argc, const Variant* argv) {
	if (_cursor == _collection.end())
		return nullptr;

	Command* c = *_cursor;
	Command* ret = c->redo(obj, argc, argv);
	++_cursor;

	return ret;
}

Command* CommandQueue::undo(Object::Ptr obj, int argc, const Variant* argv) {
	if (_cursor == _collection.begin())
		return nullptr;

	--_cursor;
	Command* c = *_cursor;

	return c->undo(obj, argc, argv);
}

int CommandQueue::simplify(void) {
	if (_cursor != _collection.end())
		return 0;

	if (_collection.size() < 2)
		return 0;

	Collection::reverse_iterator rit = _collection.rbegin();
	const Command* cmd1 = *rit++;
	Command* cmd0 = *rit++;
	if (!cmd1->isSimilarTo(cmd0))
		return 0;

	if (!cmd0->mergeWith(cmd1))
		return 0;

	Command* obsolete = _collection.back();
	destroyCommand(obsolete);
	_collection.pop_back();

	_cursor = _collection.end();

	if (_savepoint >= cursor())
		_savepoint = -1;

	return 1;
}

bool CommandQueue::hasUnsavedChanges(void) const {
	return _savepoint == -1 || _savepoint != cursor();
}

void CommandQueue::markChangesSaved(void) {
	_savepoint = cursor();
}

CommandQueue* CommandQueue::reg(unsigned type, Command::Creator create, Command::Destroyer destroy) {
	Factories::iterator it = _factories.find(type);
	if (it != _factories.end()) {
		GBBASIC_ASSERT(false && "Already registered.");

		return this;
	}

	_factories.insert(std::make_pair(type, Factory(create, destroy)));

	return this;
}

CommandQueue* CommandQueue::unreg(unsigned type) {
	Factories::iterator it = _factories.find(type);
	if (it == _factories.end()) {
		GBBASIC_ASSERT(false && "Not yet registered.");

		return this;
	}

	_factories.erase(it);

	return this;
}

Command* CommandQueue::createCommand(unsigned type) {
	Factories::iterator it = _factories.find(type);
	if (it == _factories.end())
		return nullptr;

	Command* cmd = it->second.create();
	cmd->_id = _idSeed++;
	if (_idSeed == 0)
		_idSeed = 1;

	return cmd;
}

void CommandQueue::destroyCommand(Command* ptr) {
	Factories::iterator it = _factories.find(ptr->type());
	if (it == _factories.end())
		return;

	it->second.destroy(ptr);
}

/* ===========================================================================} */
