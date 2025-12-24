/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "image.h"
#include "window.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#if SDL_VERSION_ATLEAST(2, 0, 9)
static_assert((unsigned)SDL_ORIENTATION_UNKNOWN == (unsigned)Window::UNKNOWN, "Value does not match.");
static_assert((unsigned)SDL_ORIENTATION_LANDSCAPE == (unsigned)Window::LANDSCAPE, "Value does not match.");
static_assert((unsigned)SDL_ORIENTATION_LANDSCAPE_FLIPPED == (unsigned)Window::LANDSCAPE_FLIPPED, "Value does not match.");
static_assert((unsigned)SDL_ORIENTATION_PORTRAIT == (unsigned)Window::PORTRAIT, "Value does not match.");
static_assert((unsigned)SDL_ORIENTATION_PORTRAIT_FLIPPED == (unsigned)Window::PORTRAIT_FLIPPED, "Value does not match.");
#endif /* SDL_VERSION_ATLEAST(2, 0, 9) */

/* ===========================================================================} */

/*
** {===========================================================================
** Window
*/

class WindowImpl : public Window {
private:
	SDL_Window* _window = nullptr;
	int _scale = 1;

	bool _bordered = true;
	bool _resizable = true;
#if WINDOW_SET_STATE_LAZILY
	bool _lazySetState = false;
	Uint32 _lazyStateValue = 0;
#endif /* WINDOW_SET_STATE_LAZILY */

public:
	virtual ~WindowImpl() {
	}

	virtual void* pointer(void) override {
		return _window;
	}

	virtual bool open(
		const char* title,
		int displayIndex,
		int x, int y,
		int width, int height,
		int minWidth, int minHeight,
		bool borderless,
		bool highDpi, bool opengl,
		bool alwaysOnTop
	) override {
		if (_window)
			return false;

#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
		Uint32 flags = SDL_WINDOW_RESIZABLE;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		if (highDpi)
			flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		if (opengl)
			flags |= SDL_WINDOW_OPENGL;
		if (alwaysOnTop)
			flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#elif defined GBBASIC_OS_HTML
		Uint32 flags = SDL_WINDOW_RESIZABLE;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		if (opengl)
			flags |= SDL_WINDOW_OPENGL;
		if (alwaysOnTop)
			flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#elif defined GBBASIC_OS_IOS || defined GBBASIC_OS_ANDROID
		Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		if (highDpi)
			flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		if (opengl)
			flags |= SDL_WINDOW_OPENGL;
		if (alwaysOnTop)
			flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#else /* Platform macro. */
		Uint32 flags = SDL_WINDOW_RESIZABLE;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		if (highDpi)
			flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		if (opengl)
			flags |= SDL_WINDOW_OPENGL;
		if (alwaysOnTop)
			flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#endif /* Platform macro. */
		displayIndex = Math::clamp(displayIndex, 0, SDL_GetNumVideoDisplays() - 1);
		if (x == -1 && y == -1) {
			x = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
			y = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
		} else {
			SDL_Rect bounds;
			if (SDL_GetDisplayUsableBounds(displayIndex, &bounds) == 0) {
				width = Math::clamp(width, 0, bounds.w);
				height = Math::clamp(height, 0, bounds.h);
				x = Math::clamp(x, bounds.x, bounds.x + bounds.w - 1 - width);
				y = Math::clamp(y, bounds.y, bounds.y + bounds.h - 1 - height);
			}
		}
		_window = SDL_CreateWindow(
			title,
			x, y,
			width, height,
			flags
		);
		SDL_SetWindowMinimumSize(_window, minWidth, minHeight);

		_bordered = !borderless;

		fprintf(stdout, "Window opened.\n");

		return true;
	}
	virtual bool close(void) override {
		if (!_window)
			return false;

		SDL_DestroyWindow(_window);
		_window = nullptr;

		fprintf(stdout, "Window closed.\n");

		return true;
	}

	virtual const char* title(void) const override {
		if (!_window)
			return nullptr;

		return SDL_GetWindowTitle(_window);
	}
	virtual void title(const char* txt) override {
		if (!_window)
			return;

		SDL_SetWindowTitle(_window, txt);
	}

	virtual void icon(class Image* img) override {
		if (!img)
			return;

		SDL_Surface* sur = (SDL_Surface*)img->pointer();
		SDL_SetWindowIcon(_window, sur);
	}

	virtual UInt32 id(void) const override {
		return SDL_GetWindowID(_window);
	}

	virtual int displayIndex(void) const override {
		return SDL_GetWindowDisplayIndex(_window);
	}
	virtual void displayIndex(int idx) override {
		SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED_DISPLAY(idx), SDL_WINDOWPOS_CENTERED_DISPLAY(idx));
	}

	virtual Math::Recti displayBounds(void) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		SDL_Rect bound;
		SDL_GetDisplayUsableBounds(idx, &bound);
		const int x0 = bound.x;
		const int y0 = bound.y;
		const int x1 = bound.x + bound.w - 1;
		const int y1 = bound.y + bound.h - 1;

		return Math::Recti(x0, y0, x1, y1);
	}

	virtual bool dpi(float* ddpi, float* hdpi, float* vdpi) const override {
		if (ddpi)
			*ddpi = 0;
		if (hdpi)
			*hdpi = 0;
		if (vdpi)
			*vdpi = 0;

		const int dspIdx = displayIndex();
		float ddpi_ = 0, hdpi_ = 0, vdpi_ = 0;
		const int ret = SDL_GetDisplayDPI(dspIdx, &ddpi_, &hdpi_, &vdpi_);

		if (ddpi)
			*ddpi = ddpi_;
		if (hdpi)
			*hdpi = hdpi_;
		if (vdpi)
			*vdpi = vdpi_;

		return ret == 0;
	}

	virtual Orientations orientation(void) const override {
#if SDL_VERSION_ATLEAST(2, 0, 9)
		const int dspIdx = displayIndex();
		SDL_DisplayOrientation result = SDL_GetDisplayOrientation(dspIdx);

		return (Orientations)result;
#else /* SDL_VERSION_ATLEAST(2, 0, 9) */
		return Orientations::UNKNOWN;
#endif /* SDL_VERSION_ATLEAST(2, 0, 9) */
	}

	virtual void centralize(void) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED_DISPLAY(idx), SDL_WINDOWPOS_CENTERED_DISPLAY(idx));
	}

	virtual void makeVisible(void) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		SDL_Rect bound;
		SDL_GetDisplayUsableBounds(idx, &bound);
		const int x0 = bound.x;
		const int y0 = bound.y;
		const int x1 = bound.x + bound.w - 1;
		const int y1 = bound.y + bound.h - 1;

		int w = 0, h = 0;
		SDL_GetWindowSize(_window, &w, &h);

		int x = 0, y = 0;
		SDL_GetWindowPosition(_window, &x, &y);

		const int x_ = Math::max(Math::min(x + w, x1) - w, x0);
		const int y_ = Math::max(Math::min(y + h, y1) - h, y0);
		if (x != x_ || y != y_)
			SDL_SetWindowPosition(_window, x_, y_);
	}

	virtual void borderSize(int* left, int* top, int* right, int* bottom) const override {
		SDL_GetWindowBordersSize(_window, top, left, bottom, right);
	}

	virtual Math::Vec2i position(void) const override {
		int x = 0, y = 0;
		SDL_GetWindowPosition(_window, &x, &y);

		return Math::Vec2i(x, y);
	}
	virtual void position(const Math::Vec2i &val) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		int x = val.x;
		int y = val.y;
		if (x == -1 && y == -1) {
			x = SDL_WINDOWPOS_CENTERED_DISPLAY(idx);
			y = SDL_WINDOWPOS_CENTERED_DISPLAY(idx);
		}

		SDL_SetWindowPosition(_window, x, y);
	}

	virtual Math::Vec2i size(void) const override {
		int w = 0, h = 0;
		SDL_GetWindowSize(_window, &w, &h);

		return Math::Vec2i(w, h);
	}
	virtual void size(const Math::Vec2i &val) override {
		const int idx = SDL_GetWindowDisplayIndex(_window);
		SDL_Rect bound;
		SDL_GetDisplayUsableBounds(idx, &bound);

		SDL_SetWindowSize(_window, Math::min((int)val.x, bound.w), Math::min((int)val.y, bound.h));
	}

	virtual Math::Vec2i minimumSize(void) const override {
		int w = 0, h = 0;
		SDL_GetWindowMinimumSize(_window, &w, &h);

		return Math::Vec2i(w, h);
	}
	virtual void minimumSize(const Math::Vec2i &val) override {
		SDL_SetWindowMinimumSize(_window, (int)val.x, (int)val.y);
	}
	virtual Math::Vec2i maximumSize(void) const override {
		int w = 0, h = 0;
		SDL_GetWindowMaximumSize(_window, &w, &h);

		return Math::Vec2i(w, h);
	}
	virtual void maximumSize(const Math::Vec2i &val) override {
		SDL_SetWindowMaximumSize(_window, (int)val.x, (int)val.y);
	}

	virtual bool bordered(void) const override {
		return _bordered;
	}
	virtual void bordered(bool val) override {
		_bordered = val;

		SDL_SetWindowBordered(_window, val ? SDL_TRUE : SDL_FALSE);

		if (!(SDL_GetWindowFlags(_window) & SDL_WINDOW_RESIZABLE) && _resizable)
			SDL_SetWindowResizable(_window, SDL_TRUE);
	}

	virtual bool resizable(void) const override {
		return _resizable;
	}
	virtual void resizable(bool val) override {
		_resizable = val;

		SDL_SetWindowResizable(_window, val ? SDL_TRUE : SDL_FALSE);
	}

	virtual void show(void) override {
		SDL_ShowWindow(_window);
	}
	virtual void hide(void) override {
		SDL_HideWindow(_window);
	}
	virtual void raise(void) override {
		SDL_RaiseWindow(_window);
	}

	virtual bool maximized(void) const override {
		const Uint32 flags = SDL_GetWindowFlags(_window);

		return !!(flags & SDL_WINDOW_MAXIMIZED);
	}
	virtual void maximize(void) override {
#if WINDOW_SET_STATE_LAZILY
		_lazySetState = true;
		_lazyStateValue |= SDL_WINDOW_MAXIMIZED;
		_lazyStateValue &= ~SDL_WINDOW_MINIMIZED;
#else /* WINDOW_SET_STATE_LAZILY */
		SDL_MaximizeWindow(_window);
#endif /* WINDOW_SET_STATE_LAZILY */
	}
	virtual bool minimized(void) const override {
		const Uint32 flags = SDL_GetWindowFlags(_window);

		return !!(flags & SDL_WINDOW_MINIMIZED);
	}
	virtual void minimize(void) override {
#if WINDOW_SET_STATE_LAZILY
		_lazySetState = true;
		_lazyStateValue |= SDL_WINDOW_MINIMIZED;
		_lazyStateValue &= ~SDL_WINDOW_MAXIMIZED;
#else /* WINDOW_SET_STATE_LAZILY */
		SDL_MinimizeWindow(_window);
#endif /* WINDOW_SET_STATE_LAZILY */
	}
	virtual void restore(void) override {
#if WINDOW_SET_STATE_LAZILY
		_lazySetState = true;
		_lazyStateValue &= ~SDL_WINDOW_MAXIMIZED;
		_lazyStateValue &= ~SDL_WINDOW_MINIMIZED;
#else /* WINDOW_SET_STATE_LAZILY */
		SDL_RestoreWindow(_window);
#endif /* WINDOW_SET_STATE_LAZILY */
	}
	virtual bool fullscreen(void) const override {
		const Uint32 flags = SDL_GetWindowFlags(_window);

		return !!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	virtual void fullscreen(bool val) override {
#if WINDOW_SET_STATE_LAZILY
		_lazySetState = true;
		if (val)
			_lazyStateValue |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		else
			_lazyStateValue &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
#else /* WINDOW_SET_STATE_LAZILY */
		if (val) {
			Uint32 flags = SDL_GetWindowFlags(_window);
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(_window, flags);
		} else {
			Uint32 flags = SDL_GetWindowFlags(_window);
			flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
			SDL_SetWindowFullscreen(_window, flags);

			if (!(flags & SDL_WINDOW_MAXIMIZED))
				makeVisible();
		}
#endif /* WINDOW_SET_STATE_LAZILY */
	}

	virtual int width(void) const override {
		int width = 0, height = 0;
		SDL_GetWindowSize(_window, &width, &height);

		return width;
	}
	virtual int height(void) const override {
		int width = 0, height = 0;
		SDL_GetWindowSize(_window, &width, &height);

		return height;
	}

	virtual int scale(void) const override {
		return _scale;
	}
	virtual void scale(int val) override {
		val = Math::max(val, 1);
		if (_scale == val)
			return;

		_scale = val;
	}

	virtual void update(void) override {
#if WINDOW_SET_STATE_LAZILY
		if (_lazySetState) {
			_lazySetState = false;
			Uint32 flags = SDL_GetWindowFlags(_window);
			if (!(_lazyStateValue & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
				if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
					flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
					SDL_SetWindowFullscreen(_window, flags);
				}
			} else if ((_lazyStateValue & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
				if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
					flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
					SDL_SetWindowFullscreen(_window, flags);
				}
			}
			if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
				if ((_lazyStateValue & SDL_WINDOW_MAXIMIZED) && !(flags & SDL_WINDOW_MAXIMIZED)) {
					SDL_MaximizeWindow(_window);
				} else if ((_lazyStateValue & SDL_WINDOW_MINIMIZED) && !(flags & SDL_WINDOW_MINIMIZED)) {
					SDL_MinimizeWindow(_window);
				} else {
					SDL_RestoreWindow(_window);
					centralize();
				}
			}
			_lazyStateValue = SDL_GetWindowFlags(_window);
		}
#endif /* WINDOW_SET_STATE_LAZILY */
	}
};

Window* Window::create(void) {
	WindowImpl* result = new WindowImpl();

	return result;
}

void Window::destroy(Window* ptr) {
	WindowImpl* impl = static_cast<WindowImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
