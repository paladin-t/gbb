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

#ifndef VRAM_DEBUGGER_MAX_WIDTH
#	define VRAM_DEBUGGER_MAX_WIDTH 256.0f
#endif /* VRAM_DEBUGGER_MAX_WIDTH */

#ifndef VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT
#	define VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT 1
#endif /* VRAM_DEBUGGER_FILLING_SKIP_FRAME_COUNT */

#ifndef VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK
#	define VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK 16
#endif /* VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK */
#ifndef VRAM_DEBUGGER_TILES_AREA_HEIGHT
#	define VRAM_DEBUGGER_TILES_AREA_HEIGHT 24
#endif /* VRAM_DEBUGGER_TILES_AREA_HEIGHT */

#ifndef VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT
#	define VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT 8
#endif /* VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT */

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
		Image::Ptr image = nullptr;
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

			image = Image::Ptr(Image::create());
			image->fromBlank(width, height, 0);
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

			image->set(x, y, col);
		}
	};

	typedef std::array<Device::Obj, DEVICE_OBJ_COUNT> Objs;
	typedef std::array<bool, DEVICE_OBJ_COUNT> ObjVisibilities;

private:
	bool _opened = false;

	BufferTexture _bufferTextureTiles;
	BufferTexture _bufferTextureBgMap;
	Objs _objs;
	ObjVisibilities _objVisibilities;

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
		Device::MapSourceTypes mapSrc = device->getMapSourceType(Device::LayerTypes::BG);

		Device::TileBuffer tilesBuf;
		device->getTileBuffer(tilesBuf);

		Device::MapBuffer mapBuf;
		device->getMapBuffer(mapSrc, mapBuf);
		Device::MapBuffer mapAttrBuf;
		if (device->deviceHasCgbSupport())
			device->getMapAttrBuffer(mapSrc, mapAttrBuf);

		UInt8 bgX, bgY;
		device->getBgScroll(&bgX, &bgY);
		UInt8 wndX, wndY;
		device->getWindowScroll(&wndX, &wndY);

		const bool is8x16Obj = device->is8x16Obj();
		for (int i = 0; i < DEVICE_OBJ_COUNT; ++i) {
			_objs[i] = device->getObj(i);
			_objVisibilities[i] = device->isObjVisible(&_objs[i]);
		}

		// Bake the palette.
		// TODO

		// Translate data.
		if (_bufferTextureTiles.begin(rnd)) {
			for (int k = 0; k < (int)tilesBuf.size(); ++k) {
				const std::div_t kdiv = std::div(k, DEVICE_TILE_BUFFER_WIDTH);
				const int kx = kdiv.rem / GBBASIC_TILE_SIZE;
				const int ky = kdiv.quot / GBBASIC_TILE_SIZE;
				const int kidx = kx + ky * (DEVICE_TILE_BUFFER_WIDTH / GBBASIC_TILE_SIZE);

				const int bank = (k < (int)tilesBuf.size() / 2) ? 0 : 1;
				const std::div_t tdiv = std::div(kidx, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
				const int tx = tdiv.rem;
				const int ty = tdiv.quot % VRAM_DEBUGGER_TILES_AREA_HEIGHT;
				const int px = kdiv.rem % GBBASIC_TILE_SIZE;
				const int py = kdiv.quot % GBBASIC_TILE_SIZE;
				/*const bool isForBg = ty >= VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT;
				const bool isForObj = ty < VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT * 2;
				const UInt8 bgTile = isForBg ?
					((ty < VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT * 2) ?
						ty :
						(ty - VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT * 2)) :
					0;
				const UInt8 objTile = isForObj ?
					ty :
					0;*/

				const int x = (tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE + px;
				const int y = ty * GBBASIC_TILE_SIZE + py;

				const UInt8 val = tilesBuf[k];
				const Colour col = device->classicPalette(val);
				_bufferTextureTiles.set(rnd, x, y, col); // Copy to the tiles texture, and write to the tiles image.
			}

			_bufferTextureTiles.end(rnd);
		}

		if (_bufferTextureBgMap.begin(rnd)) {
			for (int k = 0; k < (int)mapBuf.size(); ++k) {
				const std::div_t kdiv = std::div(k, DEVICE_MAP_BUFFER_WIDTH);
				const int kx = kdiv.rem;
				const int ky = kdiv.quot;

				const int x = kx * GBBASIC_TILE_SIZE;
				const int y = ky * GBBASIC_TILE_SIZE;

				const UInt8 tile = mapBuf[k];
				const int bank = 0; // TODO: bank.
				const bool xFlip = false; // TODO
				const bool yFlip = false; // TODO
				const std::div_t sdiv = std::div(tile, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
				const int sx = (sdiv.rem + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE;
				const int sy = (
					((sdiv.quot < VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT) ?
						(sdiv.quot + VRAM_DEBUGGER_TILES_AREA_SECTION_HEIGHT * 2) :
						sdiv.quot) *
					GBBASIC_TILE_SIZE
				);

				_bufferTextureTiles.image->blit( // Blit to the BG map image from the tiles image.
					_bufferTextureBgMap.image.get(),
					x, y, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
					sx, sy,
					xFlip, yFlip
				);
			}
			memcpy( // Copy to the BG map texture from the BG map image.
				_bufferTextureBgMap.pixels,
				_bufferTextureBgMap.image->pixels(),
				DEVICE_MAP_BUFFER_SIZE * (GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE) * sizeof(Colour)
			);

			_bufferTextureBgMap.end(rnd);
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

		const ImVec2 dstSize(VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 * GBBASIC_TILE_SIZE, VRAM_DEBUGGER_TILES_AREA_HEIGHT * GBBASIC_TILE_SIZE);
		if (regSize.x < VRAM_DEBUGGER_MAX_WIDTH) {
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

			const ImVec2 frameSize = ImVec2(regSize.x, dstSize.y + style.ScrollbarSize) + ImVec2(style.WindowBorderSize * 2, style.WindowBorderSize * 2 + 1);
			ImGui::BeginChildFrame(
				ImGui::GetID("@Tls"),
				frameSize,
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			);
			{
				ImGui::Image(
					_bufferTextureTiles.texture->pointer(rnd),
					dstSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
				);
				// TODO: grids.
				// TODO: tooltips.
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
			}
			ImGui::EndChildFrame();
		} else {
			ImGui::Image(
				_bufferTextureTiles.texture->pointer(rnd),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);
			// TODO: grids.
			// TODO: tooltips.
		}
	}
	void bgMap(Window* /* wnd */, Renderer* rnd, Theme* theme, Device* /* device */) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_BgMap());

		const ImVec2 dstSize(DEVICE_MAP_BUFFER_WIDTH * GBBASIC_TILE_SIZE, DEVICE_MAP_BUFFER_HEIGHT * GBBASIC_TILE_SIZE);
		if (regSize.x < VRAM_DEBUGGER_MAX_WIDTH) {
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

			const ImVec2 frameSize = ImVec2(regSize.x, dstSize.y + style.ScrollbarSize) + ImVec2(style.WindowBorderSize * 2, style.WindowBorderSize * 2 + 1);
			ImGui::BeginChildFrame(
				ImGui::GetID("@Map"),
				frameSize,
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			);
			{
				ImGui::Image(
					_bufferTextureBgMap.texture->pointer(rnd),
					dstSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
				);
				// TODO: grids.
				// TODO: tooltips.
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
			}
			ImGui::EndChildFrame();
		} else {
			ImGui::Image(
				_bufferTextureBgMap.texture->pointer(rnd),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);
			// TODO: grids.
			// TODO: tooltips.
		}
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
