/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "editor_actor.h"
#include "editor_code.h"
#include "editor_font.h"
#include "editor_map.h"
#include "editor_music.h"
#include "editor_scene.h"
#include "editor_sfx.h"
#include "editor_tiles.h"
#include "project.h"
#include "workspace.h"
#include "../compiler/compiler.h"
#include "../utils/archive.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/filesystem.h"
#include "../utils/rom_inspector.h"
#include "../../lib/jpath/jpath.hpp"
#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#endif /* GBBASIC_OS_HTML */

/*
** {===========================================================================
** Utilities
*/

#if defined GBBASIC_OS_HTML
EM_JS(
	void, projectFree, (void* ptr), {
		_free(ptr);
	}
);
EM_JS(
	const char*, projectGetSramFileName, (), {
		let ret = '';
		if (typeof getSramFileName == 'function')
			ret = getSramFileName();
		if (ret == null)
			ret = '';
		const lengthBytes = lengthBytesUTF8(ret) + 1;
		const stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes);

		return stringOnWasmHeap;
	}
);
#else /* GBBASIC_OS_HTML */
static void projectFree(void*) {
	// Do nothing.
}
static const char* projectGetSramFileName(void) {
	return nullptr;
}
#endif /* GBBASIC_OS_HTML */

/* ===========================================================================} */

/*
** {===========================================================================
** Project
*/

Project::Project(class Window* wnd, Renderer* rnd, class Workspace* ws) {
	contentType(ContentTypes::BASIC);
	fileSync(FileSync::Ptr(FileSync::create()));
	hasRtc(false);
	version("1.0.0");
	caseInsensitive(true);
	strictOn(true);
	superFeaturesEnabled(false);
	borderFrameType(BorderFrameTypes::NONE);
	customizedSuperPalettes(false);
	created(-1);
	modified(-1);
	order(PROJECT_DEFAULT_ORDER);
	preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
	preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
	preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
	preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
	preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
	preferencesPreviewAnchorPoint(true);
	preferencesPreviewPaletteBits(true);
	preferencesUseByteMatrix(false);
	preferencesShowGrids(true);
	preferencesCodePageForBindedRoutine(0);
	preferencesCodeLineForBindedRoutine(Left<int>(0));
	preferencesMapRef(0);
	preferencesMusicPreviewStroke(true);
	preferencesSfxShowSoundShape(true);
	preferencesActorApplyPropertiesToAllTiles(false);
	preferencesActorApplyPropertiesToAllFrames(true);
	preferencesActorUses8x16Sprites(true);
	preferencesSceneRefMap(0);
	preferencesSceneShowActors(true);
	preferencesSceneShowTriggers(true);
	preferencesSceneShowProperties(true);
	preferencesSceneShowAttributes(false);
	preferencesSceneUseGravity(false);

	window(wnd);
	renderer(rnd);
	workspace(ws);
	engine(0);
	hasDirtyInformation(false);
	hasDirtyAsset(false);
	hasDirtyEditor(false);
	toPollEditor(false);
	isPlain(false);
	preferPlain(false);
	isExample(false);
	activeMajorCodeIndex(0);
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	isMajorCodeEditorActive(true);
	isMinorCodeEditorEnabled(false);
	activeMinorCodeIndex(-1);
	minorCodeEditor(nullptr);
	minorCodeEditorWidth(0.0f);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	activePaletteIndex(-1);
	activeFontIndex(-1);
	fontPreviewHeight(0.0f);
	activeTilesIndex(-1);
	activeMapIndex(-1);
	activeMusicIndex(-1);
	activeSfxIndex(-1);
	sharedSfxEditor(nullptr);
	sharedSfxEditorDisabled(false);
	activeActorIndex(-1);
	activeSceneIndex(-1);

	inLibTextWidth(0.0f);
}

Project::~Project() {
	fileSync(nullptr);
}

Project &Project::operator = (const Project &other) {
	contentType(other.contentType());
	path(other.path());
	title(other.title());
	kernel(other.kernel());
	cartridgeType(other.cartridgeType());
	sramType(other.sramType());
	hasRtc(other.hasRtc());
	description(other.description());
	author(other.author());
	genre(other.genre());
	version(other.version());
	url(other.url());
	iconCode(other.iconCode());
	caseInsensitive(other.caseInsensitive());
	strictOn(other.strictOn());
	superFeaturesEnabled(other.superFeaturesEnabled());
	borderFrameType(other.borderFrameType());
	borderFrameCode(other.borderFrameCode());
	superPalettes(other.superPalettes());
	customizedSuperPalettes(other.customizedSuperPalettes());
	created(other.created());
	modified(other.modified());
	order(other.order());
	preferencesFontSize(other.preferencesFontSize());
	preferencesFontOffset(other.preferencesFontOffset());
	preferencesFontIsTwoBitsPerPixel(other.preferencesFontIsTwoBitsPerPixel());
	preferencesFontPreferFullWord(other.preferencesFontPreferFullWord());
	preferencesFontPreferFullWordForNonAscii(other.preferencesFontPreferFullWordForNonAscii());
	preferencesPreviewAnchorPoint(other.preferencesPreviewAnchorPoint());
	preferencesPreviewPaletteBits(other.preferencesPreviewPaletteBits());
	preferencesUseByteMatrix(other.preferencesUseByteMatrix());
	preferencesShowGrids(other.preferencesShowGrids());
	preferencesCodePageForBindedRoutine(other.preferencesCodePageForBindedRoutine());
	preferencesCodeLineForBindedRoutine(other.preferencesCodeLineForBindedRoutine());
	preferencesMapRef(other.preferencesMapRef());
	preferencesMusicPreviewStroke(other.preferencesMusicPreviewStroke());
	preferencesSfxShowSoundShape(other.preferencesSfxShowSoundShape());
	preferencesActorApplyPropertiesToAllTiles(other.preferencesActorApplyPropertiesToAllTiles());
	preferencesActorApplyPropertiesToAllFrames(other.preferencesActorApplyPropertiesToAllFrames());
	preferencesActorUses8x16Sprites(other.preferencesActorUses8x16Sprites());
	preferencesSceneRefMap(other.preferencesSceneRefMap());
	preferencesSceneShowActors(other.preferencesSceneShowActors());
	preferencesSceneShowTriggers(other.preferencesSceneShowTriggers());
	preferencesSceneShowProperties(other.preferencesSceneShowProperties());
	preferencesSceneShowAttributes(other.preferencesSceneShowAttributes());
	preferencesSceneUseGravity(other.preferencesSceneUseGravity());

	window(other.window());
	renderer(other.renderer());
	workspace(other.workspace());
	engine(other.engine());
	if (other.assets()) {
		assets(AssetsBundle::Ptr(new AssetsBundle()));
		assets()->palette = other.assets()->palette;
		assets()->fonts = other.assets()->fonts;
		assets()->code = other.assets()->code;
		assets()->tiles = other.assets()->tiles;
		assets()->maps = other.assets()->maps;
		assets()->music = other.assets()->music;
		assets()->sfx = other.assets()->sfx;
		assets()->actors = other.assets()->actors;
		assets()->scenes = other.assets()->scenes;

		TilesAssets &tilesAssets = assets()->tiles;
		for (int i = 0; i < tilesAssets.count(); ++i) {
			TilesAssets::Entry* entry = tilesAssets.get(i);
			entry->getPalette = paletteGetter();
		}
		MapAssets &mapAssets = assets()->maps;
		for (int i = 0; i < mapAssets.count(); ++i) {
			MapAssets::Entry* entry = mapAssets.get(i);
			entry->getTiles = tilesGetter();
		}
		ActorAssets &actorAssets = assets()->actors;
		for (int i = 0; i < actorAssets.count(); ++i) {
			ActorAssets::Entry* entry = actorAssets.get(i);
			entry->getPalette = paletteGetter();
		}
		SceneAssets &sceneAssets = assets()->scenes;
		for (int i = 0; i < sceneAssets.count(); ++i) {
			SceneAssets::Entry* entry = sceneAssets.get(i);
			entry->getMap = mapGetter();
			entry->getActor = actorGetter();
		}
	} else {
		assets(nullptr);
	}
	if (other.rom()) {
		rom(Bytes::Ptr(Bytes::create()));
		rom()->writeBytes(other.rom().get());
	} else {
		rom(nullptr);
	}
	attributesTexture(other.attributesTexture());
	propertiesTexture(other.propertiesTexture());
	actorsTexture(other.actorsTexture());
	behaviourSerializer(other.behaviourSerializer());
	behaviourParser(other.behaviourParser());
	hasDirtyInformation(other.hasDirtyInformation());
	hasDirtyAsset(other.hasDirtyAsset());
	hasDirtyEditor(other.hasDirtyEditor());
	toPollEditor(other.toPollEditor());
	isPlain(other.isPlain());
	preferPlain(other.preferPlain());
	isExample(other.isExample());
	activeMajorCodeIndex(other.activeMajorCodeIndex());
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	isMajorCodeEditorActive(other.isMajorCodeEditorActive());
	isMinorCodeEditorEnabled(other.isMinorCodeEditorEnabled());
	activeMinorCodeIndex(other.activeMinorCodeIndex());
	minorCodeEditor(nullptr);
	minorCodeEditorWidth(other.minorCodeEditorWidth());
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	activePaletteIndex(other.activePaletteIndex());
	activeFontIndex(other.activeFontIndex());
	fontPreviewHeight(other.fontPreviewHeight());
	activeTilesIndex(other.activeTilesIndex());
	activeMapIndex(other.activeMapIndex());
	activeMusicIndex(other.activeMusicIndex());
	activeSfxIndex(other.activeSfxIndex());
	sharedSfxEditor(nullptr);
	sharedSfxEditorDisabled(other.sharedSfxEditorDisabled());
	activeActorIndex(other.activeActorIndex());
	activeSceneIndex(other.activeSceneIndex());
	shortTitle(other.shortTitle());
	createdString(other.createdString());
	modifiedString(other.modifiedString());

	inLibTextWidth(other.inLibTextWidth());

	_iconTexture = other._iconTexture;
	_iconTextureDirty = other._iconTextureDirty;
	_iconTexture2Bpp = other._iconTexture2Bpp;
	_iconTexture2BppDirty = other._iconTexture2BppDirty;

	return *this;
}

unsigned Project::type(void) const {
	return TYPE();
}

void Project::title(const std::string &val) {
	_title = val;

	std::wstring wstr = Unicode::toWide(_title);
	if (wstr.length() < 20) {
		shortTitle(_title);
	} else {
		const std::wstring begin = wstr.substr(0, 11);
		const std::wstring end = wstr.substr(wstr.length() - 6);
		wstr = begin + L"..." + end;
		shortTitle(Unicode::fromWide(wstr));
	}
}

void Project::iconCode(const std::string &val) {
	if (_iconCode == val)
		return;

	_iconCode = val;

	_iconTextureDirty = true;
	_iconTexture2BppDirty = true;
}

Texture::Ptr &Project::touchIconTexture(void) {
	if (_iconTextureIsBeingGenerated.working())
		return _iconTexture;

	if (!_iconTextureDirty)
		return _iconTexture;

	_iconTextureDirty = false;

	if (_iconTexture)
		_iconTexture = nullptr;

	if (iconCode().empty())
		return _iconTexture;

	_iconTextureIsBeingGenerated = workspace()->async(
		std::bind(
			[] (WorkTask* /* task */, const std::string code) -> uintptr_t { // On work thread.
				Bytes::Ptr bytes(Bytes::create());
				if (!Base64::toBytes(bytes.get(), code))
					return 0;

				Image* img = Image::create();
				if (!img->fromBytes(bytes.get())) {
					Image::destroy(img);

					return 0;
				}

				if (img->width() != GBBASIC_ICON_WIDTH || img->height() != GBBASIC_ICON_HEIGHT) {
					int w = 0;
					int h = 0;
					if ((float)img->width() / img->height() < (float)GBBASIC_ICON_WIDTH / GBBASIC_ICON_HEIGHT) {
						w = GBBASIC_ICON_WIDTH;
						h = (int)(GBBASIC_ICON_WIDTH * ((float)img->height() / img->width()));
					} else {
						w = (int)(GBBASIC_ICON_HEIGHT * ((float)img->width() / img->height()));
						h = GBBASIC_ICON_HEIGHT;
					}
					if (!img->resize(w, h, true)) {
						Image::destroy(img);

						return 0;
					}

					Image* tmp = Image::create();
					if (!tmp->fromBlank(GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, 0)) {
						Image::destroy(img);
						Image::destroy(tmp);

						return 0;
					}
					const int x = (w - GBBASIC_ICON_WIDTH) / 2;
					const int y = (h - GBBASIC_ICON_HEIGHT) / 2;
					if (!img->blit(tmp, 0, 0, GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, x, y)) {
						Image::destroy(img);
						Image::destroy(tmp);

						return 0;
					}
					std::swap(img, tmp);
					Image::destroy(tmp);
				}
				const Math::Vec2i corner[] = {
					Math::Vec2i( 0,  0), Math::Vec2i( 1,  0), Math::Vec2i( 2,  0), Math::Vec2i( 0,  1), Math::Vec2i( 1,  1), Math::Vec2i( 0,  2),
					Math::Vec2i(31,  0), Math::Vec2i(30,  0), Math::Vec2i(29,  0), Math::Vec2i(31,  1), Math::Vec2i(30,  1), Math::Vec2i(31,  2),
					Math::Vec2i( 0, 31), Math::Vec2i( 1, 31), Math::Vec2i( 2, 31), Math::Vec2i( 0, 30), Math::Vec2i( 1, 30), Math::Vec2i( 0, 29),
					Math::Vec2i(31, 31), Math::Vec2i(30, 31), Math::Vec2i(29, 31), Math::Vec2i(31, 30), Math::Vec2i(30, 30), Math::Vec2i(31, 29)
				};
				for (const Math::Vec2i &c : corner) {
					img->set(c.x, c.y, Colour(255, 255, 255, 0));
				}

				return (uintptr_t)img;
			},
			std::placeholders::_1, iconCode()
		),
		[this] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
			Image* img = (Image*)ptr;
			Texture::Ptr tex(Texture::create());
			if (!tex->fromImage(renderer(), Texture::Usages::STATIC, img, Texture::ScaleModes::NEAREST))
				return;

			_iconTexture = tex;
		},
		[] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
			Image* img = (Image*)ptr;
			Image::destroy(img);
		}
	);

	return _iconTexture;
}

Texture::Ptr &Project::touchIconTexture2Bpp(void) {
	if (_iconTexture2BppDirtyIsBeingGenerated.working())
		return _iconTexture2Bpp;

	if (!_iconTexture2BppDirty)
		return _iconTexture2Bpp;

	_iconTexture2BppDirty = false;

	if (_iconTexture2Bpp)
		_iconTexture2Bpp = nullptr;

	if (iconCode().empty())
		return _iconTexture;

	_iconTexture2BppDirtyIsBeingGenerated = workspace()->async(
		std::bind(
			[] (WorkTask* /* task */, const std::string code) -> uintptr_t { // On work thread.
				Bytes::Ptr bytes(Bytes::create());
				if (!Base64::toBytes(bytes.get(), code))
					return 0;

				Image::Ptr img(Image::create());
				if (!img->fromBytes(bytes.get()))
					return 0;

				if (img->width() != GBBASIC_ICON_WIDTH || img->height() != GBBASIC_ICON_HEIGHT) {
					int w = 0;
					int h = 0;
					if ((float)img->width() / img->height() < (float)GBBASIC_ICON_WIDTH / GBBASIC_ICON_HEIGHT) {
						w = GBBASIC_ICON_WIDTH;
						h = (int)(GBBASIC_ICON_WIDTH * ((float)img->height() / img->width()));
					} else {
						w = (int)(GBBASIC_ICON_HEIGHT * ((float)img->width() / img->height()));
						h = GBBASIC_ICON_HEIGHT;
					}
					if (!img->resize(w, h, true)) {
						return 0;
					}

					Image::Ptr tmp(Image::create());
					if (!tmp->fromBlank(GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, 0)) {
						return 0;
					}
					const int x = (w - GBBASIC_ICON_WIDTH) / 2;
					const int y = (h - GBBASIC_ICON_HEIGHT) / 2;
					if (!img->blit(tmp.get(), 0, 0, GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, x, y)) {
						return 0;
					}
					std::swap(img, tmp);
				}

				Image* paletted = img->quantized2Bpp();

				return (uintptr_t)paletted;
			},
			std::placeholders::_1, iconCode()
		),
		[this] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
			Image* img = (Image*)ptr;
			Texture::Ptr tex(Texture::create());
			if (!tex->fromImage(renderer(), Texture::Usages::STATIC, img, Texture::ScaleModes::NEAREST))
				return;

			_iconTexture2Bpp = tex;
		},
		[] (WorkTask* /* task */, uintptr_t ptr) -> void { // On main thread.
			Image* img = (Image*)ptr;
			Image::destroy(img);
		}
	);

	return _iconTexture2Bpp;
}

Image::Ptr Project::touchIconImage2Bpp(Bytes::Ptr tiles) {
	if (tiles)
		tiles->clear();

	if (iconCode().empty())
		return nullptr;

	Bytes::Ptr bytes(Bytes::create());
	if (!Base64::toBytes(bytes.get(), iconCode()))
		return nullptr;

	Image::Ptr img(Image::create());
	if (!img->fromBytes(bytes.get()))
		return nullptr;

	if (img->width() != GBBASIC_ICON_WIDTH || img->height() != GBBASIC_ICON_HEIGHT) {
		int w = 0;
		int h = 0;
		if ((float)img->width() / img->height() < (float)GBBASIC_ICON_WIDTH / GBBASIC_ICON_HEIGHT) {
			w = GBBASIC_ICON_WIDTH;
			h = (int)(GBBASIC_ICON_WIDTH * ((float)img->height() / img->width()));
		} else {
			w = (int)(GBBASIC_ICON_HEIGHT * ((float)img->width() / img->height()));
			h = GBBASIC_ICON_HEIGHT;
		}
		if (!img->resize(w, h, true)) {
			return 0;
		}

		Image::Ptr tmp(Image::create());
		if (!tmp->fromBlank(GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, 0)) {
			return 0;
		}
		const int x = (w - GBBASIC_ICON_WIDTH) / 2;
		const int y = (h - GBBASIC_ICON_HEIGHT) / 2;
		if (!img->blit(tmp.get(), 0, 0, GBBASIC_ICON_WIDTH, GBBASIC_ICON_HEIGHT, x, y)) {
			return 0;
		}
		std::swap(img, tmp);
	}

	Image::Ptr paletted(img->quantized2Bpp());

	if (tiles) {
		for (int j = 0; j < paletted->height(); j += GBBASIC_TILE_SIZE * 2) {
			for (int i = 0; i < paletted->width(); i += GBBASIC_TILE_SIZE) {
				for (int k = 0; k < 2; ++k) {
					for (int m = 0; m < GBBASIC_TILE_SIZE; ++m) {
						const int y = j + GBBASIC_TILE_SIZE * k + m;
						Byte ln0 = 0;
						Byte ln1 = 0;
						for (int n = 0; n < GBBASIC_TILE_SIZE; ++n) {
							const int x = i + n;
							int idx = 0;
							paletted->get(x, y, idx);
							const int p = (GBBASIC_TILE_SIZE - 1) - n;
							switch (idx) {
							case 0: /* Do nothing. */             break;
							case 1: ln0 |= 1 << p;                break;
							case 2: ln1 |= 1 << p;                break;
							case 3: ln0 |= 1 << p; ln1 |= 1 << p; break;
							}
						}
						tiles->writeByte(ln0);
						tiles->writeByte(ln1);
					}
				}
			}
		}
	}

	return paletted;
}

void Project::borderFrameType(BorderFrameTypes y) {
	if (_borderFrameType == y)
		return;

	_borderFrameType = y;
}

void Project::borderFrameCode(const std::string &val) {
	if (_borderFrameCode == val)
		return;

	_borderFrameCode = val;
}

void Project::superPalettes(const SuperPalettes &val) {
	if (_superPalettes == val)
		return;

	_superPalettes = val;
}

void Project::superPalettes(const Colour* val) {
	if (memcmp(&_superPalettes.front(), val, _superPalettes.size() * sizeof(Colour)) == 0)
		return;

	memcpy(&_superPalettes.front(), val, _superPalettes.size() * sizeof(Colour));
}

int Project::countSuperPaletteColor(void) const {
	return (int)_superPalettes.size();
}

const Colour* Project::getSuperPaletteColor(int idx) const {
	if (idx < 0 || idx >= (int)_superPalettes.size())
		return nullptr;

	return &_superPalettes[idx];
}

bool Project::setSuperPaletteColor(int idx, const Colour &val) {
	if (idx < 0 || idx >= (int)_superPalettes.size())
		return false;

	_superPalettes[idx] = val;

	customizedSuperPalettes(true);

	return true;
}

void Project::synchronizeSuperPalettes(const PaletteAssets &val) {
	if (customizedSuperPalettes()) // Ignore.
		return;

	int k = 0;
	for (int i = 0; i < 4; ++i) {
		const PaletteAssets::Entry* entry = val.get(i);
		if (!entry)
			continue;

		for (int j = 0; j < 4; ++j) {
			Colour col;
			if (!entry->data->get(j + (i % 2), col))
				continue;

			if (k != 0 && (k % 7) == 0)
				_superPalettes[k++] = _superPalettes[0];
			else
				_superPalettes[k++] = col;
		}
	}
}

void Project::created(const long long &val) {
	if (_created == val)
		return;

	_created = val;
	createdString(DateTime::toString(_created));
}

void Project::modified(const long long &val) {
	if (_modified == val)
		return;

	_modified = val;
	modifiedString(DateTime::toString(_modified));
}

int Project::palettePageCount(void) const {
	if (!assets())
		return 0;

	const PaletteAssets &assets_ = assets()->palette;

	return assets_.count();
}

bool Project::addGlobalPalettePages(WarningOrErrorHandler onWarningOrError) {
	if (!assets())
		return false;

	PaletteAssets palette;
	assets()->palette = palette;

	return true;
}

const PaletteAssets::Entry* Project::getPalette(int index) const {
	if (!assets())
		return nullptr;

	PaletteAssets &assets_ = assets()->palette;

	return assets_.get(index);
}

PaletteAssets::Entry* Project::getPalette(int index) {
	if (!assets())
		return nullptr;

	PaletteAssets &assets_ = assets()->palette;

	return assets_.get(index);
}

PaletteAssets &Project::touchPalette(void) {
	PaletteAssets &assets_ = assets()->palette;

	if (assets_.empty()) {
		addGlobalPalettePages(nullptr);
		activePaletteIndex(0);
	}

	return assets_;
}

PaletteAssets::Getter Project::paletteGetter(void) {
	return [this] (int index) -> PaletteAssets::Entry* {
		return getPalette(index);
	};
}

Bytes::Ptr Project::backgroundPalettes(void) {
	const PaletteAssets &assets_ = touchPalette();

	return assets_.serializeBackgroundPalettes();
}

Bytes::Ptr Project::spritePalettes(void) {
	const PaletteAssets &assets_ = touchPalette();

	return assets_.serializeSpritePalettes();
}

int Project::fontPageCount(void) const {
	if (!assets())
		return 0;

	const FontAssets &assets_ = assets()->fonts;

	return assets_.count();
}

bool Project::addGlobalFontPages(const char* fontConfigPath, WarningOrErrorHandler onWarningOrError) {
	if (!fontConfigPath)
		return false;

	std::string dir;
	Path::split(fontConfigPath, nullptr, nullptr, &dir);

	File::Ptr file(File::create());
	if (!file->open(fontConfigPath, Stream::READ))
		return false;

	std::string txt;
	if (!file->readString(txt)) {
		file->close();

		return false;
	}
	file->close();

	FontAssets fonts;
	if (fonts.fromString(txt, dir, false, onWarningOrError)) {
		for (int i = 0; i < fonts.count(); ++i) {
			const FontAssets::Entry* entry = fonts.get(i);
			if (!entry)
				continue;

			assets()->fonts.add(*entry);
		}
	}

	return true;
}

void Project::makeFontEntry(FontAssets::Entry &entry, bool toBeCopied, bool toBeEmbedded, const std::string &path_, const Math::Vec2i* size, const std::string &content_, bool generateObject) {
	std::string dir;
	std::string finalPath = path_;
	do {
		if (path().empty())
			break;
		Path::split(path(), nullptr, nullptr, &dir);

		if (!toBeCopied)
			break;
		std::string name;
		std::string ext;
		Path::split(finalPath, &name, &ext, nullptr);
		const std::string relPath = name + "." + ext;
		const std::string newPath = Path::combine(dir.c_str(), relPath.c_str());
		const std::string absPath0 = Path::absoluteOf(finalPath);
		const std::string absPath1 = Path::absoluteOf(newPath);
		if (absPath0 == absPath1 || Path::copyFile(finalPath.c_str(), newPath.c_str())) {
			// Do nothing.
		} else {
			toBeCopied = false;

			break;
		}
		finalPath = relPath;
	} while (false);

	entry = FontAssets::Entry(toBeCopied, toBeEmbedded, dir, finalPath, size, content_, generateObject);
}

bool Project::addFontPage(bool toBeCopied, bool toBeEmbedded, const std::string &path_, const Math::Vec2i* size, const std::string &content_, bool isNew) {
	FontAssets &assets_ = assets()->fonts;
	FontAssets::Entry entry_;
	makeFontEntry(entry_, toBeCopied, toBeEmbedded, path_, size, content_, true);
	const bool result = assets_.add(entry_);
	if (result && isNew) {
		const int index = assets_.count() - 1;
		FontAssets::Entry* entry = assets_.get(index);
		if (entry->name.empty())
			entry->name = getUsableFontName(index); // Unique name.
	}
	for (int i = 0; i < assets_.count(); ++i) {
		FontAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::duplicateFontPage(const FontAssets::Entry* src) {
	FontAssets &assets_ = assets()->fonts;
	FontAssets::Entry entry_ = *src;
	entry_.editor = nullptr;
	entry_.name = getUsableFontName(-1);
	entry_.isAsset = true;
	entry_.object = nullptr;
	entry_.glyphs.entries.clear();
	entry_.bank = 0;
	entry_.address = 0;
	entry_.cleanupSubstitution();
	if (!entry_.touch())
		return false;

	const bool result = assets_.add(entry_);
	for (int i = 0; i < assets_.count(); ++i) {
		FontAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeFontPage(int index) {
	FontAssets &assets_ = assets()->fonts;
	if (index < 0 || index >= assets_.count())
		return false;

	FontAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorFont::destroy((EditorFont*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

void Project::clearFontPages(void) {
	FontAssets &assets_ = assets()->fonts;
	assets_.clear();
}

const FontAssets::Entry* Project::getFont(int index) const {
	FontAssets &assets_ = assets()->fonts;

	return assets_.get(index);
}

FontAssets::Entry* Project::getFont(int index) {
	FontAssets &assets_ = assets()->fonts;

	return assets_.get(index);
}

bool Project::canRenameFont(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const FontAssets &assets_ = assets()->fonts;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const FontAssets::Entry* entry = assets_.get(i);
		if (entry->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableFontName(int index) const {
	int id = 0;
	std::string name = "Font" + Text::toString(id);
	while (!canRenameFont(index, name, nullptr)) {
		++id;
		name = "Font" + Text::toString(id);
	}

	return name;
}

int Project::codePageCount(void) const {
	if (!assets())
		return 0;

	const CodeAssets &assets_ = assets()->code;

	return assets_.count();
}

bool Project::addCodePage(const std::string &val) {
	CodeAssets &assets_ = assets()->code;
	const bool result = assets_.add(val);
	for (int i = 0; i < assets_.count(); ++i) {
		CodeAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeCodePage(int index) {
	CodeAssets &assets_ = assets()->code;
	if (index < 0 || index >= assets_.count())
		return false;

	CodeAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorCode::destroy((EditorCode*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const CodeAssets::Entry* Project::getCode(int index) const {
	CodeAssets &assets_ = assets()->code;

	return assets_.get(index);
}

CodeAssets::Entry* Project::getCode(int index) {
	CodeAssets &assets_ = assets()->code;

	return assets_.get(index);
}

int Project::tilesPageCount(void) const {
	if (!assets())
		return 0;

	const TilesAssets &assets_ = assets()->tiles;

	return assets_.count();
}

bool Project::addTilesPage(const std::string &val, bool isNew) {
	TilesAssets &assets_ = assets()->tiles;
	const bool result = assets_.add(TilesAssets::Entry(renderer(), val, paletteGetter()));
	if (result && isNew) {
		const int index = assets_.count() - 1;
		TilesAssets::Entry* entry = assets_.get(index);
		if (entry->name.empty())
			entry->name = getUsableTilesName(index); // Unique name.
	}
	for (int i = 0; i < assets_.count(); ++i) {
		TilesAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeTilesPage(int index) {
	TilesAssets &assets_ = assets()->tiles;
	if (index < 0 || index >= assets_.count())
		return false;

	TilesAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorTiles::destroy((EditorTiles*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const TilesAssets::Entry* Project::getTiles(int index) const {
	TilesAssets &assets_ = assets()->tiles;

	return assets_.get(index);
}

TilesAssets::Entry* Project::getTiles(int index) {
	TilesAssets &assets_ = assets()->tiles;

	return assets_.get(index);
}

TilesAssets::Getter Project::tilesGetter(void) {
	return [this] (int index) -> TilesAssets::Entry* {
		return getTiles(index);
	};
}

bool Project::canRenameTiles(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const TilesAssets &assets_ = assets()->tiles;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const TilesAssets::Entry* entry = assets_.get(i);
		if (entry->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableTilesName(int index) const {
	int id = 0;
	std::string name = "Tiles" + Text::toString(id);
	while (!canRenameTiles(index, name, nullptr)) {
		++id;
		name = "Tiles" + Text::toString(id);
	}

	return name;
}

int Project::mapPageCount(void) const {
	if (!assets())
		return 0;

	const MapAssets &assets_ = assets()->maps;

	return assets_.count();
}

bool Project::addMapPage(const std::string &val, bool isNew, const char* preferedName /* nullable */) {
	MapAssets &assets_ = assets()->maps;
	const bool result = assets_.add(MapAssets::Entry(val, tilesGetter(), attributesTexture()));
	if (result && isNew) {
		const int index = assets_.count() - 1;
		MapAssets::Entry* entry = assets_.get(index);
		if (entry->name.empty()) {
			if (preferedName && canRenameMap(index, preferedName, nullptr))
				entry->name = preferedName;
			else
				entry->name = getUsableMapName(index); // Unique name.
		}
	}
	for (int i = 0; i < assets_.count(); ++i) {
		MapAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeMapPage(int index) {
	MapAssets &assets_ = assets()->maps;
	if (index < 0 || index >= assets_.count())
		return false;

	MapAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorMap::destroy((EditorMap*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const MapAssets::Entry* Project::getMap(int index) const {
	MapAssets &assets_ = assets()->maps;

	return assets_.get(index);
}

MapAssets::Entry* Project::getMap(int index) {
	MapAssets &assets_ = assets()->maps;

	return assets_.get(index);
}

MapAssets::Getter Project::mapGetter(void) {
	return [this] (int index) -> MapAssets::Entry* {
		return getMap(index);
	};
}

bool Project::canRenameMap(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const MapAssets &assets_ = assets()->maps;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const MapAssets::Entry* entry = assets_.get(i);
		if (entry->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableMapName(int index) const {
	int id = 0;
	std::string name = "Map" + Text::toString(id);
	while (!canRenameMap(index, name, nullptr)) {
		++id;
		name = "Map" + Text::toString(id);
	}

	return name;
}

Project::Indices Project::getMapsRefToTiles(int tilesIndex) const {
	Indices result;
	const MapAssets &assets_ = assets()->maps;
	for (int i = 0; i < assets_.count(); ++i) {
		const MapAssets::Entry* entry = assets_.get(i);
		if (entry->ref == tilesIndex)
			result.push_back(i);
	}

	return result;
}

int Project::musicPageCount(void) const {
	if (!assets())
		return 0;

	const MusicAssets &assets_ = assets()->music;

	return assets_.count();
}

bool Project::addMusicPage(const std::string &val, bool isNew) {
	MusicAssets &assets_ = assets()->music;
	const bool result = assets_.add(MusicAssets::Entry(val));
	if (result && isNew) {
		const int index = assets_.count() - 1;
		MusicAssets::Entry* entry = assets_.get(index);
		Music::Ptr &music = entry->data;
		Music::Song* song = music->pointer();
		if (song->name.empty())
			song->name = getUsableMusicName(index); // Unique name.
		for (int i = 0; i < song->instrumentCount(); ++i) {
			Music::BaseInstrument* inst = song->instrumentAt(i, nullptr);
			if (inst->name.empty())
				inst->name = getUsableMusicInstrumentName(index, i); // Unique name.
		}
	}
	for (int i = 0; i < assets_.count(); ++i) {
		MusicAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeMusicPage(int index) {
	MusicAssets &assets_ = assets()->music;
	if (index < 0 || index >= assets_.count())
		return false;

	MusicAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorMusic::destroy((EditorMusic*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const MusicAssets::Entry* Project::getMusic(int index) const {
	MusicAssets &assets_ = assets()->music;

	return assets_.get(index);
}

MusicAssets::Entry* Project::getMusic(int index) {
	MusicAssets &assets_ = assets()->music;

	return assets_.get(index);
}

bool Project::canRenameMusic(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const MusicAssets &assets_ = assets()->music;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const MusicAssets::Entry* entry = assets_.get(i);
		if (entry->data->pointer()->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableMusicName(int index) const {
	int id = 0;
	std::string name = "Music" + Text::toString(id);
	while (!canRenameMusic(index, name, nullptr)) {
		++id;
		name = "Music" + Text::toString(id);
	}

	return name;
}

std::string Project::getUsableMusicName(const std::string &prefered, int index) const {
	if (canRenameMusic(index, prefered, nullptr))
		return prefered;

	int id = 0;
	std::string name = prefered + Text::toString(id);
	while (!canRenameMusic(index, name, nullptr)) {
		++id;
		name = prefered + Text::toString(id);
	}

	return name;
}

bool Project::canRenameMusicInstrument(int index, int instIndex, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	const MusicAssets &assets_ = assets()->music;
	if (index < 0 || index >= assets_.count())
		return false;

	const MusicAssets::Entry* entry = assets_.get(index);
	const Music::Song* song = entry->data->pointer();
	for (int i = 0; i < song->instrumentCount(); ++i) {
		if (i == instIndex)
			continue;

		const Music::BaseInstrument* inst = song->instrumentAt(i, nullptr);
		if (inst->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableMusicInstrumentName(int index, int instIndex) const {
	std::string prefix = "Instrument";
	do {
		const MusicAssets &assets_ = assets()->music;
		if (index < 0 || index >= assets_.count())
			break;

		const MusicAssets::Entry* entry = assets_.get(index);
		const Music::Song* song = entry->data->pointer();
		Music::Instruments type;
		const Music::BaseInstrument* inst = song->instrumentAt(instIndex, &type);
		if (!inst)
			break;

		switch (type) {
		case Music::Instruments::SQUARE:
			prefix = "Duty";

			break;
		case Music::Instruments::WAVE:
			prefix = "Wave";

			break;
		case Music::Instruments::NOISE:
			prefix = "Noise";

			break;
		}
	} while (false);

	int id = 1;
	std::string name = prefix + Text::toString(id);
	while (!canRenameMusicInstrument(index, instIndex, name, nullptr)) {
		++id;
		name = prefix + Text::toString(id);
	}

	return name;
}

int Project::sfxPageCount(void) const {
	if (!assets())
		return 0;

	const SfxAssets &assets_ = assets()->sfx;

	return assets_.count();
}

bool Project::addSfxPage(const std::string &val, bool isNew, bool insertBefore, bool insertAfter) {
	SfxAssets &assets_ = assets()->sfx;
	bool result = false;
	if (isNew) {
		int index = 0;
		if (!assets_.empty()) {
			if (insertBefore)
				index = activeSfxIndex();
			else if (insertAfter)
				index = activeSfxIndex() + 1;
			else
				index = sfxPageCount();
		}
		result = assets_.insert(SfxAssets::Entry(val), index);
		if (result) {
			SfxAssets::Entry* entry = assets_.get(index);
			Sfx::Ptr &sfx = entry->data;
			Sfx::Sound* sound = sfx->pointer();
			if (sound->name.empty())
				sound->name = getUsableSfxName(index); // Unique name.

			sharedSfxEditor()->added(entry, index); // After adding.
		}
	} else {
		result = assets_.add(SfxAssets::Entry(val));
	}
	for (int i = 0; i < assets_.count(); ++i) {
		SfxAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeSfxPage(int index) {
	SfxAssets &assets_ = assets()->sfx;
	if (index < 0 || index >= assets_.count())
		return false;

	SfxAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		sharedSfxEditor()->removed(entry, index); // Before removing.

		entry->editor->close(index);
		EditorSfx::destroy((EditorSfx*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const SfxAssets::Entry* Project::getSfx(int index) const {
	SfxAssets &assets_ = assets()->sfx;

	return assets_.get(index);
}

SfxAssets::Entry* Project::getSfx(int index) {
	SfxAssets &assets_ = assets()->sfx;

	return assets_.get(index);
}

bool Project::swapSfxPages(int leftIndex, int rightIndex) {
	SfxAssets &assets_ = assets()->sfx;

	return assets_.swap(leftIndex, rightIndex);
}

bool Project::canRenameSfx(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const SfxAssets &assets_ = assets()->sfx;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const SfxAssets::Entry* entry = assets_.get(i);
		if (entry->data->pointer()->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableSfxName(int index) const {
	return getUsableSfxName("Sfx", index);
}

std::string Project::getUsableSfxName(const std::string &prefix, int index) const {
	int id = 0;
	std::string name = prefix + Text::toString(id);
	while (!canRenameSfx(index, name, nullptr)) {
		++id;
		name = prefix + Text::toString(id);
	}

	return name;
}

void Project::createSharedSfxEditor(void) {
	if (sharedSfxEditorDisabled())
		return;

	if (sharedSfxEditor())
		return;

	sharedSfxEditor(EditorSfx::create(window(), renderer(), workspace(), this));
}

void Project::destroySharedSfxEditor(void) {
	if (!sharedSfxEditor())
		return;

	sharedSfxEditor(EditorSfx::destroy(sharedSfxEditor()));
}

int Project::actorPageCount(void) const {
	if (!assets())
		return 0;

	const ActorAssets &assets_ = assets()->actors;

	return assets_.count();
}

bool Project::addActorPage(const std::string &val, bool isNew) {
	ActorAssets &assets_ = assets()->actors;
	const bool result = assets_.add(ActorAssets::Entry(renderer(), val, paletteGetter(), behaviourSerializer(), behaviourParser()));
	if (result && isNew) {
		const int index = assets_.count() - 1;
		ActorAssets::Entry* entry = assets_.get(index);
		if (entry->name.empty())
			entry->name = getUsableActorName(index); // Unique name.
	}
	for (int i = 0; i < assets_.count(); ++i) {
		ActorAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeActorPage(int index) {
	ActorAssets &assets_ = assets()->actors;
	if (index < 0 || index >= assets_.count())
		return false;

	ActorAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorActor::destroy((EditorActor*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const ActorAssets::Entry* Project::getActor(int index) const {
	ActorAssets &assets_ = assets()->actors;

	return assets_.get(index);
}

ActorAssets::Entry* Project::getActor(int index) {
	ActorAssets &assets_ = assets()->actors;

	return assets_.get(index);
}

ActorAssets::Getter Project::actorGetter(void) {
	return [this] (int index) -> ActorAssets::Entry* {
		return getActor(index);
	};
}

bool Project::canRenameActor(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const ActorAssets &assets_ = assets()->actors;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const ActorAssets::Entry* entry = assets_.get(i);
		if (entry->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableActorName(int index) const {
	int id = 0;
	std::string name = "Actor" + Text::toString(id);
	while (!canRenameActor(index, name, nullptr)) {
		++id;
		name = "Actor" + Text::toString(id);
	}

	return name;
}

int Project::scenePageCount(void) const {
	if (!assets())
		return 0;

	const SceneAssets &assets_ = assets()->scenes;

	return assets_.count();
}

bool Project::addScenePage(const std::string &val, bool isNew, const char* preferedName) {
	SceneAssets &assets_ = assets()->scenes;
	const bool result = assets_.add(SceneAssets::Entry(val, mapGetter(), actorGetter(), propertiesTexture(), actorsTexture()));
	if (result && isNew) {
		const int index = assets_.count() - 1;
		SceneAssets::Entry* entry = assets_.get(index);
		if (entry->name.empty()) {
			if (preferedName && canRenameScene(index, preferedName, nullptr))
				entry->name = preferedName;
			else
				entry->name = getUsableSceneName(index); // Unique name.
		}
	}
	for (int i = 0; i < assets_.count(); ++i) {
		SceneAssets::Entry* entry = assets_.get(i);
		if (entry->editor)
			entry->editor->statusInvalidated();
	}

	return result;
}

bool Project::removeScenePage(int index) {
	SceneAssets &assets_ = assets()->scenes;
	if (index < 0 || index >= assets_.count())
		return false;

	SceneAssets::Entry* entry = assets_.get(index);
	if (entry->editor) {
		entry->editor->close(index);
		EditorScene::destroy((EditorScene*)entry->editor);
		entry->editor = nullptr;
	}
	if (!assets_.remove(index))
		return false;

	return true;
}

const SceneAssets::Entry* Project::getScene(int index) const {
	SceneAssets &assets_ = assets()->scenes;

	return assets_.get(index);
}

SceneAssets::Entry* Project::getScene(int index) {
	SceneAssets &assets_ = assets()->scenes;

	return assets_.get(index);
}

bool Project::canRenameScene(int index, const std::string &name, int* another) const {
	if (another)
		*another = -1;

	if (name.empty())
		return false;

	const SceneAssets &assets_ = assets()->scenes;
	for (int i = 0; i < assets_.count(); ++i) {
		if (i == index)
			continue;

		const SceneAssets::Entry* entry = assets_.get(i);
		if (entry->name == name) {
			if (another)
				*another = i;

			return false;
		}
	}

	return true;
}

std::string Project::getUsableSceneName(int index) const {
	int id = 0;
	std::string name = "Scene" + Text::toString(id);
	while (!canRenameScene(index, name, nullptr)) {
		++id;
		name = "Scene" + Text::toString(id);
	}

	return name;
}

bool Project::sramExists(class Workspace* ws) const {
	const std::string path_ = sramPath(ws);

	return Path::fileExists(path_.c_str());
}

std::string Project::sramPath(class Workspace* ws) const {
	if (path().empty()) {
		const std::string name = title().empty() ? GBBASIC_NONAME_PROJECT_NAME : title();
		const std::string writableDir = Path::writableDirectory();
		const std::string path_ = Path::combine(writableDir.c_str(), name.c_str()) + "." GBBASIC_SRAM_STATE_EXT;

		return path_;
	}

	std::string name;
	std::string ext;
	std::string dir;
	Path::split(path(), &name, &ext, &dir);
	Text::toLowerCase(ext);

	if (ext == GBBASIC_SRAM_STATE_EXT) // The project has the same extension name with the SRAM file type.
		return ""; // Not saved in this case.

	const char* name_ = projectGetSramFileName();
	if (name_) {
		name = name_;
		projectFree((void*)name_);
	}

#if defined GBBASIC_OS_HTML
	const std::string writableDir = Path::writableDirectory();
	const std::string path_ = Path::combine(writableDir.c_str(), name.c_str()) + "." GBBASIC_SRAM_STATE_EXT;
#else /* GBBASIC_OS_HTML */
	std::string path_ = Path::combine(dir.c_str(), name.c_str()) + "." GBBASIC_SRAM_STATE_EXT;
	if (ws->isUnderExamplesDirectory(path_.c_str()) || ws->isUnderKitsDirectory(path_.c_str())) {
		const std::string writableDir = Path::writableDirectory();
		const std::string sramDir = Path::combine(writableDir.c_str(), "sram");
		if (!Path::directoryExists(sramDir.c_str()))
			Path::touchDirectory(sramDir.c_str());
		path_ = Path::combine(sramDir.c_str(), name.c_str()) + "." GBBASIC_SRAM_STATE_EXT;
	}
#endif /* GBBASIC_OS_HTML */

	return path_;
}

bool Project::dirty(void) {
	if (!assets())
		return false;

	if (path().empty())
		return true;

	if (hasDirtyInformation())
		return true;

	if (hasDirtyAsset())
		return true;

	do {
		if (!toPollEditor())
			break;

		hasDirtyEditor(false);
		toPollEditor(false);

		FontAssets &fonts = assets()->fonts;
		for (int i = 0; i < fonts.count(); ++i) {
			FontAssets::Entry* entry = fonts.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (minorCodeEditor() && minorCodeEditor()->hasUnsavedChanges()) {
			hasDirtyEditor(true);

			break;
		}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		CodeAssets &codes = assets()->code;
		for (int i = 0; i < codes.count(); ++i) {
			CodeAssets::Entry* entry = codes.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

		TilesAssets &tiles = assets()->tiles;
		for (int i = 0; i < tiles.count(); ++i) {
			TilesAssets::Entry* entry = tiles.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

		MapAssets &maps = assets()->maps;
		for (int i = 0; i < maps.count(); ++i) {
			MapAssets::Entry* entry = maps.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

		MusicAssets &music = assets()->music;
		for (int i = 0; i < music.count(); ++i) {
			MusicAssets::Entry* entry = music.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

		SfxAssets &sfx = assets()->sfx;
		for (int i = 0; i < sfx.count(); ++i) {
			SfxAssets::Entry* entry = sfx.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

		ActorAssets &actors = assets()->actors;
		for (int i = 0; i < actors.count(); ++i) {
			ActorAssets::Entry* entry = actors.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;

		SceneAssets &scenes = assets()->scenes;
		for (int i = 0; i < scenes.count(); ++i) {
			SceneAssets::Entry* entry = scenes.get(i);
			if (entry->editor && entry->editor->hasUnsavedChanges()) {
				hasDirtyEditor(true);

				break;
			}
		}
		if (hasDirtyEditor())
			break;
	} while (false);
	if (hasDirtyEditor())
		return true;

	return false;
}

bool Project::open(const char* path_) {
	// Prepare.
	path(path_);

	// Determine the file type.
	std::string ext;
	Path::split(path(), nullptr, &ext, nullptr);
	Text::toLowerCase(ext);

	const bool isBin = ext == GBBASIC_CLASSIC_ROM_EXT || ext == GBBASIC_COLORED_ROM_EXT || ext == "zip";
	const bool isZip = ext == "zip";

	// For binary (ROM).
	if (isBin) {
		std::string title_;
		Path::split(path(), &title_, nullptr, nullptr);
		title(title_);

		Bytes::Ptr bytes(Bytes::create());
		const bool ret = read(
			path().c_str(),
			[&] (File::Ptr file) -> bool {
				if (!file->readBytes(bytes.get()))
					return false;

				return true;
			}
		);
		if (!ret)
			return false;

		if (isZip) {
			Bytes::Ptr icon(Bytes::create());
			if (!extractRom(bytes.get(), icon.get()))
				return false;

			std::string txt;
			if (!Base64::fromBytes(txt, icon.get()))
				return false;

			iconCode(txt);
		}

		//kernel
		const long long now = DateTime::now();
		contentType(ContentTypes::ROM);
		RomInspector header;
		header.load(bytes);
		{
			std::string compCode = header.getCartridgeCompatibilityCode();
			compCode = "0x" + compCode;
			unsigned cartComp = 0;
			Text::Array cartTypes;
			if (Text::fromString(compCode, cartComp)) {
				if (cartComp == 0x00 || (cartComp & 0x80))
					cartTypes.push_back(PROJECT_CARTRIDGE_TYPE_CLASSIC);
				if ((cartComp & 0x80) || (cartComp & 0xc0))
					cartTypes.push_back(PROJECT_CARTRIDGE_TYPE_COLORED);
				if (cartComp & 0x20)
					cartTypes.push_back(PROJECT_CARTRIDGE_TYPE_EXTENSION);
			} else {
				cartTypes.push_back(PROJECT_CARTRIDGE_TYPE_CLASSIC);
				cartTypes.push_back(PROJECT_CARTRIDGE_TYPE_COLORED);
				cartTypes.push_back(PROJECT_CARTRIDGE_TYPE_EXTENSION);
			}
			cartridgeType(Text::join(cartTypes, PROJECT_CARTRIDGE_TYPE_SEPARATOR));

			std::string ramCode = header.getRamSizeCode();
			if (ramCode.length() > 1)
				ramCode = ramCode.substr(ramCode.length() - 1);
			sramType(ramCode);

			const std::string cartCode = header.getCartridgeTypeCode();
			hasRtc(cartCode == "0f" || cartCode == "10");
		}
		header.unload();
		description("");
		author("");
		genre("");
		version("1.0.0");
		url("");
		caseInsensitive(true);
		strictOn(true);
		superFeaturesEnabled(false);
		customizedSuperPalettes(false);
		created(now);
		modified(now);
		preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
		preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
		preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
		preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
		preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
		preferencesMapRef(0);
		preferencesMusicPreviewStroke(true);
		preferencesSfxShowSoundShape(true);
		preferencesActorApplyPropertiesToAllTiles(false);
		preferencesActorApplyPropertiesToAllFrames(true);
		preferencesActorUses8x16Sprites(true);
		preferencesSceneRefMap(0);
		preferencesSceneShowActors(true);
		preferencesSceneShowTriggers(true);
		preferencesSceneShowProperties(true);
		preferencesSceneShowAttributes(false);
		preferencesSceneUseGravity(false);

		isPlain(true);
		preferPlain(true);

		// Finish.
		return true;
	}

	// Read from the file.
	std::string source;
	const bool ret = read(
		path().c_str(),
		[&] (File::Ptr file) -> bool {
			if (!file->readString(source))
				return false;

			return true;
		}
	);
	if (!ret)
		return false;

	// Determine the source type.
	std::string content;
	const std::string sectionBegin = COMPILER_PROGRAM_BEGIN;
	const std::string sectionEnd = COMPILER_PROGRAM_END;
	const size_t beginIndex = Text::indexOf(source, sectionBegin);
	const size_t endIndex = Text::indexOf(source, sectionEnd, beginIndex + sectionBegin.length());
	if (beginIndex == std::string::npos && endIndex == std::string::npos) { // Is a plain source code file.
		// Fill the information.
		std::string title_;
		std::string ext;
		Path::split(path(), &title_, &ext, nullptr);
		title(title_);

		const long long now = DateTime::now();
		contentType(ContentTypes::BASIC);
		kernel("default");
		cartridgeType(PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION);
		sramType("0");
		hasRtc(false);
		description("");
		author("");
		genre("");
		version("1.0.0");
		url("");
		caseInsensitive(true);
		strictOn(true);
		superFeaturesEnabled(false);
		customizedSuperPalettes(false);
		created(now);
		modified(now);
		preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
		preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
		preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
		preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
		preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
		preferencesMapRef(0);
		preferencesMusicPreviewStroke(true);
		preferencesSfxShowSoundShape(true);
		preferencesActorApplyPropertiesToAllTiles(false);
		preferencesActorApplyPropertiesToAllFrames(true);
		preferencesActorUses8x16Sprites(true);
		preferencesSceneRefMap(0);
		preferencesSceneShowActors(true);
		preferencesSceneShowTriggers(true);
		preferencesSceneShowProperties(true);
		preferencesSceneShowAttributes(false);
		preferencesSceneUseGravity(false);

		isPlain(true);
		preferPlain(ext == GBBASIC_PLAIN_PROJECT_EXT);

		// Finish.
		return true;
	} else { // Is a rich project file.
		content = Text::trim(source);
		isPlain(false);
		preferPlain(false);
	}

	// Parse the information.
	const bool result = loadInformation(content, nullptr);

	// Finish.
	return result;
}

bool Project::close(bool deep) {
	// Destroy the editors.
	foreach(
		[] (AssetsBundle::Categories category, int index, BaseAssets::Entry* entry, Editable* editor) -> void {
			if (!editor)
				return;

			switch (category) {
			case AssetsBundle::Categories::PALETTE:
				// Do nothing.

				return;
			case AssetsBundle::Categories::FONT:
				editor->close(index);
				EditorFont::destroy((EditorFont*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::CODE:
				editor->close(index);
				EditorCode::destroy((EditorCode*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::TILES:
				editor->close(index);
				EditorTiles::destroy((EditorTiles*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::MAP:
				editor->close(index);
				EditorMap::destroy((EditorMap*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::MUSIC:
				editor->close(index);
				EditorMusic::destroy((EditorMusic*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::SFX:
				editor->close(index);
				EditorSfx::destroy((EditorSfx*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::ACTOR:
				editor->close(index);
				EditorActor::destroy((EditorActor*)editor);
				entry->editor = nullptr;

				break;
			case AssetsBundle::Categories::SCENE:
				editor->close(index);
				EditorScene::destroy((EditorScene*)editor);
				entry->editor = nullptr;

				break;
			default:
				GBBASIC_ASSERT(false && "Impossible.");

				break;
			}
		}
	);

	sharedSfxEditorDisabled(true);

	destroySharedSfxEditor();

	// Destroy the icon textures.
	_iconTextureIsBeingGenerated.wait();
	_iconTexture2BppDirtyIsBeingGenerated.wait();

	if (deep) {
		_iconTexture = nullptr;
		_iconTextureDirty = true;
		_iconTexture2Bpp = nullptr;
		_iconTexture2BppDirty = true;
	}

	// Destroy the assets.
	assets(nullptr);
	rom(nullptr);

	attributesTexture(nullptr);
	propertiesTexture(nullptr);
	actorsTexture(nullptr);

	behaviourSerializer(nullptr);
	behaviourParser(nullptr);

	// Mark as without modification.
	hasDirtyInformation(false);
	hasDirtyAsset(false);
	hasDirtyEditor(false);

	// Finish.
	return true;
}

bool Project::exists(void) const {
	if (path().empty())
		return false;

	if (!Path::fileExists(path().c_str()))
		return false;

	return true;
}

bool Project::loaded(void) const {
	return !!assets();
}

void Project::unload(void) {
	assets(nullptr);
	rom(nullptr);
}

bool Project::load(bool allowBin, const char* fontConfigPath, WarningOrErrorHandler onWarningOrError) {
	// Prepare.
	sharedSfxEditorDisabled(false);

	// Determine the file type.
	std::string ext;
	Path::split(path(), nullptr, &ext, nullptr);
	Text::toLowerCase(ext);

	const bool isBin = ext == GBBASIC_CLASSIC_ROM_EXT || ext == GBBASIC_COLORED_ROM_EXT || ext == "zip";

	// Load.
	if (isBin) {
		if (!allowBin)
			return false;

		return loadRom(onWarningOrError);
	}

	return loadBasic(fontConfigPath, onWarningOrError);
}

bool Project::save(const char* path_, bool redirect, WarningOrErrorHandler onWarningOrError) {
	// Only allow saving BASIC projects.
	if (contentType() != ContentTypes::BASIC) {
		GBBASIC_ASSERT(false && "Saving non-BASIC project is not supported.");

		if (onWarningOrError)
			onWarningOrError("Saving non-BASIC project is not supported.", true);

		return false;
	}

	// Save.
	return saveBasic(path_, redirect, onWarningOrError);
}

bool Project::rename(const char* name) {
	auto proc = [] (const std::string &self, const std::string &dir, const std::string &name, std::string ext) -> bool {
		const std::string oldName = self + "." + ext;
		const std::string oldPath = Path::combine(dir.c_str(), oldName.c_str());
		if (!Path::fileExists(oldPath.c_str()))
			return true;

		const std::string newName = name + "." + ext;
		const std::string newPath = Path::combine(dir.c_str(), newName.c_str());

		return Path::moveFile(oldPath.c_str(), newPath.c_str());
	};

	if (!exists())
		return false;

	std::string self;
	std::string ext;
	std::string dir;
	Path::split(path(), &self, &ext, &dir);

	int id = 0;
	std::string name_ = name;
	std::string newName = name_ + std::string(".") + ext;
	std::string newPath = Path::combine(dir.c_str(), newName.c_str());
	if (newPath == path())
		return true;

	while (Path::fileExists(newPath.c_str())) {
		++id;
		name_ = name + std::string(" (") + Text::toString(id) + ")";
		newName = name_ + std::string(".") + ext;
		newPath = Path::combine(dir.c_str(), newName.c_str());
	}

	if (!Path::moveFile(path().c_str(), newPath.c_str()))
		return false;

	proc(self, dir, name_, GBBASIC_CLASSIC_ROM_EXT);
	proc(self, dir, name_, GBBASIC_COLORED_ROM_EXT);
	proc(self, dir, name_, GBBASIC_SRAM_STATE_EXT);
	proc(self, dir, name_, GBBASIC_ROM_DUMP_EXT);

	path(newPath);

	return true;
}

void Project::foreach(AssetHandler handle) {
	if (!assets())
		return;

	PaletteAssets &palette = assets()->palette;
	for (int i = 0; i < palette.count(); ++i) {
		PaletteAssets::Entry* entry = palette.get(i);
		handle(AssetsBundle::Categories::PALETTE, i, entry, entry->editor);
	}

	FontAssets &fonts = assets()->fonts;
	for (int i = 0; i < fonts.count(); ++i) {
		FontAssets::Entry* entry = fonts.get(i);
		handle(AssetsBundle::Categories::FONT, i, entry, entry->editor);
	}

	CodeAssets &codes = assets()->code;
	for (int i = 0; i < codes.count(); ++i) {
		CodeAssets::Entry* entry = codes.get(i);
		handle(AssetsBundle::Categories::CODE, i, entry, entry->editor);
	}

	TilesAssets &tiles = assets()->tiles;
	for (int i = 0; i < tiles.count(); ++i) {
		TilesAssets::Entry* entry = tiles.get(i);
		handle(AssetsBundle::Categories::TILES, i, entry, entry->editor);
	}

	MapAssets &maps = assets()->maps;
	for (int i = 0; i < maps.count(); ++i) {
		MapAssets::Entry* entry = maps.get(i);
		handle(AssetsBundle::Categories::MAP, i, entry, entry->editor);
	}

	MusicAssets &music = assets()->music;
	for (int i = 0; i < music.count(); ++i) {
		MusicAssets::Entry* entry = music.get(i);
		handle(AssetsBundle::Categories::MUSIC, i, entry, entry->editor);
	}

	SfxAssets &sfx = assets()->sfx;
	for (int i = 0; i < sfx.count(); ++i) {
		SfxAssets::Entry* entry = sfx.get(i);
		handle(AssetsBundle::Categories::SFX, i, entry, entry->editor);
	}

	ActorAssets &actors = assets()->actors;
	for (int i = 0; i < actors.count(); ++i) {
		ActorAssets::Entry* entry = actors.get(i);
		handle(AssetsBundle::Categories::ACTOR, i, entry, entry->editor);
	}

	SceneAssets &scenes = assets()->scenes;
	for (int i = 0; i < scenes.count(); ++i) {
		SceneAssets::Entry* entry = scenes.get(i);
		handle(AssetsBundle::Categories::SCENE, i, entry, entry->editor);
	}
}

void Project::transfer(void) {
	FontAssets &fonts = assets()->fonts;
	for (int i = 0; i < fonts.count(); ++i) {
		FontAssets::Entry* entry = fonts.get(i);
		if (!entry->isAsset)
			continue;

		if (entry->copying == FontAssets::Entry::Copying::WAITING_FOR_COPYING) {
			std::string dir;
			Path::split(path(), nullptr, nullptr, &dir);

			std::string name;
			std::string ext;
			Path::split(entry->path, &name, &ext, nullptr);
			const std::string relPath = name + "." + ext;
			const std::string newPath = Path::combine(dir.c_str(), relPath.c_str());
			const std::string absPath0 = Path::absoluteOf(entry->path);
			const std::string absPath1 = Path::absoluteOf(newPath);
			if (Path::equals(absPath0.c_str(), absPath1.c_str()) || Path::copyFile(entry->path.c_str(), newPath.c_str())) {
				entry->copying = FontAssets::Entry::Copying::COPIED;
				entry->directory = dir;
				entry->path = relPath; // Modify to use relative path.
			} else {
				entry->copying = FontAssets::Entry::Copying::REFERENCED;
				entry->directory = dir;
			}
		}
	}
}

bool Project::extractRom(Bytes* data, Bytes* icon) const {
	// Prepare.
	Archive::Ptr arc(Archive::create(Archive::ZIP));
	if (!arc->open(path().c_str(), Stream::Accesses::READ))
		return false;

	// Read all entries in the archive.
	Text::Array entries = arc->find(
		[] (const char* name) -> bool {
			return Text::endsWith(name, "." GBBASIC_CLASSIC_ROM_EXT, true) || Text::endsWith(name, "." GBBASIC_COLORED_ROM_EXT, true);
		}
	);
	if (entries.empty()) {
		arc->close();

		return false;
	}

	// Parse ROM data.
	if (data) {
		if (!arc->toBytes(data, entries.front().c_str())) {
			arc->close();

			return false;
		}
	}

	// Parse icon.
	entries = arc->find(
		[] (const char* name) -> bool {
			return
				Text::endsWith(name, ".png", true) ||
				Text::endsWith(name, ".jpg", true) ||
				Text::endsWith(name, ".bmp", true) ||
				Text::endsWith(name, ".tga", true);
		}
	);
	do {
		if (entries.empty())
			break;

		if (!icon)
			break;

		if (!arc->toBytes(icon, entries.front().c_str()))
			break;
	} while (false);

	// Finish.
	arc->close();

	return true;
}

bool Project::loadRom(WarningOrErrorHandler onWarningOrError) {
	std::string ext;
	Path::split(path(), nullptr, &ext, nullptr);
	Text::toLowerCase(ext);

	const bool isZip = ext == "zip";

	Bytes::Ptr bytes(Bytes::create());
	const bool ret = read(
		path().c_str(),
		[&] (File::Ptr file) -> bool {
			if (!file->readBytes(bytes.get()))
				return false;

			return true;
		}
	);
	if (!ret)
		return false;

	if (isZip) {
		if (!extractRom(bytes.get(), nullptr))
			return false;
	}

	if (!rom())
		rom(Bytes::Ptr(Bytes::create()));
	rom()->writeBytes(bytes.get());

	return true;
}

bool Project::loadBasic(const char* fontConfigPath, WarningOrErrorHandler onWarningOrError) {
	// Read from the file.
	std::string source;
	const bool ret = read(
		path().c_str(),
		[&] (File::Ptr file) -> bool {
			if (!file->readString(source))
				return false;

			return true;
		}
	);
	if (!ret)
		return false;

	if (!Unicode::isPrintable(source.c_str(), source.length())) {
		fprintf(stderr, "Invalid file.\n");

		return false;
	}

	// Determine the source type.
	std::string content;
	const std::string sectionBegin = COMPILER_PROGRAM_BEGIN;
	const std::string sectionEnd = COMPILER_PROGRAM_END;
	const size_t beginIndex = Text::indexOf(source, sectionBegin);
	const size_t endIndex = Text::indexOf(source, sectionEnd, beginIndex + sectionBegin.length());
	if (beginIndex == std::string::npos && endIndex == std::string::npos) { // Is a plain source code file.
		// Fill the information.
		std::string title_;
		std::string ext;
		Path::split(path(), &title_, &ext, nullptr);
		title(title_);

		const long long now = DateTime::now();
		contentType(ContentTypes::BASIC);
		kernel("default");
		cartridgeType(PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION);
		sramType("0");
		hasRtc(false);
		description("");
		author("");
		genre("");
		version("1.0.0");
		url("");
		caseInsensitive(true);
		strictOn(true);
		superFeaturesEnabled(false);
		customizedSuperPalettes(false);
		created(now);
		modified(now);
		preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
		preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
		preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
		preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
		preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
		preferencesPreviewAnchorPoint(true);
		preferencesPreviewPaletteBits(true);
		preferencesUseByteMatrix(false);
		preferencesShowGrids(true);
		preferencesCodePageForBindedRoutine(0);
		preferencesCodeLineForBindedRoutine(Left<int>(0));
		preferencesMapRef(0);
		preferencesMusicPreviewStroke(true);
		preferencesSfxShowSoundShape(true);
		preferencesActorApplyPropertiesToAllTiles(false);
		preferencesActorApplyPropertiesToAllFrames(true);
		preferencesActorUses8x16Sprites(true);
		preferencesSceneRefMap(0);
		preferencesSceneShowActors(true);
		preferencesSceneShowTriggers(true);
		preferencesSceneShowProperties(true);
		preferencesSceneShowAttributes(false);
		preferencesSceneUseGravity(false);

		// Fill the assets.
		assets(AssetsBundle::Ptr(new AssetsBundle()));

		touchPalette();

		clearFontPages();
		addGlobalFontPages(fontConfigPath, onWarningOrError);
		activeFontIndex(0);

		CodeAssets code;
		code.add(source);
		assets()->code = code;
		isPlain(true);
		preferPlain(ext == GBBASIC_PLAIN_PROJECT_EXT);
		activeMajorCodeIndex(0);
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		isMajorCodeEditorActive(true);
		isMinorCodeEditorEnabled(false);
		activeMinorCodeIndex(-1);
		minorCodeEditorWidth(0.0f);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		activePaletteIndex(-1);

		activeFontIndex(-1);

		fontPreviewHeight(0.0f);

		activeTilesIndex(-1);

		activeMapIndex(-1);

		activeMusicIndex(-1);

		activeSfxIndex(-1);

		activeActorIndex(-1);

		activeSceneIndex(-1);

		// Finish.
		return true;
	} else { // Is a rich project file.
		content = Text::trim(source);
		isPlain(false);
		preferPlain(false);
	}

	// Parse the information.
	loadInformation(content, onWarningOrError);

	// Parse the assets.
	loadAssets(fontConfigPath, content, onWarningOrError);

	// Finish.
	return true;
}

bool Project::saveBasic(const char* path_, bool redirect, WarningOrErrorHandler onWarningOrError) {
	// Prepare.
	if (path().empty() || redirect)
		path(path_);

	// Determine the source type.
	const bool saveAsPlain = isPlain() && preferPlain();
	if (saveAsPlain) { // Save as plain source code file.
		// Prepare.
		std::string source;

		// Check the assets.
		if (assets()->fonts.count() > 1) {
			if (onWarningOrError)
				onWarningOrError("Ignored font assets after page 1.", true);
		}
		if (assets()->code.count() > 1) {
			if (onWarningOrError)
				onWarningOrError("Ignored code after page 1.", true);
		}
		if (assets()->tiles.count() > 0) {
			if (onWarningOrError)
				onWarningOrError("Ignored tiles assets.", true);
		}
		if (assets()->maps.count() > 0) {
			if (onWarningOrError)
				onWarningOrError("Ignored map assets.", true);
		}
		if (assets()->music.count() > 0) {
			if (onWarningOrError)
				onWarningOrError("Ignored music assets.", true);
		}
		if (assets()->sfx.count() > 0) {
			if (onWarningOrError)
				onWarningOrError("Ignored SFX assets.", true);
		}
		if (assets()->actors.count() > 0) {
			if (onWarningOrError)
				onWarningOrError("Ignored actor assets.", true);
		}
		if (assets()->scenes.count() > 0) {
			if (onWarningOrError)
				onWarningOrError("Ignored scene assets.", true);
		}

		// Serialize the code.
		CodeAssets::Entry* entry = assets()->code.get(0);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		if (!entry->toString(source, onWarningOrError))
			return false;

		// Write to the file.
		const bool ret = write(
			path().c_str(),
			[&] (File::Ptr file) -> bool {
				if (source.empty())
					return true;

				if (!file->writeString(source))
					return false;

				return true;
			}
		);
		if (!ret)
			return false;

		// Finish.
		hasDirtyAsset(false);
		hasDirtyEditor(false);
		toPollEditor(false);

		return true;
	} else { // Save as rich project file.
		// Prepare.
		std::string source;
		std::string content;

		// Transfer external stuff that is pending.
		transfer();

		// Update the modification timestamp.
		const long long now = DateTime::now();
		modified(now);

		// Serialize the information.
		saveInformation(content);

		// Serialize the assets.
		if (!saveAssets(content, onWarningOrError))
			return false;

		// Write to the file.
		content = Text::trim(content);
		put(source, COMPILER_PROGRAM_BEGIN, COMPILER_PROGRAM_END, content);

		const bool ret = write(
			path().c_str(),
			[&] (File::Ptr file) -> bool {
				if (!file->writeString(source))
					return false;

				return true;
			}
		);
		if (!ret)
			return false;

		// Finish.
		return true;
	}
}

bool Project::loadInformation(const std::string &content, WarningOrErrorHandler onWarningOrError) {
	if (contentType() != ContentTypes::BASIC) {
		GBBASIC_ASSERT(false && "Loading non-BASIC assets is not supported.");

		return false;
	}

	auto report = [onWarningOrError] (const char* msg, bool isWarning) -> void {
		if (onWarningOrError)
			onWarningOrError(msg, isWarning);
	};

	std::string section;
	if (!retrieve(content, COMPILER_INFO_BEGIN, COMPILER_INFO_END, section)) {
		report("The project has no <info /> section.", false);

		return false;
	}

	rapidjson::Document doc;
	if (!Json::fromString(doc, section.c_str())) {
		report("The project's <info /> section is invalid.", false);

		return false;
	}

	std::string txt;
	if (Jpath::get(doc, txt, "title")) {
		title(txt);
	} else {
		title("...");

		report("The project has no <title /> section.", true);
	}

	kernel().clear();
	if (!Jpath::get(doc, txt, "kernel")) {
		txt = "default";
	}
	kernel(txt);

	cartridgeType().clear();
	if (!Jpath::get(doc, txt, "cartridge_type")) {
		txt = PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED;

		const std::string msg = Text::format("The project information has no \"cartridge_type\" field, falls to \"{0}\".", { txt });
		report(msg.c_str(), true);
	}
	if (
		txt != PROJECT_CARTRIDGE_TYPE_CLASSIC &&
		txt != PROJECT_CARTRIDGE_TYPE_COLORED &&
		txt != PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED &&
		txt != PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION &&
		txt != PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION &&
		txt != PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_EXTENSION
	) {
		txt = PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED;

		const std::string msg = Text::format("The project information's \"cartridge_type\" field is invalid, falls to \"{0}\".", { txt });
		report(msg.c_str(), true);
	}
	cartridgeType(txt);

	sramType().clear();
	if (!Jpath::get(doc, txt, "sram_type")) {
		txt = PROJECT_SRAM_TYPE_32KB;

		const std::string msg = Text::format("The project information has no \"sram_type\" field, falls to \"{0}\".", { txt });
		report(msg.c_str(), true);
	}
	int tmp = 0;
	if (Text::fromString(txt, tmp)) {
		if (tmp < 0x00 || tmp > 0x04) {
			txt = PROJECT_SRAM_TYPE_32KB;

			const std::string msg = Text::format("The project information's \"sram_type\" field is invalid, falls to \"{0}\".", { txt });
			report(msg.c_str(), true);
		}
	} else {
		txt = PROJECT_SRAM_TYPE_32KB;

		const std::string msg = Text::format("The project information's \"sram_type\" field is invalid, falls to \"{0}\".", { txt });
		report(msg.c_str(), true);
	}
	sramType(txt);

	hasRtc(false);
	if (!Jpath::get(doc, hasRtc(), "has_rtc")) {
		hasRtc(false);

		const std::string msg = Text::format("The project information has no \"has_rtc\" field, falls to \"{0}\".", { Text::toString(hasRtc()) });
		report(msg.c_str(), true);
	}

	description().clear();
	if (!Jpath::get(doc, txt, "description")) {
		txt.clear();
	}
	description(txt);

	author().clear();
	if (!Jpath::get(doc, txt, "author")) {
		txt.clear();
	}
	author(txt);

	genre().clear();
	if (!Jpath::get(doc, txt, "genre")) {
		txt.clear();
	}
	genre(txt);

	version().clear();
	if (!Jpath::get(doc, txt, "version")) {
		txt.clear();
	}
	version(txt);

	url().clear();
	if (!Jpath::get(doc, txt, "url")) {
		txt.clear();
	}
	url(txt);

	do {
		if (!Jpath::get(doc, txt, "icon")) {
			iconCode("");

			break;
		}

		if (txt.empty()) {
			iconCode("");

			break;
		}

		Bytes::Ptr bytes(Bytes::create());
		if (!Base64::toBytes(bytes.get(), txt)) {
			report("The project information's \"icon\" field is invalid.", true);

			iconCode("");

			break;
		}

		Texture::Ptr texture(Texture::create());
		Image::Ptr image(Image::create());
		if (!image->fromBytes(bytes->pointer(), bytes->count())) {
			report("The project information's \"icon\" field is invalid.", true);

			iconCode("");

			break;
		}

		if (!texture->fromImage(renderer(), Texture::STATIC, image.get(), Texture::NEAREST)) {
			report("The project information's \"icon\" field is invalid.", true);

			iconCode("");

			break;
		}

		iconCode(txt);
	} while (false);

	caseInsensitive(true);
	if (!Jpath::get(doc, caseInsensitive(), "case_insensitive")) {
		caseInsensitive(true);
	}

	strictOn(true);
	if (!Jpath::get(doc, strictOn(), "strict_on")) {
		strictOn(true);
	}

	superFeaturesEnabled(false);
	if (!Jpath::get(doc, superFeaturesEnabled(), "super_features", "enabled")) {
		superFeaturesEnabled(false);
	}

	borderFrameType(BorderFrameTypes::NONE);
	unsigned utmp = 0;
	if (Jpath::get(doc, utmp, "super_features", "border_frame_type")) {
		utmp = Math::clamp(utmp, (unsigned)BorderFrameTypes::NONE, (unsigned)BorderFrameTypes::COUNT - 1);
		borderFrameType((BorderFrameTypes)utmp);
	} else {
		borderFrameType(BorderFrameTypes::NONE);
	}

	borderFrameCode("");
	if (!Jpath::get(doc, txt, "super_features", "border_frame")) {
		txt.clear();
	}
	borderFrameCode(txt);

	const Colour DEFAULT_COLORS_ARRAY[] = INDEXED_DEFAULT_COLORS_FOR_SUPER_PALETTES;
	superPalettes(DEFAULT_COLORS_ARRAY);
	bool ok = true;
	SuperPalettes cols = superPalettes();
	for (int i = 0; i < (int)superPalettes().size(); ++i) {
		UInt32 rgba = 0;
		if (!Jpath::get(doc, rgba, "super_features", "colors", "data", i)) {
			ok = false;

			break;
		}
		cols[i].fromRGBA(rgba);
	}
	if (ok)
		superPalettes(cols);

	customizedSuperPalettes(false);
	if (!Jpath::get(doc, customizedSuperPalettes(), "super_features", "colors", "customized")) {
		customizedSuperPalettes(false);
	}

	const long long now = DateTime::now();
	long long timestamp = 0;
	if (Jpath::get(doc, timestamp, "created")) {
		if (timestamp == 0)
			created(now);
		else
			created(timestamp);
	} else {
		created(now);

		report("The project information's \"created\" field is invalid.", true);
	}

	if (Jpath::get(doc, timestamp, "modified")) {
		if (timestamp == 0)
			modified(now);
		else
			modified(timestamp);
	} else {
		modified(now);

		report("The project information's \"modified\" field is invalid.", true);
	}

	order(PROJECT_DEFAULT_ORDER);
	if (!Jpath::get(doc, order(), "order")) {
		order(PROJECT_DEFAULT_ORDER);
	}

	preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
	int x = 0, y = 0;
	if (Jpath::get(doc, x, "preference", "font_size", 0) && Jpath::get(doc, y, "preference", "font_size", 1)) {
		preferencesFontSize(Math::Vec2i(x, y));
	} else {
		preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
	}

	preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
	if (!Jpath::get(doc, preferencesFontOffset(), "preference", "font_offset")) {
		preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
	}

	preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
	if (!Jpath::get(doc, preferencesFontIsTwoBitsPerPixel(), "preference", "font_is_two_bits_per_pixel")) {
		preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
	}

	preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
	if (!Jpath::get(doc, preferencesFontPreferFullWord(), "preference", "font_prefer_full_word")) {
		preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
	}

	preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
	if (!Jpath::get(doc, preferencesFontPreferFullWordForNonAscii(), "preference", "font_prefer_full_word_for_non_ascii")) {
		preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
	}

	preferencesPreviewAnchorPoint(true);
	if (!Jpath::get(doc, preferencesPreviewAnchorPoint(), "preference", "preview_anchor_point"))
		preferencesPreviewAnchorPoint(true);

	preferencesPreviewPaletteBits(true);
	if (!Jpath::get(doc, preferencesPreviewPaletteBits(), "preference", "preview_palette_bits"))
		preferencesPreviewPaletteBits(true);

	preferencesUseByteMatrix(false);
	if (!Jpath::get(doc, preferencesUseByteMatrix(), "preference", "use_byte_matrix"))
		preferencesUseByteMatrix(false);

	preferencesShowGrids(true);
	if (!Jpath::get(doc, preferencesShowGrids(), "preference", "show_grids"))
		preferencesShowGrids(true);

	preferencesCodePageForBindedRoutine(0);
	if (!Jpath::get(doc, preferencesCodePageForBindedRoutine(), "preference", "code_page_for_binded_routine"))
		preferencesCodePageForBindedRoutine(0);

	preferencesCodeLineForBindedRoutine(Left<int>(0));
	switch (Jpath::typeOf(doc, "preference", "code_line_for_binded_routine")) {
	case Jpath::Types::NUMBER: {
			int val = 0;
			if (Jpath::get(doc, val, "preference", "code_line_for_binded_routine"))
				preferencesCodeLineForBindedRoutine(Left<int>(val));
		}

		break;
	case Jpath::Types::STRING: {
			std::string val;
			if (Jpath::get(doc, val, "preference", "code_line_for_binded_routine"))
				preferencesCodeLineForBindedRoutine(Right<std::string>(val));
		}

		break;
	default:
		preferencesCodeLineForBindedRoutine(Left<int>(0));

		break;
	}

	preferencesMapRef(0);
	if (!Jpath::get(doc, preferencesMapRef(), "preference", "map_ref")) {
		preferencesMapRef(0);
	}

	preferencesMusicPreviewStroke(true);
	if (!Jpath::get(doc, preferencesMusicPreviewStroke(), "preference", "music_preview_stroke")) {
		preferencesMusicPreviewStroke(true);
	}

	preferencesSfxShowSoundShape(true);
	if (!Jpath::get(doc, preferencesSfxShowSoundShape(), "preference", "sfx_show_sound_shape")) {
		preferencesSfxShowSoundShape(true);
	}

	preferencesActorApplyPropertiesToAllTiles(false);
	if (!Jpath::get(doc, preferencesActorApplyPropertiesToAllTiles(), "preference", "actor_apply_properties_to_all_tiles")) {
		preferencesActorApplyPropertiesToAllTiles(false);
	}

	preferencesActorApplyPropertiesToAllFrames(true);
	if (!Jpath::get(doc, preferencesActorApplyPropertiesToAllFrames(), "preference", "actor_apply_properties_to_all_frames")) {
		preferencesActorApplyPropertiesToAllFrames(true);
	}

	preferencesActorUses8x16Sprites(true);
	if (!Jpath::get(doc, preferencesActorUses8x16Sprites(), "preference", "actor_uses_8x16_sprites")) {
		preferencesActorUses8x16Sprites(true);
	}

	preferencesSceneRefMap(0);
	if (!Jpath::get(doc, preferencesSceneRefMap(), "preference", "scene_ref_map")) {
		preferencesSceneRefMap(0);
	}

	preferencesSceneShowActors(true);
	if (!Jpath::get(doc, preferencesSceneShowActors(), "preference", "scene_show_actors")) {
		preferencesSceneShowActors(true);
	}

	preferencesSceneShowTriggers(true);
	if (!Jpath::get(doc, preferencesSceneShowTriggers(), "preference", "scene_show_triggers")) {
		preferencesSceneShowTriggers(true);
	}

	preferencesSceneShowProperties(true);
	if (!Jpath::get(doc, preferencesSceneShowProperties(), "preference", "scene_show_properties")) {
		preferencesSceneShowProperties(true);
	}

	preferencesSceneShowAttributes(false);
	if (!Jpath::get(doc, preferencesSceneShowAttributes(), "preference", "scene_show_attributes")) {
		preferencesSceneShowAttributes(false);
	}

	preferencesSceneUseGravity(false);
	if (!Jpath::get(doc, preferencesSceneUseGravity(), "preference", "scene_use_gravity")) {
		preferencesSceneUseGravity(false);
	}

	unsigned engineCode = 0;
	do {
		section.clear();
		if (!retrieve(content, COMPILER_ENGINE_BEGIN, COMPILER_ENGINE_END, section)) {
			break;
		}

		if (!Text::fromString(section, engineCode)) {
			engineCode = 0;
		}
	} while (false);
	engine(engineCode);

	return true;
}

bool Project::saveInformation(std::string &content) {
	if (contentType() != ContentTypes::BASIC) {
		GBBASIC_ASSERT(false && "Saving non-BASIC project is not supported.");

		return false;
	}

	rapidjson::Document doc;
	doc.SetObject();
	std::string txt;

	Jpath::set(doc, doc, title(), "title");

	Jpath::set(doc, doc, kernel(), "kernel");

	Jpath::set(doc, doc, cartridgeType(), "cartridge_type");

	Jpath::set(doc, doc, sramType(), "sram_type");

	Jpath::set(doc, doc, hasRtc(), "has_rtc");

	Jpath::set(doc, doc, description(), "description");

	Jpath::set(doc, doc, author(), "author");

	Jpath::set(doc, doc, genre(), "genre");

	Jpath::set(doc, doc, version(), "version");

	Jpath::set(doc, doc, url(), "url");

	Jpath::set(doc, doc, iconCode(), "icon");

	Jpath::set(doc, doc, caseInsensitive(), "case_insensitive");

	Jpath::set(doc, doc, strictOn(), "strict_on");

	Jpath::set(doc, doc, superFeaturesEnabled(), "super_features", "enabled");

	Jpath::set(doc, doc, (unsigned)borderFrameType(), "super_features", "border_frame_type");

	Jpath::set(doc, doc, borderFrameCode(), "super_features", "border_frame");

	for (int i = 0; i < (int)superPalettes().size(); ++i) {
		const UInt32 rgba = superPalettes()[i].toRGBA();
		Jpath::set(doc, doc, rgba, "super_features", "colors", "data", i);
	}

	Jpath::set(doc, doc, customizedSuperPalettes(), "super_features", "colors", "customized");

	Jpath::set(doc, doc, created(), "created");

	Jpath::set(doc, doc, modified(), "modified");

	if (order() != PROJECT_DEFAULT_ORDER)
		Jpath::set(doc, doc, order(), "order");

	Jpath::set(doc, doc, preferencesFontSize().x, "preference", "font_size", 0);
	Jpath::set(doc, doc, preferencesFontSize().y, "preference", "font_size", 1);

	Jpath::set(doc, doc, preferencesFontOffset(), "preference", "font_offset");

	Jpath::set(doc, doc, preferencesFontIsTwoBitsPerPixel(), "preference", "font_is_two_bits_per_pixel");

	Jpath::set(doc, doc, preferencesFontPreferFullWord(), "preference", "font_prefer_full_word");

	Jpath::set(doc, doc, preferencesFontPreferFullWordForNonAscii(), "preference", "font_prefer_full_word_for_non_ascii");

	Jpath::set(doc, doc, preferencesPreviewAnchorPoint(), "preference", "preview_anchor_point");

	Jpath::set(doc, doc, preferencesPreviewPaletteBits(), "preference", "preview_palette_bits");

	Jpath::set(doc, doc, preferencesUseByteMatrix(), "preference", "use_byte_matrix");

	Jpath::set(doc, doc, preferencesShowGrids(), "preference", "show_grids");

	Jpath::set(doc, doc, preferencesCodePageForBindedRoutine(), "preference", "code_page_for_binded_routine");

	if (preferencesCodeLineForBindedRoutine().isLeft())
		Jpath::set(doc, doc, preferencesCodeLineForBindedRoutine().left().get(), "preference", "code_line_for_binded_routine");
	else
		Jpath::set(doc, doc, preferencesCodeLineForBindedRoutine().right().get(), "preference", "code_line_for_binded_routine");

	Jpath::set(doc, doc, preferencesMapRef(), "preference", "map_ref");

	Jpath::set(doc, doc, preferencesMusicPreviewStroke(), "preference", "music_preview_stroke");

	Jpath::set(doc, doc, preferencesSfxShowSoundShape(), "preference", "sfx_show_sound_shape");

	Jpath::set(doc, doc, preferencesActorApplyPropertiesToAllTiles(), "preference", "actor_apply_properties_to_all_tiles");

	Jpath::set(doc, doc, preferencesActorApplyPropertiesToAllFrames(), "preference", "actor_apply_properties_to_all_frames");

	Jpath::set(doc, doc, preferencesActorUses8x16Sprites(), "preference", "actor_uses_8x16_sprites");

	Jpath::set(doc, doc, preferencesSceneRefMap(), "preference", "scene_ref_map");

	Jpath::set(doc, doc, preferencesSceneShowActors(), "preference", "scene_show_actors");

	Jpath::set(doc, doc, preferencesSceneShowTriggers(), "preference", "scene_show_triggers");

	Jpath::set(doc, doc, preferencesSceneShowProperties(), "preference", "scene_show_properties");

	Jpath::set(doc, doc, preferencesSceneShowAttributes(), "preference", "scene_show_attributes");

	Jpath::set(doc, doc, preferencesSceneUseGravity(), "preference", "scene_use_gravity");

	if (!Json::toString(doc, txt))
		return false;

	put(content, COMPILER_INFO_BEGIN, COMPILER_INFO_END, txt);

	if (engine() == 0) // Overwrite it if it's zero.
		engine(GBBASIC_ENGINE_VERSION);
	txt = Text::toString(engine());
	put(content, COMPILER_ENGINE_BEGIN, COMPILER_ENGINE_END, txt);

	hasDirtyInformation(false);

	return true;
}

bool Project::loadAssets(const char* fontConfigPath, const std::string &content, WarningOrErrorHandler onWarningOrError) {
	// Only allow saving BASIC projects.
	if (contentType() != ContentTypes::BASIC) {
		GBBASIC_ASSERT(false && "Loading non-BASIC assets is not supported.");

		return false;
	}

	// Prepare.
	assets(AssetsBundle::Ptr(new AssetsBundle()));

	// Load palette.
	do {
		std::string section;
		if (!retrieve(content, COMPILER_PALETTE_BEGIN, COMPILER_PALETTE_END, section))
			break;
		section = Text::trim(section);

		PaletteAssets palette;
		if (!palette.fromString(section, onWarningOrError))
			break;

		touchPalette().copyFrom(palette, nullptr);

		synchronizeSuperPalettes(palette);
	} while (false);

	if (palettePageCount() > 0)
		activePaletteIndex(Math::clamp(activePaletteIndex(), 0, palettePageCount() - 1));

	// Load font.
	clearFontPages();
	addGlobalFontPages(fontConfigPath, onWarningOrError);
	do {
		// Load from the program.
		std::string dir;
		Path::split(path(), nullptr, nullptr, &dir);

		std::string section;
		if (!retrieve(content, COMPILER_FONT_BEGIN, COMPILER_FONT_END, section))
			break;
		section = Text::trim(section);

		FontAssets fonts;
		if (!fonts.fromString(section, dir, true, onWarningOrError))
			break;

		for (int i = 0; i < fonts.count(); ++i) {
			const FontAssets::Entry* entry = fonts.get(i);
			if (!entry)
				continue;

			assets()->fonts.add(*entry);
		}
	} while (false);

	if (fontPageCount() > 0)
		activeFontIndex(Math::clamp(activeFontIndex(), 0, fontPageCount() - 1));

	// Load code.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_CODE_BEGIN, Text::toString(i), 0), COMPILER_CODE_END,
				section
			)
		) {
			break;
		}
		if (!section.empty() && section.front() == '\n')
			section.erase(section.begin());
		if (!section.empty() && section.back() == '\n')
			section.pop_back();

		addCodePage(section);
	}

	if (codePageCount() > 0)
		activeMajorCodeIndex(Math::clamp(activeMajorCodeIndex(), 0, codePageCount() - 1));
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	if (activeMinorCodeIndex() != -1 && codePageCount() > 0)
		activeMinorCodeIndex(Math::clamp(activeMinorCodeIndex(), 0, codePageCount() - 1));
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	// Load tiles.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_TILES_BEGIN, Text::toString(i), 0), COMPILER_TILES_END,
				section
			)
		) {
			break;
		}
		section = Text::trim(section);

		addTilesPage(section, false);
	}

	if (tilesPageCount() > 0)
		activeTilesIndex(Math::clamp(activeTilesIndex(), 0, tilesPageCount() - 1));

	// Load map.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_MAP_BEGIN, Text::toString(i), 0), COMPILER_MAP_END,
				section
			)
		) {
			break;
		}
		section = Text::trim(section);

		addMapPage(section, false, nullptr);
	}

	if (mapPageCount() > 0)
		activeMapIndex(Math::clamp(activeMapIndex(), 0, mapPageCount() - 1));

	// Load music.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_MUSIC_BEGIN, Text::toString(i), 0), COMPILER_MUSIC_END,
				section
			)
		) {
			break;
		}
		section = Text::trim(section);

		addMusicPage(section, false);
	}

	if (musicPageCount() > 0)
		activeMusicIndex(Math::clamp(activeMusicIndex(), 0, musicPageCount() - 1));

	// Load SFX.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_SFX_BEGIN, Text::toString(i), 0), COMPILER_SFX_END,
				section
			)
		) {
			break;
		}
		section = Text::trim(section);

		addSfxPage(section, false, false, false);
	}

	if (sfxPageCount() > 0)
		activeSfxIndex(Math::clamp(activeSfxIndex(), 0, sfxPageCount() - 1));

	// Load actor.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_ACTOR_BEGIN, Text::toString(i), 0), COMPILER_ACTOR_END,
				section
			)
		) {
			break;
		}
		section = Text::trim(section);

		addActorPage(section, false);
	}

	if (actorPageCount() > 0)
		activeActorIndex(Math::clamp(activeActorIndex(), 0, actorPageCount() - 1));

	// Load scene.
	for (int i = 0; ; ++i) {
		std::string section;
		if (
			!retrieve(
				content,
				Text::format(COMPILER_SCENE_BEGIN, Text::toString(i), 0), COMPILER_SCENE_END,
				section
			)
		) {
			break;
		}
		section = Text::trim(section);

		addScenePage(section, false, nullptr);
	}

	if (scenePageCount() > 0)
		activeSceneIndex(Math::clamp(activeSceneIndex(), 0, scenePageCount() - 1));

	// Finish.
	return true;
}

bool Project::saveAssets(std::string &content, WarningOrErrorHandler onWarningOrError) {
	// Only allow saving BASIC projects.
	if (contentType() != ContentTypes::BASIC) {
		GBBASIC_ASSERT(false && "Saving non-BASIC project is not supported.");

		return false;
	}

	// Save palette.
	do {
		std::string txt_;

		PaletteAssets &palette = touchPalette();
		if (!palette.toString(txt_, onWarningOrError))
			break;

		put(content, COMPILER_PALETTE_BEGIN, COMPILER_PALETTE_END, txt_);
	} while (false);

	// Save font.
	do {
		const FontAssets &fonts = assets()->fonts;
		if (fonts.empty()) {
			std::string txt_;
			FontAssets fonts;
			if (!fonts.toString(txt_, onWarningOrError))
				break;

			put(content, COMPILER_FONT_BEGIN, COMPILER_FONT_END, txt_);
		} else {
			for (int i = 0; i < fonts.count(); ++i) {
				const FontAssets::Entry* entry = fonts.get(i);
				if (entry->editor && entry->editor->hasUnsavedChanges()) {
					entry->editor->flush();
					entry->editor->markChangesSaved();
				}
			}
			std::string txt_;
			if (!fonts.toString(txt_, onWarningOrError))
				break;

			put(content, COMPILER_FONT_BEGIN, COMPILER_FONT_END, txt_);
		}
	} while (false);

	// Save code.
	for (int i = 0; i < assets()->code.count(); ++i) {
		CodeAssets::Entry* entry = assets()->code.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (isMinorCodeEditorEnabled() && activeMinorCodeIndex() == i) {
			if (minorCodeEditor() && minorCodeEditor()->hasUnsavedChanges()) {
				minorCodeEditor()->flush();
				minorCodeEditor()->markChangesSaved();
			}
		}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			return false; // Critical error.

		put(content, Text::format(COMPILER_CODE_BEGIN, Text::toString(i), 0), COMPILER_CODE_END, txt_);
	}

	// Save tiles.
	for (int i = 0; i < assets()->tiles.count(); ++i) {
		TilesAssets::Entry* entry = assets()->tiles.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			continue;

		put(content, Text::format(COMPILER_TILES_BEGIN, Text::toString(i), 0), COMPILER_TILES_END, txt_);
	}

	// Save maps.
	for (int i = 0; i < assets()->maps.count(); ++i) {
		MapAssets::Entry* entry = assets()->maps.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			continue;

		put(content, Text::format(COMPILER_MAP_BEGIN, Text::toString(i), 0), COMPILER_MAP_END, txt_);
	}

	// Save music.
	for (int i = 0; i < assets()->music.count(); ++i) {
		MusicAssets::Entry* entry = assets()->music.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			continue;

		put(content, Text::format(COMPILER_MUSIC_BEGIN, Text::toString(i), 0), COMPILER_MUSIC_END, txt_);
	}

	// Save SFX.
	for (int i = 0; i < assets()->sfx.count(); ++i) {
		SfxAssets::Entry* entry = assets()->sfx.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			continue;

		put(content, Text::format(COMPILER_SFX_BEGIN, Text::toString(i), 0), COMPILER_SFX_END, txt_);
	}


	// Save actors.
	for (int i = 0; i < assets()->actors.count(); ++i) {
		ActorAssets::Entry* entry = assets()->actors.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			continue;

		put(content, Text::format(COMPILER_ACTOR_BEGIN, Text::toString(i), 0), COMPILER_ACTOR_END, txt_);
	}

	// Save scenes.
	for (int i = 0; i < assets()->scenes.count(); ++i) {
		SceneAssets::Entry* entry = assets()->scenes.get(i);
		if (entry->editor && entry->editor->hasUnsavedChanges()) {
			entry->editor->flush();
			entry->editor->markChangesSaved();
		}
		std::string txt_;
		if (!entry->toString(txt_, onWarningOrError))
			continue;

		put(content, Text::format(COMPILER_SCENE_BEGIN, Text::toString(i), 0), COMPILER_SCENE_END, txt_);
	}

	// Finish.
	hasDirtyAsset(false);
	hasDirtyEditor(false);
	toPollEditor(false);

	return true;
}

bool Project::read(const char* path, FileReadingHandler read_) {
#if !defined GBBASIC_OS_HTML
	std::string path_ = path;
	std::string name;
	std::string ext;
	std::string dir;
	Path::split(path_, &name, &ext, &dir);
	path_ = Path::combine(dir.c_str(), name.c_str()) + ".trans";
	if (Path::fileExists(path) && Path::fileExists(path_.c_str()))
		path_ = path;
	else if (Path::fileExists(path) && !Path::fileExists(path_.c_str()))
		path_ = path;
	else if (!Path::fileExists(path) && !Path::fileExists(path_.c_str()))
		path_ = path;

	File::Ptr file(File::create());
	if (!file->open(path_.c_str(), Stream::READ))
		return false;

	if (!read_(file)) {
		file->close();

		return false;
	}
	file->close();

	return true;
#else /* Platform macro. */
	File::Ptr file(File::create());
	if (!file->open(path, Stream::READ))
		return false;

	if (!read_(file)) {
		file->close();

		return false;
	}
	file->close();

	return true;
#endif /* Platform macro. */
}

bool Project::write(const char* path, FileWritingHandler write_) {
#if !defined GBBASIC_OS_HTML
	std::string path_ = path;
	std::string name;
	std::string ext;
	std::string dir;
	Path::split(path_, &name, &ext, &dir);
	path_ = Path::combine(dir.c_str(), name.c_str()) + ".trans";

	File::Ptr file(File::create());
	if (!file->open(path_.c_str(), Stream::WRITE))
		return false;

	if (!write_(file)) {
		file->close();

		Path::removeFile(path_.c_str(), false);

		return false;
	}
	file->close();

	Path::removeFile(path, false);
	Path::moveFile(path_.c_str(), path);

	return true;
#else /* Platform macro. */
	File::Ptr file(File::create());
	if (!file->open(path, Stream::WRITE))
		return false;

	if (!write_(file)) {
		file->close();

		return false;
	}
	file->close();

	return true;
#endif /* Platform macro. */
}

bool Project::retrieve(const std::string &content, const std::string &sectionBegin, const std::string &sectionEnd, std::string &section) {
	const size_t beginIndex = Text::indexOf(content, sectionBegin);
	if (beginIndex == std::string::npos)
		return false;
	const size_t endIndex = Text::indexOf(content, sectionEnd, beginIndex + sectionBegin.length());
	if (endIndex == std::string::npos)
		return false;

	section = content.substr(beginIndex + sectionBegin.length(), endIndex - (beginIndex + sectionBegin.length()));

	return true;
}

bool Project::put(std::string &content, const std::string &sectionBegin, const std::string &sectionEnd, const std::string &section) {
	content += sectionBegin;
	content += "\n";
	if (!section.empty()) {
		content += section;
		content += "\n";
	}
	content += sectionEnd;
	content += "\n";

	return true;
}

/* ===========================================================================} */
