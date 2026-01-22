/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "../gbbasic.h"
#include "colour.h"
#include "plus.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef GBBASIC_RENDER_TARGET
#	define GBBASIC_RENDER_TARGET(RND, TEX) \
	ProcedureGuard<class Texture> GBBASIC_UNIQUE_NAME(__TARGET__)( \
		std::bind( \
			[&] (class Renderer* rnd) -> class Texture* { \
				class Texture* result = rnd->target(); \
				rnd->target(TEX); \
				return result; \
			}, \
			(RND) \
		), \
		std::bind( \
			[] (class Renderer* rnd, class Texture* tex) -> void { \
				rnd->target(tex); \
			}, \
			(RND), std::placeholders::_1 \
		) \
	);
#endif /* GBBASIC_RENDER_TARGET */

#ifndef GBBASIC_RENDER_SCALE
#	define GBBASIC_RENDER_SCALE(RND, SCL) \
	ProcedureGuard<int> GBBASIC_UNIQUE_NAME(__SCALE__)( \
		std::bind( \
			[&] (class Renderer* rnd) -> int* { \
				int* result = (int*)(uintptr_t)rnd->scale(); \
				rnd->scale(SCL); \
				return result; \
			}, \
			(RND) \
		), \
		std::bind( \
			[] (class Renderer* rnd, int* scl) -> void { \
				rnd->scale((int)(uintptr_t)scl); \
			}, \
			(RND), std::placeholders::_1 \
		) \
	);
#endif /* GBBASIC_RENDER_SCALE */

/* ===========================================================================} */

/*
** {===========================================================================
** Renderer
*/

/**
 * @brief Renderer structure and context.
 */
class Renderer {
public:
	/**
	 * @brief Gets the raw pointer.
	 *
	 * @return `SDL_Renderer*`.
	 */
	virtual void* pointer(void) = 0;

	/**
	 * @brief Opens the renderer for further operation.
	 */
	virtual bool open(class Window* wnd, bool software) = 0;
	/**
	 * @brief Closes the renderer after all operations.
	 */
	virtual bool close(void) = 0;

	/**
	 * @brief Gets the backend driver of the renderer.
	 */
	virtual const char* driver(void) const = 0;

	/**
	 * @brief Gets whether render target is supported by the renderer.
	 */
	virtual bool renderTargetSupported(void) const = 0;

	/**
	 * @brief Gets the maximum texture width supported by the renderer.
	 */
	virtual int maxTextureWidth(void) const = 0;
	/**
	 * @brief Gets the maximum texture height supported by the renderer.
	 */
	virtual int maxTextureHeight(void) const = 0;

	/**
	 * @brief Gets the current width of the renderer.
	 */
	virtual int width(void) const = 0;
	/**
	 * @brief Gets the current height of the renderer.
	 */
	virtual int height(void) const = 0;

	/**
	 * @brief Gets the current scale of the renderer.
	 */
	virtual int scale(void) const = 0;
	/**
	 * @brief Sets the current scale of the renderer.
	 */
	virtual void scale(int val) = 0;

	/**
	 * @brief Gets the current target of the renderer.
	 */
	virtual class Texture* target(void) = 0;
	/**
	 * @brief Sets the current target of the renderer.
	 */
	virtual void target(class Texture* tex /* nullable */) = 0;

	/**
	 * @brief Gets the current draw color of the renderer.
	 */
	virtual Colour drawColor(void) const = 0;
	/**
	 * @brief Sets the current draw color of the renderer.
	 */
	virtual void drawColor(const Colour &col) = 0;

	/**
	 * @brief Gets the current blend mode of the renderer.
	 */
	virtual unsigned blend(void) const = 0;
	/**
	 * @brief Sets the current blend mode of the renderer.
	 */
	virtual void blend(unsigned mode) = 0;

	/**
	 * @brief Sets the current clip area of the renderer.
	 */
	virtual void clip(int x, int y, int width, int height) = 0;
	/**
	 * @brief Resets the current clip area of the renderer.
	 */
	virtual void clip(void) = 0;

	/**
	 * @brief Draws a point at the specific position to the current render target.
	 */
	virtual bool drawPoint(int x, int y) = 0;

	/**
	 * @brief Clears the renderer with the specific color.
	 */
	virtual void clear(const Colour* col /* nullable */) = 0;

	/**
	 * @brief Renders the specific texture.
	 *   For `STATIC`, `STREAMING`, `TARGET`.
	 */
	virtual void render(
		class Texture* tex,
		const Math::Recti* srcRect /* nullable */, const Math::Recti* dstRect /* nullable */,
		const double* rotAngle /* nullable */, const Math::Vec2f* rotCenter /* nullable */,
		bool hFlip, bool vFlip,
		const Colour* color /* nullable */, bool colorChanged, bool alphaChanged
	) = 0;

	/**
	 * @brief Flushes the renderer.
	 */
	virtual void flush(void) = 0;

	static Renderer* create(void);
	static void destroy(Renderer* ptr);
};

/* ===========================================================================} */

#endif /* __RENDERER_H__ */
