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
#include "../../lib/binjgb/src/emulator.h"
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
		struct {
			int index = 0; // OAM index.
			int palette = 0; // Classic palette for sprite.
			bool visible = false;
		} oam;

		TileDetail() {
		}
		TileDetail(Usages use, const Math::Vec2i &pos, int pal) :
			usage(use),
			position(pos),
			palette(pal)
		{
		}
		TileDetail(Usages use, const Math::Vec2i &pos, int pal, int idx, int obp_, bool vis) :
			usage(use),
			position(pos),
			palette(pal)
		{
			oam.index = idx;
			oam.palette = obp_;
			oam.visible = vis;
		}

		static void clear(TileDetail::Banks &details) {
			for (int b = 0; b < (int)details.size(); ++b) {
				Bank &bank = details[b];
				for (int s = 0; s < (int)bank.size(); ++s) {
					Section &section = bank[s];
					for (int i = 0; i < (int)section.size(); ++i) {
						Ref &ref = section[i];
						ref.details.clear();
						ref.refCount = 0;
					}
				}
			}
		}
	};

	struct TilesBuffer {
		typedef std::array<Math::Vec2i, 2> Points;

		BufferTexture buffer;
		Points highlights;

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
		bool highlight = false;

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

	struct DataArea {
		UInt16 address = 0;
		std::string tips;

		DataArea() {
		}

		const std::string &refresh(const std::string &fmt, UInt16 addr) {
			if (address != addr) {
				address = addr;
				tips = Text::format(fmt, { Text::toHex(address, 4, '0', true) });
			}

			return tips;
		}
	};

private:
	bool _opened = false;
	struct {
		bool isBgLayerActive = true;
		float paletteTextWidthPerLine = 0;
		float startY = 0;
		int safeHeight = 0;
	} _options;
	Window* _window = nullptr; // Foreign.
	Renderer* _renderer = nullptr; // Foreign.
	Workspace* _workspace = nullptr; // Foreign.
	Theme* _theme = nullptr; // Foreign.
	Device* _device = nullptr; // Foreign.

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
	DataArea _tileDataArea;
	DataArea _bgDataArea;
	DataArea _winDataArea;

	struct {
		int tile = -1;
		UInt8 bank = 0;
		UInt16 address = 0;
		int palette = -1;
		int refCount = 0;
		std::string text;

		void refresh(
			Theme* theme,
			UInt8 tile_,
			UInt8 bank_, UInt16 addr,
			int plt,
			int ref
		) {
			if (
				tile == tile_ &&
				bank == bank_ && address == addr &&
				palette == plt &&
				refCount == ref
			) {
				return;
			}

			tile = tile_;
			bank = bank_;
			address = addr;
			palette = plt;
			refCount = ref;

			text = Text::format(
				theme->tooltipEmulator_VramDebugger_Tile(),
				{
					Text::toHex(tile, 2, '0', true), Text::toString(tile),
					Text::toString(bank), Text::toHex(address, 4, '0', true),
					palette == -1 ? "-" : Text::toString(palette),
					Text::toString(refCount)
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

	struct {
		bool highlighted = false;
		Math::Recti area;
	} _inGameHighlight;

public:
	VramDebuggerImpl() {
	}
	virtual ~VramDebuggerImpl() {
		close();
	}

	virtual bool open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Theme* theme, class Device* device) override {
		if (_opened)
			return true;

		_window = wnd;
		_renderer = rnd;
		_workspace = ws;
		_theme = theme;
		_device = device;

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

		_window = nullptr;
		_renderer = nullptr;
		_workspace = nullptr;
		_theme = nullptr;
		_device = nullptr;

		_opened = false;

		return true;
	}

	virtual int safeHeight(void) const override {
		return _options.safeHeight;
	}

	virtual int highlightCount(void) const override {
		if (_inGameHighlight.highlighted)
			return 1;

		return 0;
	}
	virtual bool getHighlight(int index, Math::Recti* area) const override {
		if (area)
			*area = Math::Recti();

		if (!_inGameHighlight.highlighted)
			return false;
		if (index != 0)
			return false;

		if (area)
			*area = _inGameHighlight.area;

		return true;
	}

	virtual void update(
		bool previewPaletteBits, bool showGrids,
		bool isNewFrame,
		bool showTitle
	) override {
		refresh(previewPaletteBits, isNewFrame);

		begin(showTitle);
		{
			tiles(showGrids);
			ImGui::NewLine(1);
			ImGui::Separator();

			map(showGrids);
			ImGui::NewLine(1);
			ImGui::Separator();

			oam(showGrids);
			ImGui::NewLine(1);
			ImGui::Separator();

			palettes();
			ImGui::NewLine(1);
			ImGui::Separator();

			status();
		}
		end();
	}

private:
	void refresh(bool previewPaletteBits, bool /* isNewFrame */) {
		// Retrieve data.
		const bool isCgb = _device->isDeviceCgbCompatible();

		const Device::MapSourceTypes bgMapSrc = _device->getMapSourceType(Device::LayerTypes::BG);
		const Device::MapSourceTypes winMapSrc = _device->getMapSourceType(Device::LayerTypes::WINDOW);

		Device::TileBuffer tilesBuf;
		_device->getTileBuffer(tilesBuf); // Retrieve tile data.

		_device->getMapBuffer(bgMapSrc, _bgMapBuf); // Retrieve BG map data.
		if (isCgb)
			_device->getMapAttrBuffer(bgMapSrc, _bgMapAttrBuf); // Retrieve BG map attributes.
		else
			_bgMapAttrBuf.fill(0);

		_device->getMapBuffer(winMapSrc, _winMapBuf); // Retrieve WIN map data.
		if (isCgb)
			_device->getMapAttrBuffer(winMapSrc, _winMapAttrBuf); // Retrieve WIN map attributes.
		else
			_winMapAttrBuf.fill(0);

		_is8x16Obj = _device->is8x16Obj();
		for (int i = 0; i < DEVICE_OBJ_COUNT; ++i) {
			_objs[i].obj = _device->getObj(i); // Retrieve OBJ data.
			_objs[i].visible = _device->isObjVisible(&_objs[i].obj);
		}

		for (int i = 0; i < (int)Device::PaletteTypes::COUNT; ++i) {
			const Device::PaletteRgba &pltRgba = _device->getPaletteRgba((Device::PaletteTypes)i); // Retrieve classic palettes.
			for (int k = 0; k < 4; ++k) {
				const UInt32 rgba = pltRgba.color[k];
				const Colour col = Colour::byRGBA8888(rgba);
				_palettes[i].color[k] = col;
			}
		}
		if (isCgb) {
			for (int i = 0; i < (int)Device::CgbPaletteTypes::COUNT; ++i) {
				for (int j = 0; j < 8; ++j) {
					const Device::PaletteRgba &pltRgba = _device->getCgbPaletteRgba((Device::CgbPaletteTypes)i, j); // Retrieve CGB palettes.
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
			auto addDetail = [] (TileDetail::Ref &ref, const TileDetail &detail) -> void {
				if (ref.details.empty()) {
					ref.details.push_back(detail);

					return;
				}

				if (ref.details.front().oam.visible) {
					ref.details.push_back(detail);

					return;
				}

				ref.details.insert(ref.details.begin(), detail);
			};

			for (int i = 0; i < (int)objs.size(); ++i) {
				const ObjBuffer &obj = objs[i];
				const Device::Obj &dobj = obj.obj;

				const int ox = dobj.x;
				const int oy = dobj.y;

				const UInt8 tile = dobj.tile;
				const int obp = dobj.palette;
				const int bank = dobj.bank;
				const int plt = dobj.cgbPalette;

				TileDetail::Ref &ref = tileDetails[bank][(int)TileDetail::Usages::OBJ][tile];
				if (obj.visible)
					++ref.refCount;
				const TileDetail detail(TileDetail::Usages::OBJ, Math::Vec2i(ox, oy), plt, i, obp, obj.visible);
				addDetail(ref, detail);
				if (is8x16Obj) {
					TileDetail::Ref &ref_ = tileDetails[bank][(int)TileDetail::Usages::OBJ][(tile + 1) % 255];
					if (obj.visible)
						++ref_.refCount;
					const TileDetail detail_(TileDetail::Usages::OBJ, Math::Vec2i(ox, oy + GBBASIC_TILE_SIZE), plt, i, obp, obj.visible);
					addDetail(ref_, detail_);
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
		auto translateTiles = [this, previewPaletteBits, isCgb] (
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
					const UInt8 bgTileY = isForBg ?
						(UInt8)((ty < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2) ?
							ty :
							(ty - VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2)) :
						0;
					const UInt8 objTileY = isForObj ?
						(UInt8)ty :
						0;
					const UInt8 bgTile = (UInt8)(tx + bgTileY * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
					const TileDetail::Ref &bgRef = tileDetails[bank][(int)TileDetail::Usages::MAP][bgTile];
					const UInt8 objTile = (UInt8)(tx + objTileY * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
					const TileDetail::Ref &objRef = tileDetails[bank][(int)TileDetail::Usages::OBJ][objTile];
					const TileDetail::Array* details = nullptr;
					int refCount = 0;
					if (isForBg) {
						details = &bgRef.details; // Prefer MAP.
						refCount = bgRef.refCount;
						if (refCount == 0 && objRef.refCount > 0 && isForObj) {
							details = &objRef.details;
							refCount = objRef.refCount;
						}
					} else /* if (isForObj) */ {
						details = &objRef.details; // Prefer OAM.
						refCount = objRef.refCount;
						if (refCount == 0 && bgRef.refCount > 0 && isForBg) {
							details = &bgRef.details;
							refCount = bgRef.refCount;
						}
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
									col = palettes[(int)detail.usage + detail.oam.palette].color[val];
								}
							}
						} else {
							col = _device->classicPalette(val);
						}
					} else {
						const Colour col_ = _device->classicPalette(val);
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

		auto translateMap = [this, previewPaletteBits, isCgb] (
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
									col = _device->classicPalette(val);
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

	void begin(bool showTitle) {
		_options.startY = ImGui::GetCursorPosY();

		if (showTitle) {
			ImGui::AlignTextToFramePadding();
			ImGui::Dummy(ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::TextUnformatted(_theme->windowEmulator_VramDebugger());
		}
	}
	void end(void) {
		_options.safeHeight = (int)(ImGui::GetCursorPosY() - _options.startY + 48);
	}

	void tiles(bool showGrids) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_VramDebugger_Tiles().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		auto drawGrids = [] (const ImVec2 &curPos, const ImVec2 &dstSize) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const int w = (int)dstSize.x;
			const int h = (int)dstSize.y;
			for (int i = GBBASIC_TILE_SIZE; i < w; i += GBBASIC_TILE_SIZE) {
				const ImVec4 col = (i == w / 2) ? ImVec4(0.5f, 0.5f, 1, 0.25f) : ImVec4(1, 1, 1, 0.25f);
				drawList->AddLine(
					curPos + ImVec2((float)i, 0),
					curPos + ImVec2((float)i, (float)h),
					ImGui::GetColorU32(col)
				);
			}
			for (int j = GBBASIC_TILE_SIZE; j < h; j += GBBASIC_TILE_SIZE) {
				const ImVec4 col = (j == h / 3 || j == h / 3 * 2) ? ImVec4(0.5f, 0.5f, 1, 0.25f) : ImVec4(1, 1, 1, 0.25f);
				drawList->AddLine(
					curPos + ImVec2(0, (float)j),
					curPos + ImVec2((float)w, (float)j),
					ImGui::GetColorU32(col)
				);
			}
			drawList->AddRect(
				curPos + ImVec2(-1, -1),
				curPos + ImVec2((float)w, (float)h) + ImVec2(1, 1),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
			);
		};
		auto drawHighlights = [] (const ImVec2 &curPos, const TilesBuffer::Points &pos) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			for (const Math::Vec2i &pos_ : pos) {
				if (pos_ == Math::Vec2i(-1, -1))
					continue;

				const ImVec2 startPos((float)(pos_.x * GBBASIC_TILE_SIZE), (float)(pos_.y * GBBASIC_TILE_SIZE));
				const ImVec2 endPos = startPos + ImVec2(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
				drawList->AddRect(
					curPos + startPos,
					curPos + endPos + ImVec2(1, 1),
					ImGui::GetColorU32(ImVec4(1, 0, 0, 0.75f))
				);
			}
		};

		bool hasInfoForMap = false;
		bool hasInfoForOam = false;
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
					_tiles.buffer.texture->pointer(_renderer),
					dstSize,
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
				);

				if (showGrids)
					drawGrids(curPos, dstSize);

				drawHighlights(curPos, _tiles.highlights);

				if (ImGui::IsItemHovered()) {
					const ImVec2 mousePos = ImGui::GetMousePos();
					const ImVec2 diff = mousePos - curPos;
					const ImVec2 tilePos_ = diff / GBBASIC_TILE_SIZE;
					if (tilePos_.x >= 0 && tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 && tilePos_.y >= 0 && tilePos_.y < VRAM_DEBUGGER_TILES_AREA_HEIGHT) {
						hasInfoForMap = tilePos_.y >= VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT;
						hasInfoForOam = tilePos_.y < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2;
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
				_tiles.buffer.texture->pointer(_renderer),
				dstSize,
				ImVec2(0, 0), ImVec2(1, 1),
				ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
			);

			if (showGrids)
				drawGrids(curPos, dstSize);

			drawHighlights(curPos, _tiles.highlights);

			if (ImGui::IsItemHovered()) {
				const ImVec2 mousePos = ImGui::GetMousePos();
				const ImVec2 diff = mousePos - curPos;
				const ImVec2 tilePos_ = diff / GBBASIC_TILE_SIZE;
				if (tilePos_.x >= 0 && tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * 2 && tilePos_.y >= 0 && tilePos_.y < VRAM_DEBUGGER_TILES_AREA_HEIGHT) {
					hasInfoForMap = tilePos_.y >= VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT;
					hasInfoForOam = tilePos_.y < VRAM_DEBUGGER_TILES_SECTION_HALF_HEIGHT * 2;
					infoBank = tilePos_.x < VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK ? 0 : 1;
					tilePos = Math::Vec2i((int)tilePos_.x - VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * infoBank, (int)tilePos_.y);
				}
			}
		}

		if (hasInfoForMap || hasInfoForOam) {
			const UInt8 tile = (UInt8)(tilePos.x + tilePos.y * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
			const TileDetail::Ref &mapRef = _tileDetails[infoBank][(int)TileDetail::Usages::MAP][tile];
			const TileDetail::Ref &oamRef = _tileDetails[infoBank][(int)TileDetail::Usages::OBJ][tile];
			const TileDetail::Array &mapDetails = mapRef.details;
			const TileDetail::Array &oamDetails = oamRef.details;

			const UInt16 addr = (UInt16)(
				(0x8000 + (tilePos.x + tilePos.y * VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK) * 16)
			);
			int plt = -1;
			if (hasInfoForMap) {
				plt = mapDetails.empty() ? -1 : mapDetails.front().palette; // Prefer MAP palette.
				if (plt == -1)
					plt = oamDetails.empty() ? -1 : oamDetails.front().palette;
			} else if (hasInfoForOam) {
				plt = oamDetails.empty() ? -1 : oamDetails.front().palette; // Prefer OAM palette.
				if (plt == -1)
					plt = mapDetails.empty() ? -1 : mapDetails.front().palette;
			}
			int refCount = 0;
			if (hasInfoForMap)
				refCount += mapRef.refCount;
			if (hasInfoForOam)
				refCount += oamRef.refCount;
			_tileTips.refresh(
				_theme,
				tile,
				(UInt8)infoBank, addr,
				plt,
				refCount
			);
			if (!_tileTips.text.empty()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(_tileTips.text);
			}

			if (hasInfoForMap) {
				MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
				for (const TileDetail &detail : mapDetails) {
					highlights.push_back(detail.position);
				}
			}
			if (hasInfoForOam) {
				for (const TileDetail &detail : oamDetails) {
					_objs[detail.oam.index].highlight = true;
				}
			}
		}

		_tiles.highlights[0] = Math::Vec2i(-1, -1);
		_tiles.highlights[1] = Math::Vec2i(-1, -1);
	}
	void map(bool showGrids) {
		UInt8 bgX, bgY;
		_device->getBgScroll(&bgX, &bgY);
		UInt8 wndX, wndY;
		_device->getWindowScroll(&wndX, &wndY);

		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_VramDebugger_BgWinMap().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		ImGui::Dummy(ImVec2(2, 0));
		ImGui::SameLine();
		const float posX = ImGui::GetCursorPosX();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_theme->windowEmulator_VramDebugger_Layer());
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
					mapBuf->buffer.texture->pointer(_renderer),
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
				mapBuf->buffer.texture->pointer(_renderer),
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

			const Device::MapSourceTypes mapSrc = _options.isBgLayerActive ? _device->getMapSourceType(Device::LayerTypes::BG) : _device->getMapSourceType(Device::LayerTypes::WINDOW);
			const UInt16 base = mapSrc == Device::MapSourceTypes::FROM_9800_TO_9BFF ? 0x9800 : 0x9c00;
			const UInt16 addr = (UInt16)(
				(0x8000 + (
					tile < VRAM_DEBUGGER_TILES_SECTION_HALF_SIZE ?
						tile + VRAM_DEBUGGER_TILES_SECTION_SIZE :
						tile
				) * 16)
			);
			_mapTips.refresh(
				_theme,
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
			_tiles.highlights[0] = Math::Vec2i(tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank, ty_);
		}

		MapBuffer::Points &highlights = _options.isBgLayerActive ? _bgMap.highlights : _winMap.highlights;
		highlights.clear();
	}
	void oam(bool showGrids) {
		constexpr const int OBJECT_COUNT_PER_LINE = 8;

		ImGuiStyle &style = ImGui::GetStyle();

		Renderer* rnd = _renderer;

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_VramDebugger_Oam().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		auto drawInvisibieLine = [] (const ImVec2 &curPos, const ImVec2 &tileSize) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->AddLine(
				curPos + ImVec2(0, -1),
				curPos + tileSize + ImVec2(0, -1),
				ImGui::GetColorU32(ImVec4(1, 0, 0, 0.95f))
			);
		};
		auto drawGrid = [] (const ImVec2 &curPos, const ImVec2 &tileSize) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->AddRect(
				curPos,
				curPos + tileSize,
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f))
			);
		};
		auto drawHighlight = [] (const ImVec2 &curPos, const ImVec2 &tileSize, bool isFirst) -> void {
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->AddRect(
				curPos + ImVec2(isFirst ? 1.0f : 0.0f, 0.0f),
				curPos + tileSize,
				ImGui::GetColorU32(ImVec4(1, 0, 0, 0.75f))
			);
		};
		auto drawOams = [rnd, showGrids, OBJECT_COUNT_PER_LINE, drawInvisibieLine, drawGrid, drawHighlight] (bool is8x16Obj, ObjBuffer::Array &objs, const ImVec2 &tileSize) -> int {
			int hovering = -1;
			for (int i = 0; i < (int)objs.size(); ++i) {
				const int indexInLinePlusOne = (i + 1) % OBJECT_COUNT_PER_LINE;
				ObjBuffer &obj = objs[i];
				const bool visible = obj.visible;
				const bool highlight = obj.highlight;
				if (highlight)
					obj.highlight = false;

				if (is8x16Obj) {
					const ImVec2 curPos = ImGui::GetCursorScreenPos();
					const ImVec2 tileSize_(tileSize.x, tileSize.y * 2);
					ImGui::Image(
						obj.buffer.texture->pointer(rnd),
						tileSize_,
						ImVec2(0, 0), ImVec2(1, 1),
						ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
					);

					if (!visible)
						drawInvisibieLine(curPos, tileSize_);

					if (showGrids)
						drawGrid(curPos, tileSize_);

					if (highlight)
						drawHighlight(curPos, tileSize_, indexInLinePlusOne == 1);

					if (hovering == -1 && ImGui::IsItemHovered())
						hovering = i;
				} else {
					const ImVec2 curPos = ImGui::GetCursorScreenPos();
					ImGui::Image(
						obj.buffer.texture->pointer(rnd),
						tileSize,
						ImVec2(0, 0), ImVec2(1, 0.5f),
						ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 0.0f)
					);

					if (!visible)
						drawInvisibieLine(curPos, tileSize);

					if (showGrids)
						drawGrid(curPos, tileSize);

					if (highlight)
						drawHighlight(curPos, tileSize, indexInLinePlusOne == 1);

					if (hovering == -1 && ImGui::IsItemHovered())
						hovering = i;
				}
				ImGui::SameLine();
				if (indexInLinePlusOne == 0) {
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

		_inGameHighlight.highlighted = false;
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
				_theme,
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

			if (_tiles.highlights[0] == Math::Vec2i(-1, -1)) {
				const std::div_t tdiv = std::div(tile, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
				const int tx = tdiv.rem;
				const int ty = tdiv.quot % VRAM_DEBUGGER_TILES_AREA_HEIGHT;
				_tiles.highlights[0] = Math::Vec2i(tx + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank, ty);
				if (_is8x16Obj) {
					const UInt8 tile_ = tile + 1;
					const std::div_t tdiv_ = std::div(tile_, VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK);
					const int tx_ = tdiv_.rem;
					const int ty_ = tdiv_.quot % VRAM_DEBUGGER_TILES_AREA_HEIGHT;
					_tiles.highlights[1] = Math::Vec2i(tx_ + VRAM_DEBUGGER_TILES_AREA_WIDTH_PER_BANK * bank, ty_);
				}
			}

			int x_ = x;
			int y_ = y;
			if (x_ >= SCREEN_WIDTH)
				x_ -= 255;
			if (y_ >= SCREEN_HEIGHT)
				y_ -= 255;
			_inGameHighlight.highlighted = true;
			_inGameHighlight.area = Math::Recti::byXYWH(x_, y_, GBBASIC_TILE_SIZE, _is8x16Obj ? GBBASIC_TILE_SIZE * 2 : GBBASIC_TILE_SIZE);
		}
	}
	void palettes(void) {
		const bool isCgb = _device->isDeviceCgbCompatible();

		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_VramDebugger_Palettes().c_str(), regSize.x, flags))
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

							ImGui::ColorTooltip("", col, ImGuiColorEditFlags_None);
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

						ImGui::ColorTooltip("", col, ImGuiColorEditFlags_None);
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
	void status(void) {
		bool bgOn = _device->getBgDisplay();
		bool winOn = _device->getWindowDisplay();
		bool objOn = _device->getObjDisplay();

		const Device::TileSourceTypes tileSrc = _device->getTileSourceType();
		const UInt16 tileBase = tileSrc == Device::TileSourceTypes::FROM_8800_TO_97FF ? 0x8800 : 0x8000;
		const std::string &tileTxt = _tileDataArea.refresh(_theme->windowEmulator_VramDebugger_StatusReadonly_TileArea(), tileBase);

		const Device::MapSourceTypes mapSrc = _device->getMapSourceType(Device::LayerTypes::BG);
		const UInt16 mapBase = mapSrc == Device::MapSourceTypes::FROM_9800_TO_9BFF ? 0x9800 : 0x9c00;
		const std::string &mapTxt = _bgDataArea.refresh(_theme->windowEmulator_VramDebugger_StatusReadonly_BgArea(), mapBase);

		const Device::MapSourceTypes winSrc = _device->getMapSourceType(Device::LayerTypes::WINDOW);
		const UInt16 winBase = winSrc == Device::MapSourceTypes::FROM_9800_TO_9BFF ? 0x9800 : 0x9c00;
		const std::string &winTxt = _winDataArea.refresh(_theme->windowEmulator_VramDebugger_StatusReadonly_WinArea(), winBase);

		ImGuiStyle &style = ImGui::GetStyle();

		const float borderSize = style.ChildBorderSize;
		const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 regSize(
			regMax.x - regMin.x - borderSize * 2,
			regMax.y - regMin.y - borderSize * 2
		);

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!ImGui::CollapsingHeader(_theme->windowEmulator_VramDebugger_StatusReadonly().c_str(), regSize.x, flags))
			return;
		ImGui::NewLine(1);

		if (ImGui::Checkbox(_theme->windowEmulator_VramDebugger_StatusReadonly_BgOn(), &bgOn)) {
			// Do nothing.
		}
		if (ImGui::Checkbox(_theme->windowEmulator_VramDebugger_StatusReadonly_WinOn(), &winOn)) {
			// Do nothing.
		}
		if (ImGui::Checkbox(_theme->windowEmulator_VramDebugger_StatusReadonly_ObjOn(), &objOn)) {
			// Do nothing.
		}

		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(tileTxt);

		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(mapTxt);

		ImGui::Dummy(ImVec2(1, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(winTxt);
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
