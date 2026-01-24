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
#include "../utils/datetime.h"
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

		// Plots a pixel to the texture, and write to the image.
		void plot(int x, int y, const Colour &col) {
			GBBASIC_ASSERT(texture && image && "Wrong data.");
			GBBASIC_ASSERT(pixels && "Locking required.");

			Colour* ptr = (Colour*)pixels;
			ptr[x + y * width] = col;

			image->set(x, y, col);
		}

		// Draws a pixel to the image.
		void draw(int x, int y, const Colour &col) {
			GBBASIC_ASSERT(image && "Wrong data.");

			image->set(x, y, col);
		}

		// Blits to the image from another image.
		void blit(BufferTexture &other, int x, int y, int w, int h, int sx, int sy, bool hFlip, bool vFlip) const {
			GBBASIC_ASSERT(image && other.image && "Wrong data.");

			image->blit(other.image.get(), x, y, w, h, sx, sy, hFlip, vFlip);
		}

		// Commits to the texture from the image.
		void commit(size_t size) {
			GBBASIC_ASSERT(texture && image && "Wrong data.");
			GBBASIC_ASSERT(pixels && "Locking required.");

			memcpy(pixels, image->pixels(), size);
		}
	};

	struct TileDetail {
		typedef std::vector<TileDetail> Array;
		struct Ref {
			Array details;
			int refCount = 0;
		};
		typedef std::array<Ref, VRAM_DEBUGGER_TILES_SECTION_SIZE> Section;
		typedef std::array<Section, 2> Bank; // For BG map and OBJ respectively.
		typedef std::array<Bank, 2> Banks; // For bank 0 and bank 1 respectively.

		enum class Usages {
			BG_MAP = 0,
			OBJ
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
						Ref &ref = section[i];
						ref.details.clear();
					}
				}
			}
		}
	};

	struct TilesBuffer {
		BufferTexture buffer;
		Math::Vec2i highlight = Math::Vec2i(-1, -1);

		void touch(Renderer* rnd) {
			buffer.touch(rnd, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 * GBBASIC_TILE_SIZE, VRAM_DEBUGGER_TILES_AREA_HEIGHT * GBBASIC_TILE_SIZE);
		}
	};

	struct MapBuffer {
		typedef std::vector<Math::Vec2i> Points;

		BufferTexture buffer;
		Points highlights;

		void touch(Renderer* rnd) {
			buffer.touch(rnd, DEVICE_MAP_BUFFER_WIDTH * GBBASIC_TILE_SIZE, DEVICE_MAP_BUFFER_HEIGHT * GBBASIC_TILE_SIZE);
		}
	};

	struct Obj {
		typedef std::array<Obj, DEVICE_OBJ_COUNT> Array;

		Device::Obj obj;
		bool visible = false;
	};
	struct ObjBuffer {
		Obj::Array buffer;
	};

	struct Palette {
		typedef std::array<Palette, 3> Collection; // For BG, OBJ0, and OBJ1 respectively.

		Colour color[4];
	};
	struct CgbPalette {
		typedef std::array<CgbPalette, 8> Group;
		typedef std::array<Group, 2> Collection; // For BG and OBJ respectively.

		Colour color[4];
	};

private:
	bool _opened = false;
	struct {
		bool isBgLayerActive = true;
	} _options;

	Palette::Collection _palettes;
	CgbPalette::Collection _cgbPalettes;
	Device::MapBuffer _bgMapBuf;
	Device::MapBuffer _bgMapAttrBuf;
	Device::MapBuffer _winMapBuf;
	Device::MapBuffer _winMapAttrBuf;

	TileDetail::Banks _tileDetails;
	TilesBuffer _tiles;
	MapBuffer _bgMap;
	MapBuffer _winMap;
	ObjBuffer _objs;

public:
	VramDebuggerImpl() {
	}
	virtual ~VramDebuggerImpl() {
		close();
	}

	virtual bool open(class Renderer* rnd, class Theme* /* theme */) override {
		if (_opened)
			return true;

		_tiles.touch(rnd);
		_bgMap.touch(rnd);
		_winMap.touch(rnd);

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		return true;
	}

	virtual void update(
		class Renderer* rnd, class Theme* theme, class Device* device,
		bool previewPaletteBits, bool showGrids
	) override {
		refresh(rnd, theme, device, previewPaletteBits);

		tiles(rnd, theme, device, showGrids);
		ImGui::NewLine(2);
		ImGui::Separator();

		bgMap(rnd, theme, device, showGrids);
		ImGui::NewLine(2);
		ImGui::Separator();

		oam(rnd, theme, device);
		ImGui::NewLine(2);
		ImGui::Separator();

		palettes(rnd, theme, device);
	}

private:
	void refresh(Renderer* /* rnd */, Theme* /* theme */, Device* device, bool previewPaletteBits) {
		// Retrieve data.
		const bool isCgb = device->deviceHasCgbSupport();

		const Device::MapSourceTypes bgMapSrc = device->getMapSourceType(Device::LayerTypes::BG);
		const Device::MapSourceTypes winMapSrc = device->getMapSourceType(Device::LayerTypes::WINDOW);

		Device::TileBuffer tilesBuf;
		device->getTileBuffer(tilesBuf);

		device->getMapBuffer(bgMapSrc, _bgMapBuf);
		if (isCgb)
			device->getMapAttrBuffer(bgMapSrc, _bgMapAttrBuf);
		else
			_bgMapAttrBuf.fill(0);

		device->getMapBuffer(winMapSrc, _winMapBuf);
		if (isCgb)
			device->getMapAttrBuffer(winMapSrc, _winMapAttrBuf);
		else
			_winMapAttrBuf.fill(0);

		const bool is8x16Obj = device->is8x16Obj();
		for (int i = 0; i < DEVICE_OBJ_COUNT; ++i) {
			_objs.buffer[i].obj = device->getObj(i);
			_objs.buffer[i].visible = device->isObjVisible(&_objs.buffer[i].obj);
		}

		for (int i = 0; i < (int)Device::PaletteTypes::COUNT; ++i) {
			const Device::PaletteRgba &pltRgba = device->getPaletteRgba((Device::PaletteTypes)i);
			for (int k = 0; k < 4; ++k) {
				const UInt32 rgba = pltRgba.color[k];
				const Colour col = Colour::byRGBA8888(rgba);
				_palettes[i].color[k] = col;
			}
		}
		for (int i = 0; i < (int)Device::CgbPaletteTypes::COUNT; ++i) {
			for (int j = 0; j < 8; ++j) {
				const Device::PaletteRgba &pltRgba = device->getCgbPaletteRgba((Device::CgbPaletteTypes)i, j);
				for (int k = 0; k < 4; ++k) {
					const UInt32 rgba = pltRgba.color[k];
					const Colour col = Colour::byRGBA8888(rgba);
					_cgbPalettes[i][j].color[k] = col;
				}
			}
		}

		// Collect the reference information, palettes, etc.
		auto collectMapInfo = [] (
			TileDetail::Banks &tileDetails,
			const Device::MapBuffer &mapBuf, const Device::MapBuffer &mapAttrBuf,
			bool refOnly
		) -> void {
			for (int k = 0; k < (int)mapBuf.size(); ++k) {
				const std::div_t kdiv = std::div(k, DEVICE_MAP_BUFFER_WIDTH);
				const int kx = kdiv.rem;
				const int ky = kdiv.quot;

				const int mx = kx;
				const int my = ky;

				const UInt8 tile = mapBuf[k];
				const UInt8 attrs = mapAttrBuf[k];
				const int plt = attrs & ((0x00000001 << GBBASIC_MAP_PALETTE_BIT0) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT1) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT2));
				const int bank = !!((attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001) ? 1 : 0;

				TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::BG_MAP][tile];
				++ref.refCount;
				if (!refOnly) {
					const TileDetail detail(TileDetail::Usages::BG_MAP, Math::Vec2i(mx, my), plt);
					ref.details.push_back(detail);
				}
			}
		};

		TileDetail::clear(_tileDetails);
		collectMapInfo(
			_tileDetails,
			_bgMapBuf, _bgMapAttrBuf,
			!_options.isBgLayerActive
		);
		collectMapInfo(
			_tileDetails,
			_winMapBuf, _winMapAttrBuf,
			_options.isBgLayerActive
		);

		// Translate data.
		auto translateTiles = [device, previewPaletteBits, isCgb] (
			TilesBuffer &tiles,
			const Device::TileBuffer &tilesBuf, const TileDetail::Banks &tileDetails,
			const Palette::Collection &palettes, const CgbPalette::Collection &cgbPalettes
		) -> void {
			if (tiles.buffer.begin()) {
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
					int refCount = 0;
					if (isForBg) {
						const UInt8 tile = (UInt8)(tx + bgTile * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
						const TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::BG_MAP][tile];
						details = &ref.details;
						refCount = ref.refCount;
					} else /* if (isForObj) */ {
						const UInt8 tile = (UInt8)(tx + objTile * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
						const TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::OBJ][tile];
						details = &ref.details;
						refCount = ref.refCount;
					}

					const int x = (tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE + px;
					const int y = ty * GBBASIC_TILE_SIZE + py;

					const UInt8 val = tilesBuf[k];
					Colour col;
					if (details && refCount) {
						if (previewPaletteBits) {
							if (isCgb && !details->empty()) {
								const TileDetail &detail = details->front(); // Use the first for paletting.
								col = cgbPalettes[(int)detail.usage][detail.palette].color[val];
							} else {
								col = palettes[(int)Device::PaletteTypes::BGP].color[val];
							}
						} else {
							col = device->classicPalette(val);
						}
					} else {
						const Colour col_ = device->classicPalette(val);
						const UInt8 gray = (UInt8)Math::clamp(col_.toGray(), 0, 255);
						col = Colour(gray, gray, gray);
					}
					tiles.buffer.plot(x, y, col); // Plot to the tiles texture, and write to the tiles image.
				}

				tiles.buffer.end();
			}
		};

		translateTiles(
			_tiles,
			tilesBuf, _tileDetails,
			_palettes, _cgbPalettes
		);

		auto getTileBuf = [] (const Device::TileBuffer &tilesBuf, int x, int y) -> UInt8 {
			const int p = x + y * DEVICE_TILE_BUFFER_WIDTH;

			return tilesBuf[p];
		};
		auto translateMap = [device, previewPaletteBits, isCgb, getTileBuf] (
			MapBuffer &map,
			const Device::MapBuffer &mapBuf, const Device::MapBuffer &mapAttrBuf,
			const Device::TileBuffer &tilesBuf, const TilesBuffer &tiles, const TileDetail::Banks &tileDetails,
			const Palette::Collection &palettes, const CgbPalette::Collection &cgbPalettes
		) -> void {
			if (map.buffer.begin()) {
				for (int k = 0; k < (int)mapBuf.size(); ++k) {
					const std::div_t kdiv = std::div(k, DEVICE_MAP_BUFFER_WIDTH);
					const int kx = kdiv.rem;
					const int ky = kdiv.quot;

					const int x = kx * GBBASIC_TILE_SIZE;
					const int y = ky * GBBASIC_TILE_SIZE;

					const UInt8 tile = mapBuf[k];
					const UInt8 attrs = mapAttrBuf[k];
					const int plt = attrs & ((0x00000001 << GBBASIC_MAP_PALETTE_BIT0) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT1) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT2));
					const int bank = !!((attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001) ? 1 : 0;
					const bool hFlip = !!((attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001);
					const bool vFlip = !!((attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001);

					bool toBlit = true;
					const TileDetail::Array &details = tileDetails[bank][(int)TileDetail::Usages::BG_MAP][tile].details;
					if (!details.empty()) {
						if (details.front().palette != plt)
							toBlit = false;
					}
					if (toBlit) {
						const std::div_t sdiv = std::div(tile, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
						const int sx = (sdiv.rem + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE;
						const int sy = (
							((sdiv.quot < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT) ?
								(sdiv.quot + VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2) :
								sdiv.quot) *
							GBBASIC_TILE_SIZE
						);

						tiles.buffer.blit( // Blit to the BG/WIN map image from the tiles image.
							map.buffer,
							x, y, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
							sx, sy,
							hFlip, vFlip
						);
					} else {
						constexpr const int RAW_TILES_SECTION_HALF_HEIGHT = ((DEVICE_TILE_BUFFER_HEIGHT / GBBASIC_TILE_SIZE) / 2 / 3);
						const std::div_t sdiv = std::div(tile, DEVICE_TILE_BUFFER_WIDTH / 8);
						const int sx = sdiv.rem * GBBASIC_TILE_SIZE;
						const int sy = (
							((sdiv.quot < RAW_TILES_SECTION_HALF_HEIGHT) ?
								(sdiv.quot + RAW_TILES_SECTION_HALF_HEIGHT * 2):
								sdiv.quot) *
							GBBASIC_TILE_SIZE +
							((DEVICE_TILE_BUFFER_HEIGHT / 2) * bank)
						);

						for (int j = 0; j < GBBASIC_TILE_SIZE; ++j) {
							for (int i = 0; i < GBBASIC_TILE_SIZE; ++i) {
								const int sx_ = sx + i;
								const int sy_ = sy + j;
								const UInt8 val = getTileBuf(tilesBuf, sx_, sy_);
								Colour col;
								if (previewPaletteBits) {
									if (isCgb) {
										col = cgbPalettes[(int)Device::PaletteTypes::BGP][plt].color[val];
									} else {
										col = palettes[(int)Device::PaletteTypes::BGP].color[val];
									}
								} else {
									col = device->classicPalette(val);
								}
								const int dx = hFlip ?
									x + (GBBASIC_TILE_SIZE - i - 1) :
									x + i;
								const int dy = vFlip ?
									y + (GBBASIC_TILE_SIZE - j - 1) :
									y + j;
								map.buffer.draw(dx, dy, col); // Draw to the BG/WIN map image.
							}
						}
					}
				}
				map.buffer.commit( // Commits to the BG/WIN map texture from the BG/WIN map image.
					DEVICE_MAP_BUFFER_SIZE * (GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE) * sizeof(Colour)
				);

				map.buffer.end();
			}
		};

		if (_options.isBgLayerActive) {
			translateMap(
				_bgMap,
				_bgMapBuf, _bgMapAttrBuf,
				tilesBuf, _tiles, _tileDetails,
				_palettes, _cgbPalettes
			);
		} else {
			translateMap(
				_winMap,
				_winMapBuf, _winMapAttrBuf,
				tilesBuf, _tiles, _tileDetails,
				_palettes, _cgbPalettes
			);
		}
	}

	void tiles(Renderer* rnd, Theme* theme, Device* /* device */, bool showGrids) {
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

		// TODO: options.

		auto drawGrids = [] (const ImVec2 &curPos, const ImVec2 &dstSize) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const float w = dstSize.x;
			const float h = dstSize.y;
			for (float i = GBBASIC_TILE_SIZE; i < w; i += GBBASIC_TILE_SIZE) {
				drawList->AddLine(
					curPos + ImVec2(i, 0),
					curPos + ImVec2(i, h),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
				);
			}
			for (float j = GBBASIC_TILE_SIZE; j < h; j += GBBASIC_TILE_SIZE) {
				drawList->AddLine(
					curPos + ImVec2(0, j),
					curPos + ImVec2(w, j),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
				);
			}
			drawList->AddRect(
				curPos + ImVec2(-1, -1),
				curPos + ImVec2(w, h) + ImVec2(1, 1),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
			);
		};
		auto drawHighlight = [] (const ImVec2 &curPos, const Math::Vec2i &pos) -> void {
			if (pos == Math::Vec2i(-1, -1))
				return;

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const ImVec2 startPos((float)(pos.x * GBBASIC_TILE_SIZE), (float)(pos.y * GBBASIC_TILE_SIZE));
			const ImVec2 endPos = startPos + ImVec2(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
			drawList->AddRect(
				curPos + startPos,
				curPos + endPos + ImVec2(1, 1),
				ImGui::GetColorU32(ImVec4(1, 0, 0, 0.75f))
			);
		};

		bool hasInfo = false;
		int infoBank = 0;
		Math::Vec2i tilePos;
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
				const ImVec2 curPos = ImGui::GetCursorScreenPos();
				ImGui::Image(
					_tiles.buffer.texture->pointer(rnd),
					dstSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
				);

				if (showGrids)
					drawGrids(curPos, dstSize);

				drawHighlight(curPos, _tiles.highlight);

				if (ImGui::IsItemHovered()) {
					const ImVec2 mousePos = ImGui::GetMousePos();
					const ImVec2 diff = mousePos - curPos;
					const ImVec2 tilePos_ = diff / GBBASIC_TILE_SIZE;
					if (tilePos_.x >= 0 && tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 && tilePos_.y >= 0 && tilePos_.y < VRAM_DEBUGGER_TILES_AREA_HEIGHT) {
						hasInfo = true;
						infoBank = tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK ? 0 : 1;
						tilePos = Math::Vec2i((int)tilePos_.x - VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * infoBank, (int)tilePos_.y);
					}
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
			}
			ImGui::EndChildFrame();
		} else {
			const ImVec2 curPos = ImGui::GetCursorScreenPos();
			ImGui::Image(
				_tiles.buffer.texture->pointer(rnd),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);

			if (showGrids)
				drawGrids(curPos, dstSize);

			drawHighlight(curPos, _tiles.highlight);

			if (ImGui::IsItemHovered()) {
				const ImVec2 mousePos = ImGui::GetMousePos();
				const ImVec2 diff = mousePos - curPos;
				const ImVec2 tilePos_ = diff / GBBASIC_TILE_SIZE;
				if (tilePos_.x >= 0 && tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 && tilePos_.y >= 0 && tilePos_.y < VRAM_DEBUGGER_TILES_AREA_HEIGHT) {
					hasInfo = true;
					infoBank = tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK ? 0 : 1;
					tilePos = Math::Vec2i((int)tilePos_.x - VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * infoBank, (int)tilePos_.y);
				}
			}
		}

		MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
		highlights.clear();
		if (hasInfo) {
			// TODO: tooltips.

			const UInt8 tile = (UInt8)(tilePos.x + tilePos.y * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
			const TileDetail::Array &details = _tileDetails[infoBank][(int)TileDetail::Usages::BG_MAP][tile].details;
			for (const TileDetail &detail : details) {
				highlights.push_back(detail.position);
			}
		}
	}
	void bgMap(Renderer* rnd, Theme* theme, Device* device, bool showGrids) {
		UInt8 bgX, bgY;
		device->getBgScroll(&bgX, &bgY);
		UInt8 wndX, wndY;
		device->getWindowScroll(&wndX, &wndY);

		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_BgWinMap());

		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();
		const float posX = ImGui::GetCursorPosX();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme->windowEmulator_VramDebugger_Layer());
		ImGui::SameLine();
		const float diff = ImGui::GetCursorPosX() - posX;
		const float remain = regSize.x * 0.3f - diff;
		ImGui::Dummy(ImVec2(remain, 0));
		ImGui::SameLine();
		do {
			const char* items[] = {
				"BG", "WIN"
			};

			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			int val = _options.isBgLayerActive ? 0 : 1;
			ImGui::SetNextItemWidth(regSize.x * 0.7f);
			if (ImGui::Combo("", &val, items, GBBASIC_COUNTOF(items))) {
				_options.isBgLayerActive = val == 0;
			}
		} while (false);

		// TODO: options.

		auto drawGrids = [] (const ImVec2 &curPos, const ImVec2 &dstSize) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const float w = dstSize.x;
			const float h = dstSize.y;
			for (float i = GBBASIC_TILE_SIZE; i < w; i += GBBASIC_TILE_SIZE) {
				drawList->AddLine(
					curPos + ImVec2(i, 0),
					curPos + ImVec2(i, h),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
				);
			}
			for (float j = GBBASIC_TILE_SIZE; j < h; j += GBBASIC_TILE_SIZE) {
				drawList->AddLine(
					curPos + ImVec2(0, j),
					curPos + ImVec2(w, j),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
				);
			}
			drawList->AddRect(
				curPos + ImVec2(-1, -1),
				curPos + ImVec2(w, h) + ImVec2(1, 1),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
			);
		};
		auto drawHighlights = [] (const ImVec2 &curPos, const MapBuffer::Points &pos) -> void {
			if (pos.empty())
				return;

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			for (const Math::Vec2i &pos_ : pos) {
				const ImVec2 startPos((float)(pos_.x * GBBASIC_TILE_SIZE), (float)(pos_.y * GBBASIC_TILE_SIZE));
				const ImVec2 endPos = startPos + ImVec2(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
				drawList->AddRect(
					curPos + startPos,
					curPos + endPos + ImVec2(1, 1),
					ImGui::GetColorU32(ImVec4(1, 0, 0, 0.75f))
				);
			}
		};
		auto drawCamera = [] (const ImVec2 &curPos, const ImVec2 &dstSize, UInt8 camX, UInt8 camY) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const bool tick = !!(((int)(DateTime::toSeconds(DateTime::ticks()) * 1)) % 2);
			drawList->AddRect(
				curPos + ImVec2(-1, -1) + ImVec2(camX, camY),
				curPos + ImVec2(1, 1) + ImVec2((float)(camX + GBBASIC_SCREEN_WIDTH), (float)(camY + GBBASIC_SCREEN_HEIGHT)),
				tick ? IM_COL32_WHITE : IM_COL32_BLACK
			);
			if (camX > dstSize.x - GBBASIC_SCREEN_WIDTH) {
				const int camX_ = (int)(camX - dstSize.x);
				drawList->AddRect(
					curPos + ImVec2(-1, -1) + ImVec2((float)camX_, camY),
					curPos + ImVec2(1, 1) + ImVec2((float)(camX_ + GBBASIC_SCREEN_WIDTH), (float)(camY + GBBASIC_SCREEN_HEIGHT)),
					tick ? IM_COL32_WHITE : IM_COL32_BLACK
				);
			}
			if (camY > dstSize.y - GBBASIC_SCREEN_HEIGHT) {
				const int camY_ = (int)(camY - dstSize.y);
				drawList->AddRect(
					curPos + ImVec2(-1, -1) + ImVec2(camX, (float)camY_),
					curPos + ImVec2(1, 1) + ImVec2((float)(camX + GBBASIC_SCREEN_WIDTH), (float)(camY_ + GBBASIC_SCREEN_HEIGHT)),
					tick ? IM_COL32_WHITE : IM_COL32_BLACK
				);
				if (camX > dstSize.x - GBBASIC_SCREEN_WIDTH) {
					const int camX_ = (int)(camX - dstSize.x);
					drawList->AddRect(
						curPos + ImVec2(-1, -1) + ImVec2((float)camX_, (float)camY_),
						curPos + ImVec2(1, 1) + ImVec2((float)(camX_ + GBBASIC_SCREEN_WIDTH), (float)(camY_ + GBBASIC_SCREEN_HEIGHT)),
						tick ? IM_COL32_WHITE : IM_COL32_BLACK
					);
				}
			}
		};

		const MapBuffer* mapBuf = nullptr;
		UInt8 camX;
		UInt8 camY;
		if (_options.isBgLayerActive) {
			mapBuf = &_bgMap;
			camX = bgX;
			camY = bgY;
		} else {
			mapBuf = &_winMap;
			camX = wndX;
			camY = -wndY;
		}
		bool hasInfo = false;
		Math::Vec2i mapPos;
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
				const ImVec2 curPos = ImGui::GetCursorScreenPos();
				ImGui::Image(
					mapBuf->buffer.texture->pointer(rnd),
					dstSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
				);

				if (showGrids)
					drawGrids(curPos, dstSize);

				const MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
				drawHighlights(curPos, highlights);

				drawCamera(curPos, dstSize, camX, camY);

				if (ImGui::IsItemHovered()) {
					const ImVec2 mousePos = ImGui::GetMousePos();
					const ImVec2 diff = mousePos - curPos;
					const ImVec2 mapPos_ = diff / GBBASIC_TILE_SIZE;
					if (mapPos_.x >= 0 && mapPos_.x < DEVICE_MAP_BUFFER_WIDTH && mapPos_.y >= 0 && mapPos_.y < DEVICE_MAP_BUFFER_HEIGHT) {
						hasInfo = true;
						mapPos = Math::Vec2i((int)mapPos_.x, (int)mapPos_.y);
					}
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
			}
			ImGui::EndChildFrame();
		} else {
			const ImVec2 curPos = ImGui::GetCursorScreenPos();
			ImGui::Image(
				mapBuf->buffer.texture->pointer(rnd),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);

			if (showGrids)
				drawGrids(curPos, dstSize);

			ImGui::PushClipRect(curPos, curPos + dstSize, true);
			{
				const MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
				drawHighlights(curPos, highlights);

				drawCamera(curPos, dstSize, camX, camY);
			}
			ImGui::PopClipRect();

			if (ImGui::IsItemHovered()) {
				const ImVec2 mousePos = ImGui::GetMousePos();
				const ImVec2 diff = mousePos - curPos;
				const ImVec2 mapPos_ = diff / GBBASIC_TILE_SIZE;
				if (mapPos_.x >= 0 && mapPos_.x < DEVICE_MAP_BUFFER_WIDTH && mapPos_.y >= 0 && mapPos_.y < DEVICE_MAP_BUFFER_HEIGHT) {
					hasInfo = true;
					mapPos = Math::Vec2i((int)mapPos_.x, (int)mapPos_.y);
				}
			}
		}

		_tiles.highlight = Math::Vec2i(-1, -1);
		if (hasInfo) {
			// TODO: tooltips.

			const Device::MapBuffer &mapBuf = _options.isBgLayerActive ? _bgMapBuf : _winMapBuf;
			const Device::MapBuffer &mapAttrBuf = _options.isBgLayerActive ? _bgMapAttrBuf : _winMapAttrBuf;
			const int val = mapPos.x + mapPos.y * DEVICE_MAP_BUFFER_WIDTH;
			const UInt8 tile = mapBuf[val];
			const UInt8 attrs = mapAttrBuf[val];
			const int bank = !!((attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001) ? 1 : 0;
			const std::div_t tdiv = std::div(tile, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
			const int tx = tdiv.rem;
			const int ty = tdiv.quot % VRAM_DEBUGGER_TILES_AREA_HEIGHT;
			const int ty_ = (
				((ty < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT) ?
					(ty + VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2) :
					ty)
			);
			_tiles.highlight = Math::Vec2i(tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank, ty_);
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
