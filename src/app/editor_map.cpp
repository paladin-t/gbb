/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_map.h"
#include "editor_map.h"
#include "editor_tiles.h"
#include "theme.h"
#include "workspace.h"
#include "resource/inline_resource.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#include "../utils/marker.h"
#include "../utils/parsers.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

#include "editor_map.inl"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_MAP_UNLOAD_SYMBOLS_ON_FINISH
#	define EDITOR_MAP_UNLOAD_SYMBOLS_ON_FINISH 1
#endif /* EDITOR_MAP_UNLOAD_SYMBOLS_ON_FINISH */

#ifndef EDITOR_MAP_LOCAL_PALETTE_COLOR_CHANGED_DEBOUNCE_INTERVAL
#	define EDITOR_MAP_LOCAL_PALETTE_COLOR_CHANGED_DEBOUNCE_INTERVAL 0.5
#endif /* EDITOR_MAP_LOCAL_PALETTE_COLOR_CHANGED_DEBOUNCE_INTERVAL */

/* ===========================================================================} */

/*
** {===========================================================================
** Map editor
*/

class EditorMapImpl : public EditorMap {
private:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> CelGetter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> CelSetter;

	typedef std::function<int(const Math::Recti* /* nullable */, Editing::Dots &)> CelsGetter;
	typedef std::function<int(const Math::Recti* /* nullable */, const Editing::Dots &)> CelsSetter;

	typedef std::function<void(void)> PostHandler;

	struct TileIssue {
		typedef std::vector<TileIssue> Array;

		int x = -1;
		int y = -1;
		std::string message;

		TileIssue() {
		}
		TileIssue(int x_, int y_, const char* msg) :
			x(x_), y(y_)
		{
			if (msg)
				message = msg;
		}
	};

	struct Processor {
		typedef std::function<void(Renderer*)> Handler;

		Handler down = nullptr;
		Handler move = nullptr;
		Handler up = nullptr;
		Handler hover = nullptr;

		Processor() {
		}
		Processor(Handler d, Handler m, Handler u, Handler h) :
			down(d), move(m), up(u), hover(h)
		{
		}
	};

private:
	bool _opened = false;

	Project* _project = nullptr; // Foreign.
	int _index = -1;
	CommandQueue* _commands = nullptr;

	Math::Vec2i _tileSize = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
	Editing::Tiler _cursor;
	struct {
		Math::Vec2i initial = Math::Vec2i(-1, -1);
		Editing::Tiler tiler;

		ImVec2 mouse = ImVec2(-1, -1);
		int idle = 1;

		Editing::Tiler hoveringMapIndex;
		int hoveringMapCel = -1;
		std::string hoveringMapTips;

		void clear(void) {
			initial = Math::Vec2i(-1, -1);
			tiler = Editing::Tiler();

			mouse = ImVec2(-1, -1);
			idle = 1;

			hoveringMapIndex = Editing::Tiler();
			hoveringMapCel = -1;
			hoveringMapTips.clear();
		}
		int area(Math::Recti &area) const {
			if (tiler.empty())
				return 0;

			area = Math::Recti::byXYWH(tiler.position.x, tiler.position.y, tiler.size.x, tiler.size.y);

			return area.width() * area.height();
		}
	} _selection;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _page;
	struct {
		std::string text;
		Editing::Tiler cursor;
		Marker<bool> transferring = Marker<bool>(false);
		std::string info;

		void clear(void) {
			text.clear();
			cursor = Editing::Tiler();
			transferring = Marker<bool>(false);
			info.clear();
		}
	} _status;
	mutable float _statusWidth = 0.0f;
	TileIssue::Array _tileIssues;
	struct {
		Text::Array warnings;
		std::string text;

		bool empty(void) const {
			return warnings.empty();
		}
		void clear(void) {
			warnings.clear();
			text.clear();
		}
		bool add(const std::string &txt) {
			if (std::find(warnings.begin(), warnings.end(), txt) != warnings.end())
				return false;

			warnings.push_back(txt);

			flush();

			return true;
		}
		bool remove(const std::string &txt) {
			Text::Array::iterator it = std::find(warnings.begin(), warnings.end(), txt);
			if (it == warnings.end())
				return false;

			warnings.erase(it);

			flush();

			return true;
		}
		void flush(void) {
			text.clear();
			for (int i = 0; i < (int)warnings.size(); ++i) {
				const std::string &w = warnings[i];
				text += "* ";
				text += w;
				if (i != (int)warnings.size() - 1)
					text += "\n";
			}
		}
	} _warnings;
	std::function<void(const Command*)> _refresh = nullptr;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _estimated;
	Editing::Painting _painting;
	struct {
		int width = 0; // In tiles.
		int height = 0; // In tiles.

		void clear(void) {
			width = 0;
			height = 0;
		}
	} _size;
	EditorMapAsImage _asImage;
	struct { // Transfer for edit-as-image.
		Window* window = nullptr; // Foreign.
		Renderer* renderer = nullptr; // Foreign.
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;
		bool filled = true; // Defaults to filled.
		Semaphore generated;

		void reset(void) {
			image = nullptr;
			texture = nullptr;
			filled = false;
			generated.wait();
		}
		void clear(void) {
			renderer = nullptr;
			image = nullptr;
			texture = nullptr;
			filled = false;
			generated.wait();
		}
	} _transfer;
	struct {
		CelGetter getCel = nullptr;
		CelSetter setCel = nullptr;
		CelsGetter getCels = nullptr;
		CelsSetter setCels = nullptr;

		bool empty(void) const {
			return !getCel || !setCel || !getCels || !setCels;
		}
		void clear(void) {
			getCel = nullptr;
			setCel = nullptr;
			getCels = nullptr;
			setCels = nullptr;
		}
		void fill(CelGetter getCel_, CelSetter setCel_, CelsGetter getCels_, CelsSetter setCels_) {
			getCel = getCel_;
			setCel = setCel_;
			getCels = getCels_;
			setCels = setCels_;
		}
	} _binding;
	Editing::MapCelGetter _overlay = nullptr;

	struct Ref : public Editor::Ref {
		bool resolving = false;
		unsigned refCategory = (unsigned)(~0);
		int refIndex = -1;
		std::string refTilesText;
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;
		Editing::Brush cursor;
		Editing::Brush attributesCursor;
		Editing::Brush stamp;
		std::string byteTooltip;

		bool showGrids = true;
		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool transparentBackbroundVisible = true;

		void clear(void) {
			resolving = false;
			refCategory = (unsigned)(~0);
			refIndex = -1;
			refTilesText.clear();
			image = nullptr;
			texture = nullptr;
			cursor = Editing::Brush();
			attributesCursor = Editing::Brush();
			stamp = Editing::Brush();
			byteTooltip.clear();

			showGrids = true;
			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			transparentBackbroundVisible = true;
		}
		bool fill(Renderer* rnd, const Project* project, bool force) {
			if (!force) {
				if (image && texture)
					return true;
			}

			const TilesAssets::Entry* entry = project->getTiles(refIndex);
			if (!entry)
				return false;

			image = entry->data;
			if (!image)
				return false;

			texture = Texture::Ptr(Texture::create());
			texture->fromImage(rnd, Texture::STATIC, image.get(), Texture::NEAREST);
			texture->blend(Texture::BLEND);

			return true;
		}

		void refreshRefTilesText(Theme* theme) {
			refTilesText = Text::format(theme->windowMap_RefTiles(), { Text::toString(refIndex) });
		}
	} _ref;
	std::function<void(int)> _setLayer = nullptr;
	std::function<void(int, bool)> _setLayerEnabled = nullptr;
	struct Tools {
		int layer = 0;
		int editingLocalPaletteColorGroup = -1;
		int editingLocalPaletteColorIndex = -1;
		bool openLocalPaletteColorPicker = false;
		Debounce debounceLocalPaletteColorChanged = Debounce(EDITOR_MAP_LOCAL_PALETTE_COLOR_CHANGED_DEBOUNCE_INTERVAL);
		Editing::Tools::PaintableTools painting = Editing::Tools::PENCIL;
		Editing::Tools::BitwiseOperations bitwiseOperations = Editing::Tools::SET;
		bool scaled = false;
		int magnification = -1;
		int weighting = 0;
		std::string namableText;

		bool inputFieldFocused = false;
		bool showGrids = true;
		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool fineZooming = false;
		bool transparentBackbroundVisible = true;

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);
		int mouseActionButton = ImGuiMouseButton_Left;

		std::function<bool(void)> playMapTesting = nullptr;
		std::function<bool(void)> stopMapTesting = nullptr;
		bool isPlaying = false;
		bool isPlayerConfigLoaded = false;
		std::string playerConfigText;
		std::string playerSymbolsText;
		std::string playerAliasesText;

		PostHandler post = nullptr;
		Editing::Tools::PaintableTools postType = Editing::Tools::PENCIL;

		void clear(void) {
			layer = 0;
			editingLocalPaletteColorGroup = -1;
			editingLocalPaletteColorIndex = -1;
			openLocalPaletteColorPicker = false;
			debounceLocalPaletteColorChanged.clear();
			painting = Editing::Tools::PENCIL;
			bitwiseOperations = Editing::Tools::SET;
			scaled = false;
			magnification = -1;
			weighting = 0;
			namableText.clear();

			inputFieldFocused = false;
			showGrids = true;
			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			fineZooming = false;
			transparentBackbroundVisible = true;

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);
			mouseActionButton = ImGuiMouseButton_Left;

			playMapTesting = nullptr;
			stopMapTesting = nullptr;
			isPlaying = false;
			isPlayerConfigLoaded = false;
			playerConfigText.clear();
			playerSymbolsText.clear();
			playerAliasesText.clear();

			post = nullptr;
			postType = Editing::Tools::PENCIL;
		}
	} _tools;
	Debounce _debounce;
	Processor _processors[Editing::Tools::COUNT] = {
		Processor( // Hand.
			std::bind(&EditorMapImpl::handToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::handToolMove, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::handToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::handToolHover, this, std::placeholders::_1)
		),
		Processor( // Eyedropper.
			std::bind(&EditorMapImpl::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		),
		Processor( // Pencil.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorMapImpl::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Lasso.
			std::bind(&EditorMapImpl::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::lassoToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Line.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Line>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Line>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Box>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Box>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box fill.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse fill.
			std::bind(&EditorMapImpl::paintToolDown<Commands::Map::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolMove<Commands::Map::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Stamp, pasting.
			std::bind(&EditorMapImpl::stampToolDown, this, std::placeholders::_1),
			nullptr,
			std::bind(&EditorMapImpl::stampToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::stampToolHover, this, std::placeholders::_1)
		),
		Processor( // Smudge.
			nullptr,
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Eraser.
			nullptr,
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Move.
			nullptr,
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Resize.
			nullptr,
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Ref.
			nullptr,
			nullptr,
			nullptr,
			nullptr
		)
	};

public:
	EditorMapImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Map::Pencil>()
			->reg<Commands::Map::Line>()
			->reg<Commands::Map::Box>()
			->reg<Commands::Map::BoxFill>()
			->reg<Commands::Map::Ellipse>()
			->reg<Commands::Map::EllipseFill>()
			->reg<Commands::Map::Fill>()
			->reg<Commands::Map::Replace>()
			->reg<Commands::Map::Stamp>()
			->reg<Commands::Map::Rotate>()
			->reg<Commands::Map::Flip>()
			->reg<Commands::Map::Cut>()
			->reg<Commands::Map::Paste>()
			->reg<Commands::Map::Delete>()
			->reg<Commands::Map::SetLocalPaletteEnabled>()
			->reg<Commands::Map::SetLocalPalette>()
			->reg<Commands::Map::SetName>()
			->reg<Commands::Map::SwitchLayer>()
			->reg<Commands::Map::ToggleMask>()
			->reg<Commands::Map::Resize>()
			->reg<Commands::Map::SetOptimized>()
			->reg<Commands::Map::Import>()
			->reg<Commands::Map::BeginGroup>()
			->reg<Commands::Map::EndGroup>()
			->reg<Commands::Map::Reference>();
	}
	virtual ~EditorMapImpl() override {
		close(_index);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* project, int index, unsigned refCategory, int refIndex) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		_size.width = object()->width();
		_size.height = object()->height();
		entry()->refresh(); // Refresh the outdated editable resources.

		Map::Tiles tiles;
		if (object() && object()->tiles(tiles) && tiles.texture) {
			_tileSize = tiles.size();

			if (_tileSize.x == 0)
				_tileSize.x = GBBASIC_TILE_SIZE;
			if (_tileSize.y == 0)
				_tileSize.y = GBBASIC_TILE_SIZE;
		}

		_refresh = std::bind(&EditorMapImpl::refresh, this, ws, std::placeholders::_1);

		_ref.refCategory = refCategory;
		_ref.refIndex = refIndex;
		_ref.refreshRefTilesText(ws->theme());
		_ref.cursor.position = Math::Vec2i(0, 0);
		_ref.cursor.size = _tileSize;
		_ref.attributesCursor.position = Math::Vec2i(0, 0);
		_ref.attributesCursor.size = _tileSize;
		_ref.stamp.position = Math::Vec2i(0, 0);
		_ref.stamp.size = _tileSize;

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_ref.showGrids = _project->preferencesShowGrids();
		_ref.gridsVisible = !caps.pressed();
		_ref.gridUnit = _ref.cursor.size;
		_ref.transparentBackbroundVisible = num.pressed();

		_transfer.window = wnd;
		_transfer.renderer = rnd;
		if (entry()->image)
			_transfer.image = entry()->image;

		_setLayer = std::bind(&EditorMapImpl::changeLayer, this, wnd, rnd, ws, std::placeholders::_1);
		_setLayerEnabled = std::bind(&EditorMapImpl::toggleLayer, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2);

		_tools.magnification = entry()->magnification;
		_tools.namableText = entry()->name;
		_tools.showGrids = _project->preferencesShowGrids();
		_tools.fineZooming = _project->preferencesFineZooming();
		_tools.gridsVisible = !caps.pressed();
		_tools.gridUnit = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
		_tools.transparentBackbroundVisible = num.pressed();
		_tools.playMapTesting = std::bind(&EditorMapImpl::playMapTesting, this, wnd, rnd, ws);
		_tools.stopMapTesting = std::bind(&EditorMapImpl::stopMapTesting, this, wnd, rnd, ws);

		_asImage.initialize(
			rnd, ws,
			_project,
			[&] (void) -> Image::Ptr & {
				Image::Ptr &img = objectAsImage();

				return img;
			},
			[&] (void) -> Texture::Ptr & {
				Texture::Ptr &tex = textureAsImage();

				return tex;
			},
			&_tools.painting,
			&_tools.mouseActionButton,
			&_tools.weighting,
			&_debounce,
			[&] (void) -> void {
				_transfer.image = nullptr;
				_transfer.texture = nullptr;
			},
			[&] (void) -> void {
				_transfer.texture = nullptr;

				touchAsImage(); // Ensure the runtime resources are ready.
			},
			std::bind(&EditorMapImpl::invalidateTiled, this)
		);

		fprintf(stdout, "Map editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Map editor closed: #%d.\n", _index);

		_asImage.clear();
		_asImage.dispose();
		_transfer.clear();

		_project = nullptr;
		_index = -1;

		_tileSize = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
		_selection.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_tileIssues.clear();
		_warnings.clear();
		_refresh = nullptr;
		_estimated.clear();
		_painting.clear();
		_size.clear();
		_binding.clear();
		_overlay = nullptr;

		_ref.clear();
		_setLayer = nullptr;
		_setLayerEnabled = nullptr;
		_tools.clear();
		_debounce.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace* /* ws */) override {
		if (!isEditingAsImage()) {
			_transfer.filled = true;

			return;
		}

		/*const int ref = entry()->ref;
		const Project::Indices mapsRefToTheSameTiles = _project->getMapsRefToTiles(ref);
		if (mapsRefToTheSameTiles.size() > 1) {
			ImGui::MessagePopupBox::ConfirmedHandler confirm(
				[ws] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					// Do nothing.
				},
				nullptr
			);
			ImGui::MessagePopupBox::DeniedHandler deny(
				[ws, this] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					entry()->editAsImage = false;
				},
				nullptr
			);
			ws->messagePopupBox(
				ws->theme()->dialogPrompt_MultipleMapAssetsReferenceTheSourceTilesAssetContinueAnyway(),
				confirm,
				deny,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr
			);
		}*/
	}
	virtual void leave(class Workspace* ws) override {
		_tools.stopMapTesting();

		if (isEditingAsImage()) {
			if (!_transfer.filled) {
				ImGui::WaitingPopupBox::TimeoutHandler timeout(
					[ws, this] (void) -> void {
						if (!_transfer.filled) {
							transferImageToTiled(ws);

							_transfer.generated.wait();
						}

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ws->waitingPopupBox(
					true, ws->theme()->dialogPrompt_Transferring(),
					true, timeout,
					true
				);
			}
		}

		if (entry())
			entry()->cleanup(); // Clean up the outdated editable and runtime resources.
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return false;
	}
	virtual void readonly(bool) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		if (isEditingAsImage())
			return _asImage.hasUnsavedChanges();

		return _commands->hasUnsavedChanges();
	}
	virtual void markChangesSaved(void) override {
		if (isEditingAsImage()) {
			_asImage.markChangesSaved();

			return;
		}

		_commands->markChangesSaved();
	}
	virtual void prepareForSaving(class Workspace* ws) override {
		if (!_transfer.filled) {
			transferImageToTiled(ws);

			_transfer.generated.wait();
		}
	}

	virtual void copy(void) override {
		copy(true);
	}
	void copy(bool clearSelection) {
		if (isEditingAsImage()) {
			_asImage.copy(clearSelection);

			return;
		}

		auto toString = [] (const Math::Recti &area, const Editing::Dots &dots) -> std::string {
			rapidjson::Document doc;
			Jpath::set(doc, doc, area.width(), "width");
			Jpath::set(doc, doc, area.height(), "height");
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					Jpath::set(doc, doc, dots.indexed[k], "data", k);

					++k;
				}
			}

			std::string buf;
			Json::toString(doc, buf);

			return buf;
		};

		if (!_binding.getCels || !_binding.setCels)
			return;

		areaOrPoint(nullptr);

		Math::Recti sel;
		const Math::Recti* selPtr = nullptr;
		int size = _selection.area(sel);
		if (size == 0)
			size = object()->width() * object()->height();
		else
			selPtr = &sel;

		typedef std::vector<int> Data;

		Data vec(size, 0);
		Editing::Dots dots(&vec.front());
		_binding.getCels(selPtr, dots);

		const std::string buf = toString(sel, dots);
		const std::string osstr = Unicode::toOs(buf);

		Platform::setClipboardText(osstr.c_str());

		if (clearSelection)
			_selection.clear();
	}
	virtual void cut(void) override {
		if (isEditingAsImage()) {
			_asImage.cut();

			return;
		}

		if (!_binding.getCel || !_binding.setCel)
			return;

		copy(false);

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		enqueue<Commands::Map::Cut>()
			->with(_binding.getCel, _binding.setCel)
			->with(sel)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		_selection.clear();
	}
	virtual bool pastable(void) const override {
		if (isEditingAsImage())
			return _asImage.pastable();

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (isEditingAsImage()) {
			_asImage.paste();

			return;
		}

		auto fromString = [] (const std::string &buf, Math::Recti &area, Editing::Dot::Array &dots) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int width = -1, height = -1;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;
			area = Math::Recti::byXYWH(0, 0, width, height);
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					int idx = Map::INVALID();
					if (!Jpath::get(doc, idx, "data", k))
						return false;

					Editing::Dot dot;
					dot.indexed = idx;
					dots.push_back(dot);

					++k;
				}
			}
			dots.shrink_to_fit();

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Math::Recti area;
		Editing::Dot::Array dots;
		if (!fromString(buf, area, dots))
			return;

		const Editing::Tools::PaintableTools prevTool = _tools.painting;

		_tools.painting = Editing::Tools::STAMP;

		_tools.bitwiseOperations = Editing::Tools::SET;

		_processors[Editing::Tools::STAMP] = Processor{
			nullptr,
			nullptr,
			std::bind(&EditorMapImpl::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
			std::bind(&EditorMapImpl::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
		};

		destroyOverlay();
	}
	virtual void del(bool wholeLine) override {
		if (isEditingAsImage()) {
			_asImage.del(wholeLine);

			return;
		}

		if (!_binding.getCel || !_binding.setCel)
			return;

		areaOrPoint(nullptr);

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		enqueue<Commands::Map::Delete>()
			->with(_binding.getCel, _binding.setCel)
			->with(sel)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		_selection.clear();
	}
	int area(Math::Recti* area /* nullable */) const {
		if (isEditingAsImage())
			return _asImage.area(area);

		Math::Recti sel;
		int size = _selection.area(sel);
		if (size == 0) {
			sel = Math::Recti::byXYWH(0, 0, object()->width(), object()->height());
			size = object()->width() * object()->height();
		}
		if (area)
			*area = sel;

		return size;
	}
	int areaOrPoint(Math::Recti* area /* nullable */) {
		if (isEditingAsImage())
			return 0;

		Math::Recti sel;
		int size = _selection.area(sel);
		if (size <= 1) {
			_selection.tiler.position = Math::Vec2i(_cursor.position.x, _cursor.position.y);
			_selection.tiler.size = Math::Vec2i(1, 1);
			size = _selection.area(sel);
		}
		if (area)
			*area = sel;

		return size;
	}
	virtual bool selectable(void) const override {
		if (isEditingAsImage())
			return _asImage.selectable();

		return true;
	}

	virtual const char* redoable(void) const override {
		if (isEditingAsImage())
			return _asImage.redoable();

		const Command* cmd = _commands->redoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}
	virtual const char* undoable(void) const override {
		if (isEditingAsImage())
			return _asImage.undoable();

		const Command* cmd = _commands->undoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}

	virtual void redo(BaseAssets::Entry* entry_) override {
		if (isEditingAsImage()) {
			_asImage.redo(entry_);

			return;
		}

		auto execRedo = [this] (void) -> void {
			const Command* cmd = _commands->redoable();
			if (!cmd)
				return;

			const int width = object()->width();
			const int height = object()->height();
			const bool multipleLayers = Command::is<Commands::Map::Resize>(cmd) || Command::is<Commands::Map::Import>(cmd);
			if (multipleLayers)
				_commands->redo(object(), Variant((void*)entry()), attributes());
			else
				_commands->redo(currentLayer(), Variant((void*)entry()));

			if (width != object()->width() || height != object()->height())
				_selection.clear();

			_refresh(cmd);
		};

		const Command* cmd = _commands->redoable();
		if (!cmd)
			return;

		if (Command::is<Commands::Map::BeginGroup>(cmd)) {
			do {
				execRedo();
				cmd = _commands->redoable();
			} while (!Command::is<Commands::Map::EndGroup>(cmd));
			execRedo();
		} else {
			execRedo();
		}

		_project->toPollEditor(true);

		entry()->increaseRevision();

		modified();
	}
	virtual void undo(BaseAssets::Entry* entry_) override {
		if (isEditingAsImage()) {
			_asImage.undo(entry_);

			return;
		}

		auto execUndo = [this] (void) -> void {
			const Command* cmd = _commands->undoable();
			if (!cmd)
				return;

			const int width = object()->width();
			const int height = object()->height();
			const bool multipleLayers = Command::is<Commands::Map::Resize>(cmd) || Command::is<Commands::Map::Import>(cmd);
			if (multipleLayers)
				_commands->undo(object(), Variant((void*)entry()), attributes());
			else
				_commands->undo(currentLayer(), Variant((void*)entry()));

			if (width != object()->width() || height != object()->height())
				_selection.clear();

			_refresh(cmd);
		};

		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		if (Command::is<Commands::Map::EndGroup>(cmd)) {
			do {
				execUndo();
				cmd = _commands->undoable();
			} while (!Command::is<Commands::Map::BeginGroup>(cmd));
			execUndo();
		} else {
			execUndo();
		}

		_project->toPollEditor(true);

		entry()->increaseRevision();

		modified();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		if (isEditingAsImage()) {
			bool continue_ = true;
			const Variant ret = _asImage.post(msg, argc, argv, &continue_);
			if (!continue_)
				return ret;
		}

		switch (msg) {
		case RESIZE: {
				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				if (width == object()->width() && height == object()->height())
					return Variant(true);

				Command* cmd = enqueue<Commands::Map::Resize>()
					->with(Math::Vec2i(width, height))
					->with(_setLayer, _tools.layer)
					->exec(object(), Variant((void*)entry()), attributes());

				_refresh(cmd);

				_selection.clear();
			}

			return Variant(true);
		case SELECT_ALL:
			_selection.tiler.position = Math::Vec2i(0, 0);
			_selection.tiler.size = Math::Vec2i(object()->width(), object()->height());

			return Variant(true);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		case UPDATE_REF_INDEX: {
				Workspace* ws = (Workspace*)unpack<void*>(argc, argv, 0, nullptr);
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 1, _index);

				_ref.refIndex = (int)index;
				_ref.refreshRefTilesText(ws->theme());

				_commands->foreach(
					[] (Command* cmd) -> void {
						if (Command::is<Commands::Map::Reference>(cmd)) {
							Commands::Map::Reference* cmdRef = Command::as<Commands::Map::Reference>(cmd);
							cmdRef->valid(false); // Break the command ref when resolved ref.
						}
					}
				);
			}

			return Variant(true);
		case SET_FRAME: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				constexpr const int layers[] = {
					ASSETS_MAP_GRAPHICS_LAYER,
					ASSETS_MAP_ATTRIBUTES_LAYER
				};
				const bool isInLayers = std::any_of(
					std::begin(layers), std::end(layers),
					[index] (int layer) -> bool {
						return layer == index;
					}
				);

				if (!isInLayers)
					return Variant(false);

				_setLayer(index);
			}

			return Variant(true);
		case CLEAR_UNDO_REDO_RECORDS: {
				const bool deep = unpack<bool>(argc, argv, 0, true);

				if (deep)
					_commands->clear();
			}

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		if (_binding.empty()) {
			_binding.fill(
				std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::getCels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::setCels, this, rnd, std::placeholders::_1, std::placeholders::_2)
			);
		}

		const bool allowMouseOperations = shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		if (!entry() || !object()) {
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			ImGui::EndChild();
			refreshStatus(wnd, rnd, ws);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}
		if (_ref.resolving || entry()->ref == -1) { // This asset's dependency was lost, need to resolve; or a user is changing it manually.
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ws->theme()->style()->screenClearingColor);
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			resolveRef(wnd, rnd, ws, width, height - statusBarHeight);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			refreshStatus(wnd, rnd, ws);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}
		
		const bool editAsTiled = !(_tools.layer == ASSETS_MAP_GRAPHICS_LAYER && isEditingAsImage());
		if (editAsTiled)
			updateAsTiled(wnd, rnd, ws, width, height, splitter, statusBarHeight, statusBarActived, allowMouseOperations);
		else
			updateAsImage(wnd, rnd, ws, width, height, splitter, statusBarHeight, statusBarActived, allowMouseOperations);

		if (_tools.debounceLocalPaletteColorChanged.fire()) {
			entry()->cleanup();
			ws->skipFrame(); // Skip a frame to avoid glitch.

			if (isEditingAsImage()) {
				//_transfer.reset();

				_debounce.modified();
			} else {
				_transfer.filled = true;
			}
		}

		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;
		_status.cursor = Editing::Tiler();
		_estimated.filled = false;
	}

	virtual void added(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}
	virtual void removed(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}

	virtual void played(class Renderer*, class Workspace*) override {
		_tools.stopMapTesting();
	}
	virtual void stopped(class Renderer*, class Workspace*) override {
		_tools.stopMapTesting();
	}

	virtual void resized(class Renderer*, const Math::Vec2i &, const Math::Vec2i &) override {
		entry()->cleanup(); // Clean up the outdated editable and runtime resources.

		_ref.image = nullptr;
		_ref.texture = nullptr;

		_ref.windowResized();
	}

	virtual void focusLost(class Renderer*) override {
		entry()->cleanup(); // Clean up the outdated editable and runtime resources.

		_ref.image = nullptr;
		_ref.texture = nullptr;
	}
	virtual void focusGained(class Renderer* rnd) override {
		_ref.fill(rnd, _project, false);

		if (object()) {
			Map::Tiles tiles;
			object()->tiles(tiles);
			tiles.texture = _ref.texture;
			object()->tiles(&tiles);
		}
	}

private:
	void updateAsTiled(
		Window* wnd, Renderer* rnd,
		Workspace* ws,
		float width, float height,
		const Ref::Splitter &splitter, const float &statusBarHeight, bool &statusBarActived,
		bool allowMouseOperations
	) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (entry()->refresh()) { // Refresh the outdated editable resources.
			_ref.fill(rnd, _project, true);

			entry()->touch();
		}
		Map::Tiles tiles;
		if (!object()->tiles(tiles) || !tiles.texture)
			entry()->touch(); // Ensure the runtime resources are ready.
		if (!object()->tiles(tiles) || !tiles.texture)
			return;

		constexpr const int MAGNIFICATIONS[] = {
			1, 2, 4, 8
		};
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav;
		if (_tools.scaled) {
			flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		}
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - statusBarHeight), false, flags);
		{
			auto getPlt = [&] (const Math::Vec2i &pos) -> int {
				const int attrs = attributes()->get(pos.x, pos.y);
				const int bits = attrs & ((0x00000001 << GBBASIC_MAP_PALETTE_BIT0) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT1) | (0x00000001 << GBBASIC_MAP_PALETTE_BIT2));

				return bits;
			};
			auto getCol = [&] (int plt) -> const PaletteAssets::Entry* {
				const PaletteAssets::Entry* entry_ = nullptr;
				const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
				const bool localPaletteEnabled = entry()->localPaletteEnabled;
				const PaletteAssets::Array &localPalette = entry()->localPalette;
				if (localPaletteEnabled && (int)localPalette.size() >= n) {
					plt = Math::clamp(plt, 0, (int)localPalette.size() - 1);
					entry_ = &localPalette[plt];
				} else {
					entry_ = _project->getPalette(plt);
				}

				return entry_;
			};
			auto getFlip = [&] (const Math::Vec2i &pos) -> int {
				if (!entry()->hasAttributes)
					return (int)Editing::Flips::NONE;

				const int attrs = attributes()->get(pos.x, pos.y);
				const bool hFlip = !!((attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001);
				const bool vFlip = !!((attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001);
				if (hFlip && vFlip)
					return (int)Editing::Flips::HORIZONTAL_VERTICAL;
				else if (hFlip)
					return (int)Editing::Flips::HORIZONTAL;
				else if (vFlip)
					return (int)Editing::Flips::VERTICAL;

				return (int)Editing::Flips::NONE;
			};
			auto fillCelTooltip = [&] (const Editing::Tiler &cursor) -> void {
				if (_selection.hoveringMapIndex == cursor)
					return;

				_selection.hoveringMapIndex = cursor;

				if (_selection.hoveringMapIndex.size != Math::Vec2i(1, 1)) {
					_selection.hoveringMapTips.clear();

					return;
				}

				_selection.hoveringMapCel = currentLayer()->get(_selection.hoveringMapIndex.position.x, _selection.hoveringMapIndex.position.y);
				if (_selection.hoveringMapCel == Map::INVALID()) {
					_selection.hoveringMapTips.clear();

					return;
				}
				if (_selection.hoveringMapCel == 0) {
					_selection.hoveringMapTips.clear();

					return;
				}

				_selection.hoveringMapTips.clear();

				_selection.hoveringMapTips = " " + ws->theme()->status_Pos();
				_selection.hoveringMapTips += " ";
				_selection.hoveringMapTips += Text::toString(_selection.hoveringMapIndex.position.x);
				_selection.hoveringMapTips += ",";
				_selection.hoveringMapTips += Text::toString(_selection.hoveringMapIndex.position.y);

				_selection.hoveringMapTips += " ";

				_selection.hoveringMapTips += ws->theme()->status_Tl();
				_selection.hoveringMapTips += " ";
				_selection.hoveringMapTips += Text::toString(_selection.hoveringMapCel);
				_selection.hoveringMapTips += "(0x";
				_selection.hoveringMapTips += Text::toHex(_selection.hoveringMapCel, 2, '0', true);
				_selection.hoveringMapTips += ")";
				_selection.hoveringMapTips += "\n";

				const int attrs = _selection.hoveringMapCel;
				const int pltBits = attrs & ((1 << GBBASIC_MAP_PALETTE_BIT2) | (1 << GBBASIC_MAP_PALETTE_BIT1) | (1 << GBBASIC_MAP_PALETTE_BIT0));
				const int bankBit = (attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001;
				const int hFlipBit = (attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001;
				const int vFlipBit = (attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001;
				const int priBit = (attrs >> GBBASIC_MAP_PRIORITY_BIT) & 0x00000001;
				_selection.hoveringMapTips += Text::format(ws->theme()->tooltipMap_BitsPalette(), { Text::toString(pltBits) });
				_selection.hoveringMapTips += Text::format(ws->theme()->tooltipMap_BitBank(), { Text::toString(bankBit) });
				_selection.hoveringMapTips += Text::format(ws->theme()->tooltipMap_BitHFlip(), { Text::toString(hFlipBit) });
				_selection.hoveringMapTips += Text::format(ws->theme()->tooltipMap_BitVFlip(), { Text::toString(vFlipBit) });
				_selection.hoveringMapTips += Text::format(ws->theme()->tooltipMap_BitPriority(), { Text::toString(priBit) });
			};

			if (_cursor.empty()) {
				_cursor.position = Math::Vec2i(0, 0);
				_cursor.size = Math::Vec2i(1, 1);
			}

			Editing::Tiler cursor = _cursor;
			const bool withCursor = _tools.painting != Editing::Tools::HAND;

			if (_tools.magnification == -1) {
				const float PADDING = 32.0f;
				const float s = (splitter.first + PADDING) / (object()->width() * GBBASIC_TILE_SIZE);
				const float t = ((height - statusBarHeight) + PADDING) / (object()->height() * GBBASIC_TILE_SIZE);
				int m = 0;
				int n = 0;
				for (int i = 0; i < GBBASIC_COUNTOF(MAGNIFICATIONS); ++i) {
					if (s > MAGNIFICATIONS[i])
						m = i;
					if (t > MAGNIFICATIONS[i])
						n = i;
				}
				_tools.magnification = Math::min(m, n);
			}
			if (_tools.fineZooming) {
				_tools.magnification = Math::clamp(_tools.magnification, MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1]);
			} else {
				_tools.magnification = Math::clamp(_tools.magnification, 0, (int)GBBASIC_COUNTOF(MAGNIFICATIONS) - 1);
			}

			const ImVec2 content = ImGui::GetContentRegionAvail();
			float width_ = (float)(object()->width() * _tileSize.x);
			if (content.x / _tileSize.x > 1600 || content.y / _tileSize.y > 1600) {
				width_ *= 32;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 800 || content.y / _tileSize.y > 800) {
				width_ *= 16;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 400 || content.y / _tileSize.y > 400) {
				width_ *= 8;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 200 || content.y / _tileSize.y > 200) {
				width_ *= 4;
				_tools.scaled = true;
			} else if (content.x / _tileSize.x > 100 || content.y / _tileSize.y > 100) {
				width_ *= 2;
				_tools.scaled = true;
			}
			if (_tools.fineZooming) {
				width_ *= (float)_tools.magnification;
			} else {
				width_ *= (float)(MAGNIFICATIONS[_tools.magnification]);
			}

			if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
				_painting.set(
					Editing::map(
						rnd, ws,
						_project,
						object().get(), _tileSize,
						_project->preferencesPreviewPaletteBits(),
						width_,
						withCursor ? &cursor : nullptr, false,
						_selection.tiler.empty() ? nullptr : &_selection.tiler,
						_tools.showGrids && _tools.gridsVisible ? &_tools.gridUnit : nullptr,
						_tools.transparentBackbroundVisible,
						std::numeric_limits<int>::min(),
						_overlay,
						getPlt, getCol,
						getFlip,
						_tools.mouseActionButton
					)
				);
			} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
				const ImVec2 pos = ImGui::GetCursorPos();
				Editing::map(
					rnd, ws,
					_project,
					object().get(), _tileSize,
					_project->preferencesPreviewPaletteBits(),
					width_,
					nullptr, false,
					nullptr,
					nullptr,
					_tools.transparentBackbroundVisible,
					std::numeric_limits<int>::min(),
					nullptr,
					getPlt, getCol,
					getFlip,
					_tools.mouseActionButton
				);
				ImGui::SetCursorPos(pos);
				_painting.set(
					Editing::map(
						rnd, ws,
						_project,
						attributes().get(), _tileSize,
						false,
						width_,
						&cursor, _selection.tiler.empty(),
						_selection.tiler.empty() ? nullptr : &_selection.tiler,
						_tools.showGrids && _tools.gridsVisible ? &_tools.gridUnit : nullptr,
						false,
						std::numeric_limits<int>::min(),
						_overlay,
						nullptr, nullptr,
						nullptr,
						_tools.mouseActionButton
					)
				);
				fillCelTooltip(cursor);
				if (!_selection.hoveringMapTips.empty()) {
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(_selection.hoveringMapTips);
					}
				}
			}
			if (
				(_painting && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) ||
				_tools.painting == Editing::Tools::STAMP
			) {
				_cursor = cursor;
			}
			refreshStatus(wnd, rnd, ws, &cursor);

			if (allowMouseOperations) {
				if (_painting.down()) {
					if (_processors[_tools.painting].down)
						_processors[_tools.painting].down(rnd);
				}
				if (_painting && _painting.moved()) {
					if (_processors[_tools.painting].move)
						_processors[_tools.painting].move(rnd);
				}
				if (_painting.up()) {
					if (_processors[_tools.painting].up)
						_processors[_tools.painting].up(rnd);
				}
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
					if (_processors[_tools.painting].hover)
						_processors[_tools.painting].hover(rnd);
				}
			}

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			updateTools(
				wnd, rnd,
				ws,
				width, height,
				splitter, statusBarHeight, statusBarActived,
				false
			);
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	void updateAsImage(
		Window* wnd, Renderer* rnd,
		Workspace* ws,
		float width, float height,
		const Ref::Splitter &splitter, const float &statusBarHeight, bool &statusBarActived,
		bool allowMouseOperations
	) {
		if (entry()->refresh()) { // Refresh the outdated editable resources.
			_ref.fill(rnd, _project, true);

			entry()->touch();
		}
		Map::Tiles tiles;
		if (!object()->tiles(tiles) || !tiles.texture)
			entry()->touch(); // Ensure the runtime resources are ready.
		if (!object()->tiles(tiles) || !tiles.texture)
			return;

		_asImage.update();

		constexpr const int MAGNIFICATIONS[] = {
			1, 2, 4, 8
		};
		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav;
		if (_tools.scaled) {
			flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		}
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - statusBarHeight), false, flags);
		{
			if (textureAsImage()) {
				Image::Ptr &img = objectAsImage();
				Texture::Ptr &tex = textureAsImage();

				Editing::Brush cursor = _asImage.cursor;
				const bool withCursor = _tools.painting != Editing::Tools::HAND;

				const int pixelWidth = object()->width() * _tileSize.x;
				const int pixelHeight = object()->height() * _tileSize.y;

				if (_tools.magnification == -1) {
					const float PADDING = 32.0f;
					const float s = (splitter.first + PADDING) / pixelWidth;
					const float t = ((height - statusBarHeight) + PADDING) / pixelHeight;
					int m = 0;
					int n = 0;
					for (int i = 0; i < GBBASIC_COUNTOF(MAGNIFICATIONS); ++i) {
						if (s > MAGNIFICATIONS[i])
							m = i;
						if (t > MAGNIFICATIONS[i])
							n = i;
					}
					_tools.magnification = Math::min(m, n);
				}
				if (_tools.fineZooming) {
					_tools.magnification = Math::clamp(_tools.magnification, MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1]);
				} else {
					_tools.magnification = Math::clamp(_tools.magnification, 0, (int)GBBASIC_COUNTOF(MAGNIFICATIONS) - 1);
				}

				const ImVec2 content = ImGui::GetContentRegionAvail();
				float width_ = (float)pixelWidth;
				if (content.x / GBBASIC_TILE_SIZE > 1600 || content.y / GBBASIC_TILE_SIZE > 1600) {
					width_ *= 32;
					_tools.scaled = true;
				} else if (content.x / GBBASIC_TILE_SIZE > 800 || content.y / GBBASIC_TILE_SIZE > 800) {
					width_ *= 16;
					_tools.scaled = true;
				} else if (content.x / GBBASIC_TILE_SIZE > 400 || content.y / GBBASIC_TILE_SIZE > 400) {
					width_ *= 8;
					_tools.scaled = true;
				} else if (content.x / GBBASIC_TILE_SIZE > 200 || content.y / GBBASIC_TILE_SIZE > 200) {
					width_ *= 4;
					_tools.scaled = true;
				} else if (content.x / GBBASIC_TILE_SIZE > 100 || content.y / GBBASIC_TILE_SIZE > 100) {
					width_ *= 2;
					_tools.scaled = true;
				}
				if (_tools.fineZooming) {
					width_ *= (float)_tools.magnification;
				} else {
					width_ *= (float)(MAGNIFICATIONS[_tools.magnification]);
				}

				_asImage.painting.set(
					Editing::tiles(
						rnd, ws,
						img.get(), tex.get(),
						width_,
						withCursor ? &cursor : nullptr, false,
						_asImage.selection.brush.empty() ? nullptr : &_asImage.selection.brush,
						_asImage.overlay.texture ? _asImage.overlay.texture.get() : nullptr,
						&_asImage.tools.gridUnit, _tools.showGrids && _tools.gridsVisible,
						_tools.transparentBackbroundVisible,
						_tools.mouseActionButton,
						[&] (const Math::Vec2f &curPos, const Math::Vec2f &/* widgetSize */, float scale) -> void {
							ImGuiStyle &style = ImGui::GetStyle();
							ImDrawList* drawList = ImGui::GetWindowDrawList();

							for (const TileIssue &tileIssue : _tileIssues) {
								if (tileIssue.x == -1 || tileIssue.y == -1 || tileIssue.message.empty())
									continue;

								const ImVec2 minPos(
									(float)(curPos.x + tileIssue.x * GBBASIC_TILE_SIZE * scale),
									(float)(curPos.y + tileIssue.y * GBBASIC_TILE_SIZE * scale)
								);
								const ImVec2 maxPos(
									(float)(curPos.x + (tileIssue.x + 1) * GBBASIC_TILE_SIZE * scale + 1),
									(float)(curPos.y + (tileIssue.y + 1) * GBBASIC_TILE_SIZE * scale + 1)
								);
								drawList->AddRect(minPos, maxPos, 0xff0000ff);

								if (ImGui::IsItemHovered()) {
									const ImVec2 mousePos = ImGui::GetMousePos();
									const bool hovering = Math::intersects(Math::Recti((int)minPos.x, (int)minPos.y, (int)(minPos.x + maxPos.x), (int)(minPos.y + maxPos.y)), Math::Vec2i((int)mousePos.x, (int)mousePos.y));
									if (hovering) {
										VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

										ImGui::SetTooltip(tileIssue.message);
									}
								}
							}
						}
					)
				);
				if (
					_asImage.painting ||
					_tools.painting == Editing::Tools::STAMP
				) {
					_asImage.cursor = cursor;
				}
				refreshStatus(wnd, rnd, ws, &cursor);
			} else {
				_asImage.painting.set(false);
			}

			if (allowMouseOperations) {
				if (_asImage.painting.down()) {
					if (_asImage.processors[_tools.painting].down)
						_asImage.processors[_tools.painting].down(rnd);
				}
				if (_asImage.painting && _asImage.painting.moved()) {
					if (_asImage.processors[_tools.painting].move)
						_asImage.processors[_tools.painting].move(rnd);
				}
				if (_asImage.painting.up()) {
					if (_asImage.processors[_tools.painting].up)
						_asImage.processors[_tools.painting].up(rnd);
				}
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
					if (_asImage.processors[_tools.painting].hover)
						_asImage.processors[_tools.painting].hover(rnd);
				}
			}

			if (_asImage.binding.skipFrame) {
				_asImage.binding.skipFrame = false;
				ws->skipFrame(); // Skip a frame to avoid glitch.
			}

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			updateTools(
				wnd, rnd,
				ws,
				width, height,
				splitter, statusBarHeight, statusBarActived,
				true
			);
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
	void updateTools(
		Window* wnd, Renderer* rnd,
		Workspace* ws,
		float /* width */, float /* height */,
		const Ref::Splitter &splitter, const float &/* statusBarHeight */, bool &statusBarActived,
		bool asImage
	) {
		ImGuiStyle &style = ImGui::GetStyle();

		_ref.fill(rnd, _project, false);
		float xOffset = 0;
		const float spwidth = _ref.windowWidth(splitter.second);
		const float mwidth = Editing::Tools::measure(
			rnd, ws,
			&xOffset,
			nullptr,
			-1.0f
		);

		bool inputFieldFocused = false;
		bool inputFieldFocused_ = false;
		auto canUseShortcuts = [ws, this] (void) -> bool {
			return !_tools.inputFieldFocused && ws->canUseShortcuts() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
		};
		const bool editedAsImage = entry()->editedAsImage;
		bool editAsImage = entry()->editAsImage;
		bool localPaletteEnabled = entry()->localPaletteEnabled;

		constexpr const int MAGNIFICATIONS[] = {
			1, 2, 4, 8
		};

		ImGui::PushID("@Lyr");
		{
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			const char* items[] = {
				ws->theme()->windowMap_1_Graphics().c_str(),
				ws->theme()->windowMap_2_Attributes().c_str()
			};
			const char* tooltips[] = {
				ws->theme()->tooltip_LayerMap().c_str(),
				ws->theme()->tooltip_LayerAttributes().c_str()
			};

			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			ImGui::SetNextItemWidth(mwidth);
			if (ImGui::Combo("", &_tools.layer, items, GBBASIC_COUNTOF(items), tooltips)) {
				if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) {
#if GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED
					if (_tools.painting == Editing::Tools::PENCIL)
						_tools.painting = Editing::Tools::EYEDROPPER;
					else if (_tools.painting == Editing::Tools::STAMP)
						_tools.painting = Editing::Tools::EYEDROPPER;
#else /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
					if (_tools.painting == Editing::Tools::STAMP)
						_tools.painting = Editing::Tools::PENCIL;
#endif /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
				}
				activeLayerChanged(ws, _tools.layer);
				_ref.byteTooltip.clear();
				_status.clear();
				if (isEditingAsImage())
					refreshStatus(wnd, rnd, ws, &_asImage.cursor);
				else
					refreshStatus(wnd, rnd, ws, &_cursor);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				if (editAsImage)
					ImGui::SetTooltip(ws->theme()->tooltipMap_LayersWithEditingRecordsTips());
				else
					ImGui::SetTooltip(ws->theme()->tooltipMap_Layers());
			}
			ImGui::SameLine();
		}
		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			ImGui::NewLine();
			ImGui::NewLine(1);
			do {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(mwidth);
				if (ImGui::Checkbox(ws->theme()->windowMap_EditAsImage(), &editAsImage)) {
					const int ref = entry()->ref;
					const Project::Indices mapsRefToTheSameTiles = _project->getMapsRefToTiles(ref);
					if (editAsImage && mapsRefToTheSameTiles.size() > 1) {
						ImGui::MessagePopupBox::ConfirmedHandler confirm(
							[ws, this, editAsImage] (void) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								entry()->editAsImage = editAsImage;
								entry()->editedAsImage = true;
								editAsImageChanged(ws);

								_project->hasDirtyAsset(true);
							},
							nullptr
						);
						ImGui::MessagePopupBox::DeniedHandler deny(
							[ws] (void) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								// Do nothing.
							},
							nullptr
						);
						ws->messagePopupBox(
							ws->theme()->dialogPrompt_MultipleMapAssetsReferenceTheSourceTilesAssetContinueAnyway(),
							confirm,
							deny,
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							nullptr
						);
					} else if (editAsImage && !editedAsImage) {
						ImGui::MessagePopupBox::ConfirmedHandler confirm(
							[ws, this, editAsImage] (void) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								entry()->editAsImage = editAsImage;
								entry()->editedAsImage = true;
								editAsImageChanged(ws);

								PaletteAssets::Array localPalette = entry()->localPalette;
								touchLocalPalette(localPalette);
								Command* cmd = enqueue<Commands::Map::SetLocalPaletteEnabled>()
									->with(true, localPalette)
									->exec(object(), Variant((void*)entry()));
								_project->hasDirtyAsset(true);

								_refresh(cmd);

								localPaletteEnabledChanged(ws);
							},
							nullptr
						);
						ImGui::MessagePopupBox::DeniedHandler deny(
							[ws] (void) -> void {
								WORKSPACE_AUTO_CLOSE_POPUP(ws)

								// Do nothing.
							},
							nullptr
						);
						ws->messagePopupBox(
							ws->theme()->dialogPrompt_EditingAsAnImageMayRemvoeUnusedTilesContinueAnyway(),
							confirm,
							deny,
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							nullptr
						);
					} else {
						entry()->editAsImage = editAsImage;
						editAsImageChanged(ws);

						_project->hasDirtyAsset(true);
					}
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					if (editAsImage)
						ImGui::SetTooltip(ws->theme()->tooltipMap_EditAsImageWithEditingRecordsTips());
					else
						ImGui::SetTooltip(ws->theme()->tooltipMap_EditAsImage());
				}
				ImGui::SameLine();
			} while (false);

			ImGui::NewLine();
			Editing::Tools::separate(rnd, ws, spwidth);
			do {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(mwidth);
				localPaletteEnabled = entry()->localPaletteEnabled;
				if (ImGui::Checkbox(ws->theme()->windowMap_LocalPalette(), &localPaletteEnabled)) {
					PaletteAssets::Array localPalette = entry()->localPalette;
					touchLocalPalette(localPalette);
					Command* cmd = enqueue<Commands::Map::SetLocalPaletteEnabled>()
						->with(localPaletteEnabled, localPalette)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);

					localPaletteEnabledChanged(ws);

					_project->hasDirtyAsset(true);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltipMap_LocalPalette());
				}
				ImGui::SameLine();
			} while (false);
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			ImGui::NewLine();
			bool enabledLayer = entry()->hasAttributes;
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::SetNextItemWidth(mwidth);
			if (ImGui::Checkbox(ws->theme()->generic_Enabled(), &enabledLayer)) {
				Command* cmd = enqueue<Commands::Map::SwitchLayer>()
					->with(_setLayerEnabled)
					->with(enabledLayer)
					->with(_setLayer, _tools.layer)
					->exec(currentLayer());

				_refresh(cmd);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_WhetherToBuildThisLayer());
			}
			ImGui::SameLine();
		}
		ImGui::PopID();

		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER && entry()->localPaletteEnabled) {
			ImGui::NewLine();
			Colour newValue;
			bool colorPicked = false;
			Colour pickedColor;
			const bool changed = Editing::Tools::palette(
				rnd, ws,
				entry()->localPalette,
				newValue,
				&_tools.editingLocalPaletteColorGroup, &_tools.editingLocalPaletteColorIndex, &_tools.openLocalPaletteColorPicker,
				isEditingAsImage(), &colorPicked, &pickedColor,
				spwidth,
				ws->theme()->tooltip_LmbToPickRmbToChange().c_str()
			);
			if (changed) {
				PaletteAssets::Array localPalette = entry()->localPalette;
				const PaletteAssets::Entry &entry_ = localPalette[_tools.editingLocalPaletteColorGroup];
				const Indexed::Ptr &palette_ = entry_.data;
				palette_->set(_tools.editingLocalPaletteColorIndex, &newValue);

				Command* cmd = enqueue<Commands::Map::SetLocalPalette>()
					->with(localPalette)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);
			}
			if (colorPicked) {
				_asImage.ref.fromColor(pickedColor);
			}

			if (editAsImage) {
				ImGui::NewLine();
				ImGui::NewLine(1);
				if (_transfer.filled) {
					ImGui::BeginDisabled();
					Editing::Tools::clickable(rnd, ws, spwidth, ws->theme()->dialogPrompt_Refill().c_str());
					ImGui::EndDisabled();
				} else {
					if (Editing::Tools::clickable(rnd, ws, spwidth, ws->theme()->dialogPrompt_Refill().c_str()))
						refreshImageToTiled(ws);
				}
				ImGui::SameLine();
			}
		}

		ImGui::PushID("@Ref");
		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			const float unitSize = Editing::Tools::unitSize(rnd, ws, spwidth);

			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
			VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

			ImGui::NewLine();
			Editing::Tools::separate(rnd, ws, spwidth);
			const float curPosX = ImGui::GetCursorPosX();
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (editAsImage) {
				ImGui::BeginDisabled();
				ImGui::ImageButton(ws->theme()->iconRef()->pointer(rnd), ImVec2(unitSize, unitSize), ImColor(IM_COL32_WHITE), false, ws->theme()->tooltip_JumpToRefTiles().c_str());
				ImGui::EndDisabled();
			} else {
				if (ImGui::ImageButton(ws->theme()->iconRef()->pointer(rnd), ImVec2(unitSize, unitSize), ImColor(IM_COL32_WHITE), false, ws->theme()->tooltip_JumpToRefTiles().c_str())) {
					_ref.resolving = true;
				}
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ChangeRefTiles());
			}
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(style.ChildBorderSize, 0));
			ImGui::SameLine();
			const float prevWidth = ImGui::GetCursorPosX() - curPosX;
			const ImVec2 size(mwidth - prevWidth + xOffset, unitSize);
			if (ImGui::Button(_ref.refTilesText, size) && _ref.refIndex >= 0) {
				ws->category(Workspace::Categories::TILES);
				ws->changePage(wnd, rnd, _project, Workspace::Categories::TILES, _ref.refIndex);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_JumpToRefTiles());
			}
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			ImGui::NewLine();
			Editing::Tools::separate(rnd, ws, spwidth);
		}
		ImGui::PopID();

		auto bits = [&] (bool disabled) -> void {
			if (_tools.layer != ASSETS_MAP_ATTRIBUTES_LAYER || _status.cursor.position == Math::Vec2i(-1, -1))
				return;

#if GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED
			Byte cel = (Byte)currentLayer()->get(_status.cursor.position.x, _status.cursor.position.y);
			const std::string prompt = ws->theme()->status_Pos() + " " + Text::toString(_status.cursor.position.x) + "," + Text::toString(_status.cursor.position.y);
#else /* GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED */
			Byte cel = (Byte)(_ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_MAP_ATTRIBUTES_REF_WIDTH);
			const std::string &prompt = ws->theme()->dialogPrompt_InBits();
#endif /* GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED */
			const char* tooltipBits[8] = {
				ws->theme()->tooltipMap_Bit0().c_str(),
				ws->theme()->tooltipMap_Bit1().c_str(),
				ws->theme()->tooltipMap_Bit2().c_str(),
				ws->theme()->tooltipMap_Bit3().c_str(),
				ws->theme()->tooltipMap_Bit4().c_str(),
				ws->theme()->tooltipMap_Bit5().c_str(),
				ws->theme()->tooltipMap_Bit6().c_str(),
				ws->theme()->tooltipMap_Bit7().c_str()
			};
			const bool changed = Editing::Tools::maskable(
				rnd, ws,
				&cel,
				0b11111111,
				spwidth,
				disabled,
				prompt.c_str(),
				ws->theme()->tooltip_HighBits().c_str(), ws->theme()->tooltip_LowBits().c_str(),
				tooltipBits
			);
			if (changed) {
#if GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED
				enqueue<Commands::Map::ToggleMask>()
					->with(
						std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
						std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
						1
					)
					->with(
						Math::Vec2i(object()->width(), object()->height()),
						_status.cursor.position, cel,
						nullptr
					)
					->with(_setLayer, _tools.layer)
					->exec(currentLayer());
#endif /* GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED */

				const Image* img = ws->theme()->imageByte();
				const int objw = img->width() / _tileSize.x;
				const std::div_t div = std::div(cel, objw);
				_ref.attributesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.attributesCursor.size = _tileSize;
			}
		};

		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER && !asImage) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			ImGui::BeginChild("@Img", ImVec2(mwidth, spwidth), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);

			if (_ref.image && _ref.texture) {
				float refWidth = spwidth - style.ScrollbarSize - style.ChildBorderSize * 4;
				if (_ref.image->width() >= refWidth) {
					refWidth = (float)_ref.image->width();
				} else {
					if (_ref.image->width() > _ref.image->height())
						refWidth *= (float)_ref.image->width() / _ref.image->height();
				}
				Editing::Brush cursor = _ref.cursor;
				bool painting = false;
				if (_tools.painting == Editing::Tools::STAMP) {
					painting = Editing::tiles(
						rnd, ws,
						_ref.image.get(), _ref.texture.get(),
						refWidth,
						&cursor, true, std::move(_ref.stamp),
						nullptr,
						nullptr,
						&_ref.gridUnit, _ref.showGrids && _ref.gridsVisible,
						_ref.transparentBackbroundVisible,
						ImGuiMouseButton_Left,
						nullptr
					);
				} else {
					painting = Editing::tiles(
						rnd, ws,
						_ref.image.get(), _ref.texture.get(),
						refWidth,
						&cursor, true,
						nullptr,
						nullptr,
						&_ref.gridUnit, _ref.showGrids && _ref.gridsVisible,
						_ref.transparentBackbroundVisible,
						ImGuiMouseButton_Left,
						nullptr
					);
				}
				if (painting || _tools.painting == Editing::Tools::STAMP)
					_ref.cursor = cursor;
				if (painting)
					showRefStatus(wnd, rnd, ws, _ref.cursor);
			}

			statusBarActived |= ImGui::IsWindowFocused();

			ImGui::EndChild();
		} else if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) {
			const bool disabled = _tools.painting == Editing::Tools::STAMP;
			if (_project->preferencesUseByteMatrix()) {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::BeginChild("@Img", ImVec2(mwidth, spwidth), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);

				const Image* img = ws->theme()->imageByte();
				Texture* tex = ws->theme()->textureByte();
				if (img && tex) {
					float refWidth = spwidth - style.ScrollbarSize - style.ChildBorderSize * 4;
					if (img->width() >= refWidth) {
						refWidth = (float)img->width();
					} else {
						if (img->width() > img->height())
							refWidth *= (float)img->width() / img->height();
					}
					Editing::Brush cursor = _ref.attributesCursor;
					bool painting = false;
					if (_tools.painting == Editing::Tools::STAMP) {
						painting = Editing::tiles(
							rnd, ws,
							img, tex,
							refWidth,
							&cursor, true, std::move(_ref.stamp),
							nullptr,
							nullptr,
							&_ref.gridUnit, _ref.showGrids && _ref.gridsVisible,
							false,
							ImGuiMouseButton_Left,
							nullptr
						);
					} else {
						painting = Editing::tiles(
							rnd, ws,
							img, tex,
							refWidth,
							&cursor, true,
							nullptr,
							nullptr,
							&_ref.gridUnit, _ref.showGrids && _ref.gridsVisible,
							false,
							ImGuiMouseButton_Left,
							nullptr
						);
					}
					if (painting || _tools.painting == Editing::Tools::STAMP)
						_ref.attributesCursor = cursor;
					if (painting)
						showRefStatus(wnd, rnd, ws, _ref.attributesCursor);
				}

				statusBarActived |= ImGui::IsWindowFocused();

				ImGui::EndChild();
			} else {
				const UInt8 oldIdx = (UInt8)(_ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_MAP_ATTRIBUTES_REF_WIDTH);
				UInt8 idx = oldIdx;
				const bool painting = Editing::Tools::byte(
					rnd, ws,
					oldIdx,
					&idx,
					spwidth,
					&inputFieldFocused_,
					disabled,
					ws->theme()->dialogPrompt_Byte().c_str(),
					[ws, this] (UInt8 attrs) -> const char* {
						if (_ref.byteTooltip.empty()) {
							_ref.byteTooltip += ws->theme()->tooltipMap_Attributes();
							_ref.byteTooltip += "\n";

							const int pltBits = attrs & ((1 << GBBASIC_MAP_PALETTE_BIT2) | (1 << GBBASIC_MAP_PALETTE_BIT1) | (1 << GBBASIC_MAP_PALETTE_BIT0));
							const int bankBit = (attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001;
							const int hFlipBit = (attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001;
							const int vFlipBit = (attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001;
							const int priBit = (attrs >> GBBASIC_MAP_PRIORITY_BIT) & 0x00000001;
							_ref.byteTooltip += Text::format(ws->theme()->tooltipMap_BitsPalette(), { Text::toString(pltBits) });
							_ref.byteTooltip += Text::format(ws->theme()->tooltipMap_BitBank(), { Text::toString(bankBit) });
							_ref.byteTooltip += Text::format(ws->theme()->tooltipMap_BitHFlip(), { Text::toString(hFlipBit) });
							_ref.byteTooltip += Text::format(ws->theme()->tooltipMap_BitVFlip(), { Text::toString(vFlipBit) });
							_ref.byteTooltip += Text::format(ws->theme()->tooltipMap_BitPriority(), { Text::toString(priBit) });
						}

						return _ref.byteTooltip.c_str();
					}
				);
				if (painting) {
					const std::div_t div = std::div(idx, ASSETS_MAP_ATTRIBUTES_REF_WIDTH);
					_ref.attributesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
					_ref.byteTooltip.clear();
				}
				inputFieldFocused |= inputFieldFocused_;
			}

			ImGui::NewLine(1);
			bits(disabled);

			const char* tooltipOps[4] = {
				ws->theme()->tooltip_BitwiseSet().c_str(),
				ws->theme()->tooltip_BitwiseAnd().c_str(),
				ws->theme()->tooltip_BitwiseOr().c_str(),
				ws->theme()->tooltip_BitwiseXor().c_str()
			};
			ImGui::NewLine(1);
			Editing::Tools::bitwise(
				rnd, ws,
				&_tools.bitwiseOperations,
				spwidth,
				disabled,
				ws->theme()->tooltip_BitwiseOperations().c_str(), tooltipOps
			);
			ImGui::NewLine(1);
		}

		const unsigned mask = (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER || asImage) ?
			0xffffffff & ~(1 << Editing::Tools::STAMP) :
			0xffffffff;
		Editing::Tools::separate(rnd, ws, spwidth);
		if (editAsImage && !localPaletteEnabled) {
			Editing::Tools::colorable(
				rnd, ws,
				_asImage.ref.real,
				false,
				spwidth
			);
			ImGui::NewLine(1);
		}
		if (Editing::Tools::paintable(rnd, ws, &_tools.painting, -1.0f, canUseShortcuts(), isEditingAsImage(), mask)) {
			if (_tools.painting == Editing::Tools::STAMP) {
				_processors[Editing::Tools::STAMP] = Processor{
					std::bind(&EditorMapImpl::stampToolDown, this, std::placeholders::_1),
					nullptr,
					std::bind(&EditorMapImpl::stampToolUp, this, std::placeholders::_1),
					std::bind(&EditorMapImpl::stampToolHover, this, std::placeholders::_1)
				};
			} else {
				_ref.cursor.size = _tileSize;
				_ref.attributesCursor.size = _tileSize;
			}

			if (_tools.painting == Editing::Tools::STAMP) {
				_tools.bitwiseOperations = Editing::Tools::SET;
			}

			destroyOverlay();
		}

		Editing::Tools::separate(rnd, ws, spwidth);
		if (_tools.fineZooming) {
			Editing::Tools::magnifiable(
				rnd, ws,
				&_tools.magnification,
				MAGNIFICATIONS[0], MAGNIFICATIONS[GBBASIC_COUNTOF(MAGNIFICATIONS) - 1],
				spwidth, canUseShortcuts(),
				&inputFieldFocused_
			);
		} else {
			Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, -1.0f, canUseShortcuts());
		}

		switch (_tools.painting) {
		case Editing::Tools::PENCIL: // Fall through.
		case Editing::Tools::LINE: // Fall through.
		case Editing::Tools::BOX: // Fall through.
		case Editing::Tools::BOX_FILL: // Fall through.
		case Editing::Tools::ELLIPSE: // Fall through.
		case Editing::Tools::ELLIPSE_FILL:
			Editing::Tools::separate(rnd, ws, spwidth);
			Editing::Tools::weighable(rnd, ws, &_tools.weighting, -1.0f, canUseShortcuts());

			break;
		default: // Do nothing.
			break;
		}

		const bool flippable = (!editAsImage && !_tools.post) || (editAsImage && !_asImage.tools.post);
		if (flippable) {
			Editing::Tools::separate(rnd, ws, spwidth);
			Editing::Tools::RotationsAndFlippings flipping = Editing::Tools::INVALID;
			unsigned mask = 0xffffffff;
			Math::Recti frame;
			area(&frame);
			if (frame.width() == 1 && frame.height() == 1) {
				mask &= ~(1 << Editing::Tools::ROTATE_CLOCKWISE);
				mask &= ~(1 << Editing::Tools::ROTATE_ANTICLOCKWISE);
				mask &= ~(1 << Editing::Tools::HALF_TURN);
				mask &= ~(1 << Editing::Tools::FLIP_VERTICALLY);
				mask &= ~(1 << Editing::Tools::FLIP_HORIZONTALLY);
			} else if (frame.width() != frame.height()) {
				mask &= ~(1 << Editing::Tools::ROTATE_CLOCKWISE);
				mask &= ~(1 << Editing::Tools::ROTATE_ANTICLOCKWISE);
			}
			if (Editing::Tools::flippable(rnd, ws, &flipping, -1.0f, canUseShortcuts(), mask)) {
				flip(flipping);
			}
		}

		if (flippable && editAsImage) {
			Math::Recti sel;
			const int size = _asImage.selection.area(sel);
			if (size) {
				const Math::Recti minRepeat(std::numeric_limits<Int8>::min(), std::numeric_limits<Int8>::min(), 0, 0);
				const Math::Recti maxRepeat(0, 0, std::numeric_limits<Int8>::max(), std::numeric_limits<Int8>::max());
				Math::Recti repeat;
				if (
					Editing::Tools::repeatable(
						rnd, ws,
						&_asImage.tools.repeat,
						minRepeat, maxRepeat,
						spwidth,
						&inputFieldFocused_,
						false,
						ws->theme()->dialogPrompt_Repeat().c_str()
					)
				) {
					ImGui::WaitingPopupBox::TimeoutHandler timeout(
						[rnd, ws, this, sel] (void) -> void {
							_asImage.repeat(rnd, sel, _asImage.tools.repeat);

							_asImage.tools.repeat = Math::Recti();

							ws->popupBox(nullptr);
						},
						nullptr
					);
					ws->waitingPopupBox(
						true, ws->theme()->dialogPrompt_Filling(),
						true, timeout,
						true
					);
				}
			}
		}

		Editing::Tools::separate(rnd, ws, spwidth);
		if (
			Editing::Tools::namable(
				rnd, ws,
				entry()->name, _tools.namableText,
				spwidth,
				nullptr, &inputFieldFocused_,
				ws->theme()->dialogPrompt_Name().c_str()
			)
		) {
			if (_project->canRenameMap(_index, _tools.namableText, nullptr)) {
				Command* cmd = enqueue<Commands::Map::SetName>()
					->with(_tools.namableText)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);
			} else {
				_tools.namableText = entry()->name;

				warn(ws, ws->theme()->warning_MapNameIsAlreadyInUse(), true, true);
			}
		}
		inputFieldFocused |= inputFieldFocused_;

		Editing::Tools::separate(rnd, ws, spwidth);
		const int width_ = object()->width();
		const int height_ = object()->height();
		if (editAsImage)
			ImGui::BeginDisabled();
		if (
			Editing::Tools::resizable(
				rnd, ws,
				width_, height_,
				&_size.width, &_size.height,
				GBBASIC_MAP_MAX_WIDTH, GBBASIC_MAP_MAX_HEIGHT, GBBASIC_MAP_MAX_AREA_SIZE,
				spwidth,
				&inputFieldFocused_,
				ws->theme()->dialogPrompt_Size_InTiles().c_str(),
				ws->theme()->warning_MapSizeOutOfBounds().c_str(),
				std::bind(&EditorMapImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, true)
			)
		) {
			_size.width = Math::clamp(_size.width, 1, GBBASIC_MAP_MAX_WIDTH);
			_size.height = Math::clamp(_size.height, 1, GBBASIC_MAP_MAX_HEIGHT);
			if (_size.width != width_ || _size.height != height_)
				post(RESIZE, _size.width, _size.height);
		}
		if (editAsImage)
			ImGui::EndDisabled();
		inputFieldFocused |= inputFieldFocused_;

		Editing::Tools::separate(rnd, ws, spwidth);
		bool optimize = entry()->optimize;
		if (
			Editing::Tools::togglable(
				rnd, ws,
				&optimize,
				spwidth,
				ws->theme()->dialogPrompt_Optimize().c_str(),
				ws->theme()->tooltipMap_Optimize().c_str()
			)
		) {
			Command* cmd = enqueue<Commands::Map::SetOptimized>()
				->with(optimize)
				->exec(object(), Variant((void*)entry()));

			_refresh(cmd);
		}

		if (!editAsImage) {
			Editing::Tools::separate(rnd, ws, spwidth);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Button(ws->theme()->windowMap_CreateScene(), ImVec2(mwidth, 0))) {
				ws->addScenePageFrom(wnd, rnd, _index);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltipMap_CreateASceneReferencingThisMapPage());
			}
		}

		_tools.inputFieldFocused = inputFieldFocused;

		statusBarActived |= ImGui::IsWindowFocused();
	}

	bool shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
		if (_ref.resolving) {
			const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
			if (esc.pressed())
				_ref.resolving = false;

			return true;
		}

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		if (_ref.gridsVisible == caps.pressed()) {
			_ref.showGrids = !caps.pressed();
			_ref.gridsVisible = !caps.pressed();
		}
		_ref.transparentBackbroundVisible = num.pressed();
		if (_tools.gridsVisible == caps.pressed()) {
			_tools.showGrids = !caps.pressed();
			_tools.gridsVisible = !caps.pressed();

			_project->preferencesShowGrids(_tools.showGrids);
			_project->hasDirtyEditor(true);
		}
		_tools.transparentBackbroundVisible = num.pressed();

		if (_tools.inputFieldFocused || !ws->canUseShortcuts()) {
			if (_tools.post) {
				_tools.post();
				_tools.post = nullptr;
			}
			if (_asImage.tools.post) {
				_asImage.tools.post();
				_asImage.tools.post = nullptr;
			}

			return true;
		}

		const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true, false);
		const Editing::Shortcut alt(SDL_SCANCODE_UNKNOWN, false, false, true);
		const bool editAsImage = entry()->editAsImage;
		PostHandler* post = nullptr;
		Editing::Tools::PaintableTools* postType = nullptr;
		if (editAsImage) {
			post = &_tools.post;
			postType = &_tools.postType;
		} else {
			post = &_asImage.tools.post;
			postType = &_asImage.tools.postType;
		}
		if (shift.pressed(false) && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (!(*post)) {
				const Editing::Tools::PaintableTools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::HAND;

				*post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
						_tools.painting = prevTool;
				};
				*postType = Editing::Tools::HAND;
			}

			return true;
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
			if (!(*post)) {
				const Editing::Tools::PaintableTools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::HAND;

				_tools.mouseActionButton = ImGuiMouseButton_Middle;

				*post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
						_tools.painting = prevTool;
						_tools.mouseActionButton = ImGuiMouseButton_Left;
					}
				};
				*postType = Editing::Tools::HAND;
			}

			return true;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
			if (*post && *postType == Editing::Tools::HAND) {
				(*post)();
				*post = nullptr;

				return false;
			}
		} else if (shift.released()) {
			if (*post && *postType == Editing::Tools::HAND) {
				(*post)();
				*post = nullptr;
			}
		}
		if (alt.pressed(false)) {
			if (!(*post)) {
				const Editing::Tools::PaintableTools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::EYEDROPPER;

				*post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
						_tools.painting = prevTool;
				};
				*postType = Editing::Tools::EYEDROPPER;
			}

			return true;
		} else if (alt.released()) {
			if (*post && *postType == Editing::Tools::EYEDROPPER) {
				(*post)();
				*post = nullptr;
			}
		}

		const Editing::Shortcut num1(SDL_SCANCODE_1, true, true);
		const Editing::Shortcut num2(SDL_SCANCODE_2, true, true);
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (num1.pressed())
				changeLayer(wnd, rnd, ws, 0);
			else if (num2.pressed())
				changeLayer(wnd, rnd, ws, 1);
		}

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed()) {
			_selection.clear();
			_asImage.selection.clear();
		}

#if defined GBBASIC_OS_APPLE
		const Editing::Shortcut pgUp(SDL_SCANCODE_PAGEUP, false, false, false, false, false, true);
		const Editing::Shortcut pgDown(SDL_SCANCODE_PAGEDOWN, false, false, false, false, false, true);
#else /* GBBASIC_OS_APPLE */
		const Editing::Shortcut pgUp(SDL_SCANCODE_PAGEUP, true);
		const Editing::Shortcut pgDown(SDL_SCANCODE_PAGEDOWN, true);
#endif /* GBBASIC_OS_APPLE */
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (pgUp.pressed()) {
				int index = _index - 1;
				if (index < 0)
					index = _project->mapPageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->mapPageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, index);
			}
		}

		return true;
	}

	void context(Window* wnd, Renderer* rnd, Workspace* ws) {
		switch (_tools.layer) {
		case ASSETS_MAP_GRAPHICS_LAYER: {
				ImGuiStyle &style = ImGui::GetStyle();

				if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup("@Ctx");

					ws->bubble(nullptr);

					_cursor = _status.cursor;
					if (_selection.tiler.empty())
						_selection.tiler = _cursor;
				}

				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				if (ImGui::BeginPopup("@Ctx")) {
					if (ImGui::MenuItem(ws->theme()->menu_Cut())) {
						cut();
					}
					if (ImGui::MenuItem(ws->theme()->menu_Copy())) {
						copy();
					}
					if (ImGui::MenuItem(ws->theme()->menu_Paste(), nullptr, nullptr, pastable())) {
						paste();

						ws->bubble(ws->theme()->dialogPrompt_ClickToPut(), nullptr);
					}
					if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
						del(false);
					}
					if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
						post(Editable::SELECT_ALL);
					}
					ImGui::Separator();
					if (Platform::isClipboardImageSupported()) {
						if (ImGui::MenuItem(ws->theme()->menu_CopyAsImage())) {
							exportToImageObject(wnd, rnd, ws);
						}
					}
					if (ImGui::MenuItem(ws->theme()->menu_SaveAsImageFile(), nullptr, nullptr)) {
						exportToImageFile(wnd, rnd, ws);
					}

					ImGui::EndPopup();
				}
			}

			break;
		case ASSETS_MAP_ATTRIBUTES_LAYER: {
				ImGuiStyle &style = ImGui::GetStyle();

				if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup("@Ctx");

					ws->bubble(nullptr);

					_cursor = _status.cursor;
					if (_selection.tiler.empty())
						_selection.tiler = _cursor;
				}

				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				if (ImGui::BeginPopup("@Ctx")) {
					auto assignPalette = [] (int &attrs, int data) -> void {
						attrs &=
							(1 << GBBASIC_MAP_BANK_BIT) |
							(1 << GBBASIC_MAP_UNUSED_A_BIT) |
							(1 << GBBASIC_MAP_HFLIP_BIT) |
							(1 << GBBASIC_MAP_VFLIP_BIT) |
							(1 << GBBASIC_MAP_PRIORITY_BIT);
						attrs |= data & ((1 << GBBASIC_MAP_PALETTE_BIT2) | (1 << GBBASIC_MAP_PALETTE_BIT1) | (1 << GBBASIC_MAP_PALETTE_BIT0));
					};
					auto assignBit = [] (int &attrs, int bit, bool on) -> void {
						attrs &= ~((Byte)1 << bit);
						if (on)
							attrs |= (Byte)1 << bit;
					};

					int attrs = currentLayer()->get(_cursor.position.x, _cursor.position.y);
					const int pltBits = attrs & ((1 << GBBASIC_MAP_PALETTE_BIT2) | (1 << GBBASIC_MAP_PALETTE_BIT1) | (1 << GBBASIC_MAP_PALETTE_BIT0));
					const int bankBit = (attrs >> GBBASIC_MAP_BANK_BIT) & 0x00000001;
					const int hFlipBit = (attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001;
					const int vFlipBit = (attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001;
					const int priBit = (attrs >> GBBASIC_MAP_PRIORITY_BIT) & 0x00000001;

					bool reassign = false;
					if (ImGui::BeginMenu(ws->theme()->menu_Palette())) {
						if (ImGui::MenuItem("0", nullptr, pltBits == 0)) {
							assignPalette(attrs, 0);
							reassign = true;
						}
						if (ImGui::MenuItem("1", nullptr, pltBits == 1)) {
							assignPalette(attrs, 1);
							reassign = true;
						}
						if (ImGui::MenuItem("2", nullptr, pltBits == 2)) {
							assignPalette(attrs, 2);
							reassign = true;
						}
						if (ImGui::MenuItem("3", nullptr, pltBits == 3)) {
							assignPalette(attrs, 3);
							reassign = true;
						}
						if (ImGui::MenuItem("4", nullptr, pltBits == 4)) {
							assignPalette(attrs, 4);
							reassign = true;
						}
						if (ImGui::MenuItem("5", nullptr, pltBits == 5)) {
							assignPalette(attrs, 5);
							reassign = true;
						}
						if (ImGui::MenuItem("6", nullptr, pltBits == 6)) {
							assignPalette(attrs, 6);
							reassign = true;
						}
						if (ImGui::MenuItem("7", nullptr, pltBits == 7)) {
							assignPalette(attrs, 7);
							reassign = true;
						}

						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu(ws->theme()->menu_Bank())) {
						if (ImGui::MenuItem("0", nullptr, !bankBit)) {
							assignBit(attrs, GBBASIC_MAP_BANK_BIT, true);
							reassign = true;
						}
						if (ImGui::MenuItem("1", nullptr, !!bankBit)) {
							assignBit(attrs, GBBASIC_MAP_BANK_BIT, false);
							reassign = true;
						}

						ImGui::EndMenu();
					}
					if (ImGui::MenuItem(ws->theme()->menu_Hflip(), nullptr, !!hFlipBit)) {
						assignBit(attrs, GBBASIC_MAP_HFLIP_BIT, !hFlipBit);
						reassign = true;
					}
					if (ImGui::MenuItem(ws->theme()->menu_Vflip(), nullptr, !!vFlipBit)) {
						assignBit(attrs, GBBASIC_MAP_VFLIP_BIT, !vFlipBit);
						reassign = true;
					}
					if (ImGui::BeginMenu(ws->theme()->menu_Priority())) {
						if (ImGui::MenuItem("0", nullptr, !priBit)) {
							assignBit(attrs, GBBASIC_MAP_PRIORITY_BIT, true);
							reassign = true;
						}
						if (ImGui::MenuItem("1", nullptr, !!priBit)) {
							assignBit(attrs, GBBASIC_MAP_PRIORITY_BIT, false);
							reassign = true;
						}

						ImGui::EndMenu();
					}
					if (reassign) {
						enqueue<Commands::Map::Pencil>()
							->with(
								std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
								std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
								_tools.weighting + 1
							)
							->with(_tools.bitwiseOperations)
							->with(
								Math::Vec2i(object()->width(), object()->height()),
								_cursor.position, attrs,
								nullptr
							)
							->with(_setLayer, _tools.layer)
							->exec(currentLayer());
					}
					ImGui::Separator();

					if (ImGui::MenuItem(ws->theme()->menu_Cut())) {
						cut();
					}
					if (ImGui::MenuItem(ws->theme()->menu_Copy())) {
						copy();
					}
					if (ImGui::MenuItem(ws->theme()->menu_Paste(), nullptr, nullptr, pastable())) {
						paste();

						ws->bubble(ws->theme()->dialogPrompt_ClickToPut(), nullptr);
					}
					if (ImGui::MenuItem(ws->theme()->menu_Delete())) {
						del(false);
					}
					if (ImGui::MenuItem(ws->theme()->menu_SelectAll())) {
						post(Editable::SELECT_ALL);
					}

					ImGui::EndPopup();
				}
			}

			break;
		}
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws, const Editing::Tiler* cursor /* nullable */) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		if (cursor && _status.cursor != *cursor) {
			_status.cursor = *cursor;

			if (_status.cursor.position.x == -1 || _status.cursor.position.y == -1) {
				_status.text = " " + ws->theme()->status_Sz();
				_status.text += " ";
				_status.text += Text::toString(object()->width());
				_status.text += "x";
				_status.text += Text::toString(object()->height());
			} else {
				_status.text = " " + ws->theme()->status_Pos();
				_status.text += " ";
				_status.text += Text::toString(_status.cursor.position.x);
				_status.text += ",";
				_status.text += Text::toString(_status.cursor.position.y);

				_status.text += " ";

				_status.text += ws->theme()->status_Tl();
				_status.text += " ";
				const int idx = currentLayer()->get(_status.cursor.position.x, _status.cursor.position.y);
				_status.text += Text::toString(idx);
			}
		}
		if (!_estimated.filled) {
			_estimated.text = " " + Text::toScaledBytes(entry()->estimateFinalByteCount());
			_estimated.filled = true;
		}
	}
	void refreshStatus(Window*, Renderer*, Workspace* ws, const Editing::Brush* cursor /* nullable */) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		if (cursor && _asImage.status.cursor != *cursor) {
			_asImage.status.cursor = *cursor;

			if (_asImage.status.cursor.position.x == -1 || _asImage.status.cursor.position.y == -1) {
				_asImage.status.text = " " + ws->theme()->status_Sz();
				_asImage.status.text += " ";
				_asImage.status.text += Text::toString(object()->width());
				_asImage.status.text += "x";
				_asImage.status.text += Text::toString(object()->height());

				_asImage.status.text += " " + ws->theme()->status_Tls();
				_asImage.status.text += " ";
				_asImage.status.text += Text::toString(object()->width());
				_asImage.status.text += "x";
				_asImage.status.text += Text::toString(object()->height());
			} else {
				_asImage.status.text = " " + ws->theme()->status_Pos();
				_asImage.status.text += " ";
				_asImage.status.text += Text::toString(_asImage.status.cursor.position.x);
				_asImage.status.text += ",";
				_asImage.status.text += Text::toString(_asImage.status.cursor.position.y);

				const int x = _asImage.status.cursor.position.x / GBBASIC_TILE_SIZE;
				const int y = _asImage.status.cursor.position.y / GBBASIC_TILE_SIZE;
				_asImage.status.text += " " + ws->theme()->status_Tl();
				_asImage.status.text += " ";
				_asImage.status.text += Text::toString(x);
				_asImage.status.text += ",";
				_asImage.status.text += Text::toString(y);
			}
		}
		if (!_estimated.filled) {
			_estimated.text = " " + Text::toScaledBytes(entry()->estimateFinalByteCount());
			_estimated.filled = true;
		}
	}
	void refreshStatus(Window*, Renderer*, Workspace* ws) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		if (!_estimated.filled) {
			_estimated.text = " " + Text::toScaledBytes(entry()->estimateFinalByteCount());
			_estimated.filled = true;
		}
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height, bool actived) {
		ImGuiStyle &style = ImGui::GetStyle();

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
		if (ImGui::Button("<", ImVec2(0, height))) {
			int index = _index - 1;
			if (index < 0)
				index = _project->mapPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_PreviousPage());
		}
		ImGui::SameLine();
		if (ImGui::Button(_page.text.c_str(), ImVec2(0, height))) {
			ImGui::OpenPopup("@Pg");
		}
		ImGui::SameLine();
		if (ImGui::Button(">", ImVec2(0, height))) {
			int index = _index + 1;
			if (index >= _project->mapPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		if (entry()->ref != -1) {
			ImGui::SameLine();
			if (_tools.isPlaying) {
				if (ImGui::ImageButton(ws->theme()->iconStopPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipMap_StopTesting().c_str())) {
					_tools.stopMapTesting();
				}
			} else {
				if (ws->running()) {
					ImGui::BeginDisabled();
					ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipMap_TestMap().c_str());
					ImGui::EndDisabled();
				} else {
					if (ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipMap_TestMap().c_str())) {
						_tools.playMapTesting();
					}
				}
			}
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		if (entry()->ref == -1) {
			ImGui::TextUnformatted(" ");
			ImGui::SameLine();
			ImGui::TextUnformatted(ws->theme()->status_RefIsMissing());
		} else {
			if (isEditingAsImage())
				ImGui::TextUnformatted(_asImage.status.text);
			else
				ImGui::TextUnformatted(_status.text);
			ImGui::SameLine();
			ImGui::TextUnformatted(_estimated.text);
			ImGui::SameLine();
			do {
				_status.transferring = _transfer.generated.working();
				const Marker<bool>::Values transferring = _status.transferring.values();
				float width_ = 0.0f;
				const float wndWidth = ImGui::GetWindowWidth();
				ImGui::SetCursorPosX(wndWidth - _statusWidth);
				if (wndWidth >= 430) {
					if (transferring.first) {
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
						ImGui::ImageButton(ws->theme()->iconWorking()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Compacting().c_str());
						ImGui::PopStyleColor(2);
						width_ += ImGui::GetItemRectSize().x;
						ImGui::SameLine();
					} else if (!_transfer.filled) {
						if (ImGui::ImageButton(ws->theme()->iconRefresh()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipActor_Refresh().c_str())) {
							refreshImageToTiled(ws);
						}
						width_ += ImGui::GetItemRectSize().x;
						ImGui::SameLine();
					} else {
						if (_status.info.empty()) {
							const int w = object()->width() / GBBASIC_TILE_SIZE;
							const int h = object()->height() / GBBASIC_TILE_SIZE;
							int l = 1;
							if (entry()->hasAttributes)
								++l;
							_status.info = Text::format(
								ws->theme()->tooltipMap_Info(),
								{
									Text::toString(w * h),
									Text::toString(l),
									l <= 1 ? ws->theme()->tooltipMap_InfoLayer() : ws->theme()->tooltipMap_InfoLayers()
								}
							);
						}

						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
						ImGui::ImageButton(ws->theme()->iconInfo()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, _status.info.empty() ? nullptr : _status.info.c_str());
						ImGui::PopStyleColor(2);
						width_ += ImGui::GetItemRectSize().x;
						ImGui::SameLine();
					}
				}
				if (!_warnings.empty()) {
					const ImVec4 col = ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->warningColor);
					ImGui::PushStyleColor(ImGuiCol_Text, col);

					if (ImGui::ImageButton(ws->theme()->iconWarning()->pointer(rnd), ImVec2(13, 13), col, false, ws->theme()->tooltip_Warning().c_str())) {
						ImGui::OpenPopupTooltip("Wrn");
					}
					if (ImGui::PopupTooltip("Wrn", _warnings.text, ws->theme()->generic_Dismiss().c_str())) {
						_tileIssues.clear();
						_warnings.clear();
					}
					width_ += ImGui::GetItemRectSize().x;
					ImGui::SameLine();

					ImGui::PopStyleColor();
				}
				if (ImGui::ImageButton(ws->theme()->iconImport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Import().c_str())) {
					ImGui::OpenPopup("@Imp");
				}
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->iconExport()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Export().c_str())) {
					ImGui::OpenPopup("@Xpt");
				}
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->iconViews()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_View().c_str())) {
					ImGui::OpenPopup("@Views");
				}
				width_ += ImGui::GetItemRectSize().x;
				width_ += style.FramePadding.x;
				_statusWidth = width_;
			} while (false);
		}
		if (!actived && !EDITOR_ALWAYS_COLORED_STATUS_BAR_ENABLED) {
			ImGui::PopStyleColor(3);
		}

		statusBarMenu(wnd, rnd, ws);
	}
	void statusBarMenu(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		ImGuiStyle &style = ImGui::GetStyle();

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		// Paging.
		if (ImGui::BeginPopup("@Pg")) {
			const Text::Array &assetNames = ws->getMapPageNames();
			const int n = _project->mapPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(ws->theme()->menu_DataSequence())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					std::string txt = Unicode::fromOs(osstr);
					txt = Parsers::removeComments(txt);
					Map::Ptr newData = nullptr;
					if (!entry()->parseDataSequence(newData, txt, _tools.layer)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::Map::Import>()
						->with(newData, _tools.layer)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()), attributes());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					Map::Ptr newObj = nullptr;
					bool hasAttrib = false;
					Map::Ptr newAttrib = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, hasAttrib, newAttrib, txt, status)) {
						switch (status) {
						case BaseAssets::Entry::ParsingStatuses::NOT_A_MULTIPLE_OF_8x8:
							ws->messagePopupBox(ws->theme()->dialogPrompt_ResourceSizeIsNotAMultipleOf8x8(), nullptr, nullptr, nullptr);

							break;
						case BaseAssets::Entry::ParsingStatuses::OUT_OF_BOUNDS:
							ws->messagePopupBox(ws->theme()->dialogPrompt_ResourceSizeOutOfBounds(), nullptr, nullptr, nullptr);

							break;
						default:
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						break;
					}

					Command* cmd = enqueue<Commands::Map::Import>()
						->with(newObj, hasAttrib, newAttrib)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()), attributes());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_JsonFile())) {
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
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readString(txt)) {
						file->close(); FileMonitor::unuse(path);

						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					Map::Ptr newObj = nullptr;
					bool hasAttrib = false;
					Map::Ptr newAttrib = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, hasAttrib, newAttrib, txt, status)) {
						switch (status) {
						case BaseAssets::Entry::ParsingStatuses::NOT_A_MULTIPLE_OF_8x8:
							ws->messagePopupBox(ws->theme()->dialogPrompt_ResourceSizeIsNotAMultipleOf8x8(), nullptr, nullptr, nullptr);

							break;
						case BaseAssets::Entry::ParsingStatuses::OUT_OF_BOUNDS:
							ws->messagePopupBox(ws->theme()->dialogPrompt_ResourceSizeOutOfBounds(), nullptr, nullptr, nullptr);

							break;
						default:
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						break;
					}

					Command* cmd = enqueue<Commands::Map::Import>()
						->with(newObj, hasAttrib, newAttrib)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()), attributes());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_Image())) {
				importFromImageObject(wnd, rnd, ws);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_ImageFile())) {
				importFromImageFile(wnd, rnd, ws);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::MenuItem(ws->theme()->menu_Code())) {
				auto serialize = [ws, this] (void) -> void {
					std::string txt;
					if (!entry()->serializeBasic(txt, _index))
						return;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
				};

				if (isEditingAsImage()) {
					do {
						if (_transfer.filled) {
							serialize();

							break;
						}

						ImGui::WaitingPopupBox::TimeoutHandler timeout(
							[ws, this, serialize] (void) -> void {
								transferImageToTiled(ws);

								_transfer.generated.wait();

								serialize();

								ws->popupBox(nullptr);
							},
							nullptr
						);
						ws->waitingPopupBox(
							true, ws->theme()->dialogPrompt_Transferring(),
							true, timeout,
							true
						);
					} while (false);
				} else {
					serialize();
				}
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::BeginMenu(ws->theme()->menu_DataSequence())) {
				if (ImGui::MenuItem(ws->theme()->menu_Hex())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 16, _tools.layer))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Dec())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 10, _tools.layer))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Bin())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 2, _tools.layer))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}

				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					std::string txt;
					if (!entry()->serializeJson(txt, true))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_JsonFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-map.json" : Text::sanitizeFilename(entry()->name) + ".json",
						GBBASIC_JSON_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "json")
						path += ".json";

					std::string txt;
					if (!entry()->serializeJson(txt, false))
						break;

					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeString(txt);
					file->close();

#if !defined GBBASIC_OS_HTML
					FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
					std::string path_ = fileInfo->parentPath();
					path_ = Unicode::toOs(path_);
					Platform::browse(path_.c_str());
#endif /* Platform macro. */

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (Platform::isClipboardImageSupported()) {
				if (ImGui::MenuItem(ws->theme()->menu_Image())) {
					exportToImageObject(wnd, rnd, ws);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
				}
			}
			if (ImGui::MenuItem(ws->theme()->menu_ImageFile())) {
				exportToImageFile(wnd, rnd, ws);
			}

			ImGui::EndPopup();
		}

		// Views.
		if (ImGui::BeginPopup("@Views")) {
			switch (_tools.layer) {
			case ASSETS_MAP_ATTRIBUTES_LAYER:
				if (ImGui::MenuItem(ws->theme()->menu_UseByteMatrix(), nullptr, &_project->preferencesUseByteMatrix())) {
					_project->hasDirtyEditor(true);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_UseByteMatrixViewOnTheToolbar());
				}
				ImGui::Separator();

				break;
			}

			if (ImGui::MenuItem(ws->theme()->menu_PaletteBits(), nullptr, &_project->preferencesPreviewPaletteBits())) {
				entry()->cleanup();
				ws->skipFrame(); // Skip a frame to avoid glitch.

				_project->hasDirtyEditor(true);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_PreviewPaletteBitsForColoredOnly());
			}
			ImGui::Separator();

			const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
			if (caps.pressed()) {
				ImGui::MenuItem(ws->theme()->menu_Grids(), "Caps", true);
			} else {
				if (ImGui::MenuItem(ws->theme()->menu_Grids(), "Caps", &_tools.showGrids)) {
					_ref.showGrids = _tools.showGrids;

					_project->preferencesShowGrids(_tools.showGrids);
					_project->hasDirtyEditor(true);
				}
			}
			if (ImGui::MenuItem(ws->theme()->menu_FineZooming(), nullptr, &_tools.fineZooming)) {
				_tools.magnification = -1;

				_project->preferencesFineZooming(_tools.fineZooming);

				_project->hasDirtyEditor(true);
			}

			ImGui::EndPopup();
		}
	}
	void showRefStatus(Window*, Renderer*, Workspace* ws, const Editing::Brush &cursor) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (!_ref.image)
			return;
		if (cursor.position.x == -1 || cursor.position.y == -1)
			return;

		const Int w = _ref.image->width() / _tileSize.x;
		const Int x = cursor.position.x / _tileSize.x;
		const Int y = cursor.position.y / _tileSize.y;
		const Int idx = x + y * w;

		std::string text;

		text = " " + ws->theme()->status_Pos();
		text += " ";
		text += Text::toString(x);
		text += ",";
		text += Text::toString(y);

		text += " ";

		text += ws->theme()->status_Tl();
		text += " ";
		text += Text::toString(idx);
		text += "(0x";
		text += Text::toHex(idx, 2, '0', true);
		text += ")";

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(text);
	}

	void resolveRef(Window*, Renderer* rnd, Workspace* ws, float width, float windowHeight) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float btnWidth = 100.0f;
		const float btnHeight = 72.0f;
		const ImVec2 btnPos((width - btnWidth) * 0.5f, (windowHeight - btnHeight) * 0.5f);

		if (_ref.refIndex < 0)
			_ref.refIndex = 0;
		const int n = _project->tilesPageCount();
		ImGui::PushID("@P");
		{
			ImGui::SetCursorPos(btnPos - ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 4));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->dialogPrompt_Tiles());

			ImGui::SetCursorPos(btnPos - ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 2));
			if (n == 0) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(ws->theme()->dialogPrompt_NothingAvailable());
			} else {
				if (ImGui::ImageButton(ws->theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					if (--_ref.refIndex < 0)
						_ref.refIndex = 0;
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(btnWidth - (13 + style.FramePadding.x * 2) * 2);
				if (ImGui::DragInt("", &_ref.refIndex, 1.0f, 0, n - 1, "%d")) {
					// Do nothing.
				}
				if (ImGui::IsItemHovered()) {
					const Text::Array &assetNames = ws->getTilesPageNames();
					const std::string &pg = assetNames[_ref.refIndex];

					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(pg);
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					if (++_ref.refIndex > n - 1)
						_ref.refIndex = n - 1;
				}
			}
		}
		ImGui::PopID();

		ImGui::SetCursorPos(btnPos);
		const bool valid = _ref.refIndex >= 0 && _ref.refIndex < _project->tilesPageCount();
		if (valid) {
			if (ImGui::Button(ws->theme()->generic_Resolve(), ImVec2(btnWidth, btnHeight))) {
				_ref.refreshRefTilesText(ws->theme());
				_ref.fill(rnd, _project, true);

				entry()->ref = _ref.refIndex;
				entry()->cleanup();
				entry()->touch();

				_project->preferencesMapRef(_ref.refIndex);

				_project->hasDirtyAsset(true);

				entry()->increaseRevision();

				_ref.resolving = false;
			}
		} else {
			ImGui::BeginDisabled();
			ImGui::Button(ws->theme()->generic_Resolve(), ImVec2(btnWidth, btnHeight));
			ImGui::EndDisabled();
		}
		if (entry()->ref != -1) {
			ImGui::NewLine(3);
			ImGui::SetCursorPosX(btnPos.x);
			if (ImGui::Button(ws->theme()->generic_Cancel(), ImVec2(btnWidth, 0))) {
				_ref.resolving = false;
			}
		}
	}

	void changeLayer(Window* wnd, Renderer* rnd, Workspace* ws, int layer) {
		if (_tools.layer == layer)
			return;

		_tools.layer = layer;
		if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) {
#if GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED
			if (_tools.painting == Editing::Tools::PENCIL)
				_tools.painting = Editing::Tools::EYEDROPPER;
			else if (_tools.painting == Editing::Tools::STAMP)
				_tools.painting = Editing::Tools::EYEDROPPER;
#else /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
			if (_tools.painting == Editing::Tools::STAMP)
				_tools.painting = Editing::Tools::PENCIL;
#endif /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
		}
		_ref.byteTooltip.clear();
		_status.clear();
		if (isEditingAsImage())
			refreshStatus(wnd, rnd, ws, &_asImage.cursor);
		else
			refreshStatus(wnd, rnd, ws, &_cursor);
	}
	void toggleLayer(Window*, Renderer*, Workspace*, int layer, bool enabled) {
		if (layer == ASSETS_MAP_GRAPHICS_LAYER) {
			return;
		} else /* if (layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			entry()->hasAttributes = enabled;
		}
	}

	void warn(Workspace* ws, const std::string &msg, bool add, bool isWarning) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "Map editor: ";
				msg_ += msg;
				if (msg.back() != '.')
					msg_ += '.';
				if (isWarning)
					ws->warn(msg_.c_str());
				else
					ws->error(msg_.c_str());
			}
		} else {
			_warnings.remove(msg);
		}
	}

	void modified(void) {
		const bool editAsImage = entry()->editAsImage;
		if (!editAsImage)
			entry()->image = nullptr; // Invalidate that image.
	}

	void createOverlay(void) {
		_overlay = std::bind(&EditorMapImpl::getOverlayCel, this, std::placeholders::_1);
	}
	void destroyOverlay(void) {
		if (_overlay)
			_overlay = nullptr;
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		entry()->increaseRevision();

		modified();

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd) {
		const bool refillName =
			Command::is<Commands::Map::SetName>(cmd);
		const bool recalcSize =
			Command::is<Commands::Map::Resize>(cmd) ||
			Command::is<Commands::Map::Import>(cmd);
		const bool refreshLayers =
			Command::is<Commands::Map::SwitchLayer>(cmd);
		const bool refreshPalette =
			Command::is<Commands::Map::Pencil>(cmd) ||
			Command::is<Commands::Map::Line>(cmd) ||
			Command::is<Commands::Map::Box>(cmd) ||
			Command::is<Commands::Map::BoxFill>(cmd) ||
			Command::is<Commands::Map::Ellipse>(cmd) ||
			Command::is<Commands::Map::EllipseFill>(cmd) ||
			Command::is<Commands::Map::Fill>(cmd) ||
			Command::is<Commands::Map::Replace>(cmd) ||
			Command::is<Commands::Map::Stamp>(cmd) ||
			Command::is<Commands::Map::Rotate>(cmd) ||
			Command::is<Commands::Map::Flip>(cmd) ||
			Command::is<Commands::Map::Cut>(cmd) ||
			Command::is<Commands::Map::Paste>(cmd) ||
			Command::is<Commands::Map::Delete>(cmd) ||
			Command::is<Commands::Map::ToggleMask>(cmd);
		const bool refreshAllPalette =
			Command::is<Commands::Map::Resize>(cmd) ||
			Command::is<Commands::Map::Import>(cmd);
		const bool refreshAllPaletteWithDebounce =
			Command::is<Commands::Map::SetLocalPaletteEnabled>(cmd) ||
			Command::is<Commands::Map::SetLocalPalette>(cmd);

		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearMapPageNames();
		}
		if (recalcSize) {
			_size.width = object()->width();
			_size.height = object()->height();
			_status.info.clear();
			_estimated.filled = false;
			entry()->cleanup(); // Clean up the outdated editable and runtime resources. Will recalculate the tile size after this.
			ws->skipFrame(); // Skip a frame to avoid glitch.
		}
		if (refreshLayers) {
			_status.info.clear();
		}
		if (refreshPalette) {
			if (_project->preferencesPreviewPaletteBits()) {
				const Commands::Map::PaintableBase::Paint* cmdPaint = Command::as<Commands::Map::PaintableBase::Paint>(cmd);
				const Commands::Map::PaintableBase::Blit* cmdBlit = Command::as<Commands::Map::PaintableBase::Blit>(cmd);
				if (cmdPaint) {
					cmdPaint->populated(
						[this] (const Math::Vec2i &pos, const Editing::Dot &) -> void {
							const Math::Recti area = Math::Recti::byXYWH(pos.x, pos.y, 1, 1);
							entry()->cleanup(area);
						}
					);
				} else if (cmdBlit) {
					cmdBlit->populated(
						[this] (const Math::Vec2i &pos, const Editing::Dot &) -> void {
							const Math::Recti area = Math::Recti::byXYWH(pos.x, pos.y, 1, 1);
							entry()->cleanup(area);
						}
					);
				}
			};
		}
		if (refreshAllPalette && _project->preferencesPreviewPaletteBits()) {
			entry()->cleanup();
			ws->skipFrame(); // Skip a frame to avoid glitch.
		}
		if (refreshAllPaletteWithDebounce && _project->preferencesPreviewPaletteBits()) {
			_tools.debounceLocalPaletteColorChanged.modified();
		}
	}

	void flip(Editing::Tools::RotationsAndFlippings f) {
		if (isEditingAsImage()) {
			_asImage.flip(f);

			return;
		}

		if (!_binding.getCel || !_binding.setCel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			enqueue<Commands::Map::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(1)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			enqueue<Commands::Map::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(-1)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::HALF_TURN:
			enqueue<Commands::Map::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(2)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			enqueue<Commands::Map::Flip>()
				->with(_binding.getCel, _binding.setCel)
				->with(1)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			enqueue<Commands::Map::Flip>()
				->with(_binding.getCel, _binding.setCel)
				->with(0)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		default: // Do nothing.
			break;
		}
	}

	void handToolDown(Renderer*) {
		_tools.mousePos = ImGui::GetMousePos();
	}
	void handToolMove(Renderer*) {
		const ImVec2 pos = ImGui::GetMousePos();
		_tools.mouseDiff = pos - _tools.mousePos;
		_tools.mousePos = pos;
	}
	void handToolUp(Renderer*) {
		_tools.mouseDiff = ImVec2(0, 0);
		_tools.mouseActionButton = ImGuiMouseButton_Left;
	}
	void handToolHover(Renderer*) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		if (_tools.mouseDiff.x != 0) {
			ImGui::SetScrollX(ImGui::GetScrollX() - _tools.mouseDiff.x);
			_tools.mouseDiff.x = 0;
		}
		if (_tools.mouseDiff.y != 0) {
			ImGui::SetScrollY(ImGui::GetScrollY() - _tools.mouseDiff.y);
			_tools.mouseDiff.y = 0;
		}
	}

	void eyedropperToolDown(Renderer*) {
		const int idx = currentLayer()->get(_cursor.position.x, _cursor.position.y);
		if (idx == Map::INVALID())
			return;

		if (_tileSize.x <= 0)
			return;

		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			const std::div_t div = std::div(idx, (int)(_ref.image->width() / _tileSize.x));
			_ref.cursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
			_ref.cursor.size = _tileSize;
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			const std::div_t div = std::div(idx, ASSETS_MAP_ATTRIBUTES_REF_WIDTH);
			_ref.attributesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
			_ref.attributesCursor.size = _tileSize;
		}
	}
	void eyedropperToolMove(Renderer*) {
		const int idx = currentLayer()->get(_cursor.position.x, _cursor.position.y);
		if (idx == Map::INVALID())
			return;

		if (_tileSize.x <= 0)
			return;

		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			const std::div_t div = std::div(idx, (int)(_ref.image->width() / _tileSize.x));
			_ref.cursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
			_ref.cursor.size = _tileSize;
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			const std::div_t div = std::div(idx, ASSETS_MAP_ATTRIBUTES_REF_WIDTH);
			_ref.attributesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
			_ref.attributesCursor.size = _tileSize;
		}
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		T* cmd = enqueue<T>();
		cmd->with(_tools.bitwiseOperations);
		cmd->with(_setLayer, _tools.layer);
		Commands::Map::PaintableBase::Paint::Getter getter = std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		Commands::Map::PaintableBase::Paint::Setter setter = std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		cmd->with(getter, setter);

		int idx = 0;
		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			idx = _ref.cursor.unit().x + _ref.cursor.unit().y * (_ref.image->width() / _tileSize.x);
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			idx = _ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_MAP_ATTRIBUTES_REF_WIDTH;
		}

		cmd->with(
			Math::Vec2i(object()->width(), object()->height()),
			_selection.tiler.empty() ? nullptr : &sel,
			_cursor.position, idx
		);

		cmd->exec(currentLayer());
	}
	void paintbucketToolUp(Renderer* rnd) {
		Math::Recti sel;
		if (_selection.area(sel) && !Math::intersects(sel, _cursor.position))
			_selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Map::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Map::Fill>(rnd, sel);
	}

	void lassoToolDown(Renderer*) {
		_selection.mouse = ImGui::GetMousePos();

		_selection.initial = _cursor.position;
		_selection.tiler.position = _cursor.position;
		_selection.tiler.size = Math::Vec2i(1, 1);
	}
	void lassoToolMove(Renderer*) {
		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);
		_selection.tiler.position = Math::Vec2i(rect.xMin(), rect.yMin());
		_selection.tiler.size = Math::Vec2i(rect.width(), rect.height());
	}
	void lassoToolUp(Renderer*) {
		const ImVec2 diff = ImGui::GetMousePos() - _selection.mouse;

		if (std::abs(diff.x) < Math::EPSILON<float>() && std::abs(diff.y) < Math::EPSILON<float>()) {
			if (_selection.idle == 0) {
				_selection.clear();
				++_selection.idle;
			} else {
				_selection.idle = 0;
			}
		}
	}

	void stampToolDown(Renderer*) {
		_selection.clear();
	}
	void stampToolUp(Renderer*) {
		if (!_binding.getCel || !_binding.setCel)
			return;

		Math::Recti cursor;
		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			cursor = Math::Recti::byXYWH(
				_ref.cursor.position.x / _tileSize.x, _ref.cursor.position.y / _tileSize.y,
				_ref.cursor.size.x / _tileSize.x, _ref.cursor.size.y / _tileSize.y
			);
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			cursor = Math::Recti::byXYWH(
				_ref.attributesCursor.position.x / _tileSize.x, _ref.attributesCursor.position.y / _tileSize.y,
				_ref.attributesCursor.size.x / _tileSize.x, _ref.attributesCursor.size.y / _tileSize.y
			);
		}
		const Math::Recti area = Math::Recti::byXYWH(0, 0, cursor.width(), cursor.height());
		Editing::Dot::Array dots;
		for (int j = cursor.yMin(); j <= cursor.yMax(); ++j) {
			for (int i = cursor.xMin(); i <= cursor.xMax(); ++i) {
				const int idx = i + j * (_ref.image->width() / _tileSize.x);
				Editing::Dot dot;
				dot.indexed = idx;
				dots.push_back(dot);
			}
		}
		dots.shrink_to_fit();

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		enqueue<Commands::Map::Stamp>()
			->with(_tools.bitwiseOperations)
			->with(_binding.getCel, _binding.setCel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		destroyOverlay();
	}
	void stampToolHover(Renderer*) {
		if (_overlay && !_painting.moved())
			return;

		Math::Recti cursor;
		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			cursor = Math::Recti::byXYWH(
				_ref.cursor.position.x / _tileSize.x, _ref.cursor.position.y / _tileSize.y,
				_ref.cursor.size.x / _tileSize.x, _ref.cursor.size.y / _tileSize.y
			);
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			cursor = Math::Recti::byXYWH(
				_ref.attributesCursor.position.x / _tileSize.x, _ref.attributesCursor.position.y / _tileSize.y,
				_ref.attributesCursor.size.x / _tileSize.x, _ref.attributesCursor.size.y / _tileSize.y
			);
		}
		const Math::Recti area = Math::Recti::byXYWH(0, 0, cursor.width(), cursor.height());
		Editing::Dot::Array dots;
		for (int j = cursor.yMin(); j <= cursor.yMax(); ++j) {
			for (int i = cursor.xMin(); i <= cursor.xMax(); ++i) {
				const int idx = i + j * (_ref.image->width() / _tileSize.x);
				Editing::Dot dot;
				dot.indexed = idx;
				dots.push_back(dot);
			}
		}
		dots.shrink_to_fit();

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_overlay = std::bind(
			[] (const Math::Recti area, const Editing::Dot::Array dots, const Math::Vec2i offset, const Math::Vec2i &pos) -> int {
				const Math::Vec2i position = pos - offset;
				if (!Math::intersects(area, position))
					return Map::INVALID();

				return dots[position.x + position.y * area.width()].indexed;
			},
			area, dots, Math::Vec2i(x, y), std::placeholders::_1
		);
	}

	void stampToolUp_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots, Editing::Tools::PaintableTools prevTool) {
		if (!_binding.getCel || !_binding.setCel)
			return;

		_selection.clear();

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		enqueue<Commands::Map::Paste>()
			->with(_tools.bitwiseOperations)
			->with(_binding.getCel, _binding.setCel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		_processors[Editing::Tools::STAMP] = Processor{
			nullptr,
			nullptr,
			std::bind(&EditorMapImpl::stampToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapImpl::stampToolHover, this, std::placeholders::_1)
		};

		destroyOverlay();

		_tools.painting = prevTool;
	}
	void stampToolHover_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots) {
		if (_overlay && !_painting.moved())
			return;

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		_overlay = std::bind(
			[] (const Math::Recti area, const Editing::Dot::Array dots, const Math::Vec2i offset, const Math::Vec2i &pos) -> int {
				const Math::Vec2i position = pos - offset;
				if (!Math::intersects(area, position))
					return Map::INVALID();

				return dots[position.x + position.y * area.width()].indexed;
			},
			area, dots, Math::Vec2i(x, y), std::placeholders::_1
		);
	}

	template<typename T> void paintToolDown(Renderer* rnd) {
		_selection.clear();

		enqueue<T>()
			->with(
				std::bind(&EditorMapImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				_tools.weighting + 1
			)
			->with(_tools.bitwiseOperations)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		createOverlay();

		paintToolMove<T>(rnd);
	}
	template<typename T> void paintToolMove(Renderer*) {
		if (_commands->empty())
			return;

		Command* back = _commands->back();
		GBBASIC_ASSERT(back->type() == T::TYPE());
		T* cmd = Command::as<T>(back);
		int idx = 0;
		if (_tools.layer == ASSETS_MAP_GRAPHICS_LAYER) {
			idx = _ref.cursor.unit().x + _ref.cursor.unit().y * (_ref.image->width() / _tileSize.x);
		} else /* if (_tools.layer == ASSETS_MAP_ATTRIBUTES_LAYER) */ {
			idx = _ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_MAP_ATTRIBUTES_REF_WIDTH;
		}

		cmd->with(
			Math::Vec2i(object()->width(), object()->height()),
			_cursor.position, idx,
			nullptr
		);
	}
	void paintToolUp(Renderer*) {
		if (!_commands->empty())
			_commands->back()->redo(currentLayer());

		destroyOverlay();
	}

	MapAssets::Entry* entry(void) const {
		MapAssets::Entry* entry = _project->getMap(_index);

		return entry;
	}
	Map::Ptr &object(void) const {
		return entry()->data;
	}
	Map::Ptr &attributes(void) const {
		return entry()->attributes;
	}
	Map::Ptr &currentLayer(void) const {
		Map::Ptr &layer = _tools.layer == ASSETS_MAP_GRAPHICS_LAYER ? object() : attributes();

		return layer;
	}
	bool isEditingAsImage(void) const {
		const bool result = entry() && entry()->editAsImage;

		return result;
	}
	void touchAsImage(void) {
		if (!entry())
			return;

		if (!_transfer.image) {
			const int ref = entry()->ref;
			const TilesAssets::Entry* entry_ = _project->getTiles(ref);
			Image::Ptr img(Image::create());

			if (_project->preferencesPreviewPaletteBits()) {
				PaletteAssets::Array palette;
				touchLocalPalette(palette);
				if (!MapAssets::serializeImage(&palette, *entry_, *entry(), img.get()))
					return;
			} else {
				if (!MapAssets::serializeImage(nullptr, *entry_, *entry(), img.get()))
					return;
			}

			_transfer.image = img;
		}

		if (!_transfer.texture || !_transfer.texture->pointer(_transfer.renderer)) {
			_transfer.texture = Texture::Ptr(Texture::create());
			_transfer.texture->fromImage(_transfer.renderer, Texture::STREAMING, _transfer.image.get(), Texture::NEAREST);
			_transfer.texture->blend(Texture::BLEND);
		}
	}
	Image::Ptr &objectAsImage(void) {
		touchAsImage();

		return _transfer.image;
	}
	Texture::Ptr &textureAsImage(void) {
		touchAsImage();

		return _transfer.texture;
	}

	int getOverlayCel(const Math::Vec2i &pos) const {
		Command* back = _commands->back();
		switch (back->type()) {
		case Commands::Map::Pencil::TYPE(): // Fall through.
		case Commands::Map::Line::TYPE(): // Fall through.
		case Commands::Map::Box::TYPE(): // Fall through.
		case Commands::Map::BoxFill::TYPE(): // Fall through.
		case Commands::Map::Ellipse::TYPE(): // Fall through.
		case Commands::Map::EllipseFill::TYPE(): {
				Commands::Map::PaintableBase::Paint* cmd = Command::as<Commands::Map::PaintableBase::Paint>(back);
				const Editing::Point key(pos);
				const Editing::Dot* dot = cmd->find(key);
				if (!dot)
					return Map::INVALID();

				return dot->indexed;
			}
		default:
			return Map::INVALID();
		}
	}

	int getCels(Renderer*, const Math::Recti* area /* nullable */, Editing::Dots &dots) const {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, object()->width(), object()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				const int idx = currentLayer()->get(i, j);
				dots.indexed[k] = idx;
				++k;
				++result;
			}
		}

		return result;
	}
	int setCels(Renderer*, const Math::Recti* area /* nullable */, const Editing::Dots &dots) {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, object()->width(), object()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				const int idx = dots.indexed[k];
				currentLayer()->set(i, j, idx);
				++k;
				++result;
			}
		}

		return result;
	}
	bool getCel(Renderer*, const Math::Vec2i &pos, Editing::Dot &dot) const {
		const int idx = currentLayer()->get(pos.x, pos.y);
		if (idx == Map::INVALID())
			return false;

		dot.indexed = idx;

		return true;
	}
	bool setCel(Renderer*, const Math::Vec2i &pos, const Editing::Dot &dot) {
		return currentLayer()->set(pos.x, pos.y, dot.indexed);
	}

	bool importFromImage(Window* wnd, Renderer* rnd, Workspace* ws, Image::Ptr img, bool allowFlip, bool fillLocalPalette, bool createNew) {
		struct Error {
			typedef std::vector<Error> Array;

			bool isWarning = false;
			std::string message;

			Error() {
			}
			Error(bool warn, const std::string &msg) : isWarning(warn), message(msg) {
			}
		};

		if ((img->width() % GBBASIC_TILE_SIZE) != 0 || (img->height() % GBBASIC_TILE_SIZE) != 0) {
			ws->messagePopupBox(ws->theme()->dialogPrompt_ResourceSizeIsNotAMultipleOf8x8(), nullptr, nullptr, nullptr);

			return false;
		}
		if (img->width() * img->height() > GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE * GBBASIC_MAP_MAX_AREA_SIZE) {
			ws->messagePopupBox(ws->theme()->dialogPrompt_ResourceSizeOutOfBounds(), nullptr, nullptr, nullptr);

			return false;
		}
		Texture::Ptr attribtex(ws->theme()->textureByte(), [] (Texture*) -> void { /* Do nothing. */ });
		PaletteAssets::Array palette;
		for (int i = 0; i < _project->assets()->palette.count(); ++i) {
			const PaletteAssets::Entry* pal = _project->assets()->palette.get(i);
			palette.push_back(*pal);
		}
		TilesAssets::Entry tiles(rnd, _project->paletteGetter());
		MapAssets::Entry map("", _project->tilesGetter(), attribtex);
		Error::Array errors;
		const bool loaded = MapAssets::parseImage(
			palette, tiles, map,
			img.get(),
			allowFlip, fillLocalPalette, false, false, false,
			_project->paletteGetter(), _project->tilesGetter(), _project->tilesPageCount(),
			nullptr,
			nullptr, [&errors] (const char* msg, bool isWarning) -> void {
				errors.push_back(Error(isWarning, msg));
			}
		);
		_tileIssues.clear();
		_warnings.clear();
		if (!loaded) {
			if (!errors.empty()) {
				for (const Error &err : errors) {
					const std::string &msg = err.message;
					warn(ws, msg, true, err.isWarning);
				}
			}

			return false;
		}

		int ref = -1;
		if (createNew) {
			if (_project->tilesPageCount() >= GBBASIC_ASSET_MAX_PAGE_COUNT) {
				ws->bubble(ws->theme()->dialogPrompt_CannotAddMorePage(), nullptr);

				return false;
			}

			std::string str;
			TilesAssets::Entry default_(rnd, _project->paletteGetter());
			default_.toString(str, nullptr);

			_project->addTilesPage(str, true);
			_project->hasDirtyAsset(true);

			ws->pageAdded(wnd, rnd, _project, Workspace::Categories::TILES);

			ref = _project->tilesPageCount() - 1;

			_ref.refIndex = (int)ref;
			_ref.refreshRefTilesText(ws->theme());

			entry()->ref = ref;
			entry()->cleanup();
		} else {
			ref = entry()->ref;
		}
		if (ref == -1) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}
		_refresh(enqueue<Commands::Map::BeginGroup>()
			->with("Import")
			->exec(object(), Variant((void*)entry()), attributes()));
		{
			TilesAssets::Entry* entry_ = _project->getTiles(ref);
			if (!entry_) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			const Variant ret = ws->touchTilesEditor(wnd, rnd, _project, ref, entry_)
				->post(Editable::IMPORT, (void*)&tiles);
			const unsigned refCmdId = (unsigned)(Variant::Long)ret;

			_refresh(enqueue<Commands::Map::Reference>()
				->with(_project, (unsigned)AssetsBundle::Categories::TILES, ref, refCmdId)
				->exec(object(), Variant((void*)entry()), attributes()));

			if (fillLocalPalette) {
				PaletteAssets::Array localPalette = palette;
				const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
				if ((int)localPalette.size() != n)
					localPalette.resize(n);

				Command* cmd = enqueue<Commands::Map::SetLocalPalette>()
					->with(localPalette)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);
			}

			if (object()->width() != map.data->width() || object()->height() != map.data->height()) {
				Command* cmd = enqueue<Commands::Map::Resize>()
					->with(Math::Vec2i(map.data->width(), map.data->height()))
					->with(_setLayer, _tools.layer)
					->exec(object(), Variant((void*)entry()), attributes());

				_refresh(cmd);

				_selection.clear();
			}

			const bool hasAttributes = entry()->hasAttributes || map.hasAttributes;
			Command* cmd = enqueue<Commands::Map::Import>()
				->with(map.data, hasAttributes, map.attributes)
				->with(_setLayer, _tools.layer)
				->exec(object(), Variant((void*)entry()), attributes());

			_refresh(cmd);
		}
		_refresh(enqueue<Commands::Map::EndGroup>()
			->with("Import")
			->exec(object(), Variant((void*)entry()), attributes()));

		ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

		return true;
	}
	bool importFromImageObject(Window* wnd, Renderer* rnd, Workspace* ws) {
		auto import = [wnd, rnd, ws, this] (Image::Ptr img, bool allowFlip, bool fillLocalPalette, bool createNew) -> bool {
			return importFromImage(wnd, rnd, ws, img, allowFlip, fillLocalPalette, createNew);
		};
		auto resolve = [rnd, ws, this, import] (Image::Ptr img, bool createNew) -> void {
			const Text::Array filter = GBBASIC_IMAGE_FILE_FILTER;
			ImGui::MapResolverPopupBox::ConfirmedHandler confirm(
				[ws, import, img, createNew] (const int* /* index */, const char* /* path */, bool allowFlip, bool fillLocalPalette) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					import(img, allowFlip, fillLocalPalette, createNew);
				},
				nullptr
			);
			ImGui::MapResolverPopupBox::CanceledHandler cancel(
				[ws] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)
				},
				nullptr
			);
			ws->showExternalMapBrowser(
				rnd,
				ws->theme()->generic_Path().c_str(),
				filter,
				false, false,
				confirm,
				cancel,
				nullptr,
				nullptr
			);
		};
		auto ask = [ws, resolve] (Image::Ptr img) -> void {
			ImGui::MessagePopupBox::ConfirmedHandler confirm(
				[ws, resolve, img] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					resolve(img, true);
				},
				nullptr
			);
			ImGui::MessagePopupBox::DeniedHandler deny(
				[ws, resolve, img] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					resolve(img, false);
				},
				nullptr
			);
			ImGui::MessagePopupBox::CanceledHandler cancel(
				[ws] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)
				},
				nullptr
			);
			ws->messagePopupBox(
				ws->theme()->dialogPrompt_MultipleMapAssetsReferenceTheSourceTilesAssetCreateANewTilesAssetPageForImporting(),
				confirm,
				deny,
				cancel,
				nullptr,
				nullptr,
				nullptr,
				ws->theme()->tooltipMap_CreateANewTilesAssetPage().c_str(),
				ws->theme()->tooltipMap_UpdateTheRefTilesAssetPage().c_str(),
				nullptr
			);
		};

		if (!Platform::hasClipboardImage()) {
			ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

			return false;
		}

		Image::Ptr img(Image::create());
		if (!Platform::getClipboardImage(img.get())) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		const int ref = entry()->ref;
		TilesAssets::Entry* entry_ = _project->getTiles(ref);
		if (!entry_) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		const Project::Indices mapsRefToTheSameTiles = _project->getMapsRefToTiles(ref);
		if (mapsRefToTheSameTiles.size() > 1) {
			ask(img);

			return false;
		}

		resolve(img, false);

		return true;
	}
	bool importFromImageFile(Window* wnd, Renderer* rnd, Workspace* ws) {
		auto import = [wnd, rnd, ws, this] (const char* path, bool allowFlip, bool fillLocalPalette, bool createNew) -> bool {
			if (!path)
				return false;

			std::string path_ = path;
			Path::uniform(path_);
			if (path_.empty())
				return false;

			Bytes::Ptr bytes(Bytes::create());
			File::Ptr file(File::create());
			if (!file->open(path_.c_str(), Stream::READ)) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			if (!file->readBytes(bytes.get())) {
				file->close(); FileMonitor::unuse(path_);

				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}
			file->close(); FileMonitor::unuse(path_);

			Image::Ptr img(Image::create());
			if (!img->fromBytes(bytes.get())) {
				ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

				return false;
			}

			return importFromImage(wnd, rnd, ws, img, allowFlip, fillLocalPalette, createNew);
		};
		auto resolve = [rnd, ws, this, import] (bool createNew) -> void {
			const Text::Array filter = GBBASIC_IMAGE_FILE_FILTER;
			ImGui::MapResolverPopupBox::ConfirmedHandler confirm(
				[ws, import, createNew] (const int* /* index */, const char* path, bool allowFlip, bool fillLocalPalette) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					import(path, allowFlip, fillLocalPalette, createNew);
				},
				nullptr
			);
			ImGui::MapResolverPopupBox::CanceledHandler cancel(
				[ws] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)
				},
				nullptr
			);
			ws->showExternalMapBrowser(
				rnd,
				ws->theme()->generic_Path().c_str(),
				filter,
				false, true,
				confirm,
				cancel,
				nullptr,
				nullptr
			);
		};
		auto ask = [ws, resolve] (void) -> void {
			ImGui::MessagePopupBox::ConfirmedHandler confirm(
				[ws, resolve] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					resolve(true);
				},
				nullptr
			);
			ImGui::MessagePopupBox::DeniedHandler deny(
				[ws, resolve] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)

					resolve(false);
				},
				nullptr
			);
			ImGui::MessagePopupBox::CanceledHandler cancel(
				[ws] (void) -> void {
					WORKSPACE_AUTO_CLOSE_POPUP(ws)
				},
				nullptr
			);
			ws->messagePopupBox(
				ws->theme()->dialogPrompt_MultipleMapAssetsReferenceTheSourceTilesAssetCreateANewTilesAssetPageForImporting(),
				confirm,
				deny,
				cancel,
				nullptr,
				nullptr,
				nullptr,
				ws->theme()->tooltipMap_CreateANewTilesAssetPage().c_str(),
				ws->theme()->tooltipMap_UpdateTheRefTilesAssetPage().c_str(),
				nullptr
			);
		};

		const int ref = entry()->ref;
		TilesAssets::Entry* entry_ = _project->getTiles(ref);
		if (!entry_) {
			ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

			return false;
		}

		const Project::Indices mapsRefToTheSameTiles = _project->getMapsRefToTiles(ref);
		if (mapsRefToTheSameTiles.size() > 1) {
			ask();

			return false;
		}

		resolve(false);

		return true;
	}
	bool exportToImageObject(Window*, Renderer*, Workspace* ws) const {
		const int ref = entry()->ref;
		const TilesAssets::Entry* entry_ = _project->getTiles(ref);
		Image::Ptr img(Image::create());
		if (_project->preferencesPreviewPaletteBits()) {
			PaletteAssets::Array palette;
			touchLocalPalette(palette);
			if (!MapAssets::serializeImage(&palette, *entry_, *entry(), img.get()))
				return false;
		} else {
			if (!MapAssets::serializeImage(nullptr, *entry_, *entry(), img.get()))
				return false;
		}

		Platform::setClipboardImage(img.get());

		ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);

		return true;
	}
	bool exportToImageFile(Window*, Renderer*, Workspace* ws) const {
		pfd::save_file save(
			ws->theme()->generic_SaveTo(),
			entry()->name.empty() ? "gbbasic-map.png" : Text::sanitizeFilename(entry()->name) + ".png",
			GBBASIC_IMAGE_FILE_FILTER
		);
		std::string path = save.result();
		Path::uniform(path);
		if (path.empty())
			return false;
		const Text::Array exts = { "png", "jpg", "bmp", "tga" };
		std::string ext;
		Path::split(path, nullptr, &ext, nullptr);
		Text::toLowerCase(ext);
		if (ext.empty() || std::find(exts.begin(), exts.end(), ext) == exts.end())
			path += ".png";
		Path::split(path, nullptr, &ext, nullptr);
		Text::toLowerCase(ext);
		const char* y = ext.c_str();

		const int ref = entry()->ref;
		const TilesAssets::Entry* entry_ = _project->getTiles(ref);
		Image::Ptr img(Image::create());
		if (_project->preferencesPreviewPaletteBits()) {
			PaletteAssets::Array palette;
			touchLocalPalette(palette);
			if (!MapAssets::serializeImage(&palette, *entry_, *entry(), img.get()))
				return false;
		} else {
			if (!MapAssets::serializeImage(nullptr, *entry_, *entry(), img.get()))
				return false;
		}

		Bytes::Ptr bytes(Bytes::create());
		if (!img->toBytes(bytes.get(), y))
			return false;

		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::WRITE))
			return false;
		file->writeBytes(bytes.get());
		file->close();

#if !defined GBBASIC_OS_HTML
		FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
		std::string path_ = fileInfo->parentPath();
		path_ = Unicode::toOs(path_);
		Platform::browse(path_.c_str());
#endif /* Platform macro. */

		ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);

		return true;
	}

	void refreshImageToTiled(Workspace *ws) {
		if (_transfer.filled)
			return;

		if (!isEditingAsImage()) {
			_transfer.filled = true;

			return;
		}

		ImGui::WaitingPopupBox::TimeoutHandler timeout(
			[ws, this] (void) -> void {
				transferImageToTiled(ws);

				_transfer.generated.wait();

				ws->popupBox(nullptr);
			},
			nullptr
		);
		ws->waitingPopupBox(
			true, ws->theme()->dialogPrompt_Transferring(),
			true, timeout,
			true
		);
	}
	bool invalidateTiled(void) {
		if (!entry())
			return false;

		if (!isEditingAsImage()) {
			_transfer.filled = true;

			return true;
		}

		_transfer.filled = false;

		_debounce.modified();

		_tileIssues.clear();

		return true;
	}
	bool transferImageToTiled(Workspace* ws) {
		struct Error {
			typedef std::vector<Error> Array;

			bool isWarning = false;
			std::string message;

			Error() {
			}
			Error(bool warn, const std::string &msg) : isWarning(warn), message(msg) {
			}
		};
		struct Data : public NonCopyable {
			Project* project = nullptr;
			Image::Ptr image = nullptr;
			bool allowFlip = false;
			bool fillLocalPalette = false;
			Texture::Ptr attribtex = nullptr;
			PaletteAssets::Array* palette = nullptr;
			TilesAssets::Entry* tiles = nullptr;
			MapAssets::Entry* map = nullptr;
			TileIssue::Array tileIssues;
			Error::Array errors;
			bool loaded = false;

			Data(Renderer* rnd, Project* prj, Image::Ptr img, bool allowFlip_, bool fillLocalPalette_, Texture* texByte) {
				project = prj;
				image = img;
				allowFlip = allowFlip_;
				fillLocalPalette = fillLocalPalette_;
				attribtex = Texture::Ptr(texByte, [] (Texture*) -> void { /* Do nothing. */ });
				palette = new PaletteAssets::Array();
				tiles = new TilesAssets::Entry(rnd, prj->paletteGetter());
				map = new MapAssets::Entry("", prj->tilesGetter(), attribtex);

				for (int i = 0; i < project->assets()->palette.count(); ++i) {
					const PaletteAssets::Entry* pal = project->assets()->palette.get(i);
					palette->push_back(*pal);
				}
			}
			~Data() {
				delete map;
				map = nullptr;

				delete tiles;
				tiles = nullptr;

				delete palette;
				palette = nullptr;

				attribtex = nullptr;

				project = nullptr;
				image = nullptr;
				allowFlip = false;
				fillLocalPalette = false;
				tileIssues.clear();
				errors.clear();
			}
		};

		if (!entry())
			return false;

		if (_transfer.generated.working())
			_transfer.generated.wait();

		Image::Ptr &img = objectAsImage();
		Data* data = new Data(
			_transfer.renderer, _project,
			img, entry()->allowFlip, entry()->localPaletteEnabled,
			ws->theme()->textureByte()
		);
		_transfer.generated = ws->async(
			std::bind(
				[] (WorkTask* /* task */, Data* data) -> uintptr_t { // On work thread.
					const bool loaded = MapAssets::parseImage(
						*data->palette, *data->tiles, *data->map,
						data->image.get(),
						data->allowFlip, data->fillLocalPalette, false, false, true,
						data->project->paletteGetter(), data->project->tilesGetter(), data->project->tilesPageCount(),
						[data] (int x, int y, const char* msg) -> void {
							data->tileIssues.push_back(TileIssue(x, y, msg));
						},
						nullptr, [data] (const char* msg, bool isWarning) -> void {
							data->errors.push_back(Error(isWarning, msg));
						}
					);
					data->loaded = loaded;

					return (uintptr_t)0;
				},
				std::placeholders::_1, data
			),
			std::bind(
				[this, ws] (WorkTask* /* task */, uintptr_t /* ptr */, Data* data) -> void { // On main thread.
					const bool fillLocalPalette = data->fillLocalPalette;
					const PaletteAssets::Array &palette = *data->palette;
					const TilesAssets::Entry &tiles = *data->tiles;
					const MapAssets::Entry &map = *data->map;
					const int ref = entry()->ref;

					do {
						VariableGuard<decltype(_asImage.shared.transferring)> guardTransferring(&_asImage.shared.transferring, _asImage.shared.transferring, true);

						_refresh(enqueue<Commands::Map::BeginGroup>()
							->with("Import")
							->exec(object(), Variant((void*)entry()), attributes()));
						{
							TilesAssets::Entry* entry_ = _project->getTiles(ref);
							if (!entry_) {
								ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

								break;
							}
							const Variant ret = ws->touchTilesEditor(_transfer.window, _transfer.renderer, _project, ref, entry_)
								->post(Editable::IMPORT, (void*)&tiles);
							const unsigned refCmdId = (unsigned)(Variant::Long)ret;

							_refresh(enqueue<Commands::Map::Reference>()
								->with(_project, (unsigned)AssetsBundle::Categories::TILES, ref, refCmdId)
								->exec(object(), Variant((void*)entry()), attributes()));

							if (fillLocalPalette) {
								PaletteAssets::Array localPalette = palette;
								const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
								if ((int)localPalette.size() != n)
									localPalette.resize(n);

								Command* cmd = enqueue<Commands::Map::SetLocalPalette>()
									->with(localPalette)
									->exec(object(), Variant((void*)entry()));

								_refresh(cmd);
							}

							if (object()->width() != map.data->width() || object()->height() != map.data->height()) {
								Command* cmd = enqueue<Commands::Map::Resize>()
									->with(Math::Vec2i(map.data->width(), map.data->height()))
									->with(_setLayer, _tools.layer)
									->exec(object(), Variant((void*)entry()), attributes());

								_refresh(cmd);

								_selection.clear();
							}

							const bool hasAttributes = entry()->hasAttributes || map.hasAttributes;
							Command* cmd = enqueue<Commands::Map::Import>()
								->with(map.data, hasAttributes, map.attributes)
								->with(_setLayer, _tools.layer)
								->exec(object(), Variant((void*)entry()), attributes());

							_refresh(cmd);
						}
						_refresh(enqueue<Commands::Map::EndGroup>()
							->with("Import")
							->exec(object(), Variant((void*)entry()), attributes()));
					} while (false);

					entry()->image = data->image;

					_transfer.filled = true;

					_estimated.filled = false;
				},
				std::placeholders::_1, std::placeholders::_2, data
			),
			std::bind(
				[ws, this] (WorkTask* /* task */, uintptr_t /* ptr */, Data* data) -> void { // On main thread.
					_tileIssues.clear();
					_warnings.clear();

					_tileIssues = data->tileIssues;
					std::sort(
						_tileIssues.begin(), _tileIssues.end(),
						[] (const TileIssue &l, const TileIssue &r) -> bool {
							if (l.y < r.y)
								return true;
							else if (l.y > r.y)
								return false;

							if (l.x < r.x)
								return true;
							else if (l.x > r.x)
								return false;

							return l.message < r.message;
						}
					);

					if (!data->errors.empty()) {
						for (const Error &err : data->errors) {
							const std::string &msg = err.message;
							warn(ws, msg, true, err.isWarning);
						}
					}

					delete data;
				},
				std::placeholders::_1, std::placeholders::_2, data
			)
		);

		return true;
	}

	void activeLayerChanged(Workspace* ws, int layer) {
		if (!isEditingAsImage())
			return;

		if (layer != ASSETS_MAP_GRAPHICS_LAYER) {
			if (!_transfer.filled) {
				ImGui::WaitingPopupBox::TimeoutHandler timeout(
					[ws, this] (void) -> void {
						if (!_transfer.filled) {
							transferImageToTiled(ws);

							_transfer.generated.wait();
						}

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ws->waitingPopupBox(
					true, ws->theme()->dialogPrompt_Transferring(),
					true, timeout,
					true
				);
			}
		}
	}
	void editAsImageChanged(Workspace* ws) {
		if (!isEditingAsImage()) {
			if (!_transfer.filled) {
				ImGui::WaitingPopupBox::TimeoutHandler timeout(
					[ws, this] (void) -> void {
						if (!_transfer.filled) {
							transferImageToTiled(ws);

							_transfer.generated.wait();

							_transfer.reset();

							_transfer.filled = true;
						}

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ws->waitingPopupBox(
					true, ws->theme()->dialogPrompt_Transferring(),
					true, timeout,
					true
				);
			} else {
				_transfer.reset();

				_transfer.filled = true;
			}

			ws->skipFrame(); // Skip a frame to avoid glitch.

			return;
		}

		const Editing::Tools::PaintableTools prevTool = _tools.painting;
		if (prevTool == Editing::Tools::STAMP)
			_tools.painting = Editing::Tools::PENCIL;

		const PaletteAssets::Array &localPalette = entry()->localPalette;
		if (!localPalette.empty()) {
			const PaletteAssets::Entry &entry_ = localPalette.front();
			const Indexed::Ptr &palette_ = entry_.data;
			Colour col;
			palette_->get(0, col);
			_asImage.ref.fromColor(col);
		}

		if (entry()->image)
			_transfer.image = entry()->image;
	}
	void localPaletteEnabledChanged(Workspace* ws) {
		const bool localPaletteEnabled = entry()->localPaletteEnabled;
		if (!(isEditingAsImage() && localPaletteEnabled))
			return;

		/*if (_transfer.filled) {
			PaletteAssets::Array localPalette = entry()->localPalette;
			const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
			if ((int)localPalette.size() != n)
				localPalette.resize(n);

			const PaletteAssets &palette = _project->assets()->palette;
			for (int i = 0; i < n; ++i) {
				const PaletteAssets::Entry* entry = palette.get(i);
				localPalette[i] = *entry;
			}

			Command* cmd = enqueue<Commands::Map::SetLocalPalette>()
				->with(localPalette)
				->exec(object(), Variant((void*)entry()));

			_refresh(cmd);

			return;
		}*/

		ImGui::WaitingPopupBox::TimeoutHandler timeout(
			[ws, this] (void) -> void {
				if (!_transfer.filled) {
					transferImageToTiled(ws);

					_transfer.generated.wait();
				}

				ws->popupBox(nullptr);
			},
			nullptr
		);
		ws->waitingPopupBox(
			true, ws->theme()->dialogPrompt_Transferring(),
			true, timeout,
			true
		);
	}
	void touchLocalPalette(PaletteAssets::Array &localPalette) const {
		const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
		if ((int)localPalette.size() != n) {
			const int m = (int)localPalette.size();
			localPalette.resize(n);
			if (m < n) {
				const PaletteAssets &palette = _project->assets()->palette;
				if (palette.count() >= n) {
					for (int i = m; i < n; ++i) {
						const PaletteAssets::Entry* entry = palette.get(i);
						localPalette[i] = *entry;
					}
				}
			}
		}
	}

	bool playMapTesting(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		if (ws->running())
			return true;

		if (_tools.isPlaying)
			stopMapTesting(wnd, rnd, ws);

		if (!object())
			return false;

		// Start measuring performance.
		const long long start = DateTime::ticks();

		// The compile procedure.
		auto compile = [wnd, rnd, ws, this, start] (void) -> bool {
			// Compile.
			const Bytes::Ptr rom_ = compileMap(wnd, rnd, ws, this, entry(), _tools);

			if (!rom_)
				return false;

			// Finish measuring performance.
			const long long end = DateTime::ticks();
			const long long diff = end - start;
			const double secs = DateTime::toSeconds(diff);
			const std::string time = Text::toString(secs, 6, 0, ' ', std::ios::fixed);

			const std::string msg = "Completed in " + time + "s.";
			fprintf(stdout, "%s\n", msg.c_str());

			// Run and play.
			ws->run(wnd, rnd, rom_, true);

			_tools.isPlaying = true;

			// Finish.
			return true;
		};

		// Compile.
		if (isEditingAsImage()) {
			// Async.
			ImGui::WaitingPopupBox::TimeoutHandler timeout(
				[ws, this, compile] (void) -> void {
					if (!_transfer.filled) {
						transferImageToTiled(ws);

						_transfer.generated.wait();
					}

					compile();

					ws->popupBox(nullptr);
				},
				nullptr
			);
			ws->waitingPopupBox(
				true, ws->theme()->dialogPrompt_Transferring(),
				true, timeout,
				true
			);
		} else {
			compile();
		}

		// Finish.
		return true;
	}
	bool stopMapTesting(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		// Close the device.
		if (ws->running())
			ws->stop(wnd, rnd);

		// Stop playing.
		_tools.isPlaying = false;
#if EDITOR_MAP_UNLOAD_SYMBOLS_ON_FINISH
		_tools.isPlayerConfigLoaded = false;
		_tools.playerConfigText.clear();
		_tools.playerSymbolsText.clear();
		_tools.playerAliasesText.clear();
#endif /* EDITOR_MAP_UNLOAD_SYMBOLS_ON_FINISH */

		// Finish.
		return true;
	}
	static Bytes::Ptr compileMap(Window*, Renderer*, Workspace* ws, EditorMapImpl* self, MapAssets::Entry* entry_, Tools &tools) {
		// Prepare.
		if (!entry_ || !entry_->data)
			return nullptr;

		auto print_ = [ws] (const std::string &msg) -> void {
			ws->print(msg.c_str());
		};
		auto warn_ = [ws] (const std::string &msg) -> void {
			ws->warn(msg.c_str());
		};
		auto error_ = [ws] (const std::string &msg) -> void {
			ws->error(msg.c_str());
		};

		// Get the kernel.
		if (ws->kernels().empty()) {
			self->warn(ws, "No valid map player.", true, true);

			return nullptr;
		}

		const GBBASIC::Kernel::Ptr &krnl = ws->kernels().front();
		if (!krnl) {
			self->warn(ws, "No valid kernel.", true, true);

			return nullptr;
		}

		std::string dir;
		Path::split(krnl->path(), nullptr, nullptr, &dir);
		const std::string config = krnl->path();
		const std::string rom = Path::combine(dir.c_str(), krnl->kernelRom().c_str());
		const std::string sym = Path::combine(dir.c_str(), krnl->kernelSymbols().c_str());
		const std::string aliases = Path::combine(dir.c_str(), krnl->kernelAliases().c_str());
		const int bootstrapBank = krnl->bootstrapBank();

		// Load and parse the symbols.
		if (!tools.isPlayerConfigLoaded) {
			std::string configTxt;
			File::Ptr file(File::create());
			if (!file->open(config.c_str(), Stream::READ)) {
				self->warn(ws, "No valid config.", true, true);

				return nullptr;
			}
			if (!file->readString(configTxt)) {
				file->close();

				self->warn(ws, "No valid config.", true, true);

				return nullptr;
			}
			file->close();

			Editing::SymbolTable::Dictionary dict;
			std::string symTxt;
			std::string aliasesTxt;
			Editing::SymbolTable symbols;
			const bool loaded = symbols.load(
				dict,
				sym, symTxt,
				aliases, aliasesTxt,
				[self, ws] (const char* msg) -> void {
					self->warn(ws, msg, true, true);
				}
			);
			if (!loaded) {
				self->warn(ws, "No valid symbol.", true, true);

				return nullptr;
			}

			tools.isPlayerConfigLoaded = true;
			tools.playerConfigText     = configTxt;
			tools.playerSymbolsText    = symTxt;
			tools.playerAliasesText    = aliasesTxt;
		}

		// Compile.
		print_("Begin compiling for map testing.");

		AssetsBundle::Ptr assets(new AssetsBundle());
		do {
			// Prepare.
			const Project::Ptr &prj = ws->currentProject();
			GBBASIC_ASSERT(prj && "Impossible.");

			// Add the palette asset.
			assets->palette = prj->assets()->palette;

			// Add the tiles asset.
			const int ref = entry_->ref;
			const TilesAssets::Entry* tilesEntry = entry_->getTiles(ref);
			if (!tilesEntry || !tilesEntry->data) {
				error_("Invalid tiles asset.");

				break;
			}
			assets->tiles.add(*tilesEntry);

			// Add the map asset.
			MapAssets::Entry mapEntry = *entry_;
			mapEntry.ref = 0;
			assets->maps.add(mapEntry);

			const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
			const bool localPaletteEnabled = mapEntry.localPaletteEnabled;
			const PaletteAssets::Array &localPalette = mapEntry.localPalette;
			if (localPaletteEnabled && (int)localPalette.size() >= n && assets->palette.count() >= n) {
				for (int i = 0; i < n; ++i) {
					PaletteAssets::Entry* palEntry = assets->palette.get(i);
					*palEntry = localPalette[i]; // Replace with local palette.
				}
			}

			// Add a dummy scene asset.
			const SceneAssets::Entry sceneEntry(
				0,
				[entry_] (int) -> MapAssets::Entry* {
					return entry_;
				},
				[] (int) -> ActorAssets::Entry* {
					return nullptr;
				},
				prj->propertiesTexture(),
				prj->actorsTexture()
			);
			sceneEntry.data->hasAttributes(mapEntry.hasAttributes);
			assets->scenes.add(sceneEntry);

			// Add a dummy code asset.
			const std::string src = RES_CODE_PLAY_MAP_TESTING;
			assets->code.add(src);
		} while (false);

		const Bytes::Ptr rom_ = Workspace::compile(
			config, tools.playerConfigText,
			rom,
			sym, tools.playerSymbolsText,
			aliases, tools.playerAliasesText,
			"Map", assets,
			bootstrapBank,
			print_, warn_, error_
		);

		print_("End compiling for map testing.");

		if (!rom_)
			return nullptr;

		print_("Ok.");

		// Finish.
		return rom_;
	}
};

EditorMap* EditorMap::create(void) {
	EditorMapImpl* result = new EditorMapImpl();

	return result;
}

void EditorMap::destroy(EditorMap* ptr) {
	EditorMapImpl* impl = static_cast<EditorMapImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
