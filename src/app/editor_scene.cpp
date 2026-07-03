/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_scene.h"
#include "editor_actor.h"
#include "editor_scene.h"
#include "theme.h"
#include "workspace.h"
#include "resource/inline_resource.h"
#include "../utils/datetime.h"
#include "../utils/encoding.h"
#include "../utils/file_sandbox.h"
#include "../utils/filesystem.h"
#include "../utils/parsers.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../lib/imgui/imgui_internal.h"
#include "../../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_SCENE_UNLOAD_SYMBOLS_ON_FINISH
#	define EDITOR_SCENE_UNLOAD_SYMBOLS_ON_FINISH 1
#endif /* EDITOR_SCENE_UNLOAD_SYMBOLS_ON_FINISH */

/* ===========================================================================} */

/*
** {===========================================================================
** Scene editor
*/

class EditorSceneImpl : public EditorScene {
private:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> CelGetter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> CelSetter;

	typedef std::function<int(const Math::Recti* /* nullable */, Editing::Dots &)> CelsGetter;
	typedef std::function<int(const Math::Recti* /* nullable */, const Editing::Dots &)> CelsSetter;

	typedef std::function<bool(int, const Trigger &)> TriggerAdder;
	typedef std::function<bool(int, Trigger* /* nullable */)> TriggerRemover;
	typedef std::function<bool(int, Trigger* /* nullable */)> TriggerGetter;
	typedef std::function<bool(int, const Trigger &)> TriggerSetter;

	typedef std::function<void(void)> PostHandler;

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
		bool paintAreaFocused = false;

		Editing::Tiler hoveringMapIndex;
		int hoveringMapCel = -1;
		std::string hoveringMapTips;

		Editing::ActorIndex hoveringActor; // The hovering actor information according to the mouse.
		std::string hoveringActorTips;
		Math::Vec2i hoveringActorTipsPosition = Math::Vec2i(-1, -1);
		Editing::ActorIndex editingActor; // The editing actor information during i.e. mouse down and mouse up.
		Editing::ActorIndex selectedActor; // The selected actor information for routine overriding.
		bool actorRoutineOverridingsFilled = false;
		Text::Array actorRoutineOverridings;

		Byte hoveringTrigger = Scene::INVALID_TRIGGER(); // The hovering trigger index according to the mouse.
		std::string hoveringTriggerTips;
		Byte contextTrigger = Scene::INVALID_TRIGGER(); // The selected trigger index for context menu.
		bool contextTriggerOpened = false;
		Byte editingTrigger = Scene::INVALID_TRIGGER(); // The editing trigger index during i.e. mouse down and mouse up.
		Math::Vec2i triggerCursor = Math::Vec2i(-1, -1); // The trigger cursor according to the mouse.
		Math::Vec2i triggerResizing = Math::Vec2i(-1, -1); // The trigger resizing position according to the mouse.
		Math::Vec2i triggerMoving = Math::Vec2i(-1, -1); // The trigger moving position according to the mouse.
		bool triggerColorFilled = false;
		Colour triggerColor;
		bool triggerRoutinesFilled = false;
		Text::Array triggerRoutines;

		void clear(void) {
			initial = Math::Vec2i(-1, -1);
			tiler = Editing::Tiler();

			mouse = ImVec2(-1, -1);
			idle = 1;
			paintAreaFocused = false;

			hoveringMapIndex = Editing::Tiler();
			hoveringMapCel = -1;
			hoveringMapTips.clear();

			hoveringActor.clear();
			hoveringActorTips.clear();
			hoveringActorTipsPosition = Math::Vec2i(-1, -1);
			editingActor.clear();
			selectedActor.clear();
			actorRoutineOverridingsFilled = false;
			actorRoutineOverridings.clear();

			hoveringTrigger = Scene::INVALID_TRIGGER();
			hoveringTriggerTips.clear();
			contextTrigger = Scene::INVALID_TRIGGER();
			contextTriggerOpened = false;
			editingTrigger = Scene::INVALID_TRIGGER();
			triggerCursor = Math::Vec2i(-1, -1);
			triggerResizing = Math::Vec2i(-1, -1);
			triggerMoving = Math::Vec2i(-1, -1);
			triggerColorFilled = false;
			triggerColor = Colour();
			triggerRoutinesFilled = false;
			triggerRoutines.clear();
		}
		int area(Math::Recti &area) const {
			if (tiler.empty())
				return 0;

			area = Math::Recti::byXYWH(tiler.position.x, tiler.position.y, tiler.size.x, tiler.size.y);

			return area.width() * area.height();
		}
	} _selection;
	Editing::Tools::Objects _objects;
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
		std::string info;

		void clear(void) {
			text.clear();
			cursor = Editing::Tiler();
			info.clear();
		}
	} _status;
	float _statusWidth = 0.0f;
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
	std::function<void(void)> _checker = nullptr;
	std::function<void(void)> _determinator = nullptr;
	ActorAssets::Entry::PlayerBehaviourCheckingHandler _isPlayerBehaviour = nullptr;
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
		CelGetter getCel = nullptr;
		CelSetter setCel = nullptr;
		CelsGetter getCels = nullptr;
		CelsSetter setCels = nullptr;

		TriggerAdder addTrigger = nullptr;
		TriggerRemover removeTrigger = nullptr;
		TriggerGetter getTrigger = nullptr;
		TriggerSetter setTrigger = nullptr;
		Editing::SceneTriggerQuerier queryTrigger = nullptr;

		bool empty(void) const {
			return
				!getCel || !setCel || !getCels || !setCels ||
				!addTrigger || !removeTrigger || !getTrigger || !setTrigger || !queryTrigger;
		}
		void clear(void) {
			getCel = nullptr;
			setCel = nullptr;
			getCels = nullptr;
			setCels = nullptr;

			addTrigger = nullptr;
			removeTrigger = nullptr;
			getTrigger = nullptr;
			setTrigger = nullptr;
			queryTrigger = nullptr;
		}
		void fill(
			CelGetter getCel_, CelSetter setCel_, CelsGetter getCels_, CelsSetter setCels_,
			TriggerAdder addTrigger_, TriggerRemover removeTrigger_, TriggerGetter getTrigger_, TriggerSetter setTrigger_, Editing::SceneTriggerQuerier queryTrigger_
		) {
			getCel = getCel_;
			setCel = setCel_;
			getCels = getCels_;
			setCels = setCels_;

			addTrigger = addTrigger_;
			removeTrigger = removeTrigger_;
			getTrigger = getTrigger_;
			setTrigger = setTrigger_;
			queryTrigger = queryTrigger_;
		}
	} _binding;
	Editing::MapCelGetter _overlay = nullptr;

	struct Ref : public Editor::Ref {
		bool resolving = false;
		unsigned refCategory = (unsigned)(~0);
		int refMapIndex = -1;
		std::string refMapText;
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;
		Editing::Brush mapCursor;
		Editing::Brush attributesCursor;
		Editing::Brush propertiesCursor;
		Editing::Brush stamp;
		std::string byteTooltip;

		bool showGrids = true;
		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool transparentBackbroundVisible = true;

		void clear(void) {
			resolving = false;
			refCategory = (unsigned)(~0);
			refMapIndex = -1;
			refMapText.clear();
			image = nullptr;
			texture = nullptr;
			mapCursor = Editing::Brush();
			attributesCursor = Editing::Brush();
			propertiesCursor = Editing::Brush();
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

			const MapAssets::Entry* entry = project->getMap(refMapIndex);
			if (!entry)
				return false;
			const TilesAssets::Entry* entry_ = project->getTiles(entry->ref);
			if (!entry_)
				return false;

			image = entry_->data;
			if (!image)
				return false;

			texture = Texture::Ptr(Texture::create());
			texture->fromImage(rnd, Texture::STATIC, image.get(), Texture::NEAREST);
			texture->blend(Texture::BLEND);

			return true;
		}

		void refreshRefMapText(Theme* theme) {
			refMapText = Text::format(theme->windowScene_RefMap(), { Text::toString(refMapIndex) });
		}
	} _ref;
	std::function<void(int)> _setLayer = nullptr;
	std::function<void(int, bool)> _setLayerEnabled = nullptr;
	struct Tools {
		int layer = ASSETS_SCENE_ACTORS_LAYER; // Defaults to the objects layer.
		Editing::Tools::PaintableTools painting = Editing::Tools::SMUDGE;
		Editing::Tools::PaintableTools paintingToolForActor = Editing::Tools::SMUDGE;
		Editing::Tools::PaintableTools paintingToolForTrigger = Editing::Tools::SMUDGE;
		Editing::Tools::PaintableTools paintingToolForPaintable = Editing::Tools::PENCIL;
		Editing::Tools::BitwiseOperations bitwiseOperations = Editing::Tools::SET;
		int actorTemplate = Scene::INVALID_ACTOR();
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
		bool routinesUnfolded = false;
		bool definitionUnfolded = false;
		float definitionHeight = 0;
		scene_t definitionShadow;

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);
		int mouseActionButton = ImGuiMouseButton_Left;

		std::function<bool(void)> playSceneTesting = nullptr;
		std::function<bool(void)> stopSceneTesting = nullptr;
		bool isPlaying = false;
		bool isPlayerConfigLoaded = false;
		std::string playerConfigText;
		std::string playerSymbolsText;
		std::string playerAliasesText;

		PostHandler post = nullptr;
		Editing::Tools::PaintableTools postType = Editing::Tools::PENCIL;

		Tools() {
		}

		void clear(void) {
			layer = ASSETS_SCENE_ACTORS_LAYER;
			painting = Editing::Tools::SMUDGE;
			paintingToolForActor = Editing::Tools::SMUDGE;
			paintingToolForTrigger = Editing::Tools::SMUDGE;
			paintingToolForPaintable = Editing::Tools::PENCIL;
			bitwiseOperations = Editing::Tools::SET;
			actorTemplate = Scene::INVALID_ACTOR();
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
			routinesUnfolded = false;
			definitionUnfolded = false;
			definitionHeight = 0;
			definitionShadow = scene_t();

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);
			mouseActionButton = ImGuiMouseButton_Left;

			playSceneTesting = nullptr;
			stopSceneTesting = nullptr;
			isPlaying = false;
			isPlayerConfigLoaded = false;
			playerConfigText.clear();
			playerSymbolsText.clear();
			playerAliasesText.clear();

			post = nullptr;
			postType = Editing::Tools::PENCIL;
		}
	} _tools;
	Processor _processors[Editing::Tools::COUNT] = {
		Processor( // Hand.
			std::bind(&EditorSceneImpl::handToolDown, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::handToolMove, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::handToolUp, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::handToolHover, this, std::placeholders::_1)
		),
		Processor( // Eyedropper.
			std::bind(&EditorSceneImpl::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		),
		Processor( // Pencil.
			std::bind(&EditorSceneImpl::paintToolDown<Commands::Scene::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolMove<Commands::Scene::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorSceneImpl::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Lasso.
			std::bind(&EditorSceneImpl::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::lassoToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Line.
			std::bind(&EditorSceneImpl::paintToolDown<Commands::Scene::Line>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolMove<Commands::Scene::Line>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box.
			std::bind(&EditorSceneImpl::paintToolDown<Commands::Scene::Box>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolMove<Commands::Scene::Box>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box fill.
			std::bind(&EditorSceneImpl::paintToolDown<Commands::Scene::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolMove<Commands::Scene::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse.
			std::bind(&EditorSceneImpl::paintToolDown<Commands::Scene::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolMove<Commands::Scene::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse fill.
			std::bind(&EditorSceneImpl::paintToolDown<Commands::Scene::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolMove<Commands::Scene::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Stamp, pasting.
			std::bind(&EditorSceneImpl::stampToolDown, this, std::placeholders::_1),
			nullptr,
			std::bind(&EditorSceneImpl::stampToolUp, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::stampToolHover, this, std::placeholders::_1)
		),
		Processor( // Smudge.
			nullptr, // To be binded later.
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Eraser.
			nullptr, // To be binded later.
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Move.
			nullptr, // To be binded later.
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Resize.
			nullptr, // To be binded later.
			nullptr,
			nullptr,
			nullptr
		),
		Processor( // Ref.
			nullptr, // To be binded later.
			nullptr,
			nullptr,
			nullptr
		)
	};

public:
	EditorSceneImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Scene::Pencil>()
			->reg<Commands::Scene::Line>()
			->reg<Commands::Scene::Box>()
			->reg<Commands::Scene::BoxFill>()
			->reg<Commands::Scene::Ellipse>()
			->reg<Commands::Scene::EllipseFill>()
			->reg<Commands::Scene::Fill>()
			->reg<Commands::Scene::Replace>()
			->reg<Commands::Scene::Stamp>()
			->reg<Commands::Scene::Rotate>()
			->reg<Commands::Scene::Flip>()
			->reg<Commands::Scene::Cut>()
			->reg<Commands::Scene::Paste>()
			->reg<Commands::Scene::Delete>()
			->reg<Commands::Scene::SetActor>()
			->reg<Commands::Scene::MoveActor>()
			->reg<Commands::Scene::AddTrigger>()
			->reg<Commands::Scene::DeleteTrigger>()
			->reg<Commands::Scene::MoveTrigger>()
			->reg<Commands::Scene::ResizeTrigger>()
			->reg<Commands::Scene::SetTriggerColor>()
			->reg<Commands::Scene::SetTriggerRoutines>()
			->reg<Commands::Scene::SwapTrigger>()
			->reg<Commands::Scene::SwitchLayer>()
			->reg<Commands::Scene::ToggleMask>()
			->reg<Commands::Scene::SetName>()
			->reg<Commands::Scene::SetDefinition>()
			->reg<Commands::Scene::SetActorRoutineOverridings>()
			->reg<Commands::Scene::Resize>()
			->reg<Commands::Scene::Import>();
	}
	virtual ~EditorSceneImpl() override {
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

		int another = -1;
		if (entry()->name.empty() || !_project->canRenameScene(_index, entry()->name, &another)) {
			if (entry()->name.empty()) {
				const std::string msg = Text::format(ws->theme()->warning_SceneSceneNameIsEmptyAtPage(), { Text::toString(_index) });
				warn(ws, msg, true);
			} else {
				const std::string msg = Text::format(ws->theme()->warning_SceneDuplicateSceneNameAtPages(), { entry()->name, Text::toString(_index), Text::toString(another) });
				warn(ws, msg, true);
			}
			entry()->name = _project->getUsableSceneName(_index); // Unique name.
			_project->hasDirtyAsset(true);
		}

		entry()->refresh(); // Refresh the outdated editable resources.

		_objects = Editing::Tools::Objects(
			[&] (Editing::Tools::Objects::ObjectGetter getObj) -> void {
				const int w = actors()->width();
				const int h = actors()->height();
				for (int i = 0; i < w; ++i) {
					for (int j = 0; j < h; ++j) {
						const int cel = actors()->get(i, j);
						if (cel != Scene::INVALID_ACTOR()) {
							ActorAssets::Entry* entry = _project->getActor(cel);
							if (entry) {
								const std::string name = Text::format(
									"Actor - {0} [{1},{2}]",
									{
										entry->name,
										Text::toString(i), Text::toString(j)
									}
								);
								const std::string shortName = entry->name.empty() ?
									"[" + ws->theme()->generic_Noname() + "]" :
									entry->name;
								int w = 1, h = 1;
								entry->cleanup(entry->figure);
								Math::Vec2i anchor;
								ActorAssets::Entry::PaletteResolver resolver = nullptr;
								if (_project->preferencesPreviewPaletteBits()) {
									resolver = std::bind(&ActorAssets::Entry::colorAt, entry, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
								}
								const Texture::Ptr &tex = entry->figureTexture(&anchor, resolver);
								if (tex) {
									w = Math::max(tex->width() / GBBASIC_TILE_SIZE, 1);
									h = Math::max(tex->height() / GBBASIC_TILE_SIZE, 1);
								}
								getObj(AssetsBundle::Categories::ACTOR, Left<Math::Vec2i>(Math::Vec2i(i, j)), Math::Vec2i(w, h), name, shortName);
							}
						}
					}
				}

				const int n = (int)triggers()->size();
				for (int i = 0; i < n; ++i) {
					const Trigger &trigger = (*triggers())[i];
					const std::string name = Text::format(
						"Trigger - {0} [{1},{2}] ({3}x{4})",
						{
							Text::toString(i),
							Text::toString(trigger.xMin()), Text::toString(trigger.yMin()),
							Text::toString(trigger.width()), Text::toString(trigger.height())
						}
					);
					const std::string shortName = Text::format(
						"Trigger{0}",
						{
							Text::toString(i)
						}
					);
					getObj(AssetsBundle::Categories::TRIGGER, Right<int>(i), trigger.size(), name, shortName);
				}
			}
		);
		_objects.fill();

		_refresh = std::bind(&EditorSceneImpl::refresh, this, ws, std::placeholders::_1);

		_checker = [ws, project, this] (void) -> void {
			// Prepare.
			_warnings.clear();

			GBBASIC::Kernel::Ptr krnl = ws->activeKernel();

			// Check for actor count.
			if (krnl) {
				if ((int)entry()->refActors.size() > krnl->objectsMaxActorCount()) {
					warn(ws, ws->theme()->warning_SceneActorCountOutOfBounds(), true);
				}
			}

			// Check for actor behaviour.
			int invalidActorCount = 0;
			int playerCount = 0;

			if (krnl) {
				const GBBASIC::Kernel::Behaviour::Array &behaviours = krnl->behaviours();

				const SceneAssets::Entry::Ref &actors = entry()->refActors;
				for (SceneAssets::Entry::Ref::const_iterator it = actors.begin(); it != actors.end(); ++it) {
					const int cel = *it;
					const ActorAssets::Entry* entry = project->getActor(cel);
					if (!entry)
						continue;

					const active_t def = entry->definition;
					const UInt8 idx = def.behaviour;
					GBBASIC::Kernel::Behaviour::Array::const_iterator bit = std::find_if(
						behaviours.begin(), behaviours.end(),
						[idx] (const GBBASIC::Kernel::Behaviour &bhvr) -> bool {
							return idx == bhvr.value;
						}
					);
					if (bit == behaviours.end()) {
						++invalidActorCount;
					} else {
						const GBBASIC::Kernel::Behaviour &bhvr = *bit;
						if (bhvr.type == KERNEL_BEHAVIOUR_TYPE_PLAYER)
							++playerCount;
					}
				}
			}

			if (invalidActorCount > 0) {
				warn(ws, ws->theme()->warning_SceneInvalidActorBehaviour(), true);
			}
			if (playerCount > 1) {
				warn(ws, ws->theme()->warning_SceneMultiplePlayerActors(), true);
			}

			// Check for actor routine overridings.
			int removed = 0;
			ActorRoutineOverriding::Array actorRoutines = entry()->actorRoutineOverridings;
			for (ActorRoutineOverriding::Array::iterator it = actorRoutines.begin(); it != actorRoutines.end(); ) {
				ActorRoutineOverriding &routine = *it;
				const int cel = actors()->get(routine.position.x, routine.position.y);
				if (cel == Scene::INVALID_ACTOR()) {
					it = actorRoutines.erase(it);
					++removed;
				} else {
					++it;
				}
			}
			if (removed) {
				entry()->actorRoutineOverridings = actorRoutines;
				_project->hasDirtyAsset(true);

				ws->print(ws->theme()->dialogPrompt_RemovedUnreferencedActorRoutineOverriding().c_str());
			}

			// Check for trigger count.
			if (krnl) {
				if (triggers() && (int)triggers()->size() > krnl->objectsMaxTriggerCount()) {
					warn(ws, ws->theme()->warning_SceneTriggerCountOutOfBounds(), true);
				}
			}

			// Check for scene camera.
			const scene_t &def = entry()->definition;
			const int minX = -GBBASIC_SCREEN_WIDTH;
			const int maxX = Math::max(object()->width() * GBBASIC_TILE_SIZE - GBBASIC_SCREEN_WIDTH, 0);
			const int minY = -GBBASIC_SCREEN_HEIGHT;
			const int maxY = Math::max(object()->height() * GBBASIC_TILE_SIZE - GBBASIC_SCREEN_HEIGHT, 0);
			if (
				((int)def.camera_position.x < minX || (int)def.camera_position.x > maxX) ||
				((int)def.camera_position.y < minY || (int)def.camera_position.y > maxY)
			) {
				warn(ws, ws->theme()->warning_SceneCameraPositionOutOfBounds(), true);
			}
		};
		_checker();

		GBBASIC::Kernel::Ptr krnl = ws->activeKernel();
		if (krnl) {
			const GBBASIC::Kernel::Behaviour::Array &behaviours = krnl->behaviours();
			_isPlayerBehaviour = [&behaviours] (UInt8 val) -> bool {
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
		}
		_determinator = [ws, this] (void) -> void {
			// Check for 16x16 player.
			const bool is16x16Player = entry()->has16x16PlayerActor(_isPlayerBehaviour);
			entry()->definition.is_16x16_player = is16x16Player;
			_tools.definitionShadow.is_16x16_player = is16x16Player;
		};

		_ref.refCategory = refCategory;
		_ref.refMapIndex = refIndex;
		_ref.refreshRefMapText(ws->theme());
		_ref.mapCursor.position = Math::Vec2i(0, 0);
		_ref.mapCursor.size = _tileSize;
		_ref.attributesCursor.position = Math::Vec2i(0, 0);
		_ref.attributesCursor.size = _tileSize;
		_ref.propertiesCursor.position = Math::Vec2i(0, 0);
		_ref.propertiesCursor.size = _tileSize;
		_ref.stamp.position = Math::Vec2i(0, 0);
		_ref.stamp.size = _tileSize;

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_ref.showGrids = _project->preferencesShowGrids();
		_ref.gridsVisible = !caps.pressed();
		_ref.gridUnit = _ref.mapCursor.size;
		_ref.transparentBackbroundVisible = num.pressed();

		_setLayer = std::bind(&EditorSceneImpl::changeLayer, this, wnd, rnd, ws, std::placeholders::_1);
		_setLayerEnabled = std::bind(&EditorSceneImpl::toggleLayer, this, wnd, rnd, ws, std::placeholders::_1, std::placeholders::_2);

		_tools.magnification = entry()->magnification;
		_tools.namableText = entry()->name;
		_tools.showGrids = _project->preferencesShowGrids();
		_tools.fineZooming = _project->preferencesFineZooming();
		_tools.gridsVisible = !caps.pressed();
		_tools.gridUnit = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
		_tools.transparentBackbroundVisible = num.pressed();
		_tools.definitionShadow = entry()->definition;
		_tools.playSceneTesting = std::bind(&EditorSceneImpl::playSceneTesting, this, wnd, rnd, ws);
		_tools.stopSceneTesting = std::bind(&EditorSceneImpl::stopSceneTesting, this, wnd, rnd, ws);

		bindLayerTools(wnd, ws);

		fprintf(stdout, "Scene editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Scene editor closed: #%d.\n", _index);

		_project = nullptr;
		_index = -1;

		_tileSize = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
		_selection.clear();
		_objects.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_checker = nullptr;
		_determinator = nullptr;
		_isPlayerBehaviour = nullptr;
		_estimated.clear();
		_painting.clear();
		_binding.clear();
		_overlay = nullptr;

		_ref.clear();
		_setLayer = nullptr;
		_setLayerEnabled = nullptr;
		_tools.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		_determinator();
	}
	virtual void leave(class Workspace*) override {
		_tools.stopSceneTesting();

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
		return _commands->hasUnsavedChanges();
	}
	virtual void markChangesSaved(void) override {
		_commands->markChangesSaved();
	}
	virtual void prepareForSaving(class Workspace*) override {
		// Do nothing.
	}

	virtual void copy(void) override {
		copy(true);
	}
	void copy(bool clearSelection) {
		switch (_tools.layer) {
		case ASSETS_SCENE_ACTORS_LAYER: // Fall through.
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		default: {
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
			}

			break;
		}

		if (clearSelection)
			_selection.clear();
	}
	virtual void cut(void) override {
		switch (_tools.layer) {
		case ASSETS_SCENE_ACTORS_LAYER: // Fall through.
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		default: {
				if (!_binding.getCel || !_binding.setCel)
					return;

				copy(false);

				Math::Recti sel;
				const int size = area(&sel);
				if (size == 0)
					return;

				enqueue<Commands::Scene::Cut>()
					->with(_binding.getCel, _binding.setCel)
					->with(sel)
					->with(_setLayer, _tools.layer)
					->exec(currentLayer());
			}

			break;
		}

		_selection.clear();
	}
	virtual bool pastable(void) const override {
		switch (_tools.layer) {
		case ASSETS_SCENE_ACTORS_LAYER: // Fall through.
		case ASSETS_SCENE_TRIGGERS_LAYER:
			return false;
		default:
			return Platform::hasClipboardText();
		}
	}
	virtual void paste(void) override {
		switch (_tools.layer) {
		case ASSETS_SCENE_ACTORS_LAYER: // Fall through.
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		default: {
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
					std::bind(&EditorSceneImpl::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
					std::bind(&EditorSceneImpl::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
				};

				destroyOverlay();
			}

			break;
		}
	}
	virtual void del(bool) override {
		switch (_tools.layer) {
		case ASSETS_SCENE_ACTORS_LAYER: {
				const Editing::ActorIndex selectedActor = _selection.selectedActor; // Reserved.
				_selection.clear();
				_selection.selectedActor = selectedActor;
			}

			return;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		default: {
				if (!_binding.getCel || !_binding.setCel)
					return;

				Math::Recti sel;
				const int size = cursorOrArea(&sel);
				if (size == 0)
					return;

				enqueue<Commands::Scene::Delete>()
					->with(_binding.getCel, _binding.setCel)
					->with(sel)
					->with(_setLayer, _tools.layer)
					->exec(currentLayer());
			}

			break;
		}

		_selection.clear();
	}
	int area(Math::Recti* area /* nullable */) const {
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
	int cursorOrArea(Math::Recti* area_ /* nullable */) const {
		Math::Recti sel_;
		const int size = _selection.area(sel_);

		if (size == 0 && _cursor.position != Math::Vec2i(-1, -1) && _cursor.size != Math::Vec2i(0, 0)) {
			const Math::Recti sel = Math::Recti::byXYWH(_cursor.position.x, _cursor.position.y, _cursor.size.x, _cursor.size.y);
			if (area_)
				*area_ = sel;

			return _cursor.size.x * _cursor.size.y;
		}

		return area(area_);
	}
	virtual bool selectable(void) const override {
		if (_tools.layer == ASSETS_SCENE_TRIGGERS_LAYER) {
			return false;
		} else {
			return true;
		}
	}

	virtual const char* redoable(void) const override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}
	virtual const char* undoable(void) const override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}

	virtual void redo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->redoable();
		if (!cmd)
			return;

		const int width = object()->width();
		const int height = object()->height();
		const bool withObject =
			Command::is<Commands::Scene::SetActor>(cmd) ||
			Command::is<Commands::Scene::MoveActor>(cmd) ||
			Command::is<Commands::Scene::AddTrigger>(cmd) ||
			Command::is<Commands::Scene::DeleteTrigger>(cmd) ||
			Command::is<Commands::Scene::MoveTrigger>(cmd) ||
			Command::is<Commands::Scene::ResizeTrigger>(cmd) ||
			Command::is<Commands::Scene::SetTriggerColor>(cmd) ||
			Command::is<Commands::Scene::SetTriggerRoutines>(cmd) ||
			Command::is<Commands::Scene::SwapTrigger>(cmd) ||
			Command::is<Commands::Scene::SwitchLayer>(cmd) ||
			Command::is<Commands::Scene::SetName>(cmd) ||
			Command::is<Commands::Scene::SetDefinition>(cmd) ||
			Command::is<Commands::Scene::SetActorRoutineOverridings>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		if (withObject)
			_commands->redo(object(), Variant((void*)entry()));
		else
			_commands->redo(currentLayer());

		if (width != object()->width() || height != object()->height())
			_selection.clear();

		_refresh(cmd);

		_project->toPollEditor(true);

		modified();
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		const int width = object()->width();
		const int height = object()->height();
		const bool withObject =
			Command::is<Commands::Scene::SetActor>(cmd) ||
			Command::is<Commands::Scene::MoveActor>(cmd) ||
			Command::is<Commands::Scene::AddTrigger>(cmd) ||
			Command::is<Commands::Scene::DeleteTrigger>(cmd) ||
			Command::is<Commands::Scene::MoveTrigger>(cmd) ||
			Command::is<Commands::Scene::ResizeTrigger>(cmd) ||
			Command::is<Commands::Scene::SetTriggerColor>(cmd) ||
			Command::is<Commands::Scene::SetTriggerRoutines>(cmd) ||
			Command::is<Commands::Scene::SwapTrigger>(cmd) ||
			Command::is<Commands::Scene::SwitchLayer>(cmd) ||
			Command::is<Commands::Scene::SetName>(cmd) ||
			Command::is<Commands::Scene::SetDefinition>(cmd) ||
			Command::is<Commands::Scene::SetActorRoutineOverridings>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		if (withObject)
			_commands->undo(object(), Variant((void*)entry()));
		else
			_commands->undo(currentLayer());

		if (width != object()->width() || height != object()->height())
			_selection.clear();

		_refresh(cmd);

		_project->toPollEditor(true);

		modified();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case RESIZE: {
				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				if (width == object()->width() && height == object()->height())
					return Variant(true);

				Command* cmd = enqueue<Commands::Scene::Resize>()
					->with(Math::Vec2i(width, height))
					->with(_setLayer, _tools.layer)
					->exec(object(), Variant((void*)entry()));

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

				_ref.refMapIndex = (int)index;
				_ref.refreshRefMapText(ws->theme());
			}

			return Variant(true);
		case SET_FRAME: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				constexpr const int layers[] = {
					ASSETS_SCENE_GRAPHICS_LAYER,
					ASSETS_SCENE_ATTRIBUTES_LAYER,
					ASSETS_SCENE_PROPERTIES_LAYER,
					ASSETS_SCENE_ACTORS_LAYER,
					ASSETS_SCENE_TRIGGERS_LAYER
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
				(void)deep;

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
				std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::getCels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::setCels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::addTrigger, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::removeTrigger, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::getTrigger, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::setTrigger, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::queryTrigger, this, std::placeholders::_1, std::placeholders::_2)
			);
		}

		const bool allowMouseOperations = shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		if (!entry() || !object()) {
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			ImGui::EndChild();
			refreshStatus(wnd, rnd, ws, nullptr);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}
		if (_ref.resolving || entry()->refMap == -1) { // This asset's dependency was lost, need to resolve; or a user is changing it manually.
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ws->theme()->style()->screenClearingColor);
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			resolveRef(wnd, rnd, ws, width, height - statusBarHeight);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			refreshStatus(wnd, rnd, ws, nullptr);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}
		if (entry()->refresh()) { // Refresh the outdated editable resources.
			_ref.fill(rnd, _project, true);

			entry()->touch();
		}
		Map::Tiles tiles;
		if (!map()->tiles(tiles) || !tiles.texture)
			entry()->touch(); // Ensure the runtime resources are ready.
		const Math::Vec2i mapSz(map()->width(), map()->height());
		const Math::Vec2i sceneSz(object()->width(), object()->height());
		if (mapSz != sceneSz) {
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ws->theme()->style()->screenClearingColor);
			ImGui::BeginChild("@Blk", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			resolveSize(wnd, rnd, ws, width, height - statusBarHeight);
			ImGui::EndChild();
			ImGui::PopStyleColor();
			refreshStatus(wnd, rnd, ws, nullptr);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}

		Editing::Tiler cursor = _cursor;
		Editing::ActorIndex::Array hoveringActors;
		Editing::TriggerIndex::Array hoveringTriggers;

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
				const int refMap = entry()->refMap;
				const MapAssets::Entry* mapEntry = entry()->getMap(refMap);
				if (!mapEntry || !mapEntry->data) {
					PaletteAssets::Entry* entry_ = _project->getPalette(plt);

					return entry_;
				}

				const PaletteAssets::Entry* entry_ = nullptr;
				const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
				const bool localPaletteEnabled = mapEntry->localPaletteEnabled;
				const PaletteAssets::Array &localPalette = mapEntry->localPalette;
				if (localPaletteEnabled && (int)localPalette.size() >= n) {
					plt = Math::clamp(plt, 0, (int)localPalette.size() - 1);
					entry_ = &localPalette[plt];
				} else {
					entry_ = _project->getPalette(plt);
				}

				return entry_;
			};
			auto getFlip = [&] (const Math::Vec2i &pos) -> int {
				if (!object()->hasAttributes())
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

			if (_cursor.empty()) {
				_cursor.position = Math::Vec2i(0, 0);
				_cursor.size = Math::Vec2i(1, 1);
			}

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

			auto renderMap = [&] (bool showGrids) -> void {
				Editing::map(
					rnd, ws,
					_project,
					"@Map",
					map().get(), _tileSize,
					_project->preferencesPreviewPaletteBits(),
					width_,
					nullptr, false,
					nullptr,
					showGrids ? &_tools.gridUnit : nullptr,
					_tools.transparentBackbroundVisible,
					std::numeric_limits<int>::min(),
					nullptr,
					getPlt, getCol,
					getFlip,
					_tools.mouseActionButton
				);
			};
			auto renderAttributes = [&] (void) -> void {
				Editing::map(
					rnd, ws,
					_project,
					attributes().get(), _tileSize,
					false,
					width_,
					nullptr, false,
					nullptr,
					nullptr,
					false,
					0,
					[&] (const Math::Vec2i &pos) -> int {
						const int idx = attributes()->get(pos.x, pos.y);

						return idx;
					},
					nullptr, nullptr,
					nullptr,
					_tools.mouseActionButton
				);
			};
			auto renderProperties = [&] (void) -> void {
				Editing::map(
					rnd, ws,
					_project,
					properties().get(), _tileSize,
					false,
					width_,
					nullptr, false,
					nullptr,
					nullptr,
					false,
					0,
					[&] (const Math::Vec2i &pos) -> int {
						const int idx = properties()->get(pos.x, pos.y);

						return idx;
					},
					nullptr, nullptr,
					nullptr,
					_tools.mouseActionButton
				);
			};
			auto renderActors = [&] (void) -> void {
				Editing::actors(
					rnd, ws,
					_project,
					object().get(),
					actors().get(), _tileSize,
					width_,
					true, 128,
					nullptr, false,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					false,
					nullptr,
					_tools.mouseActionButton
				);
			};
			auto renderTriggers = [&] (void) -> void {
				Editing::triggers(
					rnd, ws,
					_project,
					triggers(),
					map().get(), _tileSize,
					width_,
					0.5f,
					nullptr, nullptr,
					nullptr,
					nullptr,
					nullptr,
					_tools.mouseActionButton
				);
			};
			auto autoSwitchToolForHovering = [] (Editing::Tools::PaintableTools &active, const Editing::Tools::PaintableTools &prefered) -> void {
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					if (active == Editing::Tools::MOVE && prefered == Editing::Tools::SMUDGE)
						active = Editing::Tools::SMUDGE;
				}
			};
			auto autoSwitchToolForNonHovering = [] (Editing::Tools::PaintableTools &active, const Editing::Tools::PaintableTools &prefered) -> void {
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					if (active == Editing::Tools::SMUDGE && prefered == Editing::Tools::SMUDGE)
						active = Editing::Tools::MOVE;
				}
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

				if (_tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER) {
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
				} else /* if (_tools.layer == ASSETS_SCENE_PROPERTIES_LAYER) */ {
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

					const int props = _selection.hoveringMapCel;
					const int blkLeftBit = (props >> GBBASIC_SCENE_BLOCKING_LEFT_BIT) & 0x00000001;
					const int blkRightBit = (props >> GBBASIC_SCENE_BLOCKING_RIGHT_BIT) & 0x00000001;
					const int blkUpBit = (props >> GBBASIC_SCENE_BLOCKING_UP_BIT) & 0x00000001;
					const int blkDownBit = (props >> GBBASIC_SCENE_BLOCKING_DOWN_BIT) & 0x00000001;
					const int isLadderBit = (props >> GBBASIC_SCENE_IS_LADDER_BIT) & 0x00000001;
					_selection.hoveringMapTips += Text::format(ws->theme()->tooltipScene_BitBlockingLeft(), { Text::toString(blkLeftBit) });
					_selection.hoveringMapTips += Text::format(ws->theme()->tooltipScene_BitBlockingRight(), { Text::toString(blkRightBit) });
					_selection.hoveringMapTips += Text::format(ws->theme()->tooltipScene_BitBlockingUp(), { Text::toString(blkUpBit) });
					_selection.hoveringMapTips += Text::format(ws->theme()->tooltipScene_BitBlockingDown(), { Text::toString(blkDownBit) });
					_selection.hoveringMapTips += Text::format(ws->theme()->tooltipScene_BitIsLadder(), { Text::toString(isLadderBit) });
				}
			};
			const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);
			if (currentLayer()) {
				switch (_tools.layer) {
				case ASSETS_SCENE_GRAPHICS_LAYER: {
						const ImVec2 pos = ImGui::GetCursorPos();
						_painting.set(
							Editing::map(
								rnd, ws,
								_project,
								currentLayer().get(), _tileSize,
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
						ImGui::SetCursorPos(pos);
						if (_project->preferencesSceneShowTriggers()) {
							renderTriggers();
							ImGui::SetCursorPos(pos);
						}
						if (_project->preferencesSceneShowActors()) {
							renderActors();
							ImGui::SetCursorPos(pos);
						}
						if (!ctrl.pressed() && _project->preferencesSceneShowAttributes()) {
							renderAttributes();
							ImGui::SetCursorPos(pos);
						} else if (!ctrl.pressed() && _project->preferencesSceneShowProperties()) {
							renderProperties();
							ImGui::SetCursorPos(pos);
						}
					}

					break;
				case ASSETS_SCENE_ATTRIBUTES_LAYER: {
						const ImVec2 pos = ImGui::GetCursorPos();
						renderMap(false);
						ImGui::SetCursorPos(pos);
						if (_project->preferencesSceneShowTriggers()) {
							renderTriggers();
							ImGui::SetCursorPos(pos);
						}
						if (_project->preferencesSceneShowActors()) {
							renderActors();
							ImGui::SetCursorPos(pos);
						}
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

					break;
				case ASSETS_SCENE_PROPERTIES_LAYER: {
						const ImVec2 pos = ImGui::GetCursorPos();
						renderMap(false);
						ImGui::SetCursorPos(pos);
						if (_project->preferencesSceneShowTriggers()) {
							renderTriggers();
							ImGui::SetCursorPos(pos);
						}
						if (_project->preferencesSceneShowActors()) {
							renderActors();
							ImGui::SetCursorPos(pos);
						}
						_painting.set(
							Editing::map(
								rnd, ws,
								_project,
								properties().get(), _tileSize,
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

					break;
				case ASSETS_SCENE_ACTORS_LAYER: {
						const ImVec2 pos = ImGui::GetCursorPos();
						renderMap(false);
						ImGui::SetCursorPos(pos);
						if (_project->preferencesSceneShowTriggers()) {
							renderTriggers();
							ImGui::SetCursorPos(pos);
						}
						if (!ctrl.pressed() && _project->preferencesSceneShowAttributes()) {
							renderAttributes();
							ImGui::SetCursorPos(pos);
						} else if (!ctrl.pressed() && _project->preferencesSceneShowProperties()) {
							renderProperties();
							ImGui::SetCursorPos(pos);
						}
						Math::Vec2i hovering;
						_painting.set(
							Editing::actors(
								rnd, ws,
								_project,
								object().get(),
								actors().get(), _tileSize,
								width_,
								false, 255,
								&cursor, true,
								_selection.tiler.empty() ? nullptr : &_selection.tiler,
								&hovering,
								&hoveringActors,
								_tools.showGrids && _tools.gridsVisible ? &_tools.gridUnit : nullptr,
								false,
								_overlay,
								_tools.mouseActionButton
							)
						);
						ImGui::SetCursorPos(pos);

						if (!ws->bubble() && !ws->popupBox()) {
							if (hoveringActors.empty()) {
								autoSwitchToolForHovering(_tools.painting, _tools.paintingToolForActor);
							} else {
								autoSwitchToolForNonHovering(_tools.painting, _tools.paintingToolForActor);
							}
						}
						if (hoveringActors.empty() || ws->popupBox()) {
							_selection.hoveringActor.clear();
							if (_painting.up())
								_selection.selectedActor.clear();

							ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
						} else {
							_selection.hoveringActor = hoveringActors.front();
							if (_painting.up())
								_selection.selectedActor = hoveringActors.front();

							ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
						}

						_selection.actorRoutineOverridingsFilled = false;
						_selection.actorRoutineOverridings.clear();
					}

					break;
				default:
					GBBASIC_ASSERT(false && "Impossible.");

					break;
				}
				if (_tools.definitionUnfolded) {
					const scene_t &def = _tools.definitionShadow;
					Editing::camera(
						rnd, ws,
						_project,
						map().get(), _tileSize,
						Math::Vec2i(def.camera_position.x, def.camera_position.y),
						width_,
						MAGNIFICATIONS[_tools.magnification]
					);
				}
				if (
					(_painting && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) ||
					_tools.painting == Editing::Tools::STAMP
				) {
					_cursor = cursor;
				}
			} else {
				switch (_tools.layer) {
				case ASSETS_SCENE_TRIGGERS_LAYER: {
						const ImVec2 pos = ImGui::GetCursorPos();
						renderMap(_tools.showGrids && _tools.gridsVisible);
						ImGui::SetCursorPos(pos);
						if (_project->preferencesSceneShowActors()) {
							renderActors();
							ImGui::SetCursorPos(pos);
						}
						if (!ctrl.pressed() && _project->preferencesSceneShowAttributes()) {
							renderAttributes();
							ImGui::SetCursorPos(pos);
						} else if (!ctrl.pressed() && _project->preferencesSceneShowProperties()) {
							renderProperties();
							ImGui::SetCursorPos(pos);
						}
						_painting.set(
							Editing::triggers(
								rnd, ws,
								_project,
								triggers(),
								map().get(), _tileSize,
								width_,
								1.0f,
								&cursor, _selection.tiler.empty() ? nullptr : &_selection.tiler,
								&_selection.triggerCursor,
								&hoveringTriggers,
								_binding.queryTrigger,
								_tools.mouseActionButton
							)
						);
						ImGui::SetCursorPos(pos);

						if (!ws->bubble() && !ws->popupBox()) {
							if (hoveringTriggers.empty()) {
								autoSwitchToolForHovering(_tools.painting, _tools.paintingToolForTrigger);
							} else {
								autoSwitchToolForNonHovering(_tools.painting, _tools.paintingToolForTrigger);
							}
						}
						if (hoveringTriggers.empty()) {
							switch (_tools.painting) {
							case Editing::Tools::HAND:
								ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

								break;
							default:
								ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

								break;
							}
						} else {
							switch (_tools.painting) {
							case Editing::Tools::HAND:
								ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

								break;
							case Editing::Tools::SMUDGE: // Fall through.
							case Editing::Tools::ERASER:
								ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

								break;
							case Editing::Tools::MOVE:
								ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

								break;
							case Editing::Tools::RESIZE:
								ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

								break;
							default:
								// Do nothing.

								break;
							}
						}
					}

					break;
				default:
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted("...");

					break;
				}
				if (_painting) {
					_cursor = cursor;
				}
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

			_selection.paintAreaFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SameLine();
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height - statusBarHeight), true, _ref.windowFlags());
		{
			_ref.fill(rnd, _project, false);
			float xOffset = 0;
			const float spwidth = _ref.windowWidth(splitter.second);
			const float mwidth = Editing::Tools::measure(
				rnd, ws,
				&xOffset,
				nullptr,
				-1.0f
			);
			const bool paintable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
			const bool togglable = _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;

			bool inputFieldFocused = false;
			bool inputFieldFocused_ = false;
			auto canUseShortcuts = [ws, this] (void) -> bool {
				return !_tools.inputFieldFocused && ws->canUseShortcuts() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
			};

			ImGui::PushID("@Lyr");
			{
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				const char* items[] = {
					ws->theme()->windowScene_1_Actors().c_str(),
					ws->theme()->windowScene_2_Triggers().c_str(),
					ws->theme()->windowScene_3_Properties().c_str(),
					ws->theme()->windowScene_4_Attributes().c_str(),
					ws->theme()->windowScene_5_Map().c_str()
				};
				const char* tooltips[] = {
					ws->theme()->tooltip_LayerActors().c_str(),
					ws->theme()->tooltip_LayerTriggers().c_str(),
					ws->theme()->tooltip_LayerProperties().c_str(),
					ws->theme()->tooltip_LayerAttributes().c_str(),
					ws->theme()->tooltip_LayerMap().c_str()
				};

				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(mwidth);
				int layer = (GBBASIC_COUNTOF(items) - 1) - _tools.layer;
				if (ImGui::Combo("", &layer, items, GBBASIC_COUNTOF(items), tooltips)) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
					_tools.layer = (GBBASIC_COUNTOF(items) - 1) - layer;
					if (_tools.layer == ASSETS_SCENE_ACTORS_LAYER) {
						_tools.painting = _tools.paintingToolForActor;
						_cursor.size = Math::Vec2i(1, 1);
					} else if (_tools.layer == ASSETS_SCENE_TRIGGERS_LAYER) {
						_tools.painting = _tools.paintingToolForTrigger;
					} else {
						_tools.painting = _tools.paintingToolForPaintable;
						_cursor.size = Math::Vec2i(1, 1);
					}
					bindLayerTools(wnd, ws);
					if (_tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER) {
#if GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED
						if (_tools.painting == Editing::Tools::PENCIL) {
							_tools.painting = Editing::Tools::EYEDROPPER;
							_tools.paintingToolForPaintable = _tools.painting;
						} else if (_tools.painting == Editing::Tools::STAMP) {
							_tools.painting = Editing::Tools::EYEDROPPER;
							_tools.paintingToolForPaintable = _tools.painting;
						}
#else /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
						if (_tools.painting == Editing::Tools::STAMP) {
							_tools.painting = Editing::Tools::PENCIL;
							_tools.paintingToolForPaintable = _tools.painting;
						}
#endif /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
					}
					_ref.byteTooltip.clear();
					_status.clear();
					refreshStatus(wnd, rnd, ws, &_cursor);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip("(Ctrl+Shift+1/2/3/4/5)");
				}
			}
			auto toggleLayerEnabled = [&] (bool enabledLayer) -> void {
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				ImGui::SetNextItemWidth(mwidth);
				if (ImGui::Checkbox(ws->theme()->generic_Enabled(), &enabledLayer)) {
					Command* cmd = enqueue<Commands::Scene::SwitchLayer>()
						->with(_setLayerEnabled)
						->with(enabledLayer)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_WhetherToBuildThisLayer());
				}
			};
			switch (_tools.layer) {
			case ASSETS_SCENE_GRAPHICS_LAYER:
				// Do nothing.

				break;
			case ASSETS_SCENE_ATTRIBUTES_LAYER:
				toggleLayerEnabled(object()->hasAttributes());

				break;
			case ASSETS_SCENE_PROPERTIES_LAYER:
				toggleLayerEnabled(object()->hasProperties());

				break;
			case ASSETS_SCENE_ACTORS_LAYER:
				toggleLayerEnabled(object()->hasActors());

				break;
			case ASSETS_SCENE_TRIGGERS_LAYER:
				toggleLayerEnabled(object()->hasTriggers());

				break;
			}
			ImGui::PopID();
			Editing::Tools::separate(rnd, ws, spwidth);

			ImGui::PushID("@Ref");
			if (
				_tools.layer == ASSETS_SCENE_GRAPHICS_LAYER ||
				_tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER
			) {
				const float unitSize = Editing::Tools::unitSize(rnd, ws, spwidth);

				VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
				VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

				const float curPosX = ImGui::GetCursorPosX();
				ImGui::Dummy(ImVec2(xOffset, 0));
				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->iconRef()->pointer(rnd), ImVec2(unitSize, unitSize), ImColor(IM_COL32_WHITE), false, ws->theme()->tooltip_JumpToRefTiles().c_str())) {
					_ref.resolving = true;
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_ChangeRefMap());
				}
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(style.ChildBorderSize, 0));
				ImGui::SameLine();
				const float prevWidth = ImGui::GetCursorPosX() - curPosX;
				const ImVec2 size(mwidth - prevWidth + xOffset, unitSize);
				if (ImGui::Button(_ref.refMapText, size) && _ref.refMapIndex >= 0) {
					ws->category(Workspace::Categories::MAP);
					ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, _ref.refMapIndex);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_JumpToRefMap());
				}
				Editing::Tools::separate(rnd, ws, spwidth);
			} else /* if (
				_tools.layer == ASSETS_SCENE_PROPERTIES_LAYER ||
				_tools.layer == ASSETS_SCENE_ACTORS_LAYER ||
				_tools.layer == ASSETS_SCENE_TRIGGERS_LAYER
			) */ {
				// Do nothing.
			}
			ImGui::PopID();

			auto bits = [&] (bool disabled) -> void {
				if (!togglable || !currentLayer() || _status.cursor.position == Math::Vec2i(-1, -1))
					return;

#if GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED
				Byte cel = (Byte)currentLayer()->get(_status.cursor.position.x, _status.cursor.position.y);
				const std::string prompt = ws->theme()->status_Pos() + " " + Text::toString(_status.cursor.position.x) + "," + Text::toString(_status.cursor.position.y);
#else /* GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED */
				Byte cel = 0;
				switch (_tools.layer) {
				case ASSETS_SCENE_ATTRIBUTES_LAYER:
					cel = (Byte)(_ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_MAP_ATTRIBUTES_REF_WIDTH);

					break;
				case ASSETS_SCENE_PROPERTIES_LAYER:
					cel = (Byte)(_ref.propertiesCursor.unit().x + _ref.propertiesCursor.unit().y * ASSETS_MAP_ATTRIBUTES_REF_WIDTH);

					break;
				default:
					// Do nothing.

					break;
				}
				const std::string &prompt = ws->theme()->dialogPrompt_InBits();
#endif /* GBBASIC_ASSET_AUTO_MODIFY_MASK_ON_TOGGLED_ENABLED */
				const char* tooltipBitsForAttributes[8] = {
					ws->theme()->tooltipMap_Bit0().c_str(),
					ws->theme()->tooltipMap_Bit1().c_str(),
					ws->theme()->tooltipMap_Bit2().c_str(),
					ws->theme()->tooltipMap_Bit3().c_str(),
					ws->theme()->tooltipMap_Bit4().c_str(),
					ws->theme()->tooltipMap_Bit5().c_str(),
					ws->theme()->tooltipMap_Bit6().c_str(),
					ws->theme()->tooltipMap_Bit7().c_str()
				};
				const char* tooltipBitsForProperties[8] = {
					ws->theme()->tooltipScene_Bit0().c_str(),
					ws->theme()->tooltipScene_Bit1().c_str(),
					ws->theme()->tooltipScene_Bit2().c_str(),
					ws->theme()->tooltipScene_Bit3().c_str(),
					ws->theme()->tooltipScene_Bit4().c_str(),
					ws->theme()->tooltipScene_Bit5().c_str(),
					ws->theme()->tooltipScene_Bit6().c_str(),
					ws->theme()->tooltipScene_Bit7().c_str()
				};
				const char** tooltipBits = nullptr;
				switch (_tools.layer) {
				case ASSETS_SCENE_ATTRIBUTES_LAYER:
					tooltipBits = tooltipBitsForAttributes;

					break;
				case ASSETS_SCENE_PROPERTIES_LAYER:
					tooltipBits = tooltipBitsForProperties;

					break;
				default:
					// Do nothing.

					break;
				}
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
					enqueue<Commands::Scene::ToggleMask>()
						->with(
							std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
							std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
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
					_ref.propertiesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
					_ref.propertiesCursor.size = _tileSize;
				}
			};

			if (paintable) {
				switch (_tools.layer) {
				case ASSETS_SCENE_GRAPHICS_LAYER: {
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
							Editing::Brush cursor = _ref.mapCursor;
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
								_ref.mapCursor = cursor;
							if (painting)
								showRefStatus(wnd, rnd, ws, _ref.mapCursor);
						}

						statusBarActived |= ImGui::IsWindowFocused();

						ImGui::EndChild();
					}

					break;
				case ASSETS_SCENE_ATTRIBUTES_LAYER: {
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
							const UInt8 oldIdx = (UInt8)(_ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_SCENE_ATTRIBUTES_REF_WIDTH);
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
										_ref.byteTooltip += ws->theme()->tooltipScene_Attributes();
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
								const std::div_t div = std::div(idx, ASSETS_SCENE_ATTRIBUTES_REF_WIDTH);
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

					break;
				case ASSETS_SCENE_PROPERTIES_LAYER: {
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
								Editing::Brush cursor = _ref.propertiesCursor;
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
									_ref.propertiesCursor = cursor;
								if (painting)
									showRefStatus(wnd, rnd, ws, _ref.propertiesCursor);
							}

							statusBarActived |= ImGui::IsWindowFocused();

							ImGui::EndChild();
						} else {
							const UInt8 oldIdx = (UInt8)(_ref.propertiesCursor.unit().x + _ref.propertiesCursor.unit().y * ASSETS_SCENE_PROPERTIES_REF_WIDTH);
							UInt8 idx = oldIdx;
							const bool painting = Editing::Tools::byte(
								rnd, ws,
								oldIdx,
								&idx,
								spwidth,
								&inputFieldFocused_,
								disabled,
								ws->theme()->dialogPrompt_Byte().c_str(),
								[ws, this] (UInt8 props) -> const char* {
									if (_ref.byteTooltip.empty()) {
										_ref.byteTooltip += ws->theme()->tooltipScene_Properties();
										_ref.byteTooltip += "\n";

										const int blkLeftBit = (props >> GBBASIC_SCENE_BLOCKING_LEFT_BIT) & 0x00000001;
										const int blkRightBit = (props >> GBBASIC_SCENE_BLOCKING_RIGHT_BIT) & 0x00000001;
										const int blkUpBit = (props >> GBBASIC_SCENE_BLOCKING_UP_BIT) & 0x00000001;
										const int blkDownBit = (props >> GBBASIC_SCENE_BLOCKING_DOWN_BIT) & 0x00000001;
										const int isLadderBit = (props >> GBBASIC_SCENE_IS_LADDER_BIT) & 0x00000001;
										_ref.byteTooltip += Text::format(ws->theme()->tooltipScene_BitBlockingLeft(), { Text::toString(blkLeftBit) });
										_ref.byteTooltip += Text::format(ws->theme()->tooltipScene_BitBlockingRight(), { Text::toString(blkRightBit) });
										_ref.byteTooltip += Text::format(ws->theme()->tooltipScene_BitBlockingUp(), { Text::toString(blkUpBit) });
										_ref.byteTooltip += Text::format(ws->theme()->tooltipScene_BitBlockingDown(), { Text::toString(blkDownBit) });
										_ref.byteTooltip += Text::format(ws->theme()->tooltipScene_BitIsLadder(), { Text::toString(isLadderBit) });
									}

									return _ref.byteTooltip.c_str();
								}
							);
							if (painting) {
								const std::div_t div = std::div(idx, ASSETS_SCENE_PROPERTIES_REF_WIDTH);
								_ref.propertiesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
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

					break;
				default:
					// Do nothing.

					break;
				}

				const unsigned mask = _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER ?
					0xffffffff & ~(1 << Editing::Tools::STAMP) :
					0xffffffff;
				Editing::Tools::separate(rnd, ws, spwidth);
				if (Editing::Tools::paintable(rnd, ws, &_tools.painting, -1.0f, canUseShortcuts(), false, mask)) {
					_tools.paintingToolForPaintable = _tools.painting;

					if (_tools.painting == Editing::Tools::STAMP) {
						_processors[Editing::Tools::STAMP] = Processor{
							std::bind(&EditorSceneImpl::stampToolDown, this, std::placeholders::_1),
							nullptr,
							std::bind(&EditorSceneImpl::stampToolUp, this, std::placeholders::_1),
							std::bind(&EditorSceneImpl::stampToolHover, this, std::placeholders::_1)
						};
					} else {
						_ref.mapCursor.size = _tileSize;
						_ref.attributesCursor.size = _tileSize;
						_ref.propertiesCursor.size = _tileSize;
					}

					if (_tools.painting == Editing::Tools::STAMP) {
						_tools.bitwiseOperations = Editing::Tools::SET;
					}

					destroyOverlay();
				}
			}

			switch (_tools.layer) {
			case ASSETS_SCENE_ACTORS_LAYER: {
					// Actor selection for putting.
					if (_tools.painting == Editing::Tools::SMUDGE) {
						Byte cel = (Byte)_tools.actorTemplate;
						ImGui::Dummy(ImVec2(xOffset, 0));
						ImGui::SameLine();
						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted(ws->theme()->dialogPrompt_SelectActor());
						if (ImGui::IsItemHovered()) {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							ImGui::SetTooltip(ws->theme()->tooltip_Space());
						}
						const bool changed = Editing::Tools::actable(
							rnd, ws,
							_project,
							spwidth,
							&cel
						);
						if (changed) {
							_tools.actorTemplate = cel;
						}
						ImGui::SameLine();
						ImGui::NewLine();
						ImGui::NewLine(1);
					}

					// The painting tools.
					if (Editing::Tools::actor(rnd, ws, &_tools.painting, -1.0f, canUseShortcuts())) {
						_tools.paintingToolForActor = _tools.painting;

						_ref.mapCursor.size = _tileSize;
						_ref.attributesCursor.size = _tileSize;
						_ref.propertiesCursor.size = _tileSize;

						destroyOverlay();
					}

					// Tooltip.
					const Editing::ActorIndex &idx = _selection.hoveringActor;
					const ActorAssets::Entry* entry_ = _project->getActor(idx.cel);
					if (entry_ && !ImGui::IsPopupOpen("_", ImGuiPopupFlags_AnyPopup)) {
						const Math::Vec2i &pos = cursor.position;
						if (_selection.hoveringActorTipsPosition != pos) {
							_selection.hoveringActorTipsPosition = pos;
							const active_t def = entry_->definition;
							const char* updateCallback = getActorUpdateRoutine(entry_, pos);
							const char* onHitsCallback = getActorOnHitsRoutine(entry_, pos);
							_selection.hoveringActorTips = Text::format(
								ws->theme()->tooltipScene_ActorDetails(),
								{
									Text::toString(idx.cel),
									Text::toString(cursor.position.x), Text::toString(cursor.position.y),
									Text::toString(def.bounds.left), Text::toString(def.bounds.top), Text::toString(def.bounds.right), Text::toString(def.bounds.bottom),
									std::string(updateCallback ? updateCallback : "-"),
									std::string(onHitsCallback ? onHitsCallback : "-")
								}
							);
						}
						if (!idx.invalid() && !_selection.hoveringActorTips.empty()) {
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

							ImGui::SetTooltip(_selection.hoveringActorTips);
						}
					} else {
						_selection.hoveringActorTips.clear();
						_selection.hoveringActorTipsPosition = Math::Vec2i(-1, -1);
					}
				}

				break;
			case ASSETS_SCENE_TRIGGERS_LAYER: {
					if (Editing::Tools::trigger(rnd, ws, &_tools.painting, -1.0f, canUseShortcuts())) {
						_tools.paintingToolForTrigger = _tools.painting;

						_ref.mapCursor.size = _tileSize;
						_ref.attributesCursor.size = _tileSize;
						_ref.propertiesCursor.size = _tileSize;

						destroyOverlay();
					}

					Byte idx = Scene::INVALID_TRIGGER();
					const bool withTooltip = !hoveringTriggers.empty() && !_selection.contextTriggerOpened;
					if (hoveringTriggers.empty()) {
						queryTrigger(_cursor.position, hoveringTriggers);
					}
					if (!hoveringTriggers.empty()) {
						for (const Editing::TriggerIndex &idx_ : hoveringTriggers) {
							Trigger trigger;
							if (getTrigger(idx_.cel, &trigger)) {
								if (_cursor.position == trigger.position() && _cursor.size == trigger.size()) {
									idx = idx_.cel;

									break;
								}
							}
						}

						if (idx == Scene::INVALID_TRIGGER())
							idx = hoveringTriggers.front().cel;
					}
					if (_selection.hoveringTrigger != idx) {
						_selection.hoveringTrigger = idx;
						Trigger trigger;
						if (getTrigger(_selection.hoveringTrigger, &trigger)) {
							_selection.hoveringTriggerTips = Text::format(
								ws->theme()->tooltipScene_TriggerDetails(),
								{
									Text::toString(_selection.hoveringTrigger),
									Text::toString(trigger.xMin()), Text::toString(trigger.yMin()),
									Text::toString(trigger.width()), Text::toString(trigger.height())
								}
							);
						} else {
							_selection.hoveringTriggerTips.clear();
						}

						_selection.triggerColorFilled = false;
						_selection.triggerRoutinesFilled = false;

						_status.clear();
						refreshStatus(wnd, rnd, ws, nullptr);
					}
					if (_selection.hoveringTrigger != Scene::INVALID_TRIGGER() && withTooltip && !ImGui::IsPopupOpen("_", ImGuiPopupFlags_AnyPopup)) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip(_selection.hoveringTriggerTips);
					}
				}

				break;
			default:
				// Do nothing.

				break;
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

			if (paintable) {
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

				if (!_tools.post) {
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
			}

			switch (_tools.layer) {
			case ASSETS_SCENE_ACTORS_LAYER: {
					Math::Vec2i pos_ = _cursor.position;
					if (!_selection.hoveringActor.invalid()) {
						pos_ = _status.cursor.position;
					} else {
						const Editing::ActorIndex &idx = _selection.selectedActor;
						if (idx.invalid()) {
							break;
						}

						const ActorAssets::Entry* entry_ = _project->getActor(idx.cel);
						if (!entry_)
							break;
					}

					const ActorRoutineOverriding::Array &actorRoutines = entry()->actorRoutineOverridings;
					const Math::Vec2i pos = getActorPositionUnderCursor(pos_);
					const ActorRoutineOverriding* overriding = Routine::search(actorRoutines, pos);
					ActorRoutineOverriding overridingl;
					if (!overriding)
						overriding = &overridingl;

					Editing::Tools::separate(rnd, ws, spwidth);
					if (!_selection.actorRoutineOverridingsFilled) {
						_selection.actorRoutineOverridings = { overriding->routines.update, overriding->routines.onHits };
						_selection.actorRoutineOverridingsFilled = true;
					}
					const Editing::Tools::InvokableList oldRoutines = { &overriding->routines.update, &overriding->routines.onHits };
					const Editing::Tools::InvokableList types = { &ws->theme()->dialogPrompt_Behave(), &ws->theme()->dialogPrompt_OnHits() };
					const Editing::Tools::InvokableList tooltips = { &ws->theme()->tooltip_OverrideThreaded(), &ws->theme()->tooltip_OverrideThreaded() };
					float height__ = 0.0f;
					const bool changed = Editing::Tools::invokable(
						rnd, ws,
						_project,
						&oldRoutines, &_selection.actorRoutineOverridings,
						&types, &tooltips,
						&_tools.routinesUnfolded,
						spwidth, &height__,
						ws->theme()->dialogPrompt_Routines().c_str(),
						nullptr,
						[wnd, rnd, ws, this, pos, overriding] (int index) -> void {
							ws->showCodeBindingEditor(
								wnd, rnd,
								std::bind(&EditorSceneImpl::bindActorCallback, this, pos, index, std::placeholders::_1),
								index == 0 ? overriding->routines.update : overriding->routines.onHits
							);
						}
					);
					if (changed) {
						_selection.actorRoutineOverridingsFilled = false;

						ActorRoutineOverriding::Array actorRoutines_ = actorRoutines;
						ActorRoutineOverriding* overriding_ = Routine::search(actorRoutines_, pos);

						const bool hasNonBlank = std::any_of(
							_selection.actorRoutineOverridings.begin(), _selection.actorRoutineOverridings.end(),
							[] (const std::string &str) {
								return !str.empty();
							}
						);
						if (hasNonBlank) {
							if (!overriding) {
								overriding_ = Routine::add(actorRoutines_, pos);
								GBBASIC_ASSERT(overriding_ && "Impossible.");
								if (!overriding_)
									return;
							}
							overriding_->routines.update = _selection.actorRoutineOverridings.size() >= 1 ? _selection.actorRoutineOverridings[0] : "";
							overriding_->routines.onHits = _selection.actorRoutineOverridings.size() >= 2 ? _selection.actorRoutineOverridings[1] : "";
						} else {
							if (overriding) {
								const bool ret = Routine::remove(actorRoutines_, pos);
								GBBASIC_ASSERT(ret && "Impossible.");
								if (!ret)
									return;
							}
						}
						Commands::Scene::SetActorRoutineOverridings* cmd = (Commands::Scene::SetActorRoutineOverridings*)enqueue<Commands::Scene::SetActorRoutineOverridings>()
							->with(actorRoutines_)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);
					}
				}

				break;
			case ASSETS_SCENE_TRIGGERS_LAYER: {
					if (_selection.hoveringTrigger == Scene::INVALID_TRIGGER())
						break;

					Trigger trigger;
					if (!getTrigger(_selection.hoveringTrigger, &trigger))
						break;

					Editing::Tools::separate(rnd, ws, spwidth);
					if (!_selection.triggerColorFilled) {
						_selection.triggerColor = trigger.color;
						_selection.triggerColorFilled = true;
					}
					if (Editing::Tools::colorable(rnd, ws, trigger.color, _selection.triggerColor, false, -1.0f, &inputFieldFocused_, ws->theme()->dialogPrompt_SetColor().c_str())) {
						_selection.triggerColorFilled = false;
						trigger.color = _selection.triggerColor;
						Commands::Scene::SetTriggerColor* cmd = (Commands::Scene::SetTriggerColor*)enqueue<Commands::Scene::SetTriggerColor>()
							->with(_binding.getTrigger, _binding.setTrigger)
							->with(_selection.hoveringTrigger, trigger)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);
					}
					inputFieldFocused |= inputFieldFocused_;

					Editing::Tools::separate(rnd, ws, spwidth);
					if (!_selection.triggerRoutinesFilled) {
						_selection.triggerRoutines = { trigger.eventRoutine };
						_selection.triggerRoutinesFilled = true;
					}
					const Editing::Tools::InvokableList oldRoutines = { &trigger.eventRoutine };
					const Editing::Tools::InvokableList types = { &ws->theme()->dialogPrompt_OnHits() };
					const Editing::Tools::InvokableList tooltips = { &ws->theme()->tooltip_Threaded() };
					float height_ = 0.0f;
					const bool changed = Editing::Tools::invokable(
						rnd, ws,
						_project,
						&oldRoutines, &_selection.triggerRoutines,
						&types, &tooltips,
						&_tools.routinesUnfolded,
						spwidth, &height_,
						ws->theme()->dialogPrompt_Routines().c_str(),
						[ws, &style, &trigger] (float xOffset, float width) -> bool {
							ImGui::Dummy(ImVec2(xOffset, 0));
							ImGui::SameLine();
							ImGui::AlignTextToFramePadding();
							ImGui::TextUnformatted(ws->theme()->dialogPrompt_HitType());

							const char* items[] = {
								ws->theme()->windowScene_TriggerEventTypeNone().c_str(),
								ws->theme()->windowScene_TriggerEventTypeEnter().c_str(),
								ws->theme()->windowScene_TriggerEventTypeLeave().c_str(),
								ws->theme()->windowScene_TriggerEventTypeBoth().c_str()
							};
							ImGui::Dummy(ImVec2(xOffset, 0));
							ImGui::SameLine();
							ImGui::SetNextItemWidth(width);
							int type = 0;
							switch (trigger.eventType) {
							case TRIGGER_HAS_NONE:                                    type = 0; break;
							case TRIGGER_HAS_ENTER_SCRIPT:                            type = 1; break;
							case TRIGGER_HAS_LEAVE_SCRIPT:                            type = 2; break;
							case TRIGGER_HAS_ENTER_SCRIPT | TRIGGER_HAS_LEAVE_SCRIPT: type = 3; break;
							default: GBBASIC_ASSERT(false && "Wrong data.");          type = 0; break;
							}
							VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
							VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));
							if (ImGui::Combo("", &type, items, GBBASIC_COUNTOF(items))) {
								switch (type) {
								case 0: trigger.eventType = TRIGGER_HAS_NONE;                                    break;
								case 1: trigger.eventType = TRIGGER_HAS_ENTER_SCRIPT;                            break;
								case 2: trigger.eventType = TRIGGER_HAS_LEAVE_SCRIPT;                            break;
								case 3: trigger.eventType = TRIGGER_HAS_ENTER_SCRIPT | TRIGGER_HAS_LEAVE_SCRIPT; break;
								}

								return true;
							}

							return false;
						},
						[wnd, rnd, ws, this, trigger] (int) -> void {
							ws->showCodeBindingEditor(
								wnd, rnd,
								std::bind(&EditorSceneImpl::bindTriggerCallback, this, trigger, std::placeholders::_1),
								trigger.eventRoutine
							);
						}
					);
					if (changed) {
						_selection.triggerRoutinesFilled = false;
						if (_selection.triggerRoutines.empty())
							trigger.eventRoutine.clear();
						else
							trigger.eventRoutine = _selection.triggerRoutines.front();
						Commands::Scene::SetTriggerRoutines* cmd = (Commands::Scene::SetTriggerRoutines*)enqueue<Commands::Scene::SetTriggerRoutines>()
							->with(_binding.getTrigger, _binding.setTrigger)
							->with(_selection.hoveringTrigger, trigger)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);
					}
				}

				break;
			default:
				// Do nothing.

				break;
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
				if (_project->canRenameScene(_index, _tools.namableText, nullptr)) {
					Command* cmd = enqueue<Commands::Scene::SetName>()
						->with(_tools.namableText)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				} else {
					_tools.namableText = entry()->name;

					warn(ws, ws->theme()->warning_SceneNameIsAlreadyInUse(), true);
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			int selObj = -1;
			if (
				Editing::Tools::listable(
					rnd, ws,
					_objects,
					&selObj,
					spwidth,
					ws->theme()->dialogPrompt_ObjectList().c_str()
				)
			) {
				if (selObj >= 0 && selObj < (int)_objects.indices.size()) {
					const Editing::Tools::Objects::Entry &entry = _objects.indices[selObj];
					if (entry.category == AssetsBundle::Categories::ACTOR) {
						GBBASIC_ASSERT(entry.index.isLeft() && "Impossible.");

						changeLayer(wnd, rnd, ws, ASSETS_SCENE_ACTORS_LAYER);

						const Math::Vec2i pos = entry.index.left().get();
						const Byte cel = (Byte)actors()->get(pos.x, pos.y);
						_selection.hoveringActor = Editing::ActorIndex(cel, Math::Vec2i(0, 0));
						_selection.selectedActor = Editing::ActorIndex(cel, Math::Vec2i(0, 0));
						_cursor.position = pos;
						_cursor.size = Math::Vec2i(1, 1);
					} else if (entry.category == AssetsBundle::Categories::TRIGGER) {
						GBBASIC_ASSERT(entry.index.isRight() && "Impossible.");

						changeLayer(wnd, rnd, ws, ASSETS_SCENE_TRIGGERS_LAYER);

						const int idx = entry.index.right().get();
						const Trigger &trigger = (*triggers())[idx];
						_selection.hoveringTrigger = Scene::INVALID_TRIGGER(); // No need to set to `(Byte)idx`.
						_selection.triggerCursor = trigger.position();
						_cursor.position = trigger.position();
						_cursor.size = trigger.size();
					} else {
						GBBASIC_ASSERT(false && "Not supported.");
					}
				}
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::definable(
					rnd, ws,
					object().get(),
					nullptr /* &entry()->definition */, &_tools.definitionShadow,
					&_tools.definitionUnfolded,
					spwidth, &_tools.definitionHeight,
					&inputFieldFocused_,
					ws->theme()->dialogPrompt_Definition().c_str(),
					[this] (Editing::Operations op) -> void {
						switch (op) {
						case Editing::Operations::COPY: // Copy.
							do {
								rapidjson::Document doc;
								if (!_tools.definitionShadow.toJson(doc, doc))
									break;

								std::string str;
								if (!Json::toString(doc, str))
									break;

								const std::string osstr = Unicode::toOs(str);

								Platform::setClipboardText(osstr.c_str());
							} while (false);

							break;
						case Editing::Operations::PASTE: // Paste.
							do {
								if (!Platform::hasClipboardText())
									break;

								std::string osstr = Platform::getClipboardText();
								const std::string txt = Unicode::fromOs(osstr);

								rapidjson::Document doc;
								if (!Json::fromString(doc, txt.c_str()))
									break;

								if (!_tools.definitionShadow.fromJson(doc))
									break;

								Command* cmd = enqueue<Commands::Scene::SetDefinition>()
									->with(_tools.definitionShadow)
									->exec(object(), Variant((void*)entry()));

								_refresh(cmd);

								simplify();
							} while (false);

							break;
						default:
							GBBASIC_ASSERT(false && "Impossible.");

							break;
						}
					}
				)
			) {
				Command* cmd = enqueue<Commands::Scene::SetDefinition>()
					->with(_tools.definitionShadow)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);

				simplify();
			}
			inputFieldFocused |= inputFieldFocused_;

			_tools.inputFieldFocused = inputFieldFocused;

			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

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
		_tools.stopSceneTesting();
	}
	virtual void stopped(class Renderer*, class Workspace*) override {
		_tools.stopSceneTesting();
	}

	virtual void resized(class Renderer*, const Math::Vec2i &, const Math::Vec2i &) override {
		entry()->cleanup(); // Clean up the outdated editable and runtime resources.

		_ref.windowResized();
	}

	virtual void focusLost(class Renderer*) override {
		entry()->cleanup(); // Clean up the outdated editable and runtime resources.
	}
	virtual void focusGained(class Renderer*) override {
		// Do nothing.
	}

private:
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

			return true;
		}

		const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true, false);
		const Editing::Shortcut alt(SDL_SCANCODE_UNKNOWN, false, false, true);
		if (shift.pressed(false) && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (!_tools.post) {
				const Editing::Tools::PaintableTools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::HAND;

				_tools.post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
						_tools.painting = prevTool;
				};
				_tools.postType = Editing::Tools::HAND;
			}

			return true;
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
			if (!_tools.post) {
				const Editing::Tools::PaintableTools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::HAND;

				_tools.mouseActionButton = ImGuiMouseButton_Middle;

				_tools.post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
						_tools.painting = prevTool;
						_tools.mouseActionButton = ImGuiMouseButton_Left;
					}
				};
				_tools.postType = Editing::Tools::HAND;
			}

			return true;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
			if (_tools.post && _tools.postType == Editing::Tools::HAND) {
				_tools.post();
				_tools.post = nullptr;

				return false;
			}
		} else if (shift.released()) {
			if (_tools.post && _tools.postType == Editing::Tools::HAND) {
				_tools.post();
				_tools.post = nullptr;
			}
		}
		if (alt.pressed(false)) {
			if (!_tools.post) {
				const Editing::Tools::PaintableTools prevTool = _tools.painting;

				_tools.painting = Editing::Tools::EYEDROPPER;

				_tools.post = [&, prevTool] (void) -> void {
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
						_tools.painting = prevTool;
				};
				_tools.postType = Editing::Tools::EYEDROPPER;
			}

			return true;
		} else if (alt.released()) {
			if (_tools.post && _tools.postType == Editing::Tools::EYEDROPPER) {
				_tools.post();
				_tools.post = nullptr;
			}
		}

		const Editing::Shortcut num1(SDL_SCANCODE_1, true, true);
		const Editing::Shortcut num2(SDL_SCANCODE_2, true, true);
		const Editing::Shortcut num3(SDL_SCANCODE_3, true, true);
		const Editing::Shortcut num4(SDL_SCANCODE_4, true, true);
		const Editing::Shortcut num5(SDL_SCANCODE_5, true, true);
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (num1.pressed())
				changeLayer(wnd, rnd, ws, ASSETS_SCENE_ACTORS_LAYER);
			else if (num2.pressed())
				changeLayer(wnd, rnd, ws, ASSETS_SCENE_TRIGGERS_LAYER);
			else if (num3.pressed())
				changeLayer(wnd, rnd, ws, ASSETS_SCENE_PROPERTIES_LAYER);
			else if (num4.pressed())
				changeLayer(wnd, rnd, ws, ASSETS_SCENE_ATTRIBUTES_LAYER);
			else if (num5.pressed())
				changeLayer(wnd, rnd, ws, ASSETS_SCENE_GRAPHICS_LAYER);
		}

		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (esc.pressed())
			_selection.clear();

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
					index = _project->scenePageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::SCENE, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->scenePageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::SCENE, index);
			}
		}

		const Editing::Shortcut del(SDL_SCANCODE_DELETE);
		if (del.pressed() && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (_tools.layer == ASSETS_SCENE_ACTORS_LAYER) {
				const Math::Vec2i pos = getActorPositionUnderCursor(_cursor.position);
				const Byte cel = (Byte)currentLayer()->get(pos.x, pos.y);
				if (cel != Scene::INVALID_ACTOR()) {
					ActorRoutineOverriding::Array actorRoutines = entry()->actorRoutineOverridings;
					Routine::remove(actorRoutines, pos);

					Commands::Scene::SetActor* cmd = (Commands::Scene::SetActor*)enqueue<Commands::Scene::SetActor>()
						->with(actorRoutines)
						->with(
							std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
							std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
							1
						)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()));
					cmd->with(
						Math::Vec2i(object()->width(), object()->height()),
						pos, Scene::INVALID_ACTOR(),
						nullptr
					);
					if (!_commands->empty())
						_commands->back()->redo(object(), Variant((void*)entry()));

					_refresh(cmd);
				}
			} else if (_tools.layer == ASSETS_SCENE_TRIGGERS_LAYER) {
				Editing::TriggerIndex::Array triggers_;
				if (_selection.triggerCursor == Math::Vec2i(-1, -1) && _cursor.position != Math::Vec2i(-1, -1))
					_selection.triggerCursor = _cursor.position;
				if (queryTrigger(_selection.triggerCursor, triggers_) > 0) {
					const Editing::TriggerIndex &idx = triggers_.front();
					Commands::Scene::DeleteTrigger* cmd = (Commands::Scene::DeleteTrigger*)enqueue<Commands::Scene::DeleteTrigger>()
						->with(_binding.addTrigger, _binding.removeTrigger)
						->with(idx.cel)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				}
			}
		}

		const Editing::Shortcut space(SDL_SCANCODE_SPACE);
		if (space.pressed(false) && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (_tools.layer == ASSETS_SCENE_ACTORS_LAYER) {
				if (_tools.painting == Editing::Tools::SMUDGE) {
					ImGui::ActorSelectorPopupBox::ConfirmedHandler confirm = ImGui::ActorSelectorPopupBox::ConfirmedHandler(
						[&, ws] (int index) -> void {
							_tools.actorTemplate = index;

							ws->popupBox(nullptr);
						}
					);
					ws->showActorSelector(
						rnd,
						_tools.actorTemplate == Scene::INVALID_ACTOR() ? -1 : _tools.actorTemplate,
						confirm,
						nullptr
					);
				}
			}
		}

		return true;
	}

	void context(Window* wnd, Renderer* rnd, Workspace* ws) {
		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER: // Fall through.
		case ASSETS_SCENE_ATTRIBUTES_LAYER: {
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
					ImGui::Separator();

					if (ImGui::MenuItem(ws->theme()->menu_JumpToTheMap())) {
						_selection.clear();

						ws->category(Workspace::Categories::MAP);
						ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, _ref.refMapIndex);
					}

					ImGui::EndPopup();
				}
			}

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER: {
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
					auto assignBit = [] (int &props, int bit, bool on) -> void {
						props &= ~((Byte)1 << bit);
						if (on)
							props |= (Byte)1 << bit;
					};

					int props = currentLayer()->get(_cursor.position.x, _cursor.position.y);
					const int blkLeftBit = (props >> GBBASIC_SCENE_BLOCKING_LEFT_BIT) & 0x00000001;
					const int blkRightBit = (props >> GBBASIC_SCENE_BLOCKING_RIGHT_BIT) & 0x00000001;
					const int blkUpBit = (props >> GBBASIC_SCENE_BLOCKING_UP_BIT) & 0x00000001;
					const int blkDownBit = (props >> GBBASIC_SCENE_BLOCKING_DOWN_BIT) & 0x00000001;
					const int isLadderBit = (props >> GBBASIC_SCENE_IS_LADDER_BIT) & 0x00000001;

					bool reassign = false;
					if (ImGui::MenuItem(ws->theme()->menu_BlockingLeft(), nullptr, !!blkLeftBit)) {
						assignBit(props, GBBASIC_SCENE_BLOCKING_LEFT_BIT, !blkLeftBit);
						reassign = true;
					}
					if (ImGui::MenuItem(ws->theme()->menu_BlockingRight(), nullptr, !!blkRightBit)) {
						assignBit(props, GBBASIC_SCENE_BLOCKING_RIGHT_BIT, !blkRightBit);
						reassign = true;
					}
					if (ImGui::MenuItem(ws->theme()->menu_BlockingUp(), nullptr, !!blkUpBit)) {
						assignBit(props, GBBASIC_SCENE_BLOCKING_UP_BIT, !blkUpBit);
						reassign = true;
					}
					if (ImGui::MenuItem(ws->theme()->menu_BlockingDown(), nullptr, !!blkDownBit)) {
						assignBit(props, GBBASIC_SCENE_BLOCKING_DOWN_BIT, !blkDownBit);
						reassign = true;
					}
					if (ImGui::MenuItem(ws->theme()->menu_IsLadder(), nullptr, !!isLadderBit)) {
						assignBit(props, GBBASIC_SCENE_IS_LADDER_BIT, !isLadderBit);
						reassign = true;
					}
					if (reassign) {
						enqueue<Commands::Scene::Pencil>()
							->with(
								std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
								std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
								_tools.weighting + 1
							)
							->with(_tools.bitwiseOperations)
							->with(
								Math::Vec2i(object()->width(), object()->height()),
								_cursor.position, props,
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
		case ASSETS_SCENE_ACTORS_LAYER: {
				const Project::Ptr &prj = ws->currentProject();
				if (prj->actorPageCount() == 0)
					break;

				const Math::Vec2i pos = getActorPositionUnderCursor(_status.cursor.position);
				Byte cel = Scene::INVALID_ACTOR();
				if (_selection.paintAreaFocused) {
					cel = (Byte)currentLayer()->get(pos.x, pos.y);
					if (cel == Scene::INVALID_ACTOR())
						cel = (Byte)currentLayer()->get(_status.cursor.position.x, _status.cursor.position.y);
				}

				ImGuiStyle &style = ImGui::GetStyle();

				if (ws->canUseShortcuts() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					_painting.clear();

					if (cel != Scene::INVALID_ACTOR()) {
						ws->category(Workspace::Categories::ACTOR);
						ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, cel);
					}
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					_cursor = _status.cursor;

					if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
						ImGui::OpenPopup("@Ctx");

						ws->bubble(nullptr);
					}
				}

				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				if (ImGui::BeginPopup("@Ctx")) {
					int newCel = -1;
					int unknownCount = 0;
					char unknownBuf[32];
					for (int i = 0; i < (int)(prj->actorPageCount() + 1); ++i) {
						if (i == 0) {
							if (cel != Scene::INVALID_ACTOR()) {
								if (ImGui::MenuItem(ws->theme()->generic_None())) {
									newCel = Scene::INVALID_ACTOR();
								}
							}

							continue;
						}

						const int idx = i - 1;
						const ActorAssets::Entry* entry = prj->getActor(idx);
						const char* entry_ = nullptr;
						if (entry->name.empty()) {
							snprintf(unknownBuf, GBBASIC_COUNTOF(unknownBuf), "Unknown %d", unknownCount++);
							entry_ = unknownBuf;
						} else {
							entry_ = entry->name.c_str();
						}
						if (ImGui::MenuItem(entry_, nullptr, nullptr, idx != cel)) {
							newCel = idx;
						}
					}
					if (cel != Scene::INVALID_ACTOR()) {
						ImGui::Separator();
						if (ImGui::MenuItem(ws->theme()->menu_JumpToTheActor())) {
							ws->category(Workspace::Categories::ACTOR);
							ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, cel);
						}

						if (ImGui::BeginMenu(ws->theme()->menu_Code())) {
							if (ImGui::MenuItem(ws->theme()->menu_BindUpdate())) {
								ws->category(Workspace::Categories::ACTOR);
								ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, cel);

								Editable* editor = ws->touchActorEditor(wnd, rnd, _project, cel, nullptr);
								if (editor)
									editor->post(Editable::BIND_CALLBACK, (Variant::Int)0);
							}
							if (ImGui::MenuItem(ws->theme()->menu_BindOnHits())) {
								ws->category(Workspace::Categories::ACTOR);
								ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, cel);

								Editable* editor = ws->touchActorEditor(wnd, rnd, _project, cel, nullptr);
								if (editor)
									editor->post(Editable::BIND_CALLBACK, (Variant::Int)1);
							}

							ImGui::EndMenu();
						}
					}

					ImGui::EndPopup();

					if (newCel != -1) {
						ActorRoutineOverriding::Array actorRoutines = entry()->actorRoutineOverridings;
						if (newCel == Scene::INVALID_ACTOR())
							Routine::remove(actorRoutines, pos);

						Commands::Scene::SetActor* cmd = (Commands::Scene::SetActor*)enqueue<Commands::Scene::SetActor>()
							->with(actorRoutines)
							->with(
								std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
								std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
								1
							)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));
						cmd->with(
							Math::Vec2i(object()->width(), object()->height()),
							pos, newCel,
							nullptr
						);
						if (!_commands->empty())
							_commands->back()->redo(object(), Variant((void*)entry()));

						_refresh(cmd);
					}
				}
			}

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER: {
				ImGuiStyle &style = ImGui::GetStyle();

				if (ws->canUseShortcuts() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					_painting.clear();

					_cursor = _status.cursor;
					_selection.contextTrigger = Scene::INVALID_TRIGGER();

					Editing::TriggerIndex::Array hoveringTriggers;
					queryTrigger(_selection.triggerCursor, hoveringTriggers);
					if (hoveringTriggers.empty())
						break;

					const Editing::TriggerIndex &idx = hoveringTriggers.front();
					Trigger trigger;
					if (!getTrigger(idx.cel, &trigger))
						break;

					_selection.contextTrigger = idx.cel;

					const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);
					if (ctrl.pressed()) {
						Editing::Tools::CodeIndex index = Editing::Tools::CodeIndex();
						index.set(trigger.eventRoutine);

						if (index.page >= 0) {
							ws->category(Workspace::Categories::CODE);
							ws->changePage(wnd, rnd, _project, Workspace::Categories::CODE, index.page);
						}
					} else {
						Trigger trigger;
						if (getTrigger(_selection.contextTrigger, &trigger)) {
							ws->showCodeBindingEditor(
								wnd, rnd,
								std::bind(&EditorSceneImpl::bindTriggerCallback, this, trigger, std::placeholders::_1),
								trigger.eventRoutine
							);
						}
					}
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					_cursor = _status.cursor;
					_selection.contextTrigger = Scene::INVALID_TRIGGER();

					Editing::TriggerIndex::Array hoveringTriggers;
					queryTrigger(_selection.triggerCursor, hoveringTriggers);
					if (!hoveringTriggers.empty()) {
						const Editing::TriggerIndex &idx = hoveringTriggers.front();
						Trigger trigger;
						if (getTrigger(idx.cel, &trigger)) {
							_selection.contextTrigger = idx.cel;
						}
					}

					if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
						ImGui::OpenPopup("@Ctx");

						ws->bubble(nullptr);
					}
				}

				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				const bool canAdd = _selection.contextTrigger == Scene::INVALID_TRIGGER();
				const bool canEdit = _selection.contextTrigger != Scene::INVALID_TRIGGER();
				_selection.contextTriggerOpened = ImGui::BeginPopup("@Ctx");
				if (_selection.contextTriggerOpened) {
					const int n = (int)triggers()->size();
					if (ImGui::MenuItem(ws->theme()->menu_New(), nullptr, nullptr, canAdd)) {
						const Math::Vec2i &cursor = _selection.triggerCursor;
						const int index = n;
						Trigger trigger = Trigger::byXYWH(cursor.x, cursor.y, 1, 1);
						trigger.color = Colour::randomize(255);
						Commands::Scene::AddTrigger* cmd = (Commands::Scene::AddTrigger*)enqueue<Commands::Scene::AddTrigger>()
							->with(_binding.addTrigger, _binding.removeTrigger)
							->with(index, trigger)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);
					}
					if (ImGui::MenuItem(ws->theme()->menu_Delete(), nullptr, nullptr, canEdit)) {
						Commands::Scene::DeleteTrigger* cmd = (Commands::Scene::DeleteTrigger*)enqueue<Commands::Scene::DeleteTrigger>()
							->with(_binding.addTrigger, _binding.removeTrigger)
							->with(_selection.contextTrigger)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);
					}
					ImGui::Separator();
					if (ImGui::BeginMenu(ws->theme()->menu_SwapOrder(), canEdit && n > 1)) {
						for (int i = 0; i < n; ++i) {
							const std::string idx = Text::toString(i);
							if (i == _selection.contextTrigger) {
								ImGui::MenuItem(idx, nullptr, nullptr, false);
							} else {
								if (ImGui::MenuItem(idx)) {
									Commands::Scene::SwapTrigger* cmd = (Commands::Scene::SwapTrigger*)enqueue<Commands::Scene::SwapTrigger>()
										->with(_binding.getTrigger, _binding.setTrigger)
										->with(_selection.contextTrigger, i)
										->with(_setLayer, _tools.layer)
										->exec(object(), Variant((void*)entry()));

									_refresh(cmd);
								}
							}
						}

						ImGui::EndMenu();
					}
					ImGui::Separator();
					if (ImGui::BeginMenu(ws->theme()->menu_Code(), canEdit)) {
						if (ImGui::MenuItem(ws->theme()->menu_TriggerCallback())) {
							std::string txt;
							entry()->serializeBasicForTriggerCallback(txt, _selection.contextTrigger);

							const std::string osstr = Unicode::toOs(txt);

							Platform::setClipboardText(osstr.c_str());

							ws->bubble(ws->theme()->dialogPrompt_CopiedCodeToClipboard(), nullptr);
						}
						if (ImGui::MenuItem(ws->theme()->menu_BindOnTrigger())) {
							Trigger trigger;
							if (getTrigger(_selection.contextTrigger, &trigger)) {
								ws->showCodeBindingEditor(
									wnd, rnd,
									std::bind(&EditorSceneImpl::bindTriggerCallback, this, trigger, std::placeholders::_1),
									trigger.eventRoutine
								);
							}
						}

						ImGui::EndMenu();
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

			switch (_tools.layer) {
			case ASSETS_SCENE_ACTORS_LAYER:
				if (_status.cursor.position.x == -1 || _status.cursor.position.y == -1 || !currentLayer()) {
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

					if (!_selection.hoveringActor.invalid()) {
						_status.text += " ";

						_status.text += ws->theme()->status_Tl();
						_status.text += " ";
						const Editing::ActorIndex &idx = _selection.hoveringActor;
						_status.text += Text::toString(idx.cel);
					}
				}

				break;
			case ASSETS_SCENE_TRIGGERS_LAYER:
				_status.text = " " + ws->theme()->status_Pos();
				_status.text += " ";
				_status.text += Text::toString(_status.cursor.position.x);
				_status.text += ",";
				_status.text += Text::toString(_status.cursor.position.y);

				if (_selection.hoveringTrigger != Scene::INVALID_TRIGGER()) {
					_status.text += " ";

					_status.text += ws->theme()->status_Tr();
					_status.text += " ";
					_status.text += Text::toString(_selection.hoveringTrigger);
				}

				break;
			default:
				if (_status.cursor.position.x == -1 || _status.cursor.position.y == -1 || !currentLayer()) {
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

				break;
			}
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
				index = _project->scenePageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::SCENE, index);
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
			if (index >= _project->scenePageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::SCENE, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		if (entry()->refMap != -1) {
			ImGui::SameLine();
			if (_tools.isPlaying) {
				if (ImGui::ImageButton(ws->theme()->iconStopPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipScene_StopTesting().c_str())) {
					_tools.stopSceneTesting();
				}
			} else {
				if (ws->running()) {
					ImGui::BeginDisabled();
					ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipScene_TestScene().c_str());
					ImGui::EndDisabled();
				} else {
					if (ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipScene_TestScene().c_str())) {
						_tools.playSceneTesting();
					}
				}
			}
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		if (entry()->refMap == -1) {
			ImGui::TextUnformatted(" ");
			ImGui::SameLine();
			ImGui::TextUnformatted(ws->theme()->status_RefIsMissing());
		} else {
			ImGui::TextUnformatted(_status.text);
			ImGui::SameLine();
			ImGui::TextUnformatted(_estimated.text);
			ImGui::SameLine();
			do {
				float width_ = 0.0f;
				const float wndWidth = ImGui::GetWindowWidth();
				ImGui::SetCursorPosX(wndWidth - _statusWidth);
				if (wndWidth >= 430) {
					if (_status.info.empty()) {
						const int w = object()->width() / GBBASIC_TILE_SIZE;
						const int h = object()->height() / GBBASIC_TILE_SIZE;
						int l = 1;
						if (object()->hasAttributes())
							++l;
						if (object()->hasProperties())
							++l;
						if (object()->hasActors())
							++l;
						if (object()->hasTriggers())
							++l;
						_status.info = Text::format(
							ws->theme()->tooltipScene_Info(),
							{
								Text::toString(w * h),
								Text::toString(l),
								l <= 1 ? ws->theme()->tooltipScene_InfoLayer() : ws->theme()->tooltipScene_InfoLayers()
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
				if (!_warnings.empty()) {
					const ImVec4 col = ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->warningColor);
					ImGui::PushStyleColor(ImGuiCol_Text, col);

					if (ImGui::ImageButton(ws->theme()->iconWarning()->pointer(rnd), ImVec2(13, 13), col, false, ws->theme()->tooltip_Warning().c_str())) {
						ImGui::OpenPopupTooltip("Wrn");
					}
					if (ImGui::PopupTooltip("Wrn", _warnings.text, ws->theme()->generic_Dismiss().c_str()))
						_warnings.clear();
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
			const Text::Array &assetNames = ws->getScenePageNames();
			const int n = _project->scenePageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::SCENE, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::MenuItem(ws->theme()->menu_DataSequence(), nullptr, nullptr, _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER || _tools.layer == ASSETS_SCENE_ACTORS_LAYER)) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					std::string txt = Unicode::fromOs(osstr);
					txt = Parsers::removeComments(txt);
					Map::Ptr newData = nullptr;
					const scene_t &def = entry()->definition;
					const ActorRoutineOverriding::Array &actorRoutines = entry()->actorRoutineOverridings;
					if (!entry()->parseDataSequence(newData, txt, _tools.layer)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}

					if (_tools.layer == ASSETS_SCENE_PROPERTIES_LAYER) {
						Command* cmd = enqueue<Commands::Scene::Import>()
							->with(nullptr, nullptr, newData, nullptr, nullptr, def, actorRoutines)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
					} else if (_tools.layer == ASSETS_SCENE_ACTORS_LAYER) {
						Command* cmd = enqueue<Commands::Scene::Import>()
							->with(nullptr, nullptr, nullptr, newData, nullptr, def, actorRoutines)
							->with(_setLayer, _tools.layer)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
					}
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
					Scene::Ptr newObj = nullptr;
					scene_t def;
					ActorRoutineOverriding::Array actorRoutines;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, def, actorRoutines, txt, status)) {
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

					Map::Ptr newMap = newObj->mapLayer();
					Map::Ptr newAttrib = newObj->attributeLayer();
					Map::Ptr newProps = newObj->propertyLayer();
					Map::Ptr newActors = newObj->actorLayer();
					const Trigger::Array* newTriggers = newObj->triggerLayer();
					if (
						(newMap->width() != newAttrib->width() || newMap->height() != newAttrib->height()) ||
						(newMap->width() != newProps->width() || newMap->height() != newProps->height()) ||
						(newMap->width() != newActors->width() || newMap->height() != newActors->height())
					) {
						ws->bubble(ws->theme()->dialogPrompt_SizeOfLayersDoNotMatch(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::Scene::Import>()
						->with(newMap, newAttrib, newProps, newActors, newTriggers, def, actorRoutines)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()));

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

					Scene::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					scene_t def;
					ActorRoutineOverriding::Array actorRoutines;
					if (!entry()->parseJson(newObj, def, actorRoutines, txt, status)) {
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

					Map::Ptr newMap = newObj->mapLayer();
					Map::Ptr newAttrib = newObj->attributeLayer();
					Map::Ptr newProps = newObj->propertyLayer();
					Map::Ptr newActors = newObj->actorLayer();
					const Trigger::Array* newTriggers = newObj->triggerLayer();
					if (
						(newMap->width() != newAttrib->width() || newMap->height() != newAttrib->height()) ||
						(newMap->width() != newProps->width() || newMap->height() != newProps->height()) ||
						(newMap->width() != newActors->width() || newMap->height() != newActors->height())
					) {
						ws->bubble(ws->theme()->dialogPrompt_SizeOfLayersDoNotMatch(), nullptr);

						break;
					}

					Command* cmd = enqueue<Commands::Scene::Import>()
						->with(newMap, newAttrib, newProps, newActors, newTriggers, def, actorRoutines)
						->with(_setLayer, _tools.layer)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::BeginMenu(ws->theme()->menu_Code())) {
				if (ImGui::MenuItem("LOAD SCENE(...)")) {
					do {
						std::string txt;
						if (!entry()->serializeBasic(txt, _index, true))
							break;

						const std::string osstr = Unicode::toOs(txt);

						Platform::setClipboardText(osstr.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem("DEF SCENE(...)")) {
					do {
						std::string txt;
						if (!entry()->serializeBasic(txt, _index, false))
							break;

						const std::string osstr = Unicode::toOs(txt);

						Platform::setClipboardText(osstr.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
					} while (false);
				}

				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::BeginMenu(ws->theme()->menu_DataSequence(), _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER || _tools.layer == ASSETS_SCENE_ACTORS_LAYER)) {
				if (ImGui::MenuItem(ws->theme()->menu_Hex())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 16, _tools.layer))
							break;

						const std::string osstr = Unicode::toOs(txt);

						Platform::setClipboardText(osstr.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Dec())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 10, _tools.layer))
							break;

						const std::string osstr = Unicode::toOs(txt);

						Platform::setClipboardText(osstr.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Bin())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 2, _tools.layer))
							break;

						const std::string osstr = Unicode::toOs(txt);

						Platform::setClipboardText(osstr.c_str());

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

					const std::string osstr = Unicode::toOs(txt);

					Platform::setClipboardText(osstr.c_str());

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
						entry()->name.empty() ? "gbbasic-scene.json" : Text::sanitizeFilename(entry()->name) + ".json",
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

			ImGui::EndPopup();
		}

		// Views.
		if (ImGui::BeginPopup("@Views")) {
			switch (_tools.layer) {
			case ASSETS_SCENE_GRAPHICS_LAYER:
				if (ImGui::MenuItem(ws->theme()->menu_Actors(), nullptr, &_project->preferencesSceneShowActors())) {
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Triggers(), nullptr, &_project->preferencesSceneShowTriggers())) {
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Properties(), nullptr, &_project->preferencesSceneShowProperties())) {
					if (_project->preferencesSceneShowProperties())
						_project->preferencesSceneShowAttributes(false);
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Attributes(), nullptr, &_project->preferencesSceneShowAttributes())) {
					if (_project->preferencesSceneShowAttributes())
						_project->preferencesSceneShowProperties(false);
					_project->hasDirtyEditor(true);
				}
				ImGui::MenuItem(ws->theme()->menu_Map(), nullptr, true, false);

				break;
			case ASSETS_SCENE_ATTRIBUTES_LAYER:
				if (ImGui::MenuItem(ws->theme()->menu_Actors(), nullptr, &_project->preferencesSceneShowActors())) {
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Triggers(), nullptr, &_project->preferencesSceneShowTriggers())) {
					_project->hasDirtyEditor(true);
				}
				ImGui::MenuItem(ws->theme()->menu_Properties(), nullptr, false, false);
				ImGui::MenuItem(ws->theme()->menu_Attributes(), nullptr, true, false);
				ImGui::MenuItem(ws->theme()->menu_Map(), nullptr, true, false);

				break;
			case ASSETS_SCENE_PROPERTIES_LAYER:
				if (ImGui::MenuItem(ws->theme()->menu_Actors(), nullptr, &_project->preferencesSceneShowActors())) {
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Triggers(), nullptr, &_project->preferencesSceneShowTriggers())) {
					_project->hasDirtyEditor(true);
				}
				ImGui::MenuItem(ws->theme()->menu_Properties(), nullptr, true, false);
				ImGui::MenuItem(ws->theme()->menu_Attributes(), nullptr, false, false);
				ImGui::MenuItem(ws->theme()->menu_Map(), nullptr, true, false);

				break;
			case ASSETS_SCENE_ACTORS_LAYER:
				ImGui::MenuItem(ws->theme()->menu_Actors(), nullptr, true, false);
				if (ImGui::MenuItem(ws->theme()->menu_Triggers(), nullptr, &_project->preferencesSceneShowTriggers())) {
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Properties(), nullptr, &_project->preferencesSceneShowProperties())) {
					if (_project->preferencesSceneShowProperties())
						_project->preferencesSceneShowAttributes(false);
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Attributes(), nullptr, &_project->preferencesSceneShowAttributes())) {
					if (_project->preferencesSceneShowAttributes())
						_project->preferencesSceneShowProperties(false);
					_project->hasDirtyEditor(true);
				}
				ImGui::MenuItem(ws->theme()->menu_Map(), nullptr, true, false);

				break;
			case ASSETS_SCENE_TRIGGERS_LAYER:
				if (ImGui::MenuItem(ws->theme()->menu_Actors(), nullptr, &_project->preferencesSceneShowActors())) {
					_project->hasDirtyEditor(true);
				}
				ImGui::MenuItem(ws->theme()->menu_Triggers(), nullptr, true, false);
				if (ImGui::MenuItem(ws->theme()->menu_Properties(), nullptr, &_project->preferencesSceneShowProperties())) {
					if (_project->preferencesSceneShowProperties())
						_project->preferencesSceneShowAttributes(false);
					_project->hasDirtyEditor(true);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Attributes(), nullptr, &_project->preferencesSceneShowAttributes())) {
					if (_project->preferencesSceneShowAttributes())
						_project->preferencesSceneShowProperties(false);
					_project->hasDirtyEditor(true);
				}
				ImGui::MenuItem(ws->theme()->menu_Map(), nullptr, true, false);

				break;
			}
			ImGui::Separator();

			switch (_tools.layer) {
			case ASSETS_SCENE_ATTRIBUTES_LAYER:
			case ASSETS_SCENE_PROPERTIES_LAYER:
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

				_objects.fill();

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

		if (_ref.refMapIndex < 0)
			_ref.refMapIndex = 0;
		const int n = _project->mapPageCount();
		ImGui::PushID("@P");
		{
			ImGui::SetCursorPos(btnPos - ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 4));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->dialogPrompt_Map());

			ImGui::SetCursorPos(btnPos - ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 2));
			if (n == 0) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(ws->theme()->dialogPrompt_NothingAvailable());
			} else {
				if (ImGui::ImageButton(ws->theme()->iconMinus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					if (--_ref.refMapIndex < 0)
						_ref.refMapIndex = 0;
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(btnWidth - (13 + style.FramePadding.x * 2) * 2);
				if (ImGui::DragInt("", &_ref.refMapIndex, 1.0f, 0, n - 1, "%d")) {
					// Do nothing.
				}
				if (ImGui::IsItemHovered()) {
					const Text::Array &assetNames = ws->getMapPageNames();
					const std::string &pg = assetNames[_ref.refMapIndex];

					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(pg);
				}
				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->iconPlus()->pointer(rnd), ImVec2(13, 13), ImColor(IM_COL32_WHITE))) {
					if (++_ref.refMapIndex > n - 1)
						_ref.refMapIndex = n - 1;
				}
			}
		}
		ImGui::PopID();

		ImGui::SetCursorPos(btnPos);
		const bool valid = _ref.refMapIndex >= 0 && _ref.refMapIndex < _project->mapPageCount();
		if (valid) {
			if (ImGui::Button(ws->theme()->generic_Resolve(), ImVec2(btnWidth, btnHeight))) {
				_ref.refreshRefMapText(ws->theme());
				_ref.fill(rnd, _project, true);

				entry()->refMap = _ref.refMapIndex;
				entry()->cleanup();
				entry()->reload();
				entry()->touch();

				_project->preferencesSceneRefMap(_ref.refMapIndex);

				_project->hasDirtyAsset(true);

				_ref.resolving = false;
			}
		} else {
			ImGui::BeginDisabled();
			ImGui::Button(ws->theme()->generic_Resolve(), ImVec2(btnWidth, btnHeight));
			ImGui::EndDisabled();
		}
		if (entry()->refMap != -1) {
			ImGui::NewLine(3);
			ImGui::SetCursorPosX(btnPos.x);
			if (ImGui::Button(ws->theme()->generic_Cancel(), ImVec2(btnWidth, 0))) {
				_ref.resolving = false;
			}
		}
	}
	void resolveSize(Window* wnd, Renderer* rnd, Workspace* ws, float width, float windowHeight) {
		ImGuiStyle &style = ImGui::GetStyle();

		const float btnWidth = 100.0f;
		const float btnHeight = 72.0f;
		const ImVec2 btnPos((width - btnWidth) * 0.5f, (windowHeight - btnHeight) * 0.5f);

		ImGui::PushID("@S");
		{
			const std::string &txt = ws->theme()->dialogPrompt_RefMapSizeHasBeenChanged();
			const float txtw = ImGui::CalcTextSize(txt.c_str(), txt.c_str() + txt.length()).x;
			ImGui::SetCursorPos(ImVec2((width - txtw) * 0.5f, btnPos.y - ImGui::GetTextLineHeightWithSpacing() * 2));
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(txt);
		}
		ImGui::PopID();

		ImGui::SetCursorPos(btnPos);
		if (ImGui::Button(ws->theme()->generic_Resize(), ImVec2(btnWidth, btnHeight))) {
			const int width_ = map()->width();
			const int height_ = map()->height();
			post(RESIZE, width_, height_);

			_commands->clear();

			_project->hasDirtyAsset(true);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltip_ResizeTheCurrentAssetToResolve());
		}
		if (entry()->refMap != -1) {
			ImGui::NewLine(3);
			ImGui::SetCursorPosX(btnPos.x);
			if (ImGui::Button(ws->theme()->generic_Goto(), ImVec2(btnWidth, 0))) {
				ws->category(Workspace::Categories::MAP);
				ws->changePage(wnd, rnd, _project, Workspace::Categories::MAP, _ref.refMapIndex);
			}
		}
	}

	void changeLayer(Window* wnd, Renderer* rnd, Workspace* ws, int layer) {
		if (_tools.layer == layer)
			return;

		_tools.layer = layer;
		if (_tools.layer == ASSETS_SCENE_ACTORS_LAYER) {
			_tools.painting = _tools.paintingToolForActor;
			_cursor.size = Math::Vec2i(1, 1);
		} else if (_tools.layer == ASSETS_SCENE_TRIGGERS_LAYER) {
			_tools.painting = _tools.paintingToolForTrigger;
		} else {
			_tools.painting = _tools.paintingToolForPaintable;
			_cursor.size = Math::Vec2i(1, 1);
		}
		bindLayerTools(wnd, ws);
		if (_tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER) {
#if GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED
			if (_tools.painting == Editing::Tools::PENCIL) {
				_tools.painting = Editing::Tools::EYEDROPPER;
				_tools.paintingToolForPaintable = _tools.painting;
			} else if (_tools.painting == Editing::Tools::STAMP) {
				_tools.painting = Editing::Tools::EYEDROPPER;
				_tools.paintingToolForPaintable = _tools.painting;
			}
#else /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
			if (_tools.painting == Editing::Tools::STAMP) {
				_tools.painting = Editing::Tools::PENCIL;
				_tools.paintingToolForPaintable = _tools.painting;
			}
#endif /* GBBASIC_ASSET_AUTO_SWITCH_TOOL_TO_EYEDROPPER_ENABLED */
		}
		_ref.byteTooltip.clear();
		_status.clear();
		refreshStatus(wnd, rnd, ws, &_cursor);
	}
	void toggleLayer(Window*, Renderer*, Workspace*, int layer, bool enabled) {
		switch (layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER:
			object()->hasAttributes(enabled);

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER:
			object()->hasProperties(enabled);

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			object()->hasActors(enabled);

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			object()->hasTriggers(enabled);

			break;
		}
	}
	void bindLayerTools(Window* wnd, Workspace* ws) {
		if (_tools.layer == ASSETS_SCENE_ACTORS_LAYER) {
			_processors[Editing::Tools::SMUDGE].down  = std::bind(&EditorSceneImpl::actorSmudgeToolDown, this, std::placeholders::_1);
			_processors[Editing::Tools::SMUDGE].move  = std::bind(&EditorSceneImpl::actorSmudgeToolMove, this, std::placeholders::_1);
			_processors[Editing::Tools::SMUDGE].up    = std::bind(&EditorSceneImpl::actorSmudgeToolUp, this, std::placeholders::_1);
			_processors[Editing::Tools::SMUDGE].hover = nullptr;

			_processors[Editing::Tools::ERASER].down  = nullptr;
			_processors[Editing::Tools::ERASER].move  = nullptr;
			_processors[Editing::Tools::ERASER].up    = std::bind(&EditorSceneImpl::actorEraserToolUp, this, std::placeholders::_1);
			_processors[Editing::Tools::ERASER].hover = nullptr;

			_processors[Editing::Tools::MOVE  ].down  = std::bind(&EditorSceneImpl::actorMoveToolDown, this, std::placeholders::_1);
			_processors[Editing::Tools::MOVE  ].move  = std::bind(&EditorSceneImpl::actorMoveToolMove, this, std::placeholders::_1);
			_processors[Editing::Tools::MOVE  ].up    = std::bind(&EditorSceneImpl::actorMoveToolUp, this, std::placeholders::_1);
			_processors[Editing::Tools::MOVE  ].hover = nullptr;

			_processors[Editing::Tools::RESIZE].down  = nullptr;
			_processors[Editing::Tools::RESIZE].move  = nullptr;
			_processors[Editing::Tools::RESIZE].up    = nullptr;
			_processors[Editing::Tools::RESIZE].hover = nullptr;

			_processors[Editing::Tools::REF   ].down  = nullptr;
			_processors[Editing::Tools::REF   ].move  = nullptr;
			_processors[Editing::Tools::REF   ].up    = std::bind(&EditorSceneImpl::actorRefToolUp, this, wnd, std::placeholders::_1, ws);
			_processors[Editing::Tools::REF   ].hover = nullptr;
		} else if (_tools.layer == ASSETS_SCENE_TRIGGERS_LAYER) {
			_processors[Editing::Tools::SMUDGE].down  = std::bind(&EditorSceneImpl::triggerSmudgeToolDown, this, std::placeholders::_1);
			_processors[Editing::Tools::SMUDGE].move  = std::bind(&EditorSceneImpl::triggerSmudgeToolMove, this, std::placeholders::_1);
			_processors[Editing::Tools::SMUDGE].up    = std::bind(&EditorSceneImpl::triggerSmudgeToolUp, this, std::placeholders::_1, ws);
			_processors[Editing::Tools::SMUDGE].hover = nullptr;

			_processors[Editing::Tools::ERASER].down  = nullptr;
			_processors[Editing::Tools::ERASER].move  = nullptr;
			_processors[Editing::Tools::ERASER].up    = std::bind(&EditorSceneImpl::triggerEraserToolUp, this, std::placeholders::_1);
			_processors[Editing::Tools::ERASER].hover = nullptr;

			_processors[Editing::Tools::MOVE  ].down  = std::bind(&EditorSceneImpl::triggerMoveToolDown, this, std::placeholders::_1);
			_processors[Editing::Tools::MOVE  ].move  = std::bind(&EditorSceneImpl::triggerMoveToolMove, this, std::placeholders::_1);
			_processors[Editing::Tools::MOVE  ].up    = std::bind(&EditorSceneImpl::triggerMoveToolUp, this, std::placeholders::_1);
			_processors[Editing::Tools::MOVE  ].hover = nullptr;

			_processors[Editing::Tools::RESIZE].down  = std::bind(&EditorSceneImpl::triggerResizeToolDown, this, std::placeholders::_1);
			_processors[Editing::Tools::RESIZE].move  = std::bind(&EditorSceneImpl::triggerResizeToolMove, this, std::placeholders::_1);
			_processors[Editing::Tools::RESIZE].up    = std::bind(&EditorSceneImpl::triggerResizeToolUp, this, std::placeholders::_1);
			_processors[Editing::Tools::RESIZE].hover = nullptr;

			_processors[Editing::Tools::REF   ].down  = nullptr;
			_processors[Editing::Tools::REF   ].move  = nullptr;
			_processors[Editing::Tools::REF   ].up    = nullptr;
			_processors[Editing::Tools::REF   ].hover = nullptr;
		} else {
			_processors[Editing::Tools::SMUDGE].down  = nullptr;
			_processors[Editing::Tools::SMUDGE].move  = nullptr;
			_processors[Editing::Tools::SMUDGE].up    = nullptr;
			_processors[Editing::Tools::SMUDGE].hover = nullptr;

			_processors[Editing::Tools::ERASER].down  = nullptr;
			_processors[Editing::Tools::ERASER].move  = nullptr;
			_processors[Editing::Tools::ERASER].up    = nullptr;
			_processors[Editing::Tools::ERASER].hover = nullptr;

			_processors[Editing::Tools::MOVE  ].down  = nullptr;
			_processors[Editing::Tools::MOVE  ].move  = nullptr;
			_processors[Editing::Tools::MOVE  ].up    = nullptr;
			_processors[Editing::Tools::MOVE  ].hover = nullptr;

			_processors[Editing::Tools::RESIZE].down  = nullptr;
			_processors[Editing::Tools::RESIZE].move  = nullptr;
			_processors[Editing::Tools::RESIZE].up    = nullptr;
			_processors[Editing::Tools::RESIZE].hover = nullptr;

			_processors[Editing::Tools::REF   ].down  = nullptr;
			_processors[Editing::Tools::REF   ].move  = nullptr;
			_processors[Editing::Tools::REF   ].up    = nullptr;
			_processors[Editing::Tools::REF   ].hover = nullptr;
		}
	}

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "Scene editor: ";
				msg_ += msg;
				if (msg.back() != '.')
					msg_ += '.';
				ws->warn(msg_.c_str());
			}
		} else {
			_warnings.remove(msg);
		}
	}

	void modified(void) {
		const bool forMap = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER;
		if (!forMap)
			return;

		const int refMap = entry()->refMap;
		const MapAssets::Entry* mapEntry = entry()->getMap(refMap);
		if (mapEntry) {
			Editable* editor = mapEntry->editor;
			if (editor)
				editor->post(Editable::CLEAR_UNDO_REDO_RECORDS, false);
		}
	}

	void createOverlay(void) {
		_overlay = std::bind(&EditorSceneImpl::getOverlayCel, this, std::placeholders::_1);
	}
	void destroyOverlay(void) {
		if (_overlay)
			_overlay = nullptr;
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		modified();

		return result;
	}
	int simplify(void) {
		const int result = _commands->simplify();

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd) {
		const bool recalcSize =
			Command::is<Commands::Scene::AddTrigger>(cmd) ||
			Command::is<Commands::Scene::DeleteTrigger>(cmd) ||
			Command::is<Commands::Scene::ToggleMask>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		const bool refreshLayers =
			Command::is<Commands::Scene::SwitchLayer>(cmd);
		const bool refreshCursor =
			Command::is<Commands::Scene::AddTrigger>(cmd) ||
			Command::is<Commands::Scene::DeleteTrigger>(cmd) ||
			Command::is<Commands::Scene::MoveTrigger>(cmd) ||
			Command::is<Commands::Scene::ResizeTrigger>(cmd);
		const bool refreshPalette =
			Command::is<Commands::Scene::Pencil>(cmd) ||
			Command::is<Commands::Scene::Line>(cmd) ||
			Command::is<Commands::Scene::Box>(cmd) ||
			Command::is<Commands::Scene::BoxFill>(cmd) ||
			Command::is<Commands::Scene::Ellipse>(cmd) ||
			Command::is<Commands::Scene::EllipseFill>(cmd) ||
			Command::is<Commands::Scene::Fill>(cmd) ||
			Command::is<Commands::Scene::Replace>(cmd) ||
			Command::is<Commands::Scene::Stamp>(cmd) ||
			Command::is<Commands::Scene::Rotate>(cmd) ||
			Command::is<Commands::Scene::Flip>(cmd) ||
			Command::is<Commands::Scene::Cut>(cmd) ||
			Command::is<Commands::Scene::Paste>(cmd) ||
			Command::is<Commands::Scene::Delete>(cmd) ||
			Command::is<Commands::Scene::ToggleMask>(cmd);
		const bool refreshAllPalette =
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		const bool refillActors =
			Command::is<Commands::Scene::SetActor>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		const bool refillSelectedActors =
			Command::is<Commands::Scene::SetActor>(cmd);
		const bool refillObjects =
			Command::is<Commands::Scene::SetActor>(cmd) ||
			Command::is<Commands::Scene::MoveActor>(cmd) ||
			Command::is<Commands::Scene::AddTrigger>(cmd) ||
			Command::is<Commands::Scene::DeleteTrigger>(cmd) ||
			Command::is<Commands::Scene::SwapTrigger>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		const bool refillName =
			Command::is<Commands::Scene::SetName>(cmd);
		const bool refillDefinition =
			Command::is<Commands::Scene::SetDefinition>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		const bool toCheck =
			Command::is<Commands::Scene::AddTrigger>(cmd) ||
			Command::is<Commands::Scene::DeleteTrigger>(cmd) ||
			Command::is<Commands::Scene::SetActor>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);
		const bool toDetermine =
			Command::is<Commands::Scene::SetActor>(cmd) ||
			Command::is<Commands::Scene::Resize>(cmd) ||
			Command::is<Commands::Scene::Import>(cmd);

		if (recalcSize) {
			_status.info.clear();
			_estimated.filled = false;
		}
		if (refreshLayers) {
			_status.info.clear();
		}
		if (refreshCursor) {
			bool found = false;
			Editing::TriggerIndex::Array triggers_;
			if (queryTrigger(_cursor.position, triggers_) > 0) {
				for (const Editing::TriggerIndex &idx : triggers_) {
					Trigger trigger;
					if (getTrigger(idx.cel, &trigger)) {
						if (_cursor.position == trigger.position() && _cursor.size == trigger.size()) {
							found = true;

							break;
						}
					}
				}

				if (!found) {
					const Editing::TriggerIndex &idx = triggers_.front();
					Trigger trigger;
					if (getTrigger(idx.cel, &trigger)) {
						_cursor.position = trigger.position();
						_cursor.size = trigger.size();
						found = true;
					}
				}
			}
			if (!found) {
				_cursor.size = Math::Vec2i(1, 1);
			}

			_selection.clear();
		}
		if (refreshPalette) {
			if (_project->preferencesPreviewPaletteBits()) {
				const Commands::Scene::PaintableBase::Paint* cmdPaint = Command::as<Commands::Scene::PaintableBase::Paint>(cmd);
				const Commands::Scene::PaintableBase::Blit* cmdBlit = Command::as<Commands::Scene::PaintableBase::Blit>(cmd);
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
		if (refreshAllPalette) {
			entry()->cleanup();
		}
		if (refillActors) {
			entry()->refActors.clear();
			entry()->uniqueRefActors.clear();
			entry()->refActors = entry()->getRefActors(&entry()->uniqueRefActors);
		}
		if (refillSelectedActors) {
			const int cel = actors()->get(_cursor.position.x, _cursor.position.y);
			if (cel == Scene::INVALID_ACTOR())
				_selection.selectedActor.clear();
			else
				_selection.selectedActor = Editing::ActorIndex((Byte)cel, Math::Vec2i());
		}
		if (refillObjects) {
			_selection.hoveringActorTipsPosition = Math::Vec2i(-1, -1);
			_objects.fill();
		}
		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearScenePageNames();
		}
		if (refillDefinition) {
			_tools.definitionShadow = entry()->definition;
		}
		if (toCheck) {
			_checker();
		}
		if (toDetermine) {
			_determinator();
		}
	}

	void flip(Editing::Tools::RotationsAndFlippings f) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		if (!_binding.getCel || !_binding.setCel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			enqueue<Commands::Scene::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(1)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			enqueue<Commands::Scene::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(-1)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::HALF_TURN:
			enqueue<Commands::Scene::Rotate>()
				->with(_binding.getCel, _binding.setCel)
				->with(2)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			enqueue<Commands::Scene::Flip>()
				->with(_binding.getCel, _binding.setCel)
				->with(1)
				->with(frame)
				->with(_setLayer, _tools.layer)
				->exec(currentLayer());

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			enqueue<Commands::Scene::Flip>()
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
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		const int idx = currentLayer()->get(_cursor.position.x, _cursor.position.y);
		if (idx == Map::INVALID())
			return;

		if (_tileSize.x <= 0)
			return;

		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER: {
				const std::div_t div = std::div(idx, (int)(_ref.image->width() / _tileSize.x));
				_ref.mapCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.mapCursor.size = _tileSize;
			}

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER: {
				const std::div_t div = std::div(idx, ASSETS_SCENE_ATTRIBUTES_REF_WIDTH);
				_ref.attributesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.attributesCursor.size = _tileSize;
			}

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER: {
				const std::div_t div = std::div(idx, ASSETS_SCENE_PROPERTIES_REF_WIDTH);
				_ref.propertiesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.propertiesCursor.size = _tileSize;
			}

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		}
	}
	void eyedropperToolMove(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		const int idx = currentLayer()->get(_cursor.position.x, _cursor.position.y);
		if (idx == Map::INVALID())
			return;

		if (_tileSize.x <= 0)
			return;

		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER: {
				const std::div_t div = std::div(idx, (int)(_ref.image->width() / _tileSize.x));
				_ref.mapCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.mapCursor.size = _tileSize;
			}

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER: {
				const std::div_t div = std::div(idx, ASSETS_SCENE_ATTRIBUTES_REF_WIDTH);
				_ref.attributesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.attributesCursor.size = _tileSize;
			}

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER: {
				const std::div_t div = std::div(idx, ASSETS_SCENE_PROPERTIES_REF_WIDTH);
				_ref.propertiesCursor.position = Math::Vec2i(div.rem * _tileSize.x, div.quot * _tileSize.y);
				_ref.propertiesCursor.size = _tileSize;
			}

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		}
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		T* cmd = enqueue<T>();
		cmd->with(_tools.bitwiseOperations);
		cmd->with(_setLayer, _tools.layer);
		Commands::Scene::PaintableBase::Paint::Getter getter = std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		Commands::Scene::PaintableBase::Paint::Setter setter = std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		cmd->with(getter, setter);

		int idx = 0;
		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER:
			idx = _ref.mapCursor.unit().x + _ref.mapCursor.unit().y * (_ref.image->width() / _tileSize.x);

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER:
			idx = _ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_SCENE_ATTRIBUTES_REF_WIDTH;

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER:
			idx = _ref.propertiesCursor.unit().x + _ref.propertiesCursor.unit().y * ASSETS_SCENE_PROPERTIES_REF_WIDTH;

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		}

		cmd->with(
			Math::Vec2i(object()->width(), object()->height()),
			_selection.tiler.empty() ? nullptr : &sel,
			_cursor.position, idx
		);

		cmd->exec(currentLayer());
	}
	void paintbucketToolUp(Renderer* rnd) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		Math::Recti sel;
		if (_selection.area(sel) && !Math::intersects(sel, _cursor.position))
			_selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Scene::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Scene::Fill>(rnd, sel);
	}

	void lassoToolDown(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		_selection.mouse = ImGui::GetMousePos();

		_selection.initial = _cursor.position;
		_selection.tiler.position = _cursor.position;
		_selection.tiler.size = Math::Vec2i(1, 1);
	}
	void lassoToolMove(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);
		_selection.tiler.position = Math::Vec2i(rect.xMin(), rect.yMin());
		_selection.tiler.size = Math::Vec2i(rect.width(), rect.height());
	}
	void lassoToolUp(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

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
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		_selection.clear();
	}
	void stampToolUp(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		if (!_binding.getCel || !_binding.setCel)
			return;

		Math::Recti cursor;
		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER:
			cursor = Math::Recti::byXYWH(
				_ref.mapCursor.position.x / _tileSize.x, _ref.mapCursor.position.y / _tileSize.y,
				_ref.mapCursor.size.x / _tileSize.x, _ref.mapCursor.size.y / _tileSize.y
			);

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER:
			cursor = Math::Recti::byXYWH(
				_ref.attributesCursor.position.x / _tileSize.x, _ref.attributesCursor.position.y / _tileSize.y,
				_ref.attributesCursor.size.x / _tileSize.x, _ref.attributesCursor.size.y / _tileSize.y
			);

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER:
			cursor = Math::Recti::byXYWH(
				_ref.propertiesCursor.position.x / _tileSize.x, _ref.propertiesCursor.position.y / _tileSize.y,
				_ref.propertiesCursor.size.x / _tileSize.x, _ref.propertiesCursor.size.y / _tileSize.y
			);

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
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

		enqueue<Commands::Scene::Stamp>()
			->with(_tools.bitwiseOperations)
			->with(_binding.getCel, _binding.setCel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		destroyOverlay();
	}
	void stampToolHover(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		if (_overlay && !_painting.moved())
			return;

		Math::Recti cursor;
		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER:
			cursor = Math::Recti::byXYWH(
				_ref.mapCursor.position.x / _tileSize.x, _ref.mapCursor.position.y / _tileSize.y,
				_ref.mapCursor.size.x / _tileSize.x, _ref.mapCursor.size.y / _tileSize.y
			);

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER:
			cursor = Math::Recti::byXYWH(
				_ref.attributesCursor.position.x / _tileSize.x, _ref.attributesCursor.position.y / _tileSize.y,
				_ref.attributesCursor.size.x / _tileSize.x, _ref.attributesCursor.size.y / _tileSize.y
			);

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER:
			cursor = Math::Recti::byXYWH(
				_ref.propertiesCursor.position.x / _tileSize.x, _ref.propertiesCursor.position.y / _tileSize.y,
				_ref.propertiesCursor.size.x / _tileSize.x, _ref.propertiesCursor.size.y / _tileSize.y
			);

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
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
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		if (!_binding.getCel || !_binding.setCel)
			return;

		_selection.clear();

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		enqueue<Commands::Scene::Paste>()
			->with(_tools.bitwiseOperations)
			->with(_binding.getCel, _binding.setCel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		_processors[Editing::Tools::STAMP] = Processor{
			nullptr,
			nullptr,
			std::bind(&EditorSceneImpl::stampToolUp, this, std::placeholders::_1),
			std::bind(&EditorSceneImpl::stampToolHover, this, std::placeholders::_1)
		};

		destroyOverlay();

		_tools.painting = prevTool;
	}
	void stampToolHover_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

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
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		_selection.clear();

		enqueue<T>()
			->with(
				std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				_tools.weighting + 1
			)
			->with(_tools.bitwiseOperations)
			->with(_setLayer, _tools.layer)
			->exec(currentLayer());

		createOverlay();

		paintToolMove<T>(rnd);
	}
	template<typename T> void paintToolMove(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		if (_commands->empty())
			return;

		Command* back = _commands->back();
		GBBASIC_ASSERT(back->type() == T::TYPE());
		T* cmd = Command::as<T>(back);
		int idx = 0;
		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER:
			idx = _ref.mapCursor.unit().x + _ref.mapCursor.unit().y * (_ref.image->width() / _tileSize.x);

			break;
		case ASSETS_SCENE_ATTRIBUTES_LAYER:
			idx = _ref.attributesCursor.unit().x + _ref.attributesCursor.unit().y * ASSETS_SCENE_ATTRIBUTES_REF_WIDTH;

			break;
		case ASSETS_SCENE_PROPERTIES_LAYER:
			idx = _ref.propertiesCursor.unit().x + _ref.propertiesCursor.unit().y * ASSETS_SCENE_PROPERTIES_REF_WIDTH;

			break;
		case ASSETS_SCENE_ACTORS_LAYER:
			// Do nothing.

			break;
		case ASSETS_SCENE_TRIGGERS_LAYER:
			// Do nothing.

			break;
		}

		cmd->with(
			Math::Vec2i(object()->width(), object()->height()),
			_cursor.position, idx,
			nullptr
		);
	}
	void paintToolUp(Renderer*) {
		const bool operatable = _tools.layer == ASSETS_SCENE_GRAPHICS_LAYER || _tools.layer == ASSETS_SCENE_ATTRIBUTES_LAYER || _tools.layer == ASSETS_SCENE_PROPERTIES_LAYER;
		if (!operatable)
			return;

		if (!_commands->empty())
			_commands->back()->redo(currentLayer());

		destroyOverlay();
	}

	void actorSmudgeToolDown(Renderer*) {
		// Do nothing.
	}
	void actorSmudgeToolMove(Renderer*) {
		// Do nothing.
	}
	void actorSmudgeToolUp(Renderer* rnd) {
		const Math::Vec2i &pos = _cursor.position;
		const Byte cel = (Byte)currentLayer()->get(pos.x, pos.y);
		if (cel == _tools.actorTemplate)
			return;

		const ActorRoutineOverriding::Array &actorRoutines = entry()->actorRoutineOverridings;

		Commands::Scene::SetActor* cmd = (Commands::Scene::SetActor*)enqueue<Commands::Scene::SetActor>()
			->with(actorRoutines)
			->with(
				std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				1
			)
			->with(_setLayer, _tools.layer)
			->exec(object(), Variant((void*)entry()));
		cmd->with(
			Math::Vec2i(object()->width(), object()->height()),
			pos, _tools.actorTemplate,
			nullptr
		);
		if (!_commands->empty())
			_commands->back()->redo(object(), Variant((void*)entry()));

		_refresh(cmd);
	}
	void actorEraserToolUp(Renderer* rnd) {
		const Math::Vec2i &pos = getActorPositionUnderCursor(_cursor.position);
		const Byte cel = (Byte)currentLayer()->get(pos.x, pos.y);
		if (cel == Scene::INVALID_ACTOR())
			return;

		ActorRoutineOverriding::Array actorRoutines = entry()->actorRoutineOverridings;
		Routine::remove(actorRoutines, pos);

		Commands::Scene::SetActor* cmd = (Commands::Scene::SetActor*)enqueue<Commands::Scene::SetActor>()
			->with(actorRoutines)
			->with(
				std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				1
			)
			->with(_setLayer, _tools.layer)
			->exec(object(), Variant((void*)entry()));
		cmd->with(
			Math::Vec2i(object()->width(), object()->height()),
			pos, Scene::INVALID_ACTOR(),
			nullptr
		);
		if (!_commands->empty())
			_commands->back()->redo(object(), Variant((void*)entry()));

		_refresh(cmd);
	}
	void actorMoveToolDown(Renderer*) {
		_selection.clear();
	}
	void actorMoveToolMove(Renderer* rnd) {
		// Enqueue a command for movement on the first move.
		if (_selection.editingActor.invalid()) {
			_selection.editingActor = _selection.hoveringActor;

			const Editing::ActorIndex &idx = _selection.editingActor;
			if (idx.invalid())
				return;

			Commands::Scene::MoveActor::Getter getter = std::bind(&EditorSceneImpl::getCel, this, rnd, std::placeholders::_1, std::placeholders::_2);
			Commands::Scene::MoveActor::Setter setter = std::bind(&EditorSceneImpl::setCel, this, rnd, std::placeholders::_1, std::placeholders::_2);
			enqueue<Commands::Scene::MoveActor>()
				->with(getter, setter)
				->with(_cursor.position + _selection.editingActor.offset, idx.cel, true)
				->with(_setLayer, _tools.layer);
		}

		// Set the parameters.
		if (_selection.editingActor.invalid())
			return;

		if (_commands->empty())
			return;

		Command* back = _commands->back();
		if (back->type() != Commands::Scene::MoveActor::TYPE())
			return;
		Commands::Scene::MoveActor* cmd = Command::as<Commands::Scene::MoveActor>(back);

		Math::Vec2i pos = _cursor.position + _selection.editingActor.offset;
		//if (pos.x < 0 || pos.x >= object()->width() || pos.y < 0 || pos.y >= object()->height())
			//return;
		pos.x = Math::clamp(pos.x, 0, object()->width() - 1);
		pos.y = Math::clamp(pos.y, 0, object()->height() - 1);
		if (pos == cmd->destinationPosition())
			return;

		const Byte cel = (Byte)currentLayer()->get(pos.x, pos.y);

		cmd
			->with(pos, cel, false);
	}
	void actorMoveToolUp(Renderer*) {
		if (_selection.editingActor.invalid())
			return;

		_selection.clear();

		if (_commands->empty())
			return;

		Command* back = _commands->back();
		if (back->type() != Commands::Scene::MoveActor::TYPE())
			return;
		Commands::Scene::MoveActor* cmd = Command::as<Commands::Scene::MoveActor>(back);
		const Math::Vec2i &src = cmd->sourcePosition();
		const Math::Vec2i &dst = cmd->destinationPosition();
		if (src == dst) {
			_commands->discard();
		} else {
			ActorRoutineOverriding::Array actorRoutines = entry()->actorRoutineOverridings;
			const int cel = actors()->get(dst.x, dst.y);
			if (cel != Scene::INVALID_ACTOR())
				Routine::remove(actorRoutines, dst);
			Routine::move(actorRoutines, src, dst);

			cmd
				->with(actorRoutines)
				->exec(object(), Variant((void*)entry()));

			_refresh(cmd);
		}
	}
	void actorRefToolUp(Window* wnd, Renderer* rnd, Workspace* ws) {
		const Byte cel = (Byte)currentLayer()->get(_cursor.position.x, _cursor.position.y);
		ws->category(Workspace::Categories::ACTOR);
		ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, cel);
	}

	void triggerSmudgeToolDown(Renderer*) {
		_selection.mouse = ImGui::GetMousePos();

		_selection.initial = _cursor.position;
		_selection.tiler.position = _cursor.position;
		_selection.tiler.size = Math::Vec2i(1, 1);
	}
	void triggerSmudgeToolMove(Renderer*) {
		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);
		_selection.tiler.position = Math::Vec2i(rect.xMin(), rect.yMin());
		_selection.tiler.size = Math::Vec2i(rect.width(), rect.height());
	}
	void triggerSmudgeToolUp(Renderer*, Workspace* ws) {
		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);
		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);

		_cursor = _selection.tiler;

		_selection.clear();

		const int index = (int)triggers()->size();
		Trigger trigger(rect);
		trigger.color = Colour::randomize(255);
		Commands::Scene::AddTrigger* cmd = (Commands::Scene::AddTrigger*)enqueue<Commands::Scene::AddTrigger>()
			->with(_binding.addTrigger, _binding.removeTrigger)
			->with(index, trigger)
			->with(_setLayer, _tools.layer)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		GBBASIC::Kernel::Ptr krnl = ws->activeKernel();
		if (krnl) {
			if ((int)triggers()->size() > krnl->objectsMaxTriggerCount()) {
				warn(ws, ws->theme()->warning_SceneTriggerCountOutOfBounds(), true);
			}
		}
	}
	void triggerEraserToolUp(Renderer*) {
		Editing::TriggerIndex::Array triggers_;
		if (queryTrigger(_cursor.position, triggers_) > 0) {
			const Editing::TriggerIndex &idx = triggers_.front();
			Trigger trigger;
			if (getTrigger(idx.cel, &trigger)) {
				Commands::Scene::DeleteTrigger* cmd = (Commands::Scene::DeleteTrigger*)enqueue<Commands::Scene::DeleteTrigger>()
					->with(_binding.addTrigger, _binding.removeTrigger)
					->with(idx.cel)
					->with(_setLayer, _tools.layer)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);
			}
		}
	}
	void triggerMoveToolDown(Renderer*) {
		const Math::Vec2i pos = _selection.triggerCursor;
		_selection.clear();
		_selection.triggerCursor = pos;
	}
	void triggerMoveToolMove(Renderer*) {
		// Enqueue a command for movement on the first move.
		if (_selection.editingTrigger == Scene::INVALID_TRIGGER()) {
			const Math::Vec2i pos = _selection.triggerCursor;

			Editing::TriggerIndex::Array triggers_;
			if (queryTrigger(pos, triggers_) == 0)
				return;

			_selection.mouse = ImGui::GetMousePos();

			_selection.initial = pos;
			_selection.triggerMoving = pos;

			const Editing::TriggerIndex &idx = triggers_.front();
			_selection.editingTrigger = idx.cel;
			Trigger trigger;
			if (getTrigger(idx.cel, &trigger)) {
				enqueue<Commands::Scene::MoveTrigger>()
					->with(_binding.getTrigger, _binding.setTrigger)
					->with(idx.cel, trigger)
					->with(_setLayer, _tools.layer)
					->exec(object(), Variant((void*)entry()));
			}
		}

		// Set the parameters.
		if (_selection.editingTrigger == Scene::INVALID_TRIGGER())
			return;

		if (_commands->empty())
			return;

		const Math::Vec2i offset_ = _selection.triggerCursor - _selection.triggerMoving;
		if (offset_.x == 0 && offset_.y == 0)
			return;
		_selection.triggerMoving = _selection.triggerCursor;

		const Math::Vec2i offset = _selection.triggerCursor - _selection.initial;

		Command* back = _commands->back();
		if (back->type() != Commands::Scene::MoveTrigger::TYPE())
			return;
		Commands::Scene::MoveTrigger* cmd = Command::as<Commands::Scene::MoveTrigger>(back);

		const Trigger &oldTrigger = cmd->oldTrigger();
		Trigger trigger = oldTrigger + offset;
		Math::Vec2i diff;
		if (trigger.xMin() < 0)
			diff.x = -trigger.xMin();
		else if (trigger.xMax() >= object()->width())
			diff.x = -(trigger.xMax() - object()->width() + 1);
		if (trigger.yMin() < 0)
			diff.y = -trigger.yMin();
		else if (trigger.yMax() >= object()->height())
			diff.y = -(trigger.yMax() - object()->height() + 1);
		trigger += diff;

		cmd
			->with(trigger)
			->exec(object(), Variant((void*)entry()));
	}
	void triggerMoveToolUp(Renderer*) {
		if (_selection.editingTrigger == Scene::INVALID_TRIGGER())
			return;

		if (_commands->empty())
			return;

		const Math::Vec2i offset = _selection.triggerCursor - _selection.initial;

		_selection.clear();

		Command* back = _commands->back();
		if (back->type() != Commands::Scene::MoveTrigger::TYPE())
			return;
		Commands::Scene::MoveTrigger* cmd = Command::as<Commands::Scene::MoveTrigger>(back);
		cmd
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		if (offset.x == 0 && offset.y == 0) {
			_commands->discard();

			_selection.clear();

			return;
		}
	}
	void triggerResizeToolDown(Renderer*) {
		const Math::Vec2i pos = _selection.triggerCursor;

		_selection.clear();

		Editing::TriggerIndex::Array triggers_;
		if (queryTrigger(pos, triggers_) > 0) {
			_selection.mouse = ImGui::GetMousePos();

			_selection.initial = pos;

			const Editing::TriggerIndex &idx = triggers_.front();
			_selection.editingTrigger = idx.cel;
			Trigger trigger;
			if (getTrigger(idx.cel, &trigger)) {
				if (std::abs(pos.x - trigger.xMin()) >= std::abs(pos.x - trigger.xMax())) {
					_selection.triggerResizing.x = trigger.xMin();
				} else {
					_selection.triggerResizing.x = trigger.xMax();
				}
				if (std::abs(pos.y - trigger.yMin()) >= std::abs(pos.y - trigger.yMax())) {
					_selection.triggerResizing.y = trigger.yMin();
				} else {
					_selection.triggerResizing.y = trigger.yMax();
				}

				enqueue<Commands::Scene::ResizeTrigger>()
					->with(_binding.getTrigger, _binding.setTrigger)
					->with(idx.cel, trigger)
					->with(_setLayer, _tools.layer)
					->exec(object(), Variant((void*)entry()));
			}
		}
	}
	void triggerResizeToolMove(Renderer*) {
		if (_selection.editingTrigger == Scene::INVALID_TRIGGER())
			return;

		const Math::Vec2i offset = _selection.triggerCursor - _selection.initial;
		if (offset.x == 0 && offset.y == 0)
			return;

		Command* back = _commands->back();
		if (back->type() != Commands::Scene::ResizeTrigger::TYPE())
			return;
		Commands::Scene::ResizeTrigger* cmd = Command::as<Commands::Scene::ResizeTrigger>(back);
		const Trigger &oldTrigger = cmd->oldTrigger();
		Trigger trigger = oldTrigger;
		if (_selection.triggerCursor.x >= _selection.triggerResizing.x) {
			trigger.x0 = _selection.triggerResizing.x;
			trigger.x1 = _selection.triggerCursor.x;
		} else {
			trigger.x0 = _selection.triggerCursor.x;
			trigger.x1 = _selection.triggerResizing.x;
		}
		if (_selection.triggerCursor.y >= _selection.triggerResizing.y) {
			trigger.y0 = _selection.triggerResizing.y;
			trigger.y1 = _selection.triggerCursor.y;
		} else {
			trigger.y0 = _selection.triggerCursor.y;
			trigger.y1 = _selection.triggerResizing.y;
		}
		cmd
			->with(trigger)
			->exec(object(), Variant((void*)entry()));
	}
	void triggerResizeToolUp(Renderer*) {
		if (_selection.editingTrigger == Scene::INVALID_TRIGGER())
			return;

		_selection.clear();

		Command* back = _commands->back();
		if (back->type() != Commands::Scene::ResizeTrigger::TYPE())
			return;
		Commands::Scene::ResizeTrigger* cmd = Command::as<Commands::Scene::ResizeTrigger>(back);
		cmd
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);
	}

	SceneAssets::Entry* entry(void) const {
		SceneAssets::Entry* entry = _project->getScene(_index);

		return entry;
	}
	Scene::Ptr &object(void) const {
		return entry()->data;
	}
	Map::Ptr map(void) const {
		return object()->mapLayer();
	}
	Map::Ptr attributes(void) const {
		return object()->attributeLayer();
	}
	Map::Ptr properties(void) const {
		return object()->propertyLayer();
	}
	Map::Ptr actors(void) const {
		return object()->actorLayer();
	}
	const Trigger::Array* triggers(void) const {
		return object()->triggerLayer();
	}
	Trigger::Array* triggers(void) {
		return object()->triggerLayer();
	}
	Map::Ptr currentLayer(void) const {
		switch (_tools.layer) {
		case ASSETS_SCENE_GRAPHICS_LAYER:
			return map();
		case ASSETS_SCENE_ATTRIBUTES_LAYER:
			return attributes();
		case ASSETS_SCENE_PROPERTIES_LAYER:
			return properties();
		case ASSETS_SCENE_ACTORS_LAYER:
			return actors();
		case ASSETS_SCENE_TRIGGERS_LAYER:
			return nullptr;
		default:
			GBBASIC_ASSERT(false && "Impossible.");

			return nullptr;
		}
	}

	int getOverlayCel(const Math::Vec2i &pos) const {
		Command* back = _commands->back();
		switch (back->type()) {
		case Commands::Scene::Pencil::TYPE(): // Fall through.
		case Commands::Scene::Line::TYPE(): // Fall through.
		case Commands::Scene::Box::TYPE(): // Fall through.
		case Commands::Scene::BoxFill::TYPE(): // Fall through.
		case Commands::Scene::Ellipse::TYPE(): // Fall through.
		case Commands::Scene::EllipseFill::TYPE(): // Fall through.
		case Commands::Scene::SetActor::TYPE(): {
				Commands::Scene::PaintableBase::Paint* cmd = Command::as<Commands::Scene::PaintableBase::Paint>(back);
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

	Math::Vec2i getActorPositionUnderCursor(const Math::Vec2i &cursor) const {
		Math::Vec2i pos = cursor;
		if (!_selection.hoveringActor.invalid())
			pos += _selection.hoveringActor.offset;
		else if (!_selection.selectedActor.invalid())
			pos += _selection.selectedActor.offset;

		return pos;
	}
	const char* getActorUpdateRoutine(const ActorAssets::Entry* entry_, const Math::Vec2i &pos) const {
		const ActorRoutineOverriding::Array &actorRoutines = entry()->actorRoutineOverridings;
		const ActorRoutineOverriding* overriding = Routine::search(actorRoutines, pos);
		if (overriding && !overriding->routines.update.empty())
			return overriding->routines.update.c_str();

		if (entry_ && entry_->data && !entry_->data->updateRoutine().empty())
			return entry_->data->updateRoutine().c_str();

		return nullptr;
	}
	const char* getActorOnHitsRoutine(const ActorAssets::Entry* entry_, const Math::Vec2i &pos) const {
		const ActorRoutineOverriding::Array &actorRoutines = entry()->actorRoutineOverridings;
		const ActorRoutineOverriding* overriding = Routine::search(actorRoutines, pos);
		if (overriding && !overriding->routines.onHits.empty())
			return overriding->routines.onHits.c_str();

		if (entry_ && entry_->data && !entry_->data->onHitsRoutine().empty())
			return entry_->data->onHitsRoutine().c_str();

		return nullptr;
	}
	void bindActorCallback(const Math::Vec2i &pos, int index, const std::initializer_list<Variant> &args) {
		ActorRoutineOverriding::Array actorRoutines = entry()->actorRoutineOverridings;
		ActorRoutineOverriding* overriding = Routine::search(actorRoutines, pos);

		if (!overriding) {
			overriding = Routine::add(actorRoutines, pos);
			GBBASIC_ASSERT(overriding && "Impossible.");
			if (!overriding)
				return;
		}

		const Variant &arg = *args.begin();
		const std::string idx = (std::string)arg;
		const bool changed =
			(index == 0 && idx != overriding->routines.update) ||
			(index == 1 && idx != overriding->routines.onHits);
		if (!changed)
			return;

		if (index == 0)
			overriding->routines.update = idx;
		else if (index == 1)
			overriding->routines.onHits = idx;

		_selection.actorRoutineOverridingsFilled = false;
		Commands::Scene::SetActorRoutineOverridings* cmd = (Commands::Scene::SetActorRoutineOverridings*)enqueue<Commands::Scene::SetActorRoutineOverridings>()
			->with(actorRoutines)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);
	}

	/**
	 * @return The matched trigger count.
	 */
	int queryTrigger(const Math::Vec2i &pos, Editing::TriggerIndex::Array &triggers_) const {
		triggers_.clear();

		for (int i = (int)triggers()->size() - 1; i >= 0; --i) {
			const Trigger &tri = (*triggers())[i];
			const Math::Recti rect = tri.toRect();
			if (Math::intersects(rect, pos))
				triggers_.push_back((Byte)i);
		}

		return (int)triggers_.size();
	}
	/**
	 * @param[in] index The position to get.
	 * @return Whether the adding operation succeeded.
	 */
	bool getTrigger(int index, Trigger* trigger /* nullable */) {
		if (index < 0 || index >= (int)triggers()->size())
			return false;

		if (trigger)
			*trigger = (*triggers())[index];

		return true;
	}
	/**
	 * @param[in] index The position to set.
	 * @return Whether the adding operation succeeded.
	 */
	bool setTrigger(int index, const Trigger &trigger) {
		if (index < 0 || index >= (int)triggers()->size())
			return false;

		(*triggers())[index] = trigger;

		return true;
	}
	/**
	 * @param[in] index The position to add before, -1 for append.
	 * @return Whether the adding operation succeeded.
	 */
	bool addTrigger(int index, const Trigger &trigger) {
		if (index == -1 || index == (int)triggers()->size()) {
			triggers()->push_back(trigger);

			return true;
		}
		if (index > 0 && index < (int)triggers()->size()) {
			triggers()->insert(triggers()->begin() + index, trigger);

			return true;
		}

		return false;
	}
	/**
	 * @param[in] index The position to remove.
	 * @return Whether the adding operation succeeded.
	 */
	bool removeTrigger(int index, Trigger* trigger /* nullable */) {
		if (index < 0 || index >= (int)triggers()->size())
			return false;

		if (trigger)
			*trigger = (*triggers())[index];

		triggers()->erase(triggers()->begin() + index);

		return true;
	}
	void bindTriggerCallback(Trigger trigger, const std::initializer_list<Variant> &args) {
		const Variant &arg = *args.begin();
		const std::string idx = (std::string)arg;
		if (idx == trigger.eventRoutine)
			return;

		_selection.triggerRoutinesFilled = false;
		Trigger trigger_ = trigger;
		if (trigger_.eventType == TRIGGER_HAS_NONE)
			trigger_.eventType = TRIGGER_HAS_ENTER_SCRIPT;
		trigger_.eventRoutine = idx;
		Commands::Scene::SetTriggerRoutines* cmd = (Commands::Scene::SetTriggerRoutines*)enqueue<Commands::Scene::SetTriggerRoutines>()
			->with(_binding.getTrigger, _binding.setTrigger)
			->with(_selection.hoveringTrigger, trigger_)
			->with(_setLayer, _tools.layer)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);
	}

	bool playSceneTesting(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		if (ws->running())
			return true;

		if (_tools.isPlaying)
			stopSceneTesting(wnd, rnd, ws);

		if (!object())
			return false;

		// Start measuring performance.
		const long long start = DateTime::ticks();

		// Compile.
		const Bytes::Ptr rom_ = compileScene(wnd, rnd, ws, this, entry(), _tools, _isPlayerBehaviour);

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
	}
	bool stopSceneTesting(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		// Close the device.
		if (ws->running())
			ws->stop(wnd, rnd);

		// Stop playing.
		_tools.isPlaying = false;
#if EDITOR_SCENE_UNLOAD_SYMBOLS_ON_FINISH
		_tools.isPlayerConfigLoaded = false;
		_tools.playerConfigText.clear();
		_tools.playerSymbolsText.clear();
		_tools.playerAliasesText.clear();
#endif /* EDITOR_SCENE_UNLOAD_SYMBOLS_ON_FINISH */

		// Finish.
		return true;
	}
	static Bytes::Ptr compileScene(Window*, Renderer*, Workspace* ws, EditorSceneImpl* self, const SceneAssets::Entry* entry_, Tools &tools, ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour) {
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
			self->warn(ws, "No valid scene player.", true);

			return nullptr;
		}

		const GBBASIC::Kernel::Ptr &krnl = ws->kernels().front();
		if (!krnl) {
			self->warn(ws, "No valid kernel.", true);

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
				self->warn(ws, "No valid config.", true);

				return nullptr;
			}
			if (!file->readString(configTxt)) {
				file->close();

				self->warn(ws, "No valid config.", true);

				return nullptr;
			}
			file->close();

			Editing::SymbolTable::Dictionary dict;
			std::string symTxt;
			std::string aliasesTxt;
			Editing::SymbolTable symbols;
			const bool loaded = symbols.load(
				dict, sym, symTxt, aliases, aliasesTxt,
				[self, ws] (const char* msg) -> void {
					self->warn(ws, msg, true);
				}
			);
			if (!loaded) {
				self->warn(ws, "No valid symbol.", true);

				return nullptr;
			}

			tools.isPlayerConfigLoaded = true;
			tools.playerConfigText     = configTxt;
			tools.playerSymbolsText    = symTxt;
			tools.playerAliasesText    = aliasesTxt;
		}

		// Compile.
		print_("Begin compiling for scene testing.");

		AssetsBundle::Ptr assets(new AssetsBundle());
		do {
			// Prepare.
			struct IndexedActor {
				int index = 0;
				ActorAssets::Entry* entry = nullptr;

				IndexedActor() {
				}
				IndexedActor(int idx, ActorAssets::Entry* ptr) : index(idx), entry(ptr) {
				}
			};
			typedef std::map<int, IndexedActor> ActorMap;

			const Project::Ptr &prj = ws->currentProject();
			GBBASIC_ASSERT(prj && "Impossible.");

			// Get the map asset.
			const int refMap = entry_->refMap;
			const MapAssets::Entry* mapEntry = entry_->getMap(refMap);
			if (!mapEntry || !mapEntry->data) {
				error_("Invalid map asset.");

				break;
			}

			// Add the palette asset.
			assets->palette = prj->assets()->palette;

			// Add the tiles asset.
			const int ref = mapEntry->ref;
			const TilesAssets::Entry* tilesEntry = mapEntry->getTiles(ref);
			if (!tilesEntry || !tilesEntry->data) {
				error_("Invalid tiles asset.");

				break;
			}
			assets->tiles.add(*tilesEntry);

			// Add the map asset.
			MapAssets::Entry mapEntry_ = *mapEntry;
			mapEntry_.ref = 0;
			assets->maps.add(mapEntry_);

			const int n = Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT / 2;
			const bool localPaletteEnabled = mapEntry_.localPaletteEnabled;
			const PaletteAssets::Array &localPalette = mapEntry_.localPalette;
			if (localPaletteEnabled && (int)localPalette.size() >= n && assets->palette.count() >= n) {
				for (int i = 0; i < n; ++i) {
					PaletteAssets::Entry* palEntry = assets->palette.get(i);
					*palEntry = localPalette[i]; // Replace with local palette.
				}
			}

			// Add the actor assets.
			SceneAssets::Entry::UniqueRef uref;
			const SceneAssets::Entry::Ref refActors_ = entry_->getRefActors(&uref);
			(void)refActors_;
			ActorMap actorMap;
			bool invalidActor = false;
			int playerCount = 0;
			for (int refActor : uref) {
				ActorAssets::Entry* actorEntry = entry_->getActor(refActor);
				if (!actorEntry || !actorEntry->data) {
					invalidActor = true;

					break;
				}

				const int idx = (int)actorMap.size();
				actorMap[refActor] = IndexedActor(idx, actorEntry); // Re-indexed.

				const UInt8 bhvr = actorEntry->definition.behaviour;
				if (isPlayerBehaviour(bhvr))
					++playerCount;
			}
			if (invalidActor) {
				error_("Invalid actor asset.");

				break;
			}

			for (ActorMap::value_type kv : actorMap) {
				const int refActor = kv.first;
				(void)refActor;
				const IndexedActor &indexedActor = kv.second;
				ActorAssets::Entry* actorEntry = indexedActor.entry;
				Actor* newActor = nullptr;
				actorEntry->data->clone(&newActor, false);

				if (!newActor->updateRoutine().empty())
					newActor->updateRoutine("");
				if (!newActor->onHitsRoutine().empty())
					newActor->onHitsRoutine("");

				ActorAssets::Entry actorEntry_ = *actorEntry;
				actorEntry_.data = Actor::Ptr(newActor);
				assets->actors.add(actorEntry_);
			}

			// Add the scene asset.
			Scene* newScene = nullptr;
			entry_->data->clone(&newScene, false);
			if (!newScene)
				break;

			newScene->triggerLayer()->clear();

			SceneAssets::Entry sceneEntry = *entry_;
			sceneEntry.data = Scene::Ptr(newScene);
			sceneEntry.refMap = 0;
			Map::Ptr actorLayer = sceneEntry.data->actorLayer();
			for (int j = 0; j < actorLayer->height(); ++j) {
				for (int i = 0; i < actorLayer->width(); ++i) {
					const int cel = actorLayer->get(i, j);
					if (cel == Scene::INVALID_ACTOR())
						continue;

					ActorMap::const_iterator it = actorMap.find(cel);
					if (it == actorMap.end())
						continue;

					const int newIdx = it->second.index;
					actorLayer->set(i, j, newIdx, false);
				}
			}
			sceneEntry.actorRoutineOverridings.clear();
			assets->scenes.add(sceneEntry);

			// Add a dummy code asset.
			const std::string src = Text::format(
				RES_CODE_PLAY_SCENE_TESTING,
				{
					Text::toString(playerCount)
				}
			);
			assets->code.add(src);
		} while (false);

		const Bytes::Ptr rom_ = Workspace::compile(
			config, tools.playerConfigText,
			rom,
			sym, tools.playerSymbolsText,
			aliases, tools.playerAliasesText,
			"Scene", assets,
			bootstrapBank,
			print_, warn_, error_
		);

		print_("End compiling for scene testing.");

		if (!rom_)
			return nullptr;

		print_("Ok.");

		// Finish.
		return rom_;
	}
};

EditorScene* EditorScene::create(void) {
	EditorSceneImpl* result = new EditorSceneImpl();

	return result;
}

void EditorScene::destroy(EditorScene* ptr) {
	EditorSceneImpl* impl = static_cast<EditorSceneImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
