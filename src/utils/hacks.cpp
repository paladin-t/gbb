/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "hacks.h"
#include "renderer.h"
#include <SDL.h>

/*
** {===========================================================================
** ImGuiSDL
*/

namespace ImGuiSDL {
	
Texture::Texture(Renderer* rnd, unsigned char* pixels, int width, int height) {
	SDL_Renderer* renderer = (SDL_Renderer*)rnd->pointer();

	surface = SDL_CreateRGBSurfaceFrom(
		pixels,
		width, height,
		32, width * 4,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
	);

	source = SDL_CreateTextureFromSurface(renderer, surface);
}

Texture::~Texture() {
	SDL_FreeSurface(surface);
	surface = nullptr;

	SDL_DestroyTexture(source);
	source = nullptr;
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Threading guard
*/

#if GBBASIC_THREADING_GUARD_ENABLED
ThreadingGuard::ThreadingGuard() {
}

ThreadingGuard::~ThreadingGuard() {
}

void ThreadingGuard::begin(const std::thread &thread) {
	_began = true;
	_executableThreadId = thread.get_id();
}

void ThreadingGuard::end(void) {
	_executableThreadId = std::thread::id();
	_ended = true;
}

void ThreadingGuard::allow(void) {
	GBBASIC_ASSERT(_began && _ended);
	_allowedThreadId = std::this_thread::get_id();
}

void ThreadingGuard::disallow(void) {
	GBBASIC_ASSERT(_began && _ended);
	_allowedThreadId = std::thread::id();
}

void ThreadingGuard::validate(void) {
	const std::thread::id zero = std::thread::id();
	const std::thread::id current = std::this_thread::get_id();
	(void)current;
	if (_allowedThreadId == zero) {
		if (_began) {
			GBBASIC_ASSERT(_executableThreadId != zero && "Cannot access from this thread.");
		}
		GBBASIC_ASSERT(_executableThreadId != current && "Cannot access from this thread.");
	} else {
		GBBASIC_ASSERT(_allowedThreadId == current && "Cannot access from this thread.");
	}
}

ThreadingGuard graphicsThreadingGuard;
#else /* GBBASIC_THREADING_GUARD_ENABLED */
ThreadingGuard::ThreadingGuard() {
}

ThreadingGuard::~ThreadingGuard() {
}

void ThreadingGuard::validate(void) {
	// Do nothing.
}

ThreadingGuard graphicsThreadingGuard;
#endif /* GBBASIC_THREADING_GUARD_ENABLED */

/* ===========================================================================} */
