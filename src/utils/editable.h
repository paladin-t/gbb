/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __EDITABLE_H__
#define __EDITABLE_H__

#include "../gbbasic.h"
#include "assets.h"
#include "dispatchable.h"

/*
** {===========================================================================
** Editable
*/

/**
 * @brief Editable interface.
 */
class Editable : public Dispatchable {
public:
	enum Messages : unsigned {
		BACK_SPACE,
		BACK_SPACE_WORDWISE,
		BIND_CALLBACK,
		CLEAR_ERRORS,
		CLEAR_LANGUAGE_DEFINITION,
		CLEAR_UNDO_REDO_RECORDS,
		DELETE_LINE,
		FIND,
		FIND_NEXT,
		FIND_PREVIOUS,
		GET_CURSOR,
		GET_PROGRAM_POINTER,
		GOTO,
		IMPORT,
		INDENT,
		MOVE_DOWN,
		MOVE_UP,
		PITCH,
		REDO,
		RESIZE,
		SELECT_ALL,
		SELECT_WORD,
		SET_COLUMN_INDICATOR,
		SET_CONTENT,
		SET_CURSOR,
		SET_ERRORS,
		SET_FRAME,
		SET_INDENT_RULE,
		SET_PROGRAM_POINTER,
		SET_SELECTION,
		SET_SHOW_SPACES,
		SYNC_DATA,
		TO_LOWER_CASE,
		TO_UPPER_CASE,
		TOGGLE_COMMENT,
		UPDATE_INDEX,
		UPDATE_REF_INDEX,
		UNDO,
		UNINDENT,

		MAX
	};

public:
	template<typename T> static bool is(const Editable* ptr) {
		return !!dynamic_cast<const T*>(ptr);
	}
	template<typename T> static const T* as(const Editable* ptr) {
		return dynamic_cast<const T*>(ptr);
	}
	template<typename T> static T* as(Editable* ptr) {
		return dynamic_cast<T*>(ptr);
	}

	virtual ~Editable();

	virtual unsigned type(void) const = 0;

	virtual void open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* project, int index, unsigned refCategory, int refIndex) = 0;
	virtual void close(int index) = 0;

	virtual int index(void) const = 0;

	virtual void enter(class Workspace* ws) = 0;
	virtual void leave(class Workspace* ws) = 0;

	virtual void flush(void) const = 0;

	virtual bool readonly(void) const = 0;
	virtual void readonly(bool ro) = 0;

	virtual bool hasUnsavedChanges(void) const = 0;
	virtual void markChangesSaved(void) = 0;

	virtual void copy(void) = 0;
	virtual void cut(void) = 0;
	virtual bool pastable(void) const = 0;
	virtual void paste(void) = 0;
	virtual void del(bool wholeLine) = 0;
	virtual bool selectable(void) const = 0;

	virtual const char* redoable(void) const = 0;
	virtual const char* undoable(void) const = 0;

	virtual void redo(BaseAssets::Entry* entry) = 0;
	virtual void undo(BaseAssets::Entry* entry) = 0;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		const char* title,
		float x, float y, float width, float height,
		double delta
	) = 0;

	virtual void statusInvalidated(void) = 0;

	virtual void added(BaseAssets::Entry* entry, int index) = 0;
	virtual void removed(BaseAssets::Entry* entry, int index) = 0;

	virtual void played(class Renderer* rnd, class Workspace* ws) = 0;
	virtual void stopped(class Renderer* rnd, class Workspace* ws) = 0;

	/**
	 * @brief Callback when the editor window is resized.
	 */
	virtual void resized(class Renderer* rnd, const Math::Vec2i &oldSize, const Math::Vec2i &newSize) = 0;

	virtual void focusLost(class Renderer* rnd) = 0;
	virtual void focusGained(class Renderer* rnd) = 0;
};

/* ===========================================================================} */

#endif /* __EDITABLE_H__ */
