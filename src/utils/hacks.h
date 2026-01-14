/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __HACKS_H__
#define __HACKS_H__

#include "../gbbasic.h"
#if GBBASIC_MULTITHREAD_ENABLED && defined GBBASIC_DEBUG
#	include <thread>
#endif /* GBBASIC_MULTITHREAD_ENABLED && GBBASIC_DEBUG */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef GBBASIC_THREADING_GUARD_ENABLED
#	if GBBASIC_MULTITHREAD_ENABLED && defined GBBASIC_DEBUG
#		define GBBASIC_THREADING_GUARD_ENABLED 1
#	else /* GBBASIC_MULTITHREAD_ENABLED && GBBASIC_DEBUG */
#		define GBBASIC_THREADING_GUARD_ENABLED 0
#	endif /* GBBASIC_MULTITHREAD_ENABLED && GBBASIC_DEBUG */
#endif /* GBBASIC_THREADING_GUARD_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

class Renderer;

struct SDL_Surface;
struct SDL_Texture;

/* ===========================================================================} */

/*
** {===========================================================================
** ImGuiSDL
*/

namespace ImGuiSDLHack {

/**
 * @brief Texture structure alias of the `Texture` struct in `ImGuiSDL`.
 */
struct Texture final {
	SDL_Surface* surface = nullptr;
	SDL_Texture* source = nullptr;

	Texture(Renderer* rnd, unsigned char* pixels, int width, int height);
	~Texture();
};

}

/* ===========================================================================} */

/*
** {===========================================================================
** Threading guard
*/

#if GBBASIC_THREADING_GUARD_ENABLED
class ThreadingGuard {
private:
	bool _began = false;
	bool _ended = false;
	std::thread::id _executableThreadId;
	std::thread::id _allowedThreadId;

public:
	ThreadingGuard();
	~ThreadingGuard();

	void begin(const std::thread &thread);
	void end(void);
	void allow(void);
	void disallow(void);

	void validate(void);
};

extern ThreadingGuard graphicsThreadingGuard;
#else /* GBBASIC_THREADING_GUARD_ENABLED */
class ThreadingGuard {
public:
	ThreadingGuard();
	~ThreadingGuard();

	void validate(void);
};

extern ThreadingGuard graphicsThreadingGuard;
#endif /* GBBASIC_THREADING_GUARD_ENABLED */

/* ===========================================================================} */

#endif /* __HACKS_H__ */
