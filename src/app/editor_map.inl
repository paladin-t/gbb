/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

/*
** {===========================================================================
** Map editor as image
*/

struct EditorMapAsImage : public Dispatchable {
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

	typedef std::function<Image::Ptr &(void)> ObjectGetter;
	typedef std::function<Texture::Ptr &(void)> TextureGetter;

	typedef std::function<void(void)> GraphicsReloader;
	typedef std::function<void(void)> TextureReloader;

	typedef std::function<void(void)> TiledInvalidator;

public:
	bool initialized = false;

	struct {
		Project* project = nullptr;
		ObjectGetter getObject = nullptr;
		TextureGetter getTexture = nullptr;
		Editing::Tools::PaintableTools* painting = nullptr;
		int* mouseActionButton = nullptr;
		int* weighting = nullptr;
		Editor::Debounce* debounce = nullptr;
		GraphicsReloader reloadGraphics = nullptr;
		TextureReloader reloadTexture = nullptr;
		TiledInvalidator invalidateTiled = nullptr;

		bool transferring = false;
	} shared;

	CommandQueue* commands = nullptr;
	Editing::Brush cursor;
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
	} selection;
	struct {
		std::string text;
		Editing::Brush cursor;

		void clear(void) {
			text.clear();
			cursor = Editing::Brush();
		}
	} status;
	std::function<void(const Command*)> refresher = nullptr;
	Editing::Painting painting;
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
	} binding;
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
	} overlay;
	struct Ref {
		int palette = 0;
		float real[4] = { 1, 1, 1, 1 };

		void clear(void) {
			palette = 0;
			for (int i = 0; i < GBBASIC_COUNTOF(real); ++i)
				real[i] = 1.0f;
		}
		Colour toColor(void) const {
			const Colour col(
				(Byte)(real[0] * 255),
				(Byte)(real[1] * 255),
				(Byte)(real[2] * 255),
				(Byte)(real[3] * 255)
			);

			return col;
		}
		void fromColor(const Colour &col) {
			real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
		}
	} ref;
	struct Tools {
		Math::Recti repeat;
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);

		PostHandler post = nullptr;
		Editing::Tools::PaintableTools postType = Editing::Tools::PENCIL;

		void clear(void) {
			repeat = Math::Recti();
			gridUnit = Math::Vec2i(0, 0);

			mousePos = ImVec2(-1, -1);
			mouseDiff = ImVec2(0, 0);

			post = nullptr;
			postType = Editing::Tools::PENCIL;
		}
	} tools;
	Processor processors[Editing::Tools::COUNT] = {
		Processor( // Hand.
			std::bind(&EditorMapAsImage::handToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::handToolMove, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::handToolUp, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::handToolHover, this, std::placeholders::_1)
		),
		Processor( // Eyedropper.
			std::bind(&EditorMapAsImage::eyedropperToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::eyedropperToolMove, this, std::placeholders::_1),
			nullptr,
			nullptr
		),
		Processor( // Pencil.
			std::bind(&EditorMapAsImage::paintToolDown<Commands::Map::AsImage::Pencil>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolMove<Commands::Map::AsImage::Pencil, false>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Paintbucket.
			nullptr,
			nullptr,
			std::bind(&EditorMapAsImage::paintbucketToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Lasso.
			std::bind(&EditorMapAsImage::lassoToolDown, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::lassoToolMove, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::lassoToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Line.
			std::bind(&EditorMapAsImage::paintToolDown<Commands::Map::AsImage::Line>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolMove<Commands::Map::AsImage::Line>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box.
			std::bind(&EditorMapAsImage::paintToolDown<Commands::Map::AsImage::Box>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolMove<Commands::Map::AsImage::Box>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Box fill.
			std::bind(&EditorMapAsImage::paintToolDown<Commands::Map::AsImage::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolMove<Commands::Map::AsImage::BoxFill>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse.
			std::bind(&EditorMapAsImage::paintToolDown<Commands::Map::AsImage::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolMove<Commands::Map::AsImage::Ellipse>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolUp, this, std::placeholders::_1),
			nullptr
		),
		Processor( // Ellipse fill.
			std::bind(&EditorMapAsImage::paintToolDown<Commands::Map::AsImage::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolMove<Commands::Map::AsImage::EllipseFill>, this, std::placeholders::_1),
			std::bind(&EditorMapAsImage::paintToolUp, this, std::placeholders::_1),
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

	EditorMapAsImage() {
	}
	~EditorMapAsImage() {
	}

	void initialize(
		Renderer* rnd, Workspace* ws,
		Project* prj,
		ObjectGetter obj, TextureGetter tex,
		Editing::Tools::PaintableTools* painting_,
		int* mouseActBtn,
		int* weighting,
		Editor::Debounce* debounce,
		GraphicsReloader reloadGfx, TextureReloader reloadTex,
		TiledInvalidator invalidateTiled
	) {
		if (initialized)
			return;
		initialized = true;

		shared.project = prj;

		shared.getObject = obj;
		shared.getTexture = tex;

		shared.painting = painting_;

		shared.mouseActionButton = mouseActBtn;

		shared.weighting = weighting;

		shared.debounce = debounce;

		shared.reloadGraphics = reloadGfx;

		shared.reloadTexture = reloadTex;

		shared.invalidateTiled = invalidateTiled;

		refresher = std::bind(&EditorMapAsImage::refresh, this, ws, std::placeholders::_1);

		commands = (new CommandQueue(GBBASIC_EDITOR_MAX_COMMAND_COUNT))
			->reg<Commands::Map::AsImage::Pencil>()
			->reg<Commands::Map::AsImage::Line>()
			->reg<Commands::Map::AsImage::Box>()
			->reg<Commands::Map::AsImage::BoxFill>()
			->reg<Commands::Map::AsImage::Ellipse>()
			->reg<Commands::Map::AsImage::EllipseFill>()
			->reg<Commands::Map::AsImage::Fill>()
			->reg<Commands::Map::AsImage::Replace>()
			->reg<Commands::Map::AsImage::Rotate>()
			->reg<Commands::Map::AsImage::Flip>()
			->reg<Commands::Map::AsImage::Cut>()
			->reg<Commands::Map::AsImage::Paste>()
			->reg<Commands::Map::AsImage::Delete>()
			->reg<Commands::Map::AsImage::Repeat>();

		binding.fill(
			std::bind(&EditorMapAsImage::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapAsImage::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapAsImage::getPixels, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapAsImage::setPixels, this, rnd, std::placeholders::_1, std::placeholders::_2)
		);

		tools.gridUnit = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
	}
	void dispose(void) {
		if (!initialized)
			return;
		initialized = false;

		shared.project = nullptr;

		shared.getObject = nullptr;
		shared.getTexture = nullptr;

		shared.painting = nullptr;

		shared.mouseActionButton = nullptr;

		shared.weighting = nullptr;

		shared.debounce = nullptr;

		shared.reloadGraphics = nullptr;

		shared.reloadTexture = nullptr;

		shared.invalidateTiled = nullptr;

		shared.transferring = false;

		refresher = nullptr;

		delete commands;
		commands = nullptr;
	}

	void clear(void) {
		selection.clear();
		status.clear();
		painting.clear();
		binding.clear();
		overlay.clear();
		ref.clear();
		tools.clear();
	}

	bool hasUnsavedChanges(void) const {
		return commands->hasUnsavedChanges();
	}
	void markChangesSaved(void) {
		commands->markChangesSaved();
	}

	void copy(bool clearSelection) {
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

		if (!binding.getPixels || !binding.setPixels)
			return;

		Math::Recti sel;
		const Math::Recti* selPtr = nullptr;
		int size = selection.area(sel);
		if (size == 0)
			size = shared.getObject()->width() * shared.getObject()->height();
		else
			selPtr = &sel;

		if (shared.getObject()->paletted()) {
			typedef std::vector<int> Data;

			Data vec(size, 0);
			Editing::Dots dots(&vec.front());
			binding.getPixels(selPtr, dots);

			const std::string buf = toString(shared.getObject(), sel, dots);
			const std::string osstr = Unicode::toOs(buf);

			Platform::setClipboardText(osstr.c_str());
		} else {
			Image::Colours vec(size, Colour());
			Editing::Dots dots(&vec.front());
			binding.getPixels(selPtr, dots);

			const std::string buf = toString(shared.getObject(), sel, dots);
			const std::string osstr = Unicode::toOs(buf);

			Platform::setClipboardText(osstr.c_str());
		}

		if (clearSelection)
			selection.clear();
	}
	void cut(void) {
		if (!binding.getPixel || !binding.setPixel)
			return;

		copy(false);

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		enqueue<Commands::Map::AsImage::Cut>()
			->with(binding.getPixel, binding.setPixel)
			->with(sel)
			->exec(shared.getObject());

		selection.clear();
	}
	bool pastable(void) const {
		return Platform::hasClipboardText();
	}
	void paste(void) {
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
			}
			dots.shrink_to_fit();

			return true;
		};

		const std::string osstr = Platform::getClipboardText();
		const std::string buf = Unicode::fromOs(osstr);
		Math::Recti area;
		Editing::Dot::Array dots;
		if (!fromString(shared.getObject(), buf, area, dots))
			return;

		const Editing::Tools::PaintableTools prevTool = *shared.painting;
		if (prevTool != Editing::Tools::STAMP) {
			*shared.painting = Editing::Tools::STAMP;

			processors[Editing::Tools::STAMP] = Processor{
				nullptr,
				nullptr,
				std::bind(&EditorMapAsImage::stampToolUp_Paste, this, std::placeholders::_1, area, dots, prevTool),
				std::bind(&EditorMapAsImage::stampToolHover_Paste, this, std::placeholders::_1, area, dots)
			};
		}

		destroyOverlay();
	}
	void del(bool) {
		if (!binding.getPixel || !binding.setPixel)
			return;

		Math::Recti sel;
		const int size = area(&sel);
		if (size == 0)
			return;

		enqueue<Commands::Map::AsImage::Delete>()
			->with(binding.getPixel, binding.setPixel)
			->with(sel)
			->exec(shared.getObject());

		selection.clear();
	}
	int area(Math::Recti* area /* nullable */) const {
		Math::Recti sel;
		int size = selection.area(sel);
		if (size == 0) {
			sel = Math::Recti::byXYWH(0, 0, shared.getObject()->width(), shared.getObject()->height());
			size = shared.getObject()->width() * shared.getObject()->height();
		}
		if (area)
			*area = sel;

		return size;
	}
	bool selectable(void) const {
		return true;
	}

	const char* redoable(void) const {
		const Command* cmd = commands->redoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}
	const char* undoable(void) const {
		const Command* cmd = commands->undoable();
		if (!cmd)
			return nullptr;

		return cmd->toString();
	}

	void redo(BaseAssets::Entry*) {
		const Command* cmd = commands->redoable();
		if (!cmd)
			return;

		const int width = shared.getObject()->width();
		const int height = shared.getObject()->height();
		commands->redo(shared.getObject());

		if (width != shared.getObject()->width() || height != shared.getObject()->height())
			selection.clear();

		refresher(cmd);

		shared.project->toPollEditor(true);
	}
	void undo(BaseAssets::Entry*) {
		const Command* cmd = commands->undoable();
		if (!cmd)
			return;

		const int width = shared.getObject()->width();
		const int height = shared.getObject()->height();
		commands->undo(shared.getObject());

		if (width != shared.getObject()->width() || height != shared.getObject()->height())
			selection.clear();

		refresher(cmd);

		shared.project->toPollEditor(true);
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		switch (msg) {
		case Editable::SELECT_ALL:
			selection.brush.position = Math::Vec2i(0, 0);
			selection.brush.size = Math::Vec2i(shared.getObject()->width(), shared.getObject()->height());

			return Variant(true);
		case Editable::CLEAR_UNDO_REDO_RECORDS: {
				if (shared.transferring)
					return Variant(true);

				const bool deep = unpack<bool>(argc, argv, 0, true);

				commands->clear();

				if (!deep)
					shared.reloadGraphics();
			}

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	Variant post(unsigned msg, int argc, const Variant* argv, bool* continue_) {
		*continue_ = true;

		Variant ret(false);
		switch (msg) {
		case Editable::SELECT_ALL:
			ret = post(msg, argc, argv);

			*continue_ = false;

			break;
		default:
			ret = post(msg, argc, argv);

			break;
		}

		return ret;
	}

	void update(void) {
		// Do nothing.
	}

	void flip(Editing::Tools::RotationsAndFlippings f) {
		if (!binding.getPixel || !binding.setPixel)
			return;

		Math::Recti frame;
		const int size = area(&frame);
		if (size == 0)
			return;

		switch (f) {
		case Editing::Tools::ROTATE_CLOCKWISE:
			enqueue<Commands::Map::AsImage::Rotate>()
				->with(binding.getPixel, binding.setPixel)
				->with(1)
				->with(frame)
				->exec(shared.getObject());

			break;
		case Editing::Tools::ROTATE_ANTICLOCKWISE:
			enqueue<Commands::Map::AsImage::Rotate>()
				->with(binding.getPixel, binding.setPixel)
				->with(-1)
				->with(frame)
				->exec(shared.getObject());

			break;
		case Editing::Tools::HALF_TURN:
			enqueue<Commands::Map::AsImage::Rotate>()
				->with(binding.getPixel, binding.setPixel)
				->with(2)
				->with(frame)
				->exec(shared.getObject());

			break;
		case Editing::Tools::FLIP_VERTICALLY:
			enqueue<Commands::Map::AsImage::Flip>()
				->with(binding.getPixel, binding.setPixel)
				->with(1)
				->with(frame)
				->exec(shared.getObject());

			break;
		case Editing::Tools::FLIP_HORIZONTALLY:
			enqueue<Commands::Map::AsImage::Flip>()
				->with(binding.getPixel, binding.setPixel)
				->with(0)
				->with(frame)
				->exec(shared.getObject());

			break;
		default: // Do nothing.
			break;
		}
	}

	void repeat(Renderer* rnd, const Math::Recti &area, const Math::Recti &repeat) {
		repeatToolUp(rnd, area, repeat);
	}

private:
	void createOverlay(Renderer* rnd) {
		overlay.blank = Image::Ptr(Image::create());
		overlay.blank->fromBlank(shared.getObject()->width(), shared.getObject()->height(), 0);
		overlay.texture = Texture::Ptr(Texture::create());
		overlay.texture->fromImage(rnd, Texture::STREAMING, overlay.blank.get(), Texture::NEAREST);
		overlay.texture->blend(Texture::BLEND);
	}
	void destroyOverlay(void) {
		overlay.clear();
	}
	void clearOverlay(Renderer* rnd) {
		overlay.texture->fromImage(rnd, Texture::STREAMING, overlay.blank.get(), Texture::NEAREST);
		overlay.texture->blend(Texture::BLEND);
	}

	template<typename T> T* enqueue(void) {
		T* result = commands->enqueue<T>();

		shared.project->toPollEditor(true);

		return result;
	}
	void refresh(Workspace* /* ws */, const Command* cmd) {
		const bool invalidateTiledData =
			Command::is<Commands::Map::AsImage::Pencil>(cmd) ||
			Command::is<Commands::Map::AsImage::Line>(cmd) ||
			Command::is<Commands::Map::AsImage::Box>(cmd) ||
			Command::is<Commands::Map::AsImage::BoxFill>(cmd) ||
			Command::is<Commands::Map::AsImage::Ellipse>(cmd) ||
			Command::is<Commands::Map::AsImage::EllipseFill>(cmd) ||
			Command::is<Commands::Map::AsImage::Fill>(cmd) ||
			Command::is<Commands::Map::AsImage::Replace>(cmd) ||
			Command::is<Commands::Map::AsImage::Rotate>(cmd) ||
			Command::is<Commands::Map::AsImage::Flip>(cmd) ||
			Command::is<Commands::Map::AsImage::Cut>(cmd) ||
			Command::is<Commands::Map::AsImage::Paste>(cmd) ||
			Command::is<Commands::Map::AsImage::Delete>(cmd) ||
			Command::is<Commands::Map::AsImage::Repeat>(cmd);

		if (invalidateTiledData) {
			shared.invalidateTiled();
		}
	}

	void handToolDown(Renderer*) {
		tools.mousePos = ImGui::GetMousePos();
	}
	void handToolMove(Renderer*) {
		const ImVec2 pos = ImGui::GetMousePos();
		tools.mouseDiff = pos - tools.mousePos;
		tools.mousePos = pos;
	}
	void handToolUp(Renderer*) {
		tools.mouseDiff = ImVec2(0, 0);
		*shared.mouseActionButton = ImGuiMouseButton_Left;
	}
	void handToolHover(Renderer*) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		if (tools.mouseDiff.x != 0) {
			ImGui::SetScrollX(ImGui::GetScrollX() - tools.mouseDiff.x);
			tools.mouseDiff.x = 0;
		}
		if (tools.mouseDiff.y != 0) {
			ImGui::SetScrollY(ImGui::GetScrollY() - tools.mouseDiff.y);
			tools.mouseDiff.y = 0;
		}
	}

	void eyedropperToolDown(Renderer*) {
		if (shared.getObject()->paletted()) {
			int idx = -1;
			shared.getObject()->get(cursor.position.x, cursor.position.y, idx);
			ref.palette = idx;
		} else {
			Colour col;
			shared.getObject()->get(cursor.position.x, cursor.position.y, col);
			ref.fromColor(col);
		}
	}
	void eyedropperToolMove(Renderer*) {
		if (shared.getObject()->paletted()) {
			int idx = -1;
			shared.getObject()->get(cursor.position.x, cursor.position.y, idx);
			ref.palette = idx;
		} else {
			Colour col;
			shared.getObject()->get(cursor.position.x, cursor.position.y, col);
			ref.fromColor(col);
		}
	}

	template<typename T> void paintbucketToolUp_(Renderer* rnd, const Math::Recti &sel) {
		T* cmd = enqueue<T>();
		Commands::Paintable::Paint::Getter getter = std::bind(&EditorMapAsImage::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		Commands::Paintable::Paint::Setter setter = std::bind(&EditorMapAsImage::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2);
		cmd->with(getter, setter);

		if (shared.getObject()->paletted()) {
			Colour col;
			Indexed::Ptr plt = shared.getObject()->palette();
			plt->get(ref.palette, col);

			cmd->with(
				Math::Vec2i(shared.getObject()->width(), shared.getObject()->height()),
				selection.brush.empty() ? nullptr : &sel,
				cursor.position, ref.palette
			);
		} else {
			const Colour col = ref.toColor();
			cmd->with(
				Math::Vec2i(shared.getObject()->width(), shared.getObject()->height()),
				selection.brush.empty() ? nullptr : &sel,
				cursor.position, col
			);
		}

		cmd->exec(shared.getObject());

		refresher(cmd);
	}
	void paintbucketToolUp(Renderer* rnd) {
		Math::Recti sel;
		if (selection.area(sel) && !Math::intersects(sel, cursor.position))
			selection.clear();

		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);

		if (ctrl.pressed())
			paintbucketToolUp_<Commands::Map::AsImage::Replace>(rnd, sel);
		else
			paintbucketToolUp_<Commands::Map::AsImage::Fill>(rnd, sel);
	}

	void lassoToolDown(Renderer*) {
		const Editing::Shortcut ctrl(SDL_SCANCODE_UNKNOWN, true, false, false);
		selection.tilewise = ctrl.pressed(false);

		selection.mouse = ImGui::GetMousePos();

		selection.initial = cursor.position;
		selection.brush.position = cursor.position;
		selection.brush.size = Math::Vec2i(1, 1);

		if (selection.tilewise) {
			selection.initial.x = (int)std::floor((float)selection.initial.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.initial.y = (int)std::floor((float)selection.initial.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.position.x = (int)std::floor((float)selection.brush.position.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.position.y = (int)std::floor((float)selection.brush.position.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.size.x = (int)std::ceil((float)selection.brush.size.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.size.y = (int)std::ceil((float)selection.brush.size.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
		}
	}
	void lassoToolMove(Renderer*) {
		const Math::Vec2i diff = cursor.position - selection.initial + Math::Vec2i(1, 1);

		const Math::Recti rect = Math::Recti::byXYWH(selection.initial.x, selection.initial.y, diff.x, diff.y);
		selection.brush.position = Math::Vec2i(rect.xMin(), rect.yMin());
		selection.brush.size = Math::Vec2i(rect.width(), rect.height());

		if (selection.tilewise) {
			selection.brush.position.x = (int)std::floor((float)selection.brush.position.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.position.y = (int)std::floor((float)selection.brush.position.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.size.x = (int)std::ceil((float)selection.brush.size.x / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			selection.brush.size.y = (int)std::ceil((float)selection.brush.size.y / GBBASIC_TILE_SIZE) * GBBASIC_TILE_SIZE;
			if (selection.brush.position.x < selection.initial.x)
				selection.brush.size.x = selection.initial.x - selection.brush.position.x + GBBASIC_TILE_SIZE;
			if (selection.brush.position.y < selection.initial.y)
				selection.brush.size.y = selection.initial.y - selection.brush.position.y + GBBASIC_TILE_SIZE;
		}
	}
	void lassoToolUp(Renderer*) {
		const ImVec2 diff = ImGui::GetMousePos() - selection.mouse;

		if (std::abs(diff.x) < Math::EPSILON<float>() && std::abs(diff.y) < Math::EPSILON<float>()) {
			if (selection.idle == 0) {
				selection.clear();
				++selection.idle;
			} else {
				selection.idle = 0;
			}
		}
	}

	void stampToolUp_Paste(Renderer*, const Math::Recti &area, const Editing::Dot::Array &dots, Editing::Tools::PaintableTools prevTool) {
		if (!binding.getPixel || !binding.setPixel)
			return;

		selection.clear();

		const int xOff = -area.width() / 2;
		const int yOff = -area.height() / 2;
		const int x = cursor.position.x + area.xMin() + xOff;
		const int y = cursor.position.y + area.yMin() + yOff;

		Command* cmd = enqueue<Commands::Map::AsImage::Paste>()
			->with(binding.getPixel, binding.setPixel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->exec(shared.getObject());

		refresher(cmd);

		destroyOverlay();

		*shared.painting = prevTool;
	}
	void stampToolHover_Paste(Renderer* rnd, const Math::Recti &area, const Editing::Dot::Array &dots) {
		bool reload = painting.moved();

		if (overlay.empty()) {
			createOverlay(rnd);

			reload = true;
		}

		if (reload) {
			clearOverlay(rnd);

			const int xOff = -area.width() / 2;
			const int yOff = -area.height() / 2;
			int k = 0;
			for (int j = area.yMin(); j <= area.yMax(); ++j) {
				const int y = cursor.position.y + j + yOff;
				for (int i = area.xMin(); i <= area.xMax(); ++i) {
					const int x = cursor.position.x + i + xOff;

					if (shared.getObject()->paletted()) {
						Colour col;
						Indexed::Ptr plt = shared.getObject()->palette();
						plt->get(dots[k].indexed, col);

						overlay.texture->set(x, y, col);
					} else {
						const Colour &col = dots[k].colored;

						overlay.texture->set(x, y, col);
					}

					++k;
				}
			}
		}
	}

	template<typename T> void paintToolDown(Renderer* rnd) {
		selection.clear();

		Command* cmd = enqueue<T>()
			->with(
				std::bind(&EditorMapAsImage::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapAsImage::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				(*shared.weighting) + 1
			)
			->exec(shared.getObject());

		refresher(cmd);

		createOverlay(rnd);

		paintToolMove<T>(rnd);
	}
	template<typename T, bool Clear = true> void paintToolMove(Renderer* rnd) {
		if (commands->empty())
			return;

		if (Clear)
			clearOverlay(rnd);

		Command* back = commands->back();
		GBBASIC_ASSERT(back->type() == T::TYPE());
		T* cmd = Command::as<T>(back);
		if (shared.getObject()->paletted()) {
			Colour col;
			Indexed::Ptr plt = shared.getObject()->palette();
			plt->get(ref.palette, col);

			cmd->with(
				Math::Vec2i(shared.getObject()->width(), shared.getObject()->height()),
				cursor.position, ref.palette,
				[&] (int x, int y) -> void {
					Colour col_ = col;
					if (col_.a == 0) {
						if (((x % 2) + (y % 2)) % 2)
							col_ = Colour(121, 121, 121, 255);
						else
							col_ = Colour(221, 221, 221, 255);
					}
					overlay.texture->set(x, y, col_);
				}
			);
		} else {
			const Colour col = ref.toColor();
			cmd->with(
				Math::Vec2i(shared.getObject()->width(), shared.getObject()->height()),
				cursor.position, col,
				[&] (int x, int y) -> void {
					Colour col_ = col;
					if (col_.a == 0) {
						if (((x % 2) + (y % 2)) % 2)
							col_ = Colour(121, 121, 121, 255);
						else
							col_ = Colour(221, 221, 221, 255);
					}
					overlay.texture->set(x, y, col_);
				}
			);
		}

		shared.debounce->modified();
	}
	void paintToolUp(Renderer*) {
		if (!commands->empty()) {
			commands->back()->redo(shared.getObject(), &shared.getTexture());
		}

		shared.debounce->modified();

		destroyOverlay();
	}

	void repeatToolUp(Renderer*, const Math::Recti &area, const Math::Recti &repeat) {
		if (!binding.getPixel || !binding.setPixel)
			return;

		const int objW = shared.getObject()->width();
		const int objH = shared.getObject()->height();
		const int xMin = area.xMin() + area.width() * repeat.xMin();
		const int yMin = area.yMin() + area.height() * repeat.yMin();
		const int xMax = area.xMax() + area.width() * repeat.xMax();
		const int yMax = area.yMax() + area.height() * repeat.yMax();
		const Math::Recti area_(Math::max(0, xMin), Math::max(0, yMin), Math::min(objW - 1, xMax), Math::min(objH - 1, yMax));
		Editing::Dot::Array dots;
		for (int j = yMin; j <= yMax; ++j) {
			if (j < 0 || j >= objH)
				continue;

			int y = (j - area.yMin()) % area.height();
			if (y < 0)
				y += area.height();
			const int py = y + area.yMin();

			for (int i = xMin; i <= xMax; ++i) {
				if (i < 0 || i >= objW)
					continue;

				int x = (i - area.xMin()) % area.width();
				if (x < 0)
					x += area.width();
				const int px = x + area.xMin();

				if (shared.getObject()->paletted()) {
					int idx = -1;
					shared.getObject()->get(px, py, idx);

					Editing::Dot dot;
					dot.indexed = idx;
					dots.push_back(dot);
				} else {
					Colour col;
					shared.getObject()->get(px, py, col);

					Editing::Dot dot;
					dot.colored = col;
					dots.push_back(dot);
				}
			}
		}
		dots.shrink_to_fit();
		GBBASIC_ASSERT(area_.width() * area_.height() == (int)dots.size() && "Wrong data.");

		enqueue<Commands::Map::AsImage::Repeat>()
			->with(binding.getPixel, binding.setPixel)
			->with(Math::Recti::byXYWH(area_.xMin(), area_.yMin(), area_.width(), area_.height()), dots)
			->exec(shared.getObject());
	}

	int getPixels(Renderer*, const Math::Recti* area /* nullable */, Editing::Dots &dots) const {
		int result = 0;

		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, shared.getObject()->width(), shared.getObject()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				if (shared.getObject()->paletted()) {
					int idx = -1;
					shared.getObject()->get(i, j, idx);
					dots.indexed[k] = idx;
				} else {
					Colour col;
					shared.getObject()->get(i, j, col);
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

		bool reload = shared.getTexture()->usage() != Texture::STREAMING;
		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, shared.getObject()->width(), shared.getObject()->height());
		int k = 0;
		for (int j = area_.yMin(); j <= area_.yMax(); ++j) {
			for (int i = area_.xMin(); i <= area_.xMax(); ++i) {
				if (shared.getObject()->paletted()) {
					const int idx = dots.indexed[k];
					shared.getObject()->set(i, j, idx);

					if (shared.getTexture()->usage() == Texture::STREAMING) {
						if (!shared.getTexture()->set(i, j, idx))
							reload = true;
					}
				} else {
					const Colour &col = dots.colored[k];
					shared.getObject()->set(i, j, col);

					if (shared.getTexture()->usage() == Texture::STREAMING) {
						if (!shared.getTexture()->set(i, j, col))
							reload = true;
					}
				}
				++k;
				++result;
			}
		}

		if (reload) {
			shared.reloadTexture();
		}

		return result;
	}
	bool getPixel(Renderer*, const Math::Vec2i &pos, Editing::Dot &dot) const {
		if (shared.getObject()->paletted())
			return shared.getObject()->get(pos.x, pos.y, dot.indexed);
		else
			return shared.getObject()->get(pos.x, pos.y, dot.colored);
	}
	bool setPixel(Renderer* rnd, const Math::Vec2i &pos, const Editing::Dot &dot) {
		if (shared.getObject()->paletted()) {
			if (!shared.getObject()->set(pos.x, pos.y, dot.indexed))
				return false;

			if (!shared.getTexture())
				return false;

			if (shared.getTexture()->usage() == Texture::STREAMING) {
				if (!shared.getTexture()->set(pos.x, pos.y, dot.indexed))
					return false;
			} else {
				shared.getTexture() = Texture::Ptr(Texture::create());
				if (!shared.getTexture()->fromImage(rnd, Texture::STREAMING, shared.getObject().get(), Texture::NEAREST))
					return false;

				shared.getTexture()->blend(Texture::BLEND);

				binding.skipFrame = true;
			}
		} else {
			if (!shared.getObject()->set(pos.x, pos.y, dot.colored))
				return false;

			if (!shared.getTexture())
				return false;

			if (shared.getTexture()->usage() == Texture::STREAMING) {
				if (!shared.getTexture()->set(pos.x, pos.y, dot.colored))
					return false;
			} else {
				shared.getTexture() = Texture::Ptr(Texture::create());
				if (!shared.getTexture()->fromImage(rnd, Texture::STREAMING, shared.getObject().get(), Texture::NEAREST))
					return false;

				shared.getTexture()->blend(Texture::BLEND);

				binding.skipFrame = true;
			}
		}

		return true;
	}
};

/* ===========================================================================} */
