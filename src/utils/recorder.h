/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __RECORDER_H__
#define __RECORDER_H__

#include "../gbbasic.h"
#include "../../lib/promise_cpp/include/promise-cpp/promise.hpp"
#include <functional>

/*
** {===========================================================================
** Recorder
*/

/**
 * @brief Recorder utilities.
 */
class Recorder {
public:
	typedef std::function<promise::Promise(void)> SaveHandler;

	typedef std::function<void(const char* /* msg */)> PrintHandler;

public:
	virtual ~Recorder();

	virtual bool recording(void) const = 0;

	virtual void start(bool drawCursor, const class Image* cursorImg, int frameCount = -1) = 0;
	virtual void stop(void) = 0;

	virtual void update(class Window* wnd, class Renderer* rnd, class Texture* tex) = 0;
	virtual void update(class Window* wnd, class Renderer* rnd) = 0;

	static Recorder* create(class Input* input, unsigned fps, SaveHandler save, PrintHandler onPrint);
	static void destroy(Recorder* ptr);
};

/* ===========================================================================} */

#endif /* __RECORDER_H__ */
