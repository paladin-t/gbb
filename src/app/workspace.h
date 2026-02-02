/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __WORKSPACE_H__
#define __WORKSPACE_H__

#include "../gbbasic.h"
#include "activities.h"
#include "device.h"
#include "editing.h"
#include "project.h"
#include "widgets.h"
#include "../compiler/static_analyzer.h"
#if GBBASIC_MULTITHREAD_ENABLED
#	include <thread>
#endif /* GBBASIC_MULTITHREAD_ENABLED */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WORKSPACE_OPTION_APPLICATION_CWD_KEY
#	define WORKSPACE_OPTION_APPLICATION_CWD_KEY "W"
#endif /* WORKSPACE_OPTION_APPLICATION_CWD_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_COMMANDLINE_ONLY_KEY
#	define WORKSPACE_OPTION_APPLICATION_COMMANDLINE_ONLY_KEY "L"
#endif /* WORKSPACE_OPTION_APPLICATION_COMMANDLINE_ONLY_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_UPGRADE_ONLY_KEY
#	define WORKSPACE_OPTION_APPLICATION_UPGRADE_ONLY_KEY "U"
#endif /* WORKSPACE_OPTION_APPLICATION_UPGRADE_ONLY_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_DO_NOT_QUIT_KEY
#	define WORKSPACE_OPTION_APPLICATION_DO_NOT_QUIT_KEY "Q"
#endif /* WORKSPACE_OPTION_APPLICATION_DO_NOT_QUIT_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_CONSOLE_ENABLED_KEY
#	define WORKSPACE_OPTION_APPLICATION_CONSOLE_ENABLED_KEY "C"
#endif /* WORKSPACE_OPTION_APPLICATION_CONSOLE_ENABLED_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_FPS_KEY
#	define WORKSPACE_OPTION_APPLICATION_FPS_KEY "A"
#endif /* WORKSPACE_OPTION_APPLICATION_FPS_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_HIDE_SPLASH_KEY
#	define WORKSPACE_OPTION_APPLICATION_HIDE_SPLASH_KEY "H"
#endif /* WORKSPACE_OPTION_APPLICATION_HIDE_SPLASH_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_NOTEPAD_MODE_KEY
#	define WORKSPACE_OPTION_APPLICATION_NOTEPAD_MODE_KEY "N"
#endif /* WORKSPACE_OPTION_APPLICATION_NOTEPAD_MODE_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_FORCE_WRITABLE_KEY
#	define WORKSPACE_OPTION_APPLICATION_FORCE_WRITABLE_KEY "P"
#endif /* WORKSPACE_OPTION_APPLICATION_FORCE_WRITABLE_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_BORDERLESS_ENABLED_KEY
#	define WORKSPACE_OPTION_WINDOW_BORDERLESS_ENABLED_KEY "B"
#endif /* WORKSPACE_OPTION_WINDOW_BORDERLESS_ENABLED_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_SIZE_KEY
#	define WORKSPACE_OPTION_WINDOW_SIZE_KEY "S"
#endif /* WORKSPACE_OPTION_WINDOW_SIZE_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY
#	define WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY "D"
#endif /* WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_ALWAYS_ON_TOP_ENABLED_KEY
#	define WORKSPACE_OPTION_WINDOW_ALWAYS_ON_TOP_ENABLED_KEY "M"
#endif /* WORKSPACE_OPTION_WINDOW_ALWAYS_ON_TOP_ENABLED_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X1_KEY
#	define WORKSPACE_OPTION_RENDERER_X1_KEY "X1"
#endif /* WORKSPACE_OPTION_RENDERER_X1_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X2_KEY
#	define WORKSPACE_OPTION_RENDERER_X2_KEY "X2"
#endif /* WORKSPACE_OPTION_RENDERER_X2_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X3_KEY
#	define WORKSPACE_OPTION_RENDERER_X3_KEY "X3"
#endif /* WORKSPACE_OPTION_RENDERER_X3_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X4_KEY
#	define WORKSPACE_OPTION_RENDERER_X4_KEY "X4"
#endif /* WORKSPACE_OPTION_RENDERER_X4_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X5_KEY
#	define WORKSPACE_OPTION_RENDERER_X5_KEY "X5"
#endif /* WORKSPACE_OPTION_RENDERER_X5_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X6_KEY
#	define WORKSPACE_OPTION_RENDERER_X6_KEY "X6"
#endif /* WORKSPACE_OPTION_RENDERER_X6_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_DRIVER_KEY
#	define WORKSPACE_OPTION_RENDERER_DRIVER_KEY "R"
#endif /* WORKSPACE_OPTION_RENDERER_DRIVER_KEY */

#ifndef WORKSPACE_PREFERENCES_NAME
#	define WORKSPACE_PREFERENCES_NAME "gbbasic"
#endif /* WORKSPACE_PREFERENCES_NAME */
#ifndef WORKSPACE_ACTIVITIES_NAME
#	define WORKSPACE_ACTIVITIES_NAME "gbbasic_activities"
#endif /* WORKSPACE_ACTIVITIES_NAME */

#ifndef WORKSPACE_FONT_DIR
#	define WORKSPACE_FONT_DIR "../fonts/" /* Relative path. */
#endif /* WORKSPACE_FONT_DIR */
#ifndef WORKSPACE_FONT_DEFAULT_CONFIG_FILE
#	define WORKSPACE_FONT_DEFAULT_CONFIG_FILE WORKSPACE_FONT_DIR "default.json"
#endif /* WORKSPACE_FONT_DEFAULT_CONFIG_FILE */

#ifndef WORKSPACE_MUSIC_DIR
#	define WORKSPACE_MUSIC_DIR "../music/" /* Relative path. */
#endif /* WORKSPACE_MUSIC_DIR */
#ifndef WORKSPACE_MUSIC_MENU_ENABLED
#	if defined GBBASIC_OS_MAC
#		define WORKSPACE_MUSIC_MENU_ENABLED 1
#	elif defined GBBASIC_OS_HTML
#		define WORKSPACE_MUSIC_MENU_ENABLED 1
#	else /* Platform macro. */
#		define WORKSPACE_MUSIC_MENU_ENABLED 0
#	endif /* Platform macro. */
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */

#ifndef WORKSPACE_SFX_DIR
#	define WORKSPACE_SFX_DIR "../sfx/" /* Relative path. */
#endif /* WORKSPACE_SFX_DIR */
#ifndef WORKSPACE_SFX_MENU_ENABLED
#	if defined GBBASIC_OS_MAC
#		define WORKSPACE_SFX_MENU_ENABLED 1
#	elif defined GBBASIC_OS_HTML
#		define WORKSPACE_SFX_MENU_ENABLED 1
#	else /* Platform macro. */
#		define WORKSPACE_SFX_MENU_ENABLED 0
#	endif /* Platform macro. */
#endif /* WORKSPACE_SFX_MENU_ENABLED */

#ifndef WORKSPACE_EXAMPLE_PROJECTS_DIR
#	define WORKSPACE_EXAMPLE_PROJECTS_DIR "../examples/" /* Relative path. */
#endif /* WORKSPACE_EXAMPLE_PROJECTS_DIR */
#ifndef WORKSPACE_EXAMPLE_PROJECTS_CONFIG_FILE
#	define WORKSPACE_EXAMPLE_PROJECTS_CONFIG_FILE WORKSPACE_EXAMPLE_PROJECTS_DIR "config.json"
#endif /* WORKSPACE_EXAMPLE_PROJECTS_CONFIG_FILE */
#ifndef WORKSPACE_EXAMPLE_PROJECTS_WRITABLE
#	if defined GBBASIC_DEBUG
#		define WORKSPACE_EXAMPLE_PROJECTS_WRITABLE 1
#	else /* GBBASIC_DEBUG */
#		define WORKSPACE_EXAMPLE_PROJECTS_WRITABLE 0
#	endif /* GBBASIC_DEBUG */
#endif /* WORKSPACE_EXAMPLE_PROJECTS_WRITABLE */
#ifndef WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED
#	if defined GBBASIC_OS_MAC
#		define WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED 1
#	elif defined GBBASIC_OS_HTML
#		define WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED 1
#	else /* Platform macro. */
#		define WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED 0
#	endif /* Platform macro. */
#endif /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */

#ifndef WORKSPACE_STARTER_KITS_PROJECTS_DIR
#	define WORKSPACE_STARTER_KITS_PROJECTS_DIR "../kits/" /* Relative path. */
#endif /* WORKSPACE_STARTER_KITS_PROJECTS_DIR */
#ifndef WORKSPACE_STARTER_KITS_PROJECTS_WRITABLE
#	if defined GBBASIC_DEBUG
#		define WORKSPACE_STARTER_KITS_PROJECTS_WRITABLE 1
#	else /* GBBASIC_DEBUG */
#		define WORKSPACE_STARTER_KITS_PROJECTS_WRITABLE 0
#	endif /* GBBASIC_DEBUG */
#endif /* WORKSPACE_STARTER_KITS_PROJECTS_WRITABLE */

#ifndef WORKSPACE_KERNEL_INSTALLER_ENABLED
#	if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
#		define WORKSPACE_KERNEL_INSTALLER_ENABLED 1
#	elif defined GBBASIC_OS_HTML
#		define WORKSPACE_KERNEL_INSTALLER_ENABLED 1
#	else /* Platform macro. */
#		define WORKSPACE_KERNEL_INSTALLER_ENABLED 1
#	endif /* Platform macro. */
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */

#ifndef WORKSPACE_LINKS_FILE
#	define WORKSPACE_LINKS_FILE "links.json"
#endif /* WORKSPACE_LINKS_FILE */

#ifndef WORKSPACE_ALTERNATIVE_ROOT_PATH
#	define WORKSPACE_ALTERNATIVE_ROOT_PATH "../.."
#endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH */
#ifndef WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED
#	if defined GBBASIC_OS_WIN
#		if defined GBBASIC_DEBUG
#			define WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED 1
#		else /* GBBASIC_DEBUG */
#			define WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED 0
#		endif /* GBBASIC_DEBUG */
#	elif defined GBBASIC_OS_MAC
#		define WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED 1
#	else /* Platform macro. */
#		define WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED 0
#	endif /* Platform macro. */
#endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED */

#ifndef WORKSPACE_OPEN_BIN_FOR_NOTEPAD_ENABLED
#	define WORKSPACE_OPEN_BIN_FOR_NOTEPAD_ENABLED 1
#endif /* WORKSPACE_OPEN_BIN_FOR_NOTEPAD_ENABLED */

#ifndef WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED
#	if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
#		define WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED 1
#	else /* Platform macro. */
#		define WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED 0
#	endif /* Platform macro. */
#endif /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */

#ifndef WORKSPACE_AUTO_CLOSE_POPUP
#	define WORKSPACE_AUTO_CLOSE_POPUP(W) \
		ProcedureGuard<void> GBBASIC_UNIQUE_NAME(__CLOSE__)( \
			std::bind( \
				[] (Workspace* ws) -> void* { \
					ImGui::PopupBox::Ptr popup = ws->popupBox(); \
					return (void*)(uintptr_t)popup.get(); \
				}, \
				(W) \
			), \
			std::bind( \
				[] (Workspace* ws, void* ptr) -> void { \
					ImGui::PopupBox* popup = (ImGui::PopupBox*)(uintptr_t)ptr; \
					if (popup == ws->popupBox().get()) \
						ws->popupBox(nullptr); \
				}, \
				(W), std::placeholders::_1 \
			) \
		);
#endif /* WORKSPACE_AUTO_CLOSE_POPUP */

constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_RECENT =
	ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_CONTENT =
	ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoBringToFrontOnFocus |
	ImGuiWindowFlags_NoNav;
constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_DOCK =
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoBringToFrontOnFocus |
	ImGuiWindowFlags_NoNav;
constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_FLOAT =
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoNav;
constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_FIELD_BAR =
	ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_AlwaysAutoResize |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoNav;

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

namespace ImGui {

class CodeEditor;

}

/* ===========================================================================} */

/*
** {===========================================================================
** Workspace
*/

/**
 * @brief Workspace entity.
 */
class Workspace : public Device::Protocol, public NonCopyable {
	friend class Operations;

public:
	/**< Compiling. */

	struct CompilingParameters {
		Window* window = nullptr;
		Renderer* renderer = nullptr;
		Workspace* self = nullptr;
		GBBASIC::Kernel::Ptr kernel = nullptr;
		Project::Ptr project = nullptr;
		Text::Dictionary arguments;
		bool threaded = false;
		bool commandlineOnly = false;
		bool doNotQuit = true;

		CompilingParameters();
		~CompilingParameters();
	};
	struct CompilingErrors : public virtual Object {
		typedef std::shared_ptr<CompilingErrors> Ptr;

		struct Entry {
			std::string message;
			bool isWarning = false;
			AssetsBundle::Categories category = AssetsBundle::Categories::NONE;
			int page = -1;
			int row = -1;
			int column = -1;

			Entry();
			Entry(std::string message, bool isWarning, int page, int row, int column);
			Entry(std::string message, bool isWarning, AssetsBundle::Categories cat, int page);

			bool operator == (const Entry &other) const;
		};
		typedef std::vector<Entry> Array;

		GBBASIC_CLASS_TYPE('C', 'P', 'E', 'R')

		Array array;

		CompilingErrors();
		virtual ~CompilingErrors() override;

		virtual unsigned type(void) const override;

		int count(void) const;
		void add(const Entry &entry);
		const Entry* get(int index) const;
		int indexOf(const Entry &entry) const;
	};

	enum class States {
		IDLE,
		COMPILING,
		COMPILED,
		LAUNCHED
	};

	typedef std::function<void(const std::string &)> CompilerOutputHandler;

	/**< Data. */

	struct Delayed {
		typedef std::list<Delayed> List;

		typedef std::function<void(void)> Handler;

		struct Amount {
			enum class Types {
				FOR_FRAMES,
				UNTIL_TIMESTAMP
			};

			Types type = Types::FOR_FRAMES;
			long long data = 0;

			Amount();
			Amount(Types y, long long d);
		};

		Handler handler = nullptr;
		std::string key;
		Amount delay = Amount(Amount::Types::FOR_FRAMES, 1);

		Delayed();
		Delayed(Handler handler_, const std::string &key_, const Amount &delay_);
	};

	struct AssetPageNames {
		Text::Array font;
		Text::Array code;
		Text::Array tiles;
		Text::Array map;
		Text::Array music;
		Text::Array sfx;
		Text::Array actor;
		Text::Array scene;

		AssetPageNames();

		void clear(void);
	};

	/**< GUI. */

	enum class Orders : unsigned {
		DEFAULT,
		TITLE,
		LATEST_MODIFIED,
		EARLIEST_MODIFIED,
		LATEST_CREATED,
		EARLIEST_CREATED,
		COUNT
	};

	enum class Categories : unsigned {
		PALETTE   = (unsigned)AssetsBundle::Categories::PALETTE,
		FONT      = (unsigned)AssetsBundle::Categories::FONT,
		CODE      = (unsigned)AssetsBundle::Categories::CODE,
		TILES     = (unsigned)AssetsBundle::Categories::TILES,
		MAP       = (unsigned)AssetsBundle::Categories::MAP,
		MUSIC     = (unsigned)AssetsBundle::Categories::MUSIC,
		SFX       = (unsigned)AssetsBundle::Categories::SFX,
		ACTOR     = (unsigned)AssetsBundle::Categories::ACTOR,
		SCENE     = (unsigned)AssetsBundle::Categories::SCENE,
		CONSOLE,
		EMULATOR,
		HOME
	};

	typedef std::vector<int> ProjectIndices;

	typedef std::function<void(void)> KernelEjectionHandler;

	/**< External events. */

	enum class ExternalEventTypes : unsigned {
		RESIZE_WINDOW,
		UNLOAD_WINDOW,
		LOAD_PROJECT,
		PATCH_PROJECT,
		INPUT_KEY,
		INPUT_TEXT,
		TO_CATEGORY,
		TO_PAGE,
		TO_LOCATION,
		COMPILE,
		RUN,
		TOGGLE_VRAM_DEBUGGER,
		COUNT
	};

private:
	/**< Emulating. */

	struct DebuggerMessages {
		Text::List messages;
		std::string text;

		DebuggerMessages();
		~DebuggerMessages();

		bool empty(void) const;
		void add(const std::string &txt);
		void clear(void);
	};

	/**< GUI. */

	struct Filter {
		bool enabled = false;
		bool initialized = false;
		std::string pattern;
		float height = 0;

		Filter();
	};

	struct Fadable {
		bool enabled = false;
		double ticks = 0;
		std::string text;

		Fadable();
	};

	typedef std::function<void(Categories, BaseAssets::Entry*, class Editable*)> EditorHandler;

public:
	/**< Data. */

	GBBASIC_PROPERTY(ImGui::Initializer, init)

	GBBASIC_PROPERTY_READONLY(bool, busy)

	GBBASIC_PROPERTY_READONLY(unsigned, activeFrameRate)

	GBBASIC_PROPERTY(Settings, settings)
	GBBASIC_PROPERTY(Activities, activities)
	GBBASIC_PROPERTY(bool, showRecentProjects)
	GBBASIC_FIELD_READONLY(Categories, category)
	GBBASIC_PROPERTY_READONLY(Categories, categoryOfAudio)
	GBBASIC_PROPERTY_READONLY(Categories, categoryBeforeCompiling)
	GBBASIC_PROPERTY_READONLY(bool, interactable)
	GBBASIC_PROPERTY_READONLY(bool, toRun)
	GBBASIC_PROPERTY_READONLY(int, toExport)
	GBBASIC_PROPERTY_READONLY(bool, forceWritable)
	GBBASIC_PROPERTY_READONLY(std::string, fontConfig)
	GBBASIC_PROPERTY_READONLY(unsigned, exampleRevision)

	GBBASIC_PROPERTY(Text::Array, droppedFiles)
	GBBASIC_PROPERTY(ProjectIndices, projectIndices)
	GBBASIC_PROPERTY(Project::Array, projects)
	GBBASIC_PROPERTY(Project::Ptr, currentProject)
#if defined GBBASIC_OS_HTML
	GBBASIC_PROPERTY_READONLY(bool, loadingProject)
#endif /* Platform macro. */

	GBBASIC_PROPERTY_READONLY_PTR(Input, input)

	GBBASIC_PROPERTY(Delayed::List, delayed)

	GBBASIC_PROPERTY_READONLY_PTR(WorkQueue, workQueue)

	GBBASIC_PROPERTY_READONLY(GBBASIC::StaticAnalyzer::Ptr, staticAnalyzer)
	GBBASIC_PROPERTY(bool, needAnalyzing)
	GBBASIC_FIELD(std::string, analyzedCodeInformation)

	GBBASIC_FIELD(AssetPageNames, assetPageNames)

	/**< Addons. */

	GBBASIC_PROPERTY_READONLY_PTR(class Theme, theme)

	GBBASIC_PROPERTY(bool, splashCustomized)
	GBBASIC_PROPERTY_PTR(Texture, splashGbbasic)

	GBBASIC_PROPERTY_READONLY_PTR(class Recorder, recorder)

	GBBASIC_PROPERTY(GBBASIC::Kernel::Array, kernels)
	GBBASIC_PROPERTY_READONLY(int, activeKernelIndex)
	GBBASIC_PROPERTY_READONLY(bool, hasDefaultKernelSourceCode)

	GBBASIC_PROPERTY(Exporter::Array, exporters)
	GBBASIC_PROPERTY(std::string, exporterLicenses)

#if WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED
	GBBASIC_PROPERTY(EntryWithPath::List, examples)
#endif /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */
	GBBASIC_PROPERTY_READONLY(int, exampleCount)

	GBBASIC_PROPERTY(EntryWithPath::Array, starterKits)

#if WORKSPACE_MUSIC_MENU_ENABLED
	GBBASIC_PROPERTY(Entry::Dictionary, music)
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */
	GBBASIC_PROPERTY_READONLY(int, musicCount)

#if WORKSPACE_SFX_MENU_ENABLED
	GBBASIC_PROPERTY(Entry::Dictionary, sfx)
#endif /* WORKSPACE_SFX_MENU_ENABLED */
	GBBASIC_PROPERTY_READONLY(int, sfxCount)

	GBBASIC_PROPERTY(Entry::Dictionary, documents)

	GBBASIC_PROPERTY(Entry::Dictionary, links)

	/**< GUI. */

	GBBASIC_PROPERTY_READONLY(Math::Vec2i, previousRendererSize)

	GBBASIC_FIELD_READONLY(ImGui::Bubble::Ptr, bubble)
	GBBASIC_FIELD_READONLY(ImGui::PopupBox::Ptr, popupBox)

	GBBASIC_PROPERTY_READONLY(bool, isFocused)
	GBBASIC_PROPERTY_READONLY(float, menuHeight)
	GBBASIC_PROPERTY_READONLY(bool, menuOpened)
	GBBASIC_PROPERTY_READONLY(float, tabsWidth)
	GBBASIC_PROPERTY(float, statusWidth)
#if WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED
	GBBASIC_PROPERTY_READONLY(bool, borderMouseDown)
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, borderMouseDownPosition)
	GBBASIC_PROPERTY_READONLY(bool, ignoreBorderMouseResizing)
	GBBASIC_PROPERTY_READONLY(unsigned, borderResizingDirection)
	GBBASIC_PROPERTY_READONLY(bool, headBarMouseDown)
	GBBASIC_PROPERTY_READONLY(Math::Vec2i, headBarMouseDownPosition)
	GBBASIC_PROPERTY_READONLY(bool, ignoreHeadBarMouseMoving)
#endif /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */

	GBBASIC_PROPERTY_READONLY(float, recentProjectItemHeight)
	GBBASIC_PROPERTY_READONLY(float, recentProjectItemIconWidth)
	GBBASIC_PROPERTY_READONLY(int, recentProjectFocusedIndex)
	GBBASIC_PROPERTY_READONLY(int, recentProjectFocusingIndex)
	GBBASIC_PROPERTY_READONLY(int, recentProjectFocusableCount)
	GBBASIC_PROPERTY_READONLY(int, recentProjectSelectingIndex)
	GBBASIC_PROPERTY_READONLY(int, recentProjectSelectedIndex)
	GBBASIC_PROPERTY_READONLY(double, recentProjectSelectedTimestamp)
	GBBASIC_PROPERTY_READONLY(int, recentProjectWithContextIndex)
	GBBASIC_PROPERTY_READONLY(int, recentProjectsColumnCount)
	GBBASIC_PROPERTY(Filter, recentProjectsFilter)
	GBBASIC_PROPERTY_READONLY(ImVec2, recentProjectsFilterPosition)
	GBBASIC_PROPERTY(bool, recentProjectsFilterFocused)
	GBBASIC_PROPERTY(Fadable, recentProjectSort)

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	GBBASIC_PROPERTY(float, codeEditorMinorWidth)
	GBBASIC_PROPERTY(bool, codeEditorMinorResizing)
	GBBASIC_PROPERTY(bool, codeEditorMinorResetting)
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	GBBASIC_PROPERTY_READONLY(Device::Ptr, canvasDevice)
	GBBASIC_PROPERTY_READONLY(bool, canvasFixRatio)
	GBBASIC_PROPERTY_READONLY(bool, canvasIntegerScale)
	GBBASIC_PROPERTY_READONLY(Texture::Ptr, canvasTexture)
	GBBASIC_PROPERTY_READONLY(Texture::Ptr, canvasTextureForBorderFrame)
	GBBASIC_PROPERTY_READONLY(bool, canvasHovering)
	GBBASIC_PROPERTY(std::string, canvasCartridgeStatusText)
	GBBASIC_PROPERTY(std::string, canvasDeviceStatusText)
	GBBASIC_PROPERTY(std::string, canvasStatusTooltip)
	GBBASIC_PROPERTY(Device::CursorTypes, canvasCursorMode)

	GBBASIC_PROPERTY_READONLY(Device::Ptr, audioDevice)

	GBBASIC_PROPERTY_PTR(class Document, document)
	GBBASIC_PROPERTY(std::string, documentTitle)

	GBBASIC_PROPERTY_READONLY_PTR(class Editable, consoleTextBox)
	GBBASIC_PROPERTY(bool, consoleVisible)
	GBBASIC_PROPERTY(bool, consoleHasWarning)
	GBBASIC_PROPERTY(bool, consoleHasError)
	GBBASIC_FIELD(Mutex, consoleLock)

	GBBASIC_PROPERTY_READONLY_PTR(class Editable, searchResultTextBox)
	GBBASIC_PROPERTY(bool, searchResultVisible)
	GBBASIC_PROPERTY(std::string, searchResultPattern)
	GBBASIC_PROPERTY(Editing::Tools::SearchResult::Array, searchResult)
	GBBASIC_PROPERTY(float, searchResultHeight)
	GBBASIC_PROPERTY(bool, searchResultResizing)

	GBBASIC_PROPERTY(DebuggerMessages, onscreenDebuggerMessages)
	GBBASIC_FIELD(Mutex, onscreenDebuggerLock)
	GBBASIC_PROPERTY_READONLY_PTR(class VramDebugger, vramDebugger)
	GBBASIC_PROPERTY(bool, vramDebuggerPreviewPaletteBits)
	GBBASIC_PROPERTY(bool, vramDebuggerShowGrids)
	GBBASIC_PROPERTY(float, vramDebuggerPreviousOuterWidth)
	GBBASIC_PROPERTY(float, vramDebuggerWidth)
	GBBASIC_PROPERTY(float, vramDebuggerHeight)
	GBBASIC_PROPERTY(bool, vramDebuggerResizing)
	GBBASIC_PROPERTY(bool, vramDebuggerResetting)

private:
	bool _opened = false;
	int _skipping = 0;
	Atomic<States> _state;
	Mutex _lock;

	bool _toUpgrade = false;
	std::string _toUpgradeToVersion;
	bool _toCompile = false;
	CompilingParameters _compilingParameters;
	CompilingErrors::Ptr _compilingErrors = nullptr;
	Bytes::Ptr _compilingOutput = nullptr;

#if defined GBBASIC_OS_HTML
	bool _hadUnsavedChanges = false;
#endif /* Platform macro. */

#if GBBASIC_MULTITHREAD_ENABLED
	std::thread _thread;
#endif /* GBBASIC_MULTITHREAD_ENABLED */

public:
	Workspace();
	~Workspace();

	bool open(Window* wnd, Renderer* rnd, const char* font, unsigned fps, bool showRecent, bool hideSplashImage, bool forceWritable, bool commandlineOnly, const char* toUpgrade /* nullable */, bool toCompile);
	bool close(Window* wnd, Renderer* rnd, bool commandlineOnly);

	States state(void) const;
	bool canUseShortcuts(void) const;
	bool canWriteProjectTo(const char* path) const;
	bool canWriteSramTo(const char* path) const;
	bool isUnderExamplesDirectory(const char* path) const;
	bool isUnderKitsDirectory(const char* path) const;

	Mutex &lock(void);

	void refreshWindowTitle(Window* wnd);
	bool skipping(void);
	void skipFrame(int n = 1);

	int getKernelIndex(const std::string &id) const;
	const std::string* getKernelId(int index) const;
	int getValidKernelIndex(const Project* prj) const;
	int getKernelUsedCountByProjects(int index) const;
	GBBASIC::Kernel::Ptr activeKernel(void) const;
	std::string serializeKernelBehaviour(int val) const;
	int parseKernelBehaviour(const std::string &id) const;
	void reloadKernels(void);

	void addMapPageFrom(Window* wnd, Renderer* rnd, int index);
	void addScenePageFrom(Window* wnd, Renderer* rnd, int index);
	void duplicateFontFrom(Window* wnd, Renderer* rnd, int index);
	void category(const Categories &category);
	bool changePage(Window* wnd, Renderer* rnd, Project* prj, Categories category, int index);
	void pageAdded(Window* wnd, Renderer* rnd, Project* prj, Categories category);
	void pageRemoved(Window* wnd, Renderer* rnd, Project* prj, Categories category, int index);

	bool load(Window* wnd, Renderer* rnd, const int* wndWidth /* nullable */, const int* wndHeight /* nullable */);
	bool save(Window* wnd, Renderer* rnd);

	bool update(Window* wnd, Renderer* rnd, double delta, unsigned fps, unsigned* fpsReq, bool* indicated);

	void record(Window* wnd, Renderer* rnd);

	void clear(void);
	bool print(const char* msg);
	bool warn(const char* msg);
	bool error(const char* msg);
	void stroke(int key);
	virtual void streamed(class Window* wnd, class Renderer* rnd, Bytes::Ptr bytes) override;
	virtual void sync(class Window* wnd, class Renderer* rnd, const char* module) override;
	virtual void debug(const char* msg) override;
	void debug(void);
	virtual void cursor(Device::CursorTypes mode) override;

	void run(class Window* wnd, class Renderer* rnd, Bytes::Ptr rom, bool traceless);
	virtual bool running(void) const override;
	virtual void pause(class Window* wnd, class Renderer* rnd) override;
	virtual void stop(class Window* wnd, class Renderer* rnd) override;

	class VramDebugger* initializeVramDebugger(class Window* wnd, class Renderer* rnd);
	void disposeVramDebugger(void);

	void focusLost(Window* wnd, Renderer* rnd);
	void focusGained(Window* wnd, Renderer* rnd);
	void renderTargetsReset(Window* wnd, Renderer* rnd);
	void moved(Window* wnd, Renderer* rnd, const Math::Vec2i &wndPos);
	void resized(Window* wnd, Renderer* rnd, const Math::Vec2i &wndSize, const Math::Vec2i &rndSize, const Math::Recti &notch, Window::Orientations orientation);
	void maximized(Window* wnd, Renderer* rnd);
	void restored(Window* wnd, Renderer* rnd);
	void dpi(Window* wnd, Renderer* rnd, const Math::Vec3f &dpi_);
	void textInput(Window* wnd, Renderer* rnd, const char* const txt);
	void fileDropped(Window* wnd, Renderer* rnd, const char* const path);
	void dropBegan(Window* wnd, Renderer* rnd);
	void dropEnded(Window* wnd, Renderer* rnd);
	bool quit(Window* wnd, Renderer* rnd);
	void sendExternalEvent(Window* wnd, Renderer* rnd, ExternalEventTypes type, void* event);

	class EditorFont* touchFontEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, FontAssets::Entry* entry /* nullable */);
	class EditorCode* touchCodeEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, bool isMajor, CodeAssets::Entry* entry /* nullable */);
	class EditorTiles* touchTilesEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, TilesAssets::Entry* entry /* nullable */);
	class EditorMap* touchMapEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, unsigned refCategory, int refIndex, MapAssets::Entry* entry /* nullable */);
	class EditorMusic* touchMusicEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, MusicAssets::Entry* entry /* nullable */);
	class EditorSfx* touchSfxEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, SfxAssets::Entry* entry /* nullable */);
	class EditorActor* touchActorEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, ActorAssets::Entry* entry /* nullable */);
	class EditorScene* touchSceneEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, unsigned refCategory, int refIndex, SceneAssets::Entry* entry /* nullable */);

	void bubble(const ImGui::Bubble::Ptr &ptr);
	void bubble(
		const std::string &content,
		const ImGui::Bubble::TimeoutHandler &timeout /* nullable */
	);
	void popupBox(const ImGui::PopupBox::Ptr &ptr);
	void waitingPopupBox(
		const ImGui::WaitingPopupBox::TimeoutHandler &timeout /* nullable */
	);
	void waitingPopupBox(
		const std::string &content,
		const ImGui::WaitingPopupBox::TimeoutHandler &timeout /* nullable */
	);
	void waitingPopupBox(
		bool dim, const std::string &content,
		bool instantly, const ImGui::WaitingPopupBox::TimeoutHandler &timeout /* nullable */,
		bool exclusive = false
	);
	void messagePopupBox(
		const std::string &content,
		const ImGui::MessagePopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::MessagePopupBox::DeniedHandler &deny /* nullable */,
		const ImGui::MessagePopupBox::CanceledHandler &cancel /* nullable */,
		const char* confirmTxt = nullptr,
		const char* denyTxt = nullptr,
		const char* cancelTxt = nullptr,
		const char* confirmTooltips = nullptr,
		const char* denyTooltips = nullptr,
		const char* cancelTooltips = nullptr,
		bool exclusive = false
	);
	void messagePopupBoxWithOption(
		const std::string &content,
		const ImGui::MessagePopupBox::ConfirmedWithOptionHandler &confirm /* nullable */,
		const ImGui::MessagePopupBox::DeniedHandler &deny /* nullable */,
		const ImGui::MessagePopupBox::CanceledHandler &cancel /* nullable */,
		const char* confirmTxt = nullptr,
		const char* denyTxt = nullptr,
		const char* cancelTxt = nullptr,
		const char* confirmTooltips = nullptr,
		const char* denyTooltips = nullptr,
		const char* cancelTooltips = nullptr,
		const char* optionBoolTxt = nullptr, bool optionBoolValue = false, bool optionBoolValueReadonly = false
	);
	void inputPopupBox(
		const std::string &content,
		const std::string &default_, unsigned flags,
		const ImGui::InputPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::InputPopupBox::CanceledHandler &cancel /* nullable */
	);
	void projectCreatingPopupBox(
		const std::string &template_,
		const std::string &kernel,
		const std::string &content,
		const std::string &default_, unsigned flags,
		const ImGui::ProjectCreatingPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::ProjectCreatingPopupBox::CanceledHandler &cancel /* nullable */
	);
	void assetsSortingPopupBox(
		Renderer* rnd,
		AssetsBundle::Categories category,
		const ImGui::AssetsSortingPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::AssetsSortingPopupBox::CanceledHandler &cancel /* nullable */
	);
	void searchPopupBox(
		const std::string &content,
		const std::string &default_, unsigned flags,
		const ImGui::SearchPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::SearchPopupBox::CanceledHandler &cancel /* nullable */
	);

	void showPaletteEditor(
		Renderer* rnd,
		int group,
		const ImGui::EditorPopupBox::ChangedHandler &changed
	);
	void showArbitraryEditor(
		Renderer* rnd,
		int fontIndex,
		const ImGui::EditorPopupBox::ChangedHandler &changed
	);
	void showCodeBindingEditor(
		Window* wnd, Renderer* rnd,
		const ImGui::EditorPopupBox::ChangedHandler &changed,
		const std::string &index
	);
	void showActorSelector(
		Renderer* rnd,
		int index,
		const ImGui::ActorSelectorPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::ActorSelectorPopupBox::CanceledHandler &cancel /* nullable */
	);
	void showPropertiesEditor(
		Renderer* rnd,
		Actor::Ptr obj, int layer,
		const Actor::Shadow::Array &shadow,
		const Text::Array &frameNames,
		const ImGui::EditorPopupBox::ChangedHandler &changed
	);
	void showExternalFontBrowser(
		Renderer* rnd,
		const std::string &content,
		const Text::Array &filter,
		const ImGui::FontResolverPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::FontResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::FontResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::FontResolverPopupBox::CustomHandler &custom /* nullable */
	);
	void showExternalMapBrowser(
		Renderer* rnd,
		const std::string &content,
		const Text::Array &filter,
		bool withTiles, bool withPath,
		const ImGui::MapResolverPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::MapResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::MapResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::MapResolverPopupBox::CustomHandler &custom /* nullable */
	);
	void showExternalSceneBrowser(
		Renderer* rnd,
		const ImGui::SceneResolverPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::SceneResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::SceneResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::SceneResolverPopupBox::CustomHandler &custom /* nullable */
	);
	void showExternalFileBrowser(
		Renderer* rnd,
		const std::string &title,
		const std::string &content,
		const Text::Array &filter,
		bool requireExisting,
		const ImGui::FileResolverPopupBox::ConfirmedHandler_Path &confirm /* nullable */,
		const ImGui::FileResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::FileResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::FileResolverPopupBox::CustomHandler &custom /* nullable */
	);
	void showExternalFileBrowser(
		Renderer* rnd,
		const std::string &title,
		const std::string &content,
		const Text::Array &filter,
		bool requireExisting,
		bool default_, const char* optTxt,
		const ImGui::FileResolverPopupBox::ConfirmedHandler_PathBoolean &confirm /* nullable */,
		const ImGui::FileResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::FileResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::FileResolverPopupBox::CustomHandler &custom /* nullable */
	);
	void showExternalFileBrowser(
		Renderer* rnd,
		const std::string &title,
		const std::string &content,
		const Text::Array &filter,
		bool requireExisting,
		int default_, int min, int max, const char* optTxt,
		bool toSave,
		const ImGui::FileResolverPopupBox::ConfirmedHandler_PathInteger &confirm /* nullable */,
		const ImGui::FileResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::FileResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::FileResolverPopupBox::CustomHandler &custom /* nullable */
	);
	void showExternalFileBrowser(
		Renderer* rnd,
		const std::string &title,
		const std::string &content,
		const Text::Array &filter,
		bool requireExisting,
		const Math::Vec2i &default_, const Math::Vec2i &min, const Math::Vec2i &max, const char* optTxt, const char* optXTxt,
		const ImGui::FileResolverPopupBox::ConfirmedHandler_PathVec2i &confirm /* nullable */,
		const ImGui::FileResolverPopupBox::CanceledHandler &cancel /* nullable */,
		const ImGui::FileResolverPopupBox::SelectedHandler &select /* nullable */,
		const ImGui::FileResolverPopupBox::CustomHandler &custom /* nullable */,
		const ImGui::FileResolverPopupBox::Vec2iResolver &resolveVec2i /* nullable */, const char* resolveVec2iTxt /* nullable */
	);
	void showRomBuildSettings(
		Renderer* rnd,
		const ImGui::RomBuildSettingsPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::RomBuildSettingsPopupBox::CanceledHandler &cancel /* nullable */
	);
	void showEmulatorBuildSettings(
		Renderer* rnd,
		const std::string &settings, const char* args /* nullable */, bool hasIcon,
		const ImGui::EmulatorBuildSettingsPopupBox::ConfirmedHandler &confirm /* nullable */,
		const ImGui::EmulatorBuildSettingsPopupBox::CanceledHandler &cancel /* nullable */
	);
	void showProjectProperty(Window* wnd, Renderer* rnd, Project* prj, bool isOpened);
	void showSearchResult(const std::string &pattern, const Editing::Tools::SearchResult::Array &found);
	void closeSearchResult(void);
	void showInstalledKernels(Window* wnd, Renderer* rnd, const char* prompt /* nullable */);
	void showPreferences(Window* wnd, Renderer* rnd, const char* tab /* nullable */);
	void showActivities(Renderer* rnd);
	void showAbout(Renderer* rnd);
	std::string getSourceCodePath(int index, std::string* name /* nullable */) const;
	void ejectSourceCode(Window* wnd, Renderer* rnd, int index, bool wait, KernelEjectionHandler ejected /* nullable */, KernelEjectionHandler canceled /* nullable */);
	void toggleDocument(const char* path /* nullable */);

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	void createMinorCodeEditor(void);
	void destroyMinorCodeEditor(void);
	void syncMajorCodeEditorDataToMinor(Window* wnd, Renderer* rnd);
	void syncMinorCodeEditorDataToMajor(Window* wnd, Renderer* rnd);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	void createAudioDevice(void);
	void destroyAudioDevice(void);
	void openAudioDevice(Bytes::Ptr rom);
	void closeAudioDevice(void);
	void updateAudioDevice(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq, Device::AudioHandler handleAudio /* nullable */);

	bool delay(Delayed::Handler delayed, const std::string &key = "", const Delayed::Amount &delay_ = Delayed::Amount(Delayed::Amount::Types::FOR_FRAMES, 5) /* 4 or 5 frames should be sufficient, even for popup initialization */);
	void clearDelayed(void);

	Semaphore async(WorkTaskFunction::OperationHandler op /* on work thread */, WorkTaskFunction::SealHandler seal /* nullable */ /* on main thread */, WorkTaskFunction::DisposeHandler dtor /* nullable */ /* on main thread */);
	void join(void);
	void wait(void);
	bool working(void) const;

	bool analyzing(void) const;
	bool analyze(bool force);
	void clearAnalyzingResult(void);
	void clearLanguageDefinition(bool clearRevision);
	unsigned getLanguageDefinitionRevision(void) const;
	const GBBASIC::Macro::List* getMacroDefinitions(void);
	const Text::Array* getDestinitions(void);
	void clearAnalyzedCodeInformation(void);
	const std::string &getAnalyzedCodeInformation(void);
	void clearAssetPageNames(void);
	void clearFontPageNames(void);
	const Text::Array &getFontPageNames(void);
	void clearCodePageNames(void);
	const Text::Array &getCodePageNames(void);
	void clearTilesPageNames(void);
	const Text::Array &getTilesPageNames(void);
	void clearMapPageNames(void);
	const Text::Array &getMapPageNames(void);
	void clearMusicPageNames(void);
	const Text::Array &getMusicPageNames(void);
	void clearSfxPageNames(void);
	const Text::Array &getSfxPageNames(void);
	void clearActorPageNames(void);
	const Text::Array &getActorPageNames(void);
	void clearScenePageNames(void);
	const Text::Array &getScenePageNames(void);

	/**
	 * @brief Upgrades project.
	 */
	void upgrade(
		Window* wnd, Renderer* rnd,
		const Text::Dictionary &arguments
	);

	void joinCompiling(void);
	/**
	 * @brief Compiles for running and exporting.
	 */
	void compile(
		Window* wnd, Renderer* rnd,
		Project::Ptr prj,
		const Text::Dictionary &arguments
	);
	/**
	 * @brief Compiles for temporary usage, i.e. audio processing.
	 */
	static Bytes::Ptr compile(
		const std::string &configPath, const std::string &config, 
		const std::string &romPath,
		const std::string &symPath, const std::string &symbols,
		const std::string &aliasesPath, const std::string &aliases,
		const std::string &title, AssetsBundle::Ptr assets,
		int bootstrapBank,
		CompilerOutputHandler print_, CompilerOutputHandler warn_, CompilerOutputHandler error_
	);

private:
	void beginSplash(Window* wnd, Renderer* rnd, bool hideSplashImage);
	void endSplash(Window* wnd, Renderer* rnd, bool hideSplashImage);

	bool loadConfig(Window* wnd, Renderer* rnd, const rapidjson::Document &doc);
	bool saveConfig(Window* wnd, Renderer* rnd, rapidjson::Document &doc);

	void loadKernels(void);
	void unloadKernels(void);

	void loadExporters(void);
	void unloadExporters(void);

	void loadExamples(Window* wnd, Renderer* rnd);
	void unloadExamples(void);

	void loadStarterKits(Window* wnd, Renderer* rnd);
	void unloadStarterKits(void);

	void loadMusic(void);
	void unloadMusic(void);

	void loadSfx(void);
	void unloadSfx(void);

	void loadDocuments(void);
	void unloadDocuments(void);

	void loadLinks(void);
	void unloadLinks(void);

	bool execute(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq, bool* isNewFrame /* nullable */);
	bool perform(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq, Device::AudioHandler handleAudio /* nullable */);
	void saveSram(Window* wnd, Renderer* rnd);

	void prepare(Window* wnd, Renderer* rnd);
	void finish(Window* wnd, Renderer* rnd);

	void shortcuts(Window* wnd, Renderer* rnd);
	void navigate(Window* wnd, Renderer* rnd, int dx, int dy, int fully, bool open, bool remove, bool rename);

	void dialog(Window* wnd, Renderer* rnd);

	void head(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq);
	void menu(Window* wnd, Renderer* rnd);
	void buttons(Window* wnd, Renderer* rnd, double delta);
	void adjust(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq);
	void tabs(Window* wnd, Renderer* rnd);
	void filter(Window* wnd, Renderer* rnd);

	void body(Window* wnd, Renderer* rnd, double delta, unsigned fps, bool isNewFrame, bool* indicated);
	void recent(Window* wnd, Renderer* rnd, float marginTop, float marginBottom);
	void notepad(Window* wnd, Renderer* rnd, float marginTop, float marginBottom);
	void font(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void code(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void tiles(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void map(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void music(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void sfx(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void actor(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void scene(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void console(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta);
	void document(Window* wnd, Renderer* rnd, float marginTop, float marginBottom);
	void emulator(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, unsigned fps, bool isNewFrame, bool* indicated);
	void debug(Window* wnd, Renderer* rnd, float marginTop, float marginBottom);
	void blank(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, Categories category);
	void found(Window* wnd, Renderer* rnd, double delta);

	void importMusic(Window* wnd, Renderer* rnd);
	void importSfx(Window* wnd, Renderer* rnd);

	/**
	 * @param[out] opened
	 * @param[out] dirty
	 * @param[out] exists
	 * @param[out] url
	 */
	void getCurrentProjectStates(
		bool* opened /* nullable */,
		bool* dirty /* nullable */,
		bool* exists /* nullable */,
		const char** url /* nullable */
	) const;
	/**
	 * @param[out] any
	 * @param[out] type
	 * @param[out] dirty
	 * @param[out] pastable
	 * @param[out] selectable
	 * @param[out] undoable
	 * @param[out] redoable
	 * @param[out] readonly
	 */
	int getCurrentAssetStates(
		bool* any /* nullable */,
		unsigned* type /* nullable */,
		bool* dirty /* nullable */,
		bool* pastable /* nullable */, bool* selectable /* nullable */,
		const char** undoable /* nullable */, const char** redoable /* nullable */,
		bool* readonly /* nullable */
	) const;
	int currentAssetPage(void) const;
	int withCurrentAsset(EditorHandler handler) const;
	int withAllAssets(EditorHandler handler) const;
	void toggleFullscreen(Window* wnd);
	void toggleMaximized(Window* wnd);
	void closeFilter(void);
	void sortProjects(void);
	void validateProject(const Project* prj);
	void launchProject(
		Window* wnd, Renderer* rnd,
		const char* cartType /* nullable */, const char* sramType /* nullable */, bool* hasRtc /* nullable */,
		bool toRun_, int toExport_
	);
	void runProject(Window* wnd, Renderer* rnd, Bytes::Ptr rom);
	void stopProject(Window* wnd, Renderer* rnd);
	void exportProject(Window* wnd, Renderer* rnd, int toExport);
};

/* ===========================================================================} */

#endif /* __WORKSPACE_H__ */
