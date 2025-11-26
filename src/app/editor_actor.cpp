/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_actor.h"
#include "editor_actor.h"
#include "editor_properties.h"
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

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_ACTOR_UNLOAD_SYMBOLS_ON_FINISH
#	define EDITOR_ACTOR_UNLOAD_SYMBOLS_ON_FINISH 1
#endif /* EDITOR_ACTOR_UNLOAD_SYMBOLS_ON_FINISH */

/* ===========================================================================} */

/*
** {===========================================================================
** Actor editor
*/

class EditorActorImpl : public EditorActor {
private:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> PixelGetter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> PixelSetter;

	typedef std::function<int(const Math::Recti* /* nullable */, Editing::Dots &)> PixelsGetter;
	typedef std::function<int(const Math::Recti* /* nullable */, const Editing::Dots &)> PixelsSetter;

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

	Editing::Brush _cursor;
	struct {
		bool tilewise = false;
		Math::Vec2i initial = Math::Vec2i(-1, -1);
		Editing::Brush brush;

		ImVec2 mouse = ImVec2(-1, -1);
		int idle = 1;

		void clear(void) {
			tilewise = false;
			initial = Math::Vec2i(-1, -1);
			brush = Editing::Brush();

			mouse = ImVec2(-1, -1);
			idle = 1;
		}
		int area(Math::Recti &area) const {
			if (brush.empty())
				return 0;

			area = Math::Recti::byXYWH(brush.position.x, brush.position.y, brush.size.x, brush.size.y);

			return area.width() * area.height();
		}
	} _selection;
	struct {
		bool filled = false;
		Text::Array routines;

		void clear(void) {
			filled = false;
			routines.clear();
		}
	} _routine;
	struct {
		int cursor = -1;
		int hovering = -1;
		Text::Array named;

		void clear(void) {
			cursor = -1;
			hovering = -1;
			named.clear();
		}
		void fill(int count, int figure) {
			if (count == 0)
				cursor = 0;
			else
				cursor = Math::clamp(cursor, 0, count - 1);
			named.clear();
			for (int i = 0; i < count; ++i) {
				std::string txt = Text::toString(i);
				if (figure == i)
					txt += '*';
				named.push_back(txt);
			}
		}
	} _frame;
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
		Editing::Brush cursor;
		Marker<bool> compacting = Marker<bool>(false);
		std::string info;

		void clear(void) {
			text.clear();
			cursor = Editing::Brush();
			compacting = Marker<bool>(false);
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
		Math::Vec2i inPixels(const Actor* actor) const {
			const bool is8x16 = actor->is8x16();
			if (is8x16)
				return Math::Vec2i(width * 8, height * 16);
			else
				return Math::Vec2i(width * GBBASIC_TILE_SIZE, height * GBBASIC_TILE_SIZE);
		}
	} _size;
	struct {
		int x = 0;
		int y = 0;

		void clear(void) {
			x = 0;
			y = 0;
		}
	} _anchor;
	struct {
		Renderer* renderer = nullptr; // Foreign.
		Image::Ptr image = nullptr;
		Texture::Ptr texture = nullptr;
		bool filled = false;
		Semaphore generated;

		void clear(void) {
			renderer = nullptr;
			image = nullptr;
			texture = nullptr;
			filled = false;
			generated.wait();
		}
	} _compacted;
	struct {
		PixelGetter getPixel = nullptr;
		PixelSetter setPixel = nullptr;
		PixelsGetter getPixels = nullptr;
		PixelsSetter setPixels = nullptr;

		bool skipFrame = false;

		bool empty(void) const {
			return !getPixel || !setPixel || !getPixels || !setPixels;
		}
		void clear(void) {
			getPixel = nullptr;
			setPixel = nullptr;
			getPixels = nullptr;
			setPixels = nullptr;

			skipFrame = false;
		}
		void fill(PixelGetter getPixel_, PixelSetter setPixel_, PixelsGetter getPixels_, PixelsSetter setPixels_) {
			getPixel = getPixel_;
			setPixel = setPixel_;
			getPixels = getPixels_;
			setPixels = setPixels_;
		}
	} _binding;
	struct {
		Image::Ptr blank = nullptr;
		Texture::Ptr texture = nullptr;

		bool empty(void) const {
			return !texture;
		}
		void clear(void) {
			blank = nullptr;
			texture = nullptr;
		}
	} _overlay;

	struct Ref : public Editor::Ref {
		int palette = 0;

		void clear(void) {
			palette = 0;
		}
	} _ref;
	std::function<void(int)> _setFrame = nullptr;
	struct Tools {
		Editing::Tools::PaintableTools painting = Editing::Tools::PENCIL;
		bool scaled = false;
		int magnification = -1;
		int weighting = 0;
		std::string namableText;

		bool focused = false;
		bool inputFieldFocused = false;
		bool showGrids = true;
		bool gridsVisible = false;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);
		bool transparentBackbroundVisible = true;
		bool routinesUnfolded = false;
		int toBindCallback = -1;
		bool definitionUnfolded = false;
		float definitionHeight = 0;
		active_t definitionShadow;

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);
		int mouseActionButton = ImGuiMouseButton_Left;

		std::function<bool(void)> playActorTesting = nullptr;
		std::function<bool(void)> stopActorTesting = nullptr;
		bool isPlaying = false;
		bool isPlayerSymbolsLoaded = false;
		std::string playerSymbolsText;
		std::string playerAliasesText;

		PostHandler post = nullptr;
		Editing::Tools::PaintableTools postType = Editing::Tools::PENCIL;

		void clear(void) {
			painting = Editing::Tools::PENCIL;
			scaled = false;
			magnification = -1;
			weighting = 0;
			namableText.clear();

			focused = false;
			inputFieldFocused = false;
			showGrids = true;
			gridsVisible = false;
			gridUnit = Math::Vec2i(0, 0);
			transparentBackbroundVisible = true;
			routinesUnfolded = false;
			toBindCallback = -1;
			definitionUnfolded = false;
			definitionHeight = 0;
			definitionShadow = active_t();

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);
			mouseActionButton = ImGuiMouseButton_Left;

			playActorTesting = nullptr;
			stopActorTesting = nullptr;
			isPlaying = false;
			isPlayerSymbolsLoaded = false;
			playerSymbolsText.clear();
			playerAliasesText.clear();

			post = nullptr;
			postType = Editing::Tools::PENCIL;
		}
	} _tools;
	Debounce _debounce;
	Processor _processors[Editing::Tools::COUNT] = {
		Processor( // Hand.
			std::bind(&EditorActorImpl::handToolDown, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::handToolMove, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::handToolUp, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::handToolHover, this, std::placeholders::_1)
		),
		Processor( // Eyedropper.
			std::bind(&EditorActorImpl::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		),
		Processor( // Pencil.
			std::bind(&EditorActorImpl::paintToolDown<Commands::Actor::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolMove<Commands::Actor::Pencil, false>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorActorImpl::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Lasso.
			std::bind(&EditorActorImpl::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::lassoToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Line.
			std::bind(&EditorActorImpl::paintToolDown<Commands::Actor::Line>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolMove<Commands::Actor::Line>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box.
			std::bind(&EditorActorImpl::paintToolDown<Commands::Actor::Box>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolMove<Commands::Actor::Box>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box fill.
			std::bind(&EditorActorImpl::paintToolDown<Commands::Actor::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolMove<Commands::Actor::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse.
			std::bind(&EditorActorImpl::paintToolDown<Commands::Actor::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolMove<Commands::Actor::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse fill.
			std::bind(&EditorActorImpl::paintToolDown<Commands::Actor::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolMove<Commands::Actor::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorActorImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Pasting (stamp).
			nullptr,
			nullptr,
			nullptr,
			nullptr
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
	EditorActorImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Actor::AddFrame>()
			->reg<Commands::Actor::CutFrame>()
			->reg<Commands::Actor::DeleteFrame>()
			->reg<Commands::Actor::Pencil>()
			->reg<Commands::Actor::Line>()
			->reg<Commands::Actor::Box>()
			->reg<Commands::Actor::BoxFill>()
			->reg<Commands::Actor::Ellipse>()
			->reg<Commands::Actor::EllipseFill>()
			->reg<Commands::Actor::Fill>()
			->reg<Commands::Actor::Replace>()
			->reg<Commands::Actor::Rotate>()
			->reg<Commands::Actor::Flip>()
			->reg<Commands::Actor::Cut>()
			->reg<Commands::Actor::Paste>()
			->reg<Commands::Actor::Delete>()
			->reg<Commands::Actor::Set8x16>()
			->reg<Commands::Actor::SetProperties>()
			->reg<Commands::Actor::SetPropertiesToAll>()
			->reg<Commands::Actor::SetFigure>()
			->reg<Commands::Actor::SetName>()
			->reg<Commands::Actor::SetRoutines>()
			->reg<Commands::Actor::SetViewAs>()
			->reg<Commands::Actor::SetDefinition>()
			->reg<Commands::Actor::Resize>()
			->reg<Commands::Actor::SetAnchor>()
			->reg<Commands::Actor::SetAnchorForAll>()
			->reg<Commands::Actor::Import>()
			->reg<Commands::Actor::ImportFrame>();
	}
	virtual ~EditorActorImpl() override {
		close(_index);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window* wnd, class Renderer* rnd, class Workspace* ws, class Project* project, int index, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		int another = -1;
		if (entry()->name.empty() || !_project->canRenameActor(_index, entry()->name, &another)) {
			if (entry()->name.empty()) {
				const std::string msg = Text::format(ws->theme()->warning_ActorActorNameIsEmptyAtPage(), { Text::toString(_index) });
				warn(ws, msg, true);
			} else {
				const std::string msg = Text::format(ws->theme()->warning_ActorDuplicateActorNameAtPages(), { entry()->name, Text::toString(_index), Text::toString(another) });
				warn(ws, msg, true);
			}
			entry()->name = _project->getUsableActorName(_index); // Unique name.
			_project->hasDirtyAsset(true);
		}

		_frame.fill(object()->count(), entry()->figure);

		if (currentFrame()) {
			const Math::Vec2i tc = currentFrame()->count(object().get());
			_size.width = tc.x;
			_size.height = tc.y;

			_anchor.x = currentFrame()->anchor.x;
			_anchor.y = currentFrame()->anchor.y;
		}

		_compacted.renderer = rnd;

		_setFrame = std::bind(&EditorActorImpl::changeFrame, this, wnd, rnd, ws, std::placeholders::_1);

		_refresh = std::bind(&EditorActorImpl::refresh, this, ws, std::placeholders::_1);

		_checker = [ws, this] (void) -> void {
			_warnings.clear();

			const active_t &def = entry()->definition;
			const int n = object()->count();
			for (int i = 0; i < GBBASIC_COUNTOF(def.animations); ++i) {
				const animation_t &anim = def.animations[i];
				if (anim.begin < 0 || anim.begin > n || anim.end < 0 || anim.end > n) {
					const std::string msg = Text::format(ws->theme()->warning_ActorFrameIndexOutOfBoundsOfAnimationN(), { Text::toString(i) });
					warn(ws, msg, true);
				}
			}
		};
		_checker();

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_tools.magnification = entry()->magnification;
		_tools.namableText = entry()->name;
		_tools.showGrids = _project->preferencesShowGrids();
		_tools.gridsVisible = !caps.pressed();
		_tools.gridUnit = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
		_tools.transparentBackbroundVisible = num.pressed();
		_tools.definitionShadow = entry()->definition;
		_tools.playActorTesting = std::bind(&EditorActorImpl::playActorTesting, this, wnd, rnd, ws);
		_tools.stopActorTesting = std::bind(&EditorActorImpl::stopActorTesting, this, wnd, rnd, ws);

		fprintf(stdout, "Actor editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Actor editor closed: #%d.\n", _index);

		_compacted.clear();

		_project = nullptr;
		_index = -1;

		_selection.clear();
		_routine.clear();
		_frame.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_checker = nullptr;
		_estimated.clear();
		_painting.clear();
		_size.clear();
		_anchor.clear();
		_binding.clear();
		_overlay.clear();

		_ref.clear();
		_setFrame = nullptr;
		_tools.clear();
		_debounce.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace* ws) override {
		if (!_compacted.filled)
			compactAllEntriesSlices(ws, false);
	}
	virtual void leave(class Workspace*) override {
		_tools.stopActorTesting();

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

	virtual void copy(void) override {
		copy(true);
	}
	void copy(bool clearSelection) {
		if (!currentFrame())
			return;

		if (_tools.focused)
			return;

		auto toString = [] (const Image::Ptr obj, const Math::Recti &area, const Editing::Dots &dots) -> std::string {
			rapidjson::Document doc;
			Jpath::set(doc, doc, area.width(), "width");
			Jpath::set(doc, doc, area.height(), "height");
			Jpath::set(doc, doc, obj->paletted() ? 8 : 0, "depth");
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

		if (!_binding.getPixels || !_binding.setPixels)
			return;

		Math::Recti sel;
		const Math::Recti* selPtr = nullptr;
		int size = _selection.area(sel);
		if (size == 0)
			size = currentFrame()->width() * currentFrame()->height();
		else
			selPtr = &sel;

		typedef std::vector<int> Data;

		Data vec(size, 0);
		Editing::Dots dots(&vec.front());
		_binding.getPixels(selPtr, dots);

		const std::string buf = toString(currentFrame()->pixels, sel, dots);
		const std::string osstr = Unicode::toOs(buf);

		Platform::setClipboardText(osstr.c_str());

		if (clearSelection)
			_selection.clear();
	}
	virtual void cut(void) override {
		if (!currentFrame())
			return;

		if (_tools.focused)
			return;

		if (!_binding.getPixel || !_binding.setPixel)
			return;

		copy(false);

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		Command* cmd = enqueue<Commands::Actor::Cut>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(sel)
			->with(_setFrame, _frame.cursor)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		_selection.clear();
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		if (!currentFrame())
			return;

		if (_tools.focused)
			return;

		auto fromString = [] (const Image::Ptr obj, const std::string &buf, Math::Recti &area, Editing::Dot::Array &dots) -> bool {
			rapidjson::Document doc;
			if (!Json::fromString(doc, buf.c_str()))
				return false;

			int width = -1, height = -1;
			int depth = -1;
			if (!Jpath::get(doc, width, "width"))
				return false;
			if (!Jpath::get(doc, height, "height"))
				return false;
			if (!Jpath::get(doc, depth, "depth"))
				return false;
			area = Math::Recti::byXYWH(0, 0, width, height);
			if (obj->paletted() && depth != 8)
				return false;
			if (!obj->paletted() && depth != 0)
				return false;
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					if (obj->paletted()) {
						int idx = -1;
						if (!Jpath::get(doc, idx, "data", k))
							return false;

						Editing::Dot dot;
						dot.indexed = idx;
						dots.push_back(dot);
					} else {
						UInt32 col = 0;
						if (!Jpath::get(doc, col, "data", k))
							return false;

						Editing::Dot dot;
						dot.colored.fromRGBA(col);
						dots.push_back(dot);
					}

					++k;
				}
				dots.shrink_to_fit();
			}

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Math::Recti area;
		Editing::Dot::Array dots;
		if (!fromString(currentFrame()->pixels, buf, area, dots))
			return;

		const Editing::Tools::PaintableTools prevTool = _tools.painting;
		if (prevTool != Editing::Tools::STAMP) {
			_tools.painting = Editing::Tools::STAMP;

			_processors[Editing::Tools::STAMP] = Processor{
				nullptr,
				nullptr,
				std::bind(&EditorActorImpl::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
				std::bind(&EditorActorImpl::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
			};
		}

		destroyOverlay();
	}
	virtual void del(bool) override {
		if (!currentFrame())
			return;

		if (_tools.focused)
			return;

		if (!_binding.getPixel || !_binding.setPixel)
			return;

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		Command* cmd = enqueue<Commands::Actor::Delete>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(sel)
			->with(_setFrame, _frame.cursor)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		_selection.clear();
	}
	int area(Math::Recti* area /* nullable */) const {
		if (!currentFrame())
			return 0;

		Math::Recti sel;
		int size = _selection.area(sel);
		if (size == 0) {
			sel = Math::Recti::byXYWH(0, 0, currentFrame()->width(), currentFrame()->height());
			size = currentFrame()->width() * currentFrame()->height();
		}
		if (area)
			*area = sel;

		return size;
	}
	virtual bool selectable(void) const override {
		return true;
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

		const int width = currentFrame()->width();
		const int height = currentFrame()->height();
		_commands->redo(object(), Variant((void*)entry()));

		if (width != currentFrame()->width() || height != currentFrame()->height())
			_selection.clear();

		_refresh(cmd);

		_project->toPollEditor(true);

		entry()->increaseRevision();
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		const int width = currentFrame()->width();
		const int height = currentFrame()->height();
		_commands->undo(object(), Variant((void*)entry()));

		if (width != currentFrame()->width() || height != currentFrame()->height())
			_selection.clear();

		_refresh(cmd);

		_project->toPollEditor(true);

		entry()->increaseRevision();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case RESIZE: {
				if (!currentFrame())
					return Variant(false);

				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				if (width == currentFrame()->width() && height == currentFrame()->height())
					return Variant(true);

				Command* cmd = enqueue<Commands::Actor::Resize>()
					->with(Math::Vec2i(width, height), Math::Vec2i(width / GBBASIC_TILE_SIZE, height / GBBASIC_TILE_SIZE))
					->with(_setFrame, _frame.cursor)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);

				_selection.clear();
			}

			return Variant(true);
		case SELECT_ALL:
			if (!currentFrame())
				return Variant(false);

			_selection.brush.position = Math::Vec2i(0, 0);
			_selection.brush.size = Math::Vec2i(currentFrame()->width(), currentFrame()->height());

			return Variant(true);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		case SET_FRAME: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				if (index < 0 || index >= object()->count())
					return Variant(false);

				_setFrame(index);
			}

			return Variant(true);
		case BIND_CALLBACK: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, 0);

				_tools.toBindCallback = index;
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
		ImGuiIO &io = ImGui::GetIO();
		ImGuiStyle &style = ImGui::GetStyle();

		if (_binding.empty()) {
			_binding.fill(
				std::bind(&EditorActorImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorActorImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorActorImpl::getPixels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorActorImpl::setPixels, this, rnd, std::placeholders::_1, std::placeholders::_2)
			);
		}

		const bool allowMouseOperations = shortcuts(wnd, rnd, ws);

		const Ref::Splitter splitter = _ref.split();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		const float height_ = height - statusBarHeight;
		if (!entry() || !object()) {
			ImGui::BeginChild("@Blk", ImVec2(width, height_), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
			ImGui::EndChild();
			refreshStatus(wnd, rnd, ws, nullptr);
			renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);

			return;
		}

		if (_project->preferencesPreviewPaletteBits()) {
			ActorAssets::Entry::PaletteResolver resolver = std::bind(&ActorAssets::Entry::colorAt, entry(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
			entry()->touch(resolver); // Ensure the runtime resources are ready.
		} else {
			entry()->touch(); // Ensure the runtime resources are ready.
		}

		bool inputFieldFocused = false;
		bool inputFieldFocused_ = false;
		auto canUseShortcuts = [ws, this] (void) -> bool {
			return !_tools.inputFieldFocused && ws->canUseShortcuts() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
		};

		const float sequenceWidth = 64.0f;
		float frameWidth = 0.0f;
		if (splitter.first >= sequenceWidth * 3.5f) {
			frameWidth = splitter.first - sequenceWidth;
			ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
			ImGui::BeginChild("@Seq", ImVec2(sequenceWidth, height_), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
			if (currentFrame()) {
				const int oldWidth = currentFrame()->width();
				const int oldHeight = currentFrame()->height();
				int hovering = _frame.hovering;
				bool appended = false;
				const bool changed = Editing::frames(
					rnd, ws,
					object().get(),
					_frame.named,
					ImGui::GetContentRegionAvail().x,
					&_frame.cursor, &hovering,
					&appended,
					canUseShortcuts(),
					_project->preferencesPreviewAnchorPoint()
				);
				if (hovering >= 0) {
					_frame.hovering = hovering;
				}
				if (appended) {
					const int frame = _frame.cursor;
					frameAdded(rnd, ws, frame, true);
				}
				if (changed || appended) {
					if (oldWidth > currentFrame()->width() || oldHeight > currentFrame()->height())
						_selection.clear();

					const Math::Vec2i tc = currentFrame()->count(object().get());
					_size.width = tc.x;
					_size.height = tc.y;
					_anchor.x = currentFrame()->anchor.x;
					_anchor.y = currentFrame()->anchor.y;
				}

				statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::SameLine();

			context(wnd, rnd, ws, true);
		} else {
			frameWidth = splitter.first;
		}

		const float posX = ImGui::GetCursorPosX();
		const float posY = ImGui::GetCursorPosY();
		float toolBarHeight = 0;
		ImGui::PushID("@Ctrl");
		{
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2(3, 3));

			const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);
			const float spwidth = frameWidth - (buttonSize.x + style.FramePadding.x * 2) * 3;
			toolBarHeight = buttonSize.y + style.FramePadding.x * 2;

			const float xPos = ImGui::GetCursorPosX();
			ImGui::Dummy(ImVec2(8, 0));
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(ws->theme()->dialogPrompt_Frame());
			ImGui::SameLine();
			{
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

				ImGui::SetNextItemWidth(spwidth - (ImGui::GetCursorPosX() - xPos));
				int index = _frame.cursor;
				if (object()->count() == 0) {
					ImGui::BeginDisabled();
					{
						ImGui::Combo("", &index, nullptr, nullptr, 0);
					}
					ImGui::EndDisabled();
				} else {
					const bool changed = ImGui::Combo(
						"",
						&index,
						[] (void* data, int idx, const char** outText) -> bool {
							const std::string* items = (const std::string*)data;
							*outText = items[idx].c_str();

							return true;
						},
						_frame.named.empty() ? nullptr : &_frame.named.front(), (int)_frame.named.size()
					);
					if (changed) {
						_frame.cursor = index;
					}
					if (ImGui::IsItemHovered()) {
						VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

						ImGui::SetTooltip("(Shift+Tab/Tab)");
					}
				}
			}

			do {
				if (object()->count() == 0) {
					ImGui::BeginDisabled();
					{
						ImGui::SameLine();
						ImGui::ImageButton(ws->theme()->iconMinus()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltip_DeleteFrame().c_str());

						ImGui::SameLine();
						ImGui::ImageButton(ws->theme()->iconInsert()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltipActor_InsertFrame().c_str());
					}
					ImGui::EndDisabled();
				} else {
					ImGui::SameLine();
					if (ImGui::ImageButton(ws->theme()->iconMinus()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltip_DeleteFrame().c_str())) {
						const int frame = _frame.cursor;
						frameRemoved(rnd, ws, frame);
					}

					ImGui::SameLine();
					if (ImGui::ImageButton(ws->theme()->iconInsert()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltipActor_InsertFrame().c_str())) {
						const int frame = _frame.cursor;
						frameAdded(rnd, ws, frame, false);
					}
				}

				ImGui::SameLine();
				if (ImGui::ImageButton(ws->theme()->iconAppend()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), false, ws->theme()->tooltipActor_AppendFrame().c_str())) {
					const int frame = _frame.cursor;
					frameAdded(rnd, ws, frame, true);
				}
			} while (false);
		}
		ImGui::PopID();

		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav;
		if (_tools.scaled) {
			flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		}
		ImGui::SetCursorPos(ImVec2(posX, posY + toolBarHeight));
		ImGui::BeginChild("@Pat", ImVec2(frameWidth, height_ - toolBarHeight), false, flags);
		{
			if (_frame.cursor < 0 || _frame.cursor >= object()->count()) {
				ImGui::TextUnformatted(ws->theme()->status_EmptyCreateANewFrameToContinue());
			} else {
				if (_cursor.empty()) {
					_cursor.position = Math::Vec2i(0, 0);
					_cursor.size = Math::Vec2i(1, 1);
				}

				Editing::Brush cursor = _cursor;
				const bool withCursor = _tools.painting != Editing::Tools::HAND;

				constexpr const int MAGNIFICATIONS[] = {
					1, 2, 4, 8
				};
				const int cwidth = currentFrame()->width();
				const int cheight = currentFrame()->height();
				if (_tools.magnification == -1) {
					const float PADDING = 32.0f;
					const float s = (frameWidth + PADDING) / cwidth;
					const float t = ((height_ - toolBarHeight) + PADDING) / cheight;
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
				_tools.magnification = Math::clamp(_tools.magnification, 0, (int)GBBASIC_COUNTOF(MAGNIFICATIONS));

				const ImVec2 content = ImGui::GetContentRegionAvail();
				float width_ = (float)cwidth;
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
				width_ *= (float)(MAGNIFICATIONS[_tools.magnification]);

				const active_t &def = _tools.definitionShadow;
				const Math::Recti boundingBox(def.bounds.left, def.bounds.top, def.bounds.right, def.bounds.bottom);
				_painting.set(
					Editing::frame(
						rnd, ws,
						object().get(),
						currentFrame()->pixels.get(), currentFrame()->texture.get(),
						width_,
						MAGNIFICATIONS[_tools.magnification],
						withCursor ? &cursor : nullptr, false,
						_selection.brush.empty() ? nullptr : &_selection.brush,
						_overlay.texture ? _overlay.texture.get() : nullptr,
						&_tools.gridUnit, _tools.showGrids && _tools.gridsVisible,
						_tools.transparentBackbroundVisible,
						&currentFrame()->anchor, _project->preferencesPreviewAnchorPoint(),
						&boundingBox, _tools.definitionUnfolded,
						_tools.mouseActionButton
					)
				);
				if (
					_painting ||
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

				if (_binding.skipFrame) {
					_binding.skipFrame = false;
					ws->skipFrame(); // Skip a frame to avoid glitch.
				}
			}

			statusBarActived |= ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			context(wnd, rnd, ws, false);
		}
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
		ImGui::SetCursorPos(ImVec2(splitter.first, posY));
		ImGui::BeginChild("@Tls", ImVec2(splitter.second, height_), true, _ref.windowFlags());
		if (currentFrame()) {
			const float spwidth = _ref.windowWidth(splitter.second);

			do {
				Indexed::Ptr palette = entry()->palette;

				int coloring = -1;
				Colour colors[GBBASIC_PALETTE_PER_GROUP_COUNT];
				for (int i = 0; i < Math::min(palette->count(), GBBASIC_PALETTE_PER_GROUP_COUNT); ++i) {
					palette->get(i, colors[i]);
				}
				const bool changed = Editing::Tools::indexable(
					rnd, ws,
					colors,
					&coloring,
					spwidth,
					canUseShortcuts(),
					_project->preferencesPreviewPaletteBits() ? ws->theme()->tooltip_OpenRefPaletteTurnOffPaletteBitsPreviewToUseThis().c_str() : ws->theme()->tooltip_OpenRefPalette().c_str(),
					[this, &colors] (int index) -> void { // Color has been changed directly from this actor editor.
						// Get the old color.
						Colour old;
						Indexed::Ptr palette = entry()->palette;
						palette->get(index, old);

						// Get the new color.
						const Colour &col = colors[index];

						// Ignore if it hasn't been changed.
						if (old == col)
							return;

						// Set the project data.
						PaletteAssets &assets = _project->touchPalette();
						PaletteAssets::Entry* entry_ = assets.get(entry()->ref);
						entry_->data->set(index, &col);

						_project->hasDirtyAsset(true);

						entry_->increaseRevision();

						// Refresh the editor.
						palette->set(index, &col);

						entry()->cleanup(); // Clean up the outdated editable and runtime resources.

						// Sync the super palettes.
						_project->synchronizeSuperPalettes(assets);
					},
					[rnd, ws, this] (void) -> void { // Open the palette editor.
						ws->showPaletteEditor(
							rnd,
							entry()->ref,
							[rnd, ws, this] (const std::initializer_list<Variant> &args) -> void { // Color or group has been changed.
								// Prepare.
								const Variant &arg = *args.begin();
								const int group = (Int)arg;

								// Set the group.
								const bool groupChanged = entry()->ref != group;
								if (groupChanged) {
									entry()->ref = group;

									entry()->increaseRevision();
								}

								// Gain the palette from the project data.
								const PaletteAssets &paletteAssets = _project->touchPalette();
								const bool refreshed = entry()->refresh(); // Refresh the outdated editable resources.

								// Need to refresh the palette because it wasn't refreshed above.
								const PaletteAssets::Entry* ref_ = paletteAssets.get(entry()->ref);
								if (groupChanged && !refreshed && ref_) {
									Indexed::Ptr palette_ = entry()->palette;
									for (int i = 0; i < Math::min(paletteAssets.count(), GBBASIC_PALETTE_PER_GROUP_COUNT); ++i) {
										Colour col;
										ref_->data->get(i, col);
										palette_->set(i, &col);
									}
									entry()->cleanup();
								}

								// Mark this asset dirty.
								_project->hasDirtyAsset(true);
							}
						);
					}
				);
				if (changed) {
					_ref.palette = coloring;
				}
			} while (false);

			const unsigned mask = 0xffffffff & ~(1 << Editing::Tools::STAMP);
			Editing::Tools::separate(rnd, ws, spwidth);
			if (Editing::Tools::paintable(rnd, ws, &_tools.painting, spwidth, canUseShortcuts(), true, mask)) {
				destroyOverlay();
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, spwidth, canUseShortcuts()))
				entry()->magnification = _tools.magnification;

			switch (_tools.painting) {
			case Editing::Tools::PENCIL: // Fall through.
			case Editing::Tools::LINE: // Fall through.
			case Editing::Tools::BOX: // Fall through.
			case Editing::Tools::BOX_FILL: // Fall through.
			case Editing::Tools::ELLIPSE: // Fall through.
			case Editing::Tools::ELLIPSE_FILL:
				Editing::Tools::separate(rnd, ws, spwidth);
				Editing::Tools::weighable(rnd, ws, &_tools.weighting, spwidth, canUseShortcuts());

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
				if (Editing::Tools::flippable(rnd, ws, &flipping, spwidth, canUseShortcuts(), mask)) {
					flip(flipping);
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
				if (_project->canRenameActor(_index, _tools.namableText, nullptr)) {
					Command* cmd = enqueue<Commands::Actor::SetName>()
						->with(_tools.namableText)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				} else {
					_tools.namableText = entry()->name;

					warn(ws, ws->theme()->warning_ActorNameIsAlreadyInUse(), true);
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			ImGui::PushID("@8x16");
			{
				Editing::Tools::separate(rnd, ws, spwidth);
				bool is8x16 = object()->is8x16();
				if (
					Editing::Tools::togglable(
						rnd, ws,
						&is8x16,
						spwidth,
						ws->theme()->dialogPrompt_8x16().c_str(),
						nullptr
					)
				) {
					Command* cmd = enqueue<Commands::Actor::Set8x16>()
						->with(is8x16)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);

					_selection.clear();
				}
			}
			ImGui::PopID();

			Editing::Tools::separate(rnd, ws, spwidth);
			const Math::Vec2i tc = currentFrame()->count(object().get());
			const int width_ = tc.x;
			const int height_ = tc.y;
			bool sizeChanged = false;
			if (
				Editing::Tools::resizable(
					rnd, ws,
					width_, height_,
					&_size.width, &_size.height,
					GBBASIC_ACTOR_MAX_WIDTH, GBBASIC_ACTOR_MAX_HEIGHT, -1,
					spwidth,
					&inputFieldFocused_,
					ws->theme()->dialogPrompt_Size_InTiles().c_str(),
					ws->theme()->warning_TileSizeOutOfBounds().c_str(),
					std::bind(&EditorActorImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				)
			) {
				_size.width = Math::clamp(_size.width, 1, GBBASIC_ACTOR_MAX_WIDTH);
				_size.height = Math::clamp(_size.height, 1, GBBASIC_ACTOR_MAX_HEIGHT);
				if (_size.width != width_ || _size.height != height_) {
					const Math::Vec2i ps = _size.inPixels(object().get());
					post(RESIZE, ps.x, ps.y);
					sizeChanged = true;

					_anchor.x = currentFrame()->anchor.x;
					_anchor.y = currentFrame()->anchor.y;

					entry()->increaseRevision();
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);
			const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true, false);
			const int anchorX = currentFrame()->anchor.x;
			const int anchorY = currentFrame()->anchor.y;
			bool anchorChanged = false;
			const Math::Vec2i ps(currentFrame()->width(), currentFrame()->height());
			int minX = 0, minY = 0, maxX = ps.x - 1, maxY = ps.y - 1;
			if (shift.pressed(false)) {
				minX = -ps.x * 2;
				minY = -ps.y * 2;
				maxX = ps.x * 2 - 1;
				maxY = ps.y * 2 - 1;
			}
			if (
				Editing::Tools::anchorable(
					rnd, ws,
					anchorX, anchorY,
					&_anchor.x, &_anchor.y,
					minX, minY, maxX, maxY,
					spwidth,
					&inputFieldFocused_,
					ws->theme()->dialogPrompt_Anchor().c_str(),
					ctrl.pressed(false) ? nullptr : ws->theme()->tooltip_HoldCtrlToApplyToTheCurrentFrameOnly().c_str(),
					ws->theme()->warning_ActorAnchorOutOfBounds().c_str(),
					std::bind(&EditorActorImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				)
			) {
				const Math::Vec2i ps_(currentFrame()->width(), currentFrame()->height());
				_anchor.x = Math::clamp(_anchor.x, 0, ps_.x);
				_anchor.y = Math::clamp(_anchor.y, 0, ps_.y);
				if (_anchor.x != anchorX || _anchor.y != anchorY) {
					Command* cmd = nullptr;
					if (ctrl.pressed(false)) {
						cmd = enqueue<Commands::Actor::SetAnchor>()
							->with(Math::Vec2i(_anchor.x, _anchor.y))
							->with(_setFrame, _frame.cursor)
							->exec(object(), Variant((void*)entry()));
					} else {
						cmd = enqueue<Commands::Actor::SetAnchorForAll>()
							->with(Math::Vec2i(_anchor.x, _anchor.y))
							->with(_setFrame, _frame.cursor)
							->exec(object(), Variant((void*)entry()));
					}

					_refresh(cmd);

					_selection.clear();

					anchorChanged = true;

					_anchor.x = Math::min(_anchor.x, currentFrame()->width() - 1);
					_anchor.y = Math::min(_anchor.y, currentFrame()->height() - 1);

					entry()->increaseRevision();
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::clickable(
					rnd, ws,
					spwidth,
					ws->theme()->dialogPrompt_EditProps().c_str()
				)
			) {
				if (!_compacted.filled) {
					compactAllEntriesSlices(ws, false);

					_compacted.generated.wait();
				}

				ws->showPropertiesEditor(
					rnd,
					object(), _frame.cursor,
					entry()->shadow,
					_frame.named,
					[this] (const std::initializer_list<Variant> &args) -> void {
						const Variant &arg = *args.begin();
						const int cursor = (int)(Variant::Int)(*(args.begin() + 1));
						const bool applyToAllTiles = (bool)(*(args.begin() + 2));
						const bool applyToAllFrames = (bool)(*(args.begin() + 3));
						EditorProperties::ImageArray* shadowImageArray = (EditorProperties::ImageArray*)(void*)(*(args.begin() + 4));
						(void)applyToAllTiles;

						if (applyToAllFrames) {
							Commands::Actor::SetPropertiesToAll::LayeredPoints layeredPoints;
							layeredPoints.resize(shadowImageArray->size());
							layeredPoints.shrink_to_fit();
							for (int k = 0; k < (int)shadowImageArray->size(); ++k) {
								const Image::Ptr &props = shadowImageArray->at(k);

								const Actor::Frame* frame = object()->get(k);
								for (int j = 0; j < props->height(); ++j) {
									for (int i = 0; i < props->width(); ++i) {
										const Math::Vec2i p(i, j);
										int u = 0;
										int v = 0;
										frame->properties->get(i, j, u);
										props->get(i, j, v);
										if (u != v) {
											layeredPoints[k].insert(Editing::Point(p, v));
										}
									}
								}
							}

							Commands::Actor::SetPropertiesToAll::Getter getter = [this] (int frame, const Math::Vec2i &pos, Editing::Dot &dot) -> bool {
								GBBASIC_ASSERT(frameAt(frame) && "Impossible.");

								return frameAt(frame)->properties->get(pos.x, pos.y, dot.indexed);
							};
							Commands::Actor::SetPropertiesToAll::Setter setter = [this] (int frame, const Math::Vec2i &pos, const Editing::Dot &dot) -> bool {
								GBBASIC_ASSERT(frameAt(frame) && "Impossible.");

								return frameAt(frame)->properties->set(pos.x, pos.y, dot.indexed);
							};

							Command* cmd = enqueue<Commands::Actor::SetPropertiesToAll>()
								->with(getter, setter)
								->with(layeredPoints)
								->with(_setFrame, cursor)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);
						} else {
							Object::Ptr obj = (Object::Ptr)arg;
							Image::Ptr props = Object::as<Image::Ptr>(obj);

							const Actor::Frame* frame = object()->get(cursor);
							Commands::Paintable::Paint::Points points;
							Commands::Paintable::Paint::Indices values;
							for (int j = 0; j < props->height(); ++j) {
								for (int i = 0; i < props->width(); ++i) {
									const Math::Vec2i p(i, j);
									int u = 0;
									int v = 0;
									frame->properties->get(i, j, u);
									props->get(i, j, v);
									if (u != v) {
										points.push_back(p);
										values.push_back(v);
									}
								}
							}
							points.shrink_to_fit();
							values.shrink_to_fit();

							Commands::Paintable::Paint::Getter getter = [this, cursor] (const Math::Vec2i &pos, Editing::Dot &dot) -> bool {
								GBBASIC_ASSERT(frameAt(cursor) && "Impossible.");

								return frameAt(cursor)->properties->get(pos.x, pos.y, dot.indexed);
							};
							Commands::Paintable::Paint::Setter setter = [this, cursor] (const Math::Vec2i &pos, const Editing::Dot &dot) -> bool {
								GBBASIC_ASSERT(frameAt(cursor) && "Impossible.");

								return frameAt(cursor)->properties->set(pos.x, pos.y, dot.indexed);
							};
							const Math::Vec2i size(currentFrame()->width(), currentFrame()->height());

							Command* cmd = enqueue<Commands::Actor::SetProperties>()
								->with(getter, setter)
								->with(size, points, values)
								->with(_setFrame, cursor)
								->exec(object(), Variant((void*)entry()));

							_refresh(cmd);
						}

						entry()->increaseRevision();
					}
				);
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			if (!_routine.filled) {
				_routine.routines = { object()->updateRoutine(), object()->onHitsRoutine() };
				_routine.filled = true;
			}
			const Editing::Tools::InvokableList oldRoutines = { &object()->updateRoutine(), &object()->onHitsRoutine() };
			const Editing::Tools::InvokableList types = { &ws->theme()->dialogPrompt_Behave(), &ws->theme()->dialogPrompt_OnHits() };
			const Editing::Tools::InvokableList tooltips = { &ws->theme()->tooltip_Threaded(), &ws->theme()->tooltip_Threaded() };
			float height__ = 0.0f;
			const bool changed = Editing::Tools::invokable(
				rnd, ws,
				_project,
				&oldRoutines, &_routine.routines,
				&types, &tooltips,
				&_tools.routinesUnfolded,
				spwidth, &height__,
				ws->theme()->dialogPrompt_Routines().c_str(),
				nullptr,
				[wnd, rnd, ws, this] (int index) -> void {
					ws->showCodeBindingEditor(
						wnd, rnd,
						std::bind(&EditorActorImpl::bindActorCallback, this, index, std::placeholders::_1),
						index == 0 ? object()->updateRoutine() : object()->onHitsRoutine()
					);
				}
			);
			if (changed) {
				_routine.filled = false;
				if (_routine.routines.size() == 2) {
					Commands::Actor::SetRoutines* cmd = (Commands::Actor::SetRoutines*)enqueue<Commands::Actor::SetRoutines>()
						->with(_routine.routines.front(), _routine.routines.back())
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				} else {
					Commands::Actor::SetRoutines* cmd = (Commands::Actor::SetRoutines*)enqueue<Commands::Actor::SetRoutines>()
						->with("", "")
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				}
			}
			if (_tools.toBindCallback != -1) {
				ws->showCodeBindingEditor(
					wnd, rnd,
					std::bind(&EditorActorImpl::bindActorCallback, this, _tools.toBindCallback, std::placeholders::_1),
					_tools.toBindCallback == 0 ? object()->updateRoutine() : object()->onHitsRoutine()
				);

				_tools.toBindCallback = -1;
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			bool asActor = entry()->asActor;
			if (
				Editing::Tools::definable(
					rnd, ws,
					object().get(),
					nullptr /* &entry()->definition */, &_tools.definitionShadow,
					&_tools.definitionUnfolded,
					&asActor,
					spwidth, &_tools.definitionHeight,
					&inputFieldFocused_,
					ws->theme()->dialogPrompt_Definition().c_str(),
					!ctrl.pressed(), // Whether to use simplified edit.
					[this] (Editing::Operations op) -> void {
						switch (op) {
						case Editing::Operations::COPY: // Copy.
							do {
								rapidjson::Document doc;
								if (!_tools.definitionShadow.toJson(doc, doc, _project->behaviourSerializer()))
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

								if (!_tools.definitionShadow.fromJson(doc, _project->behaviourParser()))
									break;

								Command* cmd = enqueue<Commands::Actor::SetDefinition>()
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
				Command* cmd = enqueue<Commands::Actor::SetDefinition>()
					->with(_tools.definitionShadow)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);

				simplify();
			}
			inputFieldFocused |= inputFieldFocused_;
			if (asActor != entry()->asActor) {
				Command* cmd = enqueue<Commands::Actor::SetViewAs>()
					->with(asActor)
					->exec(object(), Variant((void*)entry()));

				_refresh(cmd);
			}

			bool toSave = false;
			Editing::Tools::separate(rnd, ws, spwidth);
			if (
				Editing::Tools::compacted(
					rnd, ws,
					_compacted.texture.get(),
					_compacted.filled,
					&toSave,
					spwidth,
					ws->theme()->dialogPrompt_Compacted().c_str()
				)
			) {
				refreshAllEntriesSlices(ws, false);
			} else if (toSave) {
				saveAllEntriesSlices(ws);
			}

			_tools.inputFieldFocused = inputFieldFocused;

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		if (_debounce.fire()) {
			if (!_compacted.filled)
				compactAllEntriesSlices(ws, false);
		}

		renderStatus(wnd, rnd, ws, width, statusBarHeight, statusBarActived);
	}

	virtual void statusInvalidated(void) override {
		_page.filled = false;
		_status.cursor = Editing::Brush();
		_estimated.filled = false;
	}

	virtual void added(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}
	virtual void removed(BaseAssets::Entry* /* entry */, int /* index */) override {
		// Do nothing.
	}

	virtual void played(class Renderer*, class Workspace*) override {
		_tools.stopActorTesting();
	}
	virtual void stopped(class Renderer*, class Workspace*) override {
		_tools.stopActorTesting();
	}

	virtual void resized(class Renderer*, const Math::Vec2i &, const Math::Vec2i &) override {
		_ref.windowResized();
	}

	virtual void focusLost(class Renderer*) override {
		//entry()->cleanup(); // Clean up the outdated editable and runtime resources.
		// Do nothing.
	}
	virtual void focusGained(class Renderer*) override {
		// Do nothing.
	}

private:
	bool shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
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

		const Editing::Shortcut shiftTab(SDL_SCANCODE_TAB, false, true);
		const Editing::Shortcut tab(SDL_SCANCODE_TAB);
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (shiftTab.pressed()) {
				if (--_frame.cursor < 0)
					_frame.cursor = object()->count() - 1;
				changeFrame(wnd, rnd, ws, _frame.cursor);
			} else if (tab.pressed()) {
				if (++_frame.cursor >= object()->count())
					_frame.cursor = 0;
				changeFrame(wnd, rnd, ws, _frame.cursor);
			}
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
					index = _project->actorPageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->actorPageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, index);
			}
		}

		const Editing::Shortcut ctrlShiftR(SDL_SCANCODE_R, true, true);
		if (ctrlShiftR.pressed() && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			refreshAllEntriesSlices(ws, false);
		}

		return true;
	}

	void context(Window*, Renderer*, Workspace* ws, bool seq) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (seq) {
			if (ImGui::BeginPopup("@Ctx")) {
				if (ImGui::MenuItem(ws->theme()->menu_SetAsFigure())) {
					const int fig = _frame.hovering >= 0 ? _frame.hovering : _frame.cursor;
					Command* cmd = enqueue<Commands::Actor::SetFigure>()
						->with(fig)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);

					_frame.fill(object()->count(), entry()->figure);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltipActor_ForPlacingInSceneEditor());
				}

				ImGui::EndPopup();
			}
		} else {
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
				if (ImGui::MenuItem(ws->theme()->menu_SetAsFigure())) {
					const int fig = _frame.cursor;
					Command* cmd = enqueue<Commands::Actor::SetFigure>()
						->with(fig)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);

					_frame.fill(object()->count(), entry()->figure);
				}

				ImGui::EndPopup();
			}
		}
	}

	void refreshStatus(Window*, Renderer*, Workspace* ws, const Editing::Brush* cursor /* nullable */) {
		if (!_page.filled) {
			_page.text = ws->theme()->status_Pg() + " " + Text::toPageNumber(_index);
			_page.filled = true;
		}
		if (cursor && _status.cursor != *cursor) {
			_status.cursor = *cursor;

			if (_status.cursor.position.x == -1 || _status.cursor.position.y == -1) {
				_status.text = " " + ws->theme()->status_Sz();
				_status.text += " ";
				_status.text += Text::toString(currentFrame()->width());
				_status.text += "x";
				_status.text += Text::toString(currentFrame()->height());
			} else {
				_status.text = " " + ws->theme()->status_Pos();
				_status.text += " ";
				_status.text += Text::toString(_status.cursor.position.x);
				_status.text += ",";
				_status.text += Text::toString(_status.cursor.position.y);
			}
		}
		if (!_estimated.filled) {
			const std::pair<int, int> n = entry()->estimateFinalByteCount();
			if (n.first >= 0)
				_estimated.text = " " + Text::toScaledBytes(n.first);
			else
				_estimated.text = " ?KB";
			_estimated.text += "+" + Text::toScaledBytes(n.second);
			_estimated.filled = true;

			int tileCount = (int)entry()->slices.size();
			if (object()->is8x16())
				tileCount *= 2;
			const Actor::Shadow::Array &shadow = entry()->shadow;
			int maxSprites = 0;
			for (int i = 0; i < (int)shadow.size(); ++i) {
				const Actor::Shadow &sdw = shadow[i];
				const int n = (int)sdw.refs.size();
				if (n > maxSprites)
					maxSprites = n;
			}
			_status.info = Text::format(
				ws->theme()->tooltipActor_Info(),
				{
					Text::toString(tileCount),
					n.first >= 0 ? Text::toScaledBytes(n.first) : "?KB",
					Text::toString(maxSprites),
					Text::toScaledBytes(n.second)
				}
			);
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
				index = _project->actorPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, index);
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
			if (index >= _project->actorPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		if (_tools.isPlaying) {
			if (ImGui::ImageButton(ws->theme()->iconStopPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipActor_StopTesting().c_str())) {
				_tools.stopActorTesting();
			}
		} else {
			if (ws->running()) {
				ImGui::BeginDisabled();
				ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipActor_TestActor().c_str());
				ImGui::EndDisabled();
			} else {
				const bool asActor = entry()->asActor;
				if (asActor) {
					if (ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipActor_TestActor().c_str())) {
						_tools.playActorTesting();
					}
				} else {
					ImGui::BeginDisabled();
					ImGui::ImageButton(ws->theme()->iconStartPreview()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipActor_TestActor().c_str());
					ImGui::EndDisabled();
				}
			}
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(_status.text);
		ImGui::SameLine();
		ImGui::TextUnformatted(_estimated.text);
		ImGui::SameLine();
		do {
			_status.compacting = _compacted.generated.working();
			const Marker<bool>::Values compacting = _status.compacting.values();
			float width_ = 0.0f;
			const float wndWidth = ImGui::GetWindowWidth();
			ImGui::SetCursorPosX(wndWidth - _statusWidth);
			if (compacting.first) {
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
				ImGui::ImageButton(ws->theme()->iconWorking()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltip_Compacting().c_str());
				ImGui::PopStyleColor(2);
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
			} else if (!_compacted.filled) {
				if (ImGui::ImageButton(ws->theme()->iconRefresh()->pointer(rnd), ImVec2(13, 13), ImVec4(1, 1, 1, 1), false, ws->theme()->tooltipActor_Refresh().c_str())) {
					refreshAllEntriesSlices(ws, false);
				}
				width_ += ImGui::GetItemRectSize().x;
				ImGui::SameLine();
			} else {
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
			const Text::Array &assetNames = ws->getActorPageNames();
			const int n = _project->actorPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::ACTOR, i);
				}
			}

			ImGui::EndPopup();
		}

		// Importing.
		if (ImGui::BeginPopup("@Imp")) {
			if (ImGui::BeginMenu(ws->theme()->menu_DataSequence())) {
				if (ImGui::MenuItem(ws->theme()->menu_2Bpp())) {
					do {
						if (!Platform::hasClipboardText()) {
							ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

							break;
						}

						const std::string osstr = Platform::getClipboardText();
						std::string txt = Unicode::fromOs(osstr);
						txt = Parsers::removeComments(txt);
						Image::Ptr newObj = nullptr;
						if (!entry()->parseDataSequence(_frame.cursor, newObj, txt, false)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						Command* cmd = enqueue<Commands::Actor::ImportFrame>()
							->with(newObj, _frame.cursor)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_1Bpp())) {
					do {
						if (!Platform::hasClipboardText()) {
							ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

							break;
						}

						const std::string osstr = Platform::getClipboardText();
						std::string txt = Unicode::fromOs(osstr);
						txt = Parsers::removeComments(txt);
						Image::Ptr newObj = nullptr;
						if (!entry()->parseDataSequence(_frame.cursor, newObj, txt, true)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						Command* cmd = enqueue<Commands::Actor::ImportFrame>()
							->with(newObj, _frame.cursor)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
					} while (false);
				}

				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboardForTheCurrentFrameOnly());
			}
			if (ImGui::MenuItem(ws->theme()->menu_Json())) {
				do {
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					Actor::Ptr newObj = nullptr;
					int figure = 0;
					bool asActor = true;
					active_t def;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, figure, asActor, def, txt, status)) {
						switch (status) {
						case BaseAssets::Entry::ParsingStatuses::NOT_A_MULTIPLE_OF_8x8:
							ws->bubble(ws->theme()->dialogPrompt_ResourceSizeIsNotAMultipleOf8x8(), nullptr);

							break;
						case BaseAssets::Entry::ParsingStatuses::OUT_OF_BOUNDS:
							ws->bubble(ws->theme()->dialogPrompt_ResourceSizeOutOfBounds(), nullptr);

							break;
						default:
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						break;
					}

					Command* cmd = enqueue<Commands::Actor::Import>()
						->with(entry()->palette, newObj, figure, asActor, def)
						->with(_setFrame, _frame.cursor)
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

					Actor::Ptr newObj = nullptr;
					int figure = 0;
					bool asActor = true;
					active_t def;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, figure, asActor, def, txt, status)) {
						switch (status) {
						case BaseAssets::Entry::ParsingStatuses::NOT_A_MULTIPLE_OF_8x8:
							ws->bubble(ws->theme()->dialogPrompt_ResourceSizeIsNotAMultipleOf8x8(), nullptr);

							break;
						case BaseAssets::Entry::ParsingStatuses::OUT_OF_BOUNDS:
							ws->bubble(ws->theme()->dialogPrompt_ResourceSizeOutOfBounds(), nullptr);

							break;
						default:
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						break;
					}

					Command* cmd = enqueue<Commands::Actor::Import>()
						->with(entry()->palette, newObj, figure, asActor, def)
						->with(_setFrame, _frame.cursor)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::MenuItem(ws->theme()->menu_SpriteSheet())) {
				ImGui::FileResolverPopupBox::ConfirmedHandler_PathVec2i confirm(
					[ws, this] (const char* path, const Math::Vec2i &vec) -> void {
						Bytes::Ptr bytes(Bytes::create());
						File::Ptr file(File::create());
						if (!file->open(path, Stream::READ)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return;
						}
						if (!file->readBytes(bytes.get())) {
							file->close(); FileMonitor::unuse(path);

							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							return;
						}
						file->close(); FileMonitor::unuse(path);

						Actor::Ptr newObj = nullptr;
						int figure = entry()->figure;
						bool asActor = entry()->asActor;
						active_t def = entry()->definition;
						BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
						if (!entry()->parseSpriteSheet(newObj, bytes.get(), vec, status)) {
							switch (status) {
							case BaseAssets::Entry::ParsingStatuses::NOT_A_MULTIPLE_OF_8x8:
								ws->bubble(ws->theme()->dialogPrompt_ResourceSizeIsNotAMultipleOf8x8(), nullptr);

								break;
							case BaseAssets::Entry::ParsingStatuses::OUT_OF_BOUNDS:
								ws->bubble(ws->theme()->dialogPrompt_ResourceSizeOutOfBounds(), nullptr);

								break;
							default:
								ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

								break;
							}

							return;
						}
						Math::Recti bounds;
						const bool withSameAnchor = newObj->computeSafeContentRect(bounds);
						if (withSameAnchor)
							def.bounds = boundingbox_t((Int8)bounds.xMin(), (Int8)bounds.xMax(), (Int8)bounds.yMin(), (Int8)bounds.yMax());

						newObj->updateRoutine(object()->updateRoutine()); // Reserve routines.
						newObj->onHitsRoutine(object()->onHitsRoutine());

						Command* cmd = enqueue<Commands::Actor::Import>()
							->with(entry()->palette, newObj, figure, asActor, def)
							->with(_setFrame, _frame.cursor)
							->exec(object(), Variant((void*)entry()));

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CanceledHandler cancel(
					[ws] (void) -> void {
						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::Vec2iResolver resolveVec2i(
					[] (const char* path, Math::Vec2i* size) -> bool {
						Bytes::Ptr bytes(Bytes::create());
						File::Ptr file(File::create());
						if (!file->open(path, Stream::READ))
							return false;
						if (!file->readBytes(bytes.get())) {
							file->close(); FileMonitor::unuse(path);

							return false;
						}
						file->close(); FileMonitor::unuse(path);

						Image::Ptr img(Image::create());
						if (!img->fromBytes(bytes.get()))
							return false;

						return ActorAssets::computeSpriteSheetSlices(img, *size);
					},
					nullptr
				);
				ws->showExternalFileBrowser(
					rnd,
					ws->theme()->generic__Path(),
					GBBASIC_IMAGE_FILE_FILTER,
					true,
					Math::Vec2i(3, 4), Math::Vec2i(1, 1), Math::Vec2i(8, 8), ws->theme()->dialogPrompt_Split().c_str(), " x",
					confirm,
					cancel,
					nullptr,
					nullptr,
					resolveVec2i, ws->theme()->dialogPrompt_Loading().c_str()
				);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			const std::string &code = entry()->asActor ? ws->theme()->menu_CodeAsActor() : ws->theme()->menu_CodeAsProjectile();
			if (ImGui::MenuItem(code)) {
				auto serialize = [ws, this] (int index) -> void {
					std::string txt;
					if (!entry()->serializeBasic(txt, index, entry()->asActor))
						return;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				};

				do {
					if (_compacted.filled) {
						serialize(_index);

						break;
					}

					ImGui::WaitingPopupBox::TimeoutHandler timeout(
						[ws, this, serialize] (void) -> void {
							compactAllEntriesSlices(ws, true);

							_compacted.generated.wait();

							serialize(_index);

							ws->popupBox(nullptr);
						},
						nullptr
					);
					ws->waitingPopupBox(
						true, ws->theme()->dialogPrompt_Compacting(),
						true, timeout,
						true
					);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_C())) {
				do {
					std::string txt;
					if (!entry()->serializeC(txt))
						break;

					Platform::setClipboardText(txt.c_str());

					ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
				} while (false);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::MenuItem(ws->theme()->menu_CFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-actor.c" : Text::sanitizeFilename(entry()->name) + ".c",
						GBBASIC_C_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || ext != "c")
						path += ".c";

					std::string txt;
					if (!entry()->serializeC(txt))
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
			if (ImGui::BeginMenu(ws->theme()->menu_DataSequence())) {
				if (ImGui::MenuItem(ws->theme()->menu_Hex())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(_frame.cursor, txt, 16))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Dec())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(_frame.cursor, txt, 10))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Bin())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(_frame.cursor, txt, 2))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}

				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboardForTheCurrentFrameOnly());
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
						entry()->name.empty() ? "gbbasic-actor.json" : Text::sanitizeFilename(entry()->name) + ".json",
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
			if (ImGui::MenuItem(ws->theme()->menu_SpriteSheet())) {
				ImGui::FileResolverPopupBox::ConfirmedHandler_PathInteger confirm(
					[ws, this] (const char* path, int pitch) -> void {
						std::string path_ = path;
						const Text::Array exts = { "png", "jpg", "bmp", "tga" };
						std::string ext;
						Path::split(path_, nullptr, &ext, nullptr);
						Text::toLowerCase(ext);
						if (ext.empty() || std::find(exts.begin(), exts.end(), ext) == exts.end())
							path_ += ".png";
						Path::split(path_, nullptr, &ext, nullptr);
						Text::toLowerCase(ext);
						const char* y = ext.c_str();

						Bytes::Ptr bytes(Bytes::create());
						if (!entry()->serializeSpriteSheet(bytes.get(), y, pitch))
							return;

						File::Ptr file(File::create());
						if (!file->open(path_.c_str(), Stream::WRITE))
							return;
						file->writeBytes(bytes.get());
						file->close(); FileMonitor::unuse(path_);

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);

						ws->popupBox(nullptr);
					},
					nullptr
				);
				ImGui::FileResolverPopupBox::CanceledHandler cancel(
					[ws] (void) -> void {
						ws->popupBox(nullptr);
					},
					nullptr
				);
				ws->showExternalFileBrowser(
					rnd,
					ws->theme()->generic__Path(),
					GBBASIC_IMAGE_FILE_FILTER,
					false,
					3, 1, 8, ws->theme()->dialogPrompt_Pitch().c_str(),
					true,
					confirm,
					cancel,
					nullptr,
					nullptr
				);
			}

			ImGui::EndPopup();
		}

		// Views.
		if (ImGui::BeginPopup("@Views")) {
			if (ImGui::MenuItem(ws->theme()->menu_Anchor(), nullptr, &_project->preferencesPreviewAnchorPoint())) {
				entry()->cleanup();
				ws->skipFrame(); // Skip a frame to avoid glitch.

				_project->hasDirtyEditor(true);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_PreviewAnchorPoint());
			}
			ImGui::Separator();

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
					_project->preferencesShowGrids(_tools.showGrids);

					_project->hasDirtyEditor(true);
				}
			}

			ImGui::EndPopup();
		}
	}

	void changeFrame(Window* wnd, Renderer* rnd, Workspace* ws, int frame) {
		if (_frame.cursor == frame)
			return;

		_frame.cursor = frame;
		_status.clear();
		refreshStatus(wnd, rnd, ws, &_cursor);
	}

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "Actor editor: ";
				msg_ += msg;
				if (msg.back() != '.')
					msg_ += '.';
				ws->warn(msg_.c_str());
			}
		} else {
			_warnings.remove(msg);
		}
	}

	void createOverlay(Renderer* rnd) {
		_overlay.blank = Image::Ptr(Image::create());
		_overlay.blank->fromBlank(currentFrame()->width(), currentFrame()->height(), 0);
		_overlay.texture = Texture::Ptr(Texture::create());
		_overlay.texture->fromImage(rnd, Texture::STREAMING, _overlay.blank.get(), Texture::NEAREST);
		_overlay.texture->blend(Texture::BLEND);
	}
	void destroyOverlay(void) {
		_overlay.clear();
	}
	void clearOverlay(Renderer* rnd) {
		_overlay.texture->fromImage(rnd, Texture::STREAMING, _overlay.blank.get(), Texture::NEAREST);
		_overlay.texture->blend(Texture::BLEND);
	}

	template<typename T> T* enqueue(void) {
		T* result = _commands->enqueue<T>();

		_project->toPollEditor(true);

		entry()->increaseRevision();

		return result;
	}
	int simplify(void) {
		const int result = _commands->simplify();

		return result;
	}
	void refresh(Workspace* ws, const Command* cmd) {
		const bool refreshPalette =
			Command::is<Commands::Actor::Pencil>(cmd) ||
			Command::is<Commands::Actor::Line>(cmd) ||
			Command::is<Commands::Actor::Box>(cmd) ||
			Command::is<Commands::Actor::BoxFill>(cmd) ||
			Command::is<Commands::Actor::Ellipse>(cmd) ||
			Command::is<Commands::Actor::EllipseFill>(cmd) ||
			Command::is<Commands::Actor::Fill>(cmd) ||
			Command::is<Commands::Actor::Replace>(cmd) ||
			Command::is<Commands::Actor::Rotate>(cmd) ||
			Command::is<Commands::Actor::Flip>(cmd) ||
			Command::is<Commands::Actor::Cut>(cmd) ||
			Command::is<Commands::Actor::Paste>(cmd) ||
			Command::is<Commands::Actor::Delete>(cmd) ||
			Command::is<Commands::Actor::SetProperties>(cmd) ||
			Command::is<Commands::Actor::Resize>(cmd) ||
			Command::is<Commands::Actor::ImportFrame>(cmd);
		const bool refreshAllPalette =
			Command::is<Commands::Actor::Set8x16>(cmd) ||
			Command::is<Commands::Actor::SetPropertiesToAll>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd);
		const bool recalcSize =
			Command::is<Commands::Actor::Set8x16>(cmd) ||
			Command::is<Commands::Actor::Resize>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd) ||
			Command::is<Commands::Actor::ImportFrame>(cmd);
		const bool refillNames =
			Command::is<Commands::Actor::AddFrame>(cmd) ||
			Command::is<Commands::Actor::CutFrame>(cmd) ||
			Command::is<Commands::Actor::DeleteFrame>(cmd) ||
			Command::is<Commands::Actor::SetFigure>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd);
		const bool refillName =
			Command::is<Commands::Actor::SetName>(cmd);
		const bool refillDefinition =
			Command::is<Commands::Actor::SetDefinition>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd);
		const bool refillAnchor =
			Command::is<Commands::Actor::SetAnchor>(cmd) ||
			Command::is<Commands::Actor::SetAnchorForAll>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd);
		const bool invalidateRoutines =
			Command::is<Commands::Actor::SetRoutines>(cmd);
		const bool invalidateSlicesOfCurrentFrame =
			Command::is<Commands::Actor::Pencil>(cmd) ||
			Command::is<Commands::Actor::Line>(cmd) ||
			Command::is<Commands::Actor::Box>(cmd) ||
			Command::is<Commands::Actor::BoxFill>(cmd) ||
			Command::is<Commands::Actor::Ellipse>(cmd) ||
			Command::is<Commands::Actor::EllipseFill>(cmd) ||
			Command::is<Commands::Actor::Fill>(cmd) ||
			Command::is<Commands::Actor::Replace>(cmd) ||
			Command::is<Commands::Actor::Rotate>(cmd) ||
			Command::is<Commands::Actor::Flip>(cmd) ||
			Command::is<Commands::Actor::Cut>(cmd) ||
			Command::is<Commands::Actor::Paste>(cmd) ||
			Command::is<Commands::Actor::Delete>(cmd) ||
			Command::is<Commands::Actor::SetProperties>(cmd) ||
			Command::is<Commands::Actor::SetAnchor>(cmd);
		const bool invalidateSlices =
			Command::is<Commands::Actor::AddFrame>(cmd) ||
			Command::is<Commands::Actor::CutFrame>(cmd) ||
			Command::is<Commands::Actor::DeleteFrame>(cmd) ||
			Command::is<Commands::Actor::Set8x16>(cmd) ||
			Command::is<Commands::Actor::SetPropertiesToAll>(cmd) ||
			Command::is<Commands::Actor::Resize>(cmd) ||
			Command::is<Commands::Actor::SetAnchorForAll>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd) ||
			Command::is<Commands::Actor::ImportFrame>(cmd);
		const bool toCheck =
			Command::is<Commands::Actor::CutFrame>(cmd) ||
			Command::is<Commands::Actor::DeleteFrame>(cmd) ||
			Command::is<Commands::Actor::SetDefinition>(cmd) ||
			Command::is<Commands::Actor::Resize>(cmd) ||
			Command::is<Commands::Actor::Import>(cmd);

		if (refreshPalette) {
			if (_project->preferencesPreviewPaletteBits()) {
				const Commands::Layered::Layered* cmdlayered = Command::as<Commands::Layered::Layered>(cmd);
				if (cmdlayered) {
					const int frame = cmdlayered->sub();
					entry()->cleanup(frame);
				}
			};
		}
		if (refreshAllPalette) {
			entry()->cleanup();
		}
		if (recalcSize) {
			_estimated.filled = false;
			const Math::Vec2i tc = currentFrame()->count(object().get());
			_size.width = tc.x;
			_size.height = tc.y;
			_anchor.x = Math::min(_anchor.x, currentFrame()->width() - 1);
			_anchor.y = Math::min(_anchor.y, currentFrame()->height() - 1);
		}
		if (refillNames) {
			_frame.fill(object()->count(), entry()->figure);
		}
		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearActorPageNames();
		}
		if (refillDefinition) {
			_tools.definitionShadow = entry()->definition;
		}
		if (refillAnchor) {
			_anchor.x = currentFrame()->anchor.x;
			_anchor.y = currentFrame()->anchor.y;
		}
		if (invalidateRoutines) {
			_routine.clear();
		}
		if (invalidateSlicesOfCurrentFrame) {
			invalidateCurrentEntrySlices();
		}
		if (invalidateSlices) {
			invalidateAllEntriesSlices();
		}
		if (toCheck) {
			_checker();
		}
	}

	void flip(Editing::Tools::RotationsAndFlippings f) {
		if (!_binding.getPixel || !_binding.setPixel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		Command* cmd = nullptr;
		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			cmd = enqueue<Commands::Actor::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(1)
				->with(frame)
				->with(_setFrame, _frame.cursor)
				->exec(object(), Variant((void*)entry()));

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			cmd = enqueue<Commands::Actor::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(-1)
				->with(frame)
				->with(_setFrame, _frame.cursor)
				->exec(object(), Variant((void*)entry()));

			break;
		case Editing::Tools::HALF_TURN:
			cmd = enqueue<Commands::Actor::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(2)
				->with(frame)
				->with(_setFrame, _frame.cursor)
				->exec(object(), Variant((void*)entry()));

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			cmd = enqueue<Commands::Actor::Flip>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(1)
				->with(frame)
				->with(_setFrame, _frame.cursor)
				->exec(object(), Variant((void*)entry()));

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			cmd = enqueue<Commands::Actor::Flip>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(0)
				->with(frame)
				->with(_setFrame, _frame.cursor)
				->exec(object(), Variant((void*)entry()));

			break;
		default: // Do nothing.
			break;
		}

		if (cmd)
			_refresh(cmd);
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
		int idx = -1;
		currentFrame()->get(_cursor.position.x, _cursor.position.y, idx);
		_ref.palette = idx;
	}
	void eyedropperToolMove(Renderer*) {
		int idx = -1;
		currentFrame()->get(_cursor.position.x, _cursor.position.y, idx);
		_ref.palette = idx;
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		T* cmd = enqueue<T>();
		Commands::Paintable::Paint::Getter getter = std::bind(&EditorActorImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		Commands::Paintable::Paint::Setter setter = std::bind(&EditorActorImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		cmd->with(getter, setter);

		Colour col;
		Indexed::Ptr plt = entry()->palette;
		plt->get(_ref.palette, col);

		cmd->with(
			Math::Vec2i(currentFrame()->width(), currentFrame()->height()),
			_selection.brush.empty() ? nullptr : &sel,
			_cursor.position, _ref.palette
		);

		cmd->with(_setFrame, _frame.cursor);

		cmd->exec(object(), Variant((void*)entry()));

		_refresh(cmd);
	}
	void paintbucketToolUp(Renderer* rnd) {
		Math::Recti sel;
		if (_selection.area(sel) && !Math::intersects(sel, _cursor.position))
			_selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Actor::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Actor::Fill>(rnd, sel);
	}

	void lassoToolDown(Renderer*) {
		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);
		_selection.tilewise = ctrl.pressed(false);

		_selection.mouse = ImGui::GetMousePos();

		_selection.initial = _cursor.position;
		_selection.brush.position = _cursor.position;
		_selection.brush.size = Math::Vec2i(1, 1);

		if (_selection.tilewise) {
			_selection.initial.x = (int)std::floor((float)_selection.initial.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.initial.y = (int)std::floor((float)_selection.initial.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.position.x = (int)std::floor((float)_selection.brush.position.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.position.y = (int)std::floor((float)_selection.brush.position.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.size.x = (int)std::ceil((float)_selection.brush.size.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.size.y = (int)std::ceil((float)_selection.brush.size.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
		}
	}
	void lassoToolMove(Renderer*) {
		const Math::Vec2i diff = _cursor.position - _selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(_selection.initial.x, _selection.initial.y, diff.x, diff.y);
		_selection.brush.position = Math::Vec2i(rect.xMin(), rect.yMin());
		_selection.brush.size = Math::Vec2i(rect.width(), rect.height());

		if (_selection.tilewise) {
			_selection.brush.position.x = (int)std::floor((float)_selection.brush.position.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.position.y = (int)std::floor((float)_selection.brush.position.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.size.x = (int)std::ceil((float)_selection.brush.size.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			_selection.brush.size.y = (int)std::ceil((float)_selection.brush.size.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			if (_selection.brush.position.x < _selection.initial.x)
				_selection.brush.size.x = _selection.initial.x - _selection.brush.position.x + GBBASIC_TILE_SIZE;
			if (_selection.brush.position.y < _selection.initial.y)
				_selection.brush.size.y = _selection.initial.y - _selection.brush.position.y + GBBASIC_TILE_SIZE;
		}
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

	void stampToolUp_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots, Editing::Tools::PaintableTools prevTool) {
		if (!_binding.getPixel || !_binding.setPixel)
			return;

		_selection.clear();

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = _cursor.position.x + area.xMin() + xOff;
		const int y = _cursor.position.y + area.yMin() + yOff;

		Command* cmd = enqueue<Commands::Actor::Paste>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->with(_setFrame, _frame.cursor)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		destroyOverlay();

		_tools.painting = prevTool;
	}
	void stampToolHover_Paste(Renderer* rnd, const Math::Recti &area, const Editing::Dot::Array &dots) {
		bool reload = _painting.moved();

		if (_overlay.empty()) {
			createOverlay(rnd);

			reload = true;
		}

		if (reload) {
			clearOverlay(rnd);

			const int xOff = -area.width() / 2;
			const int yOff = -area.height() / 2;
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				const int y = _cursor.position.y + j + yOff;
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					const int x = _cursor.position.x + i + xOff;

					Colour col;
					Indexed::Ptr plt = entry()->palette;
					plt->get(dots[k].indexed, col);

					_overlay.texture->set(x, y, col);

					++k;
				}
			}
		}
	}

	template<typename T> void paintToolDown(Renderer* rnd) {
		_selection.clear();

		Command* cmd = enqueue<T>()
			->with(
				std::bind(&EditorActorImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorActorImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				_tools.weighting + 1
			)
			->with(_setFrame, _frame.cursor)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		createOverlay(rnd);

		paintToolMove<T>(rnd);
	}
	template<typename T, bool Clear = true> void paintToolMove(Renderer* rnd) {
		if (_commands->empty())
			return;

		if (Clear)
			clearOverlay(rnd);

		Command* back = _commands->back();
		GBBASIC_ASSERT(back->type() == T::TYPE());
		T* cmd = Command::as<T>(back);
		Colour col;
		Indexed::Ptr plt = entry()->palette;
		plt->get(_ref.palette, col);

		cmd->with(
			Math::Vec2i(currentFrame()->width(), currentFrame()->height()),
			_cursor.position, _ref.palette,
			[&] (int x, int y) -> void {
				Colour col_ = col;
				if (col_.a == 0) {
					if (((x % 2) + (y % 2)) % 2)
						col_ = Colour(121, 121, 121, 255);
					else
						col_ = Colour(221, 221, 221, 255);
				}
				_overlay.texture->set(x, y, col_);
			}
		);

		_debounce.modified();
	}
	void paintToolUp(Renderer*) {
		if (!_commands->empty()) {
			_commands->back()->redo(object());
		}

		_debounce.modified();

		destroyOverlay();
	}

	ActorAssets::Entry* entry(void) const {
		ActorAssets::Entry* entry = _project->getActor(_index);

		return entry;
	}
	Actor::Ptr &object(void) const {
		return entry()->data;
	}
	Actor::Frame* currentFrame(void) const {
		return frameAt(_frame.cursor);
	}
	Actor::Frame* frameAt(int idx) const {
		if (idx < 0 || idx >= object()->count())
			return nullptr;

		return object()->get(idx);
	}

	int getPixels(Renderer*, const Math::Recti* area /* nullable */, Editing::Dots &dots) const {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, currentFrame()->width(), currentFrame()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				int idx = -1;
				currentFrame()->get(i, j, idx);
				dots.indexed[k] = idx;
				++k;
				++result;
			}
		}

		return result;
	}
	int setPixels(Renderer*, const Math::Recti* area /* nullable */, const Editing::Dots &dots) {
		int result = 0;

		bool reload = currentFrame()->texture->usage() != Texture::STREAMING;
		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, currentFrame()->width(), currentFrame()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				const int idx = dots.indexed[k];
				currentFrame()->set(i, j, idx);

				if (currentFrame()->texture->usage() == Texture::STREAMING) {
					if (!currentFrame()->texture->set(i, j, idx))
						reload = true;
				}
				++k;
				++result;
			}
		}

		if (reload) {
			currentFrame()->texture = nullptr;
		}

		return result;
	}
	bool getPixel(Renderer*, const Math::Vec2i &pos, Editing::Dot &dot) const {
		return currentFrame()->get(pos.x, pos.y, dot.indexed);
	}
	bool setPixel(Renderer* rnd, const Math::Vec2i &pos, const Editing::Dot &dot) {
		if (!currentFrame()->set(pos.x, pos.y, dot.indexed))
			return false;

		if (currentFrame()->texture && currentFrame()->texture->usage() == Texture::STREAMING) {
			if (!currentFrame()->texture->set(pos.x, pos.y, dot.indexed))
				return false;
		} else {
			if (_project->preferencesPreviewPaletteBits()) {
				entry()->cleanup(_frame.cursor);
			} else {
				currentFrame()->texture = Texture::Ptr(Texture::create());
				if (!currentFrame()->texture->fromImage(rnd, Texture::STREAMING, currentFrame()->pixels.get(), Texture::NEAREST))
					return false;

				currentFrame()->texture->blend(Texture::BLEND);
			}

			_binding.skipFrame = true;
		}

		return true;
	}

	void frameAdded(Renderer* rnd, Workspace* ws, int frame, bool append) {
		if (object()->count() >= GBBASIC_ACTOR_FRAME_MAX_COUNT) {
			ws->messagePopupBox(ws->theme()->dialogPrompt_CannotAddMoreFrame(), nullptr, nullptr, nullptr);

			return;
		}

		const Editing::Shortcut shift(SDL_SCANCODE_UNKNOWN, false, true);

		Image::Ptr img(Image::create(entry()->palette));
		Image::Ptr props(Image::create(Indexed::Ptr(Indexed::create((int)Math::pow(2, ACTOR_PALETTE_DEPTH)))));
		if (shift.pressed()) {
			const Image::Ptr &src = currentFrame()->pixels;
			const Image::Ptr &props_ = currentFrame()->properties;
			img->fromImage(src.get());
			props->fromImage(props_.get());
		} else {
			Math::Vec2i sz(GBBASIC_ACTOR_DEFAULT_WIDTH * GBBASIC_TILE_SIZE, GBBASIC_ACTOR_DEFAULT_HEIGHT * GBBASIC_TILE_SIZE);
			if (object()->count() > 0) {
				Actor::Frame* f = nullptr;
				if (append)
					f = frameAt(object()->count() - 1);
				else
					frameAt(frame);
				if (f)
					sz = Math::Vec2i(f->width(), f->height());
			}
			img->fromBlank(sz.x, sz.y, GBBASIC_PALETTE_DEPTH);
			props->fromBlank(sz.x / GBBASIC_TILE_SIZE, sz.y / GBBASIC_TILE_SIZE, ACTOR_PALETTE_DEPTH);
		}
		Command* cmd = enqueue<Commands::Actor::AddFrame>()
			->with(append, img, props)
			->with(_setFrame, frame)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		if (append)
			_frame.cursor = object()->count() - 1;
		else
			_frame.cursor = frame;
		_frame.fill(object()->count(), entry()->figure);

		object()->touch(rnd, _frame.cursor);
	}
	void frameRemoved(Renderer*, Workspace*, int frame) {
		if (object()->count() == 0)
			return;

		Command* cmd = enqueue<Commands::Actor::DeleteFrame>()
			->with(false)
			->with(_setFrame, frame)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);

		_frame.fill(object()->count(), entry()->figure);
	}

	void refreshAllEntriesSlices(Workspace* ws, bool fast) {
		if (_compacted.filled)
			return;

		ImGui::WaitingPopupBox::TimeoutHandler timeout(
			[ws, this, fast] (void) -> void {
				compactAllEntriesSlices(ws, fast);

				_compacted.generated.wait();

				ws->popupBox(nullptr);
			},
			nullptr
		);
		ws->waitingPopupBox(
			true, ws->theme()->dialogPrompt_Compacting(),
			true, timeout,
			true
		);
	}
	void saveAllEntriesSlices(Workspace* ws) {
		if (!_compacted.image)
			return;

		pfd::save_file save(
			ws->theme()->generic_SaveTo(),
			entry()->name.empty() ? "gbbasic-actor.png" : Text::sanitizeFilename(entry()->name) + ".png",
			GBBASIC_IMAGE_FILE_FILTER
		);
		std::string path = save.result();
		Path::uniform(path);
		if (path.empty())
			return;
		const Text::Array exts = { "png", "jpg", "bmp", "tga" };
		std::string ext;
		Path::split(path, nullptr, &ext, nullptr);
		Text::toLowerCase(ext);
		if (ext.empty() || std::find(exts.begin(), exts.end(), ext) == exts.end())
			path += ".png";
		Path::split(path, nullptr, &ext, nullptr);
		Text::toLowerCase(ext);
		const char* y = ext.c_str();

		Image* ptr = nullptr;
		_compacted.image->clone(&ptr);
		Image::Ptr tmp(ptr);
		tmp->realize();
		Bytes::Ptr bytes(Bytes::create());
		tmp->toBytes(bytes.get(), y);

		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::WRITE))
			return;
		file->writeBytes(bytes.get());
		file->close();

#if !defined GBBASIC_OS_HTML
		FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
		std::string path_ = fileInfo->parentPath();
		path_ = Unicode::toOs(path_);
		Platform::browse(path_.c_str());
#endif /* Platform macro. */
	}
	bool invalidateCurrentEntrySlices(void) {
		if (!currentFrame())
			return false;

		currentFrame()->slices.clear(); // Invalidate the slices.

		_compacted.filled = false;

		_debounce.modified();

		return true;
	}
	bool invalidateAllEntriesSlices(void) {
		if (!entry())
			return false;

		for (int i = 0; i < object()->count(); ++i) {
			Actor::Frame* frame = object()->get(i);
			frame->slices.clear(); // Invalidate the slices.
		}
		entry()->animation.clear();
		entry()->shadow.clear();
		entry()->slices.clear();

		_compacted.filled = false;

		_debounce.modified();

		return true;
	}
	bool compactAllEntriesSlices(Workspace* ws, bool fast) {
		struct Data {
			Actor::Ptr actor = nullptr;
			Indexed::Ptr palette = nullptr;
			bool retained = false;
			Actor::Animation animation;
			Actor::Shadow::Array shadow;
			Actor::Slice::Array slices;
			bool fast = false;

			Data() {
			}
			Data(Actor::Ptr a, Indexed::Ptr p, bool fast_) : actor(a), palette(p), fast(fast_) {
			}
		};

		if (!entry())
			return false;

		if (_compacted.generated.working())
			_compacted.generated.wait();

		Actor* actor = nullptr;
		Indexed* palette = nullptr;
		if (!object()->clone(&actor, true) || !entry()->palette->clone(&palette)) {
			if (actor)
				Actor::destroy(actor);
			if (palette)
				Indexed::destroy(palette);

			return false;
		}

		Data* data = new Data(Actor::Ptr(actor), Indexed::Ptr(palette), fast);
		_compacted.generated = ws->async(
			std::bind(
				[] (WorkTask* /* task */, Data* data) -> uintptr_t { // On work thread.
					for (int i = 0; i < data->actor->count(); ++i) {
						const Actor::Frame* frame = data->actor->get(i);
						if (frame->slices.empty())
							data->actor->slice(i);
					}

					Image* img = Image::create(data->palette);
					data->actor->compact(data->animation, data->shadow, data->slices, img);

#if GBBASIC_MULTITHREAD_ENABLED
					if (!data->fast)
						DateTime::sleep(500); // Wait for 0.5s.
#endif /* GBBASIC_MULTITHREAD_ENABLED */

					return (uintptr_t)img;
				},
				std::placeholders::_1, data
			),
			std::bind(
				[this] (WorkTask* /* task */, uintptr_t ptr, Data* data) -> void { // On main thread.
					Image* img = (Image*)ptr;
					_compacted.image = Image::Ptr(img);
					_compacted.texture = Texture::Ptr(Texture::create());
					_compacted.texture->fromImage(_compacted.renderer, Texture::STATIC, _compacted.image.get(), Texture::NEAREST);
					_compacted.texture->blend(Texture::BLEND);
					data->retained = true;
				},
				std::placeholders::_1, std::placeholders::_2, data
			),
			std::bind(
				[this] (WorkTask* /* task */, uintptr_t ptr, Data* data) -> void { // On main thread.
					if (!data->retained) {
						Image* img = (Image*)ptr;
						Image::destroy(img);
					}

					entry()->animation = data->animation;
					entry()->shadow = data->shadow;
					entry()->slices = data->slices;

					_compacted.filled = true;

					_estimated.filled = false;

					delete data;
				},
				std::placeholders::_1, std::placeholders::_2, data
			)
		);

		return true;
	}

	void bindActorCallback(int index, const std::initializer_list<Variant> &args) {
		const Variant &arg = *args.begin();
		const std::string idx = (std::string)arg;
		const bool changed =
			(index == 0 && idx != object()->updateRoutine()) ||
			(index == 1 && idx != object()->onHitsRoutine());
		if (!changed)
			return;

		_routine.filled = false;
		Commands::Actor::SetRoutines* cmd = (Commands::Actor::SetRoutines*)enqueue<Commands::Actor::SetRoutines>()
			->with(
				index == 0 ? idx : object()->updateRoutine(),
				index == 1 ? idx : object()->onHitsRoutine()
			)
			->exec(object(), Variant((void*)entry()));

		_refresh(cmd);
	}

	bool playActorTesting(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		if (ws->running())
			return true;

		if (_tools.isPlaying)
			stopActorTesting(wnd, rnd, ws);

		if (!object())
			return false;

		// Start measuring performance.
		const long long start = DateTime::ticks();

		auto finish = [wnd, rnd, ws, this, start] (void) -> bool {
			// Compile.
			const Bytes::Ptr rom_ = compileActor(wnd, rnd, ws, this, entry(), _tools);

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

		// Async.
		ImGui::WaitingPopupBox::TimeoutHandler timeout(
			[ws, this, finish] (void) -> void {
				if (!_compacted.filled)
					compactAllEntriesSlices(ws, true);

				_compacted.generated.wait();

				finish();

				ws->popupBox(nullptr);
			},
			nullptr
		);
		ws->waitingPopupBox(
			true, ws->theme()->dialogPrompt_Compacting(),
			true, timeout,
			true
		);

		// Finish.
		return true;
	}
	bool stopActorTesting(Window* wnd, Renderer* rnd, Workspace* ws) {
		// Prepare.
		if (!_tools.isPlaying)
			return true;

		// Close the device.
		if (ws->running())
			ws->stop(wnd, rnd);

		// Stop playing.
		_tools.isPlaying = false;
#if EDITOR_ACTOR_UNLOAD_SYMBOLS_ON_FINISH
		_tools.isPlayerSymbolsLoaded = false;
		_tools.playerSymbolsText.clear();
		_tools.playerAliasesText.clear();
#endif /* EDITOR_ACTOR_UNLOAD_SYMBOLS_ON_FINISH */

		// Finish.
		return true;
	}
	static Bytes::Ptr compileActor(Window*, Renderer*, Workspace* ws, EditorActorImpl* self, const ActorAssets::Entry* entry_, Tools &tools) {
		// Prepare.
		if (!entry_ || !entry_->data || entry_->data->count() == 0)
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
			self->warn(ws, "No valid actor player.", true);

			return nullptr;
		}

		const GBBASIC::Kernel::Ptr &krnl = ws->kernels().front();
		if (!krnl) {
			self->warn(ws, "No valid kernel.", true);

			return nullptr;
		}

		std::string dir;
		Path::split(krnl->path(), nullptr, nullptr, &dir);
		const std::string rom = Path::combine(dir.c_str(), krnl->kernelRom().c_str());
		const std::string sym = Path::combine(dir.c_str(), krnl->kernelSymbols().c_str());
		const std::string aliases = Path::combine(dir.c_str(), krnl->kernelAliases().c_str());
		const int bootstrapBank = krnl->bootstrapBank();

		// Load and parse the symbols.
		if (!tools.isPlayerSymbolsLoaded) {
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

			tools.isPlayerSymbolsLoaded = true;
			tools.playerSymbolsText     = symTxt;
			tools.playerAliasesText     = aliasesTxt;
		}

		// Compile.
		print_("Begin compiling for actor testing.");

		AssetsBundle::Ptr assets(new AssetsBundle());
		do {
			// Prepare.
			const Project::Ptr &prj = ws->currentProject();
			GBBASIC_ASSERT(prj && "Impossible.");

			// Add the palette asset.
			assets->palette = prj->assets()->palette;

			// Add the actor asset.
			int tileCount = (int)entry_->slices.size();
			if (entry_->data->is8x16())
				tileCount *= 2;

			Actor* newActor = nullptr;
			entry_->data->clone(&newActor, false);

			if (!newActor->updateRoutine().empty())
				newActor->updateRoutine("");
			if (!newActor->onHitsRoutine().empty())
				newActor->onHitsRoutine("");

			ActorAssets::Entry actorEntry_ = *entry_;
			actorEntry_.data = Actor::Ptr(newActor);
			assets->actors.add(actorEntry_);

			// Add a dummy code asset.
			const int x = GBBASIC_SCREEN_WIDTH / 2;
			const int y = GBBASIC_SCREEN_HEIGHT / 2;
			const std::string src = Text::format(
				RES_CODE_PLAY_ACTOR_TESTING,
				{
					Text::toString(x), Text::toString(y),
					Text::toString(tileCount),
					Text::toString(true)
				}
			);
			assets->code.add(src);
		} while (false);

		const Bytes::Ptr rom_ = Workspace::compile(
			rom, sym, tools.playerSymbolsText, aliases, tools.playerAliasesText,
			"Actor", assets,
			bootstrapBank,
			print_, warn_, error_
		);

		print_("End compiling for actor testing.");

		if (!rom_)
			return nullptr;

		print_("Ok.");

		// Finish.
		return rom_;
	}
};

EditorActor* EditorActor::create(void) {
	EditorActorImpl* result = new EditorActorImpl();

	return result;
}

void EditorActor::destroy(EditorActor* ptr) {
	EditorActorImpl* impl = static_cast<EditorActorImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
