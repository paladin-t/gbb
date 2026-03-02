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

struct EditorMapAsImage {
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

	typedef std::function<Image::Ptr &(void)> ObjectGetter;
	typedef std::function<Texture::Ptr &(void)> TextureGetter;

	typedef std::function<void(void)> TextureReloader;

public:
	struct {
		Project* project = nullptr;
		ObjectGetter getObject = nullptr;
		TextureGetter getTexture = nullptr;
		CommandQueue* commands = nullptr;
		Editing::Tools::PaintableTools* painting = nullptr;
		int* mouseActionButton = nullptr;
		int* weighting = nullptr;
		TextureReloader reloadTexture = nullptr;
	} shared;

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
	} ref;
	struct Tools {
		Math::Vec2i gridUnit = Math::Vec2i(0, 0);

		ImVec2 mousePos = ImVec2(-1, -1);
		ImVec2 mouseDiff = ImVec2(0, 0);

		PostHandler post = nullptr;
		Editing::Tools::PaintableTools postType = Editing::Tools::PENCIL;

		void clear(void) {
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
		Renderer* rnd,
		Project* prj,
		ObjectGetter obj, TextureGetter tex,
		CommandQueue* cmds,
		Editing::Tools::PaintableTools* painting_,
		int* mouseActBtn,
		int* weighting,
		TextureReloader reloadTex
	) {
		shared.project = prj;

		shared.getObject = obj;
		shared.getTexture = tex;

		shared.commands = cmds;
		shared.commands
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
			->reg<Commands::Map::AsImage::Delete>();

		shared.painting = painting_;

		shared.mouseActionButton = mouseActBtn;

		shared.weighting = weighting;

		shared.reloadTexture = reloadTex;

		binding.fill(
			std::bind(&EditorMapAsImage::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapAsImage::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapAsImage::getPixels, this, rnd, std::placeholders::_1, std::placeholders::_2),
			std::bind(&EditorMapAsImage::setPixels, this, rnd, std::placeholders::_1, std::placeholders::_2)
		);

		tools.gridUnit = Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
	}
	void dispose(void) {
		shared.project = nullptr;

		shared.getObject = nullptr;
		shared.getTexture = nullptr;

		shared.commands = nullptr;

		shared.painting = nullptr;

		shared.mouseActionButton = nullptr;

		shared.weighting = nullptr;

		shared.reloadTexture = nullptr;
	}

	void clear(void) {
		selection.clear();
		painting.clear();
		binding.clear();
		overlay.clear();
		ref.clear();
		tools.clear();
	}

	void update(void) {
		if (!ref.latest) {
			if (!shared.getObject()->paletted()) {
				ref.latest = new Pixels();
				const Colour white(255, 255, 255, 255);
				ref.latest->colored[0] = Colour(255, 0, 0, 255);
				ref.latest->colored[1] = Colour(0, 255, 0, 255);
				ref.latest->colored[2] = Colour(255, 255, 0, 255);
				ref.latest->colored[3] = Colour(0, 0, 255, 255);
				ref.latest->colored[4] = Colour(255, 0, 255, 255);
				ref.latest->colored[5] = Colour(0, 255, 255, 255);
				ref.latest->colored[6] = Colour(0, 0, 0, 255);
				ref.latest->colored[7] = Colour(255, 255, 255, 255);
				ref.latest->colored[8] = Colour(128, 128, 128, 255);
				ref.latest->colored[9] = Colour(255, 255, 255, 0);
			}
		}
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
			ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
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
			ref.real[0] = Math::clamp(col.r / 255.0f, 0.0f, 1.0f);
			ref.real[1] = Math::clamp(col.g / 255.0f, 0.0f, 1.0f);
			ref.real[2] = Math::clamp(col.b / 255.0f, 0.0f, 1.0f);
			ref.real[3] = Math::clamp(col.a / 255.0f, 0.0f, 1.0f);
		}
	}

	template<typename T> T* enqueue(void) {
		T* result = shared.commands->enqueue<T>();

		shared.project->toPollEditor(true);

		return result;
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
			const Colour col(
				(Byte)(ref.real[0] * 255),
				(Byte)(ref.real[1] * 255),
				(Byte)(ref.real[2] * 255),
				(Byte)(ref.real[3] * 255)
			);

			cmd->with(
				Math::Vec2i(shared.getObject()->width(), shared.getObject()->height()),
				selection.brush.empty() ? nullptr : &sel,
				cursor.position, col
			);
		}

		cmd->exec(shared.getObject());
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

		enqueue<Commands::Map::AsImage::Paste>()
			->with(binding.getPixel, binding.setPixel)
			->with(Math::Recti::byXYWH(x, y, area.width(), area.height()), dots)
			->exec(shared.getObject());

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

		enqueue<T>()
			->with(
				std::bind(&EditorMapAsImage::getPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorMapAsImage::setPixel, this, rnd, std::placeholders::_1, std::placeholders::_2),
				(*shared.weighting) + 1
			)
			->exec(shared.getObject());

		createOverlay(rnd);

		paintToolMove<T>(rnd);
	}
	template<typename T, bool Clear = true> void paintToolMove(Renderer* rnd) {
		if (shared.commands->empty())
			return;

		if (Clear)
			clearOverlay(rnd);

		Command* back = shared.commands->back();
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
			const Colour col(
				(Byte)(ref.real[0] * 255),
				(Byte)(ref.real[1] * 255),
				(Byte)(ref.real[2] * 255),
				(Byte)(ref.real[3] * 255)
			);

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
	}
	void paintToolUp(Renderer*) {
		if (!shared.commands->empty()) {
			shared.commands->back()->redo(shared.getObject(), &shared.getTexture());
		}

		destroyOverlay();
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
