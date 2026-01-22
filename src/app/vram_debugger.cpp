/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "theme.h"
#include "vram_debugger.h"
#include "widgets.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT
#	define VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT 1
#endif /* VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT */

#ifndef VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK
#	define VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK 16
#endif /* VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK */
#ifndef VRAM_DEBUGGER_TILES_AREA_HEIGHT
#	define VRAM_DEBUGGER_TILES_AREA_HEIGHT 24
#endif /* VRAM_DEBUGGER_TILES_AREA_HEIGHT */

static_assert(
	(DEVICE_TILE_BUFFER_WIDTH * DEVICE_TILE_BUFFER_HEIGHT) ==
		(VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * VRAM_DEBUGGER_TILES_AREA_HEIGHT * 2 * (GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE)),
	"Wrong data."
);

/* ===========================================================================} */

/*
** {===========================================================================
** VRAM debugger
*/

class VramDebuggerImpl : public VramDebugger {
private:
	struct BufferTexture {
		Texture::Ptr texture = nullptr;
		int width = 0;
		int height = 0;
		void* pixels = nullptr;

		unsigned frameTick = 0;

		BufferTexture() {
		}

		void touch(Renderer* rnd, int width_, int height_) {
			width = width_;
			height = height_;
			if (texture && (width != texture->width() || height != texture->height()))
				texture = nullptr;

			if (texture)
				return;

			texture = Texture::Ptr(Texture::create());
			texture->scale(Texture::NEAREST);
			texture->fromBytes(rnd, Texture::STREAMING, nullptr, width, height, 0, Texture::NEAREST);
		}

		bool begin(Renderer* /* rnd */) {
#if VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT == 1
			constexpr const bool result = true;
#else /* VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT */
			const bool result = (frameTick++ % VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT) == 0;
#endif /* VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT */

			GBBASIC_ASSERT(texture && "Wrong data.");

			int pitch = 0;
			texture->lock(nullptr, &pixels, &pitch);

			return result;
		}
		void end(Renderer* /* rnd */) {
			GBBASIC_ASSERT(texture && "Wrong data.");

			pixels = nullptr;
			texture->unlock();
		}

		void set(Renderer* /* rnd */, int x, int y, const Colour &col) {
			GBBASIC_ASSERT(texture && "Wrong data.");

			Colour* ptr = (Colour*)pixels;
			ptr[x + y * width] = col;
		}
	};

private:
	bool _opened = false;

	BufferTexture _bufferTextureTiles;
	BufferTexture _bufferTextureBgMap;

public:
	VramDebuggerImpl() {
	}
	virtual ~VramDebuggerImpl() {
		close();
	}

	virtual bool open(class Window* /* wnd */, class Renderer* rnd, class Theme* /* theme */) override {
		if (_opened)
			return true;

		_bufferTextureTiles.touch(rnd, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 * GBBASIC_TILE_SIZE, VRAM_DEBUGGER_TILES_AREA_HEIGHT * GBBASIC_TILE_SIZE);
		_bufferTextureBgMap.touch(rnd, DEVICE_MAP_BUFFER_WIDTH * GBBASIC_TILE_SIZE, DEVICE_MAP_BUFFER_HEIGHT * GBBASIC_TILE_SIZE);

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		return true;
	}

	virtual void update(class Window* wnd, class Renderer* rnd, class Theme* theme, class Device* device) override {
		refresh(wnd, rnd, theme, device);

		tiles(wnd, rnd, theme, device);
		ImGui::NewLine(1);
		ImGui::Separator();

		bgMap(wnd, rnd, theme, device);
		ImGui::NewLine(1);
		ImGui::Separator();

		oam(wnd, rnd, theme, device);
		ImGui::NewLine(1);
		ImGui::Separator();

		palettes(wnd, rnd, theme, device);
	}

private:
	void refresh(Window* /* wnd */, Renderer* rnd, Theme* /* theme */, Device* device) {
		// Retrieve data.
		Device::TileBuffer buf;
		device->getTileBuffer(buf);

		// Translate data.
		if (_bufferTextureTiles.begin(rnd)) {
			for (int k = 0; k < (int)buf.size(); ++k) {
				const std::div_t kdiv = std::div(k, DEVICE_TILE_BUFFER_WIDTH);
				const int kx = kdiv.rem / GBBASIC_TILE_SIZE;
				const int ky = kdiv.quot / GBBASIC_TILE_SIZE;
				const int kidx = kx + ky * (DEVICE_TILE_BUFFER_WIDTH / GBBASIC_TILE_SIZE);

				const int bank = (k < (int)buf.size() / 2) ? 0 : 1;
				const std::div_t tdiv = std::div(kidx, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
				const int tx = tdiv.rem;
				const int ty = tdiv.quot % VRAM_DEBUGGER_TILES_AREA_HEIGHT;
				const int px = kdiv.rem % GBBASIC_TILE_SIZE;
				const int py = kdiv.quot % GBBASIC_TILE_SIZE;

				const int x = (tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE + px;
				const int y = ty * GBBASIC_TILE_SIZE + py;

				const Colour col = device->classicPalette(buf[k]);
				_bufferTextureTiles.set(rnd, x, y, col);
			}

			_bufferTextureTiles.end(rnd);
		}
	}

	void tiles(Window* /* wnd */, Renderer* rnd, Theme* theme, Device* /* device */) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_Tiles());

		const ImVec2 dstSize(DEVICE_TILE_BUFFER_WIDTH, DEVICE_TILE_BUFFER_HEIGHT);
		ImGui::Image(
			_bufferTextureTiles.texture->pointer(rnd),
			dstSize,
			ImVec2(0, 0), ImVec2(1, 1),
			ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
		);
	}
	void bgMap(Window* /* wnd */, Renderer* /* rnd */, Theme* theme, Device* /* device */) {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_BgMap());

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("// TODO");
	}
	void oam(Window* /* wnd */, Renderer* /* rnd */, Theme* theme, Device* /* device */) {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_Oam());

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("// TODO");
	}
	void palettes(Window* /* wnd */, Renderer* /* rnd */, Theme* theme, Device* /* device */) {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_Palettes());

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("// TODO");
	}
};

VramDebugger* VramDebugger::create(void) {
	VramDebuggerImpl* result = new VramDebuggerImpl();

	return result;
}

void VramDebugger::destroy(VramDebugger* ptr) {
	VramDebuggerImpl* impl = static_cast<VramDebuggerImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
