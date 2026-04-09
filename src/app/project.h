/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "../gbbasic.h"
#include "../utils/assets.h"
#include "../utils/file_sandbox.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef PROJECT_CARTRIDGE_TYPE_CLASSIC
#	define PROJECT_CARTRIDGE_TYPE_CLASSIC "classic"
#endif /* PROJECT_CARTRIDGE_TYPE_CLASSIC */
#ifndef PROJECT_CARTRIDGE_TYPE_COLORED
#	define PROJECT_CARTRIDGE_TYPE_COLORED "colored"
#endif /* PROJECT_CARTRIDGE_TYPE_COLORED */
#ifndef PROJECT_CARTRIDGE_TYPE_EXTENSION
#	define PROJECT_CARTRIDGE_TYPE_EXTENSION "extension" // GBB EXTENSION.
#endif /* PROJECT_CARTRIDGE_TYPE_EXTENSION */
#ifndef PROJECT_CARTRIDGE_TYPE_SEPARATOR
#	define PROJECT_CARTRIDGE_TYPE_SEPARATOR "|"
#endif /* PROJECT_CARTRIDGE_TYPE_SEPARATOR */

#ifndef PROJECT_SRAM_TYPE_0KB
#	define PROJECT_SRAM_TYPE_0KB "0"
#endif /* PROJECT_SRAM_TYPE_0KB */
#ifndef PROJECT_SRAM_TYPE_2KB
#	define PROJECT_SRAM_TYPE_2KB "1"
#endif /* PROJECT_SRAM_TYPE_2KB */
#ifndef PROJECT_SRAM_TYPE_8KB
#	define PROJECT_SRAM_TYPE_8KB "2"
#endif /* PROJECT_SRAM_TYPE_8KB */
#ifndef PROJECT_SRAM_TYPE_32KB
#	define PROJECT_SRAM_TYPE_32KB "3"
#endif /* PROJECT_SRAM_TYPE_32KB */
#ifndef PROJECT_SRAM_TYPE_128KB
#	define PROJECT_SRAM_TYPE_128KB "4"
#endif /* PROJECT_SRAM_TYPE_128KB */

#ifndef PROJECT_DEFAULT_ORDER
#	define PROJECT_DEFAULT_ORDER 100
#endif /* PROJECT_DEFAULT_ORDER */

/* ===========================================================================} */

/*
** {===========================================================================
** Project
*/

class Project : public virtual Object {
public:
	typedef std::shared_ptr<Project> Ptr;
	typedef std::vector<Ptr> Array;

	typedef Either<int, std::string> CodeLine;

	typedef std::vector<int> Indices;

	enum class ContentTypes {
		BASIC,
		ROM
	};

	enum class BorderFrameTypes : unsigned {
		NONE,
		DEFAULT,
		CUSTOM,
		COUNT
	};

	typedef std::array<Colour, 14> SuperPalettes;

	typedef std::function<bool(File::Ptr)> FileReadingHandler;
	typedef std::function<bool(File::Ptr)> FileWritingHandler;

	typedef std::function<void(AssetsBundle::Categories, int, BaseAssets::Entry*, Editable*)> AssetHandler;

public:
	GBBASIC_CLASS_TYPE('P', 'R', 'O', 'J')

	GBBASIC_PROPERTY             (ContentTypes,                  contentType                                      ) // Serialized in workspace.
	GBBASIC_PROPERTY             (std::string,                   path                                             ) // Serialized in project.
	GBBASIC_PROPERTY             (FileSync::Ptr,                 fileSync                                         ) // Non-serialized. For notepad mode.
	GBBASIC_FIELD                (std::string,                   title                                            ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   kernel                                           ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   cartridgeType                                    ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   sramType                                         ) // Serialized in project.
	GBBASIC_PROPERTY             (bool,                          hasRtc                                           ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   description                                      ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   author                                           ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   genre                                            ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   version                                          ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   url                                              ) // Serialized in project.
	GBBASIC_PROPERTY             (bool,                          runOnOpen                                        ) // Serialized in project.
	GBBASIC_FIELD_READONLY       (std::string,                   iconCode                                         ) // Serialized in project.
	GBBASIC_PROPERTY             (bool,                          caseInsensitive                                  ) // Serialized in project.
	GBBASIC_PROPERTY             (bool,                          strictOn                                         ) // Serialized in project.
	GBBASIC_PROPERTY             (std::string,                   preDefinedMacros                                 ) // Serialized in project.
	GBBASIC_PROPERTY             (bool,                          superFeaturesEnabled                             ) // Serialized in project.
	GBBASIC_FIELD_READONLY       (BorderFrameTypes,              borderFrameType                                  ) // Serialized in project.
	GBBASIC_FIELD_READONLY       (std::string,                   borderFrameCode                                  ) // Serialized in project.
	GBBASIC_FIELD_READONLY       (SuperPalettes,                 superPalettes                                    ) // Serialized in project.
	GBBASIC_PROPERTY             (bool,                          customizedSuperPalettes                          ) // Serialized in project.
	GBBASIC_FIELD                (long long,                     created                                          ) // Serialized in project.
	GBBASIC_FIELD                (long long,                     modified                                         ) // Serialized in project.
	GBBASIC_PROPERTY             (unsigned,                      order                                            ) // Serialized in project.
	GBBASIC_PROPERTY             (Math::Vec2i,                   preferencesFontSize                              ) // Serialized in project. For font assets.
	GBBASIC_PROPERTY             (int,                           preferencesFontOffset                            ) // Serialized in project. For font assets.
	GBBASIC_PROPERTY             (bool,                          preferencesFontIsTwoBitsPerPixel                 ) // Serialized in project. For font assets.
	GBBASIC_PROPERTY             (bool,                          preferencesFontPreferFullWord                    ) // Serialized in project. For font assets.
	GBBASIC_PROPERTY             (bool,                          preferencesFontPreferFullWordForNonAscii         ) // Serialized in project. For font assets.
	GBBASIC_PROPERTY             (bool,                          preferencesPreviewAnchorPoint                    ) // Serialized in project. For actor assets.
	GBBASIC_PROPERTY             (bool,                          preferencesPreviewPaletteBits                    ) // Serialized in project. For map, scene and actor assets.
	GBBASIC_PROPERTY             (bool,                          preferencesUseByteMatrix                         ) // Serialized in project. For map and scene assets.
	GBBASIC_PROPERTY             (bool,                          preferencesShowGrids                             ) // Serialized in project. For tiles, map, scene, actor and font assets.
	GBBASIC_PROPERTY             (int,                           preferencesCodePageForBindedRoutine              ) // Serialized in project. For scene and actor assets.
	GBBASIC_PROPERTY_VAL         (CodeLine,                      preferencesCodeLineForBindedRoutine, Left<int>(0)) // Serialized in project. For scene and actor assets.
	GBBASIC_PROPERTY             (int,                           preferencesMapRef                                ) // Serialized in project. For map assets.
	GBBASIC_PROPERTY             (bool,                          preferencesMusicPreviewStroke                    ) // Serialized in project. For music assets.
	GBBASIC_PROPERTY             (bool,                          preferencesSfxShowSoundShape                     ) // Serialized in project. For SFX assets.
	GBBASIC_PROPERTY             (bool,                          preferencesActorApplyPropertiesToAllTiles        ) // Serialized in project. For actor assets.
	GBBASIC_PROPERTY             (bool,                          preferencesActorApplyPropertiesToAllFrames       ) // Serialized in project. For actor assets.
	GBBASIC_PROPERTY             (bool,                          preferencesActorUses8x16Sprites                  ) // Serialized in project. For actor assets.
	GBBASIC_PROPERTY             (int,                           preferencesSceneRefMap                           ) // Serialized in project. For scene assets.
	GBBASIC_PROPERTY             (bool,                          preferencesSceneShowActors                       ) // Serialized in project. For scene assets.
	GBBASIC_PROPERTY             (bool,                          preferencesSceneShowTriggers                     ) // Serialized in project. For scene assets.
	GBBASIC_PROPERTY             (bool,                          preferencesSceneShowProperties                   ) // Serialized in project. For scene assets.
	GBBASIC_PROPERTY             (bool,                          preferencesSceneShowAttributes                   ) // Serialized in project. For scene assets.
	GBBASIC_PROPERTY             (bool,                          preferencesSceneUseGravity                       ) // Serialized in project. For scene assets.

	GBBASIC_PROPERTY_READONLY_PTR(class Window,                  window                                           ) // Non-serialized.
	GBBASIC_PROPERTY_READONLY_PTR(Renderer,                      renderer                                         ) // Non-serialized.
	GBBASIC_PROPERTY_READONLY_PTR(class Workspace,               workspace                                        ) // Non-serialized.
	GBBASIC_PROPERTY             (unsigned,                      engine                                           ) // Non-serialized.
	GBBASIC_PROPERTY             (AssetsBundle::Ptr,             assets                                           ) // Non-serialized.
	GBBASIC_PROPERTY             (Bytes::Ptr,                    rom                                              ) // Non-serialized.
	GBBASIC_PROPERTY             (Texture::Ptr,                  attributesTexture                                ) // Non-serialized.
	GBBASIC_PROPERTY             (Texture::Ptr,                  propertiesTexture                                ) // Non-serialized.
	GBBASIC_PROPERTY             (Texture::Ptr,                  actorsTexture                                    ) // Non-serialized.
	GBBASIC_PROPERTY             (active_t::BehaviourSerializer, behaviourSerializer                              ) // Non-serialized.
	GBBASIC_PROPERTY             (active_t::BehaviourParser,     behaviourParser                                  ) // Non-serialized.
	GBBASIC_FIELD_READONLY       (bool,                          hasDirtyInformation                              ) // Non-serialized.
	GBBASIC_FIELD_READONLY       (bool,                          hasDirtyAsset                                    ) // Non-serialized.
	GBBASIC_FIELD_READONLY       (bool,                          hasDirtyEditor                                   ) // Non-serialized.
	GBBASIC_PROPERTY             (bool,                          toPollEditor                                     ) // Non-serialized.
	GBBASIC_PROPERTY             (bool,                          isPlain                                          ) // Non-serialized.
	GBBASIC_PROPERTY             (bool,                          preferPlain                                      ) // Non-serialized.
	GBBASIC_PROPERTY             (bool,                          isExample                                        ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeMajorCodeIndex                             ) // Serialized in workspace.
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	GBBASIC_PROPERTY             (bool,                          isMajorCodeEditorActive                          ) // Non-serialized.
	GBBASIC_PROPERTY             (bool,                          isMinorCodeEditorEnabled                         ) // Serialized in workspace.
	GBBASIC_PROPERTY             (int,                           activeMinorCodeIndex                             ) // Serialized in workspace.
	GBBASIC_PROPERTY_PTR         (class EditorCode,              minorCodeEditor                                  ) // Non-serialized.
	GBBASIC_PROPERTY             (float,                         minorCodeEditorWidth                             ) // Serialized in workspace.
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	GBBASIC_PROPERTY             (int,                           activePaletteIndex                               ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeFontIndex                                  ) // Non-serialized.
	GBBASIC_PROPERTY             (float,                         fontPreviewHeight                                ) // Serialized in workspace.
	GBBASIC_PROPERTY             (int,                           activeTilesIndex                                 ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeMapIndex                                   ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeMusicIndex                                 ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeSfxIndex                                   ) // Non-serialized.
	GBBASIC_PROPERTY_READONLY_PTR(class EditorSfx,               sharedSfxEditor                                  ) // Non-serialized.
	GBBASIC_PROPERTY             (bool,                          sharedSfxEditorDisabled                          ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeActorIndex                                 ) // Non-serialized.
	GBBASIC_PROPERTY             (int,                           activeSceneIndex                                 ) // Non-serialized.
	GBBASIC_PROPERTY             (std::string,                   shortTitle                                       ) // Non-serialized.
	GBBASIC_PROPERTY             (std::string,                   createdString                                    ) // Non-serialized.
	GBBASIC_PROPERTY             (std::string,                   modifiedString                                   ) // Non-serialized.

	GBBASIC_PROPERTY             (float,                         inLibTextWidth                                   ) // Non-serialized.

private:
	Texture::Ptr _iconTexture = nullptr;
	bool _iconTextureDirty = true;
	Semaphore _iconTextureIsBeingGenerated;
	Texture::Ptr _iconTexture2Bpp = nullptr;
	bool _iconTexture2BppDirty = true;
	Semaphore _iconTexture2BppDirtyIsBeingGenerated;

public:
	Project(class Window* wnd, Renderer* rnd, class Workspace* ws);
	virtual ~Project() override;

	Project &operator = (const Project &other);

	virtual unsigned type(void) const override;

	void title(const std::string &val);
	void iconCode(const std::string &val);
	Texture::Ptr &touchIconTexture(void);
	Texture::Ptr &touchIconTexture2Bpp(void);
	Image::Ptr touchIconImage2Bpp(Bytes::Ptr tiles /* nullable */);
	void borderFrameType(BorderFrameTypes y);
	void borderFrameCode(const std::string &val);
	void superPalettes(const SuperPalettes &val);
	void superPalettes(const Colour* val);
	int countSuperPaletteColor(void) const;
	const Colour* getSuperPaletteColor(int idx) const;
	bool setSuperPaletteColor(int idx, const Colour &val);
	void synchronizeSuperPalettes(const PaletteAssets &val);
	void created(const long long &val);
	void modified(const long long &val);

	int palettePageCount(void) const;
	bool addGlobalPalettePages(WarningOrErrorHandler onWarningOrError);
	const PaletteAssets::Entry* getPalette(int index) const;
	PaletteAssets::Entry* getPalette(int index);
	PaletteAssets &touchPalette(void);
	PaletteAssets::Getter paletteGetter(void);
	Bytes::Ptr backgroundPalettes(void);
	Bytes::Ptr spritePalettes(void);

	int fontPageCount(void) const;
	bool addGlobalFontPages(const char* fontConfigPath, WarningOrErrorHandler onWarningOrError);
	void makeFontEntry(FontAssets::Entry &entry, bool toBeCopied, bool toBeEmbedded, const std::string &path_, const Math::Vec2i* size /* nullable */, const std::string &content_, bool generateObject);
	bool addFontPage(bool toBeCopied, bool toBeEmbedded, const std::string &path_, const Math::Vec2i* size /* nullable */, const std::string &content_, bool isNew);
	bool duplicateFontPage(const FontAssets::Entry* src);
	bool removeFontPage(int index);
	void clearFontPages(void);
	const FontAssets::Entry* getFont(int index) const;
	FontAssets::Entry* getFont(int index);
	bool canRenameFont(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableFontName(int index) const;

	int codePageCount(void) const;
	bool addCodePage(const std::string &val);
	bool removeCodePage(int index);
	const CodeAssets::Entry* getCode(int index) const;
	CodeAssets::Entry* getCode(int index);

	int tilesPageCount(void) const;
	bool addTilesPage(const std::string &val, bool isNew);
	bool removeTilesPage(int index);
	const TilesAssets::Entry* getTiles(int index) const;
	TilesAssets::Entry* getTiles(int index);
	TilesAssets::Getter tilesGetter(void);
	bool canRenameTiles(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableTilesName(int index) const;

	int mapPageCount(void) const;
	bool addMapPage(const std::string &val, bool isNew, const char* preferedName /* nullable */);
	bool removeMapPage(int index);
	const MapAssets::Entry* getMap(int index) const;
	MapAssets::Entry* getMap(int index);
	MapAssets::Getter mapGetter(void);
	bool canRenameMap(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableMapName(int index) const;
	Indices getMapsRefToTiles(int tilesIndex) const;

	int musicPageCount(void) const;
	bool addMusicPage(const std::string &val, bool isNew);
	bool removeMusicPage(int index);
	const MusicAssets::Entry* getMusic(int index) const;
	MusicAssets::Entry* getMusic(int index);
	bool canRenameMusic(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableMusicName(int index) const;
	std::string getUsableMusicName(const std::string &prefered, int index) const;
	bool canRenameMusicInstrument(int index, int instIndex, const std::string &name, int* another /* nullable */) const;
	std::string getUsableMusicInstrumentName(int index, int instIndex) const;

	int sfxPageCount(void) const;
	bool addSfxPage(const std::string &val, bool isNew, bool insertBefore, bool insertAfter);
	bool removeSfxPage(int index);
	const SfxAssets::Entry* getSfx(int index) const;
	SfxAssets::Entry* getSfx(int index);
	bool swapSfxPages(int leftIndex, int rightIndex);
	bool canRenameSfx(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableSfxName(int index) const;
	std::string getUsableSfxName(const std::string &prefix, int index) const;
	void createSharedSfxEditor(void);
	void destroySharedSfxEditor(void);

	int actorPageCount(void) const;
	bool addActorPage(const std::string &val, bool isNew);
	bool removeActorPage(int index);
	const ActorAssets::Entry* getActor(int index) const;
	ActorAssets::Entry* getActor(int index);
	ActorAssets::Getter actorGetter(void);
	bool canRenameActor(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableActorName(int index) const;

	int scenePageCount(void) const;
	bool addScenePage(const std::string &val, bool isNew, const char* preferedName /* nullable */);
	bool removeScenePage(int index);
	const SceneAssets::Entry* getScene(int index) const;
	SceneAssets::Entry* getScene(int index);
	bool canRenameScene(int index, const std::string &name, int* another /* nullable */) const;
	std::string getUsableSceneName(int index) const;

	bool sramExists(class Workspace* ws) const;
	std::string sramPath(class Workspace* ws) const;

	bool editable(void) const;
	bool dirty(void);
	void hasDirtyInformation(bool val);
	void hasDirtyAsset(bool val);
	void hasDirtyEditor(bool val);

	bool open(const char* path);
	bool close(bool deep);

	bool exists(void) const;
	bool loaded(void) const;
	void unload(void);
	bool load(bool allowBin, const char* fontConfigPath, WarningOrErrorHandler onWarningOrError = nullptr);
	bool save(const char* path, bool redirect, WarningOrErrorHandler onWarningOrError = nullptr);
	bool rename(const char* name);

	void foreach(AssetHandler handle);

private:
	void transfer(void);

	bool extractRom(Bytes* data /* nullable */, Bytes* icon /* nullable */) const;
	bool loadRom(WarningOrErrorHandler onWarningOrError);
	bool loadBasic(const char* fontConfigPath, WarningOrErrorHandler onWarningOrError);
	bool saveBasic(const char* path_, bool redirect, WarningOrErrorHandler onWarningOrError);

	bool loadInformation(const std::string &content, WarningOrErrorHandler onWarningOrError);
	bool saveInformation(std::string &content);

	bool loadAssets(const char* fontConfigPath, const std::string &content, WarningOrErrorHandler onWarningOrError);
	bool saveAssets(std::string &content, WarningOrErrorHandler onWarningOrError);

	static bool read(const char* path, FileReadingHandler read_);
	static bool write(const char* path, FileWritingHandler write_);

	static bool retrieve(const std::string &content, const std::string &sectionBegin, const std::string &sectionEnd, std::string &section);
	static bool put(std::string &content, const std::string &sectionBegin, const std::string &sectionEnd, const std::string &section);
};

/* ===========================================================================} */

#endif /* __PROJECT_H__ */
