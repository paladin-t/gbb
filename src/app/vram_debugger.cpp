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
#ifndef VRAM_DEBUGGER_TILES_SECTION_HALF_SIZE
#	define VRAM_DEBUGGER_TILES_SECTION_HALF_SIZE (VRAM_DEBUGGER_TILES_SECTION_WIDTH * VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT)
#endif /* VRAM_DEBUGGER_TILES_SECTION_HALF_SIZE */
#ifndef VRAM_DEBUGGER_TILES_SECTION_SIZE
#	define VRAM_DEBUGGER_TILES_SECTION_SIZE (VRAM_DEBUGGER_TILES_SECTION_HALF_SIZE * 2)
#endif /* VRAM_DEBUGGER_TILES_SECTION_SIZE */

#ifndef VRAM_DEBUGGER_OAM_PALETTE_BITS
#	define VRAM_DEBUGGER_OAM_PALETTE_BITS 0x03
#endif /* VRAM_DEBUGGER_OAM_PALETTE_BITS */
#ifndef VRAM_DEBUGGER_OAM_BANK_BIT
#	define VRAM_DEBUGGER_OAM_BANK_BIT 3
#endif /* VRAM_DEBUGGER_OAM_BANK_BIT */
#ifndef VRAM_DEBUGGER_OAM_PALETTE_BIT
#	define VRAM_DEBUGGER_OAM_PALETTE_BIT 4
#endif /* VRAM_DEBUGGER_OAM_PALETTE_BIT */
#ifndef VRAM_DEBUGGER_OAM_HFLIP_BIT
#	define VRAM_DEBUGGER_OAM_HFLIP_BIT 5
#endif /* VRAM_DEBUGGER_OAM_HFLIP_BIT */
#ifndef VRAM_DEBUGGER_OAM_VFLIP_BIT
#	define VRAM_DEBUGGER_OAM_VFLIP_BIT 6
#endif /* VRAM_DEBUGGER_OAM_VFLIP_BIT */
#ifndef VRAM_DEBUGGER_OAM_PRIORITY_BIT
#	define VRAM_DEBUGGER_OAM_PRIORITY_BIT 7
#endif /* VRAM_DEBUGGER_OAM_PRIORITY_BIT */

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
		// Resets the texture and image.
		void reset(void) {
			if (texture)
				texture = nullptr;
			if (image)
				image = nullptr;
			width = 0;
			height = 0;
			pixels = nullptr;
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
			MAP = 0,
			OBJ
		};

		Usages usage = Usages::MAP;
		Math::Vec2i position;
		int palette = 0; // CGB palette.
		int obp = 0; // Classic palette for sprite.

		TileDetail() {
		}
		TileDetail(Usages use, const Math::Vec2i &pos, int pal) :
			usage(use),
			position(pos),
			palette(pal)
		{
		}
		TileDetail(Usages use, const Math::Vec2i &pos, int pal, int obp_) :
			usage(use),
			position(pos),
			palette(pal),
			obp(obp_)
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
		void reset(void) {
			buffer.reset();
		}
	};

	struct MapBuffer {
		typedef std::vector<Math::Vec2i> Points;

		BufferTexture buffer;
		Points highlights;

		void touch(Renderer* rnd) {
			buffer.touch(rnd, DEVICE_MAP_BUFFER_WIDTH * GBBASIC_TILE_SIZE, DEVICE_MAP_BUFFER_HEIGHT * GBBASIC_TILE_SIZE);
		}
		void reset(void) {
			buffer.reset();
		}
	};

	struct ObjBuffer {
		typedef std::array<ObjBuffer, DEVICE_OBJ_COUNT> Array;

		BufferTexture buffer;
		Device::Obj obj;
		bool visible = false;

		void touch(Renderer* rnd) {
			buffer.touch(rnd, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE * 2);
		}
		void reset(void) {
			buffer.reset();
		}
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
		float paletteTextWidthPerLine = 0;
	} _options;

	Device::MapBuffer _bgMapBuf;
	Device::MapBuffer _bgMapAttrBuf;
	Device::MapBuffer _winMapBuf;
	Device::MapBuffer _winMapAttrBuf;

	TileDetail::Banks _tileDetails;
	TilesBuffer _tiles;
	MapBuffer _bgMap;
	MapBuffer _winMap;
	bool _is8x16Obj = false;
	ObjBuffer::Array _objs;
	Palette::Collection _palettes;
	CgbPalette::Collection _cgbPalettes;

	struct {
		int tile = -1;
		UInt8 bank = 0;
		UInt16 address = 0;
		int palette = -1;
		std::string text;

		void refresh(
			Theme* theme,
			UInt8 tile_,
			UInt8 bank_, UInt16 addr,
			int plt
		) {
			if (
				tile == tile_ &&
				bank == bank_ && address == addr &&
				palette == plt
			) {
				return;
			}

			tile = tile_;
			bank = bank_;
			address = addr;
			palette = plt;

			text = Text::format(
				theme->tooltipEmulator_VramDebugger_Tile(),
				{
					Text::toHex(tile, 2, '0', true), Text::toString(tile),
					Text::toString(bank), Text::toHex(address, 4, '0', true),
					palette == -1 ? "-" : Text::toString(palette)
				}
			);
		}
	} _tileTips;
	struct {
		Math::Vec2i position;
		int tile = -1;
		UInt8 attribute = 0;
		UInt16 mapAddress = 0;
		UInt8 tileBank = 0;
		UInt16 tileAddress = 0;
		bool hFlip = false;
		bool vFlip = false;
		int palette = -1;
		int priority = 0;
		std::string text;

		void refresh(
			Theme* theme,
			const Math::Vec2i &pos,
			UInt8 tile_,
			UInt8 attr,
			UInt16 mapAddr_,
			UInt8 tileBank_, UInt16 tileAddr,
			bool hFlip_, bool vFlip_,
			int plt,
			int pri
		) {
			if (
				position == pos &&
				tile == tile_ &&
				attribute == attr &&
				mapAddress == mapAddr_ &&
				tileBank == tileBank_ && tileAddress == tileAddr &&
				hFlip == hFlip_ && vFlip == vFlip_ &&
				palette == plt &&
				priority == pri
			) {
				return;
			}

			position = pos;
			tile = tile_;
			attribute = attr;
			mapAddress = mapAddr_;
			tileBank = tileBank_;
			tileAddress = tileAddr;
			hFlip = hFlip_;
			vFlip = vFlip_;
			palette = plt;
			priority = pri;

			text = Text::format(
				theme->tooltipEmulator_VramDebugger_Map(),
				{
					Text::toHex(position.x, 2, '0', true), Text::toHex(position.y, 2, '0', true),
					Text::toHex(tile, 2, '0', true), Text::toString(tile),
					Text::toHex(attribute, 2, '0', true),
					Text::toHex(mapAddress, 4, '0', true),
					Text::toString(tileBank), Text::toHex(tileAddress, 4, '0', true),
					hFlip ? "YES" : "NO", vFlip ? "YES" : "NO",
					palette == -1 ? "-" : Text::toString(palette),
					Text::toString(priority)
				}
			);
		}
	} _mapTips;
	struct {
		Math::Vec2i position;
		int tile = -1;
		UInt8 attribute = 0;
		UInt16 oamAddress = 0;
		UInt8 tileBank = 0;
		UInt16 tileAddress = 0;
		bool hFlip = false;
		bool vFlip = false;
		int palette = -1;
		int priority = 0;
		std::string text;

		void refresh(
			Theme* theme,
			const Math::Vec2i &pos,
			UInt8 tile_,
			UInt8 attr,
			UInt16 oamAddr_,
			UInt8 tileBank_, UInt16 tileAddr,
			bool hFlip_, bool vFlip_,
			int plt,
			int pri
		) {
			if (
				position == pos &&
				tile == tile_ &&
				attribute == attr &&
				oamAddress == oamAddr_ &&
				tileBank == tileBank_ && tileAddress == tileAddr &&
				hFlip == hFlip_ && vFlip == vFlip_ &&
				palette == plt &&
				priority == pri
			) {
				return;
			}

			position = pos;
			tile = tile_;
			attribute = attr;
			oamAddress = oamAddr_;
			tileBank = tileBank_;
			tileAddress = tileAddr;
			hFlip = hFlip_;
			vFlip = vFlip_;
			palette = plt;
			priority = pri;

			text = Text::format(
				theme->tooltipEmulator_VramDebugger_Oam(),
				{
					Text::toHex(position.x, 2, '0', true), Text::toHex(position.y, 2, '0', true),
					Text::toHex(tile, 2, '0', true), Text::toString(tile),
					Text::toHex(attribute, 2, '0', true),
					Text::toHex(oamAddress, 4, '0', true),
					Text::toString(tileBank), Text::toHex(tileAddress, 4, '0', true),
					hFlip ? "YES" : "NO", vFlip ? "YES" : "NO",
					palette == -1 ? "-" : Text::toString(palette),
					Text::toString(priority)
				}
			);
		}
	} _oamTips;

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
		for (ObjBuffer &obj : _objs)
			obj.touch(rnd);

		_opened = true;

		return true;
	}
	virtual bool close(void) override {
		if (!_opened)
			return true;

		_tiles.reset();
		_bgMap.reset();
		_winMap.reset();
		for (ObjBuffer &obj : _objs)
			obj.reset();

		return true;
	}

	virtual void update(
		class Renderer* rnd, class Theme* theme, class Device* device,
		bool previewPaletteBits, bool showGrids
	) override {
		refresh(rnd, theme, device, previewPaletteBits);

		tiles(rnd, theme, device, showGrids);
		ImGui::NewLine(1);
		ImGui::Separator();

		map(rnd, theme, device, showGrids);
		ImGui::NewLine(1);
		ImGui::Separator();

		oam(rnd, theme, device, showGrids);
		ImGui::NewLine(1);
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
		device->getTileBuffer(tilesBuf); // Retrieve tile data.

		device->getMapBuffer(bgMapSrc, _bgMapBuf); // Retrieve BG map data.
		if (isCgb)
			device->getMapAttrBuffer(bgMapSrc, _bgMapAttrBuf); // Retrieve BG map attributes.
		else
			_bgMapAttrBuf.fill(0);

		device->getMapBuffer(winMapSrc, _winMapBuf); // Retrieve WIN map data.
		if (isCgb)
			device->getMapAttrBuffer(winMapSrc, _winMapAttrBuf); // Retrieve WIN map attributes.
		else
			_winMapAttrBuf.fill(0);

		_is8x16Obj = device->is8x16Obj();
		for (int i = 0; i < DEVICE_OBJ_COUNT; ++i) {
			_objs[i].obj = device->getObj(i); // Retrieve OBJ data.
			_objs[i].visible = device->isObjVisible(&_objs[i].obj);
		}

		for (int i = 0; i < (int)Device::PaletteTypes::COUNT; ++i) {
			const Device::PaletteRgba &pltRgba = device->getPaletteRgba((Device::PaletteTypes)i); // Retrieve classic palettes.
			for (int k = 0; k < 4; ++k) {
				const UInt32 rgba = pltRgba.color[k];
				const Colour col = Colour::byRGBA8888(rgba);
				_palettes[i].color[k] = col;
			}
		}
		if (isCgb) {
			for (int i = 0; i < (int)Device::CgbPaletteTypes::COUNT; ++i) {
				for (int j = 0; j < 8; ++j) {
					const Device::PaletteRgba &pltRgba = device->getCgbPaletteRgba((Device::CgbPaletteTypes)i, j); // Retrieve CGB palettes.
					for (int k = 0; k < 4; ++k) {
						const UInt32 rgba = pltRgba.color[k];
						const Colour col = Colour::byRGBA8888(rgba);
						_cgbPalettes[i][j].color[k] = col;
					}
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

				TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::MAP][tile];
				++ref.refCount;
				if (!refOnly) {
					const TileDetail detail(TileDetail::Usages::MAP, Math::Vec2i(mx, my), plt);
					ref.details.push_back(detail);
				}
			}
		};
		auto collectObjInfo = [] (
			TileDetail::Banks &tileDetails,
			bool is8x16Obj, const ObjBuffer::Array &objs
		) -> void {
			for (const ObjBuffer &obj : objs) {
				const Device::Obj &dobj = obj.obj;

				const int ox = dobj.x;
				const int oy = dobj.y;

				const UInt8 tile = dobj.tile;
				const int obp = dobj.palette;
				const int bank = dobj.bank;
				const int plt = dobj.cgbPalette;

				TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::OBJ][tile];
				++ref.refCount;
				const TileDetail detail(TileDetail::Usages::OBJ, Math::Vec2i(ox, oy), plt, obp);
				ref.details.push_back(detail);
				if (is8x16Obj) {
					TileDetail::Ref &ref_ = tileDetails[bank][(int)TileDetail::Usages::OBJ][(tile + 1) % 255];
					++ref_.refCount;
					const TileDetail detail_(TileDetail::Usages::OBJ, Math::Vec2i(ox, oy + GBBASIC_TILE_SIZE), plt, obp);
					ref_.details.push_back(detail_);
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
		collectObjInfo(
			_tileDetails,
			_is8x16Obj, _objs
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
						const TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::MAP][tile];
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
							if (isCgb) {
								if (details->empty()) {
									const int cplt = isForBg ? (int)TileDetail::Usages::MAP : (int)TileDetail::Usages::OBJ;
									col = cgbPalettes[cplt][0].color[val];
								} else {
									const TileDetail &detail = details->front(); // Use the first for paletting.
									col = cgbPalettes[(int)detail.usage][detail.palette].color[val];
								}
							} else {
								if (details->empty()) {
									const int cplt = isForBg ? (int)Device::PaletteTypes::BGP : (int)Device::PaletteTypes::OBP0;
									col = palettes[cplt].color[val];
								} else {
									const TileDetail &detail = details->front(); // Use the first for paletting.
									col = palettes[(int)detail.usage + detail.obp].color[val];
								}
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

		auto translateMap = [device, previewPaletteBits, isCgb] (
			MapBuffer &map,
			const Device::MapBuffer &mapBuf, const Device::MapBuffer &mapAttrBuf,
			const Device::TileBuffer &tilesBuf, const TilesBuffer &tiles, const TileDetail::Banks &tileDetails,
			const Palette::Collection &palettes, const CgbPalette::Collection &cgbPalettes
		) -> void {
			auto getTileBuf = [] (const Device::TileBuffer &tilesBuf, int x, int y) -> UInt8 {
				const int p = x + y * DEVICE_TILE_BUFFER_WIDTH;

				return tilesBuf[p];
			};

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
					const TileDetail::Array &details = tileDetails[bank][(int)TileDetail::Usages::MAP][tile].details;
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

		auto translateOam = [] (
			ObjBuffer::Array &objs,
			bool is8x16Obj,
			const TilesBuffer &tiles
		) -> void {
			for (ObjBuffer &obj : objs) {
				if (obj.buffer.begin()) {
					const Device::Obj &dobj = obj.obj;

					const int x = 0;
					const int y = 0;

					const UInt8 tile = dobj.tile;
					const bool hFlip = dobj.xFlip;
					const bool vFlip = dobj.yFlip;
					const int bank = dobj.bank;

					const std::div_t sdiv = std::div(tile, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
					const int sx = (sdiv.rem + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE;
					const int sy = sdiv.quot * GBBASIC_TILE_SIZE;

					tiles.buffer.blit( // Blit to the OMA image from the tiles image.
						obj.buffer,
						x, y, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
						sx, sy,
						hFlip, vFlip
					);
					if (is8x16Obj) {
						const UInt8 tile_ = tile + 1;

						const std::div_t sdiv_ = std::div(tile_, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
						const int sx_ = (sdiv_.rem + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank) * GBBASIC_TILE_SIZE;
						const int sy_ = sdiv_.quot * GBBASIC_TILE_SIZE;

						tiles.buffer.blit( // Blit to the OMA image from the tiles image.
							obj.buffer,
							x, y + GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
							sx_, sy_,
							hFlip, vFlip
						);
					}
					obj.buffer.commit( // Commits to the OMA map texture from the OMA map image.
						GBBASIC_TILE_SIZE * (GBBASIC_TILE_SIZE * 2) * sizeof(Colour)
					);

					obj.buffer.end();
				}
			}
		};

		translateOam(
			_objs,
			_is8x16Obj,
			_tiles
		);
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

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(theme->windowEmulator_VramDebugger_Tiles().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

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
			const UInt8 tile = (UInt8)(tilePos.x + tilePos.y * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
			const TileDetail::Array &details = _tileDetails[infoBank][(int)TileDetail::Usages::MAP][tile].details;

			const UInt16 addr = (UInt16)(
				(0x8000 + (tilePos.x + tilePos.y * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK) * 16)
			);
			const int plt = details.empty() ? -1 : details.front().palette;
			_tileTips.refresh(
				theme,
				tile,
				(UInt8)infoBank, addr,
				plt
			);
			if (!_tileTips.text.empty()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(_tileTips.text);
			}

			for (const TileDetail &detail : details) {
				highlights.push_back(detail.position);
			}
		}
	}
	void map(Renderer* rnd, Theme* theme, Device* device, bool showGrids) {
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

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(theme->windowEmulator_VramDebugger_BgWinMap().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

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
			constexpr const char* ITEMS[] = {
				"BG", "WIN"
			};

			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			int val = _options.isBgLayerActive ? 0 : 1;
			ImGui::SetNextItemWidth(regSize.x * 0.7f);
			if (ImGui::Combo("", &val, ITEMS, GBBASIC_COUNTOF(ITEMS))) {
				_options.isBgLayerActive = val == 0;
			}
		} while (false);
		ImGui::SameLine();
		ImGui::NewLine();
		ImGui::NewLine(2);

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

				drawCamera(curPos, dstSize, camX, camY);

				const MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
				drawHighlights(curPos, highlights);

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
				drawCamera(curPos, dstSize, camX, camY);

				const MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
				drawHighlights(curPos, highlights);
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
			const Device::MapBuffer &mapBuf = _options.isBgLayerActive ? _bgMapBuf : _winMapBuf;
			const Device::MapBuffer &mapAttrBuf = _options.isBgLayerActive ? _bgMapAttrBuf : _winMapAttrBuf;
			const int val = mapPos.x + mapPos.y * DEVICE_MAP_BUFFER_WIDTH;
			const UInt8 tile = mapBuf[val];
			const UInt8 attrs = mapAttrBuf[val];
			const int plt = attrs & ((0x00000001 << GBBASIC_MAP_PALETTE_BIT0) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT1) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT2));
			const int bank = !!((attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001) ? 1 : 0;
			const bool hFlip = !!((attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001);
			const bool vFlip = !!((attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001);
			const int pri = (attrs >> GBBASIC_MAP_PRIORITY_BIT) & 0x00000001;

			const Device::MapSourceTypes mapSrc = _options.isBgLayerActive ? device->getMapSourceType(Device::LayerTypes::BG) : device->getMapSourceType(Device::LayerTypes::WINDOW);
			const UInt16 base = mapSrc == Device::MapSourceTypes::FROM_9800_TO_9BFF ? 0x9800 : 0x9c00;
			const UInt16 addr = (UInt16)(
				(0x8000 + (
					tile < VRAM_DEBUGGER_TILES_SECTION_HALF_SIZE ?
						tile + VRAM_DEBUGGER_TILES_SECTION_SIZE :
						tile
				) * 16)
			);
			_mapTips.refresh(
				theme,
				mapPos,
				tile,
				attrs,
				(UInt16)(base + (mapPos.x + mapPos.y * DEVICE_MAP_BUFFER_WIDTH)),
				(UInt8)bank, addr,
				hFlip, vFlip,
				plt,
				pri
			);
			if (!_mapTips.text.empty()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(_mapTips.text);
			}

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
	void oam(Renderer* rnd, Theme* theme, Device* /* device */, bool showGrids) {
		constexpr const int OBJECT_COUNT_PER_LINE = 8;

		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(theme->windowEmulator_VramDebugger_Oam().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		auto drawOams = [rnd, showGrids, OBJECT_COUNT_PER_LINE] (bool is8x16Obj, const ObjBuffer::Array &objs, const ImVec2 &tileSize) -> int {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			int hovering = -1;
			for (int i = 0; i < (int)objs.size(); ++i) {
				const ObjBuffer &obj = objs[i];
				const bool visible = obj.visible;

				if (is8x16Obj) {
					const ImVec2 curPos = ImGui::GetCursorScreenPos();
					ImGui::Image(
						obj.buffer.texture->pointer(rnd),
						ImVec2(tileSize.x, tileSize.y * 2),
						ImVec2(0, 0), ImVec2(1, 1),
						ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
					);

					if (!visible) {
						drawList->AddLine(
							curPos + ImVec2(0, -1),
							curPos + ImVec2(tileSize.x, tileSize.y * 2) + ImVec2(0, -1),
							ImGui::GetColorU32(ImVec4(1, 0, 0, 0.95f))
						);
					}
					if (showGrids) {
						drawList->AddRect(
							curPos,
							curPos + ImVec2(tileSize.x, tileSize.y * 2),
							ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
						);
					}

					if (hovering == -1 && ImGui::IsItemHovered()) {
						hovering = i;
					}
				} else {
					const ImVec2 curPos = ImGui::GetCursorScreenPos();
					ImGui::Image(
						obj.buffer.texture->pointer(rnd),
						tileSize,
						ImVec2(0, 0), ImVec2(1, 0.5f),
						ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
					);

					if (!visible) {
						drawList->AddLine(
							curPos + ImVec2(0, -1),
							curPos + tileSize + ImVec2(0, -1),
							ImGui::GetColorU32(ImVec4(1, 0, 0, 0.95f))
						);
					}
					if (showGrids) {
						drawList->AddRect(
							curPos,
							curPos + tileSize,
							ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
						);
					}

					if (hovering == -1 && ImGui::IsItemHovered()) {
						hovering = i;
					}
				}
				ImGui::SameLine();
				if ((i + 1) % OBJECT_COUNT_PER_LINE == 0) {
					ImGui::NewLine();
					ImGui::NewLine(1);
				}
			}

			return hovering;
		};

		bool hasInfo = false;
		int oamIndex = -1;
		const ImVec2 tileSize(32, 32);
		const ImVec2 dstSize(tileSize.x * OBJECT_COUNT_PER_LINE, _is8x16Obj ? tileSize.y * 10 + 4 : tileSize.y * 5 + 4);
		if (regSize.x < VRAM_DEBUGGER_MAX_WIDTH) {
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

			const ImVec2 frameSize = ImVec2(regSize.x, dstSize.y + style.ScrollbarSize) + ImVec2(style.WindowBorderSize * 2, style.WindowBorderSize * 2 + 1);
			ImGui::BeginChildFrame(
				ImGui::GetID("@Oam"),
				frameSize,
				ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysHorizontalScrollbar
			);
			{
				const int hovering = drawOams(_is8x16Obj, _objs, tileSize);
				if (hovering != -1) {
					hasInfo = true;
					oamIndex = hovering;
				}

				ImGui::SameLine();
				ImGui::Dummy(ImVec2(1, 0));
			}
			ImGui::EndChildFrame();
		} else {
			const int hovering = drawOams(_is8x16Obj, _objs, tileSize);
			if (hovering != -1) {
				hasInfo = true;
				oamIndex = hovering;
			}
		}

		if (hasInfo) {
			const ObjBuffer &obj = _objs[oamIndex];
			const Device::Obj &dobj = obj.obj;
			const UInt8 x = dobj.x;
			const UInt8 y = dobj.y;
			const UInt8 tile = dobj.tile;
			const bool pri = dobj.priority == Device::ObjPriorities::BEHIND_BG;
			const bool hFlip = dobj.xFlip;
			const bool vFlip = dobj.yFlip;
			const UInt8 bank = dobj.bank;
			const int plt = dobj.cgbPalette;
			const int gray = dobj.palette;

			const UInt16 oamAddr = (UInt16)(0xfe00 + 4 * oamIndex);
			const UInt16 addr = (UInt16)(0x8000 + tile * 16);
			const UInt8 attrs = (UInt8)(
				(plt              & VRAM_DEBUGGER_OAM_PALETTE_BITS) |
				(bank            << VRAM_DEBUGGER_OAM_BANK_BIT)     |
				(gray            << VRAM_DEBUGGER_OAM_PALETTE_BIT)  |
				((hFlip ? 1 : 0) << VRAM_DEBUGGER_OAM_HFLIP_BIT)    |
				((vFlip ? 1 : 0) << VRAM_DEBUGGER_OAM_VFLIP_BIT)    |
				((pri ? 1 : 0)   << VRAM_DEBUGGER_OAM_PRIORITY_BIT)
			);

			_oamTips.refresh(
				theme,
				Math::Vec2i(x, y),
				tile,
				attrs,
				oamAddr,
				bank, addr,
				hFlip, vFlip,
				plt,
				pri
			);
			if (!_oamTips.text.empty()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(_oamTips.text);
			}
		}
	}
	void palettes(Renderer* /* rnd */, Theme* theme, Device* device) {
		const bool isCgb = device->deviceHasCgbSupport();

		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(theme->windowEmulator_VramDebugger_Palettes().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		if (isCgb) {
			constexpr const char* const GRP[] = {
				"BG0",  "BG1",  "BG2",  "BG3",  "BG4",  "BG5",  "BG6",  "BG7",
				"OBJ0", "OBJ1", "OBJ2", "OBJ3", "OBJ4", "OBJ5", "OBJ6", "OBJ7"
			};

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			const bool recalcTxtWidth = _options.paletteTextWidthPerLine <= 0;
			const float colWidth = std::ceil((regSize.x - _options.paletteTextWidthPerLine) / 8);

			float posX = -1;
			const float posY = ImGui::GetCursorPosY();
			int k = 0;
			for (int i = 0; i < (int)_cgbPalettes.size(); ++i) {
				const CgbPalette::Group &group = _cgbPalettes[i];

				if (i == 1)
					ImGui::SetCursorPosY(posY);

				for (int j = 0; j < (int)group.size(); ++j, ++k) {
					const CgbPalette &palette = group[j];

					if (i == 1)
						ImGui::SetCursorPosX(posX);

					ImGui::AlignTextToFramePadding();
					if (recalcTxtWidth && j == 0) {
						const float x = ImGui::GetCursorPosX();
						if (i == 1) {
							ImGui::Dummy(ImVec2(1, 0));
							ImGui::SameLine();
						}
						ImGui::TextUnformatted(GRP[k]);
						ImGui::SameLine();
						_options.paletteTextWidthPerLine += ImGui::GetCursorPosX() - x;
					} else {
						if (i == 1) {
							ImGui::Dummy(ImVec2(1, 0));
							ImGui::SameLine();
						}
						ImGui::TextUnformatted(GRP[k]);
						ImGui::SameLine();
					}

					for (int l = 0; l < GBBASIC_COUNTOF(palette.color); ++l) {
						const Colour &col = palette.color[l];
						const ImVec4 col4v(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);

						ImGui::PushID(l);
						const float size = 19.0f;
						if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview, ImVec2(colWidth, size))) {
							// Do nothing.
						}
						if (ImGui::IsItemHovered()) {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							ImGui::ColorTooltip("", col, (ImGuiColorEditFlags_NoAlpha));
						}
						ImGui::SameLine();
						ImGui::PopID();
					}

					if (posX <= 0)
						posX = ImGui::GetCursorPosX();

					ImGui::NewLine();
					ImGui::NewLine(1);
				}
			}
		} else {
			constexpr const char* const GRP[] = {
				"BGP ",
				"OBP0", "OBP1"
			};

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			const bool recalcTxtWidth = _options.paletteTextWidthPerLine <= 0;
			const float colWidth = std::ceil((regSize.x - _options.paletteTextWidthPerLine) / 8);

			float posX = -1;
			float posY = -1;
			int k = 0;
			for (int i = 0; i < (int)_palettes.size(); ++i, ++k) {
				const Palette &palette = _palettes[i];

				if (i == 1) {
					posY = ImGui::GetCursorPosY();
				} else if (i == 2) {
					ImGui::SetCursorPosY(posY);
					ImGui::SetCursorPosX(posX);
				}

				ImGui::AlignTextToFramePadding();
				if (recalcTxtWidth && i > 0) {
					const float x = ImGui::GetCursorPosX();
					if (i == 2) {
						ImGui::Dummy(ImVec2(1, 0));
						ImGui::SameLine();
					}
					ImGui::TextUnformatted(GRP[k]);
					ImGui::SameLine();
					_options.paletteTextWidthPerLine += ImGui::GetCursorPosX() - x;
				} else {
					if (i == 2) {
						ImGui::Dummy(ImVec2(1, 0));
						ImGui::SameLine();
					}
					ImGui::TextUnformatted(GRP[k]);
					ImGui::SameLine();
				}

				for (int l = 0; l < GBBASIC_COUNTOF(palette.color); ++l) {
					const Colour &col = palette.color[l];
					const ImVec4 col4v(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);

					ImGui::PushID(l);
					const float size = 19.0f;
					if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview, ImVec2(colWidth, size))) {
						// Do nothing.
					}
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::ColorTooltip("", col, (ImGuiColorEditFlags_NoAlpha));
					}
					ImGui::SameLine();
					ImGui::PopID();
				}

				if (posX <= 0)
					posX = ImGui::GetCursorPosX();

				ImGui::NewLine();
				ImGui::NewLine(1);
			}
		}
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
