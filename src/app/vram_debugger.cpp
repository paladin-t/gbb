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

#ifndef VRAM_DEBUGGER_TILES_SECTION_WIDTH
#	define VRAM_DEBUGGER_TILES_SECTION_WIDTH 16
#endif /* VRAM_DEBUGGER_TILES_SECTION_WIDTH */
#ifndef VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT
#	define VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT 8
#endif /* VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT */
#ifndef VRAM_DEBUGGER_TILES_SECTION_SIZE
#	define VRAM_DEBUGGER_TILES_SECTION_SIZE (VRAM_DEBUGGER_TILES_SECTION_WIDTH * VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2)
#endif /* VRAM_DEBUGGER_TILES_SECTION_SIZE */

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

		// Creates a texture and image if they do not exist yet.
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

		// Begins rendering to the texture with locking it.
		bool begin(void) {
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
		// Ends rendering with unlocking the texture.
		void end(void) {
			GBBASIC_ASSERT(texture && "Wrong data.");

			pixels = nullptr;
			texture->unlock();
		}

		// Plots a pixel, copies to the texture, and write to the image.
		void plot(int x, int y, const Colour &col) {
			GBBASIC_ASSERT(texture && "Wrong data.");

			Colour* ptr = (Colour*)pixels;
			ptr[x + y * width] = col;

			image->set(x, y, col);
		}

		// Blits to the image from another image.
		void blit(BufferTexture &other, int x, int y, int w, int h, int sx, int sy, bool hFlip, bool vFlip) const {
			GBBASIC_ASSERT(image && other.image && "Wrong data.");

			image->blit(other.image.get(), x, y, w, h, sx, sy, hFlip, vFlip);
		}

		// Commits to the texture from the image.
		void commit(size_t size) {
			GBBASIC_ASSERT(texture && image && pixels && "Wrong data.");

			memcpy(pixels, image->pixels(), size);
		}
	};

	struct TileDetail {
		typedef std::vector<TileDetail> Array;
		typedef std::array<Array, VRAM_DEBUGGER_TILES_SECTION_SIZE> Section;
		typedef std::array<Section, 2> Bank; // For BG map and obj respectively.
		typedef std::array<Bank, 2> Banks; // For bank 0 and bank 1 respectively.

		enum class Usages {
			BG_MAP = 0,
			OBJ = 1
		};

		Usages usage = Usages::BG_MAP;
		Math::Vec2i position;
		int palette = 0;

		TileDetail() {
		}
		TileDetail(Usages use, const Math::Vec2i &pos, int pal) :
			usage(use),
			position(pos),
			palette(pal)
		{
		}

		static void clear(TileDetail::Banks &details) {
			for (int b = 0; b < (int)details.size(); ++b) {
				Bank &bank = details[b];
				for (int s = 0; s < (int)bank.size(); ++s) {
					Section &section = bank[s];
					for (int i = 0; i < (int)section.size(); ++i) {
						Array &array = section[i];
						array.clear();
					}
				}
			}
		}
	};

	struct Obj {
		typedef std::array<Obj, DEVICE_OBJ_COUNT> Array;

		Device::Obj obj;
		bool visible = false;
	};

private:
	bool _opened = false;

	TileDetail::Banks _tileDetails;
	struct {
		BufferTexture buffer;
	} _tiles;
	struct {
		BufferTexture buffer;
	} _bgMap;
	struct {
		Obj::Array buffer;
	} _objs;

public:
	VramDebuggerImpl() {
	}
	virtual ~VramDebuggerImpl() {
		close();
	}

	virtual bool open(class Renderer* rnd, class Theme* /* theme */) override {
		if (_opened)
			return true;

		_tiles.buffer.touch(rnd, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 * GBBASIC_TILE_SIZE, VRAM_DEBUGGER_TILES_AREA_HEIGHT * GBBASIC_TILE_SIZE);
		_bgMap.buffer.touch(rnd, DEVICE_MAP_BUFFER_WIDTH * GBBASIC_TILE_SIZE, DEVICE_MAP_BUFFER_HEIGHT * GBBASIC_TILE_SIZE);

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		return true;
	}

	virtual void update(class Renderer* rnd, class Theme* theme, class Device* device) override {
		refresh(rnd, theme, device);

		tiles(rnd, theme, device);
		ImGui::NewLine(1);
		ImGui::Separator();

		bgMap(rnd, theme, device);
		ImGui::NewLine(1);
		ImGui::Separator();

		oam(rnd, theme, device);
		ImGui::NewLine(1);
		ImGui::Separator();

		palettes(rnd, theme, device);
	}

private:
	void refresh(Renderer* /* rnd */, Theme* /* theme */, Device* device) {
		// Retrieve data.
		const bool isCgb = device->deviceHasCgbSupport();

		Device::MapSourceTypes mapSrc = device->getMapSourceType(Device::LayerTypes::BG);

		Device::TileBuffer tilesBuf;
		device->getTileBuffer(tilesBuf);

		Device::MapBuffer mapBuf;
		device->getMapBuffer(mapSrc, mapBuf);
		Device::MapBuffer mapAttrBuf;
		if (isCgb)
			device->getMapAttrBuffer(mapSrc, mapAttrBuf);

		UInt8 bgX, bgY;
		device->getBgScroll(&bgX, &bgY);
		UInt8 wndX, wndY;
		device->getWindowScroll(&wndX, &wndY);

		const bool is8x16Obj = device->is8x16Obj();
		for (int i = 0; i < DEVICE_OBJ_COUNT; ++i) {
			_objs.buffer[i].obj = device->getObj(i);
			_objs.buffer[i].visible = device->isObjVisible(&_objs.buffer[i].obj);
		}

		// Collect the reference information, palettes, etc.
		TileDetail::clear(_tileDetails);
		for (int k = 0; k < (int)mapBuf.size(); ++k) {
			const std::div_t kdiv = std::div(k, DEVICE_MAP_BUFFER_WIDTH);
			const int kx = kdiv.rem;
			const int ky = kdiv.quot;

			const int mx = kx;
			const int my = ky;

			const UInt8 tile = mapBuf[k];
			const UInt8 attrs = mapAttrBuf[k];
			const int bank = !!((attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001) ? 1 : 0;
			const int plt = attrs & ((0x00000001 << GBBASIC_MAP_PALETTE_BIT0) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT1) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT2));

			const TileDetail detail(TileDetail::Usages::BG_MAP, Math::Vec2i(mx, my), plt);
			_tileDetails[bank][(int)TileDetail::Usages::BG_MAP][tile].push_back(detail);
		}

		// Translate data.
		if (_tiles.buffer.begin()) {
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

				const bool isForObj = ty < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2;
				const bool isForBg = (ty >= VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT) ||
					!isForObj; // Must be for BG if is not for obj.
				const UInt8 bgTile = isForBg ?
					(UInt8)((ty < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2) ?
						ty :
						(ty - VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2)) :
					0;
				const UInt8 objTile = isForObj ?
					(UInt8)ty :
					0;
				const TileDetail::Array* details = nullptr;
				if (isForBg) {
					const UInt8 tile = (UInt8)(tx + bgTile * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
					details = &_tileDetails[bank][(int)TileDetail::Usages::BG_MAP][tile];
				} else /* if (isForObj) */ {
					const UInt8 tile = (UInt8)(tx + objTile * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
					details = &_tileDetails[bank][(int)TileDetail::Usages::OBJ][tile];
				}

				const int x = (tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE + px;
				const int y = ty * GBBASIC_TILE_SIZE + py;

				const UInt8 val = tilesBuf[k];
				Colour col;
				if (details && !details->empty()) {
					if (isCgb) {
						const TileDetail &detail = details->front();
						const Device::PaletteRgba &pltRgba = device->getCgbPaletteRgba(
							detail.usage == TileDetail::Usages::BG_MAP ? Device::CgbPaletteTypes::BGCP : Device::CgbPaletteTypes::OBCP,
							detail.palette
						);
						const UInt32 rgba = pltRgba.color[val];
						col.fromRGBA(rgba);
					} else {
						const Device::PaletteRgba &pltRgba = device->getPaletteRgba(
							Device::PaletteTypes::BGP
						);
						const UInt32 rgba = pltRgba.color[val];
						col.fromRGBA(rgba);
					}
				} else {
					const Colour col_ = device->classicPalette(val);
					const UInt8 gray = (UInt8)Math::clamp(col_.toGray(), 0, 255);
					col = Colour(gray, gray, gray);
				}
				_tiles.buffer.plot(x, y, col); // Copy to the tiles texture, and write to the tiles image.
			}

			_tiles.buffer.end();
		}

		if (_bgMap.buffer.begin()) {
			for (int k = 0; k < (int)mapBuf.size(); ++k) {
				const std::div_t kdiv = std::div(k, DEVICE_MAP_BUFFER_WIDTH);
				const int kx = kdiv.rem;
				const int ky = kdiv.quot;

				const int x = kx * GBBASIC_TILE_SIZE;
				const int y = ky * GBBASIC_TILE_SIZE;

				const UInt8 tile = mapBuf[k];
				const UInt8 attrs = mapAttrBuf[k];
				const int bank = !!((attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001) ? 1 : 0;
				const bool hFlip = !!((attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001);
				const bool vFlip = !!((attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001);
				const std::div_t sdiv = std::div(tile, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
				const int sx = (sdiv.rem + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE;
				const int sy = (
					((sdiv.quot < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT) ?
						(sdiv.quot + VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2) :
						sdiv.quot) *
					GBBASIC_TILE_SIZE
				);

				_tiles.buffer.blit( // Blit to the BG map image from the tiles image.
					_bgMap.buffer,
					x, y, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
					sx, sy,
					hFlip, vFlip
				);
			}
			_bgMap.buffer.commit( // Commits to the BG map texture from the BG map image.
				DEVICE_MAP_BUFFER_SIZE * (GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE) * sizeof(Colour)
			);

			_bgMap.buffer.end();
		}
	}

	void tiles(Renderer* rnd, Theme* theme, Device* /* device */) {
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

			// TODO: options.

			const ImVec2 frameSize = ImVec2(regSize.x, dstSize.y + style.ScrollbarSize) + ImVec2(style.WindowBorderSize * 2, style.WindowBorderSize * 2 + 1);
			ImGui::BeginChildFrame(
				ImGui::GetID("@Tls"),
				frameSize,
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			);
			{
				ImGui::Image(
					_tiles.buffer.texture->pointer(rnd),
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
				_tiles.buffer.texture->pointer(rnd),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);
			// TODO: grids.
			// TODO: tooltips.
		}
	}
	void bgMap(Renderer* rnd, Theme* theme, Device* /* device */) {
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

			// TODO: options.

			const ImVec2 frameSize = ImVec2(regSize.x, dstSize.y + style.ScrollbarSize) + ImVec2(style.WindowBorderSize * 2, style.WindowBorderSize * 2 + 1);
			ImGui::BeginChildFrame(
				ImGui::GetID("@Map"),
				frameSize,
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			);
			{
				ImGui::Image(
					_bgMap.buffer.texture->pointer(rnd),
					dstSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
				);
				// TODO: grids.
				// TODO: tooltips.
				// TODO: camera.
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
			}
			ImGui::EndChildFrame();
		} else {
			ImGui::Image(
				_bgMap.buffer.texture->pointer(rnd),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);
			// TODO: grids.
			// TODO: tooltips.
			// TODO: camera.
		}
	}
	void oam(Renderer* /* rnd */, Theme* theme, Device* /* device */) {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_Oam());

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("// TODO");
	}
	void palettes(Renderer* /* rnd */, Theme* theme, Device* /* device */) {
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
