/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "commands_paintable.h"
#include "../utils/shapes.h"

/*
** {===========================================================================
** Paintable commands
*/

namespace Commands {

namespace Paintable {

Paint::Paint() {
	bitwiseOperation(Editing::Tools::BitwiseOperations::SET);
	penSize(1);
}

Paint::~Paint() {
}

const Editing::Dot* Paint::find(const Editing::Point &pos) const {
	const Editing::Point key(pos);
	Editing::Point::Set::iterator it = points().find(key);
	if (it == points().end())
		return nullptr;

	return &it->dot;
}

int Paint::populated(Populator populator) const {
	int result = 0;
	for (const Editing::Point &point : points()) {
		populator(point.position, point.dot);
		++result;
	}

	return result;
}

Command* Paint::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	for (const Editing::Point &point : points()) {
		set()(point.position, point.dot);
	}

	return this;
}

Command* Paint::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	for (const Editing::Point &point : old()) {
		set()(point.position, point.dot);
	}

	return this;
}

Paint* Paint::with(Editing::Tools::BitwiseOperations bitOp) {
	bitwiseOperation(bitOp);

	return this;
}

Paint* Paint::with(Getter get_, Setter set_, int penSize_) {
	get(get_);
	set(set_);
	penSize(penSize_);

	return this;
}

Paint* Paint::with(const Math::Vec2i &, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	Editing::Point p(pos, col);
	const std::pair<Editing::Point::Set::iterator, bool> ret = points().insert(p);

	if (ret.second) {
		get()(p.position, p.dot);
		old().insert(p);

		if (extraPlotter)
			extraPlotter(p.position.x, p.position.y);
	}

	return this;
}

Paint* Paint::with(const Math::Vec2i &, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	Editing::Dot pdot;
	get()(pos, pdot);

	Editing::Point p(pos, pdot);
	const Editing::Dot* mdot = find(pos);
	if (mdot == nullptr) {
		switch (bitwiseOperation()) {
		case Editing::Tools::SET: p.dot.indexed  = idx; break;
		case Editing::Tools::AND: p.dot.indexed &= idx; break;
		case Editing::Tools::OR:  p.dot.indexed |= idx; break;
		case Editing::Tools::XOR: p.dot.indexed ^= idx; break;
		}
	}

	const std::pair<Editing::Point::Set::iterator, bool> ret = points().insert(p);

	if (ret.second) {
		p.dot = pdot;
		old().insert(p);

		if (extraPlotter)
			extraPlotter(p.position.x, p.position.y);
	}

	return this;
}

Paint* Paint::with(const Math::Vec2i &validSize, const Points &points, const Colours &values) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	for (int i = 0; i < (int)points.size(); ++i) {
		const Math::Vec2i &p = points[i];
		const Colour &col = values[i];
		if (!valid(p))
			continue;

		with(validSize, p, col, nullptr);
	}

	return this;
}

Paint* Paint::with(const Math::Vec2i &validSize, const Points &points, const Indices &values) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	for (int i = 0; i < (int)points.size(); ++i) {
		const Math::Vec2i &p = points[i];
		const int idx = values[i];
		if (!valid(p))
			continue;

		with(validSize, p, idx, nullptr);
	}

	return this;
}

Command* Paint::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

void Paint::plot(int x, int y, Plotter proc) {
	const int penSize_ = penSize();
	const int penBegin = penSize_ / 2;
	int penEnd = penBegin;
	if (penBegin * 2 < penSize_)
		penEnd += penSize_ - penBegin * 2;

	for (int dy = -penBegin; dy < penEnd; ++dy) {
		for (int dx = -penBegin; dx < penEnd; ++dx) {
			const int px = x + dx;
			const int py = y + dy;

			proc(px, py);
		}
	}
};

Pencil::Pencil() {
	lastPosition(Math::Vec2i(-1, -1));
}

Pencil::~Pencil() {
}

unsigned Pencil::type(void) const {
	return TYPE();
}

const char* Pencil::toString(void) const {
	return "Pencil";
}

Paint* Pencil::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(lastPosition())) {
		if (valid(pos))
			lastPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
			}
		);

		return this;
	}

	Shapes::line(
		lastPosition().x, lastPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
				}
			);
		}
	);

	lastPosition(pos);

	return this;
}

Paint* Pencil::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(lastPosition())) {
		if (valid(pos))
			lastPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
			}
		);

		return this;
	}

	Shapes::line(
		lastPosition().x, lastPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
				}
			);
		}
	);

	lastPosition(pos);

	return this;
}

Line::Line() {
	firstPosition(Math::Vec2i(-1, -1));
}

Line::~Line() {
}

unsigned Line::type(void) const {
	return TYPE();
}

const char* Line::toString(void) const {
	return "Line";
}

Paint* Line::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
			}
		);

		return this;
	}

	points().clear();
	Shapes::line(
		firstPosition().x, firstPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
				}
			);
		}
	);

	return this;
}

Paint* Line::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
			}
		);

		return this;
	}

	points().clear();
	Shapes::line(
		firstPosition().x, firstPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
				}
			);
		}
	);

	return this;
}

Box::Box() {
	firstPosition(Math::Vec2i(-1, -1));
}

Box::~Box() {
}

unsigned Box::type(void) const {
	return TYPE();
}

const char* Box::toString(void) const {
	return "Box";
}

Paint* Box::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
			}
		);

		return this;
	}

	points().clear();
	Shapes::box(
		firstPosition().x, firstPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
				}
			);
		}
	);

	return this;
}

Paint* Box::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
			}
		);

		return this;
	}

	points().clear();
	Shapes::box(
		firstPosition().x, firstPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
				}
			);
		}
	);

	return this;
}

BoxFill::BoxFill() {
	firstPosition(Math::Vec2i(-1, -1));
}

BoxFill::~BoxFill() {
}

unsigned BoxFill::type(void) const {
	return TYPE();
}

const char* BoxFill::toString(void) const {
	return "Fill box";
}

Paint* BoxFill::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
			}
		);

		return this;
	}

	points().clear();
	Shapes::boxFill(
		firstPosition().x, firstPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
				}
			);
		}
	);

	return this;
}

Paint* BoxFill::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
			}
		);

		return this;
	}

	points().clear();
	Shapes::boxFill(
		firstPosition().x, firstPosition().y,
		pos.x, pos.y,
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			plot(
				x, y,
				[&] (int x_, int y_) -> void {
					if (!valid(Math::Vec2i(x_, y_)))
						return;

					Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
				}
			);
		}
	);

	return this;
}

Ellipse::Ellipse() {
	firstPosition(Math::Vec2i(-1, -1));
}

Ellipse::~Ellipse() {
}

unsigned Ellipse::type(void) const {
	return TYPE();
}

const char* Ellipse::toString(void) const {
	return "Ellipse";
}

Paint* Ellipse::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
			}
		);

		return this;
	}

	int x0 = firstPosition().x * 2;
	int y0 = firstPosition().y * 2;
	int x1 = pos.x * 2;
	int y1 = pos.y * 2;
	float rx = 0.0f;
	float cx = 0.0f;
	if (x1 > x0) {
		rx = (x1 - x0) / 2.0f;
		cx = x0 + rx;
	} else if (x1 < x0) {
		rx = (x0 - x1) / 2.0f;
		cx = x1 + rx;
	} else {
		cx = (float)x0;
	}
	float ry = 0.0f;
	float cy = 0.0f;
	if (y1 > y0) {
		ry = (y1 - y0) / 2.0f;
		cy = y0 + ry;
	} else if (y1 < y0) {
		ry = (y0 - y1) / 2.0f;
		cy = y1 + ry;
	} else {
		cy = (float)y0;
	}
	points().clear();
	Shapes::ellipse(
		(int)cx, (int)cy, (int)rx, (int)ry,
		[&] (int x_, int y_) -> void {
			if (x_ <= cx)
				x_ = (int)std::floor(x_ / 2.0f);
			else
				x_ = (int)std::ceil(x_ / 2.0f);
			if (y_ <= cy)
				y_ = (int)std::floor(y_ / 2.0f);
			else
				y_ = (int)std::ceil(y_ / 2.0f);

			if (!valid(Math::Vec2i(x_, y_)))
				return;

			plot(
				x_, y_,
				[&] (int xPos, int yPos) -> void {
					if (!valid(Math::Vec2i(xPos, yPos)))
						return;

					Paint::with(validSize, Math::Vec2i(xPos, yPos), col, extraPlotter);
				}
			);
		}
	);

	return this;
}

Paint* Ellipse::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
			}
		);

		return this;
	}

	int x0 = firstPosition().x * 2;
	int y0 = firstPosition().y * 2;
	int x1 = pos.x * 2;
	int y1 = pos.y * 2;
	float rx = 0.0f;
	float cx = 0.0f;
	if (x1 > x0) {
		rx = (x1 - x0) / 2.0f;
		cx = x0 + rx;
	} else if (x1 < x0) {
		rx = (x0 - x1) / 2.0f;
		cx = x1 + rx;
	} else {
		cx = (float)x0;
	}
	float ry = 0.0f;
	float cy = 0.0f;
	if (y1 > y0) {
		ry = (y1 - y0) / 2.0f;
		cy = y0 + ry;
	} else if (y1 < y0) {
		ry = (y0 - y1) / 2.0f;
		cy = y1 + ry;
	} else {
		cy = (float)y0;
	}
	points().clear();
	Shapes::ellipse(
		(int)cx, (int)cy, (int)rx, (int)ry,
		[&] (int x_, int y_) -> void {
			if (x_ <= cx)
				x_ = (int)std::floor(x_ / 2.0f);
			else
				x_ = (int)std::ceil(x_ / 2.0f);
			if (y_ <= cy)
				y_ = (int)std::floor(y_ / 2.0f);
			else
				y_ = (int)std::ceil(y_ / 2.0f);

			if (!valid(Math::Vec2i(x_, y_)))
				return;

			plot(
				x_, y_,
				[&] (int xPos, int yPos) -> void {
					if (!valid(Math::Vec2i(xPos, yPos)))
						return;

					Paint::with(validSize, Math::Vec2i(xPos, yPos), idx, extraPlotter);
				}
			);
		}
	);

	return this;
}

EllipseFill::EllipseFill() {
	firstPosition(Math::Vec2i(-1, -1));
}

EllipseFill::~EllipseFill() {
}

unsigned EllipseFill::type(void) const {
	return TYPE();
}

const char* EllipseFill::toString(void) const {
	return "Fill ellipse";
}

Paint* EllipseFill::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Colour &col, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), col, extraPlotter);
			}
		);

		return this;
	}

	int x0 = firstPosition().x * 2;
	int y0 = firstPosition().y * 2;
	int x1 = pos.x * 2;
	int y1 = pos.y * 2;
	float rx = 0.0f;
	float cx = 0.0f;
	if (x1 > x0) {
		rx = (x1 - x0) / 2.0f;
		cx = x0 + rx;
	} else if (x1 < x0) {
		rx = (x0 - x1) / 2.0f;
		cx = x1 + rx;
	} else {
		cx = (float)x0;
	}
	float ry = 0.0f;
	float cy = 0.0f;
	if (y1 > y0) {
		ry = (y1 - y0) / 2.0f;
		cy = y0 + ry;
	} else if (y1 < y0) {
		ry = (y0 - y1) / 2.0f;
		cy = y1 + ry;
	} else {
		cy = (float)y0;
	}
	points().clear();
	Shapes::ellipseFill(
		(int)cx, (int)cy, (int)rx, (int)ry,
		[&] (int x_, int y_) -> void {
			if (x_ <= cx)
				x_ = (int)std::floor(x_ / 2.0f);
			else
				x_ = (int)std::ceil(x_ / 2.0f);
			if (y_ <= cy)
				y_ = (int)std::floor(y_ / 2.0f);
			else
				y_ = (int)std::ceil(y_ / 2.0f);

			if (!valid(Math::Vec2i(x_, y_)))
				return;

			plot(
				x_, y_,
				[&] (int xPos, int yPos) -> void {
					if (!valid(Math::Vec2i(xPos, yPos)))
						return;

					Paint::with(validSize, Math::Vec2i(xPos, yPos), col, extraPlotter);
				}
			);
		}
	);

	return this;
}

Paint* EllipseFill::with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	if (!valid(firstPosition())) {
		if (valid(pos))
			firstPosition(pos);

		plot(
			pos.x, pos.y,
			[&] (int x_, int y_) -> void {
				if (!valid(Math::Vec2i(x_, y_)))
					return;

				Paint::with(validSize, Math::Vec2i(x_, y_), idx, extraPlotter);
			}
		);

		return this;
	}

	int x0 = firstPosition().x * 2;
	int y0 = firstPosition().y * 2;
	int x1 = pos.x * 2;
	int y1 = pos.y * 2;
	float rx = 0.0f;
	float cx = 0.0f;
	if (x1 > x0) {
		rx = (x1 - x0) / 2.0f;
		cx = x0 + rx;
	} else if (x1 < x0) {
		rx = (x0 - x1) / 2.0f;
		cx = x1 + rx;
	} else {
		cx = (float)x0;
	}
	float ry = 0.0f;
	float cy = 0.0f;
	if (y1 > y0) {
		ry = (y1 - y0) / 2.0f;
		cy = y0 + ry;
	} else if (y1 < y0) {
		ry = (y0 - y1) / 2.0f;
		cy = y1 + ry;
	} else {
		cy = (float)y0;
	}
	points().clear();
	Shapes::ellipseFill(
		(int)cx, (int)cy, (int)rx, (int)ry,
		[&] (int x_, int y_) -> void {
			if (x_ <= cx)
				x_ = (int)std::floor(x_ / 2.0f);
			else
				x_ = (int)std::ceil(x_ / 2.0f);
			if (y_ <= cy)
				y_ = (int)std::floor(y_ / 2.0f);
			else
				y_ = (int)std::ceil(y_ / 2.0f);

			if (!valid(Math::Vec2i(x_, y_)))
				return;

			plot(
				x_, y_,
				[&] (int xPos, int yPos) -> void {
					if (!valid(Math::Vec2i(xPos, yPos)))
						return;

					Paint::with(validSize, Math::Vec2i(xPos, yPos), idx, extraPlotter);
				}
			);
		}
	);

	return this;
}

Fill::Fill() {
}

Fill::~Fill() {
}

unsigned Fill::type(void) const {
	return TYPE();
}

const char* Fill::toString(void) const {
	return "Fill";
}

Paint* Fill::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);
	penSize(1);

	return this;
}

Paint* Fill::with(const Math::Vec2i &validSize, const Math::Recti* selection, const Math::Vec2i &pos, const Colour &col) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	Editing::Dot old;
	get()(pos, old);

	Math::Rect<int, 1> rect;
	int* rx0 = nullptr;
	int* ry0 = nullptr;
	int* rx1 = nullptr;
	int* ry1 = nullptr;
	if (selection) {
		rect = Math::Rect<int, 1>(
			(int)selection->x0, (int)selection->y0,
			(int)selection->x1, (int)selection->y1
		);
		rx0 = &rect.x0;
		ry0 = &rect.y0;
		rx1 = &rect.x1;
		ry1 = &rect.y1;
	}
	Shapes::fill(
		pos.x, pos.y,
		validSize.x, validSize.y,
		rx0, ry0,
		rx1, ry1,
		old.colored, col,
		[&] (int x, int y) -> Colour {
			Editing::Dot dot;
			get()(Math::Vec2i(x, y), dot);

			return dot.colored;
		},
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			Paint::with(validSize, Math::Vec2i(x, y), col, nullptr);

			Editing::Dot dot;
			dot.colored = col;
			set()(Math::Vec2i(x, y), dot);
		}
	);

	return this;
}

Paint* Fill::with(const Math::Vec2i &validSize, const Math::Recti* selection, const Math::Vec2i &pos, int idx) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	Editing::Dot old;
	get()(pos, old);

	Math::Rect<int, 1> rect;
	int* rx0 = nullptr;
	int* ry0 = nullptr;
	int* rx1 = nullptr;
	int* ry1 = nullptr;
	if (selection) {
		rect = Math::Rect<int, 1>(
			(int)selection->x0, (int)selection->y0,
			(int)selection->x1, (int)selection->y1
		);
		rx0 = &rect.x0;
		ry0 = &rect.y0;
		rx1 = &rect.x1;
		ry1 = &rect.y1;
	}
	Shapes::fill(
		pos.x, pos.y,
		validSize.x, validSize.y,
		rx0, ry0,
		rx1, ry1,
		old.indexed, idx,
		[&] (int x, int y) -> int {
			Editing::Dot dot;
			get()(Math::Vec2i(x, y), dot);

			return dot.indexed;
		},
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			Paint::with(validSize, Math::Vec2i(x, y), idx, nullptr);

			Editing::Dot dot;
			get()(Math::Vec2i(x, y), dot);
			switch (bitwiseOperation()) {
			case Editing::Tools::SET: dot.indexed  = idx; break;
			case Editing::Tools::AND: dot.indexed &= idx; break;
			case Editing::Tools::OR:  dot.indexed |= idx; break;
			case Editing::Tools::XOR: dot.indexed ^= idx; break;
			}
			set()(Math::Vec2i(x, y), dot);
		}
	);

	return this;
}

Command* Fill::exec(Object::Ptr, int, const Variant*) {
	return this;
}

Replace::Replace() {
}

Replace::~Replace() {
}

unsigned Replace::type(void) const {
	return TYPE();
}

const char* Replace::toString(void) const {
	return "Replace";
}

Paint* Replace::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);
	penSize(1);

	return this;
}

Paint* Replace::with(const Math::Vec2i &validSize, const Math::Recti* selection, const Math::Vec2i &pos, const Colour &col) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	Editing::Dot old;
	get()(pos, old);

	Math::Rect<int, 1> rect;
	int* rx0 = nullptr;
	int* ry0 = nullptr;
	int* rx1 = nullptr;
	int* ry1 = nullptr;
	if (selection) {
		rect = Math::Rect<int, 1>(
			(int)selection->x0, (int)selection->y0,
			(int)selection->x1, (int)selection->y1
		);
		rx0 = &rect.x0;
		ry0 = &rect.y0;
		rx1 = &rect.x1;
		ry1 = &rect.y1;
	}
	Shapes::replace(
		pos.x, pos.y,
		validSize.x, validSize.y,
		rx0, ry0,
		rx1, ry1,
		old.colored, col,
		[&] (int x, int y) -> Colour {
			Editing::Dot dot;
			get()(Math::Vec2i(x, y), dot);

			return dot.colored;
		},
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			Paint::with(validSize, Math::Vec2i(x, y), col, nullptr);

			Editing::Dot dot;
			dot.colored = col;
			set()(Math::Vec2i(x, y), dot);
		}
	);

	return this;
}

Paint* Replace::with(const Math::Vec2i &validSize, const Math::Recti* selection, const Math::Vec2i &pos, int idx) {
	auto valid = [validSize] (const Math::Vec2i &pos) -> bool {
		return pos.x >= 0 && pos.x < validSize.x && pos.y >= 0 && pos.y < validSize.y;
	};

	Editing::Dot old;
	get()(pos, old);

	Math::Rect<int, 1> rect;
	int* rx0 = nullptr;
	int* ry0 = nullptr;
	int* rx1 = nullptr;
	int* ry1 = nullptr;
	if (selection) {
		rect = Math::Rect<int, 1>(
			(int)selection->x0, (int)selection->y0,
			(int)selection->x1, (int)selection->y1
		);
		rx0 = &rect.x0;
		ry0 = &rect.y0;
		rx1 = &rect.x1;
		ry1 = &rect.y1;
	}
	Shapes::replace(
		pos.x, pos.y,
		validSize.x, validSize.y,
		rx0, ry0,
		rx1, ry1,
		old.indexed, idx,
		[&] (int x, int y) -> int {
			Editing::Dot dot;
			get()(Math::Vec2i(x, y), dot);

			return dot.indexed;
		},
		[&] (int x, int y) -> void {
			if (!valid(Math::Vec2i(x, y)))
				return;

			Paint::with(validSize, Math::Vec2i(x, y), idx, nullptr);

			Editing::Dot dot;
			get()(Math::Vec2i(x, y), dot);
			switch (bitwiseOperation()) {
			case Editing::Tools::SET: dot.indexed  = idx; break;
			case Editing::Tools::AND: dot.indexed &= idx; break;
			case Editing::Tools::OR:  dot.indexed |= idx; break;
			case Editing::Tools::XOR: dot.indexed ^= idx; break;
			}
			set()(Math::Vec2i(x, y), dot);
		}
	);

	return this;
}

Command* Replace::exec(Object::Ptr, int, const Variant*) {
	return this;
}

Blit::Blit() {
}

Blit::~Blit() {
}

int Blit::populated(Populator populator) const {
	int result = 0;
	int k = 0;
	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			const Editing::Dot &old_ = old()[k];
			const Editing::Dot &dot = dots()[k];
			++k;
			if (old_ != dot) {
				populator(Math::Vec2i(i, j), dot);
				++result;
			}
		}
	}

	return result;
}

Command* Blit::redo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::redo(obj, argc, argv);

	int k = 0;
	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			const Editing::Dot &dot = dots()[k];
			set()(Math::Vec2i(i, j), dot);

			++k;
		}
	}

	return this;
}

Command* Blit::undo(Object::Ptr obj, int argc, const Variant* argv) {
	Layered::Layered::undo(obj, argc, argv);

	int k = 0;
	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			const Editing::Dot &dot = old()[k];
			set()(Math::Vec2i(i, j), dot);

			++k;
		}
	}

	return this;
}

Blit* Blit::with(Getter get_, Setter set_) {
	get(get_);
	set(set_);

	return this;
}

Blit* Blit::with(int) {
	// Do nothing.

	return this;
}

Blit* Blit::with(const Math::Recti &area_) {
	area(area_);

	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			Editing::Dot dot;

			get()(Math::Vec2i(i, j), dot);
			old().push_back(dot);

			dot = Editing::Dot();
			dots().push_back(dot);

			set()(Math::Vec2i(i, j), dot);
		}
		old().shrink_to_fit();
		dots().shrink_to_fit();
	}

	return this;
}

Blit* Blit::with(const Math::Recti &area_, const Editing::Dot::Array &dots_) {
	area(area_);

	int k = 0;
	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			Editing::Dot dot;

			get()(Math::Vec2i(i, j), dot);
			old().push_back(dot);

			dot = dots_[k];
			dots().push_back(dot);

			set()(Math::Vec2i(i, j), dot);

			++k;
		}
		old().shrink_to_fit();
		dots().shrink_to_fit();
	}

	return this;
}

Blit* Blit::with(const Math::Recti &area_, const Colour* col) {
	area(area_);

	int k = 0;
	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			Editing::Dot dot;

			get()(Math::Vec2i(i, j), dot);
			old().push_back(dot);

			dot.colored = col[k];
			dots().push_back(dot);

			set()(Math::Vec2i(i, j), dot);

			++k;
		}
		old().shrink_to_fit();
		dots().shrink_to_fit();
	}

	return this;
}

Blit* Blit::with(const Math::Recti &area_, const int* idx) {
	area(area_);

	int k = 0;
	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			Editing::Dot dot;

			get()(Math::Vec2i(i, j), dot);
			old().push_back(dot);

			dot.indexed = idx[k];
			dots().push_back(dot);

			set()(Math::Vec2i(i, j), dot);

			++k;
		}
		old().shrink_to_fit();
		dots().shrink_to_fit();
	}

	return this;
}

Command* Blit::exec(Object::Ptr, int, const Variant*) {
	return this;
}

Stamp::Stamp() {
}

Stamp::~Stamp() {
}

unsigned Stamp::type(void) const {
	return TYPE();
}

const char* Stamp::toString(void) const {
	return "Stamp";
}

Rotate::Rotate() {
	direction(0);
}

Rotate::~Rotate() {
}

unsigned Rotate::type(void) const {
	return TYPE();
}

const char* Rotate::toString(void) const {
	switch (direction()) {
	case -1: // Rotate anticlockwise.
		return "Rotate anticlockwise";
	case 1: // Rotate clockwise
		return "Rotate clockwise";
	case 2: // Half turn.
		return "Half turn";
	}

	return "Rotate";
}

Blit* Rotate::with(int dir) {
	direction(dir);

	return this;
}

Blit* Rotate::with(const Math::Recti &area_) {
	area(area_);

	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			Editing::Dot dot;
			get()(Math::Vec2i(i, j), dot);
			old().push_back(dot);
		}
		old().shrink_to_fit();
	}

	switch (direction()) {
	case -1: // Rotate anticlockwise.
		for (int i = area().xMax(); i >= area().xMin(); --i) {
			for (int j = area().yMin(); j <= area().yMax(); ++j) {
				Editing::Dot dot;
				get()(Math::Vec2i(i, j), dot);
				dots().push_back(dot);
			}
			dots().shrink_to_fit();
		}

		break;
	case 1: // Rotate clockwise.
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			for (int j = area().yMax(); j >= area().yMin(); --j) {
				Editing::Dot dot;
				get()(Math::Vec2i(i, j), dot);
				dots().push_back(dot);
			}
			dots().shrink_to_fit();
		}

		break;
	case 2: // Half turn.
		for (int j = area().yMax(); j >= area().yMin(); --j) {
			for (int i = area().xMax(); i >= area().xMin(); --i) {
				Editing::Dot dot;
				get()(Math::Vec2i(i, j), dot);
				dots().push_back(dot);
			}
			dots().shrink_to_fit();
		}

		break;
	}

	return this;
}

Command* Rotate::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Flip::Flip() {
	direction(0);
}

Flip::~Flip() {
}

unsigned Flip::type(void) const {
	return TYPE();
}

const char* Flip::toString(void) const {
	switch (direction()) {
	case 0: // Flip horizontally.
		return "Flip horizontally";
	case 1: // Flip vertically.
		return "Flip vertically";
	}

	return "Flip";
}

Blit* Flip::with(int dir) {
	direction(dir);

	return this;
}

Blit* Flip::with(const Math::Recti &area_) {
	area(area_);

	for (int j = area().yMin(); j <= area().yMax(); ++j) {
		for (int i = area().xMin(); i <= area().xMax(); ++i) {
			Editing::Dot dot;
			get()(Math::Vec2i(i, j), dot);
			old().push_back(dot);
		}
		old().shrink_to_fit();
	}

	switch (direction()) {
	case 0: // Flip horizontally.
		for (int j = area().yMin(); j <= area().yMax(); ++j) {
			for (int i = area().xMax(); i >= area().xMin(); --i) {
				Editing::Dot dot;
				get()(Math::Vec2i(i, j), dot);
				dots().push_back(dot);
			}
			dots().shrink_to_fit();
		}

		break;
	case 1: // Flip vertically.
		for (int j = area().yMax(); j >= area().yMin(); --j) {
			for (int i = area().xMin(); i <= area().xMax(); ++i) {
				Editing::Dot dot;
				get()(Math::Vec2i(i, j), dot);
				dots().push_back(dot);
			}
			dots().shrink_to_fit();
		}

		break;
	}

	return this;
}

Command* Flip::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Cut::Cut() {
}

Cut::~Cut() {
}

unsigned Cut::type(void) const {
	return TYPE();
}

const char* Cut::toString(void) const {
	return "Cut";
}

Paste::Paste() {
}

Paste::~Paste() {
}

unsigned Paste::type(void) const {
	return TYPE();
}

const char* Paste::toString(void) const {
	return "Paste";
}

Delete::Delete() {
}

Delete::~Delete() {
}

unsigned Delete::type(void) const {
	return TYPE();
}

const char* Delete::toString(void) const {
	return "Delete";
}

}

}

/* ===========================================================================} */
