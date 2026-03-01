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

	EditorMapAsImage() {
	}
	~EditorMapAsImage() {
	}

	void clear(void) {
		selection.clear();
		overlay.clear();
	}
};

/* ===========================================================================} */
