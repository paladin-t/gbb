/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "actor.h"
#include "matrice.h"
#include "text.h"

/*
** {===========================================================================
** Actor
*/

class ActorImpl : public Actor {
private:
	bool _is8x16 = true;
	Compactors _compactor = Compactors::NORMAL;
	Frame::Array _frames;

	std::string _updateRoutine;
	std::string _onHitsRoutine;

public:
	ActorImpl() {
	}
	virtual ~ActorImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Actor** ptr, bool represented) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		ActorImpl* result = static_cast<ActorImpl*>(Actor::create());
		result->_is8x16 = _is8x16;
		for (int i = 0; i < (int)_frames.size(); ++i) {
			const Frame &frame = _frames[i];
			if (represented)
				result->_frames.push_back(Frame(frame));
			else
				result->_frames.push_back(Frame(frame.pixels, frame.properties, frame.anchor));
		}

		*ptr = result;

		return true;
	}
	virtual bool clone(Actor** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Actor* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		result = Math::hash(result, _is8x16);

		result = Math::hash(result, _compactor);

		for (int i = 0; i < (int)_frames.size(); ++i) {
			const Frame &frame = _frames[i];
			result = Math::hash(result, frame.hash());
		}

		return result;
	}
	virtual int compare(const Actor* other) const override {
		const ActorImpl* implOther = static_cast<const ActorImpl*>(other);

		if (this == other)
			return 0;

		if (!other)
			return 1;

		if (!_is8x16 && implOther->_is8x16)
			return -1;
		else if (_is8x16 && !implOther->_is8x16)
			return 1;

		if (_compactor < implOther->_compactor)
			return -1;
		else if (_compactor > implOther->_compactor)
			return 1;

		if (_frames.size() < implOther->_frames.size())
			return -1;
		else if (_frames.size() > implOther->_frames.size())
			return 1;

		for (int i = 0; i < (int)_frames.size(); ++i) {
			const Frame &framel = _frames[i];
			const Frame &framer = implOther->_frames[i];
			if (framel < framer)
				return -1;
			else if (framel > framer)
				return 1;
		}

		return 0;
	}

	virtual int cleanup(void) override {
		int result = 0;
		for (Frame &frame : _frames) {
			frame.cleanup();
			++result;
		}

		return result;
	}
	virtual int cleanup(int frame_) override {
		if (frame_ < 0 || frame_ >= (int)_frames.size())
			return 0;

		Frame &frame = _frames[frame_];
		frame.cleanup();

		return 1;
	}

	virtual bool is8x16(void) const override {
		return _is8x16;
	}
	virtual void is8x16(bool val) override {
		_is8x16 = val;
	}

	virtual Compactors compactor(void) const override {
		return _compactor;
	}
	virtual void compactor(Compactors c) override {
		_compactor = c;
	}

	virtual int count(void) const override {
		return (int)_frames.size();
	}

	virtual const Frame* get(int idx) const override {
		if (idx < 0 || idx >= (int)_frames.size())
			return nullptr;

		return &_frames[idx];
	}
	virtual Frame* get(int idx) override {
		if (idx < 0 || idx >= (int)_frames.size())
			return nullptr;

		return &_frames[idx];
	}
	virtual bool set(int idx, const Frame &frame) override {
		if (idx < 0 || idx >= (int)_frames.size())
			return false;

		_frames[idx] = frame;

		return true;
	}
	virtual Frame* add(Image::Ptr img, Image::Ptr props, const Math::Vec2i &anchor) override {
		_frames.push_back(Frame(img, props, anchor));

		return &_frames.back();
	}
	virtual bool insert(int index, Image::Ptr img, Image::Ptr props, const Math::Vec2i &anchor) override {
		if (index < 0 || index > (int)_frames.size())
			return false;

		_frames.insert(_frames.begin() + index, Frame(img, props, anchor));

		return true;
	}
	virtual bool remove(int index, Image::Ptr* img, Image::Ptr* props) override {
		if (img)
			*img = nullptr;
		if (props)
			*props = nullptr;

		if (index < 0 || index > (int)_frames.size())
			return false;

		Frame::Array::iterator it = _frames.begin() + index;
		if (img)
			*img = it->pixels;
		if (props)
			*props = it->properties;
		_frames.erase(it);

		return true;
	}

	virtual bool touch(Renderer* rnd, int idx) override {
		GBBASIC_ASSERT(idx >= 0 && idx < (int)_frames.size());

		Frame &frame = _frames[idx];

		return frame.touch(rnd);
	}
	virtual bool touch(Renderer* rnd, int idx, Frame::PaletteResolver resolve) override {
		GBBASIC_ASSERT(idx >= 0 && idx < (int)_frames.size());

		Frame &frame = _frames[idx];

		return frame.touch(rnd, resolve);
	}

	virtual bool computeSafeContentRect(Math::Recti &rect) const override {
		bool withSameAnchor = true;
		Math::Vec2i firstAnchor;
		int xMin = std::numeric_limits<int>::max();
		int yMin = std::numeric_limits<int>::max();
		int xMax = std::numeric_limits<int>::min();
		int yMax = std::numeric_limits<int>::min();
		for (int k = 0; k < (int)_frames.size(); ++k) {
			const Frame &frame = _frames[k];
			if (withSameAnchor) {
				if (k == 0) {
					firstAnchor = frame.anchor;
				} else {
					if (firstAnchor != frame.anchor)
						withSameAnchor = false;
				}
			}

			const Math::Recti rect_ = frame.computeContentRect();
			if (rect_.xMin() < xMin) xMin = rect_.xMin();
			if (rect_.yMin() < yMin) yMin = rect_.yMin();
			if (rect_.xMax() > xMax) xMax = rect_.xMax();
			if (rect_.yMax() > yMax) yMax = rect_.yMax();
		}

		rect = Math::Recti(xMin - firstAnchor.x, yMin - firstAnchor.y, xMax - firstAnchor.x, yMax - firstAnchor.y);

		return withSameAnchor;
	}

	virtual int slice(int idx) override {
		GBBASIC_ASSERT(idx >= 0 && idx < (int)_frames.size());

		Frame &frame = _frames[idx];

		return frame.slice(this);
	}

	virtual bool compact(Animation &animation, Shadow::Array &shadow, Slice::Array &slices, Image* fullImage) const override {
		// Prepare.
		auto indexOfSlice = [] (const Slice::Array &coll, const Slice &what, bool &hFlip, bool &vFlip) -> int {
			for (int i = 0; i < (int)coll.size(); ++i) {
				if (coll[i] == what) // Compare the slices' images.
					return i;

				Image* ptr = nullptr;
				what.image->clone(&ptr);
				Image::Ptr tmp(ptr);
				tmp->flip(true, false);
				if (coll[i] == Slice(tmp, what.offset)) {
					hFlip = true;

					return i;
				}

				tmp->flip(false, true);
				if (coll[i] == Slice(tmp, what.offset)) {
					hFlip = true;
					vFlip = true;

					return i;
				}

				tmp->flip(true, false);
				if (coll[i] == Slice(tmp, what.offset)) {
					vFlip = true;

					return i;
				}
			}

			return -1;
		};
		auto indexOfShadow = [this] (const Shadow::Array &coll, const Shadow &what, int i_) -> int {
			for (int i = 0; i < (int)coll.size(); ++i) {
				if (coll[i] == what) {                           // Compare the shadows' slice indices, offsets and properties,
					if (_frames[i].anchor == _frames[i_].anchor) // as well as their anchors.
						return i;
				}
			}

			return -1;
		};

		animation.clear();
		shadow.clear();
		slices.clear();

		// Fill in the shadow.
		shadow.resize(_frames.size());
		for (int i = 0; i < (int)_frames.size(); ++i) {
			const Frame &frame = _frames[i];
			if (frame.slices.empty())
				return false;

			Actor::Shadow &s = shadow[i];
			const Math::Vec2i tsize = is8x16() ? Math::Vec2i(8, 16) : Math::Vec2i(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
			const Math::Vec2i &anchor = frame.anchor;
			int subX = anchor.x - GBBASIC_TILE_SIZE;
			int subY = anchor.y - GBBASIC_TILE_SIZE * 2;

			for (int j = 0; j < (int)frame.slices.size(); ++j) {
				const Slice &slice = frame.slices[j];
				if (slice.image->blank()) {
					if (compactor() == Compactors::NON_BLANK) {
						GBBASIC_ASSERT(false && "Wrong data.");
					}

					continue;
				}

				int index = -1;
				bool hFlip = false;
				bool vFlip = false;
				const int exists = indexOfSlice(slices, slice, hFlip, vFlip);
				if (exists == -1) {
					slices.push_back(slice);
					index = (int)slices.size() - 1;
				} else {
					index = exists;
				}

				const Math::Vec2i step(slice.offset.x - subX, slice.offset.y - subY);
				subX = slice.offset.x;
				subY = slice.offset.y;

				int props = 0;
				const int ix = slice.offset.x / tsize.x;
				const int iy = slice.offset.y / tsize.y;
				if (is8x16()) {
					const bool ret = frame.properties->get(ix, iy * 2, props); // Take the upper tile's property.
					if (!ret) {
						GBBASIC_ASSERT(false && "Wrong data.");
					}
				} else {
					const bool ret = frame.properties->get(ix, iy, props);
					if (!ret) {
						GBBASIC_ASSERT(false && "Wrong data.");
					}
				}
				props     &= ~(0x00000001 << GBBASIC_ACTOR_HFLIP_BIT);
				props     &= ~(0x00000001 << GBBASIC_ACTOR_VFLIP_BIT);
				if (hFlip)
					props |=  (0x00000001 << GBBASIC_ACTOR_HFLIP_BIT);
				if (vFlip)
					props |=  (0x00000001 << GBBASIC_ACTOR_VFLIP_BIT);

				const Shadow::Ref ref(index, slice.offset, step, props);
				s.refs.push_back(ref);
			}
		}

		// Fill in the animation.
		for (int i = 0; i < (int)shadow.size(); ++i) {
			const Shadow &s = shadow[i];

			const int exists = indexOfShadow(shadow, s, i);
			if (exists == -1)
				animation.push_back(i);
			else
				animation.push_back(exists);
		}

		// Fill the image.
		if (fullImage) {
			if (slices.empty()) {
				fullImage->fromBlank(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE, GBBASIC_PALETTE_DEPTH);
			} else {
				const Image::Ptr &sample = slices.front().image;
				const int sw = sample->width();
				const int sh = sample->height();
				const int area = sw * sh * (int)slices.size();
				const float edge = std::ceil(std::sqrt((float)area));
				const int w = (int)std::ceil(edge / sw);
				const int h = (int)std::ceil(std::ceil((float)area / (w * sw)) / GBBASIC_TILE_SIZE);
				fullImage->fromBlank(w * sw, h * GBBASIC_TILE_SIZE, GBBASIC_PALETTE_DEPTH);
				int x = 0;
				int y = 0;
				for (int k = 0; k < (int)slices.size(); ++k) {
					const Image::Ptr &img = slices[k].image;
					if (sw == sh) {
						img->blit(fullImage, x * sw, y * sh, sw, sh, 0, 0);
						if (++x >= w) {
							x = 0;
							++y;
						}
					} else /* if (sw < sh) */ {
						img->blit(fullImage, x * sw, y * sw, sw, sw, 0, 0);
						if (++x >= w) {
							x = 0;
							++y;
						}
						img->blit(fullImage, x * sw, y * (sh - sw), sw, (sh - sw), 0, sw);
						if (++x >= w) {
							x = 0;
							++y;
						}
					}
				}
			}
		}

		// Finish.
		return true;
	}

	virtual bool load(bool is8x16_) override {
		_is8x16 = is8x16_;

		return true;
	}
	virtual void unload(void) override {
		_is8x16 = false;
		_frames.clear();
	}

	virtual const std::string &updateRoutine(void) const override {
		return _updateRoutine;
	}
	virtual void updateRoutine(const std::string &val) override {
		_updateRoutine = val;
	}
	virtual const std::string &onHitsRoutine(void) const override {
		return _onHitsRoutine;
	}
	virtual void onHitsRoutine(const std::string &val) override {
		_onHitsRoutine = val;
	}

	virtual bool toC(std::string &val, const char* name_) override {
		// Prepare.
		typedef std::vector<int> AnimationIndices;
		typedef std::vector<int> FilledAnimations;

		AnimationIndices animationIndices;
		FilledAnimations filledAnimations;

		val.clear();

		const std::string name = name_ ? Text::toVariableName(name_) : "Actor";
		const std::string pascalName = Text::snakeToPascal(name);
		const std::string snakeName = Text::pascalToSnake(name);

		// Compact the actor.
		Animation animation;
		Shadow::Array shadow;
		Slice::Array slices;
		for (int i = 0; i < count(); ++i) {
			const Frame* frame = get(i);
			if (frame->slices.empty())
				slice(i);
		}
		compact(animation, shadow, slices, nullptr);

		// Serialize the header.
		val += "#include <gb/metasprites.h>\n";
		val += "\n";

		// Serialize the tiles.
		std::string val_;
		val_ += "const unsigned char " + pascalName + "Tiles[{0}] = {\n";
		int m = 0;
		for (int k = 0; k < (int)slices.size(); ++k) {
			const Slice &slice_ = slices[k];
			const Image::Ptr &img = slice_.image;
			const int sw = img->width();
			const int sh = img->height();
			const int w = sw / GBBASIC_TILE_SIZE;
			const int h = sh / GBBASIC_TILE_SIZE;
			img->serializeTiles(
				GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
				nullptr,
				nullptr,
				[&] (int y, int /* lineno */, int tx, int ty, UInt8 ln0, UInt8 ln1) -> void {
					if (m % 2 == 0) {
						val_ += "    ";
					}
					val_ += "0x" + Text::toHex(ln0, 2, '0', true); // The low bits of a line.
					val_ += ",";
					val_ += "0x" + Text::toHex(ln1, 2, '0', true); // The high bits of a line.
					const bool isFinal =
						y == GBBASIC_TILE_SIZE - 1 &&
						tx == w - 1 && ty == h - 1 &&
						k == (int)slices.size() - 1;
					if (!isFinal) {
						val_ += ",";
					}
					if (++m % 2 == 0) {
						val_ += "\n";
					}
					if (m % 8 == 0 && !isFinal) {
						val_ += "\n";
					}
				}
			);
		}
		val_ += "};\n";
		val += Text::format(val_, { Text::toString(m * 2) });

		// Serialize the animations.
		int n = 0;
		for (int i = 0; i < (int)animation.size(); ++i) {
			const int a = animation[i];
			FilledAnimations::const_iterator it = std::find(filledAnimations.begin(), filledAnimations.end(), a);
			if (it == filledAnimations.end()) {
				animationIndices.push_back(n);
				filledAnimations.push_back(a);
			} else {
				animationIndices.push_back(*it);

				continue; // Ignore if the frame has already been filled.
			}

			const Shadow &s = shadow[a];
			val += Text::format("const metasprite_t " + snakeName + "_frame{0}[] = {\n", { Text::toString(n) });
			for (int i = 0; i < (int)s.refs.size(); ++i) {
				const int t             = s.refs[i].index * (is8x16() ? 2 : 1);
				const Math::Vec2i &step = s.refs[i].step;
				const int p             = s.refs[i].properties;

				val += "    METASPR_ITEM(";
				val += Text::toString((Int8)step.y); // The Y step.
				val += ", ";
				val += Text::toString((Int8)step.x); // The X step.
				val += ", ";
				val += Text::toString((UInt8)t);     // The tile index.
				val += ", ";
				val += Text::toString((UInt8)p);     // The properties.
				val += "),\n";
			}
			val += "    METASPR_TERM";               // The end of this frame.
			val += "\n";
			val += "};\n";

			++n;
		}

		// Serialize the actor.
		val += Text::format("const metasprite_t * const " + pascalName + "[{0}] = {\n", { Text::toString((UInt32)animation.size() + 1) });
		for (int i = 0; i < (int)animationIndices.size(); ++i) {
			const int a = animationIndices[i];

			val += Text::format("    " + snakeName + "_frame{0}", { Text::toString(a) });
			val += ",\n";
		}
		val += "    NULL";
		val += "\n";
		val += "};\n";

		// Finish.
		return true;
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		val.SetObject();

		rapidjson::Value jstris8x16;
		jstris8x16.SetString("is_8x16", doc.GetAllocator());
		rapidjson::Value jvalis8x16;
		jvalis8x16.SetBool(is8x16());
		val.AddMember(jstris8x16, jvalis8x16, doc.GetAllocator());

		rapidjson::Value jstrcount;
		jstrcount.SetString("count", doc.GetAllocator());
		rapidjson::Value jvalcount;
		jvalcount.SetInt(count());
		val.AddMember(jstrcount, jvalcount, doc.GetAllocator());

		rapidjson::Value jstrdata;
		jstrdata.SetString("data", doc.GetAllocator());
		rapidjson::Value jvaldata;
		jvaldata.SetArray();
		for (int i = 0; i < count(); ++i) {
			const Frame* frame = get(i);

			rapidjson::Value jframe;
			jframe.SetObject();

			rapidjson::Value jstranchor;
			jstranchor.SetString("anchor", doc.GetAllocator());
			rapidjson::Value jvalanchor;
			jvalanchor.SetObject();
			{
				rapidjson::Value jstrax, jstray;
				jstrax.SetString("x", doc.GetAllocator());
				jstray.SetString("y", doc.GetAllocator());
				rapidjson::Value jvalax, jvalay;
				jvalax.SetInt(frame->anchor.x);
				jvalay.SetInt(frame->anchor.y);

				jvalanchor.AddMember(jstrax, jvalax, doc.GetAllocator());
				jvalanchor.AddMember(jstray, jvalay, doc.GetAllocator());
			}
			jframe.AddMember(jstranchor, jvalanchor, doc.GetAllocator());

			rapidjson::Value jpixels;
			jpixels.SetString("pixels", doc.GetAllocator());
			rapidjson::Value jvalpixels;
			jvalpixels.SetObject();
			if (!frame->pixels->toJson(jvalpixels, doc))
				continue;
			jframe.AddMember(jpixels, jvalpixels, doc.GetAllocator());

			rapidjson::Value jproperties;
			jproperties.SetString("properties", doc.GetAllocator());
			rapidjson::Value jvalproperties;
			jvalproperties.SetObject();
			if (!frame->properties->toJson(jvalproperties, doc))
				continue;
			jframe.AddMember(jproperties, jvalproperties, doc.GetAllocator());

			jvaldata.PushBack(jframe, doc.GetAllocator());
		}
		val.AddMember(jstrdata, jvaldata, doc.GetAllocator());

		if (!_updateRoutine.empty()) {
			rapidjson::Value jstrroutine;
			jstrroutine.SetString("update_routine", doc.GetAllocator());
			rapidjson::Value jvalroutine;
			jvalroutine.SetString(_updateRoutine.c_str(), doc.GetAllocator());

			val.AddMember(jstrroutine, jvalroutine, doc.GetAllocator());
		}
		if (!_onHitsRoutine.empty()) {
			rapidjson::Value jstrroutine;
			jstrroutine.SetString("on_hits_routine", doc.GetAllocator());
			rapidjson::Value jvalroutine;
			jvalroutine.SetString(_onHitsRoutine.c_str(), doc.GetAllocator());

			val.AddMember(jstrroutine, jvalroutine, doc.GetAllocator());
		}

		return true;
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(Indexed::Ptr palette, const rapidjson::Value &val) override {
		unload();

		if (!val.IsObject())
			return false;

		rapidjson::Value::ConstMemberIterator jis8x16 = val.FindMember("is_8x16");
		if (jis8x16 != val.MemberEnd() && jis8x16->value.IsBool())
			is8x16(jis8x16->value.GetBool());

		rapidjson::Value::ConstMemberIterator jcount = val.FindMember("count");
		if (jcount == val.MemberEnd() || !jcount->value.IsInt())
			return false;
		const int count = jcount->value.GetInt();

		rapidjson::Value::ConstMemberIterator jdata = val.FindMember("data");
		if (jdata == val.MemberEnd() || !jdata->value.IsArray())
			return false;

		rapidjson::Value::ConstArray jframes = jdata->value.GetArray();
		for (rapidjson::SizeType i = 0; i < jframes.Size() && (int)i < count; ++i) {
			const rapidjson::Value &jframe = jframes[i];
			rapidjson::Value::ConstMemberIterator janchor = jframe.FindMember("anchor");
			if (janchor == jframe.MemberEnd() || !janchor->value.IsObject())
				continue;
			const rapidjson::Value &jvalanchor = janchor->value;
			rapidjson::Value::ConstMemberIterator jx = jvalanchor.FindMember("x");
			rapidjson::Value::ConstMemberIterator jy = jvalanchor.FindMember("y");
			if (jx == jvalanchor.MemberEnd() || jy == jvalanchor.MemberEnd())
				continue;
			if (!jx->value.IsInt() || !jy->value.IsInt())
				continue;
			const Math::Vec2i anchor(jx->value.GetInt(), jy->value.GetInt());

			rapidjson::Value::ConstMemberIterator jpixels = jframe.FindMember("pixels");
			if (jpixels == jframe.MemberEnd() || !jpixels->value.IsObject())
				continue;
			Image::Ptr img(Image::create(palette));
			if (!img->fromJson(jpixels->value))
				continue;

			rapidjson::Value::ConstMemberIterator jproperties = jframe.FindMember("properties");
			Image::Ptr props(Image::create(Indexed::Ptr(Indexed::create((int)Math::pow(2, ACTOR_PALETTE_DEPTH)))));
			if (
				jproperties != jframe.MemberEnd() && jproperties->value.IsObject() &&
				props->fromJson(jproperties->value) && props->paletted() == ACTOR_PALETTE_DEPTH
			) {
				// Do nothing.
			} else {
				props->fromBlank(img->width() / GBBASIC_TILE_SIZE, img->height() / GBBASIC_TILE_SIZE, ACTOR_PALETTE_DEPTH);
			}

			add(img, props, anchor);
		}

		rapidjson::Value::ConstMemberIterator jupdateroutine = val.FindMember("update_routine");
		if (jupdateroutine != val.MemberEnd() && jupdateroutine->value.IsString()) {
			_updateRoutine = jupdateroutine->value.GetString();
		} else {
			_updateRoutine.clear();
		}
		rapidjson::Value::ConstMemberIterator juonhitsoutine = val.FindMember("on_hits_routine");
		if (juonhitsoutine != val.MemberEnd() && juonhitsoutine->value.IsString()) {
			_onHitsRoutine = juonhitsoutine->value.GetString();
		} else {
			_onHitsRoutine.clear();
		}

		return true;
	}
	virtual bool fromJson(Indexed::Ptr palette, const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(palette, jval);
	}
};

Actor::Slice::Slice() {
}

Actor::Slice::Slice(Image::Ptr img, const Math::Vec2i &off) :
	image(img),
	offset(off)
{
	GBBASIC_ASSERT(img && "Wrong data.");

	hash = image->hash();
}

bool Actor::Slice::operator == (const Slice &other) const {
	return equals(other);
}

bool Actor::Slice::operator != (const Slice &other) const {
	return !equals(other);
}

bool Actor::Slice::equals(const Slice &other) const {
	if (hash != other.hash)
		return false;

	return image->compare(other.image.get()) == 0;
}

Actor::Shadow::Ref::Ref() {
}

Actor::Shadow::Ref::Ref(int idx, const Math::Vec2i &off, const Math::Vec2i &s, int props) :
	index(idx), offset(off), step(s), properties(props)
{
}

bool Actor::Shadow::Ref::operator == (const Ref &other) const {
	return equals(other);
}

bool Actor::Shadow::Ref::operator != (const Ref &other) const {
	return !equals(other);
}

bool Actor::Shadow::Ref::equals(const Ref &other) const {
	if (index != other.index)
		return false;
	if (offset != other.offset)
		return false;
	if (step != other.step)
		return false;
	if (properties != other.properties)
		return false;

	return true;
}

Actor::Shadow::Shadow() {
}

bool Actor::Shadow::operator == (const Shadow &other) const {
	return equals(other);
}

bool Actor::Shadow::operator != (const Shadow &other) const {
	return !equals(other);
}

bool Actor::Shadow::equals(const Shadow &other) const {
	if (refs.size() != other.refs.size())
		return false;

	for (int i = 0; i < (int)refs.size(); ++i) {
		if (refs[i] != other.refs[i])
			return false;
	}

	return true;
}

Actor::Frame::Frame() {
}

Actor::Frame::Frame(Image::Ptr img, Image::Ptr props, const Math::Vec2i &anchor_) :
	pixels(img),
	properties(props),
	anchor(anchor_)
{
}

Actor::Frame::Frame(const Frame &other) {
	pixels = other.pixels;
	texture = other.texture;
	properties = other.properties;
	anchor = other.anchor;
	// Ignore: `slices`.
}

bool Actor::Frame::operator < (const Frame &other) const {
	return compare(other) < 0;
}

bool Actor::Frame::operator > (const Frame &other) const {
	return compare(other) > 0;
}

size_t Actor::Frame::hash(void) const {
	size_t result = 0;

	result = Math::hash(result, pixels->hash());

	result = Math::hash(result, properties->hash());

	result = Math::hash(result, anchor.x, anchor.y);

	// `slices` doesn't count.

	return result;
}

int Actor::Frame::compare(const Frame &other) const {
	if (!pixels && other.pixels)
		return -1;
	else if (pixels && !other.pixels)
		return 1;
	if (pixels && other.pixels) {
		const int ret = pixels->compare(other.pixels.get());
		if (ret)
			return ret;
	}

	if (!properties && other.properties)
		return -1;
	else if (properties && !other.properties)
		return 1;
	if (properties && other.properties) {
		const int ret = properties->compare(other.properties.get());
		if (ret)
			return ret;
	}

	if (anchor.compare(other.anchor) < 0)
		return -1;
	if (anchor.compare(other.anchor) > 0)
		return 1;

	// `slices` doesn't count.

	return 0;
}

int Actor::Frame::width(void) const {
	if (!pixels)
		return 0;

	return pixels->width();
}

int Actor::Frame::height(void) const {
	if (!pixels)
		return 0;

	return pixels->height();
}

Math::Vec2i Actor::Frame::count(const Actor* actor) const {
	const bool is8x16 = actor->is8x16();
	int dx = 0;
	int dy = 0;
	if (is8x16) {
		dx = 8;
		dy = 16;
	} else {
		dx = GBBASIC_TILE_SIZE;
		dy = GBBASIC_TILE_SIZE;
	}

	return count(Math::Vec2i(dx, dy));
}

Math::Vec2i Actor::Frame::count(const Math::Vec2i &tileSize) const {
	const std::div_t divx = std::div(width(), tileSize.x);
	const std::div_t divy = std::div(height(), tileSize.y);
	const int x = divx.quot + (divx.rem ? 1 : 0);
	const int y = divy.quot + (divy.rem ? 1 : 0);

	return Math::Vec2i(x, y);
}

bool Actor::Frame::get(int x, int y, int &index) const {
	if (!pixels)
		return false;

	return pixels->get(x, y, index);
}

bool Actor::Frame::set(int x, int y, int index) {
	if (!pixels)
		return false;

	return pixels->set(x, y, index);
}

bool Actor::Frame::getProperty(int x, int y, int &index) const {
	if (!properties)
		return false;

	return properties->get(x, y, index);
}

bool Actor::Frame::setProperty(int x, int y, int index) {
	if (!properties)
		return false;

	return properties->set(x, y, index);
}

Math::Recti Actor::Frame::computeContentRect(void) const {
	Math::Recti result;
	if (pixels) {
		int xMin = std::numeric_limits<int>::max();
		int yMin = std::numeric_limits<int>::max();
		int xMax = std::numeric_limits<int>::min();
		int yMax = std::numeric_limits<int>::min();
		for (int j = 0; j < (int)pixels->height(); ++j) {
			for (int i = 0; i < (int)pixels->width(); ++i) {
				int index = 0;
				pixels->get(i, j, index);
				if (index == 0)
					continue;

				if (i < xMin) xMin = i;
				if (j < yMin) yMin = j;
				if (i > xMax) xMax = i;
				if (j > yMax) yMax = j;
			}
		}

		result = Math::Recti(xMin, yMin, xMax, yMax);
	}

	return result;
}

void Actor::Frame::cleanup(void) {
	pixels->cleanup();
	texture = nullptr;
}

bool Actor::Frame::touch(Renderer* rnd) {
	if (texture)
		return true;
	if (!pixels)
		return false;

	texture = Texture::Ptr(Texture::create());
	texture->fromImage(rnd, Texture::STATIC, pixels.get(), Texture::NEAREST);
	texture->blend(Texture::BLEND);

	return true;
}

bool Actor::Frame::touch(Renderer* rnd, PaletteResolver resolve) {
	if (texture)
		return true;
	if (!pixels)
		return false;

	Image* tmp = nullptr;
	if (!pixels->clone(&tmp) || !tmp)
		return false;

	tmp->realize(resolve);

	texture = Texture::Ptr(Texture::create());
	texture->fromImage(rnd, Texture::STATIC, tmp, Texture::NEAREST);
	texture->blend(Texture::BLEND);

	Image::destroy(tmp);

	return true;
}

int Actor::Frame::slice(const Actor* actor) {
	// Prepare.
	GBBASIC_ASSERT(slices.empty() && "Impossible.");

	slices.clear();

	// Slice the frame.
	const bool is8x16 = actor->is8x16();
	const Compactors compactor = actor->compactor();
	if (is8x16) { // Slice into 8x16 tiles.
		// Slice pixels into matrice.
		constexpr const int _8 = 8;
		const Math::Vec2i m = count(Math::Vec2i(_8, _8));
		Matrice<Slice> matrice(m.x, m.y);
		for (int j = 0; j < m.y; ++j) {
			for (int i = 0; i < m.x; ++i) {
				const int x = i * _8;
				const int y = j * _8;

				Image::Ptr tmp(Image::create(pixels->palette()));
				tmp->fromBlank(_8, _8, pixels->paletted());
				pixels->blit(tmp.get(), 0, 0, _8, _8, x, y);

				const Math::Vec2i off(x, y);
				const Slice slice(tmp, off);
				matrice[i][j] = slice;
			}
		}

		// Determine the slices.
		const int dx = 8;
		const int dy = 16;
		for (int i = 0; i < matrice.width(); ++i) {
			int j = 0;
			while (j < matrice.height()) {
				const Slice &slice0 = matrice[i][j];
				if (compactor == Compactors::NON_BLANK && slice0.image->blank()) { // Ignore a blank tile.
					++j;

					continue;
				}

				const int x = i * _8;
				const int y = j * _8;
				const int step = j < matrice.height() - 1 ? 2 : 1;

				Image::Ptr tmp(Image::create(slice0.image->palette()));
				tmp->fromBlank(dx, dy, slice0.image->paletted());
				slice0.image->blit(tmp.get(), 0, 0, _8, _8, 0, 0);
				if (step == 2) {
					const Slice &slice1 = matrice[i][j + 1];
					if (!slice1.image->blank())
						slice1.image->blit(tmp.get(), 0, 8, _8, _8, 0, 0);
				}

				const Math::Vec2i off(x, y);
				const Slice slice(tmp, off);
				slices.push_back(slice);

				j += step;
			}
		}

		// Sort as left to right, top-down.
		std::sort(
			slices.begin(), slices.end(),
			[] (const Slice &l, const Slice &r) -> bool {
				if (l.offset.y < r.offset.y)
					return true;
				if (l.offset.y > r.offset.y)
					return false;

				if (l.offset.x < r.offset.x)
					return true;
				if (l.offset.x > r.offset.x)
					return false;

				return false;
			}
		);
	} else { // Slice into 8x8 tiles.
		const int dx = GBBASIC_TILE_SIZE;
		const int dy = GBBASIC_TILE_SIZE;
		const Math::Vec2i n = count(actor);
		for (int j = 0; j < n.y; ++j) {
			for (int i = 0; i < n.x; ++i) {
				const int x = i * dx;
				const int y = j * dy;

				Image::Ptr tmp(Image::create(pixels->palette()));
				tmp->fromBlank(dx, dy, pixels->paletted());
				pixels->blit(tmp.get(), 0, 0, dx, dy, x, y);
				if (tmp->blank())
					continue;

				const Math::Vec2i off(x, y);
				const Slice slice(tmp, off);
				slices.push_back(slice);
			}
		}
	}

	// Finish.
	return (int)slices.size();
}

Actor* Actor::create(void) {
	ActorImpl* result = new ActorImpl();

	return result;
}

void Actor::destroy(Actor* ptr) {
	ActorImpl* impl = static_cast<ActorImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
