/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "device_binjgb.h"
#include "document.h"
#include "editor_actor.h"
#include "editor_arbitrary.h"
#include "editor_code.h"
#include "editor_code_binding.h"
#include "editor_console.h"
#include "editor_font.h"
#include "editor_map.h"
#include "editor_music.h"
#include "editor_palette.h"
#include "editor_properties.h"
#include "editor_scene.h"
#include "editor_search_result.h"
#include "editor_sfx.h"
#include "editor_tiles.h"
#include "emulator.h"
#include "operations.h"
#include "theme.h"
#include "workspace.h"
#include "resource/inline_resource.h"
#include "../compiler/compiler.h"
#include "../utils/archive.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/filesystem.h"
#include "../utils/recorder.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#if defined GBBASIC_OS_HTML
#	include <emscripten.h>
#	include <emscripten/bind.h>
#endif /* GBBASIC_OS_HTML */

/*
** {===========================================================================
** Macros and constants
*/

#if !defined GBBASIC_OS_HTML
#	ifndef WORKSPACE_SPLASH_FILE
#		define WORKSPACE_SPLASH_FILE "../splash.png"
#	endif /* WORKSPACE_SPLASH_FILE */
#endif /* GBBASIC_OS_HTML */

#ifndef WORKSPACE_FADE_TIMEOUT_SECONDS
#	define WORKSPACE_FADE_TIMEOUT_SECONDS 2.0
#endif /* WORKSPACE_FADE_TIMEOUT_SECONDS */

#if !defined IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#	error "IMGUI_DISABLE_OBSOLETE_FUNCTIONS not defined."
#endif /* IMGUI_DISABLE_OBSOLETE_FUNCTIONS */

#ifndef WORKSPACE_MAIN_MENU_GUARD
#	define WORKSPACE_MAIN_MENU_GUARD(T) \
		ProcedureGuard<int> GBBASIC_UNIQUE_NAME(__SELECTIONGUARD__)( \
			[&] (void) -> int* { \
				return nullptr; \
			}, \
			[&] (int*) -> void { \
				ImGui::EndMenu(); \
			} \
		)
#endif /* WORKSPACE_MAIN_MENU_GUARD */

static_assert(sizeof(ImDrawIdx) == sizeof(unsigned int), "Wrong ImDrawIdx size.");

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

#if defined GBBASIC_OS_HTML
EMSCRIPTEN_BINDINGS(Categories) {
	emscripten::enum_<Workspace::Categories>("Categories")
		.value("PALETTE",  Workspace::Categories::PALETTE)
		.value("FONT",     Workspace::Categories::FONT)
		.value("CODE",     Workspace::Categories::CODE)
		.value("TILES",    Workspace::Categories::TILES)
		.value("MAP",      Workspace::Categories::MAP)
		.value("MUSIC",    Workspace::Categories::MUSIC)
		.value("SFX",      Workspace::Categories::SFX)
		.value("ACTOR",    Workspace::Categories::ACTOR)
		.value("SCENE",    Workspace::Categories::SCENE)
		.value("CONSOLE",  Workspace::Categories::CONSOLE)
		.value("EMULATOR", Workspace::Categories::EMULATOR)
		.value("HOME",     Workspace::Categories::HOME)
	;
};

EMSCRIPTEN_BINDINGS(ExternalEventTypes) {
	emscripten::enum_<Workspace::ExternalEventTypes>("ExternalEventTypes")
		.value("RESIZE_WINDOW", Workspace::ExternalEventTypes::RESIZE_WINDOW)
		.value("UNLOAD_WINDOW", Workspace::ExternalEventTypes::UNLOAD_WINDOW)
		.value("LOAD_PROJECT",  Workspace::ExternalEventTypes::LOAD_PROJECT)
		.value("PATCH_PROJECT", Workspace::ExternalEventTypes::PATCH_PROJECT)
		.value("INPUT_KEY",     Workspace::ExternalEventTypes::INPUT_KEY)
		.value("INPUT_TEXT",    Workspace::ExternalEventTypes::INPUT_TEXT)
		.value("TO_CATEGORY",   Workspace::ExternalEventTypes::TO_CATEGORY)
		.value("TO_PAGE",       Workspace::ExternalEventTypes::TO_PAGE)
		.value("TO_LOCATION",   Workspace::ExternalEventTypes::TO_LOCATION)
		.value("COMPILE",       Workspace::ExternalEventTypes::COMPILE)
		.value("RUN",           Workspace::ExternalEventTypes::RUN)
		.value("COUNT",         Workspace::ExternalEventTypes::COUNT)
	;
};

EM_JS(
	void, workspaceFree, (void* ptr), {
		_free(ptr);
	}
);
EM_JS(
	const char*, workspaceGetProjectData, (), {
		let ret = '';
		if (typeof getProjectData == 'function')
			ret = getProjectData();
		if (ret == null)
			ret = '';
		const lengthBytes = lengthBytesUTF8(ret) + 1;
		const stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes);

		return stringOnWasmHeap;
	}
);
EM_JS(
	const char*, workspaceGetInputText, (), {
		let ret = '';
		if (typeof getInputText == 'function')
			ret = getInputText();
		if (ret == null)
			ret = '';
		const lengthBytes = lengthBytesUTF8(ret) + 1;
		const stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes);

		return stringOnWasmHeap;
	}
);
#else /* GBBASIC_OS_HTML */
static void workspaceFree(void*) {
	// Do nothing.
}
static const char* workspaceGetProjectData(void) {
	return nullptr;
}
static const char* workspaceGetInputText(void) {
	return nullptr;
}
#endif /* GBBASIC_OS_HTML */

#if GBBASIC_SPLASH_ENABLED
#if defined GBBASIC_OS_HTML
EM_JS(
	bool, workspaceHasSplashImage, (), {
		if (typeof getSplashImage != 'function')
			return false;

		return true;
	}
);
EM_JS(
	const char*, workspaceGetSplashImage, (), {
		let ret = '';
		if (typeof getSplashImage == 'function')
			ret = getSplashImage();
		if (ret == null)
			ret = '';
		const lengthBytes = lengthBytesUTF8(ret) + 1;
		const stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes);

		return stringOnWasmHeap;
	}
);
static void workspaceSleep(int ms) {
	emscripten_sleep((unsigned)ms);
}
#else /* GBBASIC_OS_HTML */
static bool workspaceHasSplashImage(void) {
	return Path::fileExists(WORKSPACE_SPLASH_FILE);
}
static void workspaceSleep(int ms) {
	DateTime::sleep(ms);
}
#endif /* GBBASIC_OS_HTML */
static void workspaceCreateSplash(Window*, Renderer* rnd, Workspace* ws) {
	if (ws->splashGbbasic()) {
		ws->theme()->destroyTexture(rnd, ws->splashGbbasic(), nullptr);
		ws->splashGbbasic(nullptr);
	}

#if defined GBBASIC_OS_HTML
	const char* img = workspaceGetSplashImage();
	if (img) {
		Bytes::Ptr bytes(Bytes::create());
		if (Base64::toBytes(bytes.get(), img)) {
			ws->splashGbbasic(ws->theme()->createTexture(rnd, bytes->pointer(), bytes->count(), nullptr));
		}
		workspaceFree((void*)img);
	}
#else /* GBBASIC_OS_HTML */
	File::Ptr file(File::create());
	if (file->open(WORKSPACE_SPLASH_FILE, Stream::READ)) {
		Bytes::Ptr bytes(Bytes::create());
		file->readBytes(bytes.get());
		file->close();

		ws->splashGbbasic(ws->theme()->createTexture(rnd, bytes->pointer(), bytes->count(), nullptr));
	}
#endif /* GBBASIC_OS_HTML */
}
static void workspaceCreateSplash(Window*, Renderer* rnd, Workspace* ws, int index) {
	constexpr const Byte* const IMAGES[] = {
		RES_TOAST_GBBASIC
	};
	constexpr const size_t LENS[] = {
		GBBASIC_COUNTOF(RES_TOAST_GBBASIC)
	};

	if (ws->splashGbbasic()) {
		ws->theme()->destroyTexture(rnd, ws->splashGbbasic(), nullptr);
		ws->splashGbbasic(nullptr);
	}

	ws->splashGbbasic(ws->theme()->createTexture(rnd, IMAGES[index], LENS[index], nullptr));
}
static void workspaceRenderSplash(Window*, Renderer* rnd, Workspace* ws, std::function<void(Renderer*, Workspace*)> post) {
	Colour cls(0x2e, 0x32, 0x38, 0xff);
	if (ws->theme() && ws->theme()->style()) {
		const ImU32 integer = ws->theme()->style()->screenClearingColor;
		cls = Colour(integer & 0x000000ff, (integer >> 8) & 0x000000ff, (integer >> 16) & 0x000000ff, (integer >> 24) & 0x000000ff);
	}
	rnd->clear(&cls);

	if (ws->splashGbbasic()) {
		const Math::Recti dstGbbasic = Math::Recti::byXYWH(
			(rnd->width() - ws->splashGbbasic()->width()) / 2,
			(rnd->height() - ws->splashGbbasic()->height()) / 2,
			ws->splashGbbasic()->width(),
			ws->splashGbbasic()->height()
		);
		rnd->render(ws->splashGbbasic(), nullptr, &dstGbbasic, nullptr, nullptr, false, false, nullptr, false, false);
	}

	if (post)
		post(rnd, ws);

	rnd->flush();
}
#endif /* GBBASIC_SPLASH_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Workspace
*/

Workspace::CompilingParameters::CompilingParameters() {
}

Workspace::CompilingParameters::~CompilingParameters() {
}

Workspace::CompilingErrors::Entry::Entry() {
}

Workspace::CompilingErrors::Entry::Entry(std::string msg, bool isWarning_, int pg, int row_, int col) :
	message(msg),
	isWarning(isWarning_),
	page(pg),
	row(row_),
	column(col)
{
}

Workspace::CompilingErrors::Entry::Entry(std::string msg, bool isWarning_, AssetsBundle::Categories cat_, int pg) :
	message(msg),
	isWarning(isWarning_),
	category(cat_),
	page(pg)
{
}

bool Workspace::CompilingErrors::Entry::operator == (const Entry &other) const {
	return
		message == other.message &&
		isWarning == other.isWarning &&
		page == other.page &&
		row == other.row &&
		column == other.column;
}

Workspace::CompilingErrors::CompilingErrors() {
}

Workspace::CompilingErrors::~CompilingErrors() {
}

unsigned Workspace::CompilingErrors::type(void) const {
	return TYPE();
}

int Workspace::CompilingErrors::count(void) const {
	return (int)array.size();
}

void Workspace::CompilingErrors::add(const Entry &entry) {
	array.push_back(entry);
}

const Workspace::CompilingErrors::Entry* Workspace::CompilingErrors::get(int index) const {
	if (index < 0 || index >= (int)array.size())
		return nullptr;

	return &array[index];
}

int Workspace::CompilingErrors::indexOf(const Entry &entry) const {
	for (int i = 0; i < (int)array.size(); ++i) {
		const Entry &entry_ = array[i];
		if (entry == entry_)
			return i;
	}

	return -1;
}

Workspace::Delayed::Amount::Amount() {
}

Workspace::Delayed::Amount::Amount(Types y, long long d) : type(y), data(d) {
}

Workspace::Delayed::Delayed() {
}

Workspace::Delayed::Delayed(Handler handler_, const std::string &key_, const Amount &delay_) :
	handler(handler_),
	key(key_),
	delay(delay_)
{
}

Workspace::AssetPageNames::AssetPageNames() {
}

void Workspace::AssetPageNames::clear(void) {
	font.clear();
	code.clear();
	tiles.clear();
	map.clear();
	music.clear();
	sfx.clear();
	actor.clear();
	scene.clear();
}

Workspace::DebuggerMessages::DebuggerMessages() {
}

Workspace::DebuggerMessages::~DebuggerMessages() {
}

bool Workspace::DebuggerMessages::empty(void) const {
	return messages.empty();
}

void Workspace::DebuggerMessages::add(const std::string &txt) {
	messages.push_back(txt);

	if (messages.size() > 10) // Message count is limited up to 10.
		messages.pop_front();
	text.clear();
	Text::List::iterator it = messages.begin();
	for (int i = 0; i < (int)messages.size(); ++i) {
		const std::string &m = *it++;
		text += m;
		if (i != (int)messages.size() - 1)
			text += "\n";
	}
}

void Workspace::DebuggerMessages::clear(void) {
	messages.clear();
	text.clear();
}

Workspace::Filter::Filter() {
}

Workspace::Fadable::Fadable() {
}

Workspace::Workspace() {
	input(Input::create());

	workQueue(WorkQueue::create());

#if GBBASIC_COMPILER_ANALYZER_ENABLED
	staticAnalyzer(
		GBBASIC::StaticAnalyzer::Ptr(
			GBBASIC::StaticAnalyzer::create(
				std::bind(&Workspace::async, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			)
		)
	);
#endif /* GBBASIC_COMPILER_ANALYZER_ENABLED */

	theme(new Theme());

	EditorConsole* editor = EditorConsole::create();
	consoleTextBox(editor);
}

Workspace::~Workspace() {
	EditorConsole* editor = (EditorConsole*)consoleTextBox();
	EditorConsole::destroy(editor);
	consoleTextBox(nullptr);

	delete theme();
	theme(nullptr);

	WorkQueue::destroy(workQueue());
	workQueue(nullptr);

#if GBBASIC_COMPILER_ANALYZER_ENABLED
	clearAnalyzingResult();
	staticAnalyzer(nullptr);
#endif /* GBBASIC_COMPILER_ANALYZER_ENABLED */

	Input::destroy(input());
	input(nullptr);
}

bool Workspace::open(Window* wnd, Renderer* rnd, const char* font, unsigned fps, bool showRecent, bool hideSplashImage, bool forceWritable_, const char* toUpgrade, bool toCompile) {
	// Prepare.
	if (_opened)
		return false;
	_opened = true;

	Platform::threadName("GB BASIC");

	_toUpgrade = !!toUpgrade;
	if (toUpgrade)
		_toUpgradeToVersion = toUpgrade;
	_toCompile = toCompile;

	// Begin the splash.
	beginSplash(wnd, rnd, hideSplashImage);

	// Setup ImGui.
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowBorderSize = 0;
	style.ChildBorderSize = 0;
	style.ScrollbarRounding = 0;
	style.TabRounding = 0;

	// Initialize the theme.
	theme()->open(rnd);
	theme()->load(rnd);

	// Initialize the input module.
	input()->open();

	// Initialize the work queue module.
	workQueue()->startup("WORKSPACE", 2, true);

	// Initialize the properties.
	_state = States::IDLE;

	busy(false);

	activeFrameRate(fps);

	showRecentProjects(showRecent);
	category(Categories::HOME);
	categoryOfAudio(Categories::MUSIC);
	categoryBeforeCompiling(Categories::HOME);
	interactable(true);
	toRun(false);
	toExport(-1);
	forceWritable(forceWritable_);
	exampleRevision(1);
	fontConfig(font ? font : WORKSPACE_FONT_DEFAULT_CONFIG_FILE);
#if defined GBBASIC_OS_HTML
	loadingProject(false);
#endif /* Platform macro. */

	needAnalyzing(false);

	splashCustomized(false);

	activeKernelIndex(-1);
	hasDefaultKernelSourceCode(false);

	exampleCount(0);

	musicCount(0);

	sfxCount(0);

	isFocused(true);
	menuHeight(0.0f);
	menuOpened(false);
	tabsWidth(0.0f);
	statusWidth(0.0f);
#if WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED
	borderMouseDown(false);
	ignoreBorderMouseResizing(false);
	headBarMouseDown(false);
	ignoreHeadBarMouseMoving(false);
#endif /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */

	recentProjectItemHeight(0.0f);
	recentProjectItemIconWidth(0.0f);
	recentProjectFocusedIndex(-1);
	recentProjectFocusingIndex(-1);
	recentProjectFocusableCount(0);
	recentProjectSelectingIndex(-1);
	recentProjectSelectedIndex(-1);
	recentProjectSelectedTimestamp(0.0);
	recentProjectWithContextIndex(-1);
	recentProjectsColumnCount(Math::max(rnd->width() / 155, 2));
	recentProjectsFilterFocused(false);

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	codeEditorMinorWidth(0.0f);
	codeEditorMinorResizing(false);
	codeEditorMinorResetting(false);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	canvasFixRatio(true);
	canvasIntegerScale(true);
	canvasHovering(false);
	canvasCursorMode(Device::CursorTypes::POINTER);

	// Initialize the console.
	EditorConsole* editor = (EditorConsole*)consoleTextBox();
	editor->open(wnd, rnd, this, nullptr, -1, (unsigned)(~0), -1);
	consoleVisible(true);
	consoleHasWarning(false);
	consoleHasError(false);

	// Initialize the search result.
	searchResultTextBox(nullptr);
	searchResultVisible(false);
	searchResultHeight(0.0f);
	searchResultResizing(false);

	// Config the recorder.
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
	recorder(
		Recorder::create(
			input(),
			fps,
			[&] (void) -> promise::Promise {
				return Operations::popupWait(
					wnd, rnd, this,
					theme()->dialogPrompt_Writing().c_str()
				);
			},
			std::bind(&Workspace::print, this, std::placeholders::_1)
		)
	);
#endif /* Platform macro. */

	// Load kernels.
	loadKernels();

	// Load exporters.
	loadExporters();

	// Load examples.
	loadExamples(wnd, rnd);

	// Load starter kits.
	loadStarterKits(wnd, rnd);

	// Load music.
	loadMusic();

	// Load SFX.
	loadSfx();

	// Load documents.
	loadDocuments();

	// Load links.
	loadLinks();

	// End the splash.
	endSplash(wnd, rnd, hideSplashImage);

	// Popup a notice for file dialog requirements if needed.
#if defined GBBASIC_OS_LINUX
	const bool hasFileDialog = Platform::checkProgram("zenity") || Platform::checkProgram("kdialog") || Platform::checkProgram("matedialog") || Platform::checkProgram("qarma");
	if (!hasFileDialog) {
		const std::string &msg_ = theme()->dialogPrompt_LinuxFileDialogRequirements();
		messagePopupBox(
			msg_,
			nullptr,
			nullptr,
			nullptr
		);
	}
#endif /* GBBASIC_OS_LINUX */

	// Finish.
	fprintf(stdout, "Workspace opened.\n");

	return true;
}

bool Workspace::close(Window*, Renderer* rnd) {
	// Prepare.
	if (!_opened)
		return false;
	_opened = false;

	// Dispose the recorder.
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
	Recorder::destroy(recorder());
	recorder(nullptr);
#endif /* Platform macro. */

	// Close the search result.
	closeSearchResult();

	// Dispose the dialog box if there is any.
	popupBox(nullptr);

	// Unload links.
	unloadLinks();

	// Unload documents.
	unloadDocuments();

	// Unload SFX.
	unloadSfx();

	// Unload music.
	unloadMusic();

	// Unload starter kits.
	unloadStarterKits();

	// Unload examples.
	unloadExamples();

	// Unload exporters.
	unloadExporters();

	// Unload kernels.
	unloadKernels();

	// Dispose the console.
	EditorConsole* editor = (EditorConsole*)consoleTextBox();
	editor->close(-1);

	// Dispose the document.
	if (document()) {
		Document::destroy(document());
		document(nullptr);
	}

	// Dispose the minor code editor if there is any.
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	destroyMinorCodeEditor();
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	// Dispose the projects.
	if (showRecentProjects()) {
		for (int i = 0; i < (int)projects().size(); ++i) {
			Project::Ptr &prj = projects()[i];
			prj->close(true);
		}
	} else {
		Project::Ptr &prj = currentProject();
		if (prj) {
			prj->close(true);
			currentProject(nullptr);
		}
	}

	// Dispose the work queue module.
	workQueue()->shutdown();

	// Dispose the input module.
	input()->close();

	// Dispose the theme.
	theme()->save();
	theme()->close(rnd);

	// Finish.
	fprintf(stdout, "Workspace closed.\n");

	return true;
}

Workspace::States Workspace::state(void) const {
	return _state;
}

bool Workspace::canUseShortcuts(void) const {
	if (popupBox())
		return false;

	if (_toUpgrade || _toCompile)
		return false;

	if (_state == States::COMPILING)
		return false;

	if (analyzing())
		return false;

	return true;
}

bool Workspace::canWriteProjectTo(const char* path) const {
	if (!forceWritable()) {
#if !WORKSPACE_EXAMPLE_PROJECTS_WRITABLE
		const std::string absdir0 = Path::absoluteOf(WORKSPACE_EXAMPLE_PROJECTS_DIR);
		const std::string abspath0 = Path::absoluteOf(path);
		if (Path::isParentOf(absdir0.c_str(), abspath0.c_str()))
			return false;
#endif /* WORKSPACE_EXAMPLE_PROJECTS_WRITABLE */

#if !WORKSPACE_STARTER_KITS_PROJECTS_WRITABLE
		const std::string absdir1 = Path::absoluteOf(WORKSPACE_STARTER_KITS_PROJECTS_DIR);
		const std::string abspath1 = Path::absoluteOf(path);
		if (Path::isParentOf(absdir1.c_str(), abspath1.c_str()))
			return false;

#	if WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED
		const std::string altPath2 = Path::combine(WORKSPACE_ALTERNATIVE_ROOT_PATH, WORKSPACE_STARTER_KITS_PROJECTS_DIR);
		const std::string absdir2 = Path::absoluteOf(altPath2);
		const std::string abspath2 = Path::absoluteOf(path);
		if (Path::isParentOf(absdir2.c_str(), abspath2.c_str()))
			return false;
#	endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED */
#endif /* WORKSPACE_STARTER_KITS_PROJECTS_WRITABLE */
	}

	std::string dir;
	Path::split(path, nullptr, nullptr, &dir);

	return Path::isDirectoryWritable(dir.c_str());
}

bool Workspace::canWriteSramTo(const char* path) const {
	std::string dir;
	Path::split(path, nullptr, nullptr, &dir);

	return Path::isDirectoryWritable(dir.c_str());
}

bool Workspace::isUnderExamplesDirectory(const char* path) const {
	const std::string absdir = Path::absoluteOf(WORKSPACE_EXAMPLE_PROJECTS_DIR);
	const std::string abspath = Path::absoluteOf(path);
	if (Path::isParentOf(absdir.c_str(), abspath.c_str()))
		return true;

	return false;
}

bool Workspace::isUnderKitsDirectory(const char* path) const {
	const std::string absdir1 = Path::absoluteOf(WORKSPACE_STARTER_KITS_PROJECTS_DIR);
	const std::string abspath1 = Path::absoluteOf(path);
	if (Path::isParentOf(absdir1.c_str(), abspath1.c_str()))
		return true;

#if WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED
	const std::string altPath2 = Path::combine(WORKSPACE_ALTERNATIVE_ROOT_PATH, WORKSPACE_STARTER_KITS_PROJECTS_DIR);
	const std::string absdir2 = Path::absoluteOf(altPath2);
	const std::string abspath2 = Path::absoluteOf(path);
	if (Path::isParentOf(absdir2.c_str(), abspath2.c_str()))
		return true;
#endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED */

	return false;
}

Mutex &Workspace::lock(void) {
	return _lock;
}

void Workspace::refreshWindowTitle(Window* wnd) {
	const Project::Ptr &prj = currentProject();
	if (prj) {
		std::string wndTitle = std::string(GBBASIC_TITLE " v" GBBASIC_VERSION_STRING);
		if (settings().mainShowProjectPathAtTitleBar && !prj->path().empty())
			wndTitle += std::string("  [") + prj->title() + " - " + prj->path() + std::string("]");
		else
			wndTitle += std::string("  [") + prj->title() + std::string("]");
		wnd->title(wndTitle.c_str());
	} else {
		wnd->title(GBBASIC_TITLE " v" GBBASIC_VERSION_STRING);
	}
}

bool Workspace::skipping(void) {
	if (_skipping <= 0)
		return false;

	--_skipping;

	return true;
}

void Workspace::skipFrame(int n) {
	_skipping = n;
}

int Workspace::getKernelIndex(const std::string &id) const {
	for (int i = 0; i < (int)kernels().size(); ++i) {
		const GBBASIC::Kernel::Ptr &krnl = kernels()[i];
		if (krnl && krnl->id() == id)
			return i;
	}

	return -1;
}

const std::string* Workspace::getKernelId(int index) const {
	if (index < 0 || index >= (int)kernels().size())
		return nullptr;

	const GBBASIC::Kernel::Ptr &krnl = kernels()[index];

	return &krnl->id();
}

int Workspace::getValidKernelIndex(const Project* prj) const {
	if (!prj)
		return 0;

	const std::string &kernelId = prj->kernel();
	const int kernelIdx = getKernelIndex(kernelId);
	if (kernelIdx < 0)
		return 0;

	const int n = (int)kernels().size();

	return Math::clamp(kernelIdx, 0, n - 1);
}

int Workspace::getKernelUsedCountByProjects(int index) const {
	Project::Array allProjects;
	if (showRecentProjects()) {
		allProjects = projects();
	} else {
		const Project::Ptr &prj = currentProject();
		if (prj)
			allProjects.push_back(prj);
	}

	if (allProjects.empty())
		return 0;

	if (index < 0 || index >= (int)kernels().size())
		return 0;

	const GBBASIC::Kernel::Ptr &krnl = kernels()[index];
	const std::string &id = krnl->id();

	int result = 0;
	for (int i = 0; i < (int)allProjects.size(); ++i) {
		const Project::Ptr &prj = allProjects[i];
		if (prj->kernel() == id)
			++result;
	}

	return result;
}

GBBASIC::Kernel::Ptr Workspace::activeKernel(void) const {
	const int krnlIndex = activeKernelIndex();
	if (krnlIndex < 0 || krnlIndex >= (int)kernels().size())
		return nullptr;

	GBBASIC::Kernel::Ptr krnl = kernels()[krnlIndex];

	return krnl;
}

std::string Workspace::serializeKernelBehaviour(int val) const {
	const int krnlIndex = activeKernelIndex();
	if (krnlIndex < 0 || krnlIndex >= (int)kernels().size())
		return "none";

	const GBBASIC::Kernel::Ptr &krnl = kernels()[krnlIndex];
	if (!krnl)
		return "none";

	const GBBASIC::Kernel::Behaviour::Array &behaviours = krnl->behaviours();
	for (const GBBASIC::Kernel::Behaviour &bhvr : behaviours) {
		if (bhvr.value == val)
			return bhvr.id;
	}

	return "none";
}

int Workspace::parseKernelBehaviour(const std::string &id) const {
	const int krnlIndex = activeKernelIndex();
	if (krnlIndex < 0 || krnlIndex >= (int)kernels().size())
		return 0;

	const GBBASIC::Kernel::Ptr &krnl = kernels()[krnlIndex];
	if (!krnl)
		return 0;

	const GBBASIC::Kernel::Behaviour::Array &behaviours = krnl->behaviours();
	for (const GBBASIC::Kernel::Behaviour &bhvr : behaviours) {
		if (bhvr.id == id)
			return bhvr.value;
	}

	return 0;
}

void Workspace::reloadKernels(void) {
	unloadKernels();
	loadKernels();
}

void Workspace::addMapPageFrom(Window* wnd, Renderer* rnd, int index) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	prj->preferencesMapRef(index);

	prj->hasDirtyAsset(true);

	category(Categories::MAP);

	Operations::mapAddPage(wnd, rnd, this);
}

void Workspace::addScenePageFrom(Window* wnd, Renderer* rnd, int index) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	prj->preferencesSceneRefMap(index);

	prj->hasDirtyAsset(true);

	category(Categories::SCENE);

	Operations::sceneAddPage(wnd, rnd, this);
}

void Workspace::duplicateFontFrom(Window* wnd, Renderer* rnd, int index) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	Operations::fontDuplicatePage(wnd, rnd, this, index);
}

void Workspace::category(const Categories &category) {
	if (_category == category)
		return;

	withCurrentAsset(
		[this] (Categories, BaseAssets::Entry*, Editable* editor) -> void {
			editor->leave(this);
		}
	);

	if (_category != Categories::CONSOLE && category == Categories::CONSOLE) {
		EditorConsole* editor = (EditorConsole*)consoleTextBox();
		if (editor)
			editor->enter(this);
	} else if (_category == Categories::CONSOLE && category != Categories::CONSOLE) {
		EditorConsole* editor = (EditorConsole*)consoleTextBox();
		if (editor)
			editor->leave(this);
	}

	_category = category;

	withCurrentAsset(
		[this] (Categories, BaseAssets::Entry*, Editable* editor) -> void {
			editor->enter(this);
		}
	);

	switch (_category) {
	case Categories::MUSIC: // Fall through.
	case Categories::SFX:
		categoryOfAudio(_category);

		break;
	default: // Do nothing.
		break;
	}

	switch (_category) {
	case Categories::PALETTE: // Fall through.
	case Categories::CONSOLE: // Fall through.
	case Categories::EMULATOR: // Fall through.
	case Categories::HOME: // Do nothing.
		break;
	default:
		categoryBeforeCompiling(_category);

		break;
	}
}

bool Workspace::changePage(Window*, Renderer*, Project* prj, Categories category, int index) {
	bool result = false;

	withCurrentAsset(
		[this] (Categories, BaseAssets::Entry*, Editable* editor) -> void {
			editor->leave(this);
		}
	);

	switch (category) {
	case Categories::PALETTE:
		if (index < 0 || index >= prj->palettePageCount())
			break;

		prj->activePaletteIndex(index);

		result = true;

		break;
	case Categories::FONT:
		if (index < 0 || index >= prj->fontPageCount())
			break;

		prj->activeFontIndex(index);

		result = true;

		break;
	case Categories::CODE:
		if (index < 0 || index >= prj->codePageCount())
			break;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (prj->isMajorCodeEditorActive())
			prj->activeMajorCodeIndex(index);
		else
			prj->activeMinorCodeIndex(index);
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		prj->activeMajorCodeIndex(index);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		result = true;

		break;
	case Categories::TILES:
		if (index < 0 || index >= prj->tilesPageCount())
			break;

		prj->activeTilesIndex(index);

		result = true;

		break;
	case Categories::MAP:
		if (index < 0 || index >= prj->mapPageCount())
			break;

		prj->activeMapIndex(index);

		result = true;

		break;
	case Categories::MUSIC:
		if (index < 0 || index >= prj->musicPageCount())
			break;

		prj->activeMusicIndex(index);

		result = true;

		break;
	case Categories::SFX:
		if (index < 0 || index >= prj->sfxPageCount())
			break;

		prj->activeSfxIndex(index);

		result = true;

		break;
	case Categories::ACTOR:
		if (index < 0 || index >= prj->actorPageCount())
			break;

		prj->activeActorIndex(index);

		result = true;

		break;
	case Categories::SCENE:
		if (index < 0 || index >= prj->scenePageCount())
			break;

		prj->activeSceneIndex(index);

		result = true;

		break;
	default: // Do nothing.
		break;
	}

	withCurrentAsset(
		[this] (Categories, BaseAssets::Entry*, Editable* editor) -> void {
			editor->enter(this);
		}
	);

	return result;
}

void Workspace::pageAdded(Window* wnd, Renderer* rnd, Project* prj, Categories category) {
	typedef std::function<void(Categories)> PageUpdater;

	PageUpdater updatePage = nullptr;
	updatePage = [wnd, rnd, prj, this, &updatePage] (Categories category) -> void {
		switch (category) {
		case Categories::PALETTE:
			// Do nothing.

			break;
		case Categories::FONT:
			changePage(wnd, rnd, prj, Categories::FONT, prj->fontPageCount() - 1);

			break;
		case Categories::CODE:
			changePage(wnd, rnd, prj, Categories::CODE, prj->codePageCount() - 1);

			break;
		case Categories::TILES:
			changePage(wnd, rnd, prj, Categories::TILES, prj->tilesPageCount() - 1);

			break;
		case Categories::MAP:
			changePage(wnd, rnd, prj, Categories::MAP, prj->mapPageCount() - 1);

			break;
		case Categories::MUSIC:
			changePage(wnd, rnd, prj, Categories::MUSIC, prj->musicPageCount() - 1);

			break;
		case Categories::SFX:
			changePage(wnd, rnd, prj, Categories::SFX, prj->sfxPageCount() - 1);

			break;
		case Categories::ACTOR:
			changePage(wnd, rnd, prj, Categories::ACTOR, prj->actorPageCount() - 1);

			break;
		case Categories::SCENE:
			changePage(wnd, rnd, prj, Categories::SCENE, prj->scenePageCount() - 1);

			break;
		default: // Do nothing.
			break;
		}
	};

	updatePage(category);

	bubble(theme()->dialogPrompt_AddedAssetPage(), nullptr);
}

void Workspace::pageRemoved(Window* wnd, Renderer* rnd, Project* prj, Categories category, int index) {
	typedef std::function<void(Categories, int)> PageUpdater;

	PageUpdater updatePage = nullptr;
	updatePage = [wnd, rnd, prj, this, &updatePage] (Categories category, int index) -> void {
		switch (category) {
		case Categories::PALETTE:
			// Do nothing.

			break;
		case Categories::FONT:
			if (prj->fontPageCount() > 0) {
				for (int i = index; i < prj->fontPageCount(); ++i) {
					FontAssets::Entry* entry = prj->getFont(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->fontPageCount() - 1);
				changePage(wnd, rnd, prj, Categories::FONT, page_);
			}

			break;
		case Categories::CODE:
			if (prj->codePageCount() > 0) {
				for (int i = index; i < prj->codePageCount(); ++i) {
					CodeAssets::Entry* entry = prj->getCode(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->codePageCount() - 1);
				changePage(wnd, rnd, prj, Categories::CODE, page_);
			}

			break;
		case Categories::TILES:
			if (prj->tilesPageCount() > 0) {
				for (int i = index; i < prj->tilesPageCount(); ++i) {
					TilesAssets::Entry* entry = prj->getTiles(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->tilesPageCount() - 1);
				changePage(wnd, rnd, prj, Categories::TILES, page_);
			}

			for (int i = 0; i < prj->mapPageCount(); ++i) {
				MapAssets::Entry* entry = prj->getMap(i);
				if (entry->ref == index) {
					entry->ref = -1;
					entry->cleanup();

					updatePage(Categories::MAP, i);
				} else if (entry->ref > index) {
					--entry->ref;
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_REF_INDEX, this, (Variant::Int)entry->ref);
				}
			}

			break;
		case Categories::MAP:
			if (prj->mapPageCount() > 0) {
				for (int i = index; i < prj->mapPageCount(); ++i) {
					MapAssets::Entry* entry = prj->getMap(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->mapPageCount() - 1);
				changePage(wnd, rnd, prj, Categories::MAP, page_);
			}

			for (int i = 0; i < prj->scenePageCount(); ++i) {
				SceneAssets::Entry* entry = prj->getScene(i);
				if (entry->refMap == index) {
					entry->refMap = -1;
					entry->cleanup();

					updatePage(Categories::SCENE, i);
				} else if (entry->refMap > index) {
					--entry->refMap;
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_REF_INDEX, this, (Variant::Int)entry->refMap);
				}
			}

			break;
		case Categories::MUSIC:
			if (prj->musicPageCount() > 0) {
				for (int i = index; i < prj->musicPageCount(); ++i) {
					MusicAssets::Entry* entry = prj->getMusic(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->musicPageCount() - 1);
				changePage(wnd, rnd, prj, Categories::MUSIC, page_);
			}

			break;
		case Categories::SFX:
			if (prj->sfxPageCount() > 0) {
				const int index_ = Math::clamp(prj->activeSfxIndex(), 0, prj->sfxPageCount() - 1);
				EditorSfx* editor = prj->sharedSfxEditor();
				if (editor)
					editor->post(Editable::UPDATE_INDEX, (Variant::Int)index_);

				const int page_ = Math::clamp(index, 0, prj->sfxPageCount() - 1);
				changePage(wnd, rnd, prj, Categories::SFX, page_);
			}

			break;
		case Categories::ACTOR:
			if (prj->actorPageCount() > 0) {
				for (int i = index; i < prj->actorPageCount(); ++i) {
					ActorAssets::Entry* entry = prj->getActor(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->actorPageCount() - 1);
				changePage(wnd, rnd, prj, Categories::ACTOR, page_);
			}

			for (int i = 0; i < prj->scenePageCount(); ++i) {
				SceneAssets::Entry* entry = prj->getScene(i);
				SceneAssets::Entry::UniqueRef uref;
				const SceneAssets::Entry::Ref refActors_ = entry->getRefActors(&uref);
				(void)refActors_;
				entry->updateRefActors(index);
				SceneAssets::Entry::UniqueRef::iterator it = uref.find(index);
				if (it != uref.end()) {
					entry->cleanup();

					updatePage(Categories::SCENE, i);
				}
			}

			break;
		case Categories::SCENE:
			if (prj->scenePageCount() > 0) {
				for (int i = index; i < prj->scenePageCount(); ++i) {
					SceneAssets::Entry* entry = prj->getScene(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::UPDATE_INDEX, (Variant::Int)i);
				}

				const int page_ = Math::clamp(index, 0, prj->scenePageCount() - 1);
				changePage(wnd, rnd, prj, Categories::SCENE, page_);
			}

			break;
		default: // Do nothing.
			break;
		}
	};

	updatePage(category, index);

	bubble(theme()->dialogPrompt_RemovedAssetPage(), nullptr);
}

bool Workspace::load(Window* wnd, Renderer* rnd, const int* wndWidth, const int* wndHeight) {
	// Prepare.
	struct ProjectInfo {
		typedef std::vector<ProjectInfo> Array;

		int index = 0;
		std::string path;

		ProjectInfo() {
		}
	};

	const bool loadEg = !_toUpgrade && !_toCompile && showRecentProjects();

	// Load the example configuration from file.
	do {
		rapidjson::Document doc;
		File::Ptr file(File::create());
		if (file->open(WORKSPACE_EXAMPLE_PROJECTS_CONFIG_FILE, Stream::READ)) {
			std::string buf;
			file->readString(buf);
			Json::fromString(doc, buf.c_str());

			unsigned revision = 0;
			if (Jpath::get(doc, revision, "revision"))
				exampleRevision(revision);
			else
				exampleRevision(1);

			file->close();
		}
		file = nullptr;
	} while (false);

	// Read configuration from the file.
	rapidjson::Document doc;
	doc.SetNull();
	do {
		const std::string pref = Path::writableDirectory();
		const std::string path = Path::combine(pref.c_str(), WORKSPACE_PREFERENCES_NAME ".json");

		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ))
			break;

		std::string buf;
		file->readString(buf);
		file->close();
		if (!Json::fromString(doc, buf.c_str())) {
			doc.SetNull();

			break;
		}
		file = nullptr;
	} while (false);

	// Parse the configuration.
	loadConfig(wnd, rnd, doc);

	// Read and parse persistence data.
	do {
		const std::string pref = Path::writableDirectory();
		const std::string path = Path::combine(pref.c_str(), WORKSPACE_ACTIVITIES_NAME ".dat");

		activities().fromFile(path.c_str());
	} while (false);

	// Validate the projects.
	ProjectInfo::Array regularProjects;
	const std::string egDir = Path::absoluteOf(WORKSPACE_EXAMPLE_PROJECTS_DIR);
	do {
		if (!loadEg)
			break;

		const int n = Jpath::count(doc, "projects");
		for (int i = 0; i < n; ++i) {
			std::string path_;
			if (!Jpath::get(doc, path_, "projects", i, "path"))
				continue;

			const std::string abspath = Path::absoluteOf(path_);
			const bool isEg = Path::isParentOf(egDir.c_str(), abspath.c_str());

			ProjectInfo info;
			info.index = i;
			info.path = path_;
			if (!isEg)
				regularProjects.push_back(info);
		}
	} while (false);

	bool reloadExamples = false;
	if (loadEg) {
		if (settings().applicationFirstRun) {
			settings().applicationFirstRun = false;
			reloadExamples = true;
		} else if (Jpath::count(doc, "projects") == 0) {
			// Do nothing.
		} else if (settings().applicationLoadedExampleRevision < exampleRevision()) {
			reloadExamples = true;
		}
	}

	// Reload the example projects if necessary.
	if (reloadExamples) {
		// Clear the projects.
		Jpath::clear(doc, "projects");

		// Reload the regular projects.
		for (int i = 0; i < (int)regularProjects.size(); ++i) {
			const ProjectInfo &info = regularProjects[i];
			Jpath::set(doc, doc, info.path, "projects", i, "path");
		}

		// Reload the example projects.
		DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(WORKSPACE_EXAMPLE_PROJECTS_DIR);
		FileInfos::Ptr fileInfos = dirInfo->getFiles("*." GBBASIC_RICH_PROJECT_EXT ";" "*." GBBASIC_PLAIN_PROJECT_EXT, true);
		for (int i = 0; i < fileInfos->count(); ++i) {
			FileInfo::Ptr fileInfo = fileInfos->get(i);

			std::string path = fileInfo->fullPath();
			path = Path::absoluteOf(path);
			const int i_ = i + (int)regularProjects.size();
			Jpath::set(doc, doc, path, "projects", i_, "path");
		}

		// Set the example revision.
		settings().applicationLoadedExampleRevision = exampleRevision();
	}

	// Parse the projects.
	if (loadEg) {
		const int n = Jpath::count(doc, "projects");
		for (int i = 0; i < n; ++i) {
			Project::ContentTypes contentType;
			std::string ct;
			Jpath::get(doc, ct, "projects", i, "content_type");
			if      (ct == "basic") contentType = Project::ContentTypes::BASIC;
			else if (ct == "rom")   contentType = Project::ContentTypes::ROM;
			else                    contentType = Project::ContentTypes::BASIC;

			std::string path_;
			if (!Jpath::get(doc, path_, "projects", i, "path"))
				continue;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			int majorCodeIndex = 0;
			if (!Jpath::get(doc, majorCodeIndex, "projects", i, "major_code_index"))
				majorCodeIndex = 0;

			int minorCodeIndex = -1;
			if (!Jpath::get(doc, minorCodeIndex, "projects", i, "minor_code_index"))
				minorCodeIndex = -1;

			bool minorCodeEditorEnabled = false;
			if (!Jpath::get(doc, minorCodeEditorEnabled, "projects", i, "minor_code_editor_enabled"))
				minorCodeEditorEnabled = false;

			float minorCodeEditorWidth = 0.0f;
			if (!Jpath::get(doc, minorCodeEditorWidth, "projects", i, "minor_code_editor_width"))
				minorCodeEditorWidth = 0.0f;
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

			float fontPreviewHeight = 0.0f;
			if (!Jpath::get(doc, fontPreviewHeight, "projects", i, "font_preview_height"))
				fontPreviewHeight = 0.0f;

			Operations::fileAdd(wnd, rnd, this, path_.c_str());

			const std::string abspath = Path::absoluteOf(path_);
			const bool isEg = Path::isParentOf(egDir.c_str(), abspath.c_str());
			Project::Ptr &prj = projects()[i];
			if (isEg)
				prj->isExample(true);
			prj->contentType(contentType);
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			prj->activeMajorCodeIndex(majorCodeIndex);
			prj->isMinorCodeEditorEnabled(minorCodeEditorEnabled);
			prj->activeMinorCodeIndex(minorCodeIndex);
			prj->minorCodeEditorWidth(minorCodeEditorWidth);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			prj->fontPreviewHeight(fontPreviewHeight);
		}
	}

	// Sort the projects.
	if (loadEg) {
		sortProjects();
	}

	// Setup the configuration.
	if (wndWidth && wndHeight)
		settings().applicationWindowSize = Math::Vec2i(*wndWidth, *wndHeight);
	const Math::Vec2i size = wnd->size();
	if (settings().applicationWindowSize == Math::Vec2i(0, 0))
		settings().applicationWindowSize = size;
#if !defined GBBASIC_OS_HTML
	if (!settings().applicationWindowFullscreen && !settings().applicationWindowMaximized) {
		if (size != settings().applicationWindowSize)
			wnd->size(settings().applicationWindowSize);
	}
#endif /* GBBASIC_OS_HTML */
#if defined GBBASIC_OS_MAC
	wnd->displayIndex(settings().applicationWindowDisplayIndex);
	if (settings().applicationWindowFullscreen)
		wnd->fullscreen(true);
	else if (settings().applicationWindowMaximized)
		wnd->maximize();
	else
		wnd->position(settings().applicationWindowPosition);
#endif /* GBBASIC_OS_MAC */

	previousRendererSize(Math::Vec2i(rnd->width(), rnd->height()));

	canvasFixRatio(settings().canvasFixRatio);
	canvasIntegerScale(settings().canvasIntegerScale);

	input()->config(settings().inputGamepads, INPUT_GAMEPAD_COUNT);

	// Finish.
	return true;
}

bool Workspace::save(Window* wnd, Renderer* rnd) {
	// Prepare.
	const bool loadEg = !_toUpgrade && !_toCompile && showRecentProjects();

	rapidjson::Document doc;

	// Get the configuration.
	settings().applicationWindowDisplayIndex = wnd->displayIndex();

	// Serialize the configuration.
	saveConfig(wnd, rnd, doc);

	// Serialize the projects.
	if (loadEg) {
		for (int i = 0; i < (int)projects().size(); ++i) {
			const Project::Ptr &prj = projects()[i];

			std::string ct;
			switch (prj->contentType()) {
			case Project::ContentTypes::BASIC: ct = "basic"; break;
			case Project::ContentTypes::ROM:   ct = "rom";   break;
			default:                           ct = "basic"; break;
			}
			Jpath::set(doc, doc, ct, "projects", i, "content_type");

			Jpath::set(doc, doc, prj->path(), "projects", i, "path");

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			Jpath::set(doc, doc, prj->activeMajorCodeIndex(), "projects", i, "major_code_index");

			Jpath::set(doc, doc, prj->isMinorCodeEditorEnabled(), "projects", i, "minor_code_editor_enabled");

			Jpath::set(doc, doc, prj->activeMinorCodeIndex(), "projects", i, "minor_code_index");

			Jpath::set(doc, doc, prj->minorCodeEditorWidth(), "projects", i, "minor_code_editor_width");
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

			Jpath::set(doc, doc, prj->fontPreviewHeight(), "projects", i, "font_preview_height");
		}
	} else {
		rapidjson::Document doc_;
		doc_.SetNull();
		do {
			const std::string pref = Path::writableDirectory();
			const std::string path = Path::combine(pref.c_str(), WORKSPACE_PREFERENCES_NAME ".json");

			File::Ptr file(File::create());
			if (!file->open(path.c_str(), Stream::READ))
				break;
			std::string buf;
			if (!file->readString(buf)) {
				file->close();

				break;
			}
			file->close();
			if (!Json::fromString(doc_, buf.c_str()))
				break;

			const rapidjson::Value* projects = nullptr;
			if (!Jpath::get(doc_, projects, "projects"))
				break;

			Jpath::set(doc, doc, projects, "projects");
		} while (false);
	}

	// Save configuration to file.
	do {
		const std::string pref = Path::writableDirectory();
		const std::string path = Path::combine(pref.c_str(), WORKSPACE_PREFERENCES_NAME ".json");

		File::Ptr file(File::create());
		if (file->open(path.c_str(), Stream::WRITE)) {
			std::string buf;
			Json::toString(doc, buf);
			file->writeString(buf);

			file->close();
		}
		file = nullptr;
	} while (false);

	// Save persistence data.
	do {
		const std::string pref = Path::writableDirectory();
		const std::string path = Path::combine(pref.c_str(), WORKSPACE_ACTIVITIES_NAME ".dat");

		activities().toFile(path.c_str());
	} while (false);

	// Finish.
	return true;
}

bool Workspace::update(Window* wnd, Renderer* rnd, double delta, unsigned fps, unsigned* fpsReq, bool* indicated) {
	// Prepare.
	bool result = false;

	// Execute.
	result = execute(wnd, rnd, delta, fpsReq);

	// Perform.
	perform(wnd, rnd, delta, fpsReq, nullptr);

	// Begin.
	prepare(wnd, rnd);

	shortcuts(wnd, rnd);

	// Dialog boxes.
	dialog(wnd, rnd);

	// Head.
	head(wnd, rnd, delta, fpsReq);

	// Body.
	body(wnd, rnd, delta, fps, indicated);

	// Search result.
	found(wnd, rnd, delta);

	// End.
	finish(wnd, rnd);

	// Finish.
	return result;
}

void Workspace::record(Window* wnd, Renderer* rnd) {
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
	if (!recorder()->recording())
		return;

	recorder()->update(wnd, rnd);
#else /* Platform macro. */
	(void)wnd;
	(void)rnd;
#endif /* Platform macro. */
}

void Workspace::clear(void) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

	const bool withConsole = consoleVisible();
	if (withConsole) {
		EditorConsole* editor = (EditorConsole*)consoleTextBox();
		editor->fromString("", 0);
	}
}

bool Workspace::print(const char* msg) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

	const bool withConsole = consoleVisible();
	if (withConsole) {
		EditorConsole* editor = (EditorConsole*)consoleTextBox();
		editor->appendText(msg, theme()->style()->messageColor);
		editor->appendText("\n", theme()->style()->messageColor);
		editor->moveBottom();
	}

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stdout, "%s\n", osstr.c_str());

	return true;
}

bool Workspace::warn(const char* msg) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

	const bool withConsole = consoleVisible();
	if (withConsole) {
		EditorConsole* editor = (EditorConsole*)consoleTextBox();
		editor->appendText(msg, theme()->style()->warningColor);
		editor->appendText("\n", theme()->style()->warningColor);
		editor->moveBottom();
	}

	consoleHasWarning(true);

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stderr, "%s\n", osstr.c_str());

	return true;
}

bool Workspace::error(const char* msg) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

	const bool withConsole = consoleVisible();
	if (withConsole) {
		EditorConsole* editor = (EditorConsole*)consoleTextBox();
		editor->appendText(msg, theme()->style()->errorColor);
		editor->appendText("\n", theme()->style()->errorColor);
		editor->moveBottom();
	}

	consoleHasError(true);

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stderr, "%s\n", osstr.c_str());

	return true;
}

void Workspace::stroke(int key) {
	if (canvasDevice())
		canvasDevice()->stroke(key);
}

void Workspace::streamed(class Window* wnd, class Renderer* rnd, Bytes::Ptr bytes) {
	Operations::fileSaveStreamed(wnd, rnd, this, bytes);
}

void Workspace::sync(class Window* wnd, class Renderer* rnd, const char* module) {
	if (!module)
		return;

	if (strcmp(module, "sram") == 0) {
		fprintf(stdout, "Synced SRAM module.\n");

		saveSram(wnd, rnd);
	} else {
		fprintf(stderr, "Unknown module \"%s\" to be synced.\n", module);
	}
}

void Workspace::debug(const char* msg) {
	do {
		LockGuard<decltype(consoleLock())> guard(consoleLock());

		const bool withConsole = consoleVisible();
		if (withConsole) {
			EditorConsole* editor = (EditorConsole*)consoleTextBox();
			editor->appendText(msg, theme()->style()->debugColor);
			editor->appendText("\n", theme()->style()->debugColor);
			editor->moveBottom();
		}
	} while (false);

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stdout, "%s\n", osstr.c_str());

	do {
		LockGuard<decltype(debuggerLock())> guard(debuggerLock());

		debuggerMessages().add(msg);
	} while (false);
}

void Workspace::debug(void) {
	// Clear the debug messages.
	LockGuard<decltype(debuggerLock())> guard(debuggerLock());

	debuggerMessages().clear();
}

void Workspace::cursor(Device::CursorTypes mode) {
	canvasCursorMode(mode);
}

void Workspace::run(class Window* wnd, class Renderer* rnd, Bytes::Ptr rom, bool traceless) {
	Operations::projectRun(wnd, rnd, this, rom, nullptr, traceless);
}

bool Workspace::running(void) const {
	if (canvasDevice())
		return true;

	return false;
}

void Workspace::pause(class Window*, class Renderer*) {
	if (canvasDevice())
		canvasDevice()->pause();
}

void Workspace::stop(class Window* wnd, class Renderer* rnd) {
	const bool traceless = canvasDevice() && canvasDevice()->traceless();
	Operations::projectStop(wnd, rnd, this)
		.then(
			[wnd, rnd, this, traceless] (bool ok, const Bytes::Ptr sram) -> void {
				if (!traceless && ok && settings().deviceSaveSramOnStop) {
					const Project::Ptr &prj = currentProject();

					Operations::projectSaveSram(wnd, rnd, this, prj, sram, false);
				}
			}
		);
}

void Workspace::focusLost(Window* wnd, Renderer* rnd) {
	isFocused(false);

	withCurrentAsset(
		[rnd] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
			editor->focusLost(rnd);
		}
	);

	save(wnd, rnd);
}

void Workspace::focusGained(Window*, Renderer* rnd) {
	isFocused(true);

	withCurrentAsset(
		[rnd] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
			editor->focusGained(rnd);
		}
	);
}

void Workspace::renderTargetsReset(Window*, Renderer*) {
	// Do nothing.
}

void Workspace::moved(Window* wnd, Renderer* /* rnd */, const Math::Vec2i &wndPos) {
	if (wndPos.x < -50 && wndPos.y < -50) // Should be ignored.
		return;

	if (!wnd->maximized() && !wnd->fullscreen())
		settings().applicationWindowPosition = wndPos;
}

void Workspace::resized(Window* wnd, Renderer* rnd, const Math::Vec2i &wndSize, const Math::Vec2i &rndSize, const Math::Recti &/* notch */, Window::Orientations /* orientation */) {
	const Math::Vec2i minSize = wnd->minimumSize();
	if (wndSize.x < minSize.x || wndSize.y < minSize.y) // Should be ignored.
		return;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	codeEditorMinorWidth(codeEditorMinorWidth() / previousRendererSize().x * rndSize.x);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	if (!wnd->maximized() && !wnd->fullscreen())
		settings().applicationWindowSize = wndSize;

	withAllAssets(
		[rnd, this, rndSize] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
			editor->resized(rnd, previousRendererSize(), rndSize);
		}
	);

	previousRendererSize(rndSize);
}

void Workspace::maximized(Window* /* wnd */, Renderer* /* rnd */) {
	settings().applicationWindowFullscreen = false;
	settings().applicationWindowMaximized = true;
}

void Workspace::restored(Window* wnd, Renderer* /* rnd */) {
	settings().applicationWindowFullscreen = wnd->fullscreen();
	settings().applicationWindowMaximized = false;

#if WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED
	if (!wnd->bordered()) {
		const Math::Vec2i wSize = settings().applicationWindowSize;

		wnd->size(wSize + Math::Vec2i(1, 1)); // Make it a bit larger then restore the size to refresh the window.
		wnd->size(wSize);
	}
#endif /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */
}

void Workspace::dpi(Window* wnd, Renderer* rnd, const Math::Vec3f &dpi_) {
	(void)wnd;
	(void)rnd;
	(void)dpi_;
}

void Workspace::textInput(Window* wnd, Renderer* rnd, const char* const txt) {
	(void)wnd;
	(void)rnd;
	(void)txt;
}

void Workspace::fileDropped(Window*, Renderer*, const char* const path) {
	if (path)
		droppedFiles().push_back(path);
}

void Workspace::dropBegan(Window*, Renderer*) {
	droppedFiles().clear();
}

void Workspace::dropEnded(Window* wnd, Renderer* rnd) {
	const bool loadEg = !_toUpgrade && !_toCompile && showRecentProjects();

	const Text::Array files = droppedFiles();
	droppedFiles().clear();

	if (loadEg) {
		if (files.size() == 1) {
			const std::string &path_ = files.front();

			auto next = [wnd, rnd, this, path_] (void) -> void {
				Operations::fileDragAndDropFile(wnd, rnd, this, path_.c_str())
					.then(
						[wnd, rnd, this] (Project::Ptr prj) -> void {
							if (!prj)
								return;

							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl)
								return;

							Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[wnd, rnd, this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());

										if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
											launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
									}
								)
								.fail(
									[wnd, rnd, this, prj] (void) -> void {
										Operations::fileRemoveReference(wnd, rnd, this, prj);
									}
								);
						}
					);
			};

			closeFilter();

			const bool traceless = canvasDevice() && canvasDevice()->traceless();
			Operations::projectStop(wnd, rnd, this)
				.then(
					[wnd, rnd, this, next, traceless] (bool ok, const Bytes::Ptr sram) -> void {
						if (!traceless && ok && settings().deviceSaveSramOnStop) {
							const Project::Ptr &prj = currentProject();

							Operations::projectSaveSram(wnd, rnd, this, prj, sram, false);
						}

						next();
					}
				);
		} else if (files.size() > 1) {
			auto next = [wnd, rnd, this, files] (void) -> void {
				for (const std::string &path_ : files) {
					if (Path::fileExists(path_.c_str())) {
						Operations::fileDragAndDropFile(wnd, rnd, this, path_.c_str());
					}
				}
			};

			closeFilter();

			const bool traceless = canvasDevice() && canvasDevice()->traceless();
			Operations::projectStop(wnd, rnd, this)
				.then(
					[wnd, rnd, this, next, traceless] (bool ok, const Bytes::Ptr sram) -> void {
						if (!traceless && ok && settings().deviceSaveSramOnStop) {
							const Project::Ptr &prj = currentProject();

							Operations::projectSaveSram(wnd, rnd, this, prj, sram, false);
						}

						next();
					}
				);
		}
	} else {
		if (!files.empty()) {
			const std::string &path_ = files.front();

			auto next = [wnd, rnd, this, path_] (void) -> void {
				Operations::fileDragAndDropFileForNotepad(wnd, rnd, this, path_.c_str())
					.then(
						[wnd, rnd, this] (Project::Ptr prj) -> void {
							if (!prj)
								return;

							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl)
								return;

							Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());
									}
								)
								.fail(
									[wnd, rnd, this, prj] (void) -> void {
										Operations::fileRemoveReference(wnd, rnd, this, prj);
									}
								);
						}
					);
			};

			closeFilter();

			const bool traceless = canvasDevice() && canvasDevice()->traceless();
			Operations::projectStop(wnd, rnd, this)
				.then(
					[wnd, rnd, this, next, traceless] (bool ok, const Bytes::Ptr sram) -> void {
						if (!traceless && ok && settings().deviceSaveSramOnStop) {
							const Project::Ptr &prj = currentProject();

							Operations::projectSaveSram(wnd, rnd, this, prj, sram, false);
						}

						next();
					}
				);
		}
	}
}

bool Workspace::quit(Window* wnd, Renderer* rnd) {
	// Join thread if there's any.
	joinCompiling();

	// Save project.
	do {
		Project::Ptr &prj = currentProject();

		if (!prj || !prj->dirty()) {
			debug();

			const bool traceless = canvasDevice() && canvasDevice()->traceless();
			Operations::projectStop(wnd, rnd, this)
				.then(
					[wnd, rnd, this, traceless] (bool ok, const Bytes::Ptr sram) -> void {
						if (!traceless && ok && settings().deviceSaveSramOnStop) {
							const Project::Ptr &prj = currentProject();

							Operations::projectSaveSram(wnd, rnd, this, prj, sram, false)
								.always(
									[wnd, rnd, this] (void) -> void {
										Operations::fileClose(wnd, rnd, this);
									}
								);
						}
					}
				);

			join();

			break;
		}

		busy(true);

		closeFilter();

		stopProject(wnd, rnd);

		destroyAudioDevice();

		Operations::fileClose(wnd, rnd, this)
			.then(
				[this] (void) -> void {
					busy(false);

					SDL_Event evt;
					evt.type = SDL_QUIT;
					SDL_PushEvent(&evt);
				}
			)
			.fail(
				[this] (void) -> void {
					busy(false);
				}
			);
	} while (false);

	// Finish.
	if (busy())
		return false;

	return true;
}

void Workspace::sendExternalEvent(Window* wnd, Renderer* rnd, ExternalEventTypes type, void* event) {
#if defined GBBASIC_OS_HTML
	auto toCompile = [this] (Window* wnd, Renderer* rnd) -> void {
		if (!currentProject()) {
			messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

			fprintf(stderr, "No valid project.\n");

			return;
		}

		launchProject(wnd, rnd, nullptr, nullptr, nullptr, false, -1);
	};
	auto toRun = [this] (Window* wnd, Renderer* rnd) -> void {
		if (!currentProject()) {
			messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

			fprintf(stderr, "No valid project.\n");

			return;
		}

		launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
	};

	SDL_Event* evt = (SDL_Event*)event;
	switch (type) {
	case ExternalEventTypes::RESIZE_WINDOW: {
			const int width = (int)(intptr_t)evt->user.data1;
			const int height = (int)(intptr_t)evt->user.data2;

			const Math::Vec2i oldSize = wnd->size();
			Math::Vec2i size(width, height);
			wnd->size(size);
			size = wnd->size();

			fprintf(stdout, "SDL: RESIZE_WINDOW prefered [%d, %d], was [%d, %d], now [%d, %d].\n", width, height, oldSize.x, oldSize.y, size.x, size.y);
		}

		break;
	case ExternalEventTypes::UNLOAD_WINDOW: {
			fprintf(stdout, "SDL: UNLOAD_WINDOW.\n");

			save(wnd, rnd);

			saveSram(wnd, rnd);
		}

		break;
	case ExternalEventTypes::LOAD_PROJECT: {
			fprintf(stdout, "SDL: LOAD_PROJECT.\n");

			if (loadingProject()) {
				fprintf(stderr, "In loading, calm down.\n");

				return;
			}

			const int sub = (int)evt->user.code;
			const int cat = (int)(intptr_t)evt->user.data1;
			const int page = (int)(intptr_t)evt->user.data2;

			const char* content = workspaceGetProjectData();
			if (!content) {
				messagePopupBox(theme()->dialogPrompt_NoValidContent(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid content.\n");

				break;
			}
			if (!*content) {
				workspaceFree((void*)content); content = nullptr;

				messagePopupBox(theme()->dialogPrompt_NoValidContent(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid content.\n");

				break;
			}

			Text::Ptr content_(Text::create());
			content_->text(content);
			workspaceFree((void*)content); content = nullptr;
			content_->text(Text::replace(content_->text(), "\r\n", "\n"));
			content_->text(Text::replace(content_->text(), "\r", "\n"));

			loadingProject(true);

			auto next = [wnd, rnd, this, cat, page, sub, toCompile, toRun, content_] (void) -> void {
				Operations::fileImportStringForNotepad(wnd, rnd, this, content_)
					.then(
						[wnd, rnd, this, cat, page, sub, toCompile, toRun] (Project::Ptr prj) -> void {
							if (!prj) {
								loadingProject(false);

								return;
							}

							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl) {
								loadingProject(false);

								return;
							}

							Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[wnd, rnd, this, cat, page, sub, prj, toCompile, toRun] (bool ok) -> void {
										if (!ok) {
											loadingProject(false);

											return;
										}

										validateProject(prj.get());

										category((Categories)cat);
										switch ((Categories)cat) {
										case Categories::CODE: {
												fprintf(stdout, "SDL: TO CODE.\n");

												changePage(wnd, rnd, prj.get(), category(), page);

												CodeAssets::Entry* entry = prj->getCode(page);
												if (entry) {
													Editable* editor = entry->editor;
													if (!editor)
														editor = touchCodeEditor(wnd, rnd, prj.get(), page, true, entry);
													if (editor)
														editor->post(Editable::SET_CURSOR, (Variant::Int)sub);
												}
											}

											break;
										case Categories::MAP: {
												fprintf(stdout, "SDL: TO MAP.\n");

												changePage(wnd, rnd, prj.get(), category(), page);

												MapAssets::Entry* entry = prj->getMap(page);
												if (entry) {
													Editable* editor = entry->editor;
													if (!editor) {
														BaseAssets::Entry* refAsset = prj->tilesPageCount() == 0 ? nullptr : prj->getTiles(entry->ref);
														if (refAsset)
															editor = touchMapEditor(wnd, rnd, prj.get(), prj->activeMapIndex(), (unsigned)Categories::TILES, entry->ref, entry);
														else
															editor = touchMapEditor(wnd, rnd, prj.get(), prj->activeMapIndex(), (unsigned)(~0), -1, entry);
													}
													if (editor)
														editor->post(Editable::SET_FRAME, (Variant::Int)sub);
												}
											}

											break;
										case Categories::ACTOR: {
												fprintf(stdout, "SDL: TO ACTOR.\n");

												changePage(wnd, rnd, prj.get(), category(), page);

												ActorAssets::Entry* entry = prj->getActor(page);
												if (entry) {
													Editable* editor = entry->editor;
													if (!editor)
														editor = touchActorEditor(wnd, rnd, prj.get(), page, entry);
													if (editor)
														editor->post(Editable::SET_FRAME, (Variant::Int)sub);
												}
											}

											break;
										case Categories::SCENE: {
												fprintf(stdout, "SDL: TO SCENE.\n");

												changePage(wnd, rnd, prj.get(), category(), page);

												SceneAssets::Entry* entry = prj->getScene(page);
												if (entry) {
													Editable* editor = entry->editor;
													if (!entry->editor) {
														BaseAssets::Entry* refAsset = prj->mapPageCount() == 0 ? nullptr : prj->getMap(entry->refMap);
														if (refAsset)
															editor = touchSceneEditor(wnd, rnd, prj.get(), prj->activeSceneIndex(), (unsigned)Categories::MAP, entry->refMap, entry);
														else
															editor = touchSceneEditor(wnd, rnd, prj.get(), prj->activeSceneIndex(), (unsigned)(~0), -1, entry);
													}
													if (editor)
														editor->post(Editable::SET_FRAME, (Variant::Int)sub);
												}
											}

											break;
										case Categories::CONSOLE:
											fprintf(stdout, "SDL: TO COMPILE.\n");

											toCompile(wnd, rnd);

											break;
										case Categories::EMULATOR:
											fprintf(stdout, "SDL: TO RUN.\n");

											toRun(wnd, rnd);

											break;
										default:
											// Do nothing.

											break;
										}

										loadingProject(false);
									}
								)
								.fail(
									[this] (void) -> void {
										loadingProject(false);
									}
								);
						}
					)
					.fail(
						[this] (void) -> void {
							loadingProject(false);
						}
					);
			};

			debug();

			const bool traceless = canvasDevice() && canvasDevice()->traceless();
			Operations::projectStop(wnd, rnd, this)
				.then(
					[wnd, rnd, this, next, traceless] (bool ok, const Bytes::Ptr sram) -> void {
						if (!traceless && ok && settings().deviceSaveSramOnStop) {
							const Project::Ptr &prj = currentProject();

							Operations::projectSaveSram(wnd, rnd, this, prj, sram, false);
						}

						next();
					}
				)
				.fail(
					[this] (void) -> void {
						loadingProject(false);
					}
				);
		}

		break;
	case ExternalEventTypes::PATCH_PROJECT: {
			fprintf(stdout, "SDL: PATCH_PROJECT.\n");

			const int cat = (int)(intptr_t)evt->user.data1;
			const int page = (int)(intptr_t)evt->user.data2;

			const char* content = workspaceGetProjectData();
			if (!content) {
				messagePopupBox(theme()->dialogPrompt_NoValidContent(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid content.\n");

				break;
			}

			const std::string content_ = content;
			workspaceFree((void*)content); content = nullptr;
			if (content_.empty()) {
				messagePopupBox(theme()->dialogPrompt_NoValidContent(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid content.\n");

				break;
			}

			if (!currentProject()) {
				messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid project.\n");

				break;
			}

			if ((Categories)cat != Categories::CODE) { // Supports code only.
				messagePopupBox(theme()->dialogPrompt_UnsupportedOperation(), nullptr, nullptr, nullptr);

				fprintf(stderr, "Unsupported operation.\n");

				break;
			}

			Project::Ptr &prj = currentProject();

			category((Categories)cat);
			changePage(wnd, rnd, prj.get(), category(), page);

			CodeAssets::Entry* entry = prj->getCode(page);
			if (!entry) {
				messagePopupBox(theme()->dialogPrompt_UnsupportedOperation(), nullptr, nullptr, nullptr);

				fprintf(stderr, "Unsupported operation.\n");

				break;
			}
			Editable* editor = entry->editor;
			if (!editor)
				editor = touchCodeEditor(wnd, rnd, currentProject().get(), page, true, entry);
			if (editor) {
				editor->post(Editable::SELECT_ALL);
				editor->post(Editable::SET_CONTENT, (void*)content_.c_str());
			}
		}

		break;
	case ExternalEventTypes::INPUT_KEY: {
			fprintf(stdout, "SDL: INPUT_KEY.\n");

			const int keyDown = (int)(intptr_t)evt->user.data1;
			const int keyUp = (int)(intptr_t)evt->user.data2;

			if (keyDown) {
				SDL_Event evt;
				memset(&evt, 0, sizeof(SDL_Event));
				evt.type = SDL_KEYDOWN;
				evt.key.keysym.scancode = (SDL_Scancode)keyDown;
				SDL_PushEvent(&evt);
			}
			if (keyUp) {
				SDL_Event evt;
				memset(&evt, 0, sizeof(SDL_Event));
				evt.type = SDL_KEYUP;
				evt.key.keysym.scancode = (SDL_Scancode)keyUp;
				SDL_PushEvent(&evt);
			}
		}

		break;
	case ExternalEventTypes::INPUT_TEXT: {
			fprintf(stdout, "SDL: INPUT_TEXT.\n");

			const char* content = workspaceGetInputText();
			if (!content)
				break;

			const std::string content_ = content;
			workspaceFree((void*)content); content = nullptr;

			ImGuiIO &io = ImGui::GetIO();

			io.AddInputCharactersUTF8(content_.c_str());

			textInput(wnd, rnd, content_.c_str());
		}

		break;
	case ExternalEventTypes::TO_CATEGORY: {
			fprintf(stdout, "SDL: TO_CATEGORY.\n");

			const int cat = (int)(intptr_t)evt->user.data1;

			if (!currentProject()) {
				messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid project.\n");

				break;
			}

			category((Categories)cat);
		}

		break;
	case ExternalEventTypes::TO_PAGE: {
			fprintf(stdout, "SDL: TO_PAGE.\n");

			const int page = (int)(intptr_t)evt->user.data1;

			if (!currentProject()) {
				messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid project.\n");

				break;
			}

			Project::Ptr &prj = currentProject();

			changePage(wnd, rnd, prj.get(), category(), page);
		}

		break;
	case ExternalEventTypes::TO_LOCATION: {
			fprintf(stdout, "SDL: TO_LOCATION.\n");

			const int sub = (int)(intptr_t)evt->user.data1;

			if (!currentProject()) {
				messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid project.\n");

				break;
			}

			Project::Ptr &prj = currentProject();

			switch (category()) {
			case Categories::CODE: {
					const int pg = prj->activeMajorCodeIndex();

					CodeAssets::Entry* entry = prj->getCode(pg);
					if (entry) {
						Editable* editor = entry->editor;
						if (!editor)
							editor = touchCodeEditor(wnd, rnd, prj.get(), pg, true, entry);
						if (editor)
							editor->post(Editable::SET_CURSOR, (Variant::Int)sub);
					}
				}

				break;
			case Categories::MAP: {
					const int pg = prj->activeMapIndex();

					MapAssets::Entry* entry = prj->getMap(pg);
					if (entry) {
						Editable* editor = entry->editor;
						if (editor)
							editor->post(Editable::SET_FRAME, (Variant::Int)sub);
					}
				}

				break;
			case Categories::ACTOR: {
					const int pg = prj->activeActorIndex();

					ActorAssets::Entry* entry = prj->getActor(pg);
					if (entry) {
						Editable* editor = entry->editor;
						if (editor)
							editor->post(Editable::SET_FRAME, (Variant::Int)sub);
					}
				}

				break;
			case Categories::SCENE: {
					const int pg = prj->activeSceneIndex();

					SceneAssets::Entry* entry = prj->getScene(pg);
					if (entry) {
						Editable* editor = entry->editor;
						if (editor)
							editor->post(Editable::SET_FRAME, (Variant::Int)sub);
					}
				}

				break;
			default:
				// Do nothing.

				break;
			}
		}

		break;
	case ExternalEventTypes::COMPILE: {
			fprintf(stdout, "SDL: COMPILE.\n");

			if (!currentProject()) {
				messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid project.\n");

				break;
			}

			toCompile(wnd, rnd);
		}

		break;
	case ExternalEventTypes::RUN: {
			fprintf(stdout, "SDL: RUN.\n");

			if (!currentProject()) {
				messagePopupBox(theme()->dialogPrompt_NoValidProject(), nullptr, nullptr, nullptr);

				fprintf(stderr, "No valid project.\n");

				break;
			}

			toRun(wnd, rnd);
		}

		break;
	default:
		// Do nothing.

		break;
	}
#else /* Platform macro. */
	(void)wnd;
	(void)rnd;
	(void)type;
	(void)event;
#endif /* Platform macro. */
}

class EditorFont* Workspace::touchFontEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, FontAssets::Entry* entry) {
	if (!entry)
		entry = prj->getFont(idx);
	if (!entry)
		return nullptr;

	EditorFont* editor = (EditorFont*)entry->editor;
	if (!editor) {
		editor = EditorFont::create();
		editor->open(wnd, rnd, this, prj, idx, (unsigned)(~0), -1);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorCode* Workspace::touchCodeEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, bool isMajor, CodeAssets::Entry* entry) {
	if (!entry)
		entry = prj->getCode(idx);
	if (!entry)
		return nullptr;

	EditorCode* editor = (EditorCode*)entry->editor;
	if (!editor) {
		editor = EditorCode::create(this, isMajor);
		editor->open(wnd, rnd, this, prj, idx, (unsigned)(~0), -1);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorTiles* Workspace::touchTilesEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, TilesAssets::Entry* entry) {
	if (!entry)
		entry = prj->getTiles(idx);
	if (!entry)
		return nullptr;

	EditorTiles* editor = (EditorTiles*)entry->editor;
	if (!editor) {
		editor = EditorTiles::create();
		editor->open(wnd, rnd, this, prj, idx, (unsigned)(~0), -1);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorMap* Workspace::touchMapEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, unsigned refCategory, int refIndex, MapAssets::Entry* entry) {
	if (!entry)
		entry = prj->getMap(idx);
	if (!entry)
		return nullptr;

	EditorMap* editor = (EditorMap*)entry->editor;
	if (!editor) {
		editor = EditorMap::create();
		editor->open(wnd, rnd, this, prj, idx, refCategory, refIndex);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorMusic* Workspace::touchMusicEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, MusicAssets::Entry* entry) {
	if (!entry)
		entry = prj->getMusic(idx);
	if (!entry)
		return nullptr;

	EditorMusic* editor = (EditorMusic*)entry->editor;
	if (!editor) {
		editor = EditorMusic::create();
		editor->open(wnd, rnd, this, prj, idx, (unsigned)(~0), -1);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorSfx* Workspace::touchSfxEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, SfxAssets::Entry* entry) {
	if (!entry)
		entry = prj->getSfx(idx);
	if (!entry)
		return nullptr;

	EditorSfx* editor = (EditorSfx*)entry->editor;
	if (!editor) {
		editor = EditorSfx::create(wnd, rnd, this, prj);
		editor->open(wnd, rnd, this, prj, idx, (unsigned)(~0), -1);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorActor* Workspace::touchActorEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, ActorAssets::Entry* entry) {
	if (!entry)
		entry = prj->getActor(idx);
	if (!entry)
		return nullptr;

	EditorActor* editor = (EditorActor*)entry->editor;
	if (!editor) {
		editor = EditorActor::create();
		editor->open(wnd, rnd, this, prj, idx, (unsigned)(~0), -1);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

class EditorScene* Workspace::touchSceneEditor(Window* wnd, Renderer* rnd, Project* prj, int idx, unsigned refCategory, int refIndex, SceneAssets::Entry* entry) {
	if (!entry)
		entry = prj->getScene(idx);
	if (!entry)
		return nullptr;

	EditorScene* editor = (EditorScene*)entry->editor;
	if (!editor) {
		editor = EditorScene::create();
		editor->open(wnd, rnd, this, prj, idx, refCategory, refIndex);
		editor->enter(this);
		entry->editor = editor;
	}

	return editor;
}

void Workspace::bubble(const ImGui::Bubble::Ptr &ptr) {
	if (ptr && _bubble && _bubble->exclusive())
		return;

	_bubble = ptr;
}

void Workspace::bubble(
	const std::string &content,
	const ImGui::Bubble::TimeoutHandler &timeout_
) {
	ImGui::Bubble::TimeoutHandler timeout = timeout_;

	if (timeout.empty()) {
		timeout = ImGui::Bubble::TimeoutHandler(
			[&] (void) -> void {
				bubble(nullptr);
			},
			nullptr
		);
	}

	bubble(
		ImGui::Bubble::Ptr(
			new ImGui::Bubble(
				content,
				timeout
			)
		)
	);
}

void Workspace::popupBox(const ImGui::PopupBox::Ptr &ptr) {
	if (ptr && _popupBox && _popupBox->exclusive())
		return;

	if (_popupBox)
		_popupBox->dispose();

	_popupBox = ptr;

	if (canvasDevice())
		canvasDevice()->inputEnabled(!_popupBox);
}

void Workspace::waitingPopupBox(
	const ImGui::WaitingPopupBox::TimeoutHandler &timeout_
) {
	ImGui::WaitingPopupBox::TimeoutHandler timeout = timeout_;

	if (timeout.empty()) {
		timeout = ImGui::WaitingPopupBox::TimeoutHandler(
			[&] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::WaitingPopupBox(
				false, "",
				false, timeout,
				false
			)
		)
	);
}

void Workspace::waitingPopupBox(
	const std::string &content,
	const ImGui::WaitingPopupBox::TimeoutHandler &timeout_
) {
	ImGui::WaitingPopupBox::TimeoutHandler timeout = timeout_;

	if (timeout.empty()) {
		timeout = ImGui::WaitingPopupBox::TimeoutHandler(
			[&] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::WaitingPopupBox(
				content,
				timeout
			)
		)
	);
}

void Workspace::waitingPopupBox(
	bool dim, const std::string &content,
	bool instantly, const ImGui::WaitingPopupBox::TimeoutHandler &timeout_,
	bool exclusive
) {
	ImGui::WaitingPopupBox::TimeoutHandler timeout = timeout_;

	if (timeout.empty()) {
		timeout = ImGui::WaitingPopupBox::TimeoutHandler(
			[&] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::WaitingPopupBox(
				dim, content,
				instantly, timeout,
				exclusive
			)
		)
	);
}

void Workspace::messagePopupBox(
	const std::string &content,
	const ImGui::MessagePopupBox::ConfirmedHandler &confirm_,
	const ImGui::MessagePopupBox::DeniedHandler &deny,
	const ImGui::MessagePopupBox::CanceledHandler &cancel,
	const char* confirmTxt,
	const char* denyTxt,
	const char* cancelTxt,
	const char* confirmTooltips,
	const char* denyTooltips,
	const char* cancelTooltips,
	bool exclusive
) {
	const char* btnConfirm = confirmTxt;
	const char* btnDeny = denyTxt;
	const char* btnCancel = cancelTxt;

	ImGui::MessagePopupBox::ConfirmedHandler confirm = confirm_;

	if (confirm_.empty() && deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Ok().c_str();
	} else if (!confirm_.empty() && deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Ok().c_str();
	} else if (!confirm_.empty() && !deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Yes().c_str();
		if (!btnDeny)
			btnDeny = theme()->generic_No().c_str();
	} else if (!confirm_.empty() && !deny.empty() && !cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Yes().c_str();
		if (!btnDeny)
			btnDeny = theme()->generic_No().c_str();
		if (!btnCancel)
			btnCancel = theme()->generic_Cancel().c_str();
	}
	if (confirm_.empty()) {
		confirm = ImGui::MessagePopupBox::ConfirmedHandler(
			[&] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::MessagePopupBox(
				GBBASIC_TITLE,
				content,
				confirm, deny, cancel,
				btnConfirm, btnDeny, btnCancel,
				confirmTooltips, denyTooltips, cancelTooltips,
				exclusive
			)
		)
	);
}

void Workspace::messagePopupBoxWithOption(
	const std::string &content,
	const ImGui::MessagePopupBox::ConfirmedWithOptionHandler &confirm_,
	const ImGui::MessagePopupBox::DeniedHandler &deny,
	const ImGui::MessagePopupBox::CanceledHandler &cancel,
	const char* confirmTxt,
	const char* denyTxt,
	const char* cancelTxt,
	const char* confirmTooltips,
	const char* denyTooltips,
	const char* cancelTooltips,
	const char* optionBoolTxt, bool optionBoolValue, bool optionBoolValueReadonly
) {
	const char* btnConfirm = confirmTxt;
	const char* btnDeny = denyTxt;
	const char* btnCancel = cancelTxt;

	ImGui::MessagePopupBox::ConfirmedWithOptionHandler confirm = confirm_;

	if (confirm_.empty() && deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Ok().c_str();
	} else if (!confirm_.empty() && deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Ok().c_str();
	} else if (!confirm_.empty() && !deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Yes().c_str();
		if (!btnDeny)
			btnDeny = theme()->generic_No().c_str();
	} else if (!confirm_.empty() && !deny.empty() && !cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Yes().c_str();
		if (!btnDeny)
			btnDeny = theme()->generic_No().c_str();
		if (!btnCancel)
			btnCancel = theme()->generic_Cancel().c_str();
	}
	if (confirm_.empty()) {
		confirm = ImGui::MessagePopupBox::ConfirmedWithOptionHandler(
			[&] (bool) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::MessagePopupBox(
				GBBASIC_TITLE,
				content,
				confirm, deny, cancel,
				btnConfirm, btnDeny, btnCancel,
				confirmTooltips, denyTooltips, cancelTooltips,
				optionBoolTxt, optionBoolValue, optionBoolValueReadonly
			)
		)
	);
}

void Workspace::inputPopupBox(
	const std::string &content,
	const std::string &default_, unsigned flags,
	const ImGui::InputPopupBox::ConfirmedHandler &confirm_,
	const ImGui::InputPopupBox::CanceledHandler &cancel
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::InputPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::InputPopupBox::ConfirmedHandler(
			[&] (const char*) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::InputPopupBox(
				GBBASIC_TITLE,
				content, default_, flags,
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::projectCreatingPopupBox(
	const std::string &template_,
	const std::string &kernel,
	const std::string &content,
	const std::string &default_, unsigned flags,
	const ImGui::ProjectCreatingPopupBox::ConfirmedHandler &confirm_,
	const ImGui::ProjectCreatingPopupBox::CanceledHandler &cancel
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::ProjectCreatingPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::ProjectCreatingPopupBox::ConfirmedHandler(
			[&] (int, int, const char*) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::ProjectCreatingPopupBox(
				theme(),
				theme()->windowCreateProject(),
				template_,
				kernel,
				content, default_, flags,
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::assetsSortingPopupBox(
	Renderer* rnd,
	AssetsBundle::Categories category,
	const ImGui::AssetsSortingPopupBox::ConfirmedHandler &confirm_,
	const ImGui::AssetsSortingPopupBox::CanceledHandler &cancel
) {
	Project::Ptr &prj = currentProject();

	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::AssetsSortingPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::AssetsSortingPopupBox::ConfirmedHandler(
			[&] (ImGui::AssetsSortingPopupBox::Orders) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::AssetsSortingPopupBox(
				rnd,
				theme(),
				theme()->windowSortAssets(),
				prj.get(),
				(unsigned)category,
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::searchPopupBox(
	const std::string &content,
	const std::string &default_, unsigned flags,
	const ImGui::SearchPopupBox::ConfirmedHandler &confirm_,
	const ImGui::SearchPopupBox::CanceledHandler &cancel
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::SearchPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Search().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::SearchPopupBox::ConfirmedHandler(
			[&] (const char*) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::SearchPopupBox(
				theme(),
				theme()->windowSearch(),
				settings(),
				content, default_, flags,
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::showPaletteEditor(
	Renderer* rnd,
	int group,
	const ImGui::EditorPopupBox::ChangedHandler &changed
) {
	GBBASIC_ASSERT(currentProject() && "Impossible.");

	auto set = [this, group] (const PaletteAssets &data) -> void {
		Project::Ptr &prj = currentProject();
		PaletteAssets &assets = prj->touchPalette();
		PaletteAssets::Groups changed;
		assets.copyFrom(data, &changed);

		if (changed.empty())
			return;

		for (int idx : changed) {
			PaletteAssets::Entry* entry = assets.get(idx);
			entry->increaseRevision();
		}

		if (group == -1) {
			withCurrentAsset(
				[this] (Categories, BaseAssets::Entry*, Editable* editor) -> void {
					// Re-enter.
					editor->leave(this);
					editor->enter(this);
				}
			);
		}

		prj->synchronizeSuperPalettes(data);

		prj->hasDirtyAsset(true);
	};

	EditorPalette::ConfirmedHandler confirm(
		[this, set] (const PaletteAssets &data) -> void {
			set(data);

			popupBox(nullptr);
		},
		nullptr
	);
	EditorPalette::CanceledHandler cancel(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	EditorPalette::AppliedHandler apply(
		[set] (const PaletteAssets &data) -> void {
			set(data);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			EditorPalette::create(
				rnd, this,
				theme(),
				group, changed,
				theme()->windowPalette(),
				currentProject().get(),
				confirm, cancel, apply,
				theme()->generic_Ok().c_str(), theme()->generic_Cancel().c_str(), theme()->generic_Apply().c_str()
			)
		)
	);
}

void Workspace::showArbitraryEditor(
	Renderer* rnd,
	int fontIndex,
	const ImGui::EditorPopupBox::ChangedHandler &changed
) {
	GBBASIC_ASSERT(currentProject() && "Impossible.");

	EditorArbitrary::ConfirmedHandler confirm(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	EditorArbitrary::CanceledHandler cancel(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	EditorArbitrary::AppliedHandler apply(
		[] (void) -> void {
			// Do nothing.
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			EditorArbitrary::create(
				rnd, this,
				theme(),
				changed,
				theme()->windowFont_Arbitrary(),
				currentProject().get(), fontIndex,
				confirm, cancel, apply,
				theme()->generic_Ok().c_str(), theme()->generic_Cancel().c_str(), theme()->generic_Apply().c_str()
			)
		)
	);
}

void Workspace::showCodeBindingEditor(
	Window* wnd, Renderer* rnd,
	const ImGui::EditorPopupBox::ChangedHandler &changed,
	const std::string &index
) {
	GBBASIC_ASSERT(currentProject() && "Impossible.");

	auto bind = [wnd, rnd, this, changed, index] (void) -> void {
		EditorCodeBinding::ConfirmedHandler confirm(
			[this] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
		EditorCodeBinding::CanceledHandler cancel(
			[this] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
		EditorCodeBinding::GotoHandler goto_(
			[wnd, rnd, this] (int pg, int ln) -> void {
				category(Categories::CODE);
				changePage(nullptr, rnd, currentProject().get(), Categories::CODE, pg);

				CodeAssets::Entry* entry = currentProject()->getCode(pg);
				Editable* editor = entry->editor;
				if (!editor)
					editor = touchCodeEditor(wnd, rnd, currentProject().get(), pg, true, entry);
				if (editor)
					editor->post(Editable::SET_CURSOR, (Variant::Int)ln);

				popupBox(nullptr);
			},
			nullptr
		);
		popupBox(
			ImGui::PopupBox::Ptr(
				EditorCodeBinding::create(
					rnd, this,
					theme(),
					changed,
					theme()->windowScene_CodeBinding(),
					currentProject().get(),
					index,
					confirm, cancel, goto_,
					theme()->generic_Ok().c_str(), theme()->generic_Cancel().c_str(), theme()->generic_Goto().c_str()
				)
			)
		);
	};

	Operations::popupWait(wnd, rnd, this, true, theme()->dialogPrompt_Processing().c_str(), true, false)
		.then(
			[this, bind] (void) -> void {
				if (needAnalyzing()) {
					if (!analyzing())
						analyze(true); // Ensure it's analyzed.

#if defined GBBASIC_DEBUG
					do {
						// Wait for analyzing.
						wait();
					} while (needAnalyzing());
#else /* GBBASIC_DEBUG */
					for (int i = 0; i < 100; ++i) {
						// Wait for up to ~1s for analyzing or timeout.
						wait();

						if (!needAnalyzing())
							break;
					}
#endif /* GBBASIC_DEBUG */
				}

				bind();
			}
		);
}

void Workspace::showActorSelector(
	Renderer* rnd,
	int index,
	const ImGui::ActorSelectorPopupBox::ConfirmedHandler &confirm_,
	const ImGui::ActorSelectorPopupBox::CanceledHandler &cancel
) {
	GBBASIC_ASSERT(currentProject() && "Impossible.");

	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::ActorSelectorPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::ActorSelectorPopupBox::ConfirmedHandler(
			[&] (int) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::ActorSelectorPopupBox(
				rnd,
				theme(),
				theme()->dialogPrompt_SelectActor(),
				currentProject().get(),
				index,
				confirm, cancel
			)
		)
	);
}

void Workspace::showPropertiesEditor(
	Renderer* rnd,
	Actor::Ptr obj, int layer,
	const Actor::Shadow::Array &shadow,
	const Text::Array &frameNames,
	const ImGui::EditorPopupBox::ChangedHandler &changed
) {
	GBBASIC_ASSERT(currentProject() && "Impossible.");

	EditorProperties::ConfirmedHandler confirm(
		[this] (const EditorProperties::ImageArray &) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	EditorProperties::CanceledHandler cancel(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	EditorProperties::AppliedHandler apply(
		[] (const EditorProperties::ImageArray &) -> void {
			// Do nothing.
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			EditorProperties::create(
				rnd, this,
				theme(),
				obj, layer,
				shadow,
				frameNames,
				changed,
				theme()->windowActor_Properties(),
				currentProject().get(),
				confirm, cancel, apply,
				theme()->generic_Ok().c_str(), theme()->generic_Cancel().c_str(), theme()->generic_Apply().c_str()
			)
		)
	);
}

void Workspace::showExternalFontBrowser(
	Renderer* rnd,
	const std::string &content,
	const Text::Array &filter,
	const ImGui::FontResolverPopupBox::ConfirmedHandler &confirm_,
	const ImGui::FontResolverPopupBox::CanceledHandler &cancel,
	const ImGui::FontResolverPopupBox::SelectedHandler &select,
	const ImGui::FontResolverPopupBox::CustomHandler &custom
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::FontResolverPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::FontResolverPopupBox::ConfirmedHandler(
			[&] (const char*, const Math::Vec2i*) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::FontResolverPopupBox(
				rnd,
				theme(),
				theme()->windowFont(),
				Math::Vec2i(8, 8), Math::Vec2i(GBBASIC_FONT_MIN_SIZE, GBBASIC_FONT_MIN_SIZE), Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE), theme()->generic_Size().c_str(), "x",
				content, "",
				filter,
				true,
				theme()->generic_Browse().c_str(),
				confirm, cancel,
				btnConfirm, btnCancel,
				select,
				custom
			)
		)
	);
}

void Workspace::showExternalMapBrowser(
	Renderer* rnd,
	const std::string &content,
	const Text::Array &filter,
	bool withTiles, bool withPath,
	const ImGui::MapResolverPopupBox::ConfirmedHandler &confirm_,
	const ImGui::MapResolverPopupBox::CanceledHandler &cancel,
	const ImGui::MapResolverPopupBox::SelectedHandler &select,
	const ImGui::MapResolverPopupBox::CustomHandler &custom
) {
	Project::Ptr &prj = currentProject();

	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::MapResolverPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::MapResolverPopupBox::ConfirmedHandler(
			[&] (const int*, const char*, bool) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	bool allowFlip = true;
	if (!prj->cartridgeType().empty()) {
		allowFlip = false;

		const std::string cartType = prj->cartridgeType();

		const Text::Array parts = Text::split(cartType, PROJECT_CARTRIDGE_TYPE_SEPARATOR);
		for (const std::string &part : parts) {
			if (part == PROJECT_CARTRIDGE_TYPE_COLORED) {
				allowFlip = true;

				break;
			}
		}
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::MapResolverPopupBox(
				rnd,
				theme(),
				theme()->windowMap(),
				withTiles ? theme()->dialogPrompt_FromTiles().c_str() : nullptr, withPath ? theme()->dialogPrompt_FromImage().c_str() : nullptr,
				prj->preferencesMapRef(), 0, Math::max(0, prj->tilesPageCount() - 1), theme()->dialogPrompt_TilesPage().c_str(),
				content, "",
				filter,
				withPath,
				theme()->generic_Browse().c_str(),
				allowFlip,
				theme()->dialogPrompt_AllowFlip().c_str(), theme()->dialogPrompt_ColoredOnly().c_str(),
				confirm, cancel,
				btnConfirm, btnCancel,
				select,
				custom
			)
		)
	);
}

void Workspace::showExternalSceneBrowser(
	Renderer* rnd,
	const ImGui::SceneResolverPopupBox::ConfirmedHandler &confirm_,
	const ImGui::SceneResolverPopupBox::CanceledHandler &cancel,
	const ImGui::SceneResolverPopupBox::SelectedHandler &select,
	const ImGui::SceneResolverPopupBox::CustomHandler &custom
) {
	Project::Ptr &prj = currentProject();

	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::SceneResolverPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::SceneResolverPopupBox::ConfirmedHandler(
			[&] (int, bool useGravity) -> void {
				prj->preferencesSceneUseGravity(useGravity);

				popupBox(nullptr);
			},
			nullptr
		);
	}

	bool allowFlip = true;
	if (!prj->cartridgeType().empty()) {
		allowFlip = false;

		const std::string cartType = prj->cartridgeType();

		const Text::Array parts = Text::split(cartType, PROJECT_CARTRIDGE_TYPE_SEPARATOR);
		for (const std::string &part : parts) {
			if (part == PROJECT_CARTRIDGE_TYPE_COLORED) {
				allowFlip = true;

				break;
			}
		}
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::SceneResolverPopupBox(
				rnd,
				theme(),
				theme()->windowScene(),
				prj.get(),
				prj->preferencesSceneRefMap(), 0, Math::max(0, prj->mapPageCount() - 1), theme()->dialogPrompt_MapPage().c_str(),
				prj->preferencesSceneUseGravity(), theme()->dialogPrompt_UseGravity().c_str(),
				confirm, cancel,
				btnConfirm, btnCancel,
				select,
				custom
			)
		)
	);
}

void Workspace::showExternalFileBrowser(
	Renderer* rnd,
	const std::string &title,
	const std::string &content,
	const Text::Array &filter,
	bool requireExisting,
	const ImGui::FileResolverPopupBox::ConfirmedHandler_Path &confirm_,
	const ImGui::FileResolverPopupBox::CanceledHandler &cancel,
	const ImGui::FileResolverPopupBox::SelectedHandler &select,
	const ImGui::FileResolverPopupBox::CustomHandler &custom
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::FileResolverPopupBox::ConfirmedHandler_Path confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::FileResolverPopupBox::ConfirmedHandler_Path(
			[&] (const char*) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::FileResolverPopupBox(
				rnd,
				theme(),
				title,
				content, "",
				filter,
				requireExisting,
				theme()->generic_Browse().c_str(),
				confirm, cancel,
				btnConfirm, btnCancel,
				select,
				custom
			)
		)
	);
}

void Workspace::showExternalFileBrowser(
	Renderer* rnd,
	const std::string &title,
	const std::string &content,
	const Text::Array &filter,
	bool requireExisting,
	bool default_, const char* optTxt,
	const ImGui::FileResolverPopupBox::ConfirmedHandler_PathBoolean &confirm_,
	const ImGui::FileResolverPopupBox::CanceledHandler &cancel,
	const ImGui::FileResolverPopupBox::SelectedHandler &select,
	const ImGui::FileResolverPopupBox::CustomHandler &custom
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::FileResolverPopupBox::ConfirmedHandler_PathBoolean confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::FileResolverPopupBox::ConfirmedHandler_PathBoolean(
			[&] (const char*, bool) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::FileResolverPopupBox(
				rnd,
				theme(),
				title,
				content, "",
				filter,
				requireExisting,
				theme()->generic_Browse().c_str(),
				default_, optTxt,
				confirm, cancel,
				btnConfirm, btnCancel,
				select,
				custom
			)
		)
	);
}

void Workspace::showExternalFileBrowser(
	Renderer* rnd,
	const std::string &title,
	const std::string &content,
	const Text::Array &filter,
	bool requireExisting,
	int default_, int min, int max, const char* optTxt,
	bool toSave,
	const ImGui::FileResolverPopupBox::ConfirmedHandler_PathInteger &confirm_,
	const ImGui::FileResolverPopupBox::CanceledHandler &cancel,
	const ImGui::FileResolverPopupBox::SelectedHandler &select,
	const ImGui::FileResolverPopupBox::CustomHandler &custom
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::FileResolverPopupBox::ConfirmedHandler_PathInteger confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::FileResolverPopupBox::ConfirmedHandler_PathInteger(
			[&] (const char*, int) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			(
				new ImGui::FileResolverPopupBox(
					rnd,
					theme(),
					title,
					content, "",
					filter,
					requireExisting,
					theme()->generic_Browse().c_str(),
					default_, min, max, optTxt,
					confirm, cancel,
					btnConfirm, btnCancel,
					select,
					custom
				)
			)
			->toSave(toSave)
		)
	);
}

void Workspace::showExternalFileBrowser(
	Renderer* rnd,
	const std::string &title,
	const std::string &content,
	const Text::Array &filter,
	bool requireExisting,
	const Math::Vec2i &default_, const Math::Vec2i &min, const Math::Vec2i &max, const char* optTxt, const char* optXTxt,
	const ImGui::FileResolverPopupBox::ConfirmedHandler_PathVec2i &confirm_,
	const ImGui::FileResolverPopupBox::CanceledHandler &cancel,
	const ImGui::FileResolverPopupBox::SelectedHandler &select,
	const ImGui::FileResolverPopupBox::CustomHandler &custom,
	const ImGui::FileResolverPopupBox::Vec2iResolver &resolveVec2i, const char* resolveVec2iTxt
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::FileResolverPopupBox::ConfirmedHandler_PathVec2i confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::FileResolverPopupBox::ConfirmedHandler_PathVec2i(
			[&] (const char*, const Math::Vec2i &) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::FileResolverPopupBox(
				rnd,
				theme(),
				title,
				content, "",
				filter,
				requireExisting,
				theme()->generic_Browse().c_str(),
				default_, min, max, optTxt, optXTxt,
				confirm, cancel,
				btnConfirm, btnCancel,
				select,
				custom,
				resolveVec2i, resolveVec2iTxt
			)
		)
	);
}

void Workspace::showProjectProperty(Window* wnd, Renderer* rnd, Project* prj, bool isOpened) {
	auto save = [wnd, rnd, this] (Project* prj_) -> void {
		// Initialize the output methods.
		PrintHandler onPrint = [] (const char* msg) -> void {
			fprintf(stdout, "%s\n", msg);
		};
		WarningOrErrorHandler onError = [] (const char* msg, bool isWarning) -> void {
			if (isWarning)
				fprintf(stdout, "%s\n", msg);
			else
				fprintf(stderr, "%s\n", msg);
		};

		// Load the project.
		onPrint("Begin saving project property.");
		const long long start = DateTime::ticks();

		const std::string &path = prj_->path();

		const int oldKrnlIdx = activeKernelIndex();

		Project::Ptr prj = nullptr;
		do {
			if (!rnd)
				break;

			prj = Project::Ptr(new Project(wnd, rnd, this));
			if (!prj->open(path.c_str())) {
				onError("Cannot open the project.", false);

				break;
			}

			Texture::Ptr attribtex(theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
			Texture::Ptr propstex(theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
			Texture::Ptr actorstex(theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
			prj->attributesTexture(attribtex);
			prj->propertiesTexture(propstex);
			prj->actorsTexture(actorstex);

			prj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, this, std::placeholders::_1));
			prj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, this, std::placeholders::_1));

			const int kernelIdx = getValidKernelIndex(prj.get());
			activeKernelIndex(kernelIdx);

			const std::string fontConfigPath = fontConfig().empty() ? WORKSPACE_FONT_DEFAULT_CONFIG_FILE : fontConfig();
			if (!prj->load(false, fontConfigPath.c_str(), nullptr)) {
				prj->close(true);

				activeKernelIndex(oldKrnlIdx);

				onError("Cannot load the project.", false);

				break;
			}
		} while (false);

		// Save the project.
		if (!prj_->title().empty())
			prj->title(prj_->title());
		prj->kernel(prj_->kernel());
		prj->cartridgeType(prj_->cartridgeType());
		prj->sramType(prj_->sramType());
		prj->hasRtc(prj_->hasRtc());
		prj->description(prj_->description());
		prj->author(prj_->author());
		prj->genre(prj_->genre());
		prj->version(prj_->version());
		prj->url(prj_->url());
		prj->runOnOpen(prj_->runOnOpen());
		prj->iconCode(prj_->iconCode());
		prj->caseInsensitive(prj_->caseInsensitive());
		prj->strictOn(prj_->strictOn());
		prj->superFeaturesEnabled(prj_->superFeaturesEnabled());
		prj->borderFrameType(prj_->borderFrameType());
		prj->borderFrameCode(prj_->borderFrameCode());
		prj->superPalettes(prj_->superPalettes());
		prj->customizedSuperPalettes(prj_->customizedSuperPalettes());
		prj->modified(prj_->modified());

		prj->hasDirtyInformation(true);

		prj->save(path.c_str(), false, onError);

		// Dispose the project.
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		onPrint("End saving project property.");
		const std::string msg = "Completed in " + Text::toString(secs) + "s.";
		onPrint(msg.c_str());

		if (prj) {
			prj->unload();
			prj->close(true);
			prj = nullptr;
		}

		activeKernelIndex(oldKrnlIdx);

		prj_->hasDirtyInformation(false);
	};
	auto set = [this, prj, isOpened, save] (Project* prj_) -> void {
		// Prepare.
		const bool needReloadKernel =
			prj->kernel()        != prj_->kernel();
		const bool needReAnalyze =
			prj->kernel()        != prj_->kernel()        ||
			prj->cartridgeType() != prj_->cartridgeType() ||
			prj->sramType()      != prj_->sramType()      ||
			prj->hasRtc()        != prj_->hasRtc();

		// Apply the changes.
		const long long now = DateTime::now();
		if (!prj_->title().empty())
			prj->title(prj_->title());
		prj->kernel(prj_->kernel());
		prj->cartridgeType(prj_->cartridgeType());
		prj->sramType(prj_->sramType());
		prj->hasRtc(prj_->hasRtc());
		prj->description(prj_->description());
		prj->author(prj_->author());
		prj->genre(prj_->genre());
		prj->version(prj_->version());
		prj->url(prj_->url());
		prj->runOnOpen(prj_->runOnOpen());
		prj->iconCode(prj_->iconCode());
		prj->caseInsensitive(prj_->caseInsensitive());
		prj->strictOn(prj_->strictOn());
		prj->superFeaturesEnabled(prj_->superFeaturesEnabled());
		prj->borderFrameType(prj_->borderFrameType());
		prj->borderFrameCode(prj_->borderFrameCode());
		prj->superPalettes(prj_->superPalettes());
		prj->customizedSuperPalettes(prj_->customizedSuperPalettes());
		prj->modified(now);

		prj->hasDirtyInformation(true);

		// Save if needed.
		if (!isOpened) {
			save(prj);
		}

		// Reload kernel if needed.
		if (isOpened) {
			if (needReloadKernel) {
				const int kernelIdx = getValidKernelIndex(prj);
				activeKernelIndex(kernelIdx);

				clearLanguageDefinition(true);
			}
		}

		// Re-analyze if needed.
		if (isOpened) {
			if (needReAnalyze)
				analyze(true);
		}
	};

	ImGui::ProjectPropertyPopupBox::ConfirmedHandler confirm(
		[wnd, this, set, isOpened] (Project* prj_) -> void {
			if (!isOpened)
				join();

			set(prj_);

			if (isOpened)
				refreshWindowTitle(wnd);

			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::ProjectPropertyPopupBox::CanceledHandler cancel(
		[this, isOpened] (void) -> void {
			if (!isOpened)
				join();

			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::ProjectPropertyPopupBox::AppliedHandler apply(
		[wnd, this, set, isOpened] (Project* prj_) -> void {
			set(prj_);

			if (isOpened)
				refreshWindowTitle(wnd);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::ProjectPropertyPopupBox(
				rnd,
				theme(),
				theme()->windowProjectProperty(),
				prj,
				confirm, cancel, apply,
				theme()->generic_Ok().c_str(), theme()->generic_Cancel().c_str(), theme()->generic_Apply().c_str()
			)
		)
	);
}

void Workspace::showRomBuildSettings(
	Renderer* rnd,
	const ImGui::RomBuildSettingsPopupBox::ConfirmedHandler &confirm_,
	const ImGui::RomBuildSettingsPopupBox::CanceledHandler &cancel
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::RomBuildSettingsPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Build().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::RomBuildSettingsPopupBox::ConfirmedHandler(
			[&] (const char*, const char*, bool) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::RomBuildSettingsPopupBox(
				rnd,
				theme(),
				theme()->windowBuildingSettings(),
				currentProject().get(),
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::showEmulatorBuildSettings(
	Renderer* rnd,
	const std::string &settings, const char* args, bool hasIcon,
	const ImGui::EmulatorBuildSettingsPopupBox::ConfirmedHandler &confirm_,
	const ImGui::EmulatorBuildSettingsPopupBox::CanceledHandler &cancel
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::EmulatorBuildSettingsPopupBox::ConfirmedHandler confirm = confirm_;

	btnConfirm = theme()->generic_Build().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::EmulatorBuildSettingsPopupBox::ConfirmedHandler(
			[&] (const char*, const char*, Bytes::Ptr) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::EmulatorBuildSettingsPopupBox(
				rnd,
				input(), theme(),
				theme()->windowBuildingSettings(),
				settings, args, hasIcon,
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::showSearchResult(const std::string &pattern, const Editing::Tools::SearchResult::Array &found) {
	closeSearchResult();

	searchResultVisible(true);
	searchResultPattern(pattern);
	searchResult(found);
}

void Workspace::closeSearchResult(void) {
	searchResultVisible(false);
	searchResultPattern().clear();
	searchResult().clear();
	if (searchResultTextBox()) {
		EditorSearchResult* editor = (EditorSearchResult*)searchResultTextBox();
		editor->close(-1);
		EditorSearchResult::destroy(editor);
		searchResultTextBox(nullptr);
	}
}

void Workspace::showInstalledKernels(Window* wnd, Renderer* rnd, const char* prompt) {
	#define WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX 1

	ImGui::InstalledKernelsPopupBox::ConfirmedHandler confirm(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::InstalledKernelsPopupBox::AddedHandler add(
		[wnd, rnd, this] (void) -> void {
			Operations::kernelInstall(wnd, rnd, this)
				.then(
					[wnd, rnd, this] (std::string result) -> void { // Installed.
						clearLanguageDefinition(true);

#if WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX
						ImGui::MessagePopupBox::ConfirmedHandler confirm = ImGui::MessagePopupBox::ConfirmedHandler(
							[wnd, rnd, this] (void) -> void {
								showInstalledKernels(wnd, rnd, nullptr);
							},
							nullptr
						);
						messagePopupBox(result, confirm, nullptr, nullptr);
#else /* WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX */
						showInstalledKernels(wnd, rnd, result.c_str());
#endif /* WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX */
					}
				)
				.fail(
					[wnd, rnd, this] (std::string reason) -> void { // Error occurred.
#if WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX
						ImGui::MessagePopupBox::ConfirmedHandler confirm = ImGui::MessagePopupBox::ConfirmedHandler(
							[wnd, rnd, this] (void) -> void {
								showInstalledKernels(wnd, rnd, nullptr);
							},
							nullptr
						);
						messagePopupBox(reason, confirm, nullptr, nullptr);
#else /* WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX */
						showInstalledKernels(wnd, rnd, reason.c_str());
#endif /* WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX */
					}
				)
				.fail(
					[wnd, rnd, this] (void) -> void { // User canceled.
						showInstalledKernels(wnd, rnd, nullptr);
					}
				);
		},
		nullptr
	);
	ImGui::InstalledKernelsPopupBox::RemovedHandler remove(
		[wnd, rnd, this] (int idx) -> void {
			Operations::kernelUninstall(wnd, rnd, this, idx)
				.then(
					[wnd, rnd, this] (std::string result) -> void {
						clearLanguageDefinition(true);

#if WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX
						ImGui::MessagePopupBox::ConfirmedHandler confirm = ImGui::MessagePopupBox::ConfirmedHandler(
							[wnd, rnd, this] (void) -> void {
								showInstalledKernels(wnd, rnd, nullptr);
							},
							nullptr
						);
						messagePopupBox(result, confirm, nullptr, nullptr);
#else /* WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX */
						showInstalledKernels(wnd, rnd, result.c_str());
#endif /* WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX */
					}
				);
		},
		nullptr
	);
	ImGui::InstalledKernelsPopupBox::SourceCodeEjectingHandler ejectSourceCode = std::bind(
		&Workspace::ejectSourceCode, this,
		wnd, rnd,
		std::placeholders::_1, true,
		[wnd, rnd, this] (void) -> void {
			showInstalledKernels(wnd, rnd, theme()->dialogPrompt_EjectedSourceCode().c_str());
		},
		[wnd, rnd, this] (void) -> void {
			showInstalledKernels(wnd, rnd, nullptr);
		}
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::InstalledKernelsPopupBox(
				rnd,
				theme(),
				theme()->windowInstalledKernels(), prompt,
				kernels(),
				confirm, add, remove,
				theme()->generic_Ok().c_str(), theme()->generic_Install().c_str(), theme()->generic_Uninstall().c_str(),
				ejectSourceCode
			)
		)
	);

	#undef WORKSPACE_SHOW_INSTALLATION_MESSAGE_POPUP_BOX
}

void Workspace::showPreferences(Window* wnd, Renderer* rnd, const char* tab) {
	auto set = [wnd, rnd, this] (Settings &sets) -> void {
		do {
			Project::Ptr &prj = currentProject();

			if (prj && sets.mainIndentRule != settings().mainIndentRule) {
				for (int i = 0; i < prj->codePageCount(); ++i) {
					CodeAssets::Entry* entry = prj->getCode(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::SET_INDENT_RULE, (Variant::Int)sets.mainIndentRule);
				}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
				if (prj->minorCodeEditor()) {
					prj->minorCodeEditor()->post(Editable::SET_INDENT_RULE, (Variant::Int)sets.mainIndentRule);
				}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			}

			if (prj && sets.mainColumnIndicator != settings().mainColumnIndicator) {
				for (int i = 0; i < prj->codePageCount(); ++i) {
					CodeAssets::Entry* entry = prj->getCode(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::SET_COLUMN_INDICATOR, (Variant::Int)sets.mainColumnIndicator);
				}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
				if (prj->minorCodeEditor()) {
					prj->minorCodeEditor()->post(Editable::SET_COLUMN_INDICATOR, (Variant::Int)sets.mainColumnIndicator);
				}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			}

			if (prj && sets.mainShowProjectPathAtTitleBar != settings().mainShowProjectPathAtTitleBar) {
				settings().mainShowProjectPathAtTitleBar = sets.mainShowProjectPathAtTitleBar;

				refreshWindowTitle(wnd);
			}

			if (prj && sets.mainShowWhiteSpaces != settings().mainShowWhiteSpaces) {
				for (int i = 0; i < prj->codePageCount(); ++i) {
					CodeAssets::Entry* entry = prj->getCode(i);
					Editable* editor = entry->editor;
					if (editor)
						editor->post(Editable::SET_SHOW_SPACES, sets.mainShowWhiteSpaces);
				}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
				if (prj->minorCodeEditor()) {
					prj->minorCodeEditor()->post(Editable::SET_SHOW_SPACES, sets.mainShowWhiteSpaces);
				}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			}

			input()->config(sets.inputGamepads, INPUT_GAMEPAD_COUNT);

			if (sets.applicationWindowBorderless != settings().applicationWindowBorderless) {
				settings().applicationWindowBorderless = sets.applicationWindowBorderless;
				wnd->bordered(!settings().applicationWindowBorderless);
			}
			if (sets.applicationWindowFullscreen != settings().applicationWindowFullscreen) {
				toggleFullscreen(wnd);
			}
			if (sets.applicationWindowMaximized != settings().applicationWindowMaximized) {
				settings().applicationWindowMaximized = sets.applicationWindowMaximized;
			}

			if (canvasDevice()) {
				for (int i = 0; i < GBBASIC_COUNTOF(sets.deviceClassicPalette); ++i) {
					if (sets.deviceClassicPalette[i] != settings().deviceClassicPalette[i])
						canvasDevice()->classicPalette(i, sets.deviceClassicPalette[i]);
				}
			}

			if (sets.canvasFixRatio != settings().canvasFixRatio) {
				canvasFixRatio(sets.canvasFixRatio);
			}
			if (sets.canvasIntegerScale != settings().canvasIntegerScale) {
				canvasIntegerScale(sets.canvasIntegerScale);
			}
		} while (false);

		settings() = sets;
	};

	ImGui::PreferencesPopupBox::ConfirmedHandler confirm(
		[this, set] (Settings &sets) -> void {
			set(sets);

			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::PreferencesPopupBox::CanceledHandler cancel(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::PreferencesPopupBox::AppliedHandler apply(
		[set] (Settings &sets) -> void {
			set(sets);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::PreferencesPopupBox(
				rnd,
				input(),
				theme(),
				theme()->windowPreferences(),
				settings(),
				[wnd] (void) -> bool { return !wnd->fullscreen() && !wnd->maximized(); }, [wnd] (void) -> bool { return !wnd->bordered(); },
				tab ? tab : "",
				confirm, cancel, apply,
				theme()->generic_Ok().c_str(), theme()->generic_Cancel().c_str(), theme()->generic_Apply().c_str()
			)
		)
	);
}

void Workspace::showActivities(Renderer* rnd) {
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::ActivitiesPopupBox(
				rnd,
				theme()->windowActivities(),
				this,
				ImGui::ActivitiesPopupBox::ConfirmedHandler([&] (void) -> void { popupBox(nullptr); }, nullptr),
				theme()->generic_Ok().c_str()
			)
		)
	);
}

void Workspace::showAbout(Renderer* rnd) {
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::AboutPopupBox(
				rnd,
				theme()->windowAbout(),
				ImGui::AboutPopupBox::ConfirmedHandler([&] (void) -> void { popupBox(nullptr); }, nullptr),
				theme()->generic_Ok().c_str(),
				exporterLicenses().c_str()
			)
		)
	);
}

std::string Workspace::getSourceCodePath(int index, std::string* name_) const {
	if (name_)
		name_->clear();

	if (kernels().empty())
		return "";

	if (index < 0 || index >= (int)kernels().size())
		return "";

	const GBBASIC::Kernel::Ptr &krnl = kernels()[index];
	const std::string &path = krnl->path();
	std::string dir;
	Path::split(path, nullptr, nullptr, &dir);
	const std::string &srcPath = krnl->kernelSourceCode();
	std::string name;
	Path::split(srcPath, &name, nullptr, nullptr);

	std::string src = Path::combine(dir.c_str(), (name + ".zip").c_str());
	src = Path::absoluteOf(src);

	if (name_)
		*name_ = name;

	return src;
}

void Workspace::ejectSourceCode(Window* wnd, Renderer* rnd, int index, bool wait, KernelEjectionHandler ejected, KernelEjectionHandler canceled) {
	if (kernels().empty()) {
		const std::string msg = "No valid kernel.";
		error(msg.c_str());

		if (canceled)
			canceled();

		return;
	}

	std::string name;
	const std::string src = getSourceCodePath(index, &name);
	if (!Path::fileExists(src.c_str())) {
		const std::string msg = "Cannot find source code \"" + src + "\".";
		error(msg.c_str());

		if (canceled)
			canceled();

		return;
	}

	auto next = [wnd, rnd, this, ejected, canceled, name, src] (promise::Defer df) -> void {
#if defined GBBASIC_OS_HTML
		pfd::save_file save(
			theme()->generic_SaveTo(),
			Text::sanitizeFilename(name) + ".zip",
			GBBASIC_JSON_FILE_FILTER
		);
		std::string path = save.result();
		Path::uniform(path);
		if (path.empty()) {
			if (canceled)
				canceled();

			return;
		}
		std::string ext;
		Path::split(path, nullptr, &ext, nullptr);
		Text::toLowerCase(ext);
		if (ext.empty() || ext != "zip")
			path += ".zip";

		if (!Path::copyFile(src.c_str(), path.c_str())) {
			const std::string msg = "Cannot copy source code to \"" + path + "\".";
			error(msg.c_str());

			if (canceled)
				canceled();

			return;
		}
#else /* Platform macro. */
		pfd::select_folder dir(
			theme()->generic_SaveTo()
		);
		std::string path = dir.result();
		Path::uniform(path);
		if (path.empty()) {
			if (canceled)
				canceled();

			return;
		}

		path = Path::combine(path.c_str(), name.c_str());
		DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(path.c_str());
		if (!dirInfo->exists())
			dirInfo->make();

		Archive::Ptr arc(Archive::create(Archive::ZIP));
		if (!arc->open(src.c_str(), Stream::READ)) {
			const std::string msg = "Cannot read the source code \"" + src + "\".";

			if (canceled)
				canceled();

			return;
		}
		if (!arc->toDirectory(path.c_str())) {
			const std::string msg = "Cannot extract source code to \"" + path + "\".";

			if (canceled)
				canceled();

			return;
		}
		arc->close();

		std::string path_ = dirInfo->parentPath();
		path_ = Unicode::toOs(path_);
		Platform::browse(path_.c_str());
#endif /* Platform macro. */

		bubble(theme()->dialogPrompt_EjectedSourceCode(), nullptr);

		if (ejected)
			ejected();
	};

	promise::Promise begin = wait ?
		Operations::popupWait(wnd, rnd, this, true, theme()->dialogPrompt_Ejecting().c_str(), true, false) :
		Operations::always(wnd, rnd, this);
	begin
		.then(
			[next] (void) -> promise::Promise {
				return promise::newPromise(next);
			}
		);
}

void Workspace::toggleDocument(const char* path) {
	if (documents().empty()) {
		const std::string osstr = Unicode::toOs("https://paladin-t.github.io/kits/gbb/manual.html");

		Platform::surf(osstr.c_str());

		return;
	}

	const char* shown = document() ? document()->shown() : nullptr;
	const bool close = !path || (shown ? strcmp(shown, path) == 0 : false);

	if (document() && close) {
		Document::destroy(document());
		document(nullptr);

		documentTitle().clear();
	} else {
		if (document()) {
			Document::destroy(document());
			document(nullptr);
		}

		if (!path)
			path = DOCUMENT_MARKDOWN_DIR "Manual." DOCUMENT_MARKDOWN_EXT;

		FileInfo::Ptr fileInfo = FileInfo::make(path);
		documentTitle("[" + fileInfo->fileName() + "]");

		document(Document::create());
		document()->show(path);

		closeFilter();
	}
}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
void Workspace::createMinorCodeEditor(void) {
	Project::Ptr &prj = currentProject();

	if (!prj)
		return;

	if (prj->minorCodeEditor())
		return;

	prj->minorCodeEditor(EditorCode::create(this, false));
}

void Workspace::destroyMinorCodeEditor(void) {
	Project::Ptr &prj = currentProject();

	if (!prj)
		return;

	if (!prj->minorCodeEditor())
		return;

	EditorCode::destroy(prj->minorCodeEditor());
	prj->minorCodeEditor(nullptr);
}

void Workspace::syncMajorCodeEditorDataToMinor(Window*, Renderer*) {
	Project::Ptr &prj = currentProject();

	if (!prj)
		return;

	if (!prj->isMinorCodeEditorEnabled())
		return;

	if (prj->activeMinorCodeIndex() == -1)
		return;

	EditorCode* minorEditor = prj->minorCodeEditor();
	if (!minorEditor)
		return;
	const int minorIndex = prj->activeMinorCodeIndex();

	const int majorIndex = minorIndex;
	CodeAssets::Entry* majorEntry = prj->getCode(majorIndex);
	if (!majorEntry)
		return;
	EditorCode* majorEditor = (EditorCode*)majorEntry->editor;
	if (!majorEditor)
		return;

	majorEditor->post(Editable::SYNC_DATA, (Variant::Int)1, (void*)minorEditor, (Variant::Int)minorIndex);
}

void Workspace::syncMinorCodeEditorDataToMajor(Window* wnd, Renderer* rnd) {
	Project::Ptr &prj = currentProject();

	if (!prj)
		return;

	if (!prj->isMinorCodeEditorEnabled())
		return;

	if (prj->activeMinorCodeIndex() == -1)
		return;

	EditorCode* minorEditor = prj->minorCodeEditor();
	if (!minorEditor)
		return;
	const int minorIndex = prj->activeMinorCodeIndex();

	const int majorIndex = minorIndex;
	CodeAssets::Entry* majorEntry = prj->getCode(majorIndex);
	if (!majorEntry)
		return;
	EditorCode* majorEditor = touchCodeEditor(wnd, rnd, prj.get(), majorIndex, true, majorEntry);
	majorEditor->post(Editable::SYNC_DATA, (Variant::Int)0, (void*)majorEditor, (Variant::Int)majorIndex);
}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

void Workspace::createAudioDevice(void) {
	if (audioDevice())
		return;

	audioDevice(Device::Ptr(Device::create(Device::CoreTypes::BINJGB, this)));
}

void Workspace::destroyAudioDevice(void) {
	if (!audioDevice())
		return;

	if (audioDevice()->opened())
		audioDevice()->close(nullptr);
	audioDevice(nullptr);
}

void Workspace::openAudioDevice(Bytes::Ptr rom) {
	audioDevice()->open(
		rom,
		Device::DeviceTypes::CLASSIC, false,
		nullptr,
		nullptr,
		true, true, true
	);
}

void Workspace::closeAudioDevice(void) {
	if (!audioDevice())
		return;

	audioDevice()->close(nullptr);
}

void Workspace::updateAudioDevice(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq, Device::AudioHandler handleAudio) {
	if (!audioDevice())
		return;
	if (!audioDevice()->opened())
		return;

	*fpsReq = GBBASIC_ACTIVE_FRAME_RATE;

	audioDevice()->update(wnd, rnd, delta, nullptr, nullptr, false, nullptr, handleAudio);
}

bool Workspace::delay(Delayed::Handler delayed_, const std::string &key, const Delayed::Amount &delay_) {
	if (!key.empty()) {
		for (const Delayed &d : delayed()) {
			if (d.key == key)
				return false;
		}
	}

	delayed().push_back(Delayed(delayed_, key, delay_));

	return true;
}

void Workspace::clearDelayed(void) {
	delayed().clear();
}

Semaphore Workspace::async(WorkTaskFunction::OperationHandler op, WorkTaskFunction::SealHandler seal, WorkTaskFunction::DisposeHandler dtor) {
	if (!workQueue())
		return Semaphore();

	if (!op)
		return Semaphore();

	Semaphore::StatusPtr working(new bool(true));
	Semaphore result(std::bind(&Workspace::wait, this), working);

	if (dtor) {
		WorkTaskFunction::DisposeHandler dtor_ = dtor;
		dtor = [dtor_, working] (WorkTask* task, uintptr_t tmp) -> void { // On main thread.
			dtor_(task, tmp);

			*working = false;

			task->disassociated(true);
		};
	} else {
		dtor = [] (WorkTask* task, uintptr_t) -> void { // On main thread.
			task->disassociated(true);
		};
	}

	workQueue()->push(
		WorkTaskFunction::create(op /* on work thread */, seal /* on main thread */, dtor /* on main thread */)
	);

	return result;
}

void Workspace::join(void) {
	if (!workQueue())
		return;

	constexpr const int STEP = 10;
	while (!workQueue()->allProcessed()) {
		workQueue()->update();
		DateTime::sleep(STEP);
	}
}

void Workspace::wait(void) {
	if (!workQueue())
		return;

	if (workQueue()->allProcessed())
		return;

	constexpr const int STEP = 10;
	workQueue()->update();
	DateTime::sleep(STEP);
}

bool Workspace::working(void) const {
	if (!workQueue())
		return false;

	if (workQueue()->allProcessed())
		return false;

	return true;
}

bool Workspace::analyzing(void) const {
	if (!staticAnalyzer())
		return false;

	return staticAnalyzer()->analyzing();
}

bool Workspace::analyze(bool force) {
	if (!staticAnalyzer())
		return true;

	const Project::Ptr &prj = currentProject();
	if (!prj)
		return true;
	if (prj->contentType() != Project::ContentTypes::BASIC)
		return true;

	if (!force) {
		if (popupBox() || menuOpened())
			return false;
	}

	AssetsBundle::Ptr assets(new AssetsBundle());
	prj->assets()->clone(
		assets.get(),
		[] (const FontAssets::Entry* entry) -> FontAssets::Entry {
			return *entry;
		},
		[] (const CodeAssets::Entry* entry) -> std::string {
			std::string result = entry->data;
			EditorCode* codeEditor = (EditorCode*)entry->editor;
			if (codeEditor) {
				const std::string &txt = codeEditor->toString();
				if (!txt.empty())
					result = txt;
			}

			return result;
		}
	);

	auto finish = [] (Workspace* ws) -> void {
		ws->clearLanguageDefinition(false);
		ws->clearAnalyzedCodeInformation();
		ws->clearCodePageNames();

		ws->needAnalyzing(false);
	};

	if (kernels().empty()) {
		finish(this);

		return true;
	}

	const GBBASIC::Kernel::Ptr &krnl = activeKernel();

	staticAnalyzer()->analyze(
		krnl.get(),
		assets,
		[this, finish] (void) -> void { // On main thread.
			const Project::Ptr &prj = currentProject();
			if (!prj)
				return;

			finish(this);
		}
	);

	return true;
}

void Workspace::clearAnalyzingResult(void) {
	if (!staticAnalyzer())
		return;

	staticAnalyzer()->clear();

	clearLanguageDefinition(false);
	clearAnalyzedCodeInformation();
	clearAssetPageNames();
}

void Workspace::clearLanguageDefinition(bool clearRevision) {
	const Project::Ptr &prj = currentProject();
	if (!prj || !prj->assets())
		return;

	for (int i = 0; i < prj->codePageCount(); ++i) {
		CodeAssets::Entry* entry = prj->getCode(i);
		Editable* editor = entry->editor;
		if (editor)
			editor->post(Editable::CLEAR_LANGUAGE_DEFINITION, clearRevision);
	}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	if (prj->minorCodeEditor()) {
		prj->minorCodeEditor()->post(Editable::CLEAR_LANGUAGE_DEFINITION, clearRevision);
	}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
}

unsigned Workspace::getLanguageDefinitionRevision(void) const {
	if (!staticAnalyzer())
		return 0;

	return staticAnalyzer()->getLanguegeDefinitionRevision();
}

const GBBASIC::Macro::List* Workspace::getMacroDefinitions(void) {
	if (!staticAnalyzer())
		return nullptr;

	return staticAnalyzer()->getMacroDefinitions();
}

const Text::Array* Workspace::getDestinitions(void) {
	if (!staticAnalyzer())
		return nullptr;

	return staticAnalyzer()->getDestinitions();
}

void Workspace::clearAnalyzedCodeInformation(void) {
	analyzedCodeInformation().clear();
}

const std::string &Workspace::getAnalyzedCodeInformation(void) {
	if (!analyzedCodeInformation().empty())
		return analyzedCodeInformation();

	if (!staticAnalyzer())
		return analyzedCodeInformation();

	int heapAllocCount = 0;
	int heapAllocBytes = 0;
	int heapAllocDetails[(unsigned)GBBASIC::RamLocation::Usages::COUNT];
	memset(heapAllocDetails, 0, sizeof(heapAllocDetails));
	const GBBASIC::RamLocation::Dictionary* allocations = staticAnalyzer()->getRamAllocations();
	for (GBBASIC::RamLocation::Dictionary::value_type kv : *allocations) {
		const GBBASIC::RamLocation &alloc = kv.second;
		if (alloc.type == GBBASIC::RamLocation::Types::HEAP) {
			++heapAllocCount;
			heapAllocBytes += alloc.size;
			++heapAllocDetails[(unsigned)alloc.usage];
		}
	}

	const std::string allocationInfo = Text::format(
		theme()->tooltipCode_Info(),
		{
			Text::toString(heapAllocCount),
			Text::toString(heapAllocBytes / 2),
			Text::toString(heapAllocDetails[(unsigned)GBBASIC::RamLocation::Usages::VARIABLE]),
			Text::toString(heapAllocDetails[(unsigned)GBBASIC::RamLocation::Usages::ARRAY]),
			Text::toString(heapAllocDetails[(unsigned)GBBASIC::RamLocation::Usages::LOOP]),
			heapAllocCount <= 1 ? theme()->tooltipCode_InfoAllocation() : theme()->tooltipCode_InfoAllocations(),
			(heapAllocBytes / 2) <= 1 ? theme()->tooltipCode_InfoWord() : theme()->tooltipCode_InfoWords()
		}
	);
	_analyzedCodeInformation = allocationInfo;

	return analyzedCodeInformation();
}

void Workspace::clearAssetPageNames(void) {
	assetPageNames().clear();
}

void Workspace::clearFontPageNames(void) {
	assetPageNames().font.clear();
}

const Text::Array &Workspace::getFontPageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->fontPageCount();
	if ((int)assetPageNames().font.size() < n) {
		assetPageNames().font.resize(n);
		assetPageNames().font.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const FontAssets::Entry* entry = prj->getFont(i);
			if (entry && !entry->name.empty()) {
				pg += " (";
				pg += entry->name;
				pg += ")";
			}
			assetPageNames().font[i] = pg;
		}
	}

	return assetPageNames().font;
}

void Workspace::clearCodePageNames(void) {
	assetPageNames().code.clear();
}

const Text::Array &Workspace::getCodePageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->codePageCount();
	if ((int)assetPageNames().code.size() < n) {
		assetPageNames().code.resize(n);
		assetPageNames().code.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			if (staticAnalyzer()) {
				const GBBASIC::StaticAnalyzer::CodePageName* name = staticAnalyzer()->getCodePageName(i);
				if (name && !name->name.empty()) {
					pg += " (";
					pg += name->name;
					pg += ")";
				}
			}
			assetPageNames().code[i] = pg;
		}
	}

	return assetPageNames().code;
}

void Workspace::clearTilesPageNames(void) {
	assetPageNames().tiles.clear();
}

const Text::Array &Workspace::getTilesPageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->tilesPageCount();
	if ((int)assetPageNames().tiles.size() < n) {
		assetPageNames().tiles.resize(n);
		assetPageNames().tiles.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const TilesAssets::Entry* entry = prj->getTiles(i);
			if (entry && !entry->name.empty()) {
				pg += " (";
				pg += entry->name;
				pg += ")";
			}
			assetPageNames().tiles[i] = pg;
		}
	}

	return assetPageNames().tiles;
}

void Workspace::clearMapPageNames(void) {
	assetPageNames().map.clear();
}

const Text::Array &Workspace::getMapPageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->mapPageCount();
	if ((int)assetPageNames().map.size() < n) {
		assetPageNames().map.resize(n);
		assetPageNames().map.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const MapAssets::Entry* entry = prj->getMap(i);
			if (entry && !entry->name.empty()) {
				pg += " (";
				pg += entry->name;
				pg += ")";
			}
			assetPageNames().map[i] = pg;
		}
	}

	return assetPageNames().map;
}

void Workspace::clearMusicPageNames(void) {
	assetPageNames().music.clear();
}

const Text::Array &Workspace::getMusicPageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->musicPageCount();
	if ((int)assetPageNames().music.size() < n) {
		assetPageNames().music.resize(n);
		assetPageNames().music.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const MusicAssets::Entry* entry = prj->getMusic(i);
			if (entry && entry->data && entry->data->pointer() && !entry->data->pointer()->name.empty()) {
				pg += " (";
				pg += entry->data->pointer()->name;
				pg += ")";
			}
			assetPageNames().music[i] = pg;
		}
	}

	return assetPageNames().music;
}

void Workspace::clearSfxPageNames(void) {
	assetPageNames().sfx.clear();
}

const Text::Array &Workspace::getSfxPageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->sfxPageCount();
	if ((int)assetPageNames().sfx.size() < n) {
		assetPageNames().sfx.resize(n);
		assetPageNames().sfx.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const SfxAssets::Entry* entry = prj->getSfx(i);
			if (entry && entry->data && entry->data->pointer() && !entry->data->pointer()->name.empty()) {
				pg += " (";
				pg += entry->data->pointer()->name;
				pg += ")";
			}
			assetPageNames().sfx[i] = pg;
		}
	}

	return assetPageNames().sfx;
}

void Workspace::clearActorPageNames(void) {
	assetPageNames().actor.clear();
}

const Text::Array &Workspace::getActorPageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->actorPageCount();
	if ((int)assetPageNames().actor.size() < n) {
		assetPageNames().actor.resize(n);
		assetPageNames().actor.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const ActorAssets::Entry* entry = prj->getActor(i);
			if (entry && !entry->name.empty()) {
				pg += " (";
				pg += entry->name;
				pg += ")";
			}
			assetPageNames().actor[i] = pg;
		}
	}

	return assetPageNames().actor;
}

void Workspace::clearScenePageNames(void) {
	assetPageNames().scene.clear();
}

const Text::Array &Workspace::getScenePageNames(void) {
	const Project::Ptr &prj = currentProject();

	const int n = prj->scenePageCount();
	if ((int)assetPageNames().scene.size() < n) {
		assetPageNames().scene.resize(n);
		assetPageNames().scene.shrink_to_fit();

		for (int i = 0; i < n; ++i) {
			std::string pg = theme()->menu_Page() + " " + Text::toString(i);
			const SceneAssets::Entry* entry = prj->getScene(i);
			if (entry && !entry->name.empty()) {
				pg += " (";
				pg += entry->name;
				pg += ")";
			}
			assetPageNames().scene[i] = pg;
		}
	}

	return assetPageNames().scene;
}

void Workspace::upgrade(
	Window* wnd, Renderer* rnd,
	const Text::Dictionary &arguments
) {
	// Parse the arguments.
	bool commandlineOnly = false;
	Text::Dictionary::const_iterator cloOpt = arguments.find(WORKSPACE_OPTION_APPLICATION_COMMANDLINE_ONLY_KEY);
	if (cloOpt != arguments.end())
		commandlineOnly = true;
	bool doNotQuit = false;
	Text::Dictionary::const_iterator dnqOpt = arguments.find(WORKSPACE_OPTION_APPLICATION_DO_NOT_QUIT_KEY);
	if (dnqOpt != arguments.end())
		doNotQuit = true;

	// Initialize the workspace.
	category(Categories::CONSOLE);
	tabsWidth(0.0f);
	interactable(doNotQuit);

	// Initialize the output methods.
	PrintHandler onPrint = nullptr;
	WarningOrErrorHandler onError = nullptr;

	if (commandlineOnly) {
		auto onPrint_ = [] (Workspace*, const char* msg) -> void {
			fprintf(stdout, "%s\n", msg);
		};
		auto onError_ = [] (Workspace*, const char* msg, bool isWarning) -> void {
			if (isWarning)
				fprintf(stdout, "%s\n", msg);
			else
				fprintf(stderr, "%s\n", msg);
		};
		onPrint = std::bind(onPrint_, this, std::placeholders::_1);
		onError = std::bind(onError_, this, std::placeholders::_1, std::placeholders::_2);
	} else {
		auto onPrint_ = [] (Workspace* ws, const char* msg) -> void {
			ws->print(msg);
		};
		auto onError_ = [] (Workspace* ws, const char* msg, bool isWarning) -> void {
			if (isWarning)
				ws->warn(msg);
			else
				ws->error(msg);
		};
		onPrint = std::bind(onPrint_, this, std::placeholders::_1);
		onError = std::bind(onError_, this, std::placeholders::_1, std::placeholders::_2);
	}

	// Load the project.
	const int oldKrnlIdx = activeKernelIndex();

	onPrint(GBBASIC_TITLE " v" GBBASIC_VERSION_STRING);
	onPrint("");

	onPrint("Begin upgrading project.");
	const long long start = DateTime::ticks();

	Project::Ptr prj = nullptr;
	do {
		if (!rnd)
			break;

		Text::Dictionary::const_iterator srcOpt = arguments.find(COMPILER_INPUT_OPTION_KEY);
		if (srcOpt == arguments.end()) {
			onError("No input project.", false);

			break;
		}

		const std::string &path = srcOpt->second;
		prj = Project::Ptr(new Project(wnd, rnd, this));
		if (!prj->open(path.c_str())) {
			onError("Cannot open the project.", false);

			break;
		}

		Texture::Ptr attribtex(theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Texture::Ptr propstex(theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Texture::Ptr actorstex(theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
		prj->attributesTexture(attribtex);
		prj->propertiesTexture(propstex);
		prj->actorsTexture(actorstex);

		prj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, this, std::placeholders::_1));
		prj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, this, std::placeholders::_1));

		const int kernelIdx = getValidKernelIndex(prj.get());
		activeKernelIndex(kernelIdx);

		std::string fontConfigPath;
		Text::Dictionary::const_iterator fntOpt = arguments.find(COMPILER_FONT_OPTION_KEY);
		if (fntOpt != arguments.end())
			fontConfigPath = fntOpt->second;
		else
			fontConfigPath = WORKSPACE_FONT_DEFAULT_CONFIG_FILE;
		if (!prj->load(false, fontConfigPath.c_str(), nullptr)) {
			prj->close(true);

			activeKernelIndex(oldKrnlIdx);

			onError("Cannot load the project.", false);

			break;
		}
	} while (false);

	// Upgrade the project.
	if (prj && prj->assets()) {
		// Set version.
		if (!_toUpgradeToVersion.empty() && _toUpgradeToVersion != "*")
			prj->version(_toUpgradeToVersion);

		// Tidy.
		prj->preferencesFontSize(Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE));
		prj->preferencesFontOffset(GBBASIC_FONT_DEFAULT_OFFSET);
		prj->preferencesFontIsTwoBitsPerPixel(GBBASIC_FONT_DEFAULT_IS_2BPP);
		prj->preferencesFontPreferFullWord(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD);
		prj->preferencesFontPreferFullWordForNonAscii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII);
		prj->preferencesPreviewAnchorPoint(true);
		prj->preferencesPreviewPaletteBits(true);
		prj->preferencesUseByteMatrix(false);
		prj->preferencesShowGrids(true);
		prj->preferencesCodePageForBindedRoutine(0);
		prj->preferencesCodeLineForBindedRoutine(Left<int>(0));
		prj->preferencesMapRef(0);
		prj->preferencesMusicPreviewStroke(true);
		prj->preferencesSfxShowSoundShape(true);
		prj->preferencesActorApplyPropertiesToAllTiles(false);
		prj->preferencesActorApplyPropertiesToAllFrames(true);
		prj->preferencesActorUses8x16Sprites(true);
		prj->preferencesSceneRefMap(0);
		prj->preferencesSceneShowActors(true);
		prj->preferencesSceneShowTriggers(true);
		prj->preferencesSceneShowProperties(true);
		prj->preferencesSceneShowAttributes(false);
		prj->preferencesSceneUseGravity(false);

		// Save.
		const std::string &path = prj->path();
		prj->hasDirtyInformation(true);
		prj->save(path.c_str(), false, onError);
	}

	// Dispose the project.
	const long long end = DateTime::ticks();
	const long long diff = end - start;
	const double secs = DateTime::toSeconds(diff);
	onPrint("End upgrading project.");
	const std::string msg = "Completed in " + Text::toString(secs) + "s.";
	onPrint(msg.c_str());

	if (prj) {
		prj->unload();
		prj->close(true);
		prj = nullptr;
	}

	activeKernelIndex(oldKrnlIdx);

	// Finish.
	if (!doNotQuit) {
		SDL_Event evt;
		evt.type = SDL_QUIT;
		SDL_PushEvent(&evt);
	}
}

void Workspace::joinCompiling(void) {
#if GBBASIC_MULTITHREAD_ENABLED
	if (_thread.joinable())
		_thread.join();
#endif /* GBBASIC_MULTITHREAD_ENABLED */
}

void Workspace::compile(
	Window* wnd, Renderer* rnd,
	Project::Ptr prj,
	const Text::Dictionary &arguments
) {
	// The compiling procedure.
	auto proc = [] (CompilingParameters* params) -> void {
		/**< Prepare. */

		Workspace* self = params->self;
		GBBASIC::Kernel::Ptr kernel = params->kernel;
		Project::Ptr project = params->project;
		const Text::Dictionary &arguments = params->arguments;
		const bool commandlineOnly = params->commandlineOnly;
		const bool doNotQuit = params->doNotQuit;

		std::string config;
		std::string rom;
		std::string sym;
		std::string aliases;
		int bootstrap = -1;
		std::string heapSize;
		std::string stackSize;
		if (kernel) {
			config = kernel->path();
			std::string dir;
			Path::split(kernel->path(), nullptr, nullptr, &dir);
			rom = Path::combine(dir.c_str(), kernel->kernelRom().c_str());
			sym = Path::combine(dir.c_str(), kernel->kernelSymbols().c_str());
			aliases = Path::combine(dir.c_str(), kernel->kernelAliases().c_str());
			bootstrap = kernel->bootstrapBank();
			heapSize = kernel->memoryHeapSize();
			stackSize = kernel->memoryStackSize();
		}

		std::string cartType = PROJECT_CARTRIDGE_TYPE_CLASSIC PROJECT_CARTRIDGE_TYPE_SEPARATOR PROJECT_CARTRIDGE_TYPE_COLORED;
		std::string sramType = PROJECT_SRAM_TYPE_32KB;
		bool hasRtc = true;
		bool caseInsensitive = true;
		bool strictOn = true;
		if (project) {
			if (!project->cartridgeType().empty())
				cartType = project->cartridgeType();
			if (!project->sramType().empty())
				sramType = project->sramType();
			hasRtc = project->hasRtc();
			caseInsensitive = project->caseInsensitive();
			strictOn = project->strictOn();
		}

#if GBBASIC_MULTITHREAD_ENABLED
		if (params->threaded) {
			Platform::threadName("COMPILE");

			Text::locale("C");

			Math::srand();
		}
#endif /* GBBASIC_MULTITHREAD_ENABLED */

		GBBASIC::Program program;
		GBBASIC::Options options;

		if (project) {
			AssetsBundle::Ptr assets(new AssetsBundle());
			project->assets()->clone(
				assets.get(),
				nullptr,
				[] (const CodeAssets::Entry* entry) -> std::string {
					std::string result = entry->data;
					EditorCode* codeEditor = (EditorCode*)entry->editor;
					if (codeEditor) {
						const std::string &txt = codeEditor->toString();
						if (!txt.empty())
							result = txt;
					}

					return result;
				}
			);
			program.assets = assets;
		}

		/**< Initialize the compiler options. */

		// Initialize the basic options.
		if (!project) {
			Text::Dictionary::const_iterator srcOpt = arguments.find(COMPILER_INPUT_OPTION_KEY);
			if (srcOpt != arguments.end())
				options.input = Unicode::fromOs(srcOpt->second);
		}
		Text::Dictionary::const_iterator dstOpt = arguments.find(COMPILER_OUTPUT_OPTION_KEY);
		if (dstOpt != arguments.end())
			options.output = dstOpt->second;
		options.config = config;
		Text::Dictionary::const_iterator vmOpt = arguments.find(COMPILER_ROM_OPTION_KEY);
		if (vmOpt != arguments.end())
			options.rom = vmOpt->second;
		else if (!rom.empty())
			options.rom = rom;
		Text::Dictionary::const_iterator symOpt = arguments.find(COMPILER_SYM_OPTION_KEY);
		if (symOpt != arguments.end())
			options.sym = symOpt->second;
		else if (!sym.empty())
			options.sym = sym;
		Text::Dictionary::const_iterator aliasesOpt = arguments.find(COMPILER_ALIASES_OPTION_KEY);
		if (aliasesOpt != arguments.end())
			options.aliases = aliasesOpt->second;
		else if (!aliases.empty())
			options.aliases = aliases;
		Text::Dictionary::const_iterator fntOpt = arguments.find(COMPILER_FONT_OPTION_KEY);
		if (fntOpt != arguments.end())
			options.font = fntOpt->second;
		else
			options.font = WORKSPACE_FONT_DEFAULT_CONFIG_FILE;
		Text::Dictionary::const_iterator astOpt = arguments.find(COMPILER_AST_OPTION_KEY);
		if (vmOpt != arguments.end())
			options.ast = astOpt->second;

		// Initialize the cartridge options.
		if (project) {
			Bytes::Ptr iconTiles(Bytes::create());
			if (!!project->touchIconImage2Bpp(iconTiles)) {
				options.icon = iconTiles;
			}

			Image::Ptr borderImage = nullptr;
			if (project->superFeaturesEnabled()) {
				switch (project->borderFrameType()) {
				case Project::BorderFrameTypes::DEFAULT: {
						Image::Ptr img(Image::create());
						if (!img->fromBytes((Byte*)RES_IMAGE_DEFAULT_BORDER, GBBASIC_COUNTOF(RES_IMAGE_DEFAULT_BORDER)))
							break;

						borderImage = img;
					}

					break;
				case Project::BorderFrameTypes::CUSTOM: {
						if (project->borderFrameCode().empty())
							break;

						Bytes::Ptr bytes(Bytes::create());
						if (!Base64::toBytes(bytes.get(), project->borderFrameCode()))
							break;

						Image::Ptr img(Image::create());
						if (!img->fromBytes(bytes.get()))
							break;

						borderImage = img;
					}

					break;
				default: // Do nothing.
					break;
				}
			}
			options.superFeatures.enabled = project->superFeaturesEnabled();
			options.superFeatures.border = borderImage;
			options.superFeatures.palettes = GBBASIC::Options::SuperFeatures::Palettes(project->superPalettes().begin(), project->superPalettes().end());

			options.backgroundPalettes = project->backgroundPalettes();
			options.spritePalettes = project->spritePalettes();
		}
		Text::Dictionary::const_iterator titOpt = arguments.find(COMPILER_TITLE_OPTION_KEY);
		if (titOpt != arguments.end()) {
			options.title = Text::trim(titOpt->second);
		} else {
			if (project)
				options.title = project->title();
			else
				options.title = GBBASIC_NONAME_PROJECT_NAME;
		}
		Text::Dictionary::const_iterator cartOpt = arguments.find(COMPILER_CART_TYPE_OPTION_KEY);
		if (cartOpt != arguments.end()) {
			cartType = cartOpt->second;
		}
		if (!cartType.empty()) {
			options.strategies.compatibility = GBBASIC::Options::Strategies::Compatibilities::NONE;
			const Text::Array parts = Text::split(cartType, PROJECT_CARTRIDGE_TYPE_SEPARATOR);
			for (const std::string &part : parts) {
				if (part == PROJECT_CARTRIDGE_TYPE_CLASSIC)
					options.strategies.compatibility |= GBBASIC::Options::Strategies::Compatibilities::CLASSIC;
				else if (part == PROJECT_CARTRIDGE_TYPE_COLORED)
					options.strategies.compatibility |= GBBASIC::Options::Strategies::Compatibilities::COLORED;
				else if (part == PROJECT_CARTRIDGE_TYPE_EXTENSION)
					options.strategies.compatibility |= GBBASIC::Options::Strategies::Compatibilities::EXTENSION;
			}
		}
		Text::Dictionary::const_iterator ramOpt = arguments.find(COMPILER_RAM_TYPE_OPTION_KEY);
		if (ramOpt != arguments.end())
			Text::fromString(ramOpt->second, options.strategies.sramType);
		else if (!sramType.empty())
			Text::fromString(sramType, options.strategies.sramType);
		Text::Dictionary::const_iterator rtcOpt = arguments.find(COMPILER_RTC_OPTION_KEY);
		if (rtcOpt != arguments.end())
			Text::fromString(rtcOpt->second, options.strategies.cartridgeHasRtc);
		else
			options.strategies.cartridgeHasRtc = hasRtc;

		// Initialize the compiler options.
		Text::Dictionary::const_iterator csiOpt = arguments.find(COMPILER_CASE_INSENSITIVE_OPTION_KEY);
		if (csiOpt != arguments.end())
			Text::fromString(csiOpt->second, options.strategies.caseInsensitive);
		else
			options.strategies.caseInsensitive = caseInsensitive;
		Text::Dictionary::const_iterator strOpt = arguments.find(COMPILER_STRICT_ON_OPTION_KEY);
		if (strOpt != arguments.end())
			Text::fromString(strOpt->second, options.strategies.strictOn);
		else
			options.strategies.strictOn = strictOn;
		Text::Dictionary::const_iterator elnOpt = arguments.find(COMPILER_EXPLICIT_LINE_NUMBER_OPTION_KEY);
		if (elnOpt != arguments.end())
			options.strategies.completeLineNumber = false;
		Text::Dictionary::const_iterator dcrOpt = arguments.find(COMPILER_DECLARATION_REQUIRED_OPTION_KEY);
		if (dcrOpt != arguments.end())
			Text::fromString(dcrOpt->second, options.strategies.declarationRequired);
		Text::Dictionary::const_iterator bootOpt = arguments.find(COMPILER_BOOTSTRAP_OPTION_KEY);
		if (bootOpt != arguments.end())
			Text::fromString(bootOpt->second, options.strategies.bootstrapBank);
		else if (bootstrap >= 0)
			options.strategies.bootstrapBank = bootstrap;
		Text::Dictionary::const_iterator hpsOpt = arguments.find(COMPILER_HEAP_SIZE_OPTION_KEY);
		if (hpsOpt != arguments.end())
			Text::fromString(hpsOpt->second, options.strategies.heapSize);
		else if (!heapSize.empty())
			Text::fromString(heapSize, options.strategies.heapSize);
		Text::Dictionary::const_iterator stkOpt = arguments.find(COMPILER_STACK_SIZE_OPTION_KEY);
		if (stkOpt != arguments.end())
			Text::fromString(stkOpt->second, options.strategies.stackSize);
		else if (!stackSize.empty())
			Text::fromString(stackSize, options.strategies.stackSize);
		Text::Dictionary::const_iterator opcOpt = arguments.find(COMPILER_OPTIMIZE_CODE_OPTION_KEY);
		if (opcOpt != arguments.end())
			Text::fromString(opcOpt->second, options.strategies.optimizeCode);
		Text::Dictionary::const_iterator oppOpt = arguments.find(COMPILER_OPTIMIZE_ASSETS_OPTION_KEY);
		if (oppOpt != arguments.end())
			Text::fromString(oppOpt->second, options.strategies.optimizeAssets);

		// Initialize the pipeline options.
		options.piping.useWorkQueue = false;
		options.piping.lessConsoleOutput = false;

		// Initialize the output methods.
		bool resIsOk = true;
		CompilingErrors errors;
		if (kernel) {
			const GBBASIC::Kernel::Behaviour::Array &behaviours = kernel->behaviours();
			options.isPlayerBehaviour = [&behaviours] (UInt8 val) -> bool {
				GBBASIC::Kernel::Behaviour::Array::const_iterator bit = std::find_if(
					behaviours.begin(), behaviours.end(),
					[val] (const GBBASIC::Kernel::Behaviour &bhvr) -> bool {
						return val == bhvr.value;
					}
				);
				if (bit == behaviours.end()) {
					return false;
				} else {
					const GBBASIC::Kernel::Behaviour &bhvr = *bit;
					if (bhvr.type == KERNEL_BEHAVIOUR_TYPE_PLAYER)
						return true;
				}

				return false;
			};
		} else {
			options.isPlayerBehaviour = nullptr;
		}
		if (commandlineOnly) {
			options.onPrint = [] (const std::string &msg) -> void {
				fprintf(stdout, "%s\n", msg.c_str());
			};
			options.onError = [&] (const std::string &msg, bool isWarning, int page, int row, int column) -> void {
				const CompilingErrors::Entry entry(msg, isWarning, page, row, column);
				if (errors.indexOf(entry) == -1)
					errors.add(entry);
				else
					return;

				std::string msg_;
				if (row != -1 || column != -1) {
					if (page != -1) {
						msg_ += "Page ";
						msg_ += Text::toPageNumber(page);
						msg_ += ", ";
					}
					msg_ += "Ln ";
					msg_ += Text::toString(row + 1);
					msg_ += ", col ";
					msg_ += Text::toString(column + 1 + program.lineNumberWidth);
					msg_ += ": ";
				}
				msg_ += msg;
				if (isWarning)
					fprintf(stdout, "%s\n", msg_.c_str());
				else
					fprintf(stderr, "%s\n", msg_.c_str());
			};
			options.onPipelinePrint = [] (const std::string &msg, AssetsBundle::Categories /* category */) -> void {
				fprintf(stdout, "%s\n", msg.c_str());
			};
			options.onPipelineError = [&] (const std::string &msg, bool isWarning, AssetsBundle::Categories category, int page) -> void {
				const CompilingErrors::Entry entry(msg, isWarning, category, page);
				if (errors.indexOf(entry) == -1)
					errors.add(entry);
				else
					return;

				if (!isWarning)
					resIsOk = false;
				std::string msg_;
				if (page != -1) {
					msg_ += "Page ";
					msg_ += Text::toPageNumber(page);
					msg_ += ": ";
				}
				msg_ += msg;
				if (isWarning)
					fprintf(stdout, "%s\n", msg_.c_str());
				else
					fprintf(stderr, "%s\n", msg_.c_str());
			};
		} else {
			options.onPrint = [self] (const std::string &msg) -> void {
				self->print(msg.c_str());
			};
			options.onError = [&, self] (const std::string &msg, bool isWarning, int page, int row, int column) -> void {
				const CompilingErrors::Entry entry(msg, isWarning, page, row, column);
				if (errors.indexOf(entry) == -1)
					errors.add(entry);
				else
					return;

				if (page != -1 && row != -1 && column != -1) {
					LockGuard<decltype(self->_lock)> guard(self->_lock);

					const CompilingErrors::Entry error(msg, isWarning, page, row, column);
					self->_compilingErrors->add(error);
				}

				std::string msg_;
				if (row != -1 || column != -1) {
					if (page != -1) {
						msg_ += "Page ";
						msg_ += Text::toPageNumber(page);
						msg_ += ", ";
					}
					msg_ += "Ln ";
					msg_ += Text::toString(row + 1);
					msg_ += ", col ";
					msg_ += Text::toString(column + 1 + program.lineNumberWidth);
					msg_ += ": ";
				}
				msg_ += msg;
				if (isWarning)
					self->warn(msg_.c_str());
				else
					self->error(msg_.c_str());
			};
			options.onPipelinePrint = [self] (const std::string &msg, AssetsBundle::Categories /* category */) -> void {
				self->print(msg.c_str());
			};
			options.onPipelineError = [&, self] (const std::string &msg, bool isWarning, AssetsBundle::Categories category, int page) -> void {
				const CompilingErrors::Entry entry(msg, isWarning, category, page);
				if (errors.indexOf(entry) == -1)
					errors.add(entry);
				else
					return;

				if (!isWarning)
					resIsOk = false;
				std::string msg_;
				if (page != -1) {
					msg_ += "Page ";
					msg_ += Text::toPageNumber(page);
					msg_ += ": ";
				}
				msg_ += msg;
				if (isWarning)
					self->warn(msg_.c_str());
				else
					self->error(msg_.c_str());
			};
		}

		/**< Compile. */

		options.onPrint(GBBASIC_TITLE " v" GBBASIC_VERSION_STRING);
		options.onPrint("");

		if (project && !project->kernel().empty() && project->kernel() != kernel->id()) {
			options.onError(
				Text::format(
					"Kernel \"{0}\" not found, using \"{1}\" instead.",
					{
						project->kernel(),
						kernel->id()
					}
				),
				true,
				-1, -1, -1
			);
		}

		const long long start = DateTime::ticks();

		const bool codeIsOk =
			   GBBASIC::load(program, options) &&
			GBBASIC::compile(program, options) &&
			   GBBASIC::link(program, options);

		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		const std::string msg = "Completed in " + Text::toString(secs) + "s.";
		options.onPrint(msg);

		/**< Dump. */

#if defined GBBASIC_DEBUG
		do {
			const Bytes::Ptr &rom = program.compiled.bytes;
			if (!rom)
				break;

			if (!project)
				break;

			std::string name;
			std::string ext;
			std::string dir;
			Path::split(project->path(), &name, &ext, &dir);
			Text::toLowerCase(ext);
			if (ext == GBBASIC_ROM_DUMP_EXT)
				name += "." + ext + "." GBBASIC_ROM_DUMP_EXT;
			else
				name += "." GBBASIC_ROM_DUMP_EXT;
			const std::string newPath = Path::combine(dir.c_str(), name.c_str());

			Operations::projectDump(params->window, params->renderer, self, rom, newPath.c_str());
		} while (false);
#endif /* GBBASIC_DEBUG */

		/**< Finish. */

		do {
			LockGuard<decltype(self->_lock)> guard(self->_lock);

			self->_compilingParameters = CompilingParameters();

			self->_compilingOutput = codeIsOk && resIsOk ? program.compiled.bytes : nullptr;
		} while (false);

		self->_state = States::COMPILED;

		if (!doNotQuit) {
			SDL_Event evt;
			evt.type = SDL_QUIT;
			SDL_PushEvent(&evt);
		}
	};

	// Prepare.
	if (_state != States::IDLE)
		return;

	bool commandlineOnly = false;
	Text::Dictionary::const_iterator cloOpt = arguments.find(WORKSPACE_OPTION_APPLICATION_COMMANDLINE_ONLY_KEY);
	if (cloOpt != arguments.end())
		commandlineOnly = true;
	bool doNotQuit = false;
	Text::Dictionary::const_iterator dnqOpt = arguments.find(WORKSPACE_OPTION_APPLICATION_DO_NOT_QUIT_KEY);
	if (dnqOpt != arguments.end())
		doNotQuit = true;

	category(Categories::CONSOLE);
	tabsWidth(0.0f);
	interactable(doNotQuit);

	// Load the project temporarily if it has not been loaded yet.
	Project::Ptr tmpPrj = nullptr;
	do {
		if (commandlineOnly)
			break;

		if (prj)
			break;

		if (!rnd)
			break;

		Text::Dictionary::const_iterator srcOpt = arguments.find(COMPILER_INPUT_OPTION_KEY);
		if (srcOpt == arguments.end())
			break;

		const std::string &path = srcOpt->second;
		tmpPrj = Project::Ptr(new Project(wnd, rnd, this));
		if (!tmpPrj->open(path.c_str()))
			break;

		Texture::Ptr attribtex(theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Texture::Ptr propstex(theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		Texture::Ptr actorstex(theme()->textureActors(), [] (Texture*) -> void { /* Do nothing. */ });
		tmpPrj->attributesTexture(attribtex);
		tmpPrj->propertiesTexture(propstex);
		tmpPrj->actorsTexture(actorstex);

		tmpPrj->behaviourSerializer(std::bind(&Workspace::serializeKernelBehaviour, this, std::placeholders::_1));
		tmpPrj->behaviourParser(std::bind(&Workspace::parseKernelBehaviour, this, std::placeholders::_1));

		std::string fontConfigPath;
		Text::Dictionary::const_iterator fntOpt = arguments.find(COMPILER_FONT_OPTION_KEY);
		if (fntOpt != arguments.end())
			fontConfigPath = fntOpt->second;
		else
			fontConfigPath = WORKSPACE_FONT_DEFAULT_CONFIG_FILE;
		if (!tmpPrj->load(false, fontConfigPath.c_str(), nullptr)) {
			tmpPrj->close(true);

			break;
		}
	} while (false);

	// Initialize the compiling context.
	do {
		LockGuard<decltype(_lock)> guard(_lock);

		const GBBASIC::Kernel::Ptr &krnl = activeKernel();

		_compilingParameters.window = wnd;
		_compilingParameters.renderer = rnd;
		_compilingParameters.self = this;
		_compilingParameters.kernel = krnl;
		_compilingParameters.project = prj ? prj : tmpPrj;
		_compilingParameters.arguments = arguments;
		_compilingParameters.threaded = !commandlineOnly;
		_compilingParameters.commandlineOnly = commandlineOnly;
		_compilingParameters.doNotQuit = doNotQuit;

		_compilingErrors = CompilingErrors::Ptr(new CompilingErrors());

		_compilingOutput = nullptr;
	} while (false);

	// Compile it.
	_state = States::COMPILING;
#if GBBASIC_MULTITHREAD_ENABLED
	if (_compilingParameters.threaded)
		_thread = std::thread(proc, &_compilingParameters);
	else
		proc(&_compilingParameters);
#else /* GBBASIC_MULTITHREAD_ENABLED */
	proc(&_compilingParameters);
#endif /* GBBASIC_MULTITHREAD_ENABLED */

	// Dispose temporary project.
	if (tmpPrj) {
		tmpPrj->unload();
		tmpPrj->close(true);
		tmpPrj = nullptr;
	}
}

Bytes::Ptr Workspace::compile(
	const std::string &configPath, const std::string &config, 
	const std::string &romPath,
	const std::string &symPath, const std::string &symbols,
	const std::string &aliasesPath, const std::string &aliases,
	const std::string &title, AssetsBundle::Ptr assets,
	int bootstrapBank,
	CompilerOutputHandler print_, CompilerOutputHandler warn_, CompilerOutputHandler error_
) {
	GBBASIC::Program program;
	GBBASIC::Options options;

	program.config = config;
	program.symbols = symbols;
	program.aliases = aliases;
	program.assets = assets;

	bool resIsOk = true;
	CompilingErrors errors;
	options.config = configPath;
	options.rom = romPath;
	options.sym = symPath;
	options.aliases = aliasesPath;
	options.font = WORKSPACE_FONT_DEFAULT_CONFIG_FILE;
	options.backgroundPalettes = assets->palette.serializeBackgroundPalettes();
	options.spritePalettes = assets->palette.serializeSpritePalettes();
	options.title = title;
	options.strategies.compatibility = GBBASIC::Options::Strategies::Compatibilities::CLASSIC | GBBASIC::Options::Strategies::Compatibilities::COLORED;
	options.strategies.sramType = 0x00;
	options.strategies.cartridgeHasRtc = false;
	options.strategies.bootstrapBank = bootstrapBank;
	options.piping.useWorkQueue = false;
	options.piping.lessConsoleOutput = true;
	options.onPrint = [] (const std::string &msg) -> void {
		fprintf(stdout, "%s\n", msg.c_str());
	};
	options.onError = [warn_, error_, &program, &errors] (const std::string &msg, bool isWarning, int page, int row, int column) -> void {
		const CompilingErrors::Entry entry(msg, isWarning, page, row, column);
		if (errors.indexOf(entry) == -1)
			errors.add(entry);
		else
			return;

		std::string msg_;
		if (row != -1 || column != -1) {
			if (page != -1) {
				msg_ += "Page ";
				msg_ += Text::toPageNumber(page);
				msg_ += ", ";
			}
			msg_ += "Ln ";
			msg_ += Text::toString(row + 1);
			msg_ += ", col ";
			msg_ += Text::toString(column + 1 + program.lineNumberWidth);
			msg_ += ": ";
		}
		msg_ += msg;
		if (isWarning)
			warn_(msg_.c_str());
		else
			error_(msg_);
	};
	options.isPlayerBehaviour = nullptr;
	options.onPipelinePrint = [print_] (const std::string &msg, AssetsBundle::Categories /* category */) -> void {
		print_(msg);
	};
	options.onPipelineError = [warn_, error_, &resIsOk, &errors] (const std::string &msg, bool isWarning, AssetsBundle::Categories category, int page) -> void {
		const CompilingErrors::Entry entry(msg, isWarning, category, page);
		if (errors.indexOf(entry) == -1)
			errors.add(entry);
		else
			return;

		if (!isWarning)
			resIsOk = false;
		std::string msg_;
		if (page != -1) {
			msg_ += "Page ";
			msg_ += Text::toPageNumber(page);
			msg_ += ": ";
		}
		msg_ += msg;
		if (isWarning)
			warn_(msg_.c_str());
		else
			error_(msg_);
	};

	const bool codeIsOk =
		   GBBASIC::load(program, options) &&
		GBBASIC::compile(program, options) &&
		   GBBASIC::link(program, options);
	(void)codeIsOk;

	const Bytes::Ptr &rom_ = program.compiled.bytes;
	if (!rom_)
		return nullptr;

	return rom_;
}

void Workspace::beginSplash(Window* wnd, Renderer* rnd, bool hideSplashImage) {
#if GBBASIC_SPLASH_ENABLED
	if (hideSplashImage) {
		const Colour color(224, 248, 208, 255);
		rnd->clear(&color);

		rnd->flush();

		return;
	}

	if (workspaceHasSplashImage()) {
		splashCustomized(true);

		workspaceCreateSplash(wnd, rnd, this);
		workspaceRenderSplash(wnd, rnd, this, nullptr);
	} else {
		workspaceCreateSplash(wnd, rnd, this, 0);
		workspaceRenderSplash(wnd, rnd, this, nullptr);
	}
#else /* GBBASIC_SPLASH_ENABLED */
	(void)wnd;

	const Colour color(224, 248, 208, 255);
	rnd->clear(&color);

	rnd->flush();
#endif /* GBBASIC_SPLASH_ENABLED */
}

void Workspace::endSplash(Window* wnd, Renderer* rnd, bool hideSplashImage) {
#if GBBASIC_SPLASH_ENABLED
	if (hideSplashImage)
		return;

	if (splashCustomized()) {
		if (splashGbbasic()) {
			theme()->destroyTexture(rnd, splashGbbasic(), nullptr);
			splashGbbasic(nullptr);
		}
	} else {
		constexpr const int INDICES[] = {
			0
		};

		for (int i = 0; i < GBBASIC_COUNTOF(INDICES); ++i) {
			constexpr const int STEP = 20;
			const long long begin = DateTime::ticks();
			const long long end = begin + DateTime::fromSeconds(0.05);
			while (DateTime::ticks() < end) {
				workspaceSleep(STEP);
				Platform::idle();
			}

			workspaceCreateSplash(wnd, rnd, this, INDICES[i]);
			workspaceRenderSplash(wnd, rnd, this, nullptr);
		}

		if (splashGbbasic()) {
			theme()->destroyTexture(rnd, splashGbbasic(), nullptr);
			splashGbbasic(nullptr);
		}
	}
#else /* GBBASIC_SPLASH_ENABLED */
	(void)wnd;

	const Colour color(224, 248, 208, 255);
	rnd->clear(&color);

	rnd->flush();
#endif /* GBBASIC_SPLASH_ENABLED */
}

bool Workspace::loadConfig(Window*, Renderer*, const rapidjson::Document &doc) {
	// Prepare.
	if (doc.IsNull())
		return false;

	// Parse the configuration.
	Jpath::get(doc, settings().applicationWindowDisplayIndex, "application", "window", "display_index");
	Jpath::get(doc, settings().applicationWindowFullscreen, "application", "window", "fullscreen");
	Jpath::get(doc, settings().applicationWindowMaximized, "application", "window", "maximized");
	if (!Jpath::get(doc, settings().applicationWindowPosition.x, "application", "window", "position", 0))
		settings().applicationWindowPosition.x = -1;
	if (!Jpath::get(doc, settings().applicationWindowPosition.y, "application", "window", "position", 1))
		settings().applicationWindowPosition.y = -1;
	Jpath::get(doc, settings().applicationWindowSize.x, "application", "window", "size", 0);
	Jpath::get(doc, settings().applicationWindowSize.y, "application", "window", "size", 1);
	Jpath::get(doc, settings().applicationFirstRun, "application", "first_run");
	Jpath::get(doc, settings().applicationLoadedExampleRevision, "application", "loaded_example_revision");

	Jpath::get(doc, settings().exporterSettings, "exporter", "settings");
	Jpath::get(doc, settings().exporterArgs, "exporter", "args");

	Jpath::get(doc, settings().recentProjectsOrder, "recent", "projects_order");
	settings().recentProjectsOrder = Math::clamp(settings().recentProjectsOrder, (unsigned)0, (unsigned)Orders::COUNT - 1);
	Jpath::get(doc, settings().recentRemoveFromDisk, "recent", "remove_from_disk");
	Jpath::get(doc, settings().recentIconView, "recent", "icon_view");

	Jpath::get(doc, settings().mainShowProjectPathAtTitleBar, "main", "show_project_path_at_title_bar");
	unsigned indentRule = 0;
	if (Jpath::get(doc, indentRule, "main", "indent_rule"))
		settings().mainIndentRule = (Settings::IndentRules)indentRule;
	else
		settings().mainIndentRule = Settings::SPACE_2;
	unsigned columnIndicator = 0;
	if (Jpath::get(doc, columnIndicator, "main", "column_indicator"))
		settings().mainColumnIndicator = (Settings::ColumnIndicator)columnIndicator;
	else
		settings().mainColumnIndicator = Settings::COL_80;
	Jpath::get(doc, settings().mainShowWhiteSpaces, "main", "show_white_spaces");
	Jpath::get(doc, settings().mainCaseSensitive, "main", "case_sensitive");
	Jpath::get(doc, settings().mainMatchWholeWords, "main", "match_whole_words");
	Jpath::get(doc, settings().mainGlobalSearch, "main", "global_search");

	Jpath::get(doc, settings().debugShowAstEnabled, "debug", "show_ast_enabled");
	Jpath::get(doc, settings().debugOnscreenDebugEnabled, "debug", "onscreen_debug_enabled");

	Jpath::get(doc, settings().deviceType, "device", "type");
	Jpath::get(doc, settings().devicePreferSgb, "device", "prefer_sgb");
	Jpath::get(doc, settings().deviceSaveSramOnStop, "device", "save_sram_on_stop");
	for (int i = 0; i < GBBASIC_COUNTOF(settings().deviceClassicPalette); ++i) {
		UInt32 col = 0;
		if (Jpath::get(doc, col, "device", "classic_palette", i))
			settings().deviceClassicPalette[i] = Colour::byRGBA8888(col);
	}

	Jpath::get(doc, settings().emulatorMuted, "emulator", "muted");
	Jpath::get(doc, settings().emulatorSpeed, "emulator", "speed");
	Jpath::get(doc, settings().emulatorPreferedSpeed, "emulator", "prefered_speed");

	Jpath::get(doc, settings().canvasFixRatio, "canvas", "fix_ratio");
	Jpath::get(doc, settings().canvasIntegerScale, "canvas", "integer_scale");

	Jpath::get(doc, settings().consoleClearOnStart, "console", "clear_on_start");

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		Input::Gamepad &pad = settings().inputGamepads[i];
		for (int j = 0; j < Input::BUTTON_COUNT; ++j) {
			unsigned dev = pad.buttons[j].device;
			Jpath::get(doc, dev, "input", "gamepad", i, j, "device");
			Jpath::get(doc, pad.buttons[j].index, "input", "gamepad", i, j, "index");
			unsigned type = pad.buttons[j].type;
			Jpath::get(doc, type, "input", "gamepad", i, j, "type");
			pad.buttons[j].type = (Input::Types)type;
			switch (pad.buttons[j].type) {
			case Input::VALUE:
				Jpath::get(doc, pad.buttons[j].value, "input", "gamepad", i, j, "value");

				break;
			case Input::HAT: {
					Jpath::get(doc, pad.buttons[j].hat.index, "input", "gamepad", i, j, "sub");
					unsigned short subType = pad.buttons[j].hat.value;
					Jpath::get(doc, subType, "input", "gamepad", i, j, "value");
					pad.buttons[j].hat.value = (Input::Hat::Types)subType;
				}

				break;
			case Input::AXIS:
				Jpath::get(doc, pad.buttons[j].axis.index, "input", "gamepad", i, j, "sub");
				Jpath::get(doc, pad.buttons[j].axis.value, "input", "gamepad", i, j, "value");

				break;
			}

			pad.buttons[j].device = (Input::Devices)dev;
		}
	}
	Jpath::get(doc, settings().inputOnscreenGamepadEnabled, "input", "onscreen_gamepad", "enabled");
	Jpath::get(doc, settings().inputOnscreenGamepadSwapAB, "input", "onscreen_gamepad", "swap_ab");
	Jpath::get(doc, settings().inputOnscreenGamepadScale, "input", "onscreen_gamepad", "scale");
	Jpath::get(doc, settings().inputOnscreenGamepadPadding.x, "input", "onscreen_gamepad", "padding", 0);
	Jpath::get(doc, settings().inputOnscreenGamepadPadding.y, "input", "onscreen_gamepad", "padding", 1);

	// Finish.
	return true;
}

bool Workspace::saveConfig(Window*, Renderer*, rapidjson::Document &doc) {
	// Serialize the configuration.
	Jpath::set(doc, doc, settings().applicationWindowDisplayIndex, "application", "window", "display_index");
	Jpath::set(doc, doc, settings().applicationWindowFullscreen, "application", "window", "fullscreen");
	Jpath::set(doc, doc, settings().applicationWindowMaximized, "application", "window", "maximized");
	Jpath::set(doc, doc, settings().applicationWindowPosition.x, "application", "window", "position", 0);
	Jpath::set(doc, doc, settings().applicationWindowPosition.y, "application", "window", "position", 1);
	Jpath::set(doc, doc, settings().applicationWindowSize.x, "application", "window", "size", 0);
	Jpath::set(doc, doc, settings().applicationWindowSize.y, "application", "window", "size", 1);
	Jpath::set(doc, doc, settings().applicationFirstRun, "application", "first_run");
	Jpath::set(doc, doc, settings().applicationLoadedExampleRevision, "application", "loaded_example_revision");

	Jpath::set(doc, doc, settings().exporterSettings, "exporter", "settings");
	Jpath::set(doc, doc, settings().exporterArgs, "exporter", "args");

	Jpath::set(doc, doc, settings().recentProjectsOrder, "recent", "projects_order");
	Jpath::set(doc, doc, settings().recentRemoveFromDisk, "recent", "remove_from_disk");
	Jpath::set(doc, doc, settings().recentIconView, "recent", "icon_view");

	Jpath::set(doc, doc, settings().mainShowProjectPathAtTitleBar, "main", "show_project_path_at_title_bar");
	Jpath::set(doc, doc, (unsigned)settings().mainIndentRule, "main", "indent_rule");
	Jpath::set(doc, doc, (unsigned)settings().mainColumnIndicator, "main", "column_indicator");
	Jpath::set(doc, doc, settings().mainShowWhiteSpaces, "main", "show_white_spaces");
	Jpath::set(doc, doc, settings().mainCaseSensitive, "main", "case_sensitive");
	Jpath::set(doc, doc, settings().mainMatchWholeWords, "main", "match_whole_words");
	Jpath::set(doc, doc, settings().mainGlobalSearch, "main", "global_search");

	Jpath::set(doc, doc, settings().debugShowAstEnabled, "debug", "show_ast_enabled");
	Jpath::set(doc, doc, settings().debugOnscreenDebugEnabled, "debug", "onscreen_debug_enabled");

	Jpath::set(doc, doc, settings().deviceType, "device", "type");
	Jpath::set(doc, doc, settings().devicePreferSgb, "device", "prefer_sgb");
	Jpath::set(doc, doc, settings().deviceSaveSramOnStop, "device", "save_sram_on_stop");
	for (int i = 0; i < GBBASIC_COUNTOF(settings().deviceClassicPalette); ++i) {
		Jpath::set(doc, doc, settings().deviceClassicPalette[i].toRGBA(), "device", "classic_palette", i);
	}

	Jpath::set(doc, doc, settings().emulatorMuted, "emulator", "muted");
	Jpath::set(doc, doc, settings().emulatorSpeed, "emulator", "speed");
	Jpath::set(doc, doc, settings().emulatorPreferedSpeed, "emulator", "prefered_speed");

	Jpath::set(doc, doc, settings().canvasFixRatio, "canvas", "fix_ratio");
	Jpath::set(doc, doc, settings().canvasIntegerScale, "canvas", "integer_scale");

	Jpath::set(doc, doc, settings().consoleClearOnStart, "console", "clear_on_start");

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		const Input::Gamepad &pad = settings().inputGamepads[i];
		for (int j = 0; j < Input::BUTTON_COUNT; ++j) {
			const unsigned dev = pad.buttons[j].device;
			Jpath::set(doc, doc, dev, "input", "gamepad", i, j, "device");
			Jpath::set(doc, doc, pad.buttons[j].index, "input", "gamepad", i, j, "index");
			Jpath::set(doc, doc, (unsigned short)pad.buttons[j].type, "input", "gamepad", i, j, "type");
			switch (pad.buttons[j].type) {
			case Input::VALUE:
				Jpath::set(doc, doc, pad.buttons[j].value, "input", "gamepad", i, j, "value");

				break;
			case Input::HAT:
				Jpath::set(doc, doc, pad.buttons[j].hat.index, "input", "gamepad", i, j, "sub");
				Jpath::set(doc, doc, (unsigned short)pad.buttons[j].hat.value, "input", "gamepad", i, j, "value");

				break;
			case Input::AXIS:
				Jpath::set(doc, doc, pad.buttons[j].axis.index, "input", "gamepad", i, j, "sub");
				Jpath::set(doc, doc, pad.buttons[j].axis.value, "input", "gamepad", i, j, "value");

				break;
			}
		}
	}
	Jpath::set(doc, doc, settings().inputOnscreenGamepadEnabled, "input", "onscreen_gamepad", "enabled");
	Jpath::set(doc, doc, settings().inputOnscreenGamepadSwapAB, "input", "onscreen_gamepad", "swap_ab");
	Jpath::set(doc, doc, settings().inputOnscreenGamepadScale, "input", "onscreen_gamepad", "scale");
	Jpath::set(doc, doc, settings().inputOnscreenGamepadPadding.x, "input", "onscreen_gamepad", "padding", 0);
	Jpath::set(doc, doc, settings().inputOnscreenGamepadPadding.y, "input", "onscreen_gamepad", "padding", 1);

	// Finish.
	return true;
}

void Workspace::loadKernels(void) {
	// Prepare.
	auto load = [this] (const FileInfos::Ptr &fileInfos) -> void {
		for (int i = 0; i < fileInfos->count(); ++i) {
			FileInfo::Ptr fileInfo = fileInfos->get(i);

			const std::string path = fileInfo->fullPath();
			std::string name;
			Path::split(path, &name, nullptr, nullptr);

			GBBASIC::Kernel::Ptr krnl(new GBBASIC::Kernel());
			if (!krnl->open(path.c_str()))
				continue; // Not a kernel.

			if (krnl->id() == "default")
				kernels().insert(kernels().begin(), krnl); // Default kernel is always in the front.
			else
				kernels().push_back(krnl);

			Platform::idle();
		}
	};

	kernels().clear();

	// Load the system kernels.
	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(KERNEL_SYSTEM_BINARIES_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", true);
	load(fileInfos);
#if WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED
	const std::string altPath = Path::combine(WORKSPACE_ALTERNATIVE_ROOT_PATH, KERNEL_SYSTEM_BINARIES_DIR);
	dirInfo = DirectoryInfo::make(altPath.c_str());
	fileInfos = dirInfo->getFiles("*.json", true);
	load(fileInfos);
#endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED */

	// Load the user kernels.
	const std::string writableDir = Path::writableDirectory();
	const std::string userPath = Path::combine(writableDir.c_str(), KERNEL_USER_BINARIES_DIR);
	DirectoryInfo::Ptr userDirInfo = DirectoryInfo::make(userPath.c_str());
	FileInfos::Ptr userFileInfos = userDirInfo->getFiles("*.json", true);
	load(userFileInfos);

	// Load the information.
	if (kernels().empty()) {
		messagePopupBox(theme()->dialogPrompt_CannotFindAnyKernel(), nullptr, nullptr, nullptr);

		fprintf(stderr, "Cannot find any kernel.\n");

		return;
	}

	const GBBASIC::Kernel::Ptr &defaultKrnl = kernels().front();
	for (int i = 1; i < (int)kernels().size(); ++i) {
		GBBASIC::Kernel::Ptr &krnl = kernels()[i];
		if (krnl->behaviours().empty())
			krnl->behaviours(defaultKrnl->behaviours());
	}

	activeKernelIndex(0);

	const std::string src = getSourceCodePath(0, nullptr);
	hasDefaultKernelSourceCode(Path::fileExists(src.c_str()));
}

void Workspace::unloadKernels(void) {
	for (GBBASIC::Kernel::Ptr &krnl : kernels()) {
		krnl->close();
	}
	kernels().clear();
}

void Workspace::loadExporters(void) {
	auto load = [this] (const FileInfos::Ptr &fileInfos, Text::Set &licenses) -> void {
		for (int i = 0; i < fileInfos->count(); ++i) {
			FileInfo::Ptr fileInfo = fileInfos->get(i);

			const std::string path = fileInfo->fullPath();

			Exporter::Ptr ex(new Exporter());
			ex->open(path.c_str(), theme()->menu_Build().c_str());
			exporters().push_back(ex);

			for (const std::string &l : ex->licenses()) {
				if (licenses.find(l) != licenses.end())
					continue;

				licenses.insert(l);

				exporterLicenses() += l + "\n";
			}

			Platform::idle();
		}
	};

	exporterLicenses().clear();

	Text::Set licenses;
	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(EXPORTER_RULES_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", true);
	load(fileInfos, licenses);
#if WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED
	const std::string altPath = Path::combine(WORKSPACE_ALTERNATIVE_ROOT_PATH, EXPORTER_RULES_DIR);
	dirInfo = DirectoryInfo::make(altPath.c_str());
	fileInfos = dirInfo->getFiles("*.json", true);
	load(fileInfos, licenses);
#endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED */

	std::sort(
		exporters().begin(), exporters().end(),
		[] (const Exporter::Ptr &l, const Exporter::Ptr &r) -> bool {
			const int lo = l->order();
			const int ro = r->order();

			return lo < ro;
		}
	);
}

void Workspace::unloadExporters(void) {
	for (Exporter::Ptr &ex : exporters()) {
		ex->close();
	}
	exporters().clear();
}

void Workspace::loadExamples(Window* wnd, Renderer* rnd) {
	exampleCount(0);

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(WORKSPACE_EXAMPLE_PROJECTS_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." GBBASIC_RICH_PROJECT_EXT ";" "*." GBBASIC_PLAIN_PROJECT_EXT, true);

	exampleCount(fileInfos->count());

#if WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED
	examples().clear();

	Project::Array examplePtrs;

	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		const std::string path = fileInfo->fullPath();

		Project::Ptr prj(new Project(wnd, rnd, this));
		if (prj->open(path.c_str())) {
			examplePtrs.push_back(prj);
		}
		prj->close(true);

		Platform::idle();
	}

	std::sort(
		examplePtrs.begin(), examplePtrs.end(),
		[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
			if (l->order() < r->order())
				return true;
			if (l->order() > r->order())
				return false;
			if (l->title() < r->title())
				return true;
			if (l->title() > r->title())
				return false;

			return false;
		}
	);
	for (Project::Ptr &prj : examplePtrs) {
		const std::string &entry = prj->title();
		examples().push_back(EntryWithPath(entry, prj->path(), prj->description()));
	}
#else /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */
	(void)wnd;
	(void)rnd;
#endif /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */
}

void Workspace::unloadExamples(void) {
	exampleCount(0);

#if WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED
	examples().clear();
#endif /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */
}

void Workspace::loadStarterKits(Window* wnd, Renderer* rnd) {
	auto load = [wnd, rnd, this] (const FileInfos::Ptr &fileInfos, Project::Array &kitPtrs) -> void {
		for (int i = 0; i < fileInfos->count(); ++i) {
			FileInfo::Ptr fileInfo = fileInfos->get(i);

			const std::string path = fileInfo->fullPath();

			Project::Ptr prj(new Project(wnd, rnd, this));
			if (prj->open(path.c_str())) {
				kitPtrs.push_back(prj);
			}
			prj->close(true);

			Platform::idle();
		}
	};

	starterKits().clear();

	Project::Array kitPtrs;
	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(WORKSPACE_STARTER_KITS_PROJECTS_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." GBBASIC_RICH_PROJECT_EXT ";" "*." GBBASIC_PLAIN_PROJECT_EXT, true);
	load(fileInfos, kitPtrs);
#if WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED
	const std::string altPath = Path::combine(WORKSPACE_ALTERNATIVE_ROOT_PATH, WORKSPACE_STARTER_KITS_PROJECTS_DIR);
	dirInfo = DirectoryInfo::make(altPath.c_str());
	fileInfos = dirInfo->getFiles("*." GBBASIC_RICH_PROJECT_EXT ";" "*." GBBASIC_PLAIN_PROJECT_EXT, true);
	load(fileInfos, kitPtrs);
#endif /* WORKSPACE_ALTERNATIVE_ROOT_PATH_ENABLED */

	std::sort(
		kitPtrs.begin(), kitPtrs.end(),
		[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
			if (l->order() < r->order())
				return true;
			if (l->order() > r->order())
				return false;
			if (l->title() < r->title())
				return true;
			if (l->title() > r->title())
				return false;

			return false;
		}
	);
	for (Project::Ptr &prj : kitPtrs) {
		const std::string &entry = prj->title();
		starterKits().push_back(EntryWithPath(entry, prj->path(), prj->description()));
	}
}

void Workspace::unloadStarterKits(void) {
	starterKits().clear();
}

void Workspace::loadMusic(void) {
	musicCount(0);

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(WORKSPACE_MUSIC_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", true);

	musicCount(fileInfos->count());

#if WORKSPACE_MUSIC_MENU_ENABLED
	music().clear();

	Project::Array examplePtrs;

	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		const std::string package = dirInfo->fullPath();
		const std::string path = fileInfo->fullPath();
		std::string entry = path.substr(package.length());
		entry = entry.substr(0, entry.length() - strlen(".json"));

		music()[Entry(entry)] = path;

		Platform::idle();
	}
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */
}

void Workspace::unloadMusic(void) {
	musicCount(0);

#if WORKSPACE_MUSIC_MENU_ENABLED
	music().clear();
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */
}

void Workspace::loadSfx(void) {
	sfxCount(0);

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(WORKSPACE_SFX_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", true);

	sfxCount(fileInfos->count());

#if WORKSPACE_SFX_MENU_ENABLED
	sfx().clear();

	Project::Array examplePtrs;

	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		const std::string package = dirInfo->fullPath();
		const std::string path = fileInfo->fullPath();
		std::string entry = path.substr(package.length());
		entry = entry.substr(0, entry.length() - strlen(".json"));

		sfx()[Entry(entry)] = path;

		Platform::idle();
	}
#endif /* WORKSPACE_SFX_MENU_ENABLED */
}

void Workspace::unloadSfx(void) {
	sfxCount(0);

#if WORKSPACE_SFX_MENU_ENABLED
	sfx().clear();
#endif /* WORKSPACE_SFX_MENU_ENABLED */
}

void Workspace::loadDocuments(void) {
	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(DOCUMENT_MARKDOWN_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." DOCUMENT_MARKDOWN_EXT, true);

	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		const std::string package = dirInfo->fullPath();
		const std::string path = fileInfo->fullPath();
		std::string entry = path.substr(package.length());
		entry = entry.substr(0, entry.length() - strlen("." DOCUMENT_MARKDOWN_EXT));

		documents()[Entry(entry)] = path;

		Platform::idle();
	}
}

void Workspace::unloadDocuments(void) {
	documents().clear();
}

void Workspace::loadLinks(void) {
	const std::string path = Path::combine(DOCUMENT_MARKDOWN_DIR, WORKSPACE_LINKS_FILE);
	if (!Path::fileExists(path.c_str()))
		return;

	File::Ptr file(File::create());
	if (!file->open(path.c_str(), Stream::READ))
		return;

	rapidjson::Document doc;
	std::string buf;
	if (!file->readString(buf)) {
		file->close();

		return;
	}
	file->close();

	if (!Json::fromString(doc, buf.c_str()))
		return;

	const int n = Jpath::count(doc, "links");
	for (int i = 0; i < n; ++i) {
		std::string name;
		std::string content;
		if (!Jpath::get(doc, name, "links", i, "name") || !Jpath::get(doc, content, "links", i, "content"))
			continue;

		links()[Entry(name)] = content;
	}
}

void Workspace::unloadLinks(void) {
	links().clear();
}

bool Workspace::execute(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq) {
	ImGuiIO &io = ImGui::GetIO();

	if (!canvasDevice())
		return false;

	*fpsReq = GBBASIC_ACTIVE_FRAME_RATE;

	if (!canvasTexture()) {
		canvasTexture(Texture::Ptr(Texture::create()));
		canvasTexture()->scale(Texture::NEAREST);
		canvasTexture()->blend(Texture::BLEND);
		canvasTexture()->fromBytes(rnd, Texture::TARGET, nullptr, SCREEN_WIDTH, SCREEN_HEIGHT, 0, Texture::NEAREST);

		GBBASIC_RENDER_TARGET(rnd, canvasTexture().get())
		GBBASIC_RENDER_SCALE(rnd, 1)
		const Colour col(8, 8, 8, 255);
		rnd->clear(&col);
	}

	if (
		canvasDevice()->isSgbBorderSupported() &&
		canvasDevice()->cartridgeHasSgbSupport() && canvasDevice()->deviceHasSgbSupport() &&
		!canvasTextureForBorderFrame()
	) {
		canvasTextureForBorderFrame(Texture::Ptr(Texture::create()));
		canvasTextureForBorderFrame()->scale(Texture::NEAREST);
		canvasTextureForBorderFrame()->blend(Texture::BLEND);
		canvasTextureForBorderFrame()->fromBytes(rnd, Texture::TARGET, nullptr, SGB_SCREEN_WIDTH, SGB_SCREEN_HEIGHT, 0, Texture::NEAREST);

		GBBASIC_RENDER_TARGET(rnd, canvasTextureForBorderFrame().get())
		GBBASIC_RENDER_SCALE(rnd, 1)
		const Colour col(8, 8, 8, 255);
		rnd->clear(&col);
	}

	Device::AudioHandler audioHandler = nullptr;
	if (settings().emulatorMuted) {
		audioHandler = [] (void* /* specification */, Bytes* /* buffer */, UInt32 /* length */) -> bool {
			// Do nothing.

			return true;
		};
	}

	const Device::KeyboardModifiers keyMods(io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper);
	canvasDevice()->update(wnd, rnd, delta, canvasTexture().get(), canvasTextureForBorderFrame().get(), !popupBox() && !menuOpened(), &keyMods, audioHandler);

	return true;
}

bool Workspace::perform(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq, Device::AudioHandler handleAudio) {
	const double delta_ = Math::clamp(delta, 1 / 90.0, 1 / 30.0);
	updateAudioDevice(wnd, rnd, delta_, fpsReq, handleAudio);

	return true;
}

void Workspace::saveSram(Window* wnd, Renderer* rnd) {
	if (!canvasDevice())
		return;

	const Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	Bytes::Ptr sram(Bytes::create());
	canvasDevice()->writeSram(sram.get());
	Operations::projectSaveSram(wnd, rnd, this, prj, sram, true);
}

void Workspace::prepare(Window*, Renderer*) {
	// Do nothing.
}

void Workspace::finish(Window* wnd, Renderer* rnd) {
	// Update the initializer.
	init().update();

	// Process compiling stages.
	switch (_state) {
	case States::IDLE:
		// Do nothing.

		break;
	case States::COMPILING:
		// Do nothing.

		break;
	case States::COMPILED: {
			joinCompiling();

			LockGuard<decltype(_lock)> guard(_lock);

			const Project::Ptr &prj = currentProject();

			do {
				CompilingErrors::Ptr errors = _compilingErrors;
				if (prj) {
					for (int i = 0; i < prj->codePageCount(); ++i) {
						CodeAssets::Entry* entry = prj->getCode(i);
						Editable* editor = entry->editor;
						if (editor)
							editor->post(Editable::SET_ERRORS, errors);
					}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
					if (prj->minorCodeEditor()) {
						prj->minorCodeEditor()->post(Editable::SET_ERRORS, errors);
					}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

					for (int i = 0; i < errors->count(); ++i) {
						const CompilingErrors::Entry* err = errors->get(i);
						if (err->isWarning)
							continue;

						category(Categories::CODE);
						changePage(wnd, rnd, prj.get(), Categories::CODE, err->page);

						delay(
							std::bind(
								[this] (int idx, CompilingErrors::Ptr errors) -> void {
									const Project::Ptr &prj = currentProject();

									CodeAssets::Entry* entry = prj->getCode(idx);
									if (!entry)
										return;

									Editable* editor = entry->editor;
									if (editor)
										editor->post(Editable::SET_ERRORS, errors);
								},
								err->page, errors
							),
							"COMPILE ERROR"
						);
					}
				}
				_compilingErrors = nullptr;
			} while (false);
			if (_compilingOutput) {
				if (toRun()) {
					Bytes::Ptr rom = _compilingOutput;
					runProject(wnd, rnd, rom);
					toRun(false);
				} else if (toExport() >= 0) {
					Bytes::Ptr rom = _compilingOutput;
					Exporter::Ptr ex = exporters()[toExport()];
					Operations::projectBuild(wnd, rnd, this, rom, ex)
						.always(
							[&] (void) -> void {
								settings().exporterIcon = nullptr;
							}
						);
					toExport(-1);
				}
				_compilingOutput = nullptr;
			}

			_state = States::IDLE;
		}

		break;
	case States::LAUNCHED: {
			LockGuard<decltype(_lock)> guard(_lock);

			const Project::Ptr &prj = currentProject();

			if (toRun()) {
				Bytes::Ptr rom = prj->rom();
				runProject(wnd, rnd, rom);
				toRun(false);
			}

			_state = States::IDLE;
		}

		break;
	}

	// Updated the unsaved status.
#if defined GBBASIC_OS_HTML
	bool opened = false;
	bool hasUnsavedChanges = false;
	getCurrentProjectStates(&opened, &hasUnsavedChanges, nullptr, nullptr);
	if (_hadUnsavedChanges != hasUnsavedChanges) { // The unsaved status has changed.
		_hadUnsavedChanges = hasUnsavedChanges;
		if (opened && hasUnsavedChanges) {
			EM_ASM({
				if (typeof blockExit == 'function')
					blockExit('You have unsaved data.');
			});
		} else {
			EM_ASM({
				if (typeof allowExit == 'function')
					allowExit();
			});
		}
	}
#endif /* Platform macro. */

	// Process delayed actions.
	if (!delayed().empty()) {
		const long long now = DateTime::ticks();
		Delayed::List::iterator it = delayed().begin();
		while (it != delayed().end()) {
			Delayed &delay = *it;
			switch (delay.delay.type) {
			case Delayed::Amount::Types::FOR_FRAMES:
				if (--delay.delay.data == 0) {
					delay.handler();
					it = delayed().erase(it);
				} else {
					++it;
				}

				break;
			case Delayed::Amount::Types::UNTIL_TIMESTAMP:
				if (delay.delay.data < now) {
					delay.handler();
					it = delayed().erase(it);
				} else {
					++it;
				}

				break;
			}
		}
	}

	// Process work queue.
	workQueue()->update();
}

void Workspace::shortcuts(Window* wnd, Renderer* rnd) {
	// Prepare.
	ImGuiIO &io = ImGui::GetIO();

	// Get key states.
#if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
	const bool modifier = io.KeyCtrl;
#elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
	const bool modifier = io.KeySuper;
#endif /* GBBASIC_MODIFIER_KEY */

	const bool f1         = ImGui::IsKeyPressed(SDL_SCANCODE_F1);
	const bool f2         = ImGui::IsKeyPressed(SDL_SCANCODE_F2);
	const bool f3         = ImGui::IsKeyPressed(SDL_SCANCODE_F3);
	const bool f4         = ImGui::IsKeyPressed(SDL_SCANCODE_F4);
	const bool f5         = ImGui::IsKeyPressed(SDL_SCANCODE_F5);
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
	const bool f6         = ImGui::IsKeyPressed(SDL_SCANCODE_F6);
	const bool f7         = ImGui::IsKeyPressed(SDL_SCANCODE_F7);
	const bool f8         = ImGui::IsKeyPressed(SDL_SCANCODE_F8);
	const bool f11        = ImGui::IsKeyPressed(SDL_SCANCODE_F11);
#endif /* Platform macro. */

	const bool num1       = ImGui::IsKeyPressed(SDL_SCANCODE_1);
	const bool num2       = ImGui::IsKeyPressed(SDL_SCANCODE_2);
	const bool num3       = ImGui::IsKeyPressed(SDL_SCANCODE_3);
	const bool num4       = ImGui::IsKeyPressed(SDL_SCANCODE_4);
	const bool num5       = ImGui::IsKeyPressed(SDL_SCANCODE_5);
	const bool num6       = ImGui::IsKeyPressed(SDL_SCANCODE_6);
	const bool num7       = ImGui::IsKeyPressed(SDL_SCANCODE_7);
	const bool num8       = ImGui::IsKeyPressed(SDL_SCANCODE_8);
	const bool num9       = ImGui::IsKeyPressed(SDL_SCANCODE_9);
	const bool num0       = ImGui::IsKeyPressed(SDL_SCANCODE_0);

	const bool a          = ImGui::IsKeyPressed(SDL_SCANCODE_A);
	const bool b          = ImGui::IsKeyPressed(SDL_SCANCODE_B);
	const bool c          = ImGui::IsKeyPressed(SDL_SCANCODE_C);
	const bool d          = ImGui::IsKeyPressed(SDL_SCANCODE_D);
	const bool f          = ImGui::IsKeyPressed(SDL_SCANCODE_F);
	const bool g          = ImGui::IsKeyPressed(SDL_SCANCODE_G);
	const bool k          = ImGui::IsKeyPressed(SDL_SCANCODE_K);
	const bool n          = ImGui::IsKeyPressed(SDL_SCANCODE_N);
	const bool o          = ImGui::IsKeyPressed(SDL_SCANCODE_O);
	const bool r          = ImGui::IsKeyPressed(SDL_SCANCODE_R);
	const bool s          = ImGui::IsKeyPressed(SDL_SCANCODE_S);
	const bool u          = ImGui::IsKeyPressed(SDL_SCANCODE_U);
	const bool v          = ImGui::IsKeyPressed(SDL_SCANCODE_V);
	const bool w          = ImGui::IsKeyPressed(SDL_SCANCODE_W);
	const bool x          = ImGui::IsKeyPressed(SDL_SCANCODE_X);
	const bool y          = ImGui::IsKeyPressed(SDL_SCANCODE_Y);
	const bool z          = ImGui::IsKeyPressed(SDL_SCANCODE_Z);

	const bool backspace  = ImGui::IsKeyPressed(SDL_SCANCODE_BACKSPACE);
	const bool grave      = ImGui::IsKeyPressed(SDL_SCANCODE_GRAVE);      /* ` */
	const bool equals     = ImGui::IsKeyPressed(SDL_SCANCODE_EQUALS);     /* = */
	const bool backSlash  = ImGui::IsKeyPressed(SDL_SCANCODE_BACKSLASH);  /* \ */
	const bool apostrophe = ImGui::IsKeyPressed(SDL_SCANCODE_APOSTROPHE); /* ' */
	const bool comma      = ImGui::IsKeyPressed(SDL_SCANCODE_COMMA);      /* , */
	const bool period     = ImGui::IsKeyPressed(SDL_SCANCODE_PERIOD);     /* . */
	const bool slash      = ImGui::IsKeyPressed(SDL_SCANCODE_SLASH);      /* / */
	const bool return_    = ImGui::IsKeyPressed(SDL_SCANCODE_RETURN);
	const bool returnR    = ImGui::IsKeyReleased(SDL_SCANCODE_RETURN);
	const bool tab        = ImGui::IsKeyPressed(SDL_SCANCODE_TAB);
	const bool esc        = ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE);
	const bool del        = ImGui::IsKeyPressed(SDL_SCANCODE_DELETE);
	const bool home       = ImGui::IsKeyPressed(SDL_SCANCODE_HOME);
	const bool end        = ImGui::IsKeyPressed(SDL_SCANCODE_END);
	const bool pgUp       = ImGui::IsKeyPressed(SDL_SCANCODE_PAGEUP);
	const bool pgDown     = ImGui::IsKeyPressed(SDL_SCANCODE_PAGEDOWN);
	const bool right      = ImGui::IsKeyPressed(SDL_SCANCODE_RIGHT);
	const bool left       = ImGui::IsKeyPressed(SDL_SCANCODE_LEFT);
	const bool down       = ImGui::IsKeyPressed(SDL_SCANCODE_DOWN);
	const bool up         = ImGui::IsKeyPressed(SDL_SCANCODE_UP);
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
	const bool prtSc      = ImGui::IsKeyPressed(SDL_SCANCODE_PRINTSCREEN);
#endif /* Platform macro. */

	// Get project states.
	bool opened = false;
	bool hasUnsavedChanges = false;
	getCurrentProjectStates(&opened, &hasUnsavedChanges, nullptr, nullptr);

	// Window operations.
	int times = 0;
	if (modifier && !io.KeyShift && io.KeyAlt) {
		if (num1)
			times = 1;
		else if (num2)
			times = 2;
		else if (num3)
			times = 3;
		else if (num4)
			times = 4;
		else if (num5)
			times = 5;
		else if (num6)
			times = 6;
		else if (num7)
			times = 7;
		else if (num8)
			times = 8;
		else if (num9)
			times = 9;
		else if (num0)
			times = -1;
		if (times) {
			const int scale = rnd->scale() / wnd->scale();
			Math::Vec2i size;
			if (times == -1) {
				size = Math::Vec2i(GBBASIC_WINDOW_MIN_WIDTH * 3, GBBASIC_WINDOW_SCALE_MIN_HEIGHT * 3);
			} else {
				if (times == 1)
					size = Math::Vec2i(GBBASIC_WINDOW_MIN_WIDTH * scale * times, GBBASIC_WINDOW_MIN_HEIGHT * scale * times);
				else
					size = Math::Vec2i(GBBASIC_WINDOW_MIN_WIDTH * scale * times, GBBASIC_WINDOW_SCALE_MIN_HEIGHT * scale * times);
			}
			if (wnd->fullscreen())
				wnd->fullscreen(false);
			if (wnd->maximized())
				wnd->restore();
			int nl = 0, nt = 0, nr = 0, nb = 0;
			Platform::notch(&nl, &nt, &nr, &nb);
			const Math::Recti notch(nl, nt, nr, nb);
			const Window::Orientations orientation = wnd->orientation();
			wnd->size(size);
			wnd->makeVisible();
			resized(
				wnd, rnd,
				wnd->size(),
				Math::Vec2i(
					wnd->size().x / rnd->scale(),
					wnd->size().y / rnd->scale()
				),
				notch,
				orientation
			);
		} else if (equals) {
			if (!wnd->maximized() && !wnd->fullscreen())
				wnd->centralize();
		}
	} else if (modifier && !io.KeyShift && !io.KeyAlt) {
		if (return_ && !popupBox())
			toggleFullscreen(wnd);
	} else if (!modifier && !io.KeyShift && !io.KeyAlt) {
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
		if (f11) {
			if (!wnd->fullscreen() && !wnd->maximized())
				wnd->bordered(!wnd->bordered());
		}
#endif /* Platform macro. */
	}

	// Recorder operations.
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
	if ((f6 || prtSc) && !modifier && !io.KeyShift && !io.KeyAlt && !recorder()->recording()) {
		recorder()->start(false, nullptr, 1);
	} else if (f7 && !modifier && !io.KeyShift && !io.KeyAlt && !recorder()->recording()) {
		recorder()->start(true, theme()->imageCursor(), activeFrameRate() * 60); // 1 minute.
	} else if (f8 && !modifier && !io.KeyShift && !io.KeyAlt && recorder()->recording()) {
		recorder()->stop();
	}
#endif /* Platform macro. */

	// Ignore the following shortcuts if it's blocked.
	if (!canUseShortcuts())
		return;

	// Edit operations.
	if (opened) {
		if (z && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
					editor->undo(entry);
				}
			);
		} else if ((y && modifier && !io.KeyShift && !io.KeyAlt) || (z && modifier && io.KeyShift && !io.KeyAlt)) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
					editor->redo(entry);
				}
			);
		} else if (c && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->copy();
				}
			);
		} else if (x && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->cut();
				}
			);
		} else if (v && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->paste();
				}
			);
		} else if (del && !modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[io] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->del(io.KeyShift);
				}
			);
		} else if (a && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::SELECT_ALL);
				}
			);
		} else if (w && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::SELECT_WORD);
				}
			);
		} else if (del && !modifier && io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::DELETE_LINE);
				}
			);
		} else if (backspace && !modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::BACK_SPACE);
				}
			);
		} else if (backspace && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::BACK_SPACE_WORDWISE);
				}
			);
		} else if (tab && !modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::INDENT, true);
				}
			);
		} else if (tab /* && !modifier */ && io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::UNINDENT, true);
				}
			);
		} else if (u && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::TO_LOWER_CASE);
				}
			);
		} else if (u && modifier && io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::TO_UPPER_CASE);
				}
			);
		} else if ((apostrophe && modifier && !io.KeyShift && !io.KeyAlt) || (slash && modifier && !io.KeyShift && !io.KeyAlt)) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::TOGGLE_COMMENT);
				}
			);
		} else if (up && !modifier && !io.KeyShift && io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::MOVE_UP);
				}
			);
		} else if (down && !modifier && !io.KeyShift && io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::MOVE_DOWN);
				}
			);
		} else if (f && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::FIND, false);
				}
			);
		} else if (f3 && !modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::FIND_NEXT);
				}
			);
		} else if (f3 && !modifier && io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::FIND_PREVIOUS);
				}
			);
		} else if (g && modifier && !io.KeyShift && !io.KeyAlt) {
			withCurrentAsset(
				[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
					editor->post(Editable::GOTO);
				}
			);
		} else if (comma && modifier && !io.KeyShift && !io.KeyAlt) {
			switch (category()) {
			case Categories::HOME: // Fall through.
			case Categories::CONSOLE: // Fall through.
			case Categories::EMULATOR:
				// Do nothing.

				break;
			default:
				Operations::editSortAssets(wnd, rnd, this, (AssetsBundle::Categories)category());

				break;
			}
		}
	}

	// File operations.
	if (n && modifier && (opened ? io.KeyShift : true) && !io.KeyAlt) {
		if (showRecentProjects()) {
			closeFilter();

			stopProject(wnd, rnd);

			Operations::fileNew(wnd, rnd, this, fontConfig().empty() ? nullptr : fontConfig().c_str())
				.then(
					[wnd, rnd, this] (Project::Ptr prj) -> void {
						ImGuiIO &io = ImGui::GetIO();

						if (io.KeyCtrl)
							return;

						Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());
								}
							);
					}
				);

			projectIndices().clear(); projectIndices().shrink_to_fit();

			recentProjectSelectedIndex(-1);
		} else {
			stopProject(wnd, rnd);

			Operations::fileNew(wnd, rnd, this, fontConfig().empty() ? nullptr : fontConfig().c_str())
				.then(
					[wnd, rnd, this] (Project::Ptr prj) -> void {
						ImGuiIO &io = ImGui::GetIO();

						if (io.KeyCtrl)
							return;

						Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());
								}
							);
					}
				);
		}
	}
	if (o && modifier && (opened ? io.KeyShift : true) && !io.KeyAlt) {
		if (showRecentProjects()) {
			closeFilter();

			stopProject(wnd, rnd);

			Operations::fileImport(wnd, rnd, this)
				.then(
					[wnd, rnd, this] (Project::Ptr prj) -> void {
						if (!prj)
							return;

						ImGuiIO &io = ImGui::GetIO();

						if (io.KeyCtrl)
							return;

						Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[wnd, rnd, this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());

									if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
										launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
								}
							)
							.fail(
								[wnd, rnd, this, prj] (void) -> void {
									Operations::fileRemoveReference(wnd, rnd, this, prj);
								}
							);
					}
				);
		} else {
			stopProject(wnd, rnd);

			Operations::fileImportForNotepad(wnd, rnd, this)
				.then(
					[wnd, rnd, this] (Project::Ptr prj) -> void {
						if (!prj)
							return;

						ImGuiIO &io = ImGui::GetIO();

						if (io.KeyCtrl)
							return;

						Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());
								}
							);
					}
				);
		}
	}
	if (opened) {
		if (s && modifier && !io.KeyShift && !io.KeyAlt && hasUnsavedChanges) {
			if (showRecentProjects()) {
				Operations::fileSave(wnd, rnd, this, false);
			} else {
				Operations::fileSaveForNotepad(wnd, rnd, this, false);
			}
		}
	}
	if (!opened) {
		if (showRecentProjects()) {
			if (!modifier && !io.KeyShift && io.KeyAlt && returnR) {
				if (recentProjectSelectedIndex() >= 0 && recentProjectSelectedIndex() < (int)projects().size()) {
					const Project::Ptr &prj = projects()[recentProjectSelectedIndex()];
					delay(
						std::bind(
							[wnd, rnd, this, prj] (void) -> void {
								showProjectProperty(wnd, rnd, prj.get(), false);
							}
						),
						"SHOW PROJECT PROPERTY"
					);
				}
			}

			if (!modifier && !io.KeyShift && !io.KeyAlt && !recentProjectsFilter().enabled) {
				auto getLinesPerPage = [&] (void) -> int {
					ImGuiStyle &style = ImGui::GetStyle();

					const bool iconView = settings().recentIconView;

					const float menuH = menuHeight();
					const float height = (float)rnd->height() - menuH - style.WindowPadding.y;
					const float itemHeight = iconView ?
						recentProjectItemHeight() + 4 + style.ItemSpacing.y + 5 :
						recentProjectItemHeight() + 4;
					const int linesPerPage = (int)std::floor(height / itemHeight);

					return linesPerPage;
				};

				int dx = 0;
				int dy = 0;
				int fully = 0;
				if (esc) {
					recentProjectSelectingIndex(-1);
					recentProjectSelectedIndex(-1);
				} else if (right || d) {
					dx = 1;
				} else if (left || a) {
					dx = -1;
				} else if (down || s) {
					dy = 1;
				} else if (up || w) {
					dy = -1;
				} else if (end) {
					fully = 1;
				} else if (home) {
					fully = -1;
				} else if (pgUp) {
					const bool iconView = settings().recentIconView;
					int lns = getLinesPerPage();
					if (iconView)
						lns *= recentProjectsColumnCount();
					dy = -lns;
				} else if (pgDown) {
					const bool iconView = settings().recentIconView;
					int lns = getLinesPerPage();
					if (iconView)
						lns *= recentProjectsColumnCount();
					dy = lns;
				}
				navigate(wnd, rnd, dx, dy, fully, return_ || k, del, f2);
			}
		}
	}

	// Project operations.
	if (opened) {
		if (n && modifier && !io.KeyShift && !io.KeyAlt) {
			switch (category()) {
			case Categories::FONT:
				Operations::fontAddPage(wnd, rnd, this);

				break;
			case Categories::CODE:
				Operations::codeAddPage(wnd, rnd, this);

				break;
			case Categories::TILES:
				Operations::tilesAddPage(wnd, rnd, this);

				break;
			case Categories::MAP:
				Operations::mapAddPage(wnd, rnd, this);

				break;
			case Categories::MUSIC:
				Operations::musicAddPage(wnd, rnd, this);

				break;
			case Categories::SFX:
				Operations::sfxAddPage(wnd, rnd, this);

				break;
			case Categories::ACTOR:
				Operations::actorAddPage(wnd, rnd, this);

				break;
			case Categories::SCENE:
				Operations::sceneAddPage(wnd, rnd, this);

				break;
			default: // Do nothing.
				break;
			}
		}
		if (
#if defined GBBASIC_OS_HTML
			backspace &&
#else /* GBBASIC_OS_HTML */
			f4 &&
#endif /* GBBASIC_OS_HTML */
			modifier && !io.KeyShift && !io.KeyAlt
		) {
			stopProject(wnd, rnd);

			destroyAudioDevice();

			categoryOfAudio(Categories::MUSIC);

			Operations::fileClose(wnd, rnd, this);
		} else if (r && modifier && io.KeyShift && !io.KeyAlt) {
#if !defined GBBASIC_OS_HTML
			Project::Ptr &prj = currentProject();

			stopProject(wnd, rnd);

			Operations::projectReload(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());
#endif /* Platform macro. */
		} else if (f4 && !modifier && !io.KeyShift && !io.KeyAlt) {
			if (!canvasDevice()) {
				const Project::Ptr &prj = currentProject();
				const bool isEditable = prj->contentType() == Project::ContentTypes::BASIC;

				if (isEditable)
					launchProject(wnd, rnd, nullptr, nullptr, nullptr, false, -1);
			}
		} else if ((f5 && !modifier && !io.KeyShift && !io.KeyAlt) || (r && modifier && !io.KeyShift && !io.KeyAlt)) {
			if (canvasDevice()) {
				if (canvasDevice()->paused())
					canvasDevice()->resume();
			} else {
				launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
			}
		} else if ((f5 && !modifier && io.KeyShift && !io.KeyAlt) || (period && modifier && !io.KeyShift && !io.KeyAlt)) {
			if (canvasDevice()) {
				stopProject(wnd, rnd);
			}
		} else if (b && modifier && io.KeyShift && !io.KeyAlt) {
			int toExport_ = -1;
			for (int i = 0; i < (int)exporters().size(); ++i) {
				Exporter::Ptr &ex = exporters()[i];
				if (ex->isMajor() && !ex->messageEnabled()) {
					toExport_ = i;

					break;
				}
			}
			exportProject(wnd, rnd, toExport_);
		} else if (v && modifier && io.KeyShift && !io.KeyAlt) {
			int toExport_ = -1;
			for (int i = 0; i < (int)exporters().size(); ++i) {
				Exporter::Ptr &ex = exporters()[i];
				if (ex->isMinor() && !ex->messageEnabled()) {
					toExport_ = i;

					break;
				}
			}
			exportProject(wnd, rnd, toExport_);
		}
	}

	// Edit view operations.
	if (opened) {
		if (modifier && !io.KeyShift && !io.KeyAlt) {
			Project::Ptr &prj = currentProject();

			if (!prj || prj->contentType() == Project::ContentTypes::BASIC) {
				if (grave && canvasDevice()) {
					category(Categories::EMULATOR);
				} else if (num1) {
					category(Categories::CODE);
				} else if (num2) {
					category(Categories::TILES);
				} else if (num3) {
					category(Categories::MAP);
				} else if (num4) {
					category(Categories::SCENE);
				} else if (num5) {
					category(Categories::ACTOR);
				} else if (num6) {
					category(Categories::FONT);
				} else if (num7) {
					category(Categories::MUSIC);
				} else if (num8) {
					category(Categories::SFX);
				} else if (num9) {
					category(Categories::CONSOLE);
				} else if (num0) {
					Project::Ptr &prj = currentProject();

					showPaletteEditor(
						rnd,
						-1,
						[prj] (const std::initializer_list<Variant> &args) -> void { // Color or group has been changed.
							(void)args;

							prj->hasDirtyAsset(true);
						}
					);
				}
			} else {
				if (grave && canvasDevice()) {
					category(Categories::EMULATOR);
				} else if (num9) {
					category(Categories::CONSOLE);
				}
			}

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			if (backSlash) {
				switch (category()) {
				case Categories::CODE: {
						const int majorIdx = prj->activeMajorCodeIndex();
						const int minorIdx = prj->activeMinorCodeIndex();
						if (prj->isMinorCodeEditorEnabled()) {
							prj->isMajorCodeEditorActive(true);
							prj->isMinorCodeEditorEnabled(false);
						} else {
							prj->isMajorCodeEditorActive(false);
							prj->isMinorCodeEditorEnabled(true);
							if (minorIdx == -1)
								prj->activeMinorCodeIndex(majorIdx);
							else
								prj->activeMinorCodeIndex(Math::clamp(minorIdx, 0, prj->codePageCount() - 1));
						}
					}

					break;
				default:
					// Do nothing.

					break;
				}
			}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		}
	}

	// Recent view operations.
	if (category() == Categories::HOME) {
		if (showRecentProjects()) {
			if ((f && modifier && !io.KeyShift && !io.KeyAlt) || (f3 && !modifier && !io.KeyShift && !io.KeyAlt)) {
				if (recentProjectsFilter().enabled) {
					recentProjectsFilter().enabled = false;
					recentProjectsFilter().initialized = false;
					recentProjectsFilterFocused(false);
				} else {
					recentProjectsFilter().enabled = true;
					recentProjectsFilterFocused(false);
				}
			}
		}
	}

	// Help operations.
	if (f1 && !modifier && !io.KeyShift && !io.KeyAlt) {
		toggleDocument(nullptr);
	}
}

void Workspace::navigate(Window* wnd, Renderer* rnd, int dx, int dy, int fully, bool open, bool remove, bool rename) {
	if (recentProjectsFilterFocused() || projectIndices().empty())
		return;

	auto indexOf = [] (const ProjectIndices &indices, int idx) -> int {
		for (int i = 0; i < (int)indices.size(); ++i) {
			if (indices[i] == idx)
				return i;
		}

		return -1;
	};

	const bool iconView = settings().recentIconView;
	const int n = (int)projectIndices().size();
	if (dx == 1 && iconView) {
		const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
		if (ord < 0 || ord >= n) {
			const int idx = projectIndices().front();
			recentProjectSelectingIndex(idx);
		} else {
			int ord_ = ord + 1;
			if (ord_ >= n)
				ord_ = 0;
			const int idx = projectIndices()[ord_];
			recentProjectSelectingIndex(idx);
		}

		return;
	}
	if (dx == -1 && iconView) {
		const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
		if (ord < 0 || ord >= n) {
			const int idx = projectIndices().back();
			recentProjectSelectingIndex(idx);
		} else {
			int ord_ = ord - 1;
			if (ord_ < 0)
				ord_ = n - 1;
			const int idx = projectIndices()[ord_];
			recentProjectSelectingIndex(idx);
		}

		return;
	}
	if (dy == 1) {
		if (iconView) {
			const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
			if (ord < 0 || ord >= n) {
				const int idx = projectIndices().front();
				recentProjectSelectingIndex(idx);
			} else {
				int ord_ = ord + recentProjectsColumnCount();
				if (ord_ >= n) {
					if (ord == n - 1) {
						ord_ %= recentProjectsColumnCount();
					} else if (n % recentProjectsColumnCount() == 0) {
						ord_ %= n;
					} else {
						const std::div_t div = std::div(n, recentProjectsColumnCount());
						const int l = div.quot + (div.rem ? 1 : 0);
						const std::div_t div_ = std::div(ord + 1, recentProjectsColumnCount());
						const int l_ = div_.quot + (div_.rem ? 1 : 0);
						if (l == l_) {
							ord_ = (ord_ % n) - 1;
							ord_ = Math::max(ord_, 0);
						} else {
							ord_ = n - 1;
						}
					}
				}
				const int idx = projectIndices()[ord_];
				recentProjectSelectingIndex(idx);
			}
		} else {
			const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
			if (ord < 0 || ord >= n) {
				const int idx = projectIndices().front();
				recentProjectSelectingIndex(idx);
			} else {
				int ord_ = ord + 1;
				if (ord_ >= n)
					ord_ = 0;
				const int idx = projectIndices()[ord_];
				recentProjectSelectingIndex(idx);
			}
		}

		return;
	}
	if (dy == -1) {
		if (iconView) {
			const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
			if (ord < 0 || ord >= n) {
				const int idx = projectIndices().back();
				recentProjectSelectingIndex(idx);
			} else {
				int ord_ = ord - recentProjectsColumnCount();
				if (ord_ < 0) {
					if (n % recentProjectsColumnCount() == 0) {
						ord_ = n + (ord_ % n);
					} else {
						const std::div_t div = std::div(n, recentProjectsColumnCount());
						const int l = div.quot + (div.rem ? 1 : 0);
						const int m = l * recentProjectsColumnCount();
						ord_ = m + (ord_ % m);
						ord_ = Math::min(ord_, n - 1);
					}
				}
				const int idx = projectIndices()[ord_];
				recentProjectSelectingIndex(idx);
			}
		} else {
			const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
			if (ord < 0 || ord >= n) {
				const int idx = projectIndices().back();
				recentProjectSelectingIndex(idx);
			} else {
				int ord_ = ord - 1;
				if (ord_ < 0)
					ord_ = n - 1;
				const int idx = projectIndices()[ord_];
				recentProjectSelectingIndex(idx);
			}
		}

		return;
	}

	if (dy > 1) {
		const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
		if (ord < 0 || ord >= n) {
			const int idx = projectIndices().front();
			recentProjectSelectingIndex(idx);
		} else {
			int ord_ = ord + dy;
			if (ord_ >= n)
				ord_ = n - 1;
			const int idx = projectIndices()[ord_];
			recentProjectSelectingIndex(idx);
		}

		return;
	}
	if (dy < -1) {
		const int ord = indexOf(projectIndices(), recentProjectSelectedIndex());
		if (ord < 0 || ord >= n) {
			const int idx = projectIndices().back();
			recentProjectSelectingIndex(idx);
		} else {
			int ord_ = ord + dy;
			if (ord_ < 0)
				ord_ = 0;
			const int idx = projectIndices()[ord_];
			recentProjectSelectingIndex(idx);
		}

		return;
	}

	if (fully == -1) {
		const int idx = projectIndices().front();
		recentProjectSelectingIndex(idx);

		return;
	}
	if (fully == 1) {
		const int idx = projectIndices().back();
		recentProjectSelectingIndex(idx);

		return;
	}

	if (open) {
		if (recentProjectSelectedIndex() >= 0 && recentProjectSelectedIndex() < (int)projects().size()) {
			Project::Ptr &prj = projects()[recentProjectSelectedIndex()];

			closeFilter();

			Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
				.then(
					[wnd, rnd, this, prj] (bool ok) -> void {
						if (!ok)
							return;

						validateProject(prj.get());

						if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
							launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				);
		}

		return;
	}
	if (remove) {
		if (recentProjectSelectedIndex() >= 0 && recentProjectSelectedIndex() < (int)projects().size()) {
			Project::Ptr &prj = projects()[recentProjectSelectedIndex()];
			Operations::fileRemove(wnd, rnd, this, prj);
		}

		return;
	}
	if (rename) {
		if (recentProjectSelectedIndex() >= 0 && recentProjectSelectedIndex() < (int)projects().size()) {
			Project::Ptr &prj = projects()[recentProjectSelectedIndex()];
			Operations::fileRename(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());
		}

		return;
	}
}

void Workspace::dialog(Window*, Renderer*) {
	if (!init().end())
		return;

	if (bubble())
		bubble()->update(this);

	if (popupBox())
		popupBox()->update(this);
}

void Workspace::head(Window* wnd, Renderer* rnd, double delta, unsigned* fpsReq) {
	if (_toUpgrade || _toCompile)
		return;

	if (ImGui::BeginMainMenuBar()) {
		menu(wnd, rnd);
		buttons(wnd, rnd, delta);
		adjust(wnd, rnd, delta, fpsReq);
		tabs(wnd, rnd);

		ImGui::EndMainMenuBar();

		menuHeight(ImGui::GetItemRectSize().y);

		filter(wnd, rnd);
	}
}

void Workspace::menu(Window* wnd, Renderer* rnd) {
	ImGuiStyle &style = ImGui::GetStyle();

	auto buildMenu = [this] (Window* wnd, Renderer* rnd, bool editing) -> bool {
		typedef std::function<void(void)> Launcher;

		const Project::Ptr &prj = currentProject();
		const bool isEditable = prj->contentType() == Project::ContentTypes::BASIC;
		Launcher launches = nullptr;
		if (isEditable) {
			launches = [this, wnd, rnd, editing] (void) -> void {
				if (editing && !canvasDevice()) {
					if (ImGui::MenuItem(theme()->menu_Compile(), "F4")) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, false, -1);
					}
					if (ImGui::MenuItem(theme()->menu_CompileAndRun(), "F5")) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				} else {
					if (ImGui::MenuItem(theme()->menu_StopRunning(), "Shift+F5")) {
						stopProject(wnd, rnd);
					}
					if (ImGui::MenuItem(theme()->menu_Restart())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			};
		} else {
			launches = [this, wnd, rnd, editing] (void) -> void {
				if (editing && !canvasDevice()) {
					if (ImGui::MenuItem(theme()->menu_Run(), "F5")) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				} else {
					if (ImGui::MenuItem(theme()->menu_StopRunning(), "Shift+F5")) {
						stopProject(wnd, rnd);
					}
					if (ImGui::MenuItem(theme()->menu_Restart())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			};
		}

		Exporter* ex = nullptr;
		if (ImGui::ExporterMenu(exporters(), theme()->menu_Build().c_str(), ex, launches, nullptr)) {
			int toExport_ = -1;
			for (int i = 0; i < (int)exporters().size(); ++i) {
				if (exporters()[i].get() == ex) {
					if (ex->messageEnabled()) {
						messagePopupBox(ex->messageContent(), nullptr, nullptr, nullptr);

						break;
					}
					toExport_ = i;

					break;
				}
			}
			if (toExport_ != -1) {
				exportProject(wnd, rnd, toExport_);
			}
		}

		return false;
	};
	auto projectMenu = [this, &style] (Window* wnd, Renderer* rnd, bool opened, bool canSave) -> bool {
		VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

		if (ImGui::BeginMenu(theme()->menu_Project())) {
			Project::Ptr &prj = currentProject();
			const bool sel = recentProjectSelectedIndex() >= 0 && recentProjectSelectedIndex() < (int)projects().size();
			const bool isEditable = !prj || prj->contentType() == Project::ContentTypes::BASIC;

			if (showRecentProjects()) {
				if (isEditable) {
					if (ImGui::MenuItem(theme()->menu_New(), !!prj ? GBBASIC_MODIFIER_KEY_NAME "+Shift+N" : GBBASIC_MODIFIER_KEY_NAME "+N")) {
						closeFilter();

						stopProject(wnd, rnd);

						Operations::fileNew(wnd, rnd, this, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[wnd, rnd, this] (Project::Ptr prj) -> void {
									ImGuiIO &io = ImGui::GetIO();

									if (io.KeyCtrl)
										return;

									Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
										.then(
											[this, prj] (bool ok) -> void {
												if (!ok)
													return;

												validateProject(prj.get());
											}
										);
								}
							);

						projectIndices().clear(); projectIndices().shrink_to_fit();

						recentProjectSelectedIndex(-1);

						ImGui::EndMenu();

						return true;
					}
					if (ImGui::MenuItem(theme()->menu_Import(), !!prj ? GBBASIC_MODIFIER_KEY_NAME "+Shift+O" : GBBASIC_MODIFIER_KEY_NAME "+O")) {
						closeFilter();

						stopProject(wnd, rnd);

						Operations::fileImport(wnd, rnd, this)
							.then(
								[wnd, rnd, this] (Project::Ptr prj) -> void {
									if (!prj)
										return;

									ImGuiIO &io = ImGui::GetIO();

									if (io.KeyCtrl)
										return;

									Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
										.then(
											[wnd, rnd, this, prj] (bool ok) -> void {
												if (!ok)
													return;

												validateProject(prj.get());

												if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
													launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
											}
										)
										.fail(
											[wnd, rnd, this, prj] (void) -> void {
												Operations::fileRemoveReference(wnd, rnd, this, prj);
											}
										);
								}
							);

						ImGui::EndMenu();

						return true;
					}
				}
				if (!opened) {
					if (ImGui::MenuItem(theme()->menu_ClearRecent(), nullptr, nullptr, !projects().empty())) {
						closeFilter();

						stopProject(wnd, rnd);

						Operations::fileClear(wnd, rnd, this);

						projectIndices().clear(); projectIndices().shrink_to_fit();

						recentProjectSelectedIndex(-1);

						ImGui::EndMenu();

						return true;
					}
				}
			} else {
				if (ImGui::MenuItem(theme()->menu_New(), !!prj ? GBBASIC_MODIFIER_KEY_NAME "+Shift+N" : GBBASIC_MODIFIER_KEY_NAME "+N")) {
					stopProject(wnd, rnd);

					Operations::fileNew(wnd, rnd, this, fontConfig().empty() ? nullptr : fontConfig().c_str())
						.then(
							[wnd, rnd, this] (Project::Ptr prj) -> void {
								ImGuiIO &io = ImGui::GetIO();

								if (io.KeyCtrl)
									return;

								Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
									.then(
										[this, prj] (bool ok) -> void {
											if (!ok)
												return;

											validateProject(prj.get());
										}
									);
							}
						);

					ImGui::EndMenu();

					return true;
				}
				if (ImGui::MenuItem(theme()->menu_Open(), !!prj ? GBBASIC_MODIFIER_KEY_NAME "+Shift+O" : GBBASIC_MODIFIER_KEY_NAME "+O")) {
					stopProject(wnd, rnd);

					Operations::fileImportForNotepad(wnd, rnd, this)
						.then(
							[wnd, rnd, this] (Project::Ptr prj) -> void {
								if (!prj)
									return;

								ImGuiIO &io = ImGui::GetIO();

								if (io.KeyCtrl)
									return;

								Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
									.then(
										[this, prj] (bool ok) -> void {
											if (!ok)
												return;

											validateProject(prj.get());
										}
									);
							}
						);

					ImGui::EndMenu();

					return true;
				}
			}
			if (opened) {
				if (isEditable) {
					ImGui::Separator();
					if (ImGui::MenuItem(theme()->menu_Save(), GBBASIC_MODIFIER_KEY_NAME "+S", nullptr, canSave)) {
						if (showRecentProjects()) {
							Operations::fileSave(wnd, rnd, this, false);
						} else {
							Operations::fileSaveForNotepad(wnd, rnd, this, false);
						}
					}
					if (ImGui::MenuItem(theme()->menu_RemoveSramState(), nullptr, nullptr, prj->sramExists(this))) {
						Operations::projectRemoveSram(wnd, rnd, this, prj, false);
					}
					ImGui::Separator();
					if (ImGui::MenuItem(theme()->menu_Palette(), GBBASIC_MODIFIER_KEY_NAME "+0")) {
						showPaletteEditor(
							rnd,
							-1,
							[prj] (const std::initializer_list<Variant> &args) -> void { // Color or group has been changed.
								(void)args;

								prj->hasDirtyAsset(true);
							}
						);
					}
				}
				ImGui::Separator();
#if defined GBBASIC_OS_HTML
				if (!showRecentProjects()) {
					if (isEditable) {
						if (ImGui::MenuItem(theme()->menu_Download())) {
							Operations::fileExportForNotepad(wnd, rnd, this, prj);
						}
					}
				}
#endif /* GBBASIC_OS_HTML */
#if !defined GBBASIC_OS_HTML
				if (ImGui::MenuItem(theme()->menu_Browse())) {
					Operations::fileBrowse(wnd, rnd, this, prj);
				}
				if (!prj->path().empty()) {
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(prj->path());
					}
				}
#endif /* GBBASIC_OS_HTML */
				if (ImGui::MenuItem(theme()->menu_Property())) {
					showProjectProperty(wnd, rnd, prj.get(), true);
				}
			} else if (recentProjectSelectedIndex() >= 0) {
				if (showRecentProjects()) {
					Project::Ptr &prj = projects()[recentProjectSelectedIndex()];
					const bool isEditable_ = !prj || prj->contentType() == Project::ContentTypes::BASIC;
					ImGui::Separator();
					if (ImGui::MenuItem(theme()->menu_Run(), nullptr, nullptr, sel)) {
						closeFilter();

						Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[wnd, rnd, this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());

									launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
								}
							);
					}
					if (isEditable_ && ImGui::MenuItem(theme()->menu_Open(), nullptr, nullptr, sel)) {
						closeFilter();

						Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());
								}
							);
					}
					if (ImGui::MenuItem(theme()->menu_Rename(), nullptr, nullptr, sel)) {
						Operations::fileRename(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());
					}
					if (ImGui::MenuItem(theme()->menu_Remove(), nullptr, nullptr, sel)) {
						Operations::fileRemove(wnd, rnd, this, prj);
					}
					if (ImGui::MenuItem(theme()->menu_RemoveSramState(), nullptr, nullptr, prj->sramExists(this))) {
						Operations::projectRemoveSram(wnd, rnd, this, prj, false);
					}
					if (isEditable_ && ImGui::MenuItem(theme()->menu_Duplicate(), nullptr, nullptr, prj->exists())) {
						Operations::fileDuplicate(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());
					}
					ImGui::Separator();
#if !defined GBBASIC_OS_HTML
					if (ImGui::MenuItem(theme()->menu_Browse())) {
						Operations::fileBrowse(wnd, rnd, this, prj);
					}
					if (!prj->path().empty()) {
						if (ImGui::IsItemHovered()) {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							ImGui::SetTooltip(prj->path());
						}
					}
#endif /* GBBASIC_OS_HTML */
					if (ImGui::MenuItem(theme()->menu_Property())) {
						showProjectProperty(wnd, rnd, prj.get(), false);
					}
				}
			}

			ImGui::EndMenu();
		}

		if (opened) {
#if !defined GBBASIC_OS_HTML
			if (ImGui::MenuItem(theme()->menu_Reload(), GBBASIC_MODIFIER_KEY_NAME "+Shift+R")) {
				Project::Ptr &prj = currentProject();

				stopProject(wnd, rnd);

				Operations::projectReload(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());
			}
#endif /* Platform macro. */
			if (
				ImGui::MenuItem(
					theme()->menu_Close(),
#if defined GBBASIC_OS_HTML
					GBBASIC_MODIFIER_KEY_NAME "+BackSpace"
#else /* GBBASIC_OS_HTML */
					GBBASIC_MODIFIER_KEY_NAME "+F4"
#endif /* GBBASIC_OS_HTML */
				)
			) {
				stopProject(wnd, rnd);

				destroyAudioDevice();

				categoryOfAudio(Categories::MUSIC);

				Operations::fileClose(wnd, rnd, this);

				return true;
			}
		}

		return false;
	};
	auto applicationMenu = [this, &style] (Window* wnd, Renderer* rnd) -> bool {
		VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

		if (ImGui::BeginMenu(theme()->menu_Application())) {
#if defined GBBASIC_OS_WIN || defined GBBASIC_OS_MAC || defined GBBASIC_OS_LINUX
			if (ImGui::MenuItem(theme()->menu_Screenshot(), "F6", nullptr, !recorder()->recording())) {
				recorder()->start(false, nullptr, 1);
			}
			if (ImGui::MenuItem(theme()->menu_RecordGif(), "F7", nullptr, !recorder()->recording())) {
				recorder()->start(true, theme()->imageCursor(), activeFrameRate() * 60); // 1 minute.
			}
			if (ImGui::MenuItem(theme()->menu_StopRecording(), "F8", nullptr, recorder()->recording())) {
				recorder()->stop();
			}
			ImGui::Separator();
#endif /* Platform macro. */
#if WORKSPACE_KERNEL_INSTALLER_ENABLED
			if (ImGui::MenuItem(theme()->menu_Kernels())) {
				showInstalledKernels(wnd, rnd, nullptr);
			}
#endif /* WORKSPACE_KERNEL_INSTALLER_ENABLED */
			if (ImGui::MenuItem(theme()->menu_Preferences())) {
				showPreferences(wnd, rnd, nullptr);
			}
			if (ImGui::MenuItem(theme()->menu_Activities())) {
				showActivities(rnd);
			}
#if !defined GBBASIC_OS_HTML
			if (ImGui::MenuItem(theme()->menu_BrowseData())) {
				std::string path = Path::writableDirectory();
				path = Unicode::toOs(path);
				Platform::browse(path.c_str());
			}
#endif /* GBBASIC_OS_HTML */

			ImGui::EndMenu();
		}

		return false;
	};
	auto helpMenu = [this, &style] (Window* wnd, Renderer* rnd) -> bool {
		VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

		if (ImGui::BeginMenu(theme()->menu_Help())) {
#if WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED
			if (exampleCount()) {
				if (ImGui::BeginMenu(theme()->menu_Examples())) {
					std::string path;
					if (ImGui::ExampleMenu(examples(), path)) {
						closeFilter();

						stopProject(wnd, rnd);

						Operations::fileImportExamples(wnd, rnd, this, path.c_str())
							.then(
								[wnd, rnd, this] (Project::Ptr prj) -> void {
									if (!prj)
										return;

									ImGuiIO &io = ImGui::GetIO();

									if (io.KeyCtrl)
										return;

									Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
										.then(
											[wnd, rnd, this, prj] (bool ok) -> void {
												if (!ok)
													return;

												validateProject(prj.get());

												if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
													launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
											}
										)
										.fail(
											[wnd, rnd, this, prj] (void) -> void {
												Operations::fileRemoveReference(wnd, rnd, this, prj);
											}
										);
								}
							);
					}

					ImGui::EndMenu();
				}
			}
#endif /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */
			if (exampleCount() > 0) {
#if !WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED
				if (showRecentProjects()) {
					if (ImGui::MenuItem(theme()->menu_ImportExamples())) {
						closeFilter();

						stopProject(wnd, rnd);

						Operations::fileImportExamples(wnd, rnd, this, nullptr)
							.then(
								[wnd, rnd, this] (Project::Ptr prj) -> void {
									if (!prj)
										return;

									ImGuiIO &io = ImGui::GetIO();

									if (io.KeyCtrl)
										return;

									Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
										.then(
											[wnd, rnd, this, prj] (bool ok) -> void {
												if (!ok)
													return;

												validateProject(prj.get());

												if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
													launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
											}
										)
										.fail(
											[wnd, rnd, this, prj] (void) -> void {
												Operations::fileRemoveReference(wnd, rnd, this, prj);
											}
										);
								}
							);
					}
				} else {
#	if !defined GBBASIC_OS_HTML
					if (ImGui::MenuItem(theme()->menu_OpenExample())) {
						stopProject(wnd, rnd);

						Operations::fileImportExampleForNotepad(wnd, rnd, this)
							.then(
								[wnd, rnd, this] (Project::Ptr prj) -> void {
									if (!prj)
										return;

									ImGuiIO &io = ImGui::GetIO();

									if (io.KeyCtrl)
										return;

									Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
										.then(
											[this, prj] (bool ok) -> void {
												if (!ok)
													return;

												validateProject(prj.get());
											}
										);
								}
							);
					}
#	endif /* GBBASIC_OS_HTML */
				}
#endif /* WORKSPACE_EXAMPLE_PROJECTS_MENU_ENABLED */
#if !defined GBBASIC_OS_HTML
				if (ImGui::MenuItem(theme()->menu_BrowseExamples())) {
					Operations::fileBrowseExamples(wnd, rnd, this);
				}
#endif /* GBBASIC_OS_HTML */
			}
			if (documents().empty()) {
				if (ImGui::MenuItem(theme()->menu_Manual(), "F1")) {
					toggleDocument(nullptr);
				}
			} else {
				std::string path;
				if (ImGui::DocumentMenu(documents(), path)) {
					toggleDocument(path.c_str());
				}
			}
			if (!links().empty()) {
				typedef std::function<void(void)> Surf;

				ImGui::Separator();
				std::string url;
				std::string message;
				if (ImGui::LinkMenu(links(), url, message)) {
					Surf surf = nullptr;
					if (!url.empty()) {
						surf = [url] (void) -> void {
							const std::string osstr = Unicode::toOs(url);

							Platform::surf(osstr.c_str());
						};
					}
					if (message.empty()) {
						if (surf)
							surf();
					} else {
						messagePopupBox(message, nullptr, nullptr, nullptr);

						if (surf) {
							const long long now = DateTime::ticks();
							const long long tm = now + DateTime::fromSeconds(3);
							delay(surf, "SURF", Delayed::Amount(Delayed::Amount::Types::UNTIL_TIMESTAMP, tm));
						}
					}
				}
			}
			if (hasDefaultKernelSourceCode()) {
				ImGui::Separator();
				if (ImGui::MenuItem(theme()->menu_EjectSourceCodeVm())) {
					ejectSourceCode(wnd, rnd, 0, true, nullptr, nullptr);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem(theme()->menu_About())) {
				showAbout(rnd);
			}

			ImGui::EndMenu();
		}

		return false;
	};
	auto quitMenu = [this] (Window*, Renderer*) -> bool {
#if !defined GBBASIC_OS_HTML
		if (ImGui::MenuItem(theme()->menu_Quit(), "Alt+F4")) {
			SDL_Event evt;
			evt.type = SDL_QUIT;
			SDL_PushEvent(&evt);
		}
#endif /* GBBASIC_OS_HTML */

		return false;
	};

	Texture* tex = theme()->iconMainMenu();
	const ImVec2 texSize((float)tex->width(), (float)tex->height());
	const bool menuOpened_ = menuOpened();
	if (menuOpened_) {
		ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(theme()->style()->selectedButtonColor));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorConvertU32ToFloat4(theme()->style()->selectedButtonColor));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorConvertU32ToFloat4(theme()->style()->selectedButtonColor));
	}
	if (ImGui::BeginMenu(ImGui::GetID("#Main"), tex->pointer(rnd), texSize, _state != States::COMPILING && !analyzing())) {
		WORKSPACE_MAIN_MENU_GUARD(guardMainMenu);

		if (menuOpened_) {
			ImGui::PopStyleColor(3);
		}

		menuOpened(true);

		closeFilter();

		bool opened = false;
		bool hasUnsavedChanges = false;
		getCurrentProjectStates(&opened, &hasUnsavedChanges, nullptr, nullptr);

		const char* undoable = nullptr;
		const char* redoable = nullptr;
		bool readonly = false;
		getCurrentAssetStates(
			nullptr,
			nullptr,
			nullptr,
			nullptr, nullptr,
			&undoable, &redoable,
			&readonly
		);

		if (document()) {
			if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
			ImGui::Separator();
			if (applicationMenu(wnd, rnd)) return;
			if (helpMenu(wnd, rnd)) return;
			if (quitMenu(wnd, rnd)) return;
		} else {
			switch (category()) {
			case Categories::HOME:
				if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
				ImGui::Separator();
				if (applicationMenu(wnd, rnd)) return;
				if (helpMenu(wnd, rnd)) return;
				if (quitMenu(wnd, rnd)) return;

				break;
			case Categories::FONT: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Font())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::fontAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::fontRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::FONT);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::CODE: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Code())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::codeAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::codeRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Find(), GBBASIC_MODIFIER_KEY_NAME "+F")) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::FIND, false);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_FindInProject(), GBBASIC_MODIFIER_KEY_NAME "+Shift+F")) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::FIND, true);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Goto(), GBBASIC_MODIFIER_KEY_NAME "+G")) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::GOTO);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_IncreaseIndent(), "Tab", nullptr, !readonly)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::INDENT, false);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_DecreaseIndent(), "Shift+Tab", nullptr, !readonly)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::UNINDENT, false);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_ToLowerCase(), GBBASIC_MODIFIER_KEY_NAME "+U", nullptr, !readonly)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::TO_LOWER_CASE);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_ToUpperCase(), GBBASIC_MODIFIER_KEY_NAME "+Shift+U", nullptr, !readonly)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::TO_UPPER_CASE);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_ToggleComment(), GBBASIC_MODIFIER_KEY_NAME "+/", nullptr, !readonly)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
									editor->post(Editable::TOGGLE_COMMENT);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::CODE);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::TILES: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Tiles())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::tilesAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::tilesRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::TILES);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::MAP: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Map())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::mapAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::mapRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::MAP);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::MUSIC: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Music())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::musicAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::musicRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::MUSIC);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::SFX: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Sfx())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::sfxAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::sfxRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::SFX);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::ACTOR: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Actor())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::actorAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::actorRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::ACTOR);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::SCENE: {
					VariableGuard<decltype(style.ChildBorderSize)> guardChildBorderSize(&style.ChildBorderSize, style.ChildBorderSize, 1);

					if (ImGui::BeginMenu(theme()->menu_Scene())) {
						if (ImGui::MenuItem(theme()->menu_New(), GBBASIC_MODIFIER_KEY_NAME "+N")) {
							Operations::sceneAddPage(wnd, rnd, this);
						}
						if (ImGui::MenuItem(theme()->menu_Remove())) {
							Operations::sceneRemovePage(wnd, rnd, this);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_Undo(), GBBASIC_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->undo(entry);
								}
							);
						}
						if (ImGui::MenuItem(theme()->menu_Redo(), GBBASIC_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
							withCurrentAsset(
								[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
									editor->redo(entry);
								}
							);
						}
						ImGui::Separator();
						if (ImGui::MenuItem(theme()->menu_SortAssets(), GBBASIC_MODIFIER_KEY_NAME "+,")) {
							Operations::editSortAssets(wnd, rnd, this, AssetsBundle::Categories::SCENE);
						}

						ImGui::EndMenu();
					}
					if (buildMenu(wnd, rnd, true)) return;
					ImGui::Separator();
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
					ImGui::Separator();
					if (applicationMenu(wnd, rnd)) return;
					if (helpMenu(wnd, rnd)) return;
					if (quitMenu(wnd, rnd)) return;
				}

				break;
			case Categories::CONSOLE:
				if (ImGui::MenuItem(theme()->menu_ClearConsole())) {
					clear();
				}
				if (buildMenu(wnd, rnd, true)) return;
				ImGui::Separator();
				if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
				ImGui::Separator();
				if (applicationMenu(wnd, rnd)) return;
				if (helpMenu(wnd, rnd)) return;
				if (quitMenu(wnd, rnd)) return;

				break;
			case Categories::EMULATOR:
				if (buildMenu(wnd, rnd, false)) return;
				if (opened) {
					if (projectMenu(wnd, rnd, opened, hasUnsavedChanges)) return;
				}
				ImGui::Separator();
				if (applicationMenu(wnd, rnd)) return;
				if (helpMenu(wnd, rnd)) return;
				if (quitMenu(wnd, rnd)) return;

				break;
			default: // Do nothing.
				break;
			}
		}
	} else {
		if (menuOpened_) {
			ImGui::PopStyleColor(3);
		}

		menuOpened(false);
	}
	if (ImGui::IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(theme()->tooltip_Menu());
	}
}

void Workspace::buttons(Window* wnd, Renderer* rnd, double delta) {
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	if (document()) {
		if (ImGui::MenuBarImageButton(theme()->iconBack()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Back().c_str())) {
			toggleDocument(nullptr);
		}

		return;
	}

	auto undoRedo = [&] (void) -> void {
		if (rnd->width() < 370)
			return;

		bool any = false;
		unsigned type = 0;
		const char* undoable = nullptr;
		const char* redoable = nullptr;
		getCurrentAssetStates(
			&any,
			&type,
			nullptr,
			nullptr, nullptr,
			&undoable, &redoable,
			nullptr
		);
		if (!any)
			return;

		if ((Categories)type == Categories::SFX) {
			Project::Ptr &prj = currentProject();
			if (!prj || (prj->sfxPageCount() == 0 && !undoable && !redoable))
				return;
		}

		ImGui::BeginDisabled();
		ImGui::MenuBarImageButton(theme()->iconSeparator()->pointer(rnd), ImVec2(3, 13), ImVec4(1, 1, 1, 1));
		ImGui::EndDisabled();

		if (undoable) {
			if (ImGui::MenuBarImageButton(theme()->iconUndo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Undo().c_str())) {
				withCurrentAsset(
					[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
						editor->undo(entry);
					}
				);
			}
		} else {
			ImGui::BeginDisabled();
			ImGui::MenuBarImageButton(theme()->iconUndo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Undo().c_str());
			ImGui::EndDisabled();
		}
		if (redoable) {
			if (ImGui::MenuBarImageButton(theme()->iconRedo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Redo().c_str())) {
				withCurrentAsset(
					[] (Categories /* cat */, BaseAssets::Entry* entry, Editable* editor) -> void {
						editor->redo(entry);
					}
				);
			}
		} else {
			ImGui::BeginDisabled();
			ImGui::MenuBarImageButton(theme()->iconRedo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Redo().c_str());
			ImGui::EndDisabled();
		}
	};
	auto download = [&] (void) -> void {
#if defined GBBASIC_OS_HTML
		if (!showRecentProjects()) {
			ImGui::BeginDisabled();
			ImGui::MenuBarImageButton(theme()->iconSeparator()->pointer(rnd), ImVec2(3, 13), ImVec4(1, 1, 1, 1));
			ImGui::EndDisabled();

			if (ImGui::MenuBarImageButton(theme()->iconDownload()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_DownloadProject().c_str())) {
				Project::Ptr &prj = currentProject();

				Operations::fileExportForNotepad(wnd, rnd, this, prj);
			}
		}
#endif /* GBBASIC_OS_HTML */
	};

	switch (category()) {
	case Categories::HOME:
		if (showRecentProjects()) {
			if (ImGui::MenuBarImageButton(theme()->iconOpen()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Import().c_str())) {
				closeFilter();

				stopProject(wnd, rnd);

				Operations::fileImport(wnd, rnd, this)
					.then(
						[wnd, rnd, this] (Project::Ptr prj) -> void {
							if (!prj)
								return;

							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl)
								return;

							Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[wnd, rnd, this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());

										if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
											launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
									}
								)
								.fail(
									[wnd, rnd, this, prj] (void) -> void {
										Operations::fileRemoveReference(wnd, rnd, this, prj);
									}
								);
						}
					);
			}
			recentProjectsFilterPosition(ImGui::GetCursorPos());
			if (recentProjectsFilter().enabled) {
				WIDGETS_SELECTION_GUARD(theme());

				if (ImGui::MenuBarImageButton(theme()->iconFind()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Find().c_str())) {
					recentProjectsFilter().enabled = false;
					recentProjectsFilter().initialized = false;
					recentProjectsFilterFocused(false);
				}
			} else {
				if (ImGui::MenuBarImageButton(theme()->iconFind()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Find().c_str())) {
					recentProjectsFilter().enabled = true;
					recentProjectsFilterFocused(false);
				}
			}
			const char* const TOOLTIPS[] = {
				theme()->tooltip_OrderedByDefault().c_str(),
				theme()->tooltip_OrderedByTitle().c_str(),
				theme()->tooltip_OrderedByLatestModified().c_str(),
				theme()->tooltip_OrderedByEarliestModified().c_str(),
				theme()->tooltip_OrderedByLatestCreated().c_str(),
				theme()->tooltip_OrderedByEarliestCreated().c_str()
			};
			unsigned ord = Math::clamp(settings().recentProjectsOrder, (unsigned)0, (unsigned)Orders::COUNT - 1);
			if (ImGui::MenuBarImageButton(theme()->iconSort()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), TOOLTIPS[ord])) {
				if (++ord >= (unsigned)Orders::COUNT)
					ord = 0;
				settings().recentProjectsOrder = ord;
				sortProjects();

				switch ((Orders)settings().recentProjectsOrder) {
				case Orders::DEFAULT:
					recentProjectSort().enabled = true;
					recentProjectSort().ticks = 0;
					recentProjectSort().text = theme()->status_Default();

					break;
				case Orders::TITLE:
					recentProjectSort().enabled = true;
					recentProjectSort().ticks = 0;
					recentProjectSort().text = theme()->status_Title();

					break;
				case Orders::LATEST_MODIFIED:
					recentProjectSort().enabled = true;
					recentProjectSort().ticks = 0;
					recentProjectSort().text = theme()->status_LatestModified();

					break;
				case Orders::EARLIEST_MODIFIED:
					recentProjectSort().enabled = true;
					recentProjectSort().ticks = 0;
					recentProjectSort().text = theme()->status_EarliestModified();

					break;
				case Orders::LATEST_CREATED:
					recentProjectSort().enabled = true;
					recentProjectSort().ticks = 0;
					recentProjectSort().text = theme()->status_LatestCreated();

					break;
				case Orders::EARLIEST_CREATED:
					recentProjectSort().enabled = true;
					recentProjectSort().ticks = 0;
					recentProjectSort().text = theme()->status_EarliestCreated();

					break;
				default:
					GBBASIC_ASSERT(false && "Impossible.");

					break;
				}
			}
			if (recentProjectSort().enabled) {
				recentProjectSort().ticks += delta;
				const float factor = (float)Math::clamp(recentProjectSort().ticks / WORKSPACE_FADE_TIMEOUT_SECONDS, 0.0, 1.0);
				const ImU32 ucol = ImGui::GetColorU32(ImGuiCol_Text);
				ImVec4 col = ImGui::ColorConvertU32ToFloat4(ucol);
				col.w = 1.0f - factor;
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				{
					ImGui::Dummy(ImVec2(4, menuHeight()));
					ImGui::SameLine();
					ImGui::AlignTextToFramePadding();
					ImGui::MenuBarTextUnformatted(recentProjectSort().text);
				}
				ImGui::PopStyleColor();
				if (recentProjectSort().ticks > WORKSPACE_FADE_TIMEOUT_SECONDS) {
					recentProjectSort().enabled = false;
					recentProjectSort().ticks = 0;
					recentProjectSort().text.clear();
				}
			}
		} else {
			if (ImGui::MenuBarImageButton(theme()->iconOpen()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Open().c_str())) {
				stopProject(wnd, rnd);

				Operations::fileImportForNotepad(wnd, rnd, this)
					.then(
						[wnd, rnd, this] (Project::Ptr prj) -> void {
							if (!prj)
								return;

							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl)
								return;

							Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());
									}
								);
						}
					);
			}
		}

		break;
	case Categories::FONT: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::fontAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::fontRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::CODE: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::codeAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::codeRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (analyzing()) {
						ImGui::MenuBarImageButton(theme()->iconWaiting()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 0.75f), theme()->tooltip_Analyzing().c_str());
					} else {
						if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
							launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
						}
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::TILES: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::tilesAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::tilesRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::MAP: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::mapAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::mapRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::MUSIC: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::musicAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::musicRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::SFX: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::sfxAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::sfxRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::ACTOR: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::actorAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::actorRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					if (showRecentProjects()) {
						Operations::fileSave(wnd, rnd, this, false);
					} else {
						Operations::fileSaveForNotepad(wnd, rnd, this, false);
					}
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::SCENE: {
			bool hasUnsavedChanges = false;
			getCurrentProjectStates(nullptr, &hasUnsavedChanges, nullptr, nullptr);

			if (ImGui::MenuBarImageButton(theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_New().c_str())) {
				Operations::sceneAddPage(wnd, rnd, this);
			}
			if (ImGui::MenuBarImageButton(theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Delete().c_str())) {
				Operations::sceneRemovePage(wnd, rnd, this);
			}
			if (hasUnsavedChanges) {
				if (ImGui::MenuBarImageButton(theme()->iconSave()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipFile_Save().c_str())) {
					Operations::fileSave(wnd, rnd, this, false);
				}
			} else {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			}
			undoRedo();
			download();
		}

		break;
	case Categories::CONSOLE: {
			if (ImGui::MenuBarImageButton(theme()->iconClear()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Clear().c_str())) {
				clear();
			}
			if (_state == States::IDLE) {
				if (canvasDevice()) {
					if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
						stopProject(wnd, rnd);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_CompileAndRun().c_str())) {
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
					}
				}
			} else {
				ImGui::MenuBarImageButton(theme()->iconWaiting()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 0.75f), theme()->tooltip_Compiling().c_str());
			}
			download();
		}

		break;
	case Categories::EMULATOR: {
			if (settings().inputOnscreenGamepadEnabled) {
				WIDGETS_SELECTION_GUARD(theme());

				if (ImGui::MenuBarImageButton(theme()->iconGamepad()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEmulator_ToggleOnscreenGamepad().c_str())) {
					settings().inputOnscreenGamepadEnabled = false;
				}
			} else {
				if (ImGui::MenuBarImageButton(theme()->iconGamepad()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEmulator_ToggleOnscreenGamepad().c_str())) {
					settings().inputOnscreenGamepadEnabled = true;
				}
			}
			if (canvasDevice()->paused()) {
				if (ImGui::MenuBarImageButton(theme()->iconPlay()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Resume().c_str())) {
					canvasDevice()->resume();
				}
			} else {
				if (ImGui::MenuBarImageButton(theme()->iconPause()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_Pause().c_str())) {
					canvasDevice()->pause();
				}
			}
			if (ImGui::MenuBarImageButton(theme()->iconStop()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipProject_Stop().c_str())) {
				stopProject(wnd, rnd);
			}
			download();
		}

		break;
	default: // Do nothing.
		break;
	}
}

void Workspace::adjust(Window* wnd, Renderer* rnd, double /* delta */, unsigned* fpsReq) {
#if WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED
	// Prepare.
	if (wnd->bordered())
		return;

	const bool hasPopup = !!popupBox();

	int mx = 0;
	int my = 0;
	bool lmb = false;
	input()->immediateScreenTouch(wnd, rnd, &mx, &my, &lmb, nullptr, nullptr, nullptr, nullptr);
	const Math::Vec2i mpos(mx, my);

	const float width = (float)rnd->width();
	const float height = (float)rnd->height();
	const bool isHoveringHeadRect = ImGui::IsMouseHoveringRect(
		ImVec2(ImGui::GetCursorPosX(), 0),
		ImVec2(width - tabsWidth(), 19),
		false
	);

	const float margin = 3;
	const float padding = 2;
	const bool isHoveringUpBorder = ImGui::IsMouseHoveringRect(
		ImVec2(ImGui::GetCursorPosX(), 0),
		ImVec2(width - tabsWidth(), margin),
		false
	);
	const bool isHoveringDownBorder = ImGui::IsMouseHoveringRect(
		ImVec2(padding, height - margin),
		ImVec2(width - padding * 2, height),
		false
	);
	const bool isHoveringLeftBorder = ImGui::IsMouseHoveringRect(
		ImVec2(0, padding),
		ImVec2(margin, height - padding * 2),
		false
	);
	const bool isHoveringRightBorder = ImGui::IsMouseHoveringRect(
		ImVec2(width - margin, padding),
		ImVec2(width, height - padding * 2),
		false
	);
	const bool isHoveringUpLeftCorner = ImGui::IsMouseHoveringRect(
		ImVec2(0, 0),
		ImVec2(margin, margin),
		false
	);
	const bool isHoveringUpRightCorner = ImGui::IsMouseHoveringRect(
		ImVec2(width - margin, 0),
		ImVec2(width, margin),
		false
	);
	const bool isHoveringDownLeftCorner = ImGui::IsMouseHoveringRect(
		ImVec2(0, height - margin),
		ImVec2(margin, height),
		false
	);
	const bool isHoveringDownRightCorner = ImGui::IsMouseHoveringRect(
		ImVec2(width - margin, height - margin),
		ImVec2(width, height),
		false
	);

	auto resetResizing = [this, &lmb] (void) -> void {
		borderMouseDown(false);
		ignoreBorderMouseResizing(false);
		lmb = false;
	};
	auto resetMoving = [this, &lmb] (void) -> void {
		headBarMouseDown(false);
		ignoreHeadBarMouseMoving(false);
		lmb = false;
	};

	// Maximize and restore.
	if (wnd->fullscreen() && !hasPopup) {
		if (isHoveringHeadRect && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			toggleFullscreen(wnd);

			wnd->maximize();

			const Window::Orientations orientation = wnd->orientation();
			int nl = 0, nt = 0, nr = 0, nb = 0;
			Platform::notch(&nl, &nt, &nr, &nb);
			const Math::Recti notch(nl, nt, nr, nb);

			const Math::Recti bounds = wnd->displayBounds();
			const Math::Vec2i wPos(bounds.xMin(), bounds.yMin());
			wnd->position(wPos);
			const Math::Vec2i wSize(bounds.width(), bounds.height());
			wnd->size(wSize);
			resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

			resetResizing();
			resetMoving();
		}
	} else if (!hasPopup) {
		if (isHoveringHeadRect && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			toggleMaximized(wnd);

			const Window::Orientations orientation = wnd->orientation();
			int nl = 0, nt = 0, nr = 0, nb = 0;
			Platform::notch(&nl, &nt, &nr, &nb);
			const Math::Recti notch(nl, nt, nr, nb);

			if (!wnd->maximized()) {
				const Math::Vec2i wPos = settings().applicationWindowPosition;
				wnd->position(wPos);
				moved(wnd, rnd, wnd->position());

				const Math::Vec2i wSize = settings().applicationWindowSize;
				wnd->size(wSize);
				resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);
			}

			resetResizing();
			resetMoving();
		}
	}

	// Resize.
	if (!wnd->maximized() && !wnd->fullscreen()) {
		Directions dir = Directions::DIRECTION_NONE;
		if (!borderMouseDown()) {
			if (isHoveringUpBorder) {
				dir = Directions::DIRECTION_UP;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			}
			if (isHoveringDownBorder) {
				dir = Directions::DIRECTION_DOWN;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			}
			if (isHoveringLeftBorder) {
				dir = Directions::DIRECTION_LEFT;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			if (isHoveringRightBorder) {
				dir = Directions::DIRECTION_RIGHT;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			if (isHoveringUpLeftCorner) {
				dir = Directions::DIRECTION_UP_LEFT;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
			}
			if (isHoveringUpRightCorner) {
				dir = Directions::DIRECTION_UP_RIGHT;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
			}
			if (isHoveringDownLeftCorner) {
				dir = Directions::DIRECTION_DOWN_LEFT;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
			}
			if (isHoveringDownRightCorner) {
				dir = Directions::DIRECTION_DOWN_RIGHT;
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
			}
		}

		if (!borderMouseDown() && lmb && !ignoreBorderMouseResizing()) {
			if (dir != Directions::DIRECTION_NONE) { // Begin.
				borderMouseDown(true);
				borderMouseDownPosition(mpos);
				borderResizingDirection((unsigned)dir);
				ignoreHeadBarMouseMoving(true);
			} else { // Ignore.
				ignoreBorderMouseResizing(true);
			}
		}
		if (borderMouseDown()) {
			if (lmb) { // Move.
				if (!ignoreBorderMouseResizing()) {
					switch ((Directions)borderResizingDirection()) {
					case Directions::DIRECTION_DOWN: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
					case Directions::DIRECTION_RIGHT: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
					case Directions::DIRECTION_UP: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
					case Directions::DIRECTION_LEFT: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
					case Directions::DIRECTION_DOWN_RIGHT: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
					case Directions::DIRECTION_UP_RIGHT: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
					case Directions::DIRECTION_UP_LEFT: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
					case Directions::DIRECTION_DOWN_LEFT: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
					default: /* Do nothing. */ break;
					}

					const Math::Vec2i diff = mpos - borderMouseDownPosition();
					if (diff != Math::Vec2i(0, 0)) {
						const Window::Orientations orientation = wnd->orientation();
						int nl = 0, nt = 0, nr = 0, nb = 0;
						Platform::notch(&nl, &nt, &nr, &nb);
						const Math::Recti notch(nl, nt, nr, nb);

						const Math::Vec2i minSize = wnd->minimumSize();
						Math::Vec2i wPos = wnd->position();
						Math::Vec2i wSize = wnd->size();

						switch ((Directions)borderResizingDirection()) {
						case Directions::DIRECTION_DOWN:
							if (diff.y < 0 && wSize.y <= minSize.y)
								break;

							wSize.y += diff.y;

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							borderMouseDownPosition(mpos);

							break;
						case Directions::DIRECTION_RIGHT:
							if (diff.x < 0 && wSize.x <= minSize.x)
								break;

							wSize.x += diff.x;

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							borderMouseDownPosition(mpos);

							break;
						case Directions::DIRECTION_UP:
							if (diff.y > 0 && wSize.y <= minSize.y)
								break;

							wPos.y += diff.y;
							wSize.y -= diff.y;

							wnd->position(wPos);
							moved(wnd, rnd, wnd->position());

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							break;
						case Directions::DIRECTION_LEFT:
							if (diff.x > 0 && wSize.x <= minSize.x)
								break;

							wPos.x += diff.x;
							wSize.x -= diff.x;

							wnd->position(wPos);
							moved(wnd, rnd, wnd->position());

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							break;
						case Directions::DIRECTION_DOWN_RIGHT:
							if (!(diff.y < 0 && wSize.y <= minSize.y)) {
								wSize.y += diff.y;

								borderMouseDownPosition(Math::Vec2i(borderMouseDownPosition().x, mpos.y));
							}
							if (!(diff.x < 0 && wSize.x <= minSize.x)) {
								wSize.x += diff.x;

								borderMouseDownPosition(Math::Vec2i(mpos.x, borderMouseDownPosition().y));
							}

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							break;
						case Directions::DIRECTION_UP_RIGHT:
							if (!(diff.y > 0 && wSize.y <= minSize.y)) {
								wPos.y += diff.y;
								wSize.y -= diff.y;

								wnd->position(wPos);
								moved(wnd, rnd, wnd->position());
							}
							if (!(diff.x < 0 && wSize.x <= minSize.x)) {
								wSize.x += diff.x;

								borderMouseDownPosition(Math::Vec2i(mpos.x, borderMouseDownPosition().y));
							}

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							break;
						case Directions::DIRECTION_UP_LEFT:
							if (!(diff.y > 0 && wSize.y <= minSize.y)) {
								wPos.y += diff.y;
								wSize.y -= diff.y;
							}
							if (!(diff.x > 0 && wSize.x <= minSize.x)) {
								wPos.x += diff.x;
								wSize.x -= diff.x;
							}

							wnd->position(wPos);
							moved(wnd, rnd, wnd->position());

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							break;
						case Directions::DIRECTION_DOWN_LEFT:
							if (!(diff.x > 0 && wSize.x <= minSize.x)) {
								wPos.x += diff.x;
								wSize.x -= diff.x;

								wnd->position(wPos);
								moved(wnd, rnd, wnd->position());
							}
							if (!(diff.y < 0 && wSize.y <= minSize.y)) {
								wSize.y += diff.y;

								borderMouseDownPosition(Math::Vec2i(borderMouseDownPosition().x, mpos.y));
							}

							wnd->size(wSize);
							resized(wnd, rnd, wSize, Math::Vec2i(wSize.x / rnd->scale(), wSize.y / rnd->scale()), notch, orientation);

							break;
						default:
							// Do nothing.

							break;
						}
					}

					*fpsReq = GBBASIC_ACTIVE_FRAME_RATE;
				}
			} else { // End.
				borderMouseDown(false);
				ignoreBorderMouseResizing(false);
			}
		}
	}
	if (ignoreBorderMouseResizing() && !lmb) { // End ignoring.
		ignoreBorderMouseResizing(false);
	}

	// Header move.
	if (!wnd->maximized() && !wnd->fullscreen() && !hasPopup) {
		if (!headBarMouseDown() && lmb && !ignoreHeadBarMouseMoving()) {
			if (isHoveringHeadRect) { // Begin.
				headBarMouseDown(true);
				headBarMouseDownPosition(mpos);
				ignoreBorderMouseResizing(true);
			} else { // Ignore.
				ignoreHeadBarMouseMoving(true);
			}
		}
		if (headBarMouseDown()) {
			if (lmb) { // Move.
				if (!ignoreHeadBarMouseMoving()) {
					const Math::Vec2i diff = mpos - headBarMouseDownPosition();
					if (diff != Math::Vec2i(0, 0)) {
						Math::Vec2i wPos = wnd->position();
						wPos.x += (int)diff.x;
						wPos.y += (int)diff.y;
						wnd->position(wPos);
					}

					*fpsReq = GBBASIC_ACTIVE_FRAME_RATE;
				}
			} else { // End.
				headBarMouseDown(false);
				ignoreHeadBarMouseMoving(false);

				moved(wnd, rnd, wnd->position());
			}
		}
	}
	if (ignoreHeadBarMouseMoving() && !lmb) { // End ignoring.
		ignoreHeadBarMouseMoving(false);
	}
#else /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */
	(void)wnd;
	(void)rnd;
	(void)fpsReq;
#endif /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */
}

void Workspace::tabs(Window* wnd, Renderer* rnd) {
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	const bool docOpened = !!document();
	const bool emuOpened = !!canvasDevice();

	float width = 0.0f;
	const float wndWidth = ImGui::GetWindowWidth();
	ImGui::SetCursorPosX(wndWidth - tabsWidth());
	switch (category()) {
	case Categories::HOME:
		if (showRecentProjects()) {
			if (!docOpened) {
#if WORKSPACE_KERNEL_INSTALLER_ENABLED
				if (ImGui::MenuBarImageButton(theme()->iconKernels()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_InstalledKernels().c_str())) {
					showInstalledKernels(wnd, rnd, nullptr);
				}
				width += ImGui::GetItemRectSize().x;
#endif /* WORKSPACE_KERNEL_INSTALLER_ENABLED */
				if (settings().recentIconView) {
					if (ImGui::MenuBarImageButton(theme()->iconListView()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_ListView().c_str())) {
						settings().recentIconView = false;
						recentProjectItemHeight(0.0f);
						recentProjectItemIconWidth(0.0f);
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconIconView()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_IconView().c_str())) {
						settings().recentIconView = true;
						recentProjectItemHeight(0.0f);
						recentProjectItemIconWidth(0.0f);
					}
				}
				width += ImGui::GetItemRectSize().x;
			}
		} else {
			if (!docOpened) {
#if WORKSPACE_KERNEL_INSTALLER_ENABLED
				if (ImGui::MenuBarImageButton(theme()->iconKernels()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltip_InstalledKernels().c_str())) {
					showInstalledKernels(wnd, rnd, nullptr);
				}
				width += ImGui::GetItemRectSize().x;
#endif /* WORKSPACE_KERNEL_INSTALLER_ENABLED */
			}
		}
		if (docOpened) {
			WIDGETS_SELECTION_GUARD(theme());

			if (ImGui::MenuBarImageButton(theme()->iconDocument()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipRecent_Document().c_str())) {
				toggleDocument(nullptr);
			}
		} else {
			if (ImGui::MenuBarImageButton(theme()->iconDocument()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipRecent_Document().c_str())) {
				toggleDocument(nullptr);
			}
		}
		ImGui::SameLine();
		width += ImGui::GetItemRectSize().x;

		break;
	case Categories::FONT: // Fall through.
	case Categories::CODE: // Fall through.
	case Categories::TILES: // Fall through.
	case Categories::MAP: // Fall through.
	case Categories::MUSIC: // Fall through.
	case Categories::SFX: // Fall through.
	case Categories::ACTOR: // Fall through.
	case Categories::SCENE: // Fall through.
	case Categories::CONSOLE: // Fall through.
	case Categories::EMULATOR: {
			const bool busy = _state == States::COMPILING || analyzing();
			const Project::Ptr &prj = currentProject();
			const bool isEditable = prj->contentType() == Project::ContentTypes::BASIC;

			if (category() == Categories::EMULATOR) {
				if (docOpened) {
					if (ImGui::MenuBarImageButton(theme()->iconEmulator()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Emulator().c_str())) {
						toggleDocument(nullptr);
					}
				} else {
					WIDGETS_SELECTION_GUARD(theme());

					if (ImGui::MenuBarImageButton(theme()->iconEmulator()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Emulator().c_str())) {
						// Do nothing.
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;
			} else {
				if (emuOpened) {
					if (ImGui::MenuBarImageButton(theme()->iconEmulator()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Emulator().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::EMULATOR);
					}
					ImGui::SameLine();
					width += ImGui::GetItemRectSize().x;
				}
			}

			if (isEditable) {
				if (category() == Categories::CODE) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconCode()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Code().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconCode()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Code().c_str())) {
							// Do nothing.
						}
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconCode()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Code().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::CODE);
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;

				if (category() == Categories::TILES) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconTiles()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Tiles().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconTiles()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Tiles().c_str())) {
							// Do nothing.
						}
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconTiles()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Tiles().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::TILES);
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;

				if (category() == Categories::MAP) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconMap()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Map().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconMap()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Map().c_str())) {
							// Do nothing.
						}
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconMap()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Map().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::MAP);
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;

				if (category() == Categories::SCENE) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconScene()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Scene().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconScene()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Scene().c_str())) {
							// Do nothing.
						}
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconScene()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Scene().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::SCENE);
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;

				if (category() == Categories::ACTOR) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconActor()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Actor().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconActor()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Actor().c_str())) {
							// Do nothing.
						}
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconActor()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Actor().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::ACTOR);
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;

				if (category() == Categories::FONT) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconFont()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Font().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconFont()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Font().c_str())) {
							// Do nothing.
						}
					}
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconFont()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Font().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(Categories::FONT);
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;

				if (category() == Categories::MUSIC || category() == Categories::SFX) {
					if (docOpened) {
						if (ImGui::MenuBarImageButton(theme()->iconAudio()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Audio().c_str())) {
							toggleDocument(nullptr);
						}
					} else {
						WIDGETS_SELECTION_GUARD(theme());

						if (ImGui::MenuBarImageButton(theme()->iconAudioMore()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Audio().c_str())) {
							ImGui::OpenPopup("@Aud");

							bubble(nullptr);
						}
					}

					do {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
						VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

						if (ImGui::BeginPopup("@Aud")) {
							if (ImGui::MenuItem(theme()->menu_Music(), GBBASIC_MODIFIER_KEY_NAME "+7", category() == Categories::MUSIC)) {
								category(Categories::MUSIC);
							}
							if (ImGui::MenuItem(theme()->menu_Sfx(), GBBASIC_MODIFIER_KEY_NAME "+8", category() == Categories::SFX)) {
								category(Categories::SFX);
							}

							ImGui::EndPopup();
						}
					} while (false);
				} else {
					if (ImGui::MenuBarImageButton(theme()->iconAudio()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Audio().c_str()) && !busy) {
						if (docOpened)
							toggleDocument(nullptr);
						category(categoryOfAudio());
					}
				}
				ImGui::SameLine();
				width += ImGui::GetItemRectSize().x;
			}

			if (category() == Categories::CONSOLE) {
				if (docOpened) {
					if (ImGui::MenuBarImageButton(theme()->iconConsole()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Console().c_str())) {
						toggleDocument(nullptr);
					}
				} else {
					WIDGETS_SELECTION_GUARD(theme());

					if (ImGui::MenuBarImageButton(theme()->iconConsole()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipEdit_Console().c_str()) && !busy) {
						// Do nothing.
					}
				}
			} else {
				ImVec4 consoleCol(1, 1, 1, 1);
				do {
					LockGuard<decltype(consoleLock())> guard(consoleLock());

					if (consoleHasError())
						consoleCol = ImGui::ColorConvertU32ToFloat4(theme()->style()->errorColor);
					else if (consoleHasWarning())
						consoleCol = ImGui::ColorConvertU32ToFloat4(theme()->style()->warningColor);
				} while (false);

				if (ImGui::MenuBarImageButton(theme()->iconConsole()->pointer(rnd), ImVec2(13, 13), consoleCol, theme()->tooltipEdit_Console().c_str()) && !busy) {
					if (docOpened)
						toggleDocument(nullptr);
					category(Categories::CONSOLE);
				}
			}
			ImGui::SameLine();
			width += ImGui::GetItemRectSize().x;

			if (docOpened) {
				WIDGETS_SELECTION_GUARD(theme());

				if (ImGui::MenuBarImageButton(theme()->iconDocument()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipRecent_Document().c_str()) && !busy) {
					toggleDocument(nullptr);
				}
			} else {
				if (ImGui::MenuBarImageButton(theme()->iconDocument()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), theme()->tooltipRecent_Document().c_str()) && !busy) {
					toggleDocument(nullptr);
				}
			}
			ImGui::SameLine();
			width += ImGui::GetItemRectSize().x;
		}

		break;
	default: // Do nothing.
		break;
	}
#if WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED
	if (!wnd->bordered() && rnd->width() >= 440) {
		if (ImGui::MenuBarImageButton(theme()->iconMinimize()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
			if (wnd->maximized())
				wnd->restore();
			wnd->minimize();
		}
		ImGui::SameLine();
		width += ImGui::GetItemRectSize().x;
		if (wnd->fullscreen() || wnd->maximized()) {
			if (ImGui::MenuBarImageButton(theme()->iconRestore()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
				if (wnd->fullscreen())
					toggleFullscreen(wnd);
				else
					toggleMaximized(wnd);
			}
		} else {
			if (ImGui::MenuBarImageButton(theme()->iconMaximize()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
				toggleMaximized(wnd);
			}
		}
		ImGui::SameLine();
		width += ImGui::GetItemRectSize().x;
		ImVec4 colBtnHvr = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImVec4 colBtnAct = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		std::swap(colBtnHvr.x, colBtnHvr.y);
		std::swap(colBtnAct.x, colBtnAct.y);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colBtnHvr);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colBtnAct);
		if (ImGui::MenuBarImageButton(theme()->iconClose()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1))) {
			SDL_Event evt;
			evt.type = SDL_QUIT;
			SDL_PushEvent(&evt);
		}
		ImGui::PopStyleColor(2);
		ImGui::SameLine();
		width += ImGui::GetItemRectSize().x;
	}
#else /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */
	(void)wnd;
#endif /* WORKSPACE_HEAD_BAR_ADJUSTING_ENABLED */
	width += style.FramePadding.x;
	tabsWidth(width);
}

void Workspace::filter(Window*, Renderer* rnd) {
	if (!recentProjectsFilter().enabled)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(1, 1));

	const ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_FIELD_BAR;
	const float width = rnd->width() - recentProjectsFilterPosition().x;
	ImGui::SetNextWindowPos(
		ImVec2(
			recentProjectsFilterPosition().x,
			recentProjectsFilterPosition().y + menuHeight()
		),
		ImGuiCond_Always
	);
	ImGui::SetNextWindowSize(
		ImVec2(width, recentProjectsFilter().height),
		ImGuiCond_Always
	);
	ImGui::SetNextWindowSizeConstraints(ImVec2(width, recentProjectsFilter().height), ImVec2(width, recentProjectsFilter().height));
	if (ImGui::Begin("Fltr", nullptr, flags)) {
		if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
			closeFilter();
		}

		ImGui::PushID("@Flt");
		do {
			const float oldY = ImGui::GetCursorPosY();
			if (!recentProjectsFilter().initialized) {
				ImGui::SetKeyboardFocusHere();
				recentProjectsFilter().initialized = true;
			}
			ImGui::SetNextItemWidth(width - 1 * 2);
			char buf[256]; // Fixed size.
			const size_t n = Math::min(GBBASIC_COUNTOF(buf) - 1, recentProjectsFilter().pattern.length());
			if (n > 0)
				memcpy(buf, recentProjectsFilter().pattern.c_str(), n);
			buf[n] = '\0';
			const ImGuiInputTextFlags flags_ = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AllowTabInput;
			ImGui::InputText(
				"", buf, sizeof(buf),
				flags_,
				[] (ImGuiInputTextCallbackData* data) -> int {
					std::string* what = (std::string*)data->UserData;
					what->assign(data->Buf, data->BufTextLen);

					return 0;
				},
				&recentProjectsFilter().pattern
			);
			recentProjectsFilterFocused(ImGui::GetActiveID() == ImGui::GetID(""));
			const float newY = ImGui::GetCursorPosY();
			recentProjectsFilter().height = newY - oldY - 1 * 2;
		} while (false);
		ImGui::PopID();

		ImGui::End();
	}
	ImGuiWindow* window = ImGui::FindWindowByName("Filter");
	ImGui::BringWindowToDisplayFront(window);
}

void Workspace::body(Window* wnd, Renderer* rnd, double delta, unsigned fps, bool* indicated) {
	const float menuH = menuHeight();
	const float searchResultH = searchResultVisible() ? searchResultHeight() : 0.0f;

	if (_toUpgrade || _toCompile) {
		console(wnd, rnd, menuH, searchResultH, delta);

		return;
	}

	if (document()) {
		document(wnd, rnd, menuH, 0.0f);

		return;
	}

	switch (category()) {
	case Categories::HOME:
		if (showRecentProjects())
			recent(wnd, rnd, menuH, searchResultH);
		else
			notepad(wnd, rnd, menuH, searchResultH);

		break;
	case Categories::FONT:
		font(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::CODE:
		code(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::TILES:
		tiles(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::MAP:
		map(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::MUSIC:
		music(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::SFX:
		sfx(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::ACTOR:
		actor(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::SCENE:
		scene(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::CONSOLE:
		console(wnd, rnd, menuH, searchResultH, delta);

		break;
	case Categories::EMULATOR:
		emulator(wnd, rnd, menuH, 0.0f, fps, indicated);

		break;
	default: // Do nothing.
		break;
	}
}

void Workspace::recent(Window* wnd, Renderer* rnd, float marginTop, float marginBottom) {
	// Prepare.
	ImGuiStyle &style = ImGui::GetStyle();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(0, 0, 0, 0));

	ImGui::SetNextWindowPos(ImVec2(0.0f, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	ImGui::Begin("Recent", nullptr, WORKSPACE_WND_FLAGS_RECENT);

	// Show recent contents.
	do {
		// Prepare.
		const bool iconView = settings().recentIconView;
		const bool columned = iconView && recentProjectsColumnCount() > 0;
		if (columned)
			ImGui::Columns(recentProjectsColumnCount(), nullptr, false);
		else
			ImGui::Columns(1, nullptr, false);
		const int scale = (int)theme()->style()->projectIconScale;
		float selectableWidth = 0.0f;

		// Iterate the projects.
		if (iconView) {
			recentProjectsColumnCount(0);
		}
		int filteredIdx = 0;
		projectIndices().clear();
		for (int idx = 0; idx < (int)projects().size(); ++idx) {
			// Prepare.
			Project::Ptr &prj = projects()[idx];
			const float COL_HALF_SPACE = style.ItemSpacing.x * 0.5f;
			const std::string &title = prj->title();
			bool selected = idx == recentProjectSelectedIndex();
			float oldX = 0.0f;
			const float oldY = ImGui::GetCursorPosY();
			float diffX = 0.0f;

			// Filter by pattern.
			if (recentProjectsFilter().enabled && !recentProjectsFilter().pattern.empty()) {
				std::string pattern = recentProjectsFilter().pattern;
				if (pattern.front() != '*')
					pattern.insert(pattern.begin(), '*');
				if (pattern.back() != '*')
					pattern.push_back('*');
				if (!Text::matchWildcard(prj->title(), pattern.c_str(), true)) {
					if (selected)
						recentProjectSelectedIndex(-1);

					continue;
				}
			}

			projectIndices().push_back(idx);

			// Update the selection.
			if (recentProjectFocusingIndex() != -1) {
				if (filteredIdx == recentProjectFocusingIndex()) {
					recentProjectFocusingIndex(-1);
					recentProjectSelectedIndex(idx);
					selected = true;
				}
			}

			bool focusing = false;
			if (recentProjectSelectingIndex() != -1) {
				if (idx == recentProjectSelectingIndex()) {
					recentProjectSelectingIndex(-1);
					recentProjectSelectedIndex(idx);
					focusing = true;
				}
			}

			if (selected)
				recentProjectFocusedIndex(filteredIdx);

			++filteredIdx;

			// Render the selectable.
			const ImVec2 pos = ImGui::GetCursorPos();
			ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0, 0));
			const std::string ntitle = title + Text::toString(idx);
			if (ImGui::Selectable(ntitle.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(0, recentProjectItemHeight()))) {
				const double now = DateTime::toSeconds(DateTime::ticks());
				if (selected) {
					if (now - recentProjectSelectedTimestamp() < 0.5) { // Double clicked.
						ImGuiIO &io = ImGui::GetIO();

#if GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CTRL
						const bool modifier = io.KeyCtrl;
#elif GBBASIC_MODIFIER_KEY == GBBASIC_MODIFIER_KEY_CMD
						const bool modifier = io.KeySuper;
#endif /* GBBASIC_MODIFIER_KEY */

						closeFilter();

						if (!modifier && !io.KeyShift && io.KeyAlt) {
							showProjectProperty(wnd, rnd, prj.get(), false);
						} else {
							Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[wnd, rnd, this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());

										if (prj->runOnOpen() || prj->contentType() != Project::ContentTypes::BASIC)
											launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
									}
								);
						}
					} else { // The interval between two clicks is too short, perform single click.
						recentProjectSelectedTimestamp(now);
						recentProjectSelectedIndex(idx);
					}
				} else { // Single clicked.
					recentProjectSelectedTimestamp(now);
					recentProjectSelectedIndex(idx);

					ImGui::ScrollToItem(ImGuiScrollFlags_KeepVisibleEdgeY);
				}
			} else {
				if (focusing)
					ImGui::ScrollToItem(ImGuiScrollFlags_KeepVisibleEdgeY);
			}
			if (iconView && ImGui::IsItemHovered() && title != prj->shortTitle()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::PushStyleColor(ImGuiCol_Text, theme()->style()->builtin[ImGuiCol_Text]);

				ImGui::SetTooltip(prj->title());

				ImGui::PopStyleColor();
			}
			bool polluted = false;
			selectableWidth = ImGui::GetItemRectSize().x;
			if (recentProjectWithContextIndex() < 0 || recentProjectWithContextIndex() == idx) {
				ImGui::PushStyleColor(ImGuiCol_Text, theme()->style()->builtin[ImGuiCol_Text]);

				if (ImGui::BeginPopupContextItem("Recent context")) {
					recentProjectWithContextIndex(idx);
					recentProjectSelectedIndex(idx);

					const bool sel = recentProjectSelectedIndex() >= 0 && recentProjectSelectedIndex() < (int)projects().size();

					Project::Ptr &prj = projects()[recentProjectSelectedIndex()];
					const bool isEditable = prj->contentType() == Project::ContentTypes::BASIC;

					if (ImGui::MenuItem(theme()->menu_Run(), nullptr, nullptr, sel)) {
						closeFilter();

						Operations::fileOpen(wnd, rnd, this, prj, true, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[wnd, rnd, this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());

									launchProject(wnd, rnd, nullptr, nullptr, nullptr, true, -1);
								}
							);
					}
					if (isEditable && ImGui::MenuItem(theme()->menu_Open(), nullptr, nullptr, sel)) {
						closeFilter();

						Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());
								}
							);
					}
					if (isEditable && ImGui::MenuItem(theme()->menu_Rename(), nullptr, nullptr, sel)) {
						Operations::fileRename(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());

						polluted = true;
					}
					if (ImGui::MenuItem(theme()->menu_Remove(), nullptr, nullptr, sel)) {
						Operations::fileRemove(wnd, rnd, this, prj);

						polluted = true;
					}
					if (!polluted && ImGui::MenuItem(theme()->menu_RemoveSramState(), nullptr, nullptr, prj->sramExists(this))) {
						Operations::projectRemoveSram(wnd, rnd, this, prj, false);
					}
					if (!polluted && isEditable && ImGui::MenuItem(theme()->menu_Duplicate(), nullptr, nullptr, prj->exists())) {
						Operations::fileDuplicate(wnd, rnd, this, prj, fontConfig().empty() ? nullptr : fontConfig().c_str());

						polluted = true;
					}
					ImGui::Separator();
#if !defined GBBASIC_OS_HTML
					if (!polluted && ImGui::MenuItem(theme()->menu_Browse())) {
						Operations::fileBrowse(wnd, rnd, this, prj);
					}
#endif /* GBBASIC_OS_HTML */
					if (ImGui::MenuItem(theme()->menu_Property())) {
						showProjectProperty(wnd, rnd, prj.get(), false);
					}

					ImGui::EndPopup();
				} else {
					if (recentProjectWithContextIndex() >= 0) {
						recentProjectWithContextIndex(-1);
					}
				}

				ImGui::PopStyleColor();
			}
			ImGui::PopStyleColor();
			ImGui::SetCursorPos(pos);
			if (polluted)
				break;

			// Render the sticker.
			auto getBuiltinIcon = [rnd, this] (Project::Ptr &prj) -> void* {
				void* iconTex = nullptr;
				if (prj->contentType() == Project::ContentTypes::BASIC) {
					if (prj->isExample())
						iconTex = theme()->iconExample()->pointer(rnd);
					else if (prj->isPlain())
						iconTex = theme()->iconPlainCode()->pointer(rnd);
					else
						iconTex = theme()->iconProjectOmitted()->pointer(rnd);
				} else {
					iconTex = theme()->iconRom()->pointer(rnd);
				}

				return iconTex;
			};

			if (columned) {
				const float iconWidth = recentProjectItemIconWidth() == 0.0f ?
					72.0f : recentProjectItemIconWidth();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - iconWidth) * 0.5f - COL_HALF_SPACE);
			}
			oldX = ImGui::GetCursorPosX();
			if (iconView) {
				Texture::Ptr &icon = prj->touchIconTexture();
				if (icon) {
					ImGui::Image(
						icon->pointer(rnd),
						ImVec2((float)(ASSETS_ROM_ICON_SIZE * scale), (float)(ASSETS_ROM_ICON_SIZE * scale))
					);
				} else {
					void* iconTex = getBuiltinIcon(prj);
					ImGui::Image(
						iconTex,
						ImVec2((float)(ASSETS_ROM_ICON_SIZE * scale), (float)(ASSETS_ROM_ICON_SIZE * scale))
					);
				}
			} else {
				Texture::Ptr &icon = prj->touchIconTexture();
				if (icon) {
					ImGui::Image(
						icon->pointer(rnd),
						ImVec2((float)(ASSETS_ROM_ICON_SIZE), (float)(ASSETS_ROM_ICON_SIZE))
					);
				} else {
					void* iconTex = getBuiltinIcon(prj);
					ImGui::Image(
						iconTex,
						ImVec2((float)(ASSETS_ROM_ICON_SIZE), (float)(ASSETS_ROM_ICON_SIZE))
					);
				}
			}

			// Separate the columns.
			if (!iconView) {
				if (recentProjectItemHeight() == 0.0f) {
					recentProjectItemHeight((ImGui::GetCursorPosY() - oldY - 4));
				}
			}
			ImGui::SameLine();
			diffX = ImGui::GetCursorPosX() - oldX;
			if (recentProjectItemIconWidth() == 0.0f) {
				recentProjectItemIconWidth(diffX);
			}
			if (iconView) {
				if (recentProjectsColumnCount() == 0) {
					recentProjectsColumnCount(
						Math::max((int)(rnd->width() / (diffX * 2.0f)), 2)
					);
				}
			}
			if (iconView) {
				ImGui::NewLine();
			} else {
				ImGui::SameLine();
			}

			// Render the title.
			if (columned) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - prj->inLibTextWidth()) * 0.5f - COL_HALF_SPACE);
			}
			const ImVec2 txtPos = ImGui::GetCursorPos();
			oldX = txtPos.x;
			if (iconView)
				ImGui::TextUnformatted(prj->shortTitle());
			else
				ImGui::TextUnformatted(prj->title());
			ImGui::SameLine();
			diffX = ImGui::GetCursorPosX() - oldX;
			prj->inLibTextWidth(diffX);
			if (iconView) {
				ImGui::NewLine();
				if (recentProjectItemHeight() == 0.0f) {
					recentProjectItemHeight((ImGui::GetCursorPosY() - oldY));
				}
			}
			ImGui::NewLine();

			// Render the time.
			if (!iconView) {
				const ImVec2 pos_ = ImGui::GetCursorPos();
				ImGui::SetCursorPos(
					ImVec2(
						oldX,
						txtPos.y + ImGui::GetTextLineHeightWithSpacing() + 1
					)
				);
				switch ((Orders)settings().recentProjectsOrder) {
				case Orders::DEFAULT: // Fall through.
				case Orders::TITLE: // Fall through.
				case Orders::LATEST_MODIFIED: // Fall through.
				case Orders::EARLIEST_MODIFIED:
					ImGui::Text(theme()->windowRecent_Modified().c_str(), prj->modifiedString().c_str());
					ImGui::SetCursorPos(pos_);

					break;
				case Orders::LATEST_CREATED: // Fall through.
				case Orders::EARLIEST_CREATED:
					ImGui::Text(theme()->windowRecent_Created().c_str(), prj->createdString().c_str());
					ImGui::SetCursorPos(pos_);

					break;
				default:
					GBBASIC_ASSERT(false && "Impossible.");

					break;
				}
			}

			// Finish.
			if (columned) {
				ImGui::NextColumn();
			}
		}
		recentProjectFocusableCount(filteredIdx);

		// The "+" button.
		bool clicked = false;
		if (iconView) {
			const float COL_HALF_SPACE = style.ItemSpacing.x;
			if (columned) {
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ASSETS_ROM_ICON_SIZE * scale) * 0.5f - COL_HALF_SPACE);
			}
			clicked = ImGui::Button("+", ImVec2((float)(ASSETS_ROM_ICON_SIZE * scale), (float)(ASSETS_ROM_ICON_SIZE * scale)));
		} else {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 4);
			if (projects().empty()) {
				const ImVec2 avail = ImGui::GetContentRegionAvail();
				clicked = ImGui::Button("+", ImVec2(avail.x, (float)(ASSETS_ROM_ICON_SIZE)));
			} else {
				clicked = ImGui::Button("+", ImVec2(selectableWidth, (float)(ASSETS_ROM_ICON_SIZE)));
			}
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme()->tooltipFile_New());
		}
		if (clicked) {
			closeFilter();

			stopProject(wnd, rnd);

			Operations::fileNew(wnd, rnd, this, fontConfig().empty() ? nullptr : fontConfig().c_str())
				.then(
					[wnd, rnd, this] (Project::Ptr prj) -> void {
						ImGuiIO &io = ImGui::GetIO();

						if (io.KeyCtrl)
							return;

						Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
							.then(
								[this, prj] (bool ok) -> void {
									if (!ok)
										return;

									validateProject(prj.get());
								}
							);
					}
				);

			projectIndices().clear(); projectIndices().shrink_to_fit();

			recentProjectSelectedIndex(-1);
		}

		if (columned)
			ImGui::NextColumn();

		// Finish.
		ImGui::Columns(1);
	} while (false);

	// Finish.
	ImGui::End();

	ImGui::PopStyleColor(1);
}

void Workspace::notepad(Window* wnd, Renderer* rnd, float marginTop, float marginBottom) {
	// Prepare.
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	// Render the window.
	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	if (ImGui::Begin("Notepad", nullptr, WORKSPACE_WND_FLAGS_CONTENT | ImGuiWindowFlags_NoScrollWithMouse)) {
		// Prepare.
		const float width = (float)rnd->width();
		const float height = (float)rnd->height() - (marginTop + marginBottom);
		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		const float windowHeight = height - statusBarHeight;

		// Render the child window.
		ImGui::PushStyleColor(ImGuiCol_ChildBg, theme()->style()->screenClearingColor);
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav;
		ImGui::BeginChild("@Notepad", ImVec2(width, windowHeight), false, flags);
		{
			const float btnWidth = 100.0f;
			const float btnHeight = 72.0f;
			const ImVec2 btnPos((width - btnWidth) * 0.5f, (windowHeight - btnHeight) * 0.5f);

			ImGui::SetCursorPos(btnPos);
			if (ImGui::Button(theme()->generic_Open(), ImVec2(btnWidth, btnHeight))) {
				Operations::fileImportForNotepad(wnd, rnd, this)
					.then(
						[wnd, rnd, this] (Project::Ptr prj) -> void {
							if (!prj)
								return;

							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl)
								return;

							Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());
									}
								);
						}
					);
			}

			ImGui::NewLine(3);
			ImGui::SetCursorPosX(btnPos.x);
			if (ImGui::Button(theme()->generic_New(), ImVec2(btnWidth, 0))) {
				Operations::fileNew(wnd, rnd, this, fontConfig().empty() ? nullptr : fontConfig().c_str())
					.then(
						[wnd, rnd, this] (Project::Ptr prj) -> void {
							ImGuiIO &io = ImGui::GetIO();

							if (io.KeyCtrl)
								return;

							Operations::fileOpen(wnd, rnd, this, prj, false, fontConfig().empty() ? nullptr : fontConfig().c_str())
								.then(
									[this, prj] (bool ok) -> void {
										if (!ok)
											return;

										validateProject(prj.get());
									}
								);
						}
					);
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		// Render the status bar.
		const bool actived = ImGui::IsWindowFocused();
		if (actived || EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			const ImVec2 pos = ImGui::GetCursorPos();
			ImGui::Dummy(
				ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize),
				ImGui::GetStyleColorVec4(ImGuiCol_Button)
			);
			ImGui::SetCursorPos(pos);
		}

		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
			col.z = 1.0f;
			ImGui::PushStyleColor(ImGuiCol_Button, col);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
		}
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(theme()->status_OpenOrCreateAProjectToContinue());
		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImGui::PopStyleColor(3);
		}

		ImGui::End();
	}
}

void Workspace::font(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	FontAssets::Entry* entry = prj->getFont(prj->activeFontIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Fnt", nullptr, flags)) {
		if (entry) {
			touchFontEditor(wnd, rnd, prj.get(), prj->activeFontIndex(), entry)
				->update(
					wnd, rnd,
					this,
					prj->title().c_str(),
					0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
					delta
				);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::FONT);
		}

		ImGui::End();
	}
}

void Workspace::code(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	// Prepare.
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	ImGuiIO &io = ImGui::GetIO();
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	// VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	bool renderedStatusBar = false;
	EditorCode* focusMajor = nullptr;

	const bool dual = prj->isMinorCodeEditorEnabled();
	float minWidth = Math::min(rnd->width() * 0.2f, 256.0f * io.FontGlobalScale);
	const float maxWidth = rnd->width() * 0.8f;
	const float defaultMajorWidth = rnd->width() * 0.5f;
	if (dual) {
		if (codeEditorMinorWidth() <= 0.0f) {
			codeEditorMinorWidth(defaultMajorWidth);
			prj->minorCodeEditorWidth(defaultMajorWidth);
		}
	} else {
		if (!!prj->minorCodeEditor()) {
			destroyMinorCodeEditor();
		}
	}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	// Major editor.
	const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
	do {
		// Prepare.
		ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		const float width = dual ? (float)rnd->width() - codeEditorMinorWidth() : (float)rnd->width();
		const float height = (float)rnd->height() - (marginTop + marginBottom) - statusBarHeight;
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		const float width = (float)rnd->width();
		const float height = (float)rnd->height() - (marginTop + marginBottom);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		// Set the window properties.
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (dual) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, -1), ImVec2(maxWidth, -1));
		}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
		ImGui::SetNextWindowSize(
			ImVec2(width, height),
			ImGuiCond_Always
		);

		// Show and update the editor window.
		CodeAssets::Entry* entry = prj->getCode(prj->activeMajorCodeIndex());
		if (!entry)
			flags |= ImGuiWindowFlags_NoScrollWithMouse;
		EditorCode* editor = nullptr;
		if (ImGui::Begin("#Cd", nullptr, flags)) {
			// Get the entry and editor.
			if (!entry) {
				prj->addCodePage("");
				prj->hasDirtyAsset(true);
				prj->activeMajorCodeIndex(0);
				entry = prj->getCode(prj->activeMajorCodeIndex());
			}
			editor = (EditorCode*)entry->editor;

			// Open new editor if necessary.
			if (!editor) {
				editor = touchCodeEditor(wnd, rnd, prj.get(), prj->activeMajorCodeIndex(), true, entry);

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
				focusMajor = editor;

				syncMinorCodeEditorDataToMajor(wnd, rnd);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			}

			// Update the editor.
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
			if (editor->isToolBarVisible()) {
				minWidth = Math::max(minWidth, (float)EDITOR_CODE_WITH_TOOL_BAR_MIN_WIDTH);
			}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

			editor->update(
				wnd, rnd,
				this,
				prj->title().c_str(),
				0, marginTop, width, height,
				delta
			);

			// Finish.
			ImGui::End();
		}

		// Show and update the status bar.
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (!dual || prj->isMajorCodeEditorActive()) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, statusBarHeight));
			ImGui::SetNextWindowPos(ImVec2(0, marginTop + height), ImGuiCond_Always);
			ImGui::SetNextWindowSize(
				ImVec2((float)rnd->width(), statusBarHeight),
				ImGuiCond_Always
			);
			if (ImGui::Begin("#St", nullptr, WORKSPACE_WND_FLAGS_CONTENT | ImGuiWindowFlags_AlwaysAutoResize)) {
				editor->renderStatus(
					wnd, rnd,
					this,
					0, marginTop + height, (float)rnd->width(), statusBarHeight
				);

				ImGui::End();
			}
			renderedStatusBar = true;
		}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	} while (false);

	// Minor editor.
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	if (dual) {
		// Prepare.
		ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;
		const float x = (float)rnd->width() - codeEditorMinorWidth();
		const float width = codeEditorMinorWidth();
		const float height = (float)rnd->height() - (marginTop + marginBottom) - statusBarHeight;

		// Resize.
		const float gripMarginX = ImGui::WindowResizingPadding().x;
		const float gripPaddingY = 4.0f;
		if (codeEditorMinorResizing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			codeEditorMinorResizing(false);

			const Math::Vec2i rndSize(rnd->width(), rnd->height());

			prj->foreach(
				[rnd, rndSize] (AssetsBundle::Categories, int, BaseAssets::Entry*, Editable* editor) -> void {
					if (!editor)
						return;

					editor->resized(rnd, rndSize, rndSize);
				}
			);
		}
		if (codeEditorMinorResetting() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			codeEditorMinorResetting(false);
		}
		const bool isHoveringRect = ImGui::IsMouseHoveringRect(
			ImVec2(x, marginTop + gripPaddingY),
			ImVec2(x + gripMarginX, marginTop + height - gripPaddingY - style.ScrollbarSize),
			false
		);
		if (isHoveringRect && !popupBox()) {
			codeEditorMinorResizing(ImGui::IsMouseDown(ImGuiMouseButton_Left));

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				codeEditorMinorResetting(true);

			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			codeEditorMinorResizing(false);
		}
		if (codeEditorMinorResizing() && !codeEditorMinorResetting()) {
			flags &= ~ImGuiWindowFlags_NoResize;
		}

		// Set the window properties.
		ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, -1), ImVec2(maxWidth, -1));
		ImGui::SetNextWindowPos(ImVec2(x, marginTop), ImGuiCond_Always);
		ImGui::SetNextWindowSize(
			ImVec2(width, height),
			ImGuiCond_Always
		);

		// Show and update the editor window.
		if (ImGui::Begin("#Cdm", nullptr, flags)) {
			// Open new editor if necessary.
			const int minorIdx = Math::clamp(prj->activeMinorCodeIndex(), 0, prj->codePageCount() - 1);
			if (minorIdx != prj->activeMinorCodeIndex())
				prj->activeMinorCodeIndex(minorIdx);

			bool newOpened = false;
			if (!prj->minorCodeEditor()) {
				createMinorCodeEditor();
				prj->minorCodeEditor()->open(wnd, rnd, this, prj.get(), prj->activeMinorCodeIndex(), (unsigned)(~0), -1);
				if (!focusMajor)
					prj->minorCodeEditor()->enter(this);

				newOpened = true;
			}
			if (prj->minorCodeEditor()->index() != prj->activeMinorCodeIndex()) {
				prj->minorCodeEditor()->leave(this);
				prj->minorCodeEditor()->close(prj->minorCodeEditor()->index());
				prj->minorCodeEditor()->open(wnd, rnd, this, prj.get(), prj->activeMinorCodeIndex(), (unsigned)(~0), -1);
				if (!focusMajor)
					prj->minorCodeEditor()->enter(this);

				newOpened = true;
			}

			if (newOpened) {
				syncMajorCodeEditorDataToMinor(wnd, rnd);
			}

			// Update the editor.
			if (codeEditorMinorResetting()) {
				codeEditorMinorWidth(defaultMajorWidth);
				prj->minorCodeEditorWidth(defaultMajorWidth);
			} else {
				codeEditorMinorWidth(ImGui::GetWindowSize().x);
				prj->minorCodeEditorWidth(ImGui::GetWindowSize().x);
			}

			prj->minorCodeEditor()->update(
				wnd, rnd,
				this,
				prj->title().c_str(),
				x, marginTop, width, height,
				delta
			);

			// Finish.
			ImGui::End();
		}

		// Show the status bar.
		if (!renderedStatusBar && !prj->isMajorCodeEditorActive()) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, statusBarHeight));
			ImGui::SetNextWindowPos(ImVec2(0, marginTop + height), ImGuiCond_Always);
			ImGui::SetNextWindowSize(
				ImVec2((float)rnd->width(), statusBarHeight),
				ImGuiCond_Always
			);
			if (ImGui::Begin("#St", nullptr, WORKSPACE_WND_FLAGS_CONTENT | ImGuiWindowFlags_AlwaysAutoResize)) {
				prj->minorCodeEditor()->renderStatus(
					wnd, rnd,
					this,
					0, marginTop + height, (float)rnd->width(), statusBarHeight
				);

				ImGui::End();
			}
		}
	}

	// Process the focus status.
	if (focusMajor) {
		focusMajor->enter(this);
	}
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
}

void Workspace::tiles(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	TilesAssets::Entry* entry = prj->getTiles(prj->activeTilesIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Tl", nullptr, flags)) {
		if (entry) {
			touchTilesEditor(wnd, rnd, prj.get(), prj->activeTilesIndex(), entry)
				->update(
					wnd, rnd,
					this,
					prj->title().c_str(),
					0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
					delta
				);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::TILES);
		}

		ImGui::End();
	}
}

void Workspace::map(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	MapAssets::Entry* entry = prj->getMap(prj->activeMapIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Mp", nullptr, flags)) {
		if (entry) {
			if (!entry->editor) {
				BaseAssets::Entry* refAsset = prj->tilesPageCount() == 0 ? nullptr : prj->getTiles(entry->ref);
				if (refAsset)
					touchMapEditor(wnd, rnd, prj.get(), prj->activeMapIndex(), (unsigned)Categories::TILES, entry->ref, entry);
				else
					touchMapEditor(wnd, rnd, prj.get(), prj->activeMapIndex(), (unsigned)(~0), -1, entry);
			}
			entry->editor->update(
				wnd, rnd,
				this,
				prj->title().c_str(),
				0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
				delta
			);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::MAP);
		}

		ImGui::End();
	}
}

void Workspace::music(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	MusicAssets::Entry* entry = prj->getMusic(prj->activeMusicIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Ad", nullptr, flags)) {
		if (entry) {
			touchMusicEditor(wnd, rnd, prj.get(), prj->activeMusicIndex(), entry)
				->update(
					wnd, rnd,
					this,
					prj->title().c_str(),
					0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
					delta
				);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::MUSIC);
		}

		ImGui::End();
	}
}

void Workspace::sfx(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	if (prj->sharedSfxEditorDisabled())
		return;

	prj->createSharedSfxEditor();

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	SfxAssets::Entry* entry = prj->getSfx(prj->activeSfxIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Ad", nullptr, flags)) {
		if (entry) {
			touchSfxEditor(wnd, rnd, prj.get(), prj->activeSfxIndex(), entry)
				->update(
					wnd, rnd,
					this,
					prj->title().c_str(),
					0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
					delta
				);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::SFX);
			EditorSfx* editor = prj->sharedSfxEditor();
			editor->update(
				wnd, rnd,
				this,
				prj->title().c_str(),
				0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
				delta
			);
		}

		ImGui::End();
	}
}

void Workspace::actor(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	ActorAssets::Entry* entry = prj->getActor(prj->activeActorIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Act", nullptr, flags)) {
		if (entry) {
			touchActorEditor(wnd, rnd, prj.get(), prj->activeActorIndex(), entry)
				->update(
					wnd, rnd,
					this,
					prj->title().c_str(),
					0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
					delta
				);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::ACTOR);
		}

		ImGui::End();
	}
}

void Workspace::scene(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	SceneAssets::Entry* entry = prj->getScene(prj->activeSceneIndex());
	if (!entry)
		flags |= ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin("#Sc", nullptr, flags)) {
		if (entry) {
			if (!entry->editor) {
				BaseAssets::Entry* refAsset = prj->mapPageCount() == 0 ? nullptr : prj->getMap(entry->refMap);
				if (refAsset)
					touchSceneEditor(wnd, rnd, prj.get(), prj->activeSceneIndex(), (unsigned)Categories::MAP, entry->refMap, entry);
				else
					touchSceneEditor(wnd, rnd, prj.get(), prj->activeSceneIndex(), (unsigned)(~0), -1, entry);
			}
			entry->editor->update(
				wnd, rnd,
				this,
				prj->title().c_str(),
				0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
				delta
			);
		} else {
			blank(wnd, rnd, marginTop, marginBottom, Categories::SCENE);
		}

		ImGui::End();
	}
}

void Workspace::console(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, double delta) {
	if (!consoleVisible())
		return;

	EditorConsole* editor = (EditorConsole*)consoleTextBox();
	editor->update(
		wnd, rnd,
		this,
		"#Con",
		0, marginTop, (float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom),
		delta
	);
}

void Workspace::document(Window* wnd, Renderer* rnd, float marginTop, float marginBottom) {
	if (!document())
		return;

	if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
		Document::destroy(document());
		document(nullptr);

		return;
	}

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

	const ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	if (ImGui::Begin("#Doc", nullptr, flags)) {
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);

		const ImVec2 size = ImGui::GetContentRegionAvail();
		ImGui::BeginChild("@Doc", size, false, ImGuiWindowFlags_NoNav);
		{
			document()->update(wnd, rnd, theme(), false);
		}
		ImGui::EndChild();

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void Workspace::emulator(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, unsigned fps, bool* indicated) {
	// Prepare.
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	Project::Ptr &prj = currentProject();

	ButtonEventHandler onCartridgeButtonClicked = nullptr;
	if (prj && prj->contentType() == Project::ContentTypes::BASIC) {
		onCartridgeButtonClicked = [&] (void) -> void {
			showProjectProperty(wnd, rnd, prj.get(), true);
		};
	}

	// Show the window.
	ImGui::SetNextWindowPos(ImVec2(0, marginTop), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), (float)rnd->height() - (marginTop + marginBottom)),
		ImGuiCond_Always
	);
	const ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT | ImGuiWindowFlags_NoScrollWithMouse;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
	if (ImGui::Begin("#Emu", nullptr, flags)) {
		// Prepare.
		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;

		// Render emulation.
		::emulator(
			wnd, rnd,
			theme(),
			input(),
			canvasDevice(), canvasTexture(), canvasTextureForBorderFrame(),
			canvasCartridgeStatusText(), canvasDeviceStatusText(), canvasStatusTooltip(), statusWidth(), statusBarHeight, true,
			settings().emulatorMuted, settings().emulatorSpeed, settings().emulatorPreferedSpeed,
			settings().canvasIntegerScale, settings().canvasFixRatio,
			settings().inputOnscreenGamepadEnabled, settings().inputOnscreenGamepadSwapAB, settings().inputOnscreenGamepadScale, settings().inputOnscreenGamepadPadding,
			settings().debugOnscreenDebugEnabled,
			canvasCursorMode(),
			!!popupBox(),
			fps,
			[&] (void) -> void {
				showPreferences(wnd, rnd, "device");
			},
			onCartridgeButtonClicked,
			[&] (void) -> void {
				debug(wnd, rnd, marginTop, marginBottom);
			}
		);

		// Update the indicated state.
		if (indicated) {
			if (ImGui::IsWindowHovered())
				*indicated = true;
		}

		// Finish.
		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void Workspace::debug(Window*, Renderer* rnd, float marginTop, float marginBottom) {
	LockGuard<decltype(debuggerLock())> guard(debuggerLock());

	if (debuggerMessages().empty())
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.PopupBorderSize)> guardPopupBorderSize(&style.PopupBorderSize, style.PopupBorderSize, 0);

	ImGui::SetNextWindowPos(ImVec2(4.0f, marginTop + 4.0f));
	const ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_CONTENT | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_Tooltip;
	ImGui::BeginChild("Dbg", ImVec2((float)rnd->width(), (float)rnd->height() - 4.0f * 2 - (marginTop + marginBottom)), false, flags);
	{
		const ImVec2 pos = ImGui::GetCursorPos();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.11f, 0.11f, 0.11f, 1.00f));
		ImGui::TextUnformatted(debuggerMessages().text);
		ImGui::PopStyleColor();
		ImGui::SetCursorPos(pos - ImVec2(0, 1));
		ImGui::PushStyleColor(ImGuiCol_Text, theme()->style()->debugColor);
		ImGui::TextUnformatted(debuggerMessages().text);
		ImGui::PopStyleColor();
	}
	ImGui::EndChild();
}

void Workspace::blank(Window* wnd, Renderer* rnd, float marginTop, float marginBottom, Categories category_) {
	// Prepare.
	ImGuiStyle &style = ImGui::GetStyle();

	const float width = (float)rnd->width();
	const float height = (float)rnd->height() - (marginTop + marginBottom);
	const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
	const float windowHeight = height - statusBarHeight;

	// Render the child window.
	ImGui::PushStyleColor(ImGuiCol_ChildBg, theme()->style()->screenClearingColor);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav;
	ImGui::BeginChild("@Blank", ImVec2(width, windowHeight), false, flags);
	{
		const float btnWidth = 100.0f;
		const float btnHeight = 72.0f;
		const ImVec2 btnPos((width - btnWidth) * 0.5f, (windowHeight - btnHeight) * 0.5f);

		ImGui::SetCursorPos(btnPos);
		if (ImGui::Button(theme()->generic_New(), ImVec2(btnWidth, btnHeight))) {
			switch (category_) {
			case Categories::FONT:
				Operations::fontAddPage(wnd, rnd, this);

				break;
			case Categories::TILES:
				Operations::tilesAddPage(wnd, rnd, this);

				break;
			case Categories::MAP:
				Operations::mapAddPage(wnd, rnd, this);

				break;
			case Categories::MUSIC:
				Operations::musicAddPage(wnd, rnd, this);

				break;
			case Categories::SFX:
				Operations::sfxAddPage(wnd, rnd, this);

				break;
			case Categories::ACTOR:
				Operations::actorAddPage(wnd, rnd, this);

				break;
			case Categories::SCENE:
				Operations::sceneAddPage(wnd, rnd, this);

				break;
			default:
				GBBASIC_ASSERT(false && "Impossible.");

				break;
			}
		}

		switch (category_) {
		case Categories::MUSIC:
			ImGui::NewLine(3);
			ImGui::SetCursorPosX(btnPos.x);
			if (ImGui::Button(theme()->generic_Import(), ImVec2(btnWidth, 0))) {
				ImGui::OpenPopup("@Imp");

				bubble(nullptr);
			}

			importMusic(wnd, rnd);

			break;
		case Categories::SFX:
			ImGui::NewLine(3);
			ImGui::SetCursorPosX(btnPos.x);
			if (ImGui::Button(theme()->generic_Import(), ImVec2(btnWidth, 0))) {
				importSfx(wnd, rnd);
			}

			break;
		default:
			// Do nothing.

			break;
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	// Render the status bar.
	const bool actived = ImGui::IsWindowFocused();
	if (actived || EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
		const ImVec2 pos = ImGui::GetCursorPos();
		ImGui::Dummy(
			ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize),
			ImGui::GetStyleColorVec4(ImGuiCol_Button)
		);
		ImGui::SetCursorPos(pos);
	}

	if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	}
	ImGui::Dummy(ImVec2(8, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(theme()->status_EmptyCreateANewPageToContinue());
	switch (category_) {
	case Categories::MUSIC: {
			ImGui::SameLine();
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - statusWidth());
			do {
				WIDGETS_SELECTION_GUARD(theme());

				if (ImGui::ImageButton(theme()->iconMusic()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme()->tooltipAudio_Music().c_str())) {
					// Do nothing.
				}
			} while (false);
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			if (ImGui::ImageButton(theme()->iconSfx()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme()->tooltipAudio_Sfx().c_str())) {
				category(Categories::SFX);
			}
			width_ += ImGui::GetItemRectSize().x;
			width_ += style.FramePadding.x;
			statusWidth(width_);
		}

		break;
	case Categories::SFX: {
			ImGui::SameLine();
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - statusWidth());
			if (ImGui::ImageButton(theme()->iconMusic()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme()->tooltipAudio_Music().c_str())) {
				category(Categories::MUSIC);
			}
			width_ += ImGui::GetItemRectSize().x;
			ImGui::SameLine();
			do {
				WIDGETS_SELECTION_GUARD(theme());

				if (ImGui::ImageButton(theme()->iconSfx()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, theme()->tooltipAudio_Sfx().c_str())) {
					// Do nothing.
				}
			} while (false);
			width_ += ImGui::GetItemRectSize().x;
			width_ += style.FramePadding.x;
			statusWidth(width_);
		}

		break;
	default: // Do nothing.
		break;
	}
	if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
		ImGui::PopStyleColor(3);
	}
}

void Workspace::found(Window* wnd, Renderer* rnd, double delta) {
	if (!searchResultVisible())
		return;

	if (document())
		return;

	switch (category()) {
	case Categories::EMULATOR:
		return;
	default:
		// Do nothing.

		break;
	}

	if (!searchResultTextBox()) {
		EditorSearchResult* editor = EditorSearchResult::create();
		searchResultTextBox(editor);
		Project::Ptr &prj = currentProject();
		editor->open(wnd, rnd, this, prj.get(), -1, (unsigned)(~0), -1);
		editor->setLineClickedHandler(
			[&] (int line, bool doubleClicked) -> void {
				if (!doubleClicked)
					return;

				--line;
				if (line < 0 || line >= (int)searchResult().size())
					return;

				const Editing::Tools::SearchResult &sr = searchResult()[line];

				Project::Ptr &prj_ = currentProject();
				category(Categories::CODE);
				changePage(wnd, rnd, prj_.get(), (Categories)sr.begin.category, sr.begin.page);
				CodeAssets::Entry* entry = prj_->getCode(sr.begin.page);
				Editable* editor = entry->editor;
				if (!editor)
					return;

#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
				if (prj->isMajorCodeEditorActive()) {
					editor->post(Editable::SET_CURSOR, (Variant::Int)sr.begin.line);
					editor->post(Editable::SET_SELECTION, (Variant::Int)sr.begin.line, (Variant::Int)sr.begin.column, (Variant::Int)sr.end.line, (Variant::Int)sr.end.column);
				} else {
					GBBASIC_ASSERT(!!prj->minorCodeEditor() && "Impossible.");

					if (prj->activeMinorCodeIndex() != sr.begin.page)
						prj->activeMinorCodeIndex(sr.begin.page);

					if (prj->minorCodeEditor()->index() != prj->activeMinorCodeIndex()) {
						prj->minorCodeEditor()->leave(this);
						prj->minorCodeEditor()->close(prj->minorCodeEditor()->index());
						prj->minorCodeEditor()->open(wnd, rnd, this, prj.get(), prj->activeMinorCodeIndex(), (unsigned)(~0), -1);
						prj->minorCodeEditor()->enter(this);

						syncMajorCodeEditorDataToMinor(wnd, rnd);
					}

					prj->minorCodeEditor()->post(Editable::SET_CURSOR, (Variant::Int)sr.begin.line);
					prj->minorCodeEditor()->post(Editable::SET_SELECTION, (Variant::Int)sr.begin.line, (Variant::Int)sr.begin.column, (Variant::Int)sr.end.line, (Variant::Int)sr.end.column);
				}
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
				editor->post(Editable::SET_CURSOR, (Variant::Int)sr.begin.line);
				editor->post(Editable::SET_SELECTION, (Variant::Int)sr.begin.line, (Variant::Int)sr.begin.column, (Variant::Int)sr.end.line, (Variant::Int)sr.end.column);
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
			}
		);

		std::string tips;
		if (searchResult().empty()) {
			tips = Text::format(theme()->dialogPrompt_SearchForFound(), { searchResultPattern(), "0" });
		} else {
			tips = Text::format(
				theme()->dialogPrompt_SearchForFoundDoubleClickToJump(),
				{
					searchResultPattern(),
					Text::toString((UInt32)searchResult().size())
				}
			);
		}
		std::string txt = tips + "\n";
		for (const Editing::Tools::SearchResult &sr : searchResult()) {
			txt += Text::format("{0} - {1}", { sr.head, sr.body });
			txt += "\n";
		}
		editor->fromString(txt.c_str(), txt.length());
	}

	const float searchResultY = rnd->height() - searchResultHeight();
	searchResultTextBox()->update(
		wnd, rnd,
		this,
		theme()->windowSearchResult().c_str(),
		0, searchResultY, (float)rnd->width(), searchResultHeight(),
		delta
	);

	const bool toClose = !searchResultVisible();
	if (toClose)
		closeSearchResult();
}

void Workspace::importMusic(Window* wnd, Renderer* rnd) {
	ImGuiStyle &style = ImGui::GetStyle();

	auto import = [wnd, rnd, this] (const std::string &txt, const std::string &y) -> void {
		Operations::musicAddPage(wnd, rnd, this);

		bubble(nullptr);

		const Project::Ptr &prj = currentProject();
		MusicAssets::Entry* entry = prj->getMusic(prj->activeMusicIndex());
		Editable* editor = entry->editor;
		if (editor) {
			GBBASIC_ASSERT(false && "Impossible.");
		} else {
			touchMusicEditor(wnd, rnd, prj.get(), prj->activeMusicIndex(), entry)
				->post(Editable::IMPORT, txt, y);
		}
	};

	do {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(theme()->menu_C())) {
				do {
					if (!Platform::hasClipboardText()) {
						bubble(theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					import(txt, "C");
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(theme()->menu_CFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_C_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					std::string txt;
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						bubble(theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						bubble(theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					import(txt, "C");
				} while (false);
			}
			if (ImGui::MenuItem(theme()->menu_Json())) {
				do {
					if (!Platform::hasClipboardText()) {
						bubble(theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					import(txt, "JSON");
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(theme()->menu_JsonFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_JSON_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					std::string txt;
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						bubble(theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						bubble(theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					import(txt, "JSON");
				} while (false);
			}
			if (musicCount() > 0) {
#if WORKSPACE_MUSIC_MENU_ENABLED
				if (ImGui::BeginMenu(theme()->menu_Library())) {
					std::string path;
					if (ImGui::MusicMenu(music(), path)) {
						std::string txt;
						File::Ptr file(File::create());
						if (!file->open(path.c_str(), Stream::READ)) {
							bubble(theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}
						if (!file->readString(txt)) {
							file->close(); FileMonitor::unuse(path);

							bubble(theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}
						file->close(); FileMonitor::unuse(path);

						import(txt, "JSON");
					}

					ImGui::EndMenu();
				}
#else /* WORKSPACE_MUSIC_MENU_ENABLED */
				if (ImGui::MenuItem(theme()->menu_FromLibrary())) {
					const std::string musicDirectory = Path::absoluteOf(WORKSPACE_MUSIC_DIR);
					std::string defaultPath = musicDirectory;
					do {
						DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(musicDirectory.c_str());
						FileInfos::Ptr fileInfos = dirInfo->getFiles("*.json", false);
						if (fileInfos->count() == 0)
							break;

						FileInfo::Ptr fileInfo = fileInfos->get(0);
						defaultPath = fileInfo->fullPath();
#if defined GBBASIC_OS_WIN
						defaultPath = Text::replace(defaultPath, "/", "\\");
#endif /* GBBASIC_OS_WIN */
					} while (false);

					do {
						pfd::open_file open(
							GBBASIC_TITLE,
							defaultPath,
							GBBASIC_JSON_FILE_FILTER,
							pfd::opt::none
						);
						if (open.result().empty())
							break;

						std::string path = open.result().front();
						Path::uniform(path);
						if (path.empty())
							break;

						std::string txt;
						File::Ptr file(File::create());
						if (!file->open(path.c_str(), Stream::READ)) {
							bubble(theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}
						if (!file->readString(txt)) {
							file->close(); FileMonitor::unuse(path);

							bubble(theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}
						file->close(); FileMonitor::unuse(path);

						import(txt, "JSON");
					} while (false);
				}
#endif /* WORKSPACE_MUSIC_MENU_ENABLED */
			}

			ImGui::EndPopup();
		}
	} while (false);
}

void Workspace::importSfx(Window*, Renderer*) {
	const Project::Ptr &prj = currentProject();
	if (prj->sharedSfxEditor()) {
		EditorSfx* editor = prj->sharedSfxEditor();
		if (editor)
			editor->post(Editable::IMPORT);
	}
}

void Workspace::getCurrentProjectStates(
	bool* opened,
	bool* dirty,
	bool* exists,
	const char** url
) const {
	if (opened)
		*opened = false;
	if (dirty)
		*dirty = false;
	if (exists)
		*exists = false;
	if (url)
		*url = nullptr;

	const Project::Ptr &prj = currentProject();
	if (!prj)
		return;

	if (opened)
		*opened = !!prj;
	if (dirty)
		*dirty = prj->dirty();
	if (exists)
		*exists = !prj->exists();
	if (url && !prj->url().empty())
		*url = prj->url().c_str();
}

int Workspace::getCurrentAssetStates(
	bool* any,
	unsigned* type,
	bool* dirty,
	bool* pastable, bool* selectable,
	const char** undoable, const char** redoable,
	bool* readonly
) const {
	if (any)
		*any = false;
	if (type)
		*type = 0;
	if (dirty)
		*dirty = false;
	if (pastable)
		*pastable = false;
	if (selectable)
		*selectable = false;
	if (undoable)
		*undoable = nullptr;
	if (redoable)
		*redoable = nullptr;
	if (readonly)
		*readonly = false;

	return withCurrentAsset(
		[&] (Categories cat, BaseAssets::Entry* /* entry */, Editable* editor) -> void {
			if (any)
				*any = true;

			if (type)
				*type = (unsigned)cat;

			if (dirty)
				*dirty = editor->hasUnsavedChanges();

			if (pastable)
				*pastable = editor->pastable();
			if (selectable)
				*selectable = editor->selectable();

			if (undoable)
				*undoable = editor->undoable();
			if (redoable)
				*redoable = editor->redoable();

			if (readonly)
				*readonly = editor->readonly();
		}
	);
}

int Workspace::currentAssetPage(void) const {
	const Project::Ptr &prj = currentProject();
	if (!prj)
		return -1;

	switch (category()) {
	case Categories::FONT:
		return prj->activeFontIndex();
	case Categories::CODE:
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (prj->isMajorCodeEditorActive())
			return prj->activeMajorCodeIndex();
		else
			return prj->activeMinorCodeIndex();
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		return prj->activeMajorCodeIndex();
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	case Categories::TILES:
		return prj->activeTilesIndex();
	case Categories::MAP:
		return prj->activeMapIndex();
	case Categories::MUSIC:
		return prj->activeMusicIndex();
	case Categories::SFX:
		return prj->activeSfxIndex();
	case Categories::ACTOR:
		return prj->activeActorIndex();
	case Categories::SCENE:
		return prj->activeSceneIndex();
	default: // Do nothing.
		break;
	}

	return -1;
}

int Workspace::withCurrentAsset(EditorHandler handler) const {
	const Project::Ptr &prj = currentProject();
	if (!prj)
		return 0;

	BaseAssets::Entry* entry = nullptr;
	switch (category()) {
	case Categories::FONT:
		entry = prj->getFont(prj->activeFontIndex());

		break;
	case Categories::CODE:
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
		if (prj->isMajorCodeEditorActive())
			entry = prj->getCode(prj->activeMajorCodeIndex());
		else
			entry = prj->getCode(prj->activeMinorCodeIndex());
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
		entry = prj->getCode(prj->activeMajorCodeIndex());
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

		break;
	case Categories::TILES:
		entry = prj->getTiles(prj->activeTilesIndex());

		break;
	case Categories::MAP:
		entry = prj->getMap(prj->activeMapIndex());

		break;
	case Categories::MUSIC:
		entry = prj->getMusic(prj->activeMusicIndex());

		break;
	case Categories::SFX:
		entry = prj->getSfx(prj->activeSfxIndex());

		if (!entry && prj->sharedSfxEditor()) {
			Editable* editor_ = prj->sharedSfxEditor();
			if (handler)
				handler(category(), nullptr, editor_);
		}

		break;
	case Categories::ACTOR:
		entry = prj->getActor(prj->activeActorIndex());

		break;
	case Categories::SCENE:
		entry = prj->getScene(prj->activeSceneIndex());

		break;
	default: // Do nothing.
		break;
	}

	if (!entry)
		return 0;

	Editable* editor = nullptr;
#if GBBASIC_EDITOR_CODE_SPLIT_ENABLED
	if (prj->isMajorCodeEditorActive())
		editor = entry->editor;
	else
		editor = prj->minorCodeEditor();
#else /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */
	editor = entry->editor;
#endif /* GBBASIC_EDITOR_CODE_SPLIT_ENABLED */

	if (!editor)
		return 0;

	if (handler)
		handler(category(), entry, editor);

	return 1;
}

int Workspace::withAllAssets(EditorHandler handler) const {
	const Project::Ptr &prj = currentProject();
	if (!prj)
		return 0;

	int n = 0;
	for (int i = 0; i < prj->fontPageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getFont(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->codePageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getCode(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->tilesPageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getTiles(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->mapPageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getMap(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->musicPageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getMusic(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->sfxPageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getSfx(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->actorPageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getActor(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	for (int i = 0; i < prj->scenePageCount(); ++i) {
		BaseAssets::Entry* entry = prj->getScene(i);
		if (!entry)
			continue;
		Editable* editor = entry->editor;
		if (!editor)
			continue;

		if (handler)
			handler(category(), entry, editor);
		++n;
	}

	return n;
}

void Workspace::toggleFullscreen(Window* wnd) {
	settings().applicationWindowMaximized = false;

	settings().applicationWindowFullscreen = !settings().applicationWindowFullscreen;
	wnd->fullscreen(settings().applicationWindowFullscreen);
}

void Workspace::toggleMaximized(Window* wnd) {
	if (settings().applicationWindowFullscreen) {
		settings().applicationWindowFullscreen = false;
		wnd->fullscreen(false);
	}

	settings().applicationWindowMaximized = !settings().applicationWindowMaximized;
	if (settings().applicationWindowMaximized)
		wnd->maximize();
	else
		wnd->restore();
}

void Workspace::closeFilter(void) {
	if (!recentProjectsFilter().enabled)
		return;

	recentProjectsFilter().enabled = false;
	recentProjectsFilter().initialized = false;
	recentProjectsFilterFocused(false);
}

void Workspace::sortProjects(void) {
	switch ((Orders)settings().recentProjectsOrder) {
	case Orders::DEFAULT:
		std::sort(
			projects().begin(), projects().end(),
			[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
				if (l->order() < r->order())
					return true;
				if (l->order() > r->order())
					return false;

				std::string lt = l->title();
				std::string rt = r->title();
				Text::toLowerCase(lt);
				Text::toLowerCase(rt);

				const Text::Array la = Text::split(lt, "\t .)");
				const Text::Array ra = Text::split(rt, "\t .)");
				if (la.empty() || ra.empty())
					return lt < rt;

				std::string ls = la.front();
				std::string rs = ra.front();
				ls = Text::trimLeft(ls, "0");
				rs = Text::trimLeft(rs, "0");
				int ln = 0;
				int rn = 0;
				if (!Text::fromString(ls, ln) || !Text::fromString(rs, rn))
					return lt < rt;

				return ln < rn;
			}
		);

		break;
	case Orders::TITLE:
		std::sort(
			projects().begin(), projects().end(),
			[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
				std::string lt = l->title();
				std::string rt = r->title();
				Text::toLowerCase(lt);
				Text::toLowerCase(rt);

				const Text::Array la = Text::split(lt, "\t .)");
				const Text::Array ra = Text::split(rt, "\t .)");
				if (la.empty() || ra.empty())
					return lt < rt;

				std::string ls = la.front();
				std::string rs = ra.front();
				ls = Text::trimLeft(ls, "0");
				rs = Text::trimLeft(rs, "0");
				int ln = 0;
				int rn = 0;
				if (!Text::fromString(ls, ln) || !Text::fromString(rs, rn))
					return lt < rt;

				return ln < rn;
			}
		);

		break;
	case Orders::LATEST_MODIFIED:
		std::sort(
			projects().begin(), projects().end(),
			[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
				return l->modified() > r->modified();
			}
		);

		break;
	case Orders::EARLIEST_MODIFIED:
		std::sort(
			projects().begin(), projects().end(),
			[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
				return l->modified() < r->modified();
			}
		);

		break;
	case Orders::LATEST_CREATED:
		std::sort(
			projects().begin(), projects().end(),
			[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
				return l->created() > r->created();
			}
		);

		break;
	case Orders::EARLIEST_CREATED:
		std::sort(
			projects().begin(), projects().end(),
			[] (const Project::Ptr &l, const Project::Ptr &r) -> bool {
				return l->created() < r->created();
			}
		);

		break;
	default:
		GBBASIC_ASSERT(false && "Impossible.");

		break;
	}
}

void Workspace::validateProject(const Project* prj) {
	for (int index = 0; index < prj->musicPageCount(); ++index) {
		const MusicAssets::Entry* entry = prj->getMusic(index);
		const Music::Ptr &data = entry->data;
		const Music::Song* song = data->pointer();

		int another = -1;
		if (song->name.empty()) {
			const std::string msg = Text::format(theme()->warning_MusicMusicNameIsEmptyAtPage(), { Text::toString(index) });
			warn(msg.c_str());
		} else if (!prj->canRenameMusic(index, song->name, &another)) {
			const std::string msg = Text::format(theme()->warning_MusicDuplicateMusicNameAtPages(), { song->name, Text::toString(index), Text::toString(another) });
			warn(msg.c_str());
		}
		for (int i = 0; i < song->instrumentCount(); ++i) {
			another = -1;
			const Music::BaseInstrument* inst = song->instrumentAt(i, nullptr);
			if (inst->name.empty()) {
				const std::string msg = Text::format(theme()->warning_MusicMusicInstrumentNameIsEmptyAt(), { Text::toString(i) });
				warn(msg.c_str());
			} else if (!prj->canRenameMusicInstrument(index, i, inst->name, &another)) {
				const std::string msg = Text::format(theme()->warning_MusicDuplicateMusicInstrumentNameAt(), { inst->name, Text::toString(i), Text::toString(another) });
				warn(msg.c_str());
			}
		}
	}

	for (int index = 0; index < prj->sfxPageCount(); ++index) {
		const SfxAssets::Entry* entry = prj->getSfx(index);
		const Sfx::Ptr &data = entry->data;
		const Sfx::Sound* sound = data->pointer();

		int another = -1;
		if (sound->name.empty()) {
			const std::string msg = Text::format(theme()->warning_SfxSfxNameIsEmptyAtPage(), { Text::toString(index) });
			warn(msg.c_str());
		} else if (!prj->canRenameSfx(index, sound->name, &another)) {
			const std::string msg = Text::format(theme()->warning_SfxDuplicateSfxNameAtPages(), { sound->name, Text::toString(index), Text::toString(another) });
			warn(msg.c_str());
		}
	}

	for (int index = 0; index < prj->actorPageCount(); ++index) {
		const ActorAssets::Entry* entry = prj->getActor(index);

		int another = -1;
		if (entry->name.empty()) {
			const std::string msg = Text::format(theme()->warning_ActorActorNameIsEmptyAtPage(), { Text::toString(index) });
			warn(msg.c_str());
		} else if (!prj->canRenameActor(index, entry->name, &another)) {
			const std::string msg = Text::format(theme()->warning_ActorDuplicateActorNameAtPages(), { entry->name, Text::toString(index), Text::toString(another) });
			warn(msg.c_str());
		}
	}

	for (int index = 0; index < prj->scenePageCount(); ++index) {
		const SceneAssets::Entry* entry = prj->getScene(index);

		int another = -1;
		if (entry->name.empty()) {
			const std::string msg = Text::format(theme()->warning_SceneSceneNameIsEmptyAtPage(), { Text::toString(index) });
			warn(msg.c_str());
		} else if (!prj->canRenameScene(index, entry->name, &another)) {
			const std::string msg = Text::format(theme()->warning_SceneDuplicateSceneNameAtPages(), { entry->name, Text::toString(index), Text::toString(another) });
			warn(msg.c_str());
		}
	}
}

void Workspace::launchProject(
	Window* wnd, Renderer* rnd,
	const char* cartType, const char* sramType, bool* hasRtc,
	bool toRun_, int toExport_
) {
	if (settings().consoleClearOnStart)
		clear();

	const Project::Ptr &prj = currentProject();
	if (prj->contentType() == Project::ContentTypes::BASIC) {
		toRun(toRun_);
		toExport(toExport_);
		Operations::projectCompile(wnd, rnd, this, cartType, sramType, hasRtc, fontConfig().empty() ? nullptr : fontConfig().c_str(), true);
	} else {
		GBBASIC_ASSERT(toRun_ && toExport_ == -1 && "Not supported");

		toRun(toRun_);
		toExport(toExport_);

		_state = States::LAUNCHED;
	}
}

void Workspace::runProject(Window* wnd, Renderer* rnd, Bytes::Ptr rom) {
	// Notify the editors.
	Project::Ptr &prj = currentProject();
	if (prj) {
		prj->foreach(
			[rnd, this] (AssetsBundle::Categories, int, BaseAssets::Entry*, Editable* editor) -> void {
				if (!editor)
					return;

				editor->played(rnd, this);
			}
		);
	}

	// Start running.
	Bytes::Ptr sram(Bytes::create());
	Operations::projectLoadSram(wnd, rnd, this, prj, sram)
		.always(
			[wnd, rnd, this, rom, sram] (void) -> void {
				Operations::projectRun(wnd, rnd, this, rom, sram, false);
			}
		);
}

void Workspace::stopProject(Window* wnd, Renderer* rnd) {
	// Reset the debug information.
	debug();

	// Stop running.
	const bool traceless = canvasDevice() && canvasDevice()->traceless();
	Operations::projectStop(wnd, rnd, this)
		.then(
			[wnd, rnd, this, traceless] (bool ok, const Bytes::Ptr sram) -> void {
				if (!traceless && ok && settings().deviceSaveSramOnStop) {
					const Project::Ptr &prj = currentProject();

					Operations::projectSaveSram(wnd, rnd, this, prj, sram, false);
				}
			}
		);

	// Notify the editors.
	Project::Ptr &prj = currentProject();
	if (prj) {
		prj->foreach(
			[rnd, this] (AssetsBundle::Categories, int, BaseAssets::Entry*, Editable* editor) -> void {
				if (!editor)
					return;

				editor->stopped(rnd, this);
			}
		);
	}
}

void Workspace::exportProject(Window* wnd, Renderer* rnd, int toExport_) {
	Exporter::Ptr ex = exporters()[toExport_];

	const Project::Ptr &prj = currentProject();
	const bool isEditable = prj->contentType() == Project::ContentTypes::BASIC;
	if (!isEditable) {
		if (ex->packageArchived() || ex->buildEnabled()) {
			bubble(theme()->dialogPrompt_NoNeedToBuildRom(), nullptr);

			return;
		}
	}

	if (ex->packageArchived()) {
		Operations::popupEmulatorBuildSettings(wnd, rnd, this, ex, settings().exporterSettings.c_str(), settings().exporterArgs.c_str(), !ex->packageIcon().empty())
			.then(
				[wnd, rnd, this, toExport_] (bool /* ok */) -> void {
					launchProject(wnd, rnd, nullptr, nullptr, nullptr, false, toExport_);
				}
			);
	} else if (ex->buildEnabled()) {
		Operations::popupRomBuildSettings(wnd, rnd, this, ex)
			.then(
				[wnd, rnd, this, toExport_] (bool ok, const char* cartType, const char* sramType, bool hasRtc) -> void {
					if (ok)
						launchProject(wnd, rnd, cartType, sramType, &hasRtc, false, toExport_);
					else
						launchProject(wnd, rnd, nullptr, nullptr, nullptr, false, toExport_);
				}
			);
	} else {
		if (toExport_ >= 0) {
			Exporter::Ptr ex = exporters()[toExport_];
			ex->run(
				nullptr, nullptr, nullptr, nullptr,
				"", "", nullptr,
				[this] (const std::string &msg, int lv) -> void {
					switch (lv) {
					case 0: // Print.
						print(msg.c_str());

						break;
					case 1: // Warn.
						warn(msg.c_str());

						break;
					case 2: // Error.
						error(msg.c_str());

						break;
					}
				}
			);
		}
	}
}

/* ===========================================================================} */
