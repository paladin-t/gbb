/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_tiles.h"
#include "editor_tiles.h"
#include "theme.h"
#include "workspace.h"
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
** Tiles editor
*/

class EditorTilesImpl : public EditorTiles {
private:
	struct Pixels {
		Colour colored[EDITING_ITEM_COUNT_PER_LINE * 2];
	};

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
		std::string info;

		void clear(void) {
			text.clear();
			cursor = Editing::Brush();
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
	struct {
		int pitch = 0; // In tiles.
		int width = 0; // In pixels.
		int height = 0; // In pixels.

		void clear(void) {
			pitch = 0;
			width = 0;
			height = 0;
		}
	} _pitch;
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
		float real[4] = { 1, 1, 1, 1 };
		Pixels* latest = nullptr;

		void clear(void) {
			palette = 0;
			for (int i = 0; i < GBBASIC_COUNTOF(real); ++i)
				real[i] = 1.0f;
			if (latest) {
				delete latest;
				latest = nullptr;
			}
		}
	} _ref;
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

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);

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

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);

			post = nullptr;
			postType = Editing::Tools::PENCIL;
		}
	} _tools;
	Processor _processors[Editing::Tools::COUNT] = {
		Processor( // Hand.
			std::bind(&EditorTilesImpl::handToolDown, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::handToolMove, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::handToolUp, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::handToolHover, this, std::placeholders::_1)
		),
		Processor( // Eyedropper.
			std::bind(&EditorTilesImpl::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		),
		Processor( // Pencil.
			std::bind(&EditorTilesImpl::paintToolDown<Commands::Tiles::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolMove<Commands::Tiles::Pencil, false>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorTilesImpl::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Lasso.
			std::bind(&EditorTilesImpl::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::lassoToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Line.
			std::bind(&EditorTilesImpl::paintToolDown<Commands::Tiles::Line>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolMove<Commands::Tiles::Line>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box.
			std::bind(&EditorTilesImpl::paintToolDown<Commands::Tiles::Box>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolMove<Commands::Tiles::Box>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box fill.
			std::bind(&EditorTilesImpl::paintToolDown<Commands::Tiles::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolMove<Commands::Tiles::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse.
			std::bind(&EditorTilesImpl::paintToolDown<Commands::Tiles::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolMove<Commands::Tiles::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse fill.
			std::bind(&EditorTilesImpl::paintToolDown<Commands::Tiles::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolMove<Commands::Tiles::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorTilesImpl::paintToolUp, this, std::placeholders::_1),
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
	EditorTilesImpl() {
		_commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Tiles::Pencil>()
			->reg<Commands::Tiles::Line>()
			->reg<Commands::Tiles::Box>()
			->reg<Commands::Tiles::BoxFill>()
			->reg<Commands::Tiles::Ellipse>()
			->reg<Commands::Tiles::EllipseFill>()
			->reg<Commands::Tiles::Fill>()
			->reg<Commands::Tiles::Replace>()
			->reg<Commands::Tiles::Rotate>()
			->reg<Commands::Tiles::Flip>()
			->reg<Commands::Tiles::Cut>()
			->reg<Commands::Tiles::Paste>()
			->reg<Commands::Tiles::Delete>()
			->reg<Commands::Tiles::SetName>()
			->reg<Commands::Tiles::Resize>()
			->reg<Commands::Tiles::Pitch>()
			->reg<Commands::Tiles::Import>();
	}
	virtual ~EditorTilesImpl() override {
		close(_index);

		delete _commands;
		_commands = nullptr;
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(class Window*, class Renderer*, class Workspace* ws, class Project* project, int index, unsigned /* refCategory */, int /* refIndex */) override {
		if (_opened)
			return;
		_opened = true;

		_project = project;
		_index = index;

		_refresh = std::bind(&EditorTilesImpl::refresh, this, ws, std::placeholders::_1);

		_size.width = object()->width() / GBBASIC_TILE_SIZE;
		_size.height = object()->height() / GBBASIC_TILE_SIZE;
		_pitch.width = object()->width();
		_pitch.height = object()->height();
		_pitch.pitch = object()->width() / GBBASIC_TILE_SIZE;
		if (object()->paletted()) {
			entry()->refresh(); // Refresh the outdated editable resources.
		}

		const Editing::Shortcut caps(SDL_SCANCODE_UNKNOWN, false, false, false, false, true);
		const Editing::Shortcut num(SDL_SCANCODE_UNKNOWN, false, false, false, true, false);
		_tools.magnification = entry()->magnification;
		_tools.namableText = entry()->name;
		_tools.showGrids = _project->preferencesShowGrids();
		_tools.gridsVisible = !caps.pressed();
		_tools.gridUnit = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
		_tools.transparentBackbroundVisible = num.pressed();

		fprintf(stdout, "Tiles editor opened: #%d.\n", _index);
	}
	virtual void close(int /* index */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Tiles editor closed: #%d.\n", _index);

		_project = nullptr;
		_index = -1;

		_selection.clear();
		_page.clear();
		_status.clear();
		_statusWidth = 0.0f;
		_warnings.clear();
		_refresh = nullptr;
		_estimated.clear();
		_painting.clear();
		_size.clear();
		_pitch.clear();
		_binding.clear();
		_overlay.clear();

		_ref.clear();
		_tools.clear();
	}

	virtual int index(void) const override {
		return _index;
	}

	virtual void enter(class Workspace*) override {
		// Do nothing.
	}
	virtual void leave(class Workspace*) override {
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
					if (obj->paletted())
						Jpath::set(doc, doc, dots.indexed[k], "data", k);
					else
						Jpath::set(doc, doc, dots.colored[k].toRGBA(), "data", k);

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
			size = object()->width() * object()->height();
		else
			selPtr = &sel;

		if (object()->paletted()) {
			typedef std::vector<int> Data;

			Data vec(size, 0);
			Editing::Dots dots(&vec.front());
			_binding.getPixels(selPtr, dots);

			const std::string buf = toString(object(), sel, dots);
			const std::string osstr = Unicode::toOs(buf);

			Platform::setClipboardText(osstr.c_str());
		} else {
			typedef std::vector<Colour> Data;

			Data vec(size, Colour());
			Editing::Dots dots(&vec.front());
			_binding.getPixels(selPtr, dots);

			const std::string buf = toString(object(), sel, dots);
			const std::string osstr = Unicode::toOs(buf);

			Platform::setClipboardText(osstr.c_str());
		}

		if (clearSelection)
			_selection.clear();
	}
	virtual void cut(void) override {
		if (_tools.focused)
			return;

		if (!_binding.getPixel || !_binding.setPixel)
			return;

		copy(false);

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		enqueue<Commands::Tiles::Cut>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(sel)
			->exec(object());

		_selection.clear();
	}
	virtual bool pastable(void) const override {
		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
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
		if (!fromString(object(), buf, area, dots))
			return;

		const Editing::Tools::PaintableTools prevTool = _tools.painting;
		if (prevTool != Editing::Tools::STAMP) {
			_tools.painting = Editing::Tools::STAMP;

			_processors[Editing::Tools::STAMP] = Processor{
				nullptr,
				nullptr,
				std::bind(&EditorTilesImpl::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
				std::bind(&EditorTilesImpl::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
			};
		}

		destroyOverlay();
	}
	virtual void del(bool) override {
		if (_tools.focused)
			return;

		if (!_binding.getPixel || !_binding.setPixel)
			return;

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		enqueue<Commands::Tiles::Delete>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(sel)
			->exec(object());

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

		const int width = object()->width();
		const int height = object()->height();
		_commands->redo(object(), Variant((void*)entry()), &texture());

		if (width != object()->width() || height != object()->height())
			_selection.clear();

		_refresh(cmd);

		_project->toPollEditor(true);

		entry()->increaseRevision();
	}
	virtual void undo(BaseAssets::Entry*) override {
		const Command* cmd = _commands->undoable();
		if (!cmd)
			return;

		const int width = object()->width();
		const int height = object()->height();
		_commands->undo(object(), Variant((void*)entry()), &texture());

		if (width != object()->width() || height != object()->height())
			_selection.clear();

		_refresh(cmd);

		_project->toPollEditor(true);

		entry()->increaseRevision();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case RESIZE: {
				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				if (width == object()->width() && height == object()->height())
					return Variant(true);

				Command* cmd = enqueue<Commands::Tiles::Resize>()
					->with(Math::Vec2i(width, height))
					->exec(object(), Variant((void*)entry()), &texture());

				_refresh(cmd);

				_selection.clear();
			}

			return Variant(true);
		case PITCH: {
				const Variant::Int width = unpack<Variant::Int>(argc, argv, 0, 0);
				const Variant::Int height = unpack<Variant::Int>(argc, argv, 1, 0);
				const Variant::Int pitch = unpack<Variant::Int>(argc, argv, 2, 0);
				if (pitch == object()->width())
					return Variant(true);

				const int maxHeight = (int)std::floor((float)GBBASIC_TILES_MAX_AREA_SIZE / _pitch.pitch) * GBBASIC_TILE_SIZE;
				Command* cmd = enqueue<Commands::Tiles::Pitch>()
					->with(width, height, pitch, maxHeight)
					->exec(object(), Variant((void*)entry()), &texture());

				_refresh(cmd);

				_selection.clear();
			}

			return Variant(true);
		case SELECT_ALL:
			_selection.brush.position = Math::Vec2i(0, 0);
			_selection.brush.size = Math::Vec2i(object()->width(), object()->height());

			return Variant(true);
		case UPDATE_INDEX: {
				const Variant::Int index = unpack<Variant::Int>(argc, argv, 0, _index);

				_index = (int)index;

				statusInvalidated();
			}

			return Variant(true);
		case REDO: {
				const unsigned id = (unsigned)unpack<Variant::Long>(argc, argv, 0, 0);

				int n = 0;
				const Command* cmd = _commands->seek(id, 1, &n);
				if (!cmd)
					return Variant(false);

				if (cmd->id() != id)
					return Variant(false);

				for (int i = 0; i <= n; ++i)
					redo(entry());
			}

			return Variant(true);
		case UNDO: {
				const unsigned id = (unsigned)unpack<Variant::Long>(argc, argv, 0, 0);

				int n = 0;
				const Command* cmd = _commands->seek(id, -1, &n);
				if (!cmd)
					return Variant(false);

				if (cmd->id() != id)
					return Variant(false);

				for (int i = 0; i <= n; ++i)
					undo(entry());
			}

			return Variant(true);
		case IMPORT: {
				const void* ptr = unpack<void*>(argc, argv, 0, nullptr);
				const TilesAssets::Entry* tiles = (const TilesAssets::Entry*)ptr;

				const Image::Ptr &newObj = tiles->data;

				Command* cmd = enqueue<Commands::Tiles::Import>()
					->with(newObj)
					->exec(object(), Variant((void*)entry()), &texture());

				_refresh(cmd);

				unsigned id = 0;
				_commands->current(&id);

				return Variant((Variant::Long)id);
			}
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
				std::bind(&EditorTilesImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorTilesImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorTilesImpl::getPixels, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorTilesImpl::setPixels, this, rnd, std::placeholders::_1, std::placeholders::_2)
			);
		}

		shortcuts(wnd, rnd, ws);

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
		if (object()->paletted()) {
			Indexed::Ptr ref = object()->palette();
			if (!ref)
				return;

			entry()->refresh(); // Refresh the outdated editable resources.
		}

		ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav;
		if (_tools.scaled) {
			flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		}
		ImGui::BeginChild("@Pat", ImVec2(splitter.first, height - statusBarHeight), false, flags);
		{
			entry()->touch(); // Ensure the runtime resources are ready.

			if (texture()) {
				Editing::Brush cursor = _cursor;
				const bool withCursor = _tools.painting != Editing::Tools::HAND;

				constexpr const int MAGNIFICATIONS[] = {
					1, 2, 4, 8
				};
				if (_tools.magnification == -1) {
					const float PADDING = 32.0f;
					const float s = (splitter.first + PADDING) / object()->width();
					const float t = ((height - statusBarHeight) + PADDING) / object()->height();
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
				float width_ = (float)object()->width();
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

				_painting.set(
					Editing::tiles(
						rnd, ws,
						object().get(), texture().get(),
						width_,
						withCursor ? &cursor : nullptr, false,
						_selection.brush.empty() ? nullptr : &_selection.brush,
						_overlay.texture ? _overlay.texture.get() : nullptr,
						&_tools.gridUnit, _tools.showGrids && _tools.gridsVisible,
						_tools.transparentBackbroundVisible,
						nullptr
					)
				);
				if (
					_painting ||
					_tools.painting == Editing::Tools::STAMP
				) {
					_cursor = cursor;
				}
				refreshStatus(wnd, rnd, ws, &cursor);
			} else {
				_painting.set(false);
			}

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

			if (_binding.skipFrame) {
				_binding.skipFrame = false;
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
			float xOffset = 0;
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

			const float spwidth = _ref.windowWidth(splitter.second);
			if (object()->paletted()) {
				Indexed::Ptr palette = object()->palette();

				//Editing::palette(rnd, ws, palette.get(), spwidth, nullptr, &_ref.palette, true, ws->theme()->iconTransparent());

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
					ws->theme()->tooltip_OpenRefPalette().c_str(),
					[this, &colors] (int index) -> void { // Color has been changed directly from this tiles editor.
						// Get the old color.
						Colour old;
						Indexed::Ptr palette = object()->palette();
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

						// Synchronize the super palettes.
						_project->synchronizeSuperPalettes(assets);
					},
					[rnd, ws, this] (void) -> void { // Open the palette editor.
						ws->showPaletteEditor(
							rnd,
							entry()->ref,
							[rnd, ws, this] (const std::initializer_list<Variant> &args) -> void { // Color or group has been changed.
								// Prepare.
								const Variant &arg = *args.begin();
								const int group = (int)arg;

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
									Indexed::Ptr palette_ = object()->palette();
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
			} else {
				ImGui::SetNextItemWidth(spwidth);
				ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview;
				if (spwidth < ImGui::ColorPickerMinWidthForInput()) {
					flags |= ImGuiColorEditFlags_NoInputs;
					ImGui::ColorPicker4("", _ref.real, flags);

					ImGui::PushID("@Alf");
					ImGui::SetNextItemWidth(spwidth);
					int alpha = (int)(_ref.real[3] * 255);
					if (ImGui::DragInt("", &alpha, 1, 0, 255, "A: %d"))
						_ref.real[3] = alpha / 255.0f;
					ImGui::PopID();
				} else {
					ImGui::ColorPicker4("", _ref.real, flags);
				}
			}

			if (!_ref.latest) {
				if (!object()->paletted()) {
					_ref.latest = new Pixels();
					const Colour white(255, 255, 255, 255);
					_ref.latest->colored[0] = Colour(255, 0, 0, 255);
					_ref.latest->colored[1] = Colour(0, 255, 0, 255);
					_ref.latest->colored[2] = Colour(255, 255, 0, 255);
					_ref.latest->colored[3] = Colour(0, 0, 255, 255);
					_ref.latest->colored[4] = Colour(255, 0, 255, 255);
					_ref.latest->colored[5] = Colour(0, 255, 255, 255);
					_ref.latest->colored[6] = Colour(0, 0, 0, 255);
					_ref.latest->colored[7] = Colour(255, 255, 255, 255);
					_ref.latest->colored[8] = Colour(128, 128, 128, 255);
					_ref.latest->colored[9] = Colour(255, 255, 255, 0);
				}
			}

			if (_ref.latest) {
				Editing::Tools::separate(rnd, ws, spwidth);
				int coloring = -1;
				const bool changed = Editing::Tools::colorable(rnd, ws, _ref.latest->colored, &coloring, spwidth, canUseShortcuts());
				if (changed) {
					const Colour col = _ref.latest->colored[coloring];
					_ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
					_ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
					_ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
					_ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
				}
			}

			const unsigned mask = 0xffffffff & ~(1 << Editing::Tools::STAMP);
			Editing::Tools::separate(rnd, ws, spwidth);
			if (Editing::Tools::paintable(rnd, ws, &_tools.painting, spwidth, canUseShortcuts(), true, mask)) {
				destroyOverlay();
			}

			Editing::Tools::separate(rnd, ws, spwidth);
			Editing::Tools::magnifiable(rnd, ws, &_tools.magnification, spwidth, canUseShortcuts());

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
				if (_project->canRenameTiles(_index, _tools.namableText, nullptr)) {
					Command* cmd = enqueue<Commands::Tiles::SetName>()
						->with(_tools.namableText)
						->exec(object(), Variant((void*)entry()));

					_refresh(cmd);
				} else {
					_tools.namableText = entry()->name;

					warn(ws, ws->theme()->warning_TilesNameIsAlreadyInUse(), true);
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			const int width_ = object()->width() / GBBASIC_TILE_SIZE;
			const int height_ = object()->height() / GBBASIC_TILE_SIZE;
			bool sizeChanged = false;
			if (
				Editing::Tools::resizable(
					rnd, ws,
					width_, height_,
					&_size.width, &_size.height,
					GBBASIC_TILES_MAX_AREA_SIZE, GBBASIC_TILES_MAX_AREA_SIZE, GBBASIC_TILES_MAX_AREA_SIZE,
					spwidth,
					&inputFieldFocused_,
					ws->theme()->dialogPrompt_Size_InTiles().c_str(),
					ws->theme()->warning_TilesSizeOutOfBounds().c_str(),
					std::bind(&EditorTilesImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				)
			) {
				_size.width = Math::clamp(_size.width, 1, GBBASIC_TILES_MAX_AREA_SIZE);
				_size.height = Math::clamp(_size.height, 1, GBBASIC_TILES_MAX_AREA_SIZE);
				if (_size.width != width_ || _size.height != height_) {
					post(RESIZE, _size.width * GBBASIC_TILE_SIZE, _size.height * GBBASIC_TILE_SIZE);
					sizeChanged = true;

					entry()->increaseRevision();
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			const int pitch = object()->width() / GBBASIC_TILE_SIZE;
			if (
				Editing::Tools::pitchable(
					rnd, ws,
					_pitch.width, _pitch.height, sizeChanged,
					pitch,
					&_pitch.pitch,
					GBBASIC_TILES_MAX_AREA_SIZE,
					spwidth,
					&inputFieldFocused_,
					ws->theme()->dialogPrompt_Pitch_InTiles().c_str(),
					ws->theme()->warning_TilesPitchOutOfBounds().c_str(),
					std::bind(&EditorTilesImpl::warn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				)
			) {
				_pitch.pitch = Math::clamp(_pitch.pitch, 1, GBBASIC_TILES_MAX_AREA_SIZE);
				if (_pitch.pitch != pitch) {
					post(PITCH, _pitch.width, _pitch.height, _pitch.pitch * GBBASIC_TILE_SIZE);

					entry()->increaseRevision();
				}
			}
			inputFieldFocused |= inputFieldFocused_;

			Editing::Tools::separate(rnd, ws, spwidth);
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
			if (ImGui::Button(ws->theme()->windowTiles_CreateMap(), ImVec2(mwidth, 0))) {
				ws->addMapPageFrom(wnd, rnd, _index);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltipTiles_CreateAMapReferencingThisTilesPage());
			}

			_tools.inputFieldFocused = inputFieldFocused;

			_tools.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows); // Ignore shortcuts when the window is not focused.
			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		refreshStatus(wnd, rnd, ws, nullptr);
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
		// Do nothing.
	}
	virtual void stopped(class Renderer*, class Workspace*) override {
		// Do nothing.
	}

	virtual void resized(class Renderer*, const Math::Vec2i &, const Math::Vec2i &) override {
		_ref.windowResized();
	}

	virtual void focusLost(class Renderer*) override {
		entry()->cleanup(); // Clean up the outdated editable and runtime resources.
	}
	virtual void focusGained(class Renderer*) override {
		// Do nothing.
	}

private:
	void shortcuts(Window* wnd, Renderer* rnd, Workspace* ws) {
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

			return;
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

			return;
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

			return;
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
					index = _project->tilesPageCount() - 1;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::TILES, index);
			} else if (pgDown.pressed()) {
				int index = _index + 1;
				if (index >= _project->tilesPageCount())
					index = 0;
				if (index != _index)
					ws->changePage(wnd, rnd, _project, Workspace::Categories::TILES, index);
			}
		}
	}

	void context(Window*, Renderer*, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Ctx");

			ws->bubble(nullptr);
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Ctx")) {
			Math::Recti sel;
			const int size = _selection.area(sel);

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
			if (ImGui::MenuItem(ws->theme()->menu_ExportSelection(), nullptr, nullptr, !!size)) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-tiles.png" : entry()->name + ".png",
						GBBASIC_IMAGE_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					const Text::Array exts = { "png", "jpg", "bmp", "tga" };
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || std::find(exts.begin(), exts.end(), ext) == exts.end())
						path += ".png";
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					const char* y = ext.c_str();

					Bytes::Ptr bytes(Bytes::create());
					if (!entry()->serializeImage(bytes.get(), y, &sel))
						break;

					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeBytes(bytes.get());
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
				_status.text += Text::toString(object()->width());
				_status.text += "x";
				_status.text += Text::toString(object()->height());

				_status.text += " " + ws->theme()->status_Tls();
				_status.text += " ";
				_status.text += Text::toString(object()->width() / GBBASIC_TILE_SIZE);
				_status.text += "x";
				_status.text += Text::toString(object()->height() / GBBASIC_TILE_SIZE);
			} else {
				_status.text = " " + ws->theme()->status_Pos();
				_status.text += " ";
				_status.text += Text::toString(_status.cursor.position.x);
				_status.text += ",";
				_status.text += Text::toString(_status.cursor.position.y);

				const int x = _status.cursor.position.x / GBBASIC_TILE_SIZE;
				const int y = _status.cursor.position.y / GBBASIC_TILE_SIZE;
				_status.text += " " + ws->theme()->status_Idx();
				_status.text += " ";
				_status.text += Text::toString(x + y * object()->width() / GBBASIC_TILE_SIZE);
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
				index = _project->tilesPageCount() - 1;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::TILES, index);
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
			if (index >= _project->tilesPageCount())
				index = 0;
			if (index != _index)
				ws->changePage(wnd, rnd, _project, Workspace::Categories::TILES, index);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEdit_NextPage());
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
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
					_status.info = Text::format(
						ws->theme()->tooltipTiles_Info(),
						{
							Text::toString(w * h)
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
			const Text::Array &assetNames = ws->getTilesPageNames();
			const int n = _project->tilesPageCount();
			for (int i = 0; i < n; ++i) {
				const std::string &pg = assetNames[i];
				if (i == _index) {
					ImGui::MenuItem(pg, nullptr, true);
				} else {
					if (ImGui::MenuItem(pg))
						ws->changePage(wnd, rnd, _project, Workspace::Categories::TILES, i);
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
						if (!entry()->parseDataSequence(newObj, txt, false)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						Command* cmd = enqueue<Commands::Tiles::Import>()
							->with(newObj)
							->exec(object(), Variant((void*)entry()), &texture());

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
						if (!entry()->parseDataSequence(newObj, txt, true)) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						Command* cmd = enqueue<Commands::Tiles::Import>()
							->with(newObj)
							->exec(object(), Variant((void*)entry()), &texture());

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
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
					if (!Platform::hasClipboardText()) {
						ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

						break;
					}

					const std::string osstr = Platform::getClipboardText();
					const std::string txt = Unicode::fromOs(osstr);
					Image::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, txt, status)) {
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

					Command* cmd = enqueue<Commands::Tiles::Import>()
						->with(newObj)
						->exec(object(), Variant((void*)entry()), &texture());

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

					Image::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseJson(newObj, txt, status)) {
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

					Command* cmd = enqueue<Commands::Tiles::Import>()
						->with(newObj)
						->exec(object(), Variant((void*)entry()), &texture());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}
			if (Platform::isClipboardImageSupported()) {
				if (ImGui::MenuItem(ws->theme()->menu_Image())) {
					do {
						if (!Platform::hasClipboardImage()) {
							ws->bubble(ws->theme()->dialogPrompt_NoData(), nullptr);

							break;
						}

						Image::Ptr img(Image::create());
						if (!Platform::getClipboardImage(img.get())) {
							ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

							break;
						}

						Image::Ptr newObj = nullptr;
						BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
						if (!entry()->parseImage(newObj, img.get(), status)) {
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

						Command* cmd = enqueue<Commands::Tiles::Import>()
							->with(newObj)
							->exec(object(), Variant((void*)entry()), &texture());

						_refresh(cmd);

						ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
				}
			}
			if (ImGui::MenuItem(ws->theme()->menu_ImageFile())) {
				do {
					pfd::open_file open(
						GBBASIC_TITLE,
						"",
						GBBASIC_IMAGE_FILE_FILTER,
						pfd::opt::none
					);
					if (open.result().empty())
						break;
					std::string path = open.result().front();
					Path::uniform(path);
					if (path.empty())
						break;

					Bytes::Ptr bytes(Bytes::create());
					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::READ)) {
						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					if (!file->readBytes(bytes.get())) {
						file->close(); FileMonitor::unuse(path);

						ws->bubble(ws->theme()->dialogPrompt_InvalidData(), nullptr);

						break;
					}
					file->close(); FileMonitor::unuse(path);

					Image::Ptr newObj = nullptr;
					BaseAssets::Entry::ParsingStatuses status = BaseAssets::Entry::ParsingStatuses::SUCCESS;
					if (!entry()->parseImage(newObj, bytes.get(), status)) {
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

					Command* cmd = enqueue<Commands::Tiles::Import>()
						->with(newObj)
						->exec(object(), Variant((void*)entry()), &texture());

					_refresh(cmd);

					ws->bubble(ws->theme()->dialogPrompt_ImportedAsset(), nullptr);
				} while (false);
			}

			ImGui::EndPopup();
		}

		// Exporting.
		if (ImGui::BeginPopup("@Xpt")) {
			if (ImGui::BeginMenu(ws->theme()->menu_Code())) {
				if (ImGui::MenuItem(ws->theme()->menu_ForTiles())) {
					do {
						std::string txt;
						if (!entry()->serializeBasic(txt, _index, true))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_ForSprite())) {
					do {
						std::string txt;
						if (!entry()->serializeBasic(txt, _index, false))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedCode(), nullptr);
					} while (false);
				}

				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
			}
			if (ImGui::BeginMenu(ws->theme()->menu_DataSequence())) {
				if (ImGui::MenuItem(ws->theme()->menu_Hex())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 16))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Dec())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 10))
							break;

						Platform::setClipboardText(txt.c_str());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::MenuItem(ws->theme()->menu_Bin())) {
					do {
						std::string txt;
						if (!entry()->serializeDataSequence(txt, 2))
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
						entry()->name.empty() ? "gbbasic-tiles.json" : entry()->name + ".json",
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
					do {
						Image::Ptr img(Image::create());
						if (!entry()->serializeImage(img.get(), nullptr))
							break;

						Platform::setClipboardImage(img.get());

						ws->bubble(ws->theme()->dialogPrompt_ExportedAsset(), nullptr);
					} while (false);
				}
				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltip_ViaClipboard());
				}
			}
			if (ImGui::MenuItem(ws->theme()->menu_ImageFile())) {
				do {
					pfd::save_file save(
						ws->theme()->generic_SaveTo(),
						entry()->name.empty() ? "gbbasic-tiles.png" : entry()->name + ".png",
						GBBASIC_IMAGE_FILE_FILTER
					);
					std::string path = save.result();
					Path::uniform(path);
					if (path.empty())
						break;
					const Text::Array exts = { "png", "jpg", "bmp", "tga" };
					std::string ext;
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					if (ext.empty() || std::find(exts.begin(), exts.end(), ext) == exts.end())
						path += ".png";
					Path::split(path, nullptr, &ext, nullptr);
					Text::toLowerCase(ext);
					const char* y = ext.c_str();

					Bytes::Ptr bytes(Bytes::create());
					if (!entry()->serializeImage(bytes.get(), y, nullptr))
						break;

					File::Ptr file(File::create());
					if (!file->open(path.c_str(), Stream::WRITE))
						break;
					file->writeBytes(bytes.get());
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

	void warn(Workspace* ws, const std::string &msg, bool add) {
		if (add) {
			if (_warnings.add(msg)) {
				std::string msg_ = "Tiles editor: ";
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
		_overlay.blank->fromBlank(object()->width(), object()->height(), 0);
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
	void refresh(Workspace* ws, const Command* cmd) {
		const bool refillName =
			Command::is<Commands::Tiles::SetName>(cmd);
		const bool recalcSize =
			Command::is<Commands::Tiles::Resize>(cmd) ||
			Command::is<Commands::Tiles::Pitch>(cmd) ||
			Command::is<Commands::Tiles::Import>(cmd);

		if (refillName) {
			_tools.namableText = entry()->name;
			ws->clearTilesPageNames();
		}
		if (recalcSize) {
			_size.width = object()->width() / GBBASIC_TILE_SIZE;
			_size.height = object()->height() / GBBASIC_TILE_SIZE;
			_pitch.width = object()->width();
			_pitch.height = object()->height();
			_pitch.pitch = object()->width() / GBBASIC_TILE_SIZE;
			_status.info.clear();
			_estimated.filled = false;
		}
	}

	void flip(Editing::Tools::RotationsAndFlippings f) {
		if (!_binding.getPixel || !_binding.setPixel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			enqueue<Commands::Tiles::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(1)
				->with(frame)
				->exec(object());

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			enqueue<Commands::Tiles::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(-1)
				->with(frame)
				->exec(object());

			break;
		case Editing::Tools::HALF_TURN:
			enqueue<Commands::Tiles::Rotate>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(2)
				->with(frame)
				->exec(object());

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			enqueue<Commands::Tiles::Flip>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(1)
				->with(frame)
				->exec(object());

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			enqueue<Commands::Tiles::Flip>()
				->with(_binding.getPixel, _binding.setPixel)
				->with(0)
				->with(frame)
				->exec(object());

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
		if (object()->paletted()) {
			int idx = -1;
			object()->get(_cursor.position.x, _cursor.position.y, idx);
			_ref.palette = idx;
		} else {
			Colour col;
			object()->get(_cursor.position.x, _cursor.position.y, col);
			_ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			_ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			_ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			_ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
		}
	}
	void eyedropperToolMove(Renderer*) {
		if (object()->paletted()) {
			int idx = -1;
			object()->get(_cursor.position.x, _cursor.position.y, idx);
			_ref.palette = idx;
		} else {
			Colour col;
			object()->get(_cursor.position.x, _cursor.position.y, col);
			_ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			_ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			_ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			_ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
		}
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		T* cmd = enqueue<T>();
		Commands::Paintable::Paint::Getter getter = std::bind(&EditorTilesImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		Commands::Paintable::Paint::Setter setter = std::bind(&EditorTilesImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		cmd->with(getter, setter);

		if (object()->paletted()) {
			Colour col;
			Indexed::Ptr plt = object()->palette();
			plt->get(_ref.palette, col);

			cmd->with(
				Math::Vec2i(object()->width(), object()->height()),
				_selection.brush.empty() ? nullptr : &sel,
				_cursor.position, _ref.palette
			);
		} else {
			const Colour col(
				(Byte)(_ref.real[0] * 255),
				(Byte)(_ref.real[1] * 255),
				(Byte)(_ref.real[2] * 255),
				(Byte)(_ref.real[3] * 255)
			);

			cmd->with(
				Math::Vec2i(object()->width(), object()->height()),
				_selection.brush.empty() ? nullptr : &sel,
				_cursor.position, col
			);
		}

		cmd->exec(object());
	}
	void paintbucketToolUp(Renderer* rnd) {
		Math::Recti sel;
		if (_selection.area(sel) && !Math::intersects(sel, _cursor.position))
			_selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Tiles::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Tiles::Fill>(rnd, sel);
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

		enqueue<Commands::Tiles::Paste>()
			->with(_binding.getPixel, _binding.setPixel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->exec(object());

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

					if (object()->paletted()) {
						Colour col;
						Indexed::Ptr plt = object()->palette();
						plt->get(dots[k].indexed, col);

						_overlay.texture->set(x, y, col);
					} else {
						const Colour &col = dots[k].colored;

						_overlay.texture->set(x, y, col);
					}

					++k;
				}
			}
		}
	}

	template<typename T> void paintToolDown(Renderer* rnd) {
		_selection.clear();

		enqueue<T>()
			->with(
				std::bind(&EditorTilesImpl::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorTilesImpl::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				_tools.weighting + 1
			)
			->exec(object());

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
		if (object()->paletted()) {
			Colour col;
			Indexed::Ptr plt = object()->palette();
			plt->get(_ref.palette, col);

			cmd->with(
				Math::Vec2i(object()->width(), object()->height()),
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
		} else {
			const Colour col(
				(Byte)(_ref.real[0] * 255),
				(Byte)(_ref.real[1] * 255),
				(Byte)(_ref.real[2] * 255),
				(Byte)(_ref.real[3] * 255)
			);

			cmd->with(
				Math::Vec2i(object()->width(), object()->height()),
				_cursor.position, col,
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
		}
	}
	void paintToolUp(Renderer*) {
		if (!_commands->empty()) {
			_commands->back()->redo(object(), &texture());
		}

		destroyOverlay();
	}

	TilesAssets::Entry* entry(void) const {
		TilesAssets::Entry* entry = _project->getTiles(_index);

		return entry;
	}
	Image::Ptr &object(void) const {
		return entry()->data;
	}
	Texture::Ptr &texture(void) const {
		return entry()->texture;
	}

	int getPixels(Renderer*, const Math::Recti* area /* nullable */, Editing::Dots &dots) const {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, object()->width(), object()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				if (object()->paletted()) {
					int idx = -1;
					object()->get(i, j, idx);
					dots.indexed[k] = idx;
				} else {
					Colour col;
					object()->get(i, j, col);
					dots.colored[k] = col;
				}
				++k;
				++result;
			}
		}

		return result;
	}
	int setPixels(Renderer*, const Math::Recti* area /* nullable */, const Editing::Dots &dots) {
		int result = 0;

		bool reload = texture()->usage() != Texture::STREAMING;
		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, object()->width(), object()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				if (object()->paletted()) {
					const int idx = dots.indexed[k];
					object()->set(i, j, idx);

					if (texture()->usage() == Texture::STREAMING) {
						if (!texture()->set(i, j, idx))
							reload = true;
					}
				} else {
					const Colour &col = dots.colored[k];
					object()->set(i, j, col);

					if (texture()->usage() == Texture::STREAMING) {
						if (!texture()->set(i, j, col))
							reload = true;
					}
				}
				++k;
				++result;
			}
		}

		if (reload) {
			entry()->texture = nullptr;
			entry()->touch(); // Ensure the runtime resources are ready.
		}

		return result;
	}
	bool getPixel(Renderer*, const Math::Vec2i &pos, Editing::Dot &dot) const {
		if (object()->paletted())
			return object()->get(pos.x, pos.y, dot.indexed);
		else
			return object()->get(pos.x, pos.y, dot.colored);
	}
	bool setPixel(Renderer* rnd, const Math::Vec2i &pos, const Editing::Dot &dot) {
		if (object()->paletted()) {
			if (!object()->set(pos.x, pos.y, dot.indexed))
				return false;

			if (!texture())
				return false;

			if (texture()->usage() == Texture::STREAMING) {
				if (!texture()->set(pos.x, pos.y, dot.indexed))
					return false;
			} else {
				texture() = Texture::Ptr(Texture::create());
				if (!texture()->fromImage(rnd, Texture::STREAMING, object().get(), Texture::NEAREST))
					return false;

				texture()->blend(Texture::BLEND);

				_binding.skipFrame = true;
			}
		} else {
			if (!object()->set(pos.x, pos.y, dot.colored))
				return false;

			if (!texture())
				return false;

			if (texture()->usage() == Texture::STREAMING) {
				if (!texture()->set(pos.x, pos.y, dot.colored))
					return false;
			} else {
				texture() = Texture::Ptr(Texture::create());
				if (!texture()->fromImage(rnd, Texture::STREAMING, object().get(), Texture::NEAREST))
					return false;

				texture()->blend(Texture::BLEND);

				_binding.skipFrame = true;
			}
		}

		return true;
	}
};

EditorTiles* EditorTiles::create(void) {
	EditorTilesImpl* result = new EditorTilesImpl();

	return result;
}

void EditorTiles::destroy(EditorTiles* ptr) {
	EditorTilesImpl* impl = static_cast<EditorTilesImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
