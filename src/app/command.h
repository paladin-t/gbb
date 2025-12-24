/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "../gbbasic.h"
#include "../utils/object.h"
#include <deque>
#include <functional>
#include <map>

/*
** {===========================================================================
** Command
*/

class Command {
	friend class CommandQueue;

public:
	typedef std::function<Command*(void)> Creator;
	typedef std::function<void(Command*)> Destroyer;

private:
	unsigned _id = 0;

public:
	Command();
	virtual ~Command();

	unsigned id(void) const;

	virtual unsigned type(void) const = 0;

	virtual const char* toString(void) const = 0;

	template<typename T> static bool is(const Command* ptr) {
		return !!dynamic_cast<const T*>(ptr);
	}
	template<typename T> static const T* as(const Command* ptr) {
		return dynamic_cast<const T*>(ptr);
	}
	template<typename T> static T* as(Command* ptr) {
		return dynamic_cast<T*>(ptr);
	}

	virtual Command* redo(Object::Ptr obj, int argc, const Variant* argv) = 0;
	Command* redo(Object::Ptr obj) {
		return redo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Command* redo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return redo(obj, (int)n, argv);
	}
	virtual Command* undo(Object::Ptr obj, int argc, const Variant* argv) = 0;
	Command* undo(Object::Ptr obj) {
		return undo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Command* undo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return undo(obj, (int)n, argv);
	}

	virtual Command* exec(Object::Ptr obj, int argc, const Variant* argv) = 0;
	Command* exec(Object::Ptr obj) {
		return exec(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Command* exec(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return exec(obj, (int)n, argv);
	}

	virtual bool isSimilarTo(const Command* other) const;
	virtual bool mergeWith(const Command* other);

protected:
	template<typename Arg> static Arg unpack(int argc, const Variant* argv, int idx, Arg default_) {
		return (0 <= idx && idx < argc && argv) ? (Arg)argv[idx] : default_;
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Command queue
*/

class CommandQueue {
private:
	struct Factory {
		Command::Creator create = nullptr;
		Command::Destroyer destroy = nullptr;

		Factory(Command::Creator create, Command::Destroyer destroy);
	};
	typedef std::map<unsigned, Factory> Factories;

	typedef std::deque<Command*> Collection;

	typedef std::function<void(Command*)> CommandHandler;

private:
	Factories _factories;
	unsigned _idSeed = 1;
	Collection _collection;
	Collection::iterator _cursor;
	int _savepoint = 0;

	int _threshold = -1;

public:
	CommandQueue(int threshold = -1);
	~CommandQueue();

	int cursor(void) const;
	int offset(void) const;

	Command* enqueue(unsigned type);
	template<typename T> T* enqueue(void) {
		return static_cast<T*>(enqueue(T::TYPE()));
	}
	bool discard(void);
	bool empty(void) const;
	Command* back(void);
	Command* at(int index);
	int clear(void);
	void foreach(CommandHandler handler);

	const Command* redoable(void) const;
	const Command* undoable(void) const;
	const Command* current(unsigned* id /* nullable */) const;
	const Command* seek(unsigned id, int dir, int* count /* nullable */) const;

	Command* redo(Object::Ptr obj, int argc, const Variant* argv);
	Command* redo(Object::Ptr obj) {
		return redo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Command* redo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return redo(obj, (int)n, argv);
	}
	Command* undo(Object::Ptr obj, int argc, const Variant* argv);
	Command* undo(Object::Ptr obj) {
		return undo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Command* undo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return undo(obj, (int)n, argv);
	}

	int simplify(void);

	bool hasUnsavedChanges(void) const;
	void markChangesSaved(void);

	CommandQueue* reg(unsigned type, Command::Creator create, Command::Destroyer destroy);
	template<typename T> CommandQueue* reg(void) {
		return reg(T::TYPE(), T::create, T::destroy);
	}
	CommandQueue* unreg(unsigned type);
	template<typename T> CommandQueue* unreg(void) {
		return unreg(T::TYPE());
	}

private:
	Command* createCommand(unsigned type);
	void destroyCommand(Command* ptr);
};

/* ===========================================================================} */

#endif /* __COMMAND_H__ */
