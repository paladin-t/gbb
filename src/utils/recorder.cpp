/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "datetime.h"
#include "encoding.h"
#include "file_sandbox.h"
#include "filesystem.h"
#include "image.h"
#include "input.h"
#include "platform.h"
#include "recorder.h"
#include "renderer.h"
#include "texture.h"
#include "window.h"
#include "work_queue.h"
#include "../../lib/jo_gif/jo_gif.h"
#include "../../lib/lz4/lib/lz4.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef RECORDER_SKIP_FRAME_COUNT
#	define RECORDER_SKIP_FRAME_COUNT 5
#endif /* RECORDER_SKIP_FRAME_COUNT */

#ifndef RECORDER_FOOTPRINT_LIMIT
#	define RECORDER_FOOTPRINT_LIMIT (1024 * 1024 * 512) // 512MB.
#endif /* RECORDER_FOOTPRINT_LIMIT */

/* ===========================================================================} */

/*
** {===========================================================================
** Recorder
*/

class RecorderImpl : public Recorder {
private:
	struct Frame {
		typedef std::list<Frame> List;

		Bytes::Ptr compressed = nullptr;
		Math::Vec2i cursor = Math::Vec2i(-1, -1);
		bool pressed = false;

		Frame(Bytes::Ptr cmp) : compressed(cmp) {
		}
		Frame(Bytes::Ptr cmp, const Math::Vec2i &pos, bool p) : compressed(cmp), cursor(pos), pressed(p) {
		}
	};

private:
	Input* _input = nullptr; // Foreign.
	unsigned _fps = GBBASIC_ACTIVE_FRAME_RATE;
	unsigned _frameSkipping = 0;
	bool _drawCursor = false;
	const Image* _cursorImage = nullptr;
	SaveHandler _save = nullptr;
	PrintHandler _onPrint = nullptr;

	int _width = 0;
	int _height = 0;

	int _recording = 0;
	Frame::List _frames;

	Bytes::Ptr _cache = nullptr;
	int _footprint = 0;

public:
	RecorderImpl(Input* input, unsigned fps, SaveHandler save, PrintHandler onPrint) : _input(input), _fps(fps), _save(save), _onPrint(onPrint) {
	}
	virtual ~RecorderImpl() override {
		clear();
	}

	virtual bool recording(void) const override {
		return !!_recording;
	}

	virtual void start(bool drawCursor, const class Image* cursorImg, int frameCount) override {
		_frameSkipping = RECORDER_SKIP_FRAME_COUNT;
		_drawCursor = drawCursor;
		_cursorImage = cursorImg;

		_recording = Math::max(frameCount, 1);
		_footprint = 0;
	}
	virtual void stop(void) override {
		_recording = 0;

		promise::Promise canSave = promise::newPromise([] (promise::Defer df) -> void { df.resolve(); });
		if (_save)
			canSave = _save();

		canSave
			.then(
				[&] (void) -> void {
					save();

					_drawCursor = false;
					_cursorImage = nullptr;
				}
			)
			.fail([] (void) -> void { /* Do nothing. */ })
			.always(
				[&] (void) -> void {
					clear();

					_drawCursor = false;
					_cursorImage = nullptr;
				}
			);
	}

	virtual void update(class Window* wnd, class Renderer* rnd, class Texture* tex) override {
		if (_recording == 0)
			return;

		if (_width == 0 && _height == 0) {
			_width = tex->width();
			_height = tex->height();
		}

		if (_width != tex->width() || _height != tex->height()) { // Size changed.
			stop();

			return;
		}

		if (++_frameSkipping == RECORDER_SKIP_FRAME_COUNT + 1) {
			_frameSkipping = 0;

			if (!_cache)
				_cache = Bytes::Ptr(Bytes::create());
			_cache->resize(_width * _height * sizeof(Colour));
			tex->toBytes(rnd, _cache->pointer()); // Save the raw RGBA pixels.

			Bytes::Ptr compressed(Bytes::create());
			int n = LZ4_compressBound((int)_cache->count());
			compressed->resize((size_t)n);
			n = LZ4_compress_default( // Compress the raw RGBA pixels.
				(const char*)_cache->pointer(), (char*)compressed->pointer(),
				(int)_cache->count(), (int)compressed->count()
			);
			GBBASIC_ASSERT(n);
			if (n) {
				compressed->resize((size_t)n);

				_footprint += n;
			}
			if (_drawCursor) {
				int x = 0;
				int y = 0;
				bool b0 = false;
				Math::Vec2i pos;
				if (_input->immediateTouch(wnd, rnd, &x, &y, &b0, nullptr, nullptr, nullptr, nullptr)) {
					pos = Math::Vec2i(x, y);
				}
				_frames.push_back(Frame(compressed, pos, b0)); // Add the compressed frame and touch states.
			} else {
				_frames.push_back(Frame(compressed)); // Add the compressed frame.
			}
			_cache->clear();

			if (_recording > 0 && --_recording == 0) // Frame limit reached.
				stop();
			else if (_footprint >= RECORDER_FOOTPRINT_LIMIT) // Footprint limit reached.
				stop();
		}
	}
	virtual void update(class Window* wnd, class Renderer* rnd) override {
		if (_recording == 0)
			return;

		if (_width == 0 && _height == 0) {
			_width = wnd->width();
			_height = wnd->height();
		}

		if (_width != wnd->width() || _height != wnd->height()) { // Size changed.
			stop();

			return;
		}

		if (++_frameSkipping == RECORDER_SKIP_FRAME_COUNT + 1) {
			_frameSkipping = 0;

			if (!_cache)
				_cache = Bytes::Ptr(Bytes::create());
			_cache->resize(_width * _height * sizeof(Colour));
			Uint32 fmt = SDL_PIXELFORMAT_ABGR8888;
			SDL_RenderReadPixels( // Save the raw RGBA pixels.
				(SDL_Renderer*)rnd->pointer(),
				nullptr,
				fmt,
				_cache->pointer(), _width * sizeof(Colour)
			);

			Bytes::Ptr compressed(Bytes::create());
			int n = LZ4_compressBound((int)_cache->count());
			compressed->resize((size_t)n);
			n = LZ4_compress_default( // Compress the raw RGBA pixels.
				(const char*)_cache->pointer(), (char*)compressed->pointer(),
				(int)_cache->count(), (int)compressed->count()
			);
			GBBASIC_ASSERT(n);
			if (n) {
				compressed->resize((size_t)n);

				_footprint += n;
			}
			if (_drawCursor) {
				int x = 0;
				int y = 0;
				bool b0 = false;
				Math::Vec2i pos;
				if (_input->immediateTouch(wnd, rnd, &x, &y, &b0, nullptr, nullptr, nullptr, nullptr)) {
					pos = Math::Vec2i(x, y);
				}
				_frames.push_back(Frame(compressed, pos, b0)); // Add the compressed frame and touch states.
			} else {
				_frames.push_back(Frame(compressed)); // Add the compressed frame.
			}
			_cache->clear();

			if (_recording > 0 && --_recording == 0) // Frame limit reached.
				stop();
			else if (_footprint >= RECORDER_FOOTPRINT_LIMIT) // Footprint limit reached.
				stop();
		}
	}

private:
	void save(void) {
		// Prepare.
		typedef std::vector<Colour> Colours;
		typedef std::vector<Image::Ptr> ImageArray;

		if (_frames.empty())
			return;

		const bool single = _frames.size() == 1;

		fprintf(stdout, "Recorded %d frames in %d bytes.\n", (int)_frames.size(), _footprint);

		// Ask for a saving path.
		pfd::save_file save(
			GBBASIC_TITLE,
			single ? "gbbasic.png" : "gbbasic.gif",
			single ? std::vector<std::string>{ "PNG files (*.png)", "*.png" } : std::vector<std::string>{ "GIF files (*.gif)", "*.gif" }
		);
		std::string path = save.result();
		Path::uniform(path);
		if (path.empty())
			return;
		if (!Text::endsWith(path, single ? ".png" : ".gif", true))
			path += single ? ".png" : ".gif";

		// Save.
		if (single) {
			// Get the only frame.
			const Frame &frame = _frames.front();
			Image::Ptr img(Image::create());

			toImage(frame, img, _width, _height, _drawCursor ? _cursorImage : nullptr); // Decompress and load the frame to an image object.

			img->toBytes(_cache.get(), "png"); // Save the image object to cache as PNG.

			// Save to PNG file.
			File::Ptr file(File::create());
			if (file->open(path.c_str(), Stream::WRITE)) {
				file->writeBytes(_cache.get()); // Save the png cache to file.
				file->close();
			}

			// Clear the cache.
			_cache->clear();
		} else {
			// Prepare.
			const long long tmStart = DateTime::ticks();

			ImageArray imgArray;
			Mutex imgArrayLock;
			WorkQueue* workQueue = WorkQueue::create();
			const int threads = Math::clamp(Platform::cpuCount(), 4, 16);
			workQueue->startup("RECORDER", threads);
			constexpr const int STEP = 10;

			// Get the initial.
			const Colour DEFAULT_COLORS_ARRAY[] = INDEXED_DEFAULT_COLORS;
			const int COLOR_COUNT = Math::min((int)GBBASIC_COUNTOF(DEFAULT_COLORS_ARRAY), 255);
			Colours colors;
			for (int i = 0; i < 16; ++i)
				colors.push_back(DEFAULT_COLORS_ARRAY[i]); // Fill the color palette from the default pattern.

			// Fill colors with some frames.
			struct Less {
				bool operator () (const Colour &left, const Colour &right) const {
					return left.toRGBA() < right.toRGBA();
				}
			};
			std::set<Colour, Less> filled;
			auto fillColor = [&] (Image::Ptr &img, const Frame &frame) -> void {
				if ((int)colors.size() >= COLOR_COUNT)
					return;

				toImage(frame, img, _width, _height, _drawCursor ? _cursorImage : nullptr);

				for (int j = 0; j < img->height(); ++j) {
					for (int i = 0; i < img->width(); ++i) {
						Colour col;
						img->get(i, j, col);
						auto it = filled.find(col);
						if (it == filled.end()) {
							filled.insert(col);
							colors.push_back(col); // Fill the color palette from the real frame.
							if ((int)colors.size() >= COLOR_COUNT)
								break;
						}
					}
					if ((int)colors.size() >= COLOR_COUNT)
						break;

					Platform::idle();
				}
			};

			if (_frames.size() >= 1) {
				const Frame &frame = _frames.front();
				Image::Ptr img(Image::create());
				fillColor(img, frame);
			}
			if (_frames.size() >= 2) {
				const Frame &frame = _frames.back();
				Image::Ptr img(Image::create());
				fillColor(img, frame);
			}
			if (_frames.size() >= 3) {
				Frame::List::iterator it = _frames.begin();
				std::advance(it, _frames.size() / 2);
				const Frame &frame = *it;
				Image::Ptr img(Image::create());
				fillColor(img, frame);
			}

			while ((int)colors.size() < COLOR_COUNT)
				colors.push_back(Colour(0x80, 0x80, 0x80)); // Fill the color palette with gray.

			// Create a GIF object.
			std::string osstr = Unicode::toOs(path);
			jo_gif_t gif = jo_gif_start( // Start a GIF encoding.
				osstr.c_str(),
				(short)_width, (short)_height,
				0, COLOR_COUNT
			);

			// Fill palette.
			for (int i = 0; i < COLOR_COUNT; ++i) {
				const Colour col = colors[i];
				gif.palette[i * 3 + 0] = col.r;
				gif.palette[i * 3 + 1] = col.g;
				gif.palette[i * 3 + 2] = col.b;
			}

			const long long tmColorFilled = DateTime::ticks();

			// Stream to image frames.
			const short INTERVAL = Math::max((short)((1.0f / _fps) * 100 * (RECORDER_SKIP_FRAME_COUNT + 1)), (short)1);

			for (int i = 0; i < (int)_frames.size(); ++i) {
				Image::Ptr img(Image::create());
				imgArray.push_back(img);
			}
			imgArray.shrink_to_fit();

			int index = 0;
			for (const Frame &frame : _frames) {
				workQueue->push(
					WorkTaskFunction::create(
						std::bind(
							[frame] (WorkTask* /* task */, int index, ImageArray* imgArrayPtr, Mutex* imgArrayLockPtr, int width, int height, const Image* cursorImage) -> uintptr_t { // On work thread.
								LockGuard<decltype(*imgArrayLockPtr)> guard(*imgArrayLockPtr);

								Image::Ptr &img = (*imgArrayPtr)[index];

								toImage(frame, img, width, height, cursorImage); // Decompress and load a frame to an image object.

								return 0;
							},
							std::placeholders::_1, index, &imgArray, &imgArrayLock, _width, _height, _drawCursor ? _cursorImage : nullptr
						),
						[] (WorkTask* /* task */, uintptr_t) -> void { // On main thread.
							// Do nothing.
						},
						[] (WorkTask* task, uintptr_t) -> void { // On main thread.
							task->disassociated(true);
						}
					)
				);
				++index;
			}
			while (!workQueue->allProcessed()) {
				workQueue->update();

				DateTime::sleep(STEP);
				Platform::idle();
			}

			const long long tmFramesGenerated = DateTime::ticks();

			// Stream to GIF.
			workQueue->push(
				WorkTaskFunction::create(
					std::bind(
						[INTERVAL] (WorkTask* /* task */, ImageArray* imgArrayPtr, jo_gif_t* gifPtr) -> uintptr_t { // On work thread.
							for (Image::Ptr img : *imgArrayPtr) {
								jo_gif_frame(gifPtr, img->pixels(), INTERVAL, false, true); // Add a GIF frame.

								Platform::idle();
							}

							return 0;
						},
						std::placeholders::_1, &imgArray, &gif
					),
					[] (WorkTask* /* task */, uintptr_t) -> void { // On main thread.
						// Do nothing.
					},
					[] (WorkTask* task, uintptr_t) -> void { // On main thread.
						task->disassociated(true);
					}
				)
			);
			while (!workQueue->allProcessed()) {
				workQueue->update();

				DateTime::sleep(STEP);
				Platform::idle();
			}
			const int n = (int)imgArray.size();

			// Finish streaming to GIF.
			jo_gif_end(&gif); // End the GIF encoding.

			// Finish.
			workQueue->shutdown();
			WorkQueue::destroy(workQueue);
			workQueue = nullptr;

			const long long tmGifEncoded = DateTime::ticks();

			const long long diffColorFilled = tmColorFilled - tmStart;
			const long long diffFramesGenerated = tmFramesGenerated - tmColorFilled;
			const long long diffGifEncoded = tmGifEncoded - tmFramesGenerated;
			const double secsColorFilled = DateTime::toSeconds(diffColorFilled);
			const double secsFramesGenerated = DateTime::toSeconds(diffFramesGenerated);
			const double secsGifEncoded = DateTime::toSeconds(diffGifEncoded);
			const std::string msg = "Recorded " + Text::toString(n) + " frames in " + Text::toString(secsColorFilled + secsFramesGenerated + secsGifEncoded) + "s.\n" +
				"  Color filled in " + Text::toString(secsColorFilled) + "s.\n" +
				"  Frames generated in " + Text::toString(secsFramesGenerated) + "s.\n" +
				"  GIF encoded in " + Text::toString(secsGifEncoded) + "s.";
			_onPrint(msg.c_str());
		}

		// Finish.
#if !defined GBBASIC_OS_HTML
		FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
		if (fileInfo->exists()) {
			const std::string osstr = Unicode::toOs(fileInfo->parentPath());
			Platform::browse(osstr.c_str());
		}
#endif /* Platform macro. */
	}

	void clear(void) {
		_frameSkipping = 0;

		_width = 0;
		_height = 0;

		_recording = 0;
		_frames.clear();

		_cache = nullptr;
		_footprint = 0;
	}

	static void toImage(const Frame &frame, Image::Ptr img, int width, int height, const Image* cursorImage) {
		constexpr const Byte IMAGE_COLORED_HEADER_BYTES[] = IMAGE_COLORED_HEADER;
		const size_t headerSize = GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES) +
			sizeof(int) + sizeof(int) +
			sizeof(int);
		Bytes::Ptr cache(Bytes::create());
		cache->resize(headerSize + width * height * sizeof(Colour));
		for (int i = 0; i < GBBASIC_COUNTOF(IMAGE_COLORED_HEADER_BYTES); ++i)
			cache->writeUInt8(IMAGE_COLORED_HEADER_BYTES[i]);
		cache->writeInt32(width);
		cache->writeInt32(height);
		cache->writeInt32(0);

		const Bytes::Ptr &compressed = frame.compressed;
		const int n = LZ4_decompress_safe(
			(const char*)compressed->pointer(), (char*)cache->pointer() + headerSize,
			(int)compressed->count(), (int)cache->count() - headerSize
		);
		(void)n;
		GBBASIC_ASSERT(n && n == (int)(cache->count() - headerSize)); (void)n;

		img->fromBytes(cache.get());
		cache->clear();

		if (cursorImage) {
			if (frame.cursor != Math::Vec2i(-1, -1)) {
				cursorImage->blit(
					img.get(),
					frame.cursor.x, frame.cursor.y,
					cursorImage->width(), cursorImage->height(),
					0, 0,
					false, false,
					frame.pressed ? 200 : 255
				);
			}
		}
	}
};

Recorder::~Recorder() {
}

Recorder* Recorder::create(class Input* input, unsigned fps, SaveHandler save, PrintHandler onPrint) {
	RecorderImpl* result = new RecorderImpl(input, fps, save, onPrint);

	return result;
}

void Recorder::destroy(Recorder* ptr) {
	RecorderImpl* impl = static_cast<RecorderImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
