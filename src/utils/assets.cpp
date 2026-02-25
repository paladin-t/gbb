/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "assets.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "text.h"
#include "../compiler/compiler.h"
#include "../../lib/jpath/jpath.hpp"
#include "../../lib/rapidfuzz_cpp/rapidfuzz/fuzz.hpp"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ASSET_FUZZY_MATCHING_SCORE_THRESHOLD
#	define ASSET_FUZZY_MATCHING_SCORE_THRESHOLD 0.75
#endif /* ASSET_FUZZY_MATCHING_SCORE_THRESHOLD */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

static void assetsRaiseWarningOrError(const char* fmt, bool isWarning, WarningOrErrorHandler onWarningOrError) {
	if (!onWarningOrError)
		return;

	onWarningOrError(fmt, isWarning);
}
static void assetsRaiseWarningOrError(const char* fmt, int i, bool isWarning, WarningOrErrorHandler onWarningOrError) {
	if (!onWarningOrError)
		return;

	const std::string page = Text::toPageNumber(i);
	const std::string msg = Text::format(fmt, { page });
	onWarningOrError(msg.c_str(), isWarning);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Shared structures shared between the compiler and VM
*/

glyph_option_t::glyph_option_t() :
	two_bits_per_pixel(GBBASIC_FONT_DEFAULT_IS_2BPP),
	full_word(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD),
	full_word_for_non_ascii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII),
	inverted(false),
	reserved1(false),
	reserved2(false),
	reserved3(false),
	reserved4(false)
{
}

glyph_option_t::glyph_option_t(bool tbpp) :
	two_bits_per_pixel(tbpp),
	full_word(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD),
	full_word_for_non_ascii(GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII),
	inverted(false),
	reserved1(false),
	reserved2(false),
	reserved3(false),
	reserved4(false)
{
}

glyph_option_t::glyph_option_t(bool tbpp, bool fullword, bool fullword4all, bool inverted_) :
	two_bits_per_pixel(tbpp),
	full_word(fullword),
	full_word_for_non_ascii(fullword4all),
	inverted(inverted_),
	reserved1(false),
	reserved2(false),
	reserved3(false),
	reserved4(false)
{
}

glyph_t::glyph_t() {
}

glyph_t::glyph_t(UInt8 bank_, UInt16 ptr_, UInt8 sz) : bank(bank_), ptr(ptr_), size(sz) {
}

glyph_t::glyph_t(UInt16 cp) : codepoint(cp) {
}

upoint8_t::upoint8_t() {
}

bool upoint8_t::operator == (const upoint8_t &other) const {
	return compare(other) == 0;
}

bool upoint8_t::operator != (const upoint8_t &other) const {
	return compare(other) != 0;
}

bool upoint8_t::operator < (const upoint8_t &other) const {
	return compare(other) < 0;
}

bool upoint8_t::operator > (const upoint8_t &other) const {
	return compare(other) > 0;
}

int upoint8_t::compare(const upoint8_t &other) const {
	if (x < other.x)
		return -1;
	else if (x > other.x)
		return 1;

	if (y < other.y)
		return -1;
	else if (y > other.y)
		return 1;

	return 0;
}

bool upoint8_t::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	Jpath::set(doc, val, x, "x");

	Jpath::set(doc, val, y, "y");

	return true;
}

bool upoint8_t::fromJson(const rapidjson::Value &val) {
	Jpath::get(val, x, "x");

	Jpath::get(val, y, "y");

	return true;
}

point16_t::point16_t() {
}

bool point16_t::operator == (const point16_t &other) const {
	return compare(other) == 0;
}

bool point16_t::operator != (const point16_t &other) const {
	return compare(other) != 0;
}

bool point16_t::operator < (const point16_t &other) const {
	return compare(other) < 0;
}

bool point16_t::operator > (const point16_t &other) const {
	return compare(other) > 0;
}

int point16_t::compare(const point16_t &other) const {
	if (x < other.x)
		return -1;
	else if (x > other.x)
		return 1;

	if (y < other.y)
		return -1;
	else if (y > other.y)
		return 1;

	return 0;
}

bool point16_t::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	Jpath::set(doc, val, x, "x");

	Jpath::set(doc, val, y, "y");

	return true;
}

bool point16_t::fromJson(const rapidjson::Value &val) {
	Jpath::get(val, x, "x");

	Jpath::get(val, y, "y");

	return true;
}

upoint16_t::upoint16_t() {
}

bool upoint16_t::operator == (const upoint16_t &other) const {
	return compare(other) == 0;
}

bool upoint16_t::operator != (const upoint16_t &other) const {
	return compare(other) != 0;
}

bool upoint16_t::operator < (const upoint16_t &other) const {
	return compare(other) < 0;
}

bool upoint16_t::operator > (const upoint16_t &other) const {
	return compare(other) > 0;
}

int upoint16_t::compare(const upoint16_t &other) const {
	if (x < other.x)
		return -1;
	else if (x > other.x)
		return 1;

	if (y < other.y)
		return -1;
	else if (y > other.y)
		return 1;

	return 0;
}

bool upoint16_t::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	Jpath::set(doc, val, x, "x");

	Jpath::set(doc, val, y, "y");

	return true;
}

bool upoint16_t::fromJson(const rapidjson::Value &val) {
	Jpath::get(val, x, "x");

	Jpath::get(val, y, "y");

	return true;
}

boundingbox_t::boundingbox_t() {
}

boundingbox_t::boundingbox_t(Int8 l, Int8 r, Int8 t, Int8 b) :
	left(l), right(r), top(t), bottom(b)
{
}

bool boundingbox_t::operator == (const boundingbox_t &other) const {
	return compare(other) == 0;
}

bool boundingbox_t::operator != (const boundingbox_t &other) const {
	return compare(other) != 0;
}

bool boundingbox_t::operator < (const boundingbox_t &other) const {
	return compare(other) < 0;
}

bool boundingbox_t::operator > (const boundingbox_t &other) const {
	return compare(other) > 0;
}

int boundingbox_t::compare(const boundingbox_t &other) const {
	if (left < other.left)
		return -1;
	else if (left > other.left)
		return 1;

	if (right < other.right)
		return -1;
	else if (right > other.right)
		return 1;

	if (top < other.top)
		return -1;
	else if (top > other.top)
		return 1;

	if (bottom < other.bottom)
		return -1;
	else if (bottom > other.bottom)
		return 1;

	return 0;
}

int boundingbox_t::width(void) const {
	const Math::Recti rect(left, top, right, bottom);

	return rect.width();
}

int boundingbox_t::height(void) const {
	const Math::Recti rect(left, top, right, bottom);

	return rect.height();
}

bool boundingbox_t::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	Jpath::set(doc, val, left, "left");

	Jpath::set(doc, val, right, "right");

	Jpath::set(doc, val, top, "top");

	Jpath::set(doc, val, bottom, "bottom");

	return true;
}

bool boundingbox_t::fromJson(const rapidjson::Value &val) {
	Jpath::get(val, left, "left");

	Jpath::get(val, right, "right");

	Jpath::get(val, top, "top");

	Jpath::get(val, bottom, "bottom");

	return true;
}

animation_t::animation_t() {
}

bool animation_t::operator == (const animation_t &other) const {
	return compare(other) == 0;
}

bool animation_t::operator != (const animation_t &other) const {
	return compare(other) != 0;
}

bool animation_t::operator < (const animation_t &other) const {
	return compare(other) < 0;
}

bool animation_t::operator > (const animation_t &other) const {
	return compare(other) > 0;
}

int animation_t::compare(const animation_t &other) const {
	if (begin < other.begin)
		return -1;
	else if (begin > other.begin)
		return 1;

	if (end < other.end)
		return -1;
	else if (end > other.end)
		return 1;

	return 0;
}

bool animation_t::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	Jpath::set(doc, val, begin, "begin");

	Jpath::set(doc, val, end, "end");

	return true;
}

bool animation_t::fromJson(const rapidjson::Value &val) {
	Jpath::get(val, begin, "begin");

	Jpath::get(val, end, "end");

	return true;
}

actor_t::actor_t() {
	enabled = true;
	hidden = false;
	pinned = false;
	persistent = false;
	following = false;
}

projectile_t::projectile_t() {
}

active_t::active_t() {
}

bool active_t::operator == (const active_t &other) const {
	return compare(other) == 0;
}

bool active_t::operator != (const active_t &other) const {
	return compare(other) != 0;
}

bool active_t::operator < (const active_t &other) const {
	return compare(other) < 0;
}

bool active_t::operator > (const active_t &other) const {
	return compare(other) > 0;
}

size_t active_t::hash(void) const {
	size_t result = 0;

	result = Math::hash(
		result,

		// `actor_t`.
		enabled,
		hidden,
		pinned,
		persistent,
		following,
		direction,
		behaviour,

		// `projectile_t`.
		life_time,
		initial_offset,

		// `active_t`.
		// Calculate `bounds` later.
		sprite_bank,
		sprite_frames,
		animation_interval,
		// Calculate `animations` later.
		move_speed,
		collision_group
	);

	result = Math::hash(
		result,
		bounds.left,
		bounds.right,
		bounds.top,
		bounds.bottom
	);

	for (int i = 0; i < ASSETS_ACTOR_MAX_ANIMATIONS; ++i) {
		const animation_t &a = animations[i];
		result = Math::hash(
			result,
			a.begin,
			a.end
		);
	}

	return result;
}

int active_t::compare(const active_t &other) const {
	// `actor_t`.
	if (!enabled && other.enabled)
		return -1;
	if (enabled && !other.enabled)
		return 1;

	if (!hidden && other.hidden)
		return -1;
	else if (hidden && !other.hidden)
		return 1;

	if (!pinned && other.pinned)
		return -1;
	else if (pinned && !other.pinned)
		return 1;

	if (!persistent && other.persistent)
		return -1;
	else if (persistent && !other.persistent)
		return 1;

	if (!following && other.following)
		return -1;
	else if (following && !other.following)
		return 1;

	if (direction < other.direction)
		return -1;
	else if (direction > other.direction)
		return 1;

	if (behaviour < other.behaviour)
		return -1;
	else if (behaviour > other.behaviour)
		return 1;


	// `projectile_t`.
	if (life_time < other.life_time)
		return -1;
	else if (life_time > other.life_time)
		return 1;

	if (initial_offset < other.initial_offset)
		return -1;
	else if (initial_offset > other.initial_offset)
		return 1;


	// `active_t`.
	if (bounds < other.bounds)
		return -1;
	else if (bounds > other.bounds)
		return 1;

	if (sprite_bank < other.sprite_bank)
		return -1;
	else if (sprite_bank > other.sprite_bank)
		return 1;

	if (sprite_frames < other.sprite_frames)
		return -1;
	else if (sprite_frames > other.sprite_frames)
		return 1;

	if (animation_interval < other.animation_interval)
		return -1;
	else if (animation_interval > other.animation_interval)
		return 1;

	for (int i = 0; i < ASSETS_ACTOR_MAX_ANIMATIONS; ++i) {
		const animation_t &al = animations[i];
		const animation_t &ar = other.animations[i];
		if (al < ar)
			return -1;
		else if (al > ar)
			return 1;
	}

	if (move_speed < other.move_speed)
		return -1;
	else if (move_speed > other.move_speed)
		return 1;

	if (collision_group < other.collision_group)
		return -1;
	else if (collision_group > other.collision_group)
		return 1;

	return 0;
}

bool active_t::toJson(rapidjson::Value &val, rapidjson::Document &doc, BehaviourSerializer serializeBehaviour) const {
	// `actor_t`.
	Jpath::set(doc, val, enabled, "enabled");

	Jpath::set(doc, val, hidden, "hidden");

	Jpath::set(doc, val, pinned, "pinned");

	Jpath::set(doc, val, persistent, "persistent");

	Jpath::set(doc, val, following, "following");

	Jpath::set(doc, val, direction, "direction");

	const std::string behaviourStr = serializeBehaviour(behaviour);
	Jpath::set(doc, val, behaviourStr, "behaviour");

	// `projectile_t`.
	Jpath::set(doc, val, life_time, "life_time");

	Jpath::set(doc, val, initial_offset, "initial_offset");

	// `active_t`.
	Jpath::set(doc, val, Jpath::ANY(), "bounds");
	rapidjson::Value* bnds = nullptr;
	Jpath::get(val, bnds, "bounds");
	bounds.toJson(*bnds, doc);

	Jpath::set(doc, val, sprite_bank, "sprite_bank");

	Jpath::set(doc, val, sprite_frames, "sprite_frames");

	Jpath::set(doc, val, animation_interval, "animation_interval");

	for (int i = 0; i < GBBASIC_COUNTOF(animations); ++i) {
		Jpath::set(doc, val, Jpath::ANY(), "animations", i);
		rapidjson::Value* anim = nullptr;
		Jpath::get(val, anim, "animations", i);
		animations[i].toJson(*anim, doc);
	}

	Jpath::set(doc, val, move_speed, "move_speed");

	Jpath::set(doc, val, collision_group, "collision_group");

	return true;
}

bool active_t::fromJson(const rapidjson::Value &val, BehaviourParser parseBehaviour) {
	// `actor_t`.
	bool boolean = false;
	if (Jpath::get(val, boolean, "enabled"))
		enabled = boolean;
	else
		enabled = false;

	if (Jpath::get(val, boolean, "hidden"))
		hidden = boolean;
	else
		hidden = false;

	if (Jpath::get(val, boolean, "pinned"))
		pinned = boolean;
	else
		pinned = false;

	if (Jpath::get(val, boolean, "persistent"))
		persistent = boolean;
	else
		persistent = false;

	if (Jpath::get(val, boolean, "following"))
		following = boolean;
	else
		following = false;

	Jpath::get(val, direction, "direction");

	std::string behaviourStr;
	Jpath::get(val, behaviourStr, "behaviour");
	behaviour = (UInt8)parseBehaviour(behaviourStr);

	// `projectile_t`.
	Jpath::get(val, life_time, "life_time");

	Jpath::get(val, initial_offset, "initial_offset");

	// `active_t`.
	rapidjson::Value* bnds = nullptr;
	Jpath::get(val, bnds, "bounds");
	if (bnds)
		bounds.fromJson(*bnds);

	Jpath::get(val, sprite_bank, "sprite_bank");

	Jpath::get(val, sprite_frames, "sprite_frames");

	Jpath::get(val, animation_interval, "animation_interval");

	for (int i = 0; i < GBBASIC_COUNTOF(animations); ++i) {
		rapidjson::Value* anim = nullptr;
		Jpath::get(val, anim, "animations", i);
		if (anim)
			animations[i].fromJson(*anim);
	}

	Jpath::get(val, move_speed, "move_speed");

	Jpath::get(val, collision_group, "collision_group");

	return true;
}

scene_t::scene_t() {
	is_16x16_grid = false;
	is_16x16_player = false;
}

bool scene_t::operator == (const scene_t &other) const {
	return compare(other) == 0;
}

bool scene_t::operator != (const scene_t &other) const {
	return compare(other) != 0;
}

bool scene_t::operator < (const scene_t &other) const {
	return compare(other) < 0;
}

bool scene_t::operator > (const scene_t &other) const {
	return compare(other) > 0;
}

size_t scene_t::hash(void) const {
	size_t result = 0;

	result = Math::hash(
		is_16x16_grid,
		// `is_16x16_player` doesn't count.
		gravity,
		jump_gravity,
		jump_max_count,
		jump_max_ticks,
		climb_velocity

		// Calculate `camera_position` later.
		// Calculate `camera_deadzone` later.
	);

	result = Math::hash(
		result,
		camera_position.x,
		camera_position.y
	);

	result = Math::hash(
		result,
		camera_deadzone.x,
		camera_deadzone.y
	);

	return result;
}

int scene_t::compare(const scene_t &other) const {
	if (!is_16x16_grid && other.is_16x16_grid)
		return -1;
	else if (is_16x16_grid && !other.is_16x16_grid)
		return 1;

	// `is_16x16_player` doesn't count.

	if (gravity < other.gravity)
		return -1;
	else if (gravity > other.gravity)
		return 1;

	if (jump_gravity < other.jump_gravity)
		return -1;
	else if (jump_gravity > other.jump_gravity)
		return 1;

	if (jump_max_count < other.jump_max_count)
		return -1;
	else if (jump_max_count > other.jump_max_count)
		return 1;

	if (jump_max_ticks < other.jump_max_ticks)
		return -1;
	else if (jump_max_ticks > other.jump_max_ticks)
		return 1;

	if (climb_velocity < other.climb_velocity)
		return -1;
	else if (climb_velocity > other.climb_velocity)
		return 1;

	if (camera_position < other.camera_position)
		return -1;
	else if (camera_position > other.camera_position)
		return 1;

	if (camera_deadzone < other.camera_deadzone)
		return -1;
	else if (camera_deadzone > other.camera_deadzone)
		return 1;

	return 0;
}

bool scene_t::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	Jpath::set(doc, val, is_16x16_grid, "is_16x16_grid");

	// `is_16x16_player` doesn't count.

	Jpath::set(doc, val, gravity, "gravity");

	Jpath::set(doc, val, jump_gravity, "jump_gravity");

	Jpath::set(doc, val, jump_max_count, "jump_max_count");

	Jpath::set(doc, val, jump_max_ticks, "jump_max_ticks");

	Jpath::set(doc, val, climb_velocity, "climb_velocity");

	Jpath::set(doc, val, Jpath::ANY(), "camera_position");
	rapidjson::Value* campos = nullptr;
	Jpath::get(val, campos, "camera_position");
	camera_position.toJson(*campos, doc);

	Jpath::set(doc, val, Jpath::ANY(), "camera_deadzone");
	rapidjson::Value* camdeadzone = nullptr;
	Jpath::get(val, camdeadzone, "camera_deadzone");
	camera_deadzone.toJson(*camdeadzone, doc);

	return true;
}

bool scene_t::fromJson(const rapidjson::Value &val) {
	bool boolean = false;
	if (Jpath::get(val, boolean, "is_16x16_grid"))
		is_16x16_grid = boolean;
	else
		is_16x16_grid = false;

	// `is_16x16_player` doesn't count.

	Jpath::get(val, gravity, "gravity");

	Jpath::get(val, jump_gravity, "jump_gravity");

	Jpath::get(val, jump_max_count, "jump_max_count");

	Jpath::get(val, jump_max_ticks, "jump_max_ticks");

	Jpath::get(val, climb_velocity, "climb_velocity");

	rapidjson::Value* campos = nullptr;
	Jpath::get(val, campos, "camera_position");
	if (campos)
		camera_position.fromJson(*campos);

	rapidjson::Value* camdeadzone = nullptr;
	Jpath::get(val, camdeadzone, "camera_deadzone");
	if (camdeadzone)
		camera_deadzone.fromJson(*camdeadzone);

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Semaphore
*/

Semaphore::Semaphore() {
}

Semaphore::Semaphore(WaitHandler h, StatusPtr working) :
	_waitHandler(h),
	_working(working) {
}

bool Semaphore::working(void) {
	if (!_working)
		return false;

	if (*_working)
		return true;

	_working = nullptr;

	return false;
}

void Semaphore::wait(void) {
	if (!_waitHandler)
		return;

	if (!_working)
		return;

	while (*_working)
		_waitHandler();

	_working = nullptr;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Base assets
*/

unsigned BaseAssets::Versionable::increaseRevision(void) {
	++revision;

	return revision;
}

bool BaseAssets::Invalidatable::isRefRevisionChanged(const Versionable* versionable) const {
	return latestRefRevision != versionable->revision;
}

unsigned BaseAssets::Invalidatable::synchronizeRefRevision(const Versionable* versionable) {
	latestRefRevision = versionable->revision;

	return latestRefRevision;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Palette assets
*/

PaletteAssets::Entry::Entry() {
	data = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
}

bool PaletteAssets::Entry::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!data->toJson(val, doc))
		return false;

	return true;
}

bool PaletteAssets::Entry::fromJson(const rapidjson::Value &val) {
	if (!data->fromJson(val))
		return false;

	return true;
}

PaletteAssets::PaletteAssets() {
	const Colour DEFAULT_COLORS_ARRAY[] = INDEXED_DEFAULT_COLORS;

	int k = 0;
	for (int j = 0; j < (int)Math::pow(2, GBBASIC_PALETTE_COLOR_DEPTH) / GBBASIC_PALETTE_PER_GROUP_COUNT; ++j) {
		Entry p;
		for (int i = 0; i < GBBASIC_PALETTE_PER_GROUP_COUNT; ++i)
			p.data->set(i, &DEFAULT_COLORS_ARRAY[k++]);
		add(p);
	}
}

bool PaletteAssets::equals(const PaletteAssets &other) const {
	const int n = Math::min((int)entries.size(), (int)other.entries.size());
	for (int i = 0; i < n; ++i) {
		const Entry &le = entries[i];
		const Entry &re = other.entries[i];
		const Indexed::Ptr &li = le.data;
		const Indexed::Ptr &ri = re.data;
		const int m = Math::min(li->count(), ri->count());
		for (int j = 0; j < m; ++j) {
			Colour lc, rc;
			li->get(j, lc);
			ri->get(j, rc);
			if (lc != rc)
				return false;
		}
	}

	return true;
}

void PaletteAssets::copyFrom(const PaletteAssets &other, Groups* groups) {
	if (groups)
		groups->clear();

	const int n = Math::min((int)entries.size(), (int)other.entries.size());
	for (int i = 0; i < n; ++i) {
		const Entry &le = entries[i];
		const Entry &re = other.entries[i];
		const Indexed::Ptr &li = le.data;
		const Indexed::Ptr &ri = re.data;
		const int m = Math::min(li->count(), ri->count());
		for (int j = 0; j < m; ++j) {
			Colour lc, rc;
			li->get(j, lc);
			ri->get(j, rc);
			if (lc != rc) {
				li->set(j, &rc);
				if (groups) {
					if (std::find(groups->begin(), groups->end(), i) == groups->end())
						groups->push_back(i);
				}
			}
		}
	}
}

bool PaletteAssets::empty(void) const {
	return entries.empty();
}

int PaletteAssets::count(void) const {
	return (int)entries.size();
}

bool PaletteAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool PaletteAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

void PaletteAssets::clear(void) {
	entries.clear();
}

const PaletteAssets::Entry* PaletteAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

PaletteAssets::Entry* PaletteAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

Bytes::Ptr PaletteAssets::serializeBackgroundPalettes(void) const {
	Bytes::Ptr result(Bytes::create());
	for (int i = 0; i < count() && i < 8; ++i) {
		const PaletteAssets::Entry* entry = get(i);
		if (entry) {
			const Indexed::Ptr &palette = entry->data;
			for (int j = 0; j < palette->count(); ++j) {
				Colour color;
				palette->get(j, color);
				if (color.a == 0)
					color.fromRGBA(0xffffffff);
				const UInt16 rgb555 = color.toRGB555();
				result->writeUInt16(rgb555);
			}
		} else {
			for (int j = 0; j < GBBASIC_PALETTE_PER_GROUP_COUNT; ++j) {
				Colour color;
				const UInt16 rgb555 = color.toRGB555();
				result->writeUInt16(rgb555);
			}
		}
	}

	return result;
}

Bytes::Ptr PaletteAssets::serializeSpritePalettes(void) const {
	Bytes::Ptr result(Bytes::create());
	for (int i = 8; i < count() && i < 16; ++i) {
		const PaletteAssets::Entry* entry = get(i);
		if (entry) {
			const Indexed::Ptr &palette = entry->data;
			for (int j = 0; j < palette->count(); ++j) {
				Colour color;
				palette->get(j, color);
				if (color.a == 0)
					color.fromRGBA(0xffffffff);
				const UInt16 rgb555 = color.toRGB555();
				result->writeUInt16(rgb555);
			}
		} else {
			for (int j = 0; j < GBBASIC_PALETTE_PER_GROUP_COUNT; ++j) {
				Colour color;
				const UInt16 rgb555 = color.toRGB555();
				result->writeUInt16(rgb555);
			}
		}
	}

	return result;
}

bool PaletteAssets::serializeBasic(std::string &val) const {
	// Prepare.
	val.clear();

	// Serialize the palette.
	for (int i = 0; i < count(); ++i) {
		const Entry* entry = get(i);
		for (int j = 0; j < entry->data->count(); ++j) {
			Colour col;
			entry->data->get(j, col);

			val += "palette ";
			val += i < 8 ? "MAP_LAYER, " : "SPRITE_LAYER, ";
			val += Text::toString(i % 8) + ", ";
			val += Text::toString(j) + ", ";
			val += "rgb(";
			val += Text::toString(col.r) + ", ";
			val += Text::toString(col.g) + ", ";
			val += Text::toString(col.b);
			val += ")";
			val += "\n";
		}
	}

	// Finish.
	return true;
}

bool PaletteAssets::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the colors.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "colors");
	rapidjson::Value* colors = nullptr;
	Jpath::get(doc, colors, "colors");
	for (int i = 0; i < count(); ++i) {
		const Entry* entry = get(i);

		rapidjson::Value val_;
		if (!entry->toJson(val_, doc)) {
			assetsRaiseWarningOrError("Cannot serialize palette at page {0}.", i, true, onWarningOrError);

			continue;
		}

		colors->PushBack(val_, doc.GetAllocator());
	}

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert palette to JSON.", false, onWarningOrError);

		return false;
	}
	if (!json->toString(val, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool PaletteAssets::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Prepare.
	clear();

	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert font from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the colors.
	const rapidjson::Value* colors = nullptr;
	if (!Jpath::get(doc, colors, "colors")) {
		assetsRaiseWarningOrError("Cannot find \"colors\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!colors || !colors->IsArray()) {
		assetsRaiseWarningOrError("Invalid \"colors\" entry.", false, onWarningOrError);

		return false;
	}

	for (auto it = colors->Begin(); it != colors->End(); ++it) {
		Entry p;

		const rapidjson::Value &val_ = *it;
		if (!p.fromJson(val_))
			continue;

		add(p);
	}

	// Finish.
	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Font assets
*/

GlyphTable::Entry::Entry() {
}

GlyphTable::Entry::Entry(Font::Codepoint cp) : codepoint(cp) {
}

GlyphTable::Entry::Entry(Font::Codepoint cp, int ref) : codepoint(cp), refCount(ref) {
}

UInt8 GlyphTable::Entry::size(void) const {
	GBBASIC_ASSERT(width <= GBBASIC_FONT_MAX_SIZE && height <= GBBASIC_FONT_MAX_SIZE && "Wrong data.");

	return (UInt8)(((width - 1) << 4) | ((height - 1) & 0x0f));
}

GlyphTable::GlyphTable() {
}

int GlyphTable::count(void) const {
	return (int)entries.size();
}

bool GlyphTable::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

void GlyphTable::sort(void) {
	std::sort(
		entries.begin(), entries.end(),
		[] (const Entry &l, const Entry &r) -> bool {
			return l.codepoint < r.codepoint;
		}
	);
}

const GlyphTable::Entry* GlyphTable::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

GlyphTable::Entry* GlyphTable::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const GlyphTable::Entry* GlyphTable::find(Font::Codepoint cp) const {
	if (entries.empty())
		return nullptr;

	Entry key;
	key.codepoint = cp;
	void* ptr = std::bsearch(
		&key, &entries.front(), entries.size(), sizeof(Entry),
		[] (const void* l_, const void* r_) -> int {
			const Entry* l = (const Entry*)l_;
			const Entry* r = (const Entry*)r_;
			if (l->codepoint < r->codepoint)
				return -1;
			if (l->codepoint > r->codepoint)
				return 1;

			return 0;
		}
	);
	if (!ptr)
		return nullptr;

	return (const Entry*)ptr;
}

GlyphTable::Entry* GlyphTable::find(Font::Codepoint cp) {
	if (entries.empty())
		return nullptr;

	Entry key;
	key.codepoint = cp;
	void* ptr = std::bsearch(
		&key, &entries.front(), entries.size(), sizeof(Entry),
		[] (const void* l_, const void* r_) -> int {
			const Entry* l = (const Entry*)l_;
			const Entry* r = (const Entry*)r_;
			if (l->codepoint < r->codepoint)
				return -1;
			if (l->codepoint > r->codepoint)
				return 1;

			return 0;
		}
	);
	if (!ptr)
		return nullptr;

	return (Entry*)ptr;
}

FontAssets::Entry::Entry() {
	thresholds[0] = 200;
	thresholds[1] = 100;
	thresholds[2] = 50;
	thresholds[3] = 0;

	substitutionThresholds[0] = 200;
	substitutionThresholds[1] = 100;
	substitutionThresholds[2] = 50;
	substitutionThresholds[3] = 0;
}

FontAssets::Entry::Entry(bool toBeCopied, bool toBeEmbedded, const std::string &dir, const std::string &path_, const Math::Vec2i* size_, const std::string &content_, bool generateObject) {
	GBBASIC_ASSERT(!(toBeCopied && toBeEmbedded) && "Cannot be both to be copied and embedded.");

	std::string calcPath = path_;
	std::string calcDir = dir;
	std::string readingPath = Path::combine(dir.c_str(), path_.c_str());
	const bool pathExists = Path::fileExists(path_.c_str());
	const bool dirPathExists = Path::fileExists(readingPath.c_str());
	if (dir.empty()) {
		if (pathExists) { // Is absolute path.
			readingPath = path_;
		}

		if (toBeCopied)
			copying = Copying::WAITING_FOR_COPYING;
		else if (toBeEmbedded)
			copying = Copying::EMBEDDED;
		else
			copying = Copying::REFERENCED;
	} else {
		if (pathExists && !dirPathExists) { // Is absolute path.
			if (toBeEmbedded) {
				std::string name_;
				std::string ext;
				Path::split(path_, &name_, &ext, nullptr);
				calcPath = name_ + "." + ext;
				calcDir.clear();
				readingPath = path_;
			} else {
				readingPath = path_;
			}
		} else if (dirPathExists) { // Is relative path.
			// Do nothing.
		}

		if (toBeCopied)
			copying = Copying::COPIED;
		else if (toBeEmbedded)
			copying = Copying::EMBEDDED;
		else
			copying = Copying::REFERENCED;
	}
	directory = calcDir;
	path = calcPath;
	encodedTtfData.clear();
	bitmapData.clear();
	size = size_ ? *size_ : Math::Vec2i(-1, GBBASIC_FONT_DEFAULT_SIZE);
	maxSize = Math::Vec2i(GBBASIC_FONT_MAX_SIZE, GBBASIC_FONT_MAX_SIZE);
	trim = true;
	offset = size_ ? 0 : GBBASIC_FONT_DEFAULT_OFFSET;
	isTwoBitsPerPixel = GBBASIC_FONT_DEFAULT_IS_2BPP;
	preferFullWord = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD;
	preferFullWordForNonAscii = GBBASIC_FONT_DEFAULT_PREFER_FULL_WORD_FOR_NON_ASCII;
	enabledEffect = (int)Font::Effects::NONE;
	shadowOffset = 1;
	shadowStrength = 0.3f;
	shadowDirection = (UInt8)Directions::DIRECTION_DOWN_RIGHT;
	outlineOffset = 1;
	outlineStrength = 0.3f;
	thresholds[0] = 200;
	thresholds[1] = 100;
	thresholds[2] = 50;
	thresholds[3] = 0;
	substitutionThresholds[0] = 200;
	substitutionThresholds[1] = 100;
	substitutionThresholds[2] = 50;
	substitutionThresholds[3] = 0;
	content = content_;
	inverted = false;

	isAsset = true;
	if (generateObject) {
		object = Font::Ptr(Font::create());

		if (size_)
			loadBitmap(&bitmapData, object, readingPath, size, 0);
		else
			loadTtf(&encodedTtfData, object, readingPath, size.y, maxSize.x, maxSize.y, 0, trim);
	}
}

size_t FontAssets::Entry::hash(void) const {
	size_t result = 0;

	result = Math::hash(
		result,

		path,
		encodedTtfData,
		bitmapData,
		size.x, size.y,
		maxSize.x, maxSize.y,
		trim,
		offset,
		isTwoBitsPerPixel,
		preferFullWord,
		preferFullWordForNonAscii,
		enabledEffect,
		shadowOffset,
		shadowStrength,
		shadowDirection,
		outlineOffset,
		outlineStrength,
		// Calculate `thresholds` later.
		inverted
	);

	for (int i = 0; i < GBBASIC_COUNTOF(thresholds); ++i) {
		result = Math::hash(result, thresholds[i]);
	}

	// `content` doesn't count.

	// `frameMargin` doesn't count.

	// `characterPadding` doesn't count.

	return result;
}

int FontAssets::Entry::compare(const Entry &other) const {
	if (path < other.path)
		return -1;
	else if (path > other.path)
		return 1;

	if (encodedTtfData < other.encodedTtfData)
		return -1;
	else if (encodedTtfData > other.encodedTtfData)
		return 1;

	if (bitmapData < other.bitmapData)
		return -1;
	else if (bitmapData > other.bitmapData)
		return 1;

	if (size < other.size)
		return -1;
	else if (other.size < size)
		return 1;

	if (maxSize < other.maxSize)
		return -1;
	else if (other.maxSize < maxSize)
		return 1;

	if (!trim && other.trim)
		return -1;
	else if (trim && !other.trim)
		return 1;

	if (offset < other.offset)
		return -1;
	else if (offset > other.offset)
		return 1;

	if (!isTwoBitsPerPixel && other.isTwoBitsPerPixel)
		return -1;
	else if (isTwoBitsPerPixel && !other.isTwoBitsPerPixel)
		return 1;

	if (!preferFullWord && other.preferFullWord)
		return -1;
	else if (preferFullWord && !other.preferFullWord)
		return 1;

	if (!preferFullWordForNonAscii && other.preferFullWordForNonAscii)
		return -1;
	else if (preferFullWordForNonAscii && !other.preferFullWordForNonAscii)
		return 1;

	if (enabledEffect < other.enabledEffect)
		return -1;
	else if (enabledEffect > other.enabledEffect)
		return 1;

	if (shadowOffset < other.shadowOffset)
		return -1;
	else if (shadowOffset > other.shadowOffset)
		return 1;

	if (shadowStrength < other.shadowStrength)
		return -1;
	else if (shadowStrength > other.shadowStrength)
		return 1;

	if (shadowDirection < other.shadowDirection)
		return -1;
	else if (shadowDirection > other.shadowDirection)
		return 1;

	if (outlineOffset < other.outlineOffset)
		return -1;
	else if (outlineOffset > other.outlineOffset)
		return 1;

	if (outlineStrength < other.outlineStrength)
		return -1;
	else if (outlineStrength > other.outlineStrength)
		return 1;

	for (int i = 0; i < GBBASIC_COUNTOF(thresholds); ++i) {
		if (thresholds[i] < other.thresholds[i])
			return -1;
		else if (thresholds[i] > other.thresholds[i])
			return 1;
	}

	if (!inverted && other.inverted)
		return -1;
	else if (inverted && !other.inverted)
		return 1;

	// `content` doesn't count.

	// `frameMargin` doesn't count.

	// `characterPadding` doesn't count.

	return 0;
}

Font::Ptr &FontAssets::Entry::touch(void) {
	if (object)
		return object;

	std::string readingPath = Path::combine(directory.c_str(), path.c_str());
	const bool pathExists = Path::fileExists(path.c_str());
	const bool dirPathExists = Path::fileExists(readingPath.c_str());
	if (isAsset) {
		if (directory.empty()) {
			if (pathExists) { // Is absolute path.
				// Do nothing.
			} else if (!encodedTtfData.empty()) { // Is embedded TTF.
				// Do nothing.
			} else if (!bitmapData.empty()) { // Is bitmap-based.
				// Do nothing.
			} else {
				return object;
			}
		} else {
			if (pathExists && !dirPathExists) { // Is absolute path.
				readingPath = path;
			} else if (dirPathExists) { // Is relative path.
				// Do nothing.
			} else if (!encodedTtfData.empty()) { // Is embedded TTF.
				// Do nothing.
			} else if (!bitmapData.empty()) { // Is bitmap-based.
				// Do nothing.
			} else {
				return object;
			}
		}
	} else {
		GBBASIC_ASSERT(dirPathExists && "Wrong data.");
		if (dirPathExists) { // Is relative path.
			// Do nothing.
		} else {
			return object;
		}
	}

	object = Font::Ptr(Font::create());

	if (size.x >= 1)
		loadBitmap(isAsset ? &bitmapData : nullptr, object, readingPath, size, 0);
	else
		loadTtf(isAsset ? &encodedTtfData : nullptr, object, readingPath, size.y, maxSize.x, maxSize.y, 0, trim);

	return object;
}

void FontAssets::Entry::cleanup(void) {
	object = nullptr;
}

Font::Ptr &FontAssets::Entry::touchSubstitution(int* offset_, int* thresholds_) const {
	if (substitutionObject) {
		if (offset_)
			*offset_ = substitutionOffset;
		if (thresholds_)
			memcpy(thresholds_, substitutionThresholds, sizeof(substitutionThresholds));

		return substitutionObject;
	}

	do {
		if (!Path::fileExists(ASSETS_FONT_SUBSTITUTION_CONFIG_FILE))
			break;

		File::Ptr file(File::create());
		if (!file->open(ASSETS_FONT_SUBSTITUTION_CONFIG_FILE, Stream::READ))
			break;

		std::string content_;
		if (!file->readString(content_)) {
			file->close();

			break;
		}

		rapidjson::Document doc;
		if (!Json::fromString(doc, content_.c_str()))
			break;
		std::string path_;
		int offset__ = GBBASIC_FONT_DEFAULT_OFFSET;
		if (!Jpath::get(doc, path_, "fonts", 0, "path"))
			break;
		if (!Jpath::get(doc, offset__, "fonts", 0, "offset"))
			break;
		int thresholds__[4];
		if (!Jpath::get(doc, thresholds__[0], "fonts", 0, "thresholds", 0))
			break;
		if (!Jpath::get(doc, thresholds__[1], "fonts", 0, "thresholds", 1))
			break;
		if (!Jpath::get(doc, thresholds__[2], "fonts", 0, "thresholds", 2))
			break;
		if (!Jpath::get(doc, thresholds__[3], "fonts", 0, "thresholds", 3))
			break;

		const std::string path_1 = Path::combine(ASSETS_FONT_DIR, path_.c_str());
		if (!file->open(path_1.c_str(), Stream::READ) && !file->open(path_.c_str(), Stream::READ))
			break;

		Bytes::Ptr bytes(Bytes::create());
		if (file->readBytes(bytes.get()) == 0) {
			file->close();

			break;
		}
		file->close();

		substitutionObject = Font::Ptr(Font::create());
		substitutionObject->trim(trim);
		if (!substitutionObject->fromTtf(bytes->pointer(), bytes->count(), size.y, maxSize.x, maxSize.y, 0))
			break;

		substitutionOffset = offset__;
		memcpy(substitutionThresholds, thresholds__, sizeof(substitutionThresholds));

		if (offset_)
			*offset_ = offset__;
		if (thresholds_)
			memcpy(thresholds_, thresholds__, sizeof(thresholds__));
	} while (false);

	return substitutionObject;
}

void FontAssets::Entry::cleanupSubstitution(void) const {
	substitutionObject = nullptr;
	substitutionOffset = GBBASIC_FONT_DEFAULT_OFFSET;
	substitutionThresholds[0] = 200;
	substitutionThresholds[1] = 100;
	substitutionThresholds[2] = 50;
	substitutionThresholds[3] = 0;
}

FontAssets::FontAssets() {
}

bool FontAssets::empty(void) const {
	return entries.empty();
}

int FontAssets::count(void) const {
	return (int)entries.size();
}

bool FontAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool FontAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

void FontAssets::clear(void) {
	entries.clear();
}

const FontAssets::Entry* FontAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

FontAssets::Entry* FontAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const FontAssets::Entry* FontAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const FontAssets::Entry* FontAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			gotName = entry.name;

			return &entry;
		}

		if (entry.name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(entry.name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = entry.name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int FontAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name)
			return i;
	}

	return -1;
}

bool FontAssets::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the font information.
	rapidjson::Document doc;
	doc.SetObject();
	int j = 0;
	for (int i = 0; i < count(); ++i) {
		const Entry* entry = get(i);
		if (!entry->isAsset)
			continue;

		if (!Jpath::set(doc, doc, entry->path, "fonts", j, "path")) {
			++j;

			continue;
		}

		if (entry->encodedTtfData.empty()) {
			if (!Jpath::set(doc, doc, nullptr, "fonts", j, "encoded_ttf_data")) {
				++j;

				continue;
			}
		} else {
			if (!Jpath::set(doc, doc, entry->encodedTtfData, "fonts", j, "encoded_ttf_data")) {
				++j;

				continue;
			}
		}

		if (entry->bitmapData.empty()) {
			if (!Jpath::set(doc, doc, nullptr, "fonts", j, "bitmap_data")) {
				++j;

				continue;
			}
		} else {
			if (!Jpath::set(doc, doc, entry->bitmapData, "fonts", j, "bitmap_data")) {
				++j;

				continue;
			}
		}

		if (!Jpath::set(doc, doc, entry->size.x, "fonts", j, "size", 0)) {
			++j;

			continue;
		}
		if (!Jpath::set(doc, doc, entry->size.y, "fonts", j, "size", 1)) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->maxSize.x, "fonts", j, "max_size", 0)) {
			++j;

			continue;
		}
		if (!Jpath::set(doc, doc, entry->maxSize.y, "fonts", j, "max_size", 1)) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->trim, "fonts", j, "trim")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->offset, "fonts", j, "offset")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->isTwoBitsPerPixel, "fonts", j, "is_two_bits_per_pixel")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->preferFullWord, "fonts", j, "prefer_full_word")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->preferFullWordForNonAscii, "fonts", j, "prefer_full_word_for_non_ascii")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->enabledEffect, "fonts", j, "enabled_effect")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->shadowOffset, "fonts", j, "shadow_offset")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->shadowStrength, "fonts", j, "shadow_strength")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->shadowDirection, "fonts", j, "shadow_direction")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->outlineOffset, "fonts", j, "outline_offset")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->outlineStrength, "fonts", j, "outline_strength")) {
			++j;

			continue;
		}

		for (int k = 0; k < GBBASIC_COUNTOF(entry->thresholds); ++k) {
			if (!Jpath::set(doc, doc, entry->thresholds[k], "fonts", j, "thresholds", k))
				break;
		}

		if (!Jpath::set(doc, doc, entry->inverted, "fonts", j, "inverted")) {
			++j;

			continue;
		}

		bool isCustomized = entry->arbitrary.count() != (int)std::numeric_limits<Byte>::max() + 1;
		if (!isCustomized) {
			for (int i = 0; i < std::numeric_limits<Byte>::max() + 1 && i < entry->arbitrary.count(); ++i) {
				if (entry->arbitrary[i] != (Font::Codepoint)i) {
					isCustomized = true;

					break;
				}
			}
		}
		if (isCustomized) {
			for (int i = 0; i < std::numeric_limits<Byte>::max() + 1 && i < entry->arbitrary.count(); ++i) {
				const Font::Codepoint arb = entry->arbitrary[i];
				if (!Jpath::set(doc, doc, arb, "fonts", j, "arbitrary", i))
					continue;
			}
		} else {
			Jpath::set(doc, doc, nullptr, "fonts", j, "arbitrary");
		}

		if (!Jpath::set(doc, doc, entry->content, "fonts", j, "content")) {
			++j;

			continue;
		}

		if (!Jpath::set(doc, doc, entry->name, "fonts", j, "name")) {
			++j;

			continue;
		}

		Jpath::set(doc, doc, entry->magnification, "fonts", j, "magnification");

		// `frameMargin` is readonly.

		// `characterPadding` is readonly.

		++j;
	}

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert font to JSON.", false, onWarningOrError);

		return false;
	}
	if (!json->toString(val, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool FontAssets::fromString(const std::string &val, const std::string &dir, bool isAsset, WarningOrErrorHandler onWarningOrError) {
	// Prepare.
	clear();

	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert from JSON.", false, onWarningOrError);

		return false;
	}

	const int n = Jpath::count(doc, "fonts");
	if (n == 0)
		return false;

	// Parse the font information.
	for (int i = 0; i < n; ++i) {
		// Prepare.
		Entry f;

		const int page = isAsset ? i : i + 1;

		// Parse the font's encoded data and/or bitmap data.
		std::string encodedTtfData;
		if (isAsset) {
			if (Jpath::typeOf(doc, "fonts", i, "encoded_ttf_data") == Jpath::NIL) {
				// Do nothing.
			} else {
				if (!Jpath::get(doc, encodedTtfData, "fonts", i, "encoded_ttf_data")) {
					assetsRaiseWarningOrError("Font's \"Encoded TTF Data\" field is missing at page {0}.", page, true, onWarningOrError);

					continue;
				}
			}
		}

		std::string bitmapData;
		if (isAsset) {
			if (Jpath::typeOf(doc, "fonts", i, "bitmap_data") == Jpath::NIL) {
				// Do nothing.
			} else {
				if (!Jpath::get(doc, bitmapData, "fonts", i, "bitmap_data")) {
					assetsRaiseWarningOrError("Font's \"Bitmap Data\" field is missing at page {0}.", page, true, onWarningOrError);

					continue;
				}
			}
		}

		// Analyze the font's path.
		std::string path;
		if (!Jpath::get(doc, path, "fonts", i, "path")) {
			assetsRaiseWarningOrError("Font path is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}
		std::string readingPath = Path::combine(dir.c_str(), path.c_str());
		const bool pathExists = Path::fileExists(path.c_str());
		const bool dirPathExists = Path::fileExists(readingPath.c_str());
		if (isAsset) {
			if (encodedTtfData.empty() && bitmapData.empty()) {
				if (dir.empty()) {
					assetsRaiseWarningOrError("Font file doesn't exist at page {0}.", page, true, onWarningOrError);

					continue;
				} else {
					if (pathExists && !dirPathExists) { // Is absolute path.
						readingPath = path;
						f.copying = Entry::Copying::REFERENCED;
					} else if (dirPathExists) { // Is relative path.
						f.copying = Entry::Copying::COPIED;
					} else {
						assetsRaiseWarningOrError("Font file doesn't exist at page {0}.", page, true, onWarningOrError);

						continue;
					}
				}
			}
		} else {
			GBBASIC_ASSERT(dirPathExists && "Wrong data.");
			if (dirPathExists) { // Is relative path.
				f.copying = Entry::Copying::REFERENCED;
			} else {
				assetsRaiseWarningOrError("Font file doesn't exist at page {0}.", page, true, onWarningOrError);

				continue;
			}
		}

		// Parse the font's properties.
		f.directory = dir;
		f.path = path;
		f.encodedTtfData = encodedTtfData;
		f.bitmapData = bitmapData;

		if (!Jpath::get(doc, f.size.x, "fonts", i, "size", 0)) {
			assetsRaiseWarningOrError("Font size is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}
		if (!Jpath::get(doc, f.size.y, "fonts", i, "size", 1)) {
			assetsRaiseWarningOrError("Font size is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.maxSize.x, "fonts", i, "max_size", 0)) {
			assetsRaiseWarningOrError("Font max size is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}
		if (!Jpath::get(doc, f.maxSize.y, "fonts", i, "max_size", 1)) {
			assetsRaiseWarningOrError("Font max size is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.trim, "fonts", i, "trim")) {
			f.trim = true;
		}

		if (!Jpath::get(doc, f.offset, "fonts", i, "offset")) {
			assetsRaiseWarningOrError("Font offset is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.isTwoBitsPerPixel, "fonts", i, "is_two_bits_per_pixel")) {
			assetsRaiseWarningOrError("Font's \"Two Bits per Pixel\" field is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.preferFullWord, "fonts", i, "prefer_full_word")) {
			assetsRaiseWarningOrError("Font's \"Prefer Full Word\" field is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.preferFullWordForNonAscii, "fonts", i, "prefer_full_word_for_non_ascii")) {
			assetsRaiseWarningOrError("Font's \"Prefer Full Word for Non-ASCII\" field is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.enabledEffect, "fonts", i, "enabled_effect")) {
			f.enabledEffect = (int)Font::Effects::NONE;
		}

		if (!Jpath::get(doc, f.shadowOffset, "fonts", i, "shadow_offset")) {
			f.shadowOffset = 1;
		}

		if (!Jpath::get(doc, f.shadowStrength, "fonts", i, "shadow_strength")) {
			f.shadowStrength = 0.3f;
		}

		if (!Jpath::get(doc, f.shadowDirection, "fonts", i, "shadow_direction")) {
			f.shadowDirection = (UInt8)Directions::DIRECTION_DOWN_RIGHT;
		}

		if (!Jpath::get(doc, f.outlineOffset, "fonts", i, "outline_offset")) {
			f.outlineOffset = 1;
		}

		if (!Jpath::get(doc, f.outlineStrength, "fonts", i, "outline_strength")) {
			f.outlineStrength = 0.3f;
		}

		for (int j = 0; j < GBBASIC_COUNTOF(f.thresholds); ++j) {
			if (!Jpath::get(doc, f.thresholds[j], "fonts", i, "thresholds", j)) {
				assetsRaiseWarningOrError("Font's \"Thresholds\" field is missing or invalid at page {0}.", page, true, onWarningOrError);

				break;
			}
		}

		if (!Jpath::get(doc, f.inverted, "fonts", i, "inverted")) {
			continue;
		}

		if (!Jpath::has(doc, "fonts", i, "arbitrary") || Jpath::typeOf(doc, "fonts", i, "arbitrary") != Jpath::ARRAY) {
			for (int j = 0; j < std::numeric_limits<Byte>::max() + 1; ++j)
				f.arbitrary.add(j);
		} else {
			const int m = Jpath::count(doc, "fonts", i, "arbitrary");
			for (int j = 0; j < m; ++j) {
				Font::Codepoint arb = 0;
				if (!Jpath::get(doc, arb, "fonts", i, "arbitrary", j))
					continue;

				f.arbitrary.add(arb);
			}
		}

		if (!Jpath::get(doc, f.content, "fonts", i, "content")) {
			assetsRaiseWarningOrError("Font content is missing at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (!Jpath::get(doc, f.name, "fonts", i, "name")) {
			f.name.clear();
		}

		if (!Jpath::get(doc, f.magnification, "fonts", i, "magnification")) {
			f.magnification = -1;
		}

		if (!Jpath::get(doc, f.frameMargin.x, "fonts", i, "frame_margin", 0) || !Jpath::get(doc, f.frameMargin.y, "fonts", i, "frame_margin", 1)) {
			f.frameMargin = Math::Vec2i(1, 1);
		}

		if (!Jpath::get(doc, f.characterPadding.x, "fonts", i, "character_padding", 0) || !Jpath::get(doc, f.characterPadding.y, "fonts", i, "character_padding", 1)) {
			f.characterPadding = Math::Vec2i(0, 1);
		}

		if ((f.size.x != -1 && (f.size.x < 1 || f.size.x > ASSETS_FONT_MAX_SIZE)) || (f.size.y < 1 || f.size.y > ASSETS_FONT_MAX_SIZE)) {
			assetsRaiseWarningOrError("Invalid font size at page {0}.", page, true, onWarningOrError);

			continue;
		}

		if (f.maxSize.x > ASSETS_FONT_MAX_SIZE || f.maxSize.y > ASSETS_FONT_MAX_SIZE) {
			assetsRaiseWarningOrError("Invalid font max size at page {0}.", page, true, onWarningOrError);

			continue;
		}

		// Create a font object.
		Font::Ptr obj(Font::create());

		if (f.size.x >= 1) {
			if (!loadBitmap(isAsset ? &f.bitmapData : nullptr, obj, readingPath, f.size, 0)) {
				assetsRaiseWarningOrError("Cannot load font file at page {0}.", page, true, onWarningOrError);

				continue;
			}
		} else {
			if (!loadTtf(isAsset ? &f.encodedTtfData : nullptr, obj, readingPath, f.size.y, f.maxSize.x, f.maxSize.y, 0, f.trim)) {
				assetsRaiseWarningOrError("Cannot load font file at page {0}.", page, true, onWarningOrError);

				continue;
			}
		}

		f.object = obj;

		f.isAsset = isAsset;

		// Add it to the collection.
		add(f);
	}

	// Finish.
	return true;
}

bool FontAssets::loadTtf(std::string* encodedTtfDataPtr, Font::Ptr obj, const std::string &path, int size, int maxWidth, int maxHeight, int permeation, bool trim) {
	if (!obj)
		return false;

	const bool fromFile = !encodedTtfDataPtr || encodedTtfDataPtr->empty();
	if (fromFile) { // Load from TTF file.
		if (!Path::fileExists(path.c_str()))
			return false;

		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ))
			return false;

		Bytes::Ptr bytes(Bytes::create());
		if (file->readBytes(bytes.get()) == 0) {
			file->close();

			return false;
		}
		file->close();

		obj->trim(trim);
		if (!obj->fromTtf(bytes->pointer(), bytes->count(), size, maxWidth, maxHeight, permeation))
			return false;

		if (encodedTtfDataPtr) {
			if (!Base64::fromBytes(*encodedTtfDataPtr, bytes.get()))
				return false;
		}
	} else { // Load from encoded TTF data.
		Bytes::Ptr bytes(Bytes::create());
		if (!Base64::toBytes(bytes.get(), *encodedTtfDataPtr))
			return false;

		obj->trim(trim);
		if (!obj->fromTtf(bytes->pointer(), bytes->count(), size, maxWidth, maxHeight, permeation))
			return false;
	}

	return true;
}

bool FontAssets::loadBitmap(std::string* bitmapDataPtr, Font::Ptr obj, const std::string &path, const Math::Vec2i &size, int permeation) {
	if (!obj)
		return false;

	const bool fromFile = !bitmapDataPtr || bitmapDataPtr->empty();
	if (fromFile) { // Load from TTF file.
		if (!Path::fileExists(path.c_str()))
			return false;

		File::Ptr file(File::create());
		if (!file->open(path.c_str(), Stream::READ))
			return false;

		Bytes::Ptr bytes(Bytes::create());
		if (file->readBytes(bytes.get()) == 0) {
			file->close();

			return false;
		}
		file->close();

		Image::Ptr img(Image::create());
		if (!img->fromBytes(bytes.get()))
			return false;

		if (!obj->fromImage(img.get(), (int)size.x, (int)size.y, permeation))
			return false;

		if (bitmapDataPtr) {
			if (!Base64::fromBytes(*bitmapDataPtr, bytes.get()))
				return false;
		}
	} else { // Load from encoded TTF data.
		Bytes::Ptr bytes(Bytes::create());
		if (!Base64::toBytes(bytes.get(), *bitmapDataPtr))
			return false;

		Image::Ptr img(Image::create());
		if (!img->fromBytes(bytes.get()))
			return false;

		if (!obj->fromImage(img.get(), (int)size.x, (int)size.y, permeation))
			return false;
	}

	return true;
}

bool FontAssets::calculateBestFit(const Entry &font, int* size, int* offset) {
	if (size)
		*size = -1;
	if (offset)
		*offset = 0;

	if (!font.object)
		return false;

	size_t len = 0;
	const Byte* data = (const Byte*)font.object->pointer(&len);
	if (!data || !len)
		return false;

	constexpr const int MIN_SIZE = 8;
	constexpr const char SIZE_TESTER = 'A';
	constexpr const char OFFSET_TESTER = 'g';

	int preferedSize = -1;
	int preferedOffset = 0;

	auto calculateSharpness = [] (const Image* img) -> float {
		int mostVisibleAlpha = 0;
		int totalVisiblePixels = 0;
		int pixels[256];
		memset(pixels, 0, sizeof(pixels));
		for (int j = 0; j < img->height(); ++j) {
			for (int i = 0; i < img->width(); ++i) {
				Colour col;
				img->get(i, j, col);
				++pixels[col.a];
				if (col.a > mostVisibleAlpha)
					mostVisibleAlpha = col.a;
				if (col.a > 0)
					++totalVisiblePixels;
			}
		}

		if (mostVisibleAlpha != 255 || totalVisiblePixels == 0)
			return 0.0f;

		return (float)pixels[mostVisibleAlpha] / totalVisiblePixels;
	};
	auto checkCompleteness = [] (const Image* img) -> bool {
		if (img->height() == 0)
			return false;

		const int j = img->height() - 1;
		for (int i = 0; i < img->width(); ++i) {
			Colour col;
			img->get(i, j, col);
			if (col.a > 0)
				return true;
		}

		return false;
	};

	float score = 0.0f;
	for (int sz = MIN_SIZE; sz <= ASSETS_FONT_MAX_SIZE; ++sz) {
		Font::Ptr tmp(Font::create());
		if (!tmp->fromTtf(data, len, sz, ASSETS_FONT_MAX_SIZE, ASSETS_FONT_MAX_SIZE, 0))
			return false;
		tmp->trim(false);

		Image::Ptr img(Image::create());
		const Colour col(255, 255, 255, 255);
		int width = 0;
		int height = 0;
		const Font::Options opt;
		if (!tmp->render(SIZE_TESTER, img.get(), &col, &width, &height, 0, &opt))
			return false;

		const float score_ = calculateSharpness(img.get());
		if (score_ > score) {
			preferedSize = sz;
			score = score_;
		}
	}

	if (preferedSize >= MIN_SIZE) {
		Font::Ptr tmp(Font::create());
		if (!tmp->fromTtf(data, len, preferedSize, ASSETS_FONT_MAX_SIZE, ASSETS_FONT_MAX_SIZE, 0))
			return false;
		tmp->trim(true);

		const int minOffset = -(preferedSize / 2);
		for (int off = -1; off >= minOffset; --off) {
			Image::Ptr img(Image::create());
			const Colour col(255, 255, 255, 255);
			int width = 0;
			int height = 0;
			const Font::Options opt;
			if (!tmp->render(OFFSET_TESTER, img.get(), &col, &width, &height, off, &opt))
				return false;

			const bool complete = checkCompleteness(img.get());
			if (complete)
				preferedOffset = off;
			else
				break;
		}
	}

	if (preferedSize >= MIN_SIZE) {
		if (size)
			*size = preferedSize;
		if (offset)
			*offset = preferedOffset;

		return true;
	}

	return false;
}

bool FontAssets::measure(const Entry &font, const GlyphTable::Entry &glyph, int* width, int* height) {
	// Prepare.
	if (width)
		*width = 0;
	if (height)
		*height = 0;

	// Measure the glyph.
	Font::Options options;
	if (font.isTwoBitsPerPixel && !!font.enabledEffect && (!!font.shadowOffset || !!font.outlineOffset)) {
		options.enabledEffect = (Font::Effects)font.enabledEffect;
		options.shadowOffset = font.shadowOffset;
		options.shadowStrength = font.shadowStrength;
		options.shadowDirection = font.shadowDirection;
		options.outlineOffset = font.outlineOffset;
		options.outlineStrength = font.outlineStrength;
		for (int i = 0; i < GBBASIC_COUNTOF(font.thresholds); ++i) {
			if (font.thresholds[i] != 0 && font.thresholds[i] < options.effectThreshold)
				options.effectThreshold = font.thresholds[i];
		}
	}
	bool rendered = false;
	int offset = font.offset;
	int thresholds[4];
	bool unknown = true;
	do {
		// Measure with the font object.
		memcpy(thresholds, font.thresholds, sizeof(thresholds));
		rendered = font.object->measure(glyph.codepoint, width, height, offset, &options);
		if (rendered) {
			unknown = false;

			break;
		}

		// Measure ' ' with substitution.
		if (!rendered && glyph.codepoint == ' ') {
			rendered = font.object->measure('X', width, height, offset, &options);
		}
		if (rendered) {
			unknown = false;

			break;
		}

		// Measure with the substitution font object.
		Font::Ptr &sub = font.touchSubstitution(&offset, thresholds);
		if (sub) {
			Font::Options options_ = options;
			for (int i = 0; i < GBBASIC_COUNTOF(font.thresholds); ++i) {
				if (thresholds[i] != 0 && thresholds[i] < options_.effectThreshold)
					options_.effectThreshold = thresholds[i];
			}
			if (width) *width = 0;
			if (height) *height = 0;
			rendered = sub->measure(glyph.codepoint, width, height, offset, &options_);
			if (rendered) {
				break;
			}
		}

		// Measure '?' instead of the origin glyph with the font object.
		rendered = font.object->measure('?', width, height, offset, &options);
		if (rendered) {
			memcpy(thresholds, font.thresholds, sizeof(thresholds));

			break;
		}
	} while (false);
	if (!rendered) {
		return false;
	}

	// Finish.
	return true;
}

bool FontAssets::bake(Entry &font, GlyphTable::Entry &glyph, Bytes* buf, int* bytes_) {
	// Prepare.
	if (buf)
		buf->clear();
	if (bytes_)
		*bytes_ = 0;

	// Render the glyph to colored pixels.
	Image::Ptr pixels(Image::create());
	const Colour col(0, 255, 0, 255);
	Font::Options options;
	if (font.isTwoBitsPerPixel && !!font.enabledEffect && (!!font.shadowOffset || !!font.outlineOffset)) {
		options.enabledEffect = (Font::Effects)font.enabledEffect;
		options.shadowOffset = font.shadowOffset;
		options.shadowStrength = font.shadowStrength;
		options.shadowDirection = font.shadowDirection;
		options.outlineOffset = font.outlineOffset;
		options.outlineStrength = font.outlineStrength;
		for (int i = 0; i < GBBASIC_COUNTOF(font.thresholds); ++i) {
			if (font.thresholds[i] != 0 && font.thresholds[i] < options.effectThreshold)
				options.effectThreshold = font.thresholds[i];
		}
	}

	bool rendered = false;
	int offset = font.offset;
	int thresholds[4];
	glyph.unknown = true;
	do {
		// Render with the font object.
		memcpy(thresholds, font.thresholds, sizeof(thresholds));
		rendered = font.object->render(glyph.codepoint, pixels.get(), &col, &glyph.width, &glyph.height, offset, &options);
		if (rendered) {
			glyph.unknown = false;

			break;
		}

		// Render ' ' with substitution.
		if (!rendered && glyph.codepoint == ' ') {
			rendered = font.object->render('X', pixels.get(), &col, &glyph.width, &glyph.height, offset, &options);
			pixels->fromBlank(pixels->width(), pixels->height(), 0);
		}
		if (rendered) {
			glyph.unknown = false;

			break;
		}

		// Render with the substitution font object.
		Font::Ptr &sub = font.touchSubstitution(&offset, thresholds);
		if (sub) {
			Font::Options options_ = options;
			for (int i = 0; i < GBBASIC_COUNTOF(font.thresholds); ++i) {
				if (thresholds[i] != 0 && thresholds[i] < options_.effectThreshold)
					options_.effectThreshold = thresholds[i];
			}
			glyph.width = glyph.height = 0;
			rendered = sub->render(glyph.codepoint, pixels.get(), &col, &glyph.width, &glyph.height, offset, &options_);
			if (rendered) {
				break;
			}
		}

		// Render '?' instead of the origin glyph with the font object.
		rendered = font.object->render('?', pixels.get(), &col, &glyph.width, &glyph.height, offset, &options);
		if (rendered) {
			memcpy(thresholds, font.thresholds, sizeof(thresholds));

			break;
		}
	} while (false);
	if (!rendered) {
		return false;
	}

	// Adjust the pixels.
	if (offset != 0 || glyph.width > GBBASIC_FONT_MAX_SIZE || glyph.height > GBBASIC_FONT_MAX_SIZE) {
		Image::Ptr tmp(Image::create());
		glyph.width = Math::min(glyph.width, GBBASIC_FONT_MAX_SIZE);
		glyph.height = Math::min(glyph.height, GBBASIC_FONT_MAX_SIZE);
		tmp->fromBlank(glyph.width, glyph.height, 0);
		pixels->blit(tmp.get(), 0, 0, glyph.width, glyph.height, 0, 0 /* -offset */);
		std::swap(pixels, tmp);
	}

	// Determine the glyph's footprint.
	const int p = pixels->width() * pixels->height(); // Pixel count.
	int q = Math::ceilIntegerTimesOf(p, 8); // Byte count.
	if (font.isTwoBitsPerPixel)
		q *= 2;
	if (bytes_)
		*bytes_ = q;

	UInt8 byte0 = 0;
	UInt8 byte1 = 0;
	for (int k = 0; k < p; ++k) {
		const std::div_t div = std::div(k, pixels->width());
		const int i = div.rem;
		const int j = div.quot;
		Colour c;
		if (!pixels->get(i, j, c)) {
			GBBASIC_ASSERT(false && "Wrong data.");
		}
		const int rem = k % 8;
		const UInt8 b = 1 << (7 - rem);
		const int bits = getBits(c, font.isTwoBitsPerPixel, thresholds, font.inverted);
		switch (bits) {
		case 0b00: // Do nothing.
			break;
		case 0b01:
			byte0 |= b;

			break;
		case 0b10:
			byte1 |= b;

			break;
		case 0b11:
			byte0 |= b;
			byte1 |= b;

			break;
		default:
			GBBASIC_ASSERT(false && "Impossible.");

			break;
		}
		if (rem == 7 || k == p - 1) {
			if (buf)
				buf->writeUInt8(byte0);
			if (font.isTwoBitsPerPixel) {
				if (buf)
					buf->writeUInt8(byte1);
			}
			byte0 = 0;
			byte1 = 0;
		}
	}

#if defined GBBASIC_DEBUG
	if (glyph.codepoint == '\a') {
		fprintf(stdout, "Baked glyph '%ud', %dx%d.\n", (unsigned)glyph.codepoint, glyph.width, glyph.height);
	} else {
		std::wstring wstr;
		wstr.push_back((wchar_t)glyph.codepoint);
		const std::string str = Unicode::fromWide(wstr);
		const std::string osstr = Unicode::toOs(str);
		fprintf(stdout, "Baked glyph '%s', %dx%d.\n", osstr.c_str(), glyph.width, glyph.height);
	}
#endif /* GBBASIC_DEBUG */

	// Finish.
	return true;
}

int FontAssets::getBits(const Colour &col, bool isTwoBitsPerPixel, const int thresholds[4], bool inverted) {
	constexpr const int BITSN[] = { 0b11, 0b10, 0b01, 0b00 };
	constexpr const int BITSI[] = { 0b00, 0b01, 0b10, 0b11 };
	const int* bits = inverted ? BITSI : BITSN;
	if (isTwoBitsPerPixel) {
		if (col.a >= thresholds[0])
			return bits[0];
		else if (col.a >= thresholds[1])
			return bits[1];
		else if (col.a >= thresholds[2])
			return bits[2];
		else if (col.a >= thresholds[3])
			return bits[3];

		return bits[3];
	} else {
		if (col.a >= thresholds[0])
			return bits[0];
		else
			return bits[3];
	}
}

/* ===========================================================================} */

/*
** {===========================================================================
** Code assets
*/

CodeAssets::Entry::Entry(const std::string &val) :
	data(val)
{
}

bool CodeAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler) {
	data = val;

	return true;
}

size_t CodeAssets::Entry::hash(void) const {
	return Math::hash(0, data);
}

int CodeAssets::Entry::compare(const Entry &other) const {
	if (data < other.data)
		return -1;
	else if (data > other.data)
		return 1;

	return 0;
}

bool CodeAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	val = data;

	const size_t codeend = Text::indexOf(val, COMPILER_CODE_END);
	if (codeend != std::string::npos) {
		assetsRaiseWarningOrError("Cannot serialize code due to unexpected end tag \"" COMPILER_CODE_END "\".", false, onWarningOrError);

		return false;
	}

	return true;
}

bool CodeAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler) {
	data.assign(val, len);

	return true;
}

bool CodeAssets::empty(void) const {
	return entries.empty();
}

int CodeAssets::count(void) const {
	return (int)entries.size();
}

bool CodeAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool CodeAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

const CodeAssets::Entry* CodeAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

CodeAssets::Entry* CodeAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

/* ===========================================================================} */

/*
** {===========================================================================
** Tiles assets
*/

TilesAssets::Entry::Entry(Renderer* rnd, PaletteAssets::Getter getplt) :
	renderer(rnd),
	getPalette(getplt)
{
	palette = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
	data = Image::Ptr(Image::create(palette));

	data->fromBlank(
		GBBASIC_TILES_DEFAULT_WIDTH * GBBASIC_TILE_SIZE,
		GBBASIC_TILES_DEFAULT_HEIGHT * GBBASIC_TILE_SIZE,
		GBBASIC_PALETTE_DEPTH
	);
}

TilesAssets::Entry::Entry(Renderer* rnd, const std::string &val, PaletteAssets::Getter getplt) :
	renderer(rnd),
	getPalette(getplt)
{
	palette = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
	data = Image::Ptr(Image::create(palette));

	if (!fromString(val, nullptr)) {
		data->fromBlank(
			GBBASIC_TILES_DEFAULT_WIDTH * GBBASIC_TILE_SIZE,
			GBBASIC_TILES_DEFAULT_HEIGHT * GBBASIC_TILE_SIZE,
			GBBASIC_PALETTE_DEPTH
		);
	}
}

size_t TilesAssets::Entry::hash(void) const {
	size_t result = 0;

	if (data)
		result = Math::hash(result, data->hash());

	// Ref to a palette doesn't count.

	return result;
}

int TilesAssets::Entry::compare(const Entry &other) const {
	int ret = 0;

	if (data && other.data)
		ret = data->compare(other.data.get());
	if (ret != 0)
		return ret;

	return 0;
}

Texture::Ptr &TilesAssets::Entry::touch(void) {
	if (!texture || !texture->pointer(renderer)) {
		texture = Texture::Ptr(Texture::create());
		texture->fromImage(renderer, Texture::STREAMING, data.get(), Texture::NEAREST);
		texture->blend(Texture::BLEND);
	}

	return texture;
}

void TilesAssets::Entry::cleanup(void) {
	data->cleanup();
	texture = nullptr;
}

bool TilesAssets::Entry::refresh(void) {
	// Determine whether can refresh.
	const PaletteAssets::Entry* ref_ = getPalette(ref);
	if (!ref_)
		return false;

	// Determine whether need to refresh.
	if (!isRefRevisionChanged(ref_)) // There is unsynchronized modification in this asset's dependency.
		return false;

	// Refresh this asset.
	for (int i = 0; i < Math::min(ref_->data->count(), GBBASIC_PALETTE_PER_GROUP_COUNT); ++i) {
		Colour col;
		ref_->data->get(i, col);
		palette->set(i, &col);
	}
	cleanup(); // Clean up the outdated editable and runtime resources.

	// Mark this asset synchronized.
	synchronizeRefRevision(ref_); // Mark as synchronized for this asset's dependency.

	// Finish.
	return true;
}

int TilesAssets::Entry::estimateFinalByteCount(void) const {
	int result = data->width() * data->height();
	result *= GBBASIC_PALETTE_DEPTH;
	result /= 8;

	return result;
}

bool TilesAssets::Entry::serializeBasic(std::string &val, int page, bool forTiles) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	std::string asset;
	if (name.empty())
		asset = "#" + Text::toString(page);
	else
		asset = "\"" + name + "\"";

	const int n = (data->width() / GBBASIC_TILE_SIZE) * (data->height() / GBBASIC_TILE_SIZE);
	if (forTiles) {
		val += "fill tile(";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(n);
		val += ") = ";
		val += asset;
		val += "\n";
	} else {
		val += "sprite on";
		val += "\n";
		val += "fill sprite(";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(n);
		val += ") = ";
		val += asset;
		val += "\n";
		val += "def sprite(0) = 0";
		val += "\n";
		val += "sprite 0, x, y";
		val += "\n";
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::serializeDataSequence(std::string &val, int base) const {
	// Prepare.
	val.clear();

	// Serialize the pixels.
	if (data->paletted()) {
		data->serializeTiles(
			GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
			[&] (int /* tx */, int /* ty */) -> void {
				val += "data ";
			},
			[&] (int /* tx */, int /* ty */) -> void {
				val += "\n";
			},
			[&] (int y, int /* lineno */, int /* tx */, int /* ty */, UInt8 ln0, UInt8 ln1) -> void {
				if (base == 16) {
					val += "0x" + Text::toHex(ln0, 2, '0', false);
					val += ", ";
					val += "0x" + Text::toHex(ln1, 2, '0', false);
				} else if (base == 10) {
					val += Text::toString(ln0);
					val += ", ";
					val += Text::toString(ln1);
				} else if (base == 2) {
					val += "0b" + Text::toBin(ln0);
					val += ", ";
					val += "0b" + Text::toHex(ln1);
				} else {
					GBBASIC_ASSERT(false && "Not supported.");
				}

				if (y != GBBASIC_TILE_SIZE - 1)
					val += ", ";
			}
		);
	} else {
		for (int j = 0; j < data->height(); ++j) {
			val += "data ";
			for (int i = 0; i < data->width(); ++i) {
				Colour col;
				if (!data->get(i, j, col))
					return false;
				const UInt32 rgba = col.toRGBA();

				if (base == 16) {
					val += "0x" + Text::toHex(rgba, 8, '0', false);
				} else if (base == 10) {
					val += Text::toString(rgba);
				} else if (base == 2) {
					val += Text::toBin(rgba);
				} else {
					GBBASIC_ASSERT(false && "Not supported.");
				}

				if (i != data->width() - 1)
					val += ", ";
			}
			val += "\n";
		}
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::parseDataSequence(Image::Ptr &img, const std::string &val, bool isCompressed) const {
	// Prepare.
	typedef std::vector<Int64> Data;

	img = nullptr;

	// Parse the pixels.
	Data bytes;
	std::string val_ = val;
	val_ = Text::replace(val_, "\r\n", ",");
	val_ = Text::replace(val_, "\r", ",");
	val_ = Text::replace(val_, "\n", ",");
	val_ = Text::replace(val_, "\t", ",");
	val_ = Text::replace(val_, " ", ",");
	Text::Array tokens = Text::split(val_, ",");
	val_.clear();
	for (std::string tk : tokens) {
		tk = Text::trim(tk);
		if (tk.empty())
			continue;

		if (data->paletted()) {
			int idx = 0;
			if (!Text::fromString(tk, idx))
				continue;

			bytes.push_back(idx);
			if (isCompressed)
				bytes.push_back(idx);
		} else {
			unsigned rgba = 0;
			if (!Text::fromString(tk, rgba))
				continue;

			bytes.push_back(rgba);
		}
	}

	// Fill in an image.
	Image* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	img = Image::Ptr(ptr);

	if (img->paletted()) {
		img->parseTiles(
			GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
			[&] (void) -> int {
				return (int)bytes.size();
			},
			[&] (int idx) -> Int64 {
				return bytes[idx];
			}
		);
	} else {
		int k = 0;
		for (int j = 0; j < img->height(); ++j) {
			for (int i = 0; i < img->width(); ++i) {
				const Int64 num = bytes[k++];
				Colour col;
				col.fromRGBA((UInt32)num);
				img->set(i, j, col);

				if (k >= (int)bytes.size())
					break;
			}

			if (k >= (int)bytes.size())
				break;
		}
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::serializeJson(std::string &val, bool pretty) const {
	// Prepare.
	val.clear();

	// Serialize the pixels.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "pixels");
	rapidjson::Value* pixels = nullptr;
	Jpath::get(doc, pixels, "pixels");

	rapidjson::Document doc_;
	if (!data->toJson(doc_))
		return false;
	pixels->Set(doc_.GetObject());

	// Convert to string.
	std::string str;
	if (!Json::toString(doc, str, pretty))
		return false;

	val = str;

	// Finish.
	return true;
}

bool TilesAssets::Entry::parseJson(Image::Ptr &img, const std::string &val, ParsingStatuses &status) const {
	// Prepare.
	img = nullptr;
	status = ParsingStatuses::SUCCESS;

	// Convert from string.
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	if (!doc.IsObject())
		return false;

	// Parse the pixels.
	const rapidjson::Value* pixels = nullptr;
	if (!Jpath::get(doc, pixels, "pixels"))
		return false;
	if (!pixels || !pixels->IsObject())
		return false;

	int w = 0;
	int h = 0;
	Jpath::get(*pixels, w, "width");
	Jpath::get(*pixels, h, "height");
	if ((w % GBBASIC_TILE_SIZE) != 0 || (h % GBBASIC_TILE_SIZE) != 0) {
		status = ParsingStatuses::NOT_A_MULTIPLE_OF_8x8;

		return false;
	}
	if (w * h > GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE * GBBASIC_TILES_MAX_AREA_SIZE) {
		status = ParsingStatuses::OUT_OF_BOUNDS;

		return false;
	}

	// Fill in an image.
	Image* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	img = Image::Ptr(ptr);

	if (!img->fromJson(*pixels))
		return false;

	// Finish.
	return true;
}

bool TilesAssets::Entry::serializeImage(Image* val, const Math::Recti* area) const {
	// Prepare.
	if (!val)
		return false;

	// Serialize.
	if (data->paletted()) {
		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, data->width(), data->height());
		Image::Ptr tmp(Image::create());
		tmp->fromBlank((int)area_.width(), (int)area_.height(), 0);
		for (int j = 0; j < (int)area_.height(); ++j) {
			for (int i = 0; i < (int)area_.width(); ++i) {
				int idx = 0;
				data->get(i + (int)area_.xMin(), j + (int)area_.yMin(), idx);
				Colour col;
				palette->get(idx, col);
				tmp->set(i, j, col);
			}
		}

		if (!val->fromImage(tmp.get()))
			return false;
	} else {
		if (!val->fromImage(data.get()))
			return false;
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::serializeImage(Bytes* val, const char* type, const Math::Recti* area) const {
	// Prepare.
	if (!val)
		return false;

	val->clear();

	// Serialize.
	if (data->paletted()) {
		const Math::Recti area_ = area ? *area : Math::Recti::byXYWH(0, 0, data->width(), data->height());
		Image::Ptr tmp(Image::create());
		tmp->fromBlank((int)area_.width(), (int)area_.height(), 0);
		for (int j = 0; j < (int)area_.height(); ++j) {
			for (int i = 0; i < (int)area_.width(); ++i) {
				int idx = 0;
				data->get(i + (int)area_.xMin(), j + (int)area_.yMin(), idx);
				Colour col;
				palette->get(idx, col);
				tmp->set(i, j, col);
			}
		}

		if (!tmp->toBytes(val, type))
			return false;
	} else {
		if (!data->toBytes(val, type))
			return false;
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::parseImage(Image::Ptr &img, const Image* val, ParsingStatuses &status) const {
	// Prepare.
	img = nullptr;
	status = ParsingStatuses::SUCCESS;

	// Check the image.
	if (!val)
		return false;

	if ((val->width() % GBBASIC_TILE_SIZE) != 0 || (val->height() % GBBASIC_TILE_SIZE) != 0) {
		status = ParsingStatuses::NOT_A_MULTIPLE_OF_8x8;

		return false;
	}
	if (val->width() * val->height() > GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE * GBBASIC_TILES_MAX_AREA_SIZE) {
		status = ParsingStatuses::OUT_OF_BOUNDS;

		return false;
	}

	// Fill in an image.
	Image* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	img = Image::Ptr(ptr);

	if (img->paletted()) {
		Indexed::Lookup lookup;
		if (!palette->match(val, lookup))
			return false;

		img->resize(val->width(), val->height(), false);
		for (int j = 0; j < img->height(); ++j) {
			for (int i = 0; i < img->width(); ++i) {
				Colour col;
				val->get(i, j, col);
				const int idx = lookup[col];
				img->set(i, j, idx);
			}
		}
	} else {
		if (!img->fromImage(val))
			return false;
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::parseImage(Image::Ptr &img, const Bytes* val, ParsingStatuses &status) const {
	// Prepare.
	img = nullptr;
	status = ParsingStatuses::SUCCESS;

	// Convert from bytes.
	Image::Ptr tmp(Image::create());
	if (!tmp->fromBytes(val))
		return false;

	if ((tmp->width() % GBBASIC_TILE_SIZE) != 0 || (tmp->height() % GBBASIC_TILE_SIZE) != 0) {
		status = ParsingStatuses::NOT_A_MULTIPLE_OF_8x8;

		return false;
	}
	if (tmp->width() * tmp->height() > GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE * GBBASIC_TILES_MAX_AREA_SIZE) {
		status = ParsingStatuses::OUT_OF_BOUNDS;

		return false;
	}

	// Fill in an image.
	Image* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	img = Image::Ptr(ptr);

	if (img->paletted()) {
		Indexed::Lookup lookup;
		if (!palette->match(tmp.get(), lookup))
			return false;

		img->resize(tmp->width(), tmp->height(), false);
		for (int j = 0; j < img->height(); ++j) {
			for (int i = 0; i < img->width(); ++i) {
				Colour col;
				tmp->get(i, j, col);
				const int idx = lookup[col];
				img->set(i, j, idx);
			}
		}
	} else {
		if (!img->fromBytes(val))
			return false;
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the ref and pixels.
	rapidjson::Document doc;

	Jpath::set(doc, doc, ref, "ref");

	Jpath::set(doc, doc, name, "name");

	Jpath::set(doc, doc, magnification, "magnification");

	Jpath::set(doc, doc, Jpath::ANY(), "pixels");
	rapidjson::Value* pixels = nullptr;
	Jpath::get(doc, pixels, "pixels");

	rapidjson::Document doc_;
	if (!data->toJson(doc_)) {
		assetsRaiseWarningOrError("Cannot serialize tiles to JSON.", false, onWarningOrError);

		return false;
	}
	pixels->Set(doc_.GetObject());

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert tiles to JSON.", false, onWarningOrError);

		return false;
	}
	std::string str;
	if (!json->toString(str, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	val = str;

	// Finish.
	return true;
}

bool TilesAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert tiles from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the ref and pixels.
	if (!Jpath::get(doc, ref, "ref"))
		ref = 0;

	if (!Jpath::get(doc, magnification, "magnification"))
		magnification = -1;

	if (!Jpath::get(doc, name, "name"))
		name.clear();

	const rapidjson::Value* pixels = nullptr;
	if (!Jpath::get(doc, pixels, "pixels")) {
		assetsRaiseWarningOrError("Cannot find \"pixels\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!pixels || !pixels->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"pixels\" entry.", false, onWarningOrError);

		return false;
	}

	// Fill in with the pixels.
	if (!data->fromJson(*pixels)) {
		assetsRaiseWarningOrError("Cannot convert tiles from JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool TilesAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError) {
	const std::string val_(val, len);

	return fromString(val_, onWarningOrError);
}

bool TilesAssets::empty(void) const {
	return entries.empty();
}

int TilesAssets::count(void) const {
	return (int)entries.size();
}

bool TilesAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool TilesAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

const TilesAssets::Entry* TilesAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

TilesAssets::Entry* TilesAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const TilesAssets::Entry* TilesAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const TilesAssets::Entry* TilesAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			gotName = entry.name;

			return &entry;
		}

		if (entry.name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(entry.name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = entry.name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int TilesAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name)
			return i;
	}

	return -1;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Map assets
*/

MapAssets::Entry::Entry(int ref_, TilesAssets::Getter gettls, Texture::Ptr attribtex) :
	ref(ref_),
	getTiles(gettls)
{
	TilesAssets::Entry* ref__ = gettls(ref);
	ref__->refresh(); // Refresh the outdated editable resources.
	Texture::Ptr &tex = ref__->touch(); // Ensure the runtime resources are ready.
	Math::Vec2i count(tex->width() / GBBASIC_TILE_SIZE, tex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles tiles(tex, count);
	data = Map::Ptr(Map::create(&tiles, true));
	data->resize(GBBASIC_MAP_DEFAULT_WIDTH, GBBASIC_MAP_DEFAULT_HEIGHT);

	hasAttributes = false;
	Math::Vec2i attribcount(attribtex->width() / GBBASIC_TILE_SIZE, attribtex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles attribtiles(attribtex, attribcount);
	attributes = Map::Ptr(Map::create(&attribtiles, true));
	attributes->resize(GBBASIC_MAP_DEFAULT_WIDTH, GBBASIC_MAP_DEFAULT_HEIGHT);
}

MapAssets::Entry::Entry(const std::string &val, TilesAssets::Getter gettls, Texture::Ptr attribtex) :
	getTiles(gettls)
{
	data = Map::Ptr(Map::create(nullptr, true));

	hasAttributes = false;
	Math::Vec2i attribcount(attribtex->width() / GBBASIC_TILE_SIZE, attribtex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles attribtiles(attribtex, attribcount);
	attributes = Map::Ptr(Map::create(&attribtiles, true));

	fromString(val, nullptr);
}

size_t MapAssets::Entry::hash(void) const {
	size_t result = 0;

	if (data)
		result = Math::hash(result, data->hash());

	result = Math::hash(result, ref);

	result = Math::hash(result, hasAttributes);

	if (attributes)
		result = Math::hash(result, attributes->hash());

	return result;
}

int MapAssets::Entry::compare(const Entry &other) const {
	int ret = 0;

	if (data && other.data)
		ret = data->compare(other.data.get());
	if (ret != 0)
		return ret;

	if (ref < other.ref)
		return -1;
	if (ref > other.ref)
		return 1;

	if (!hasAttributes && other.hasAttributes)
		return -1;
	if (hasAttributes && !other.hasAttributes)
		return 1;

	if (attributes && other.attributes)
		ret = attributes->compare(attributes.get());
	if (ret != 0)
		return ret;

	return 0;
}

Texture::Ptr MapAssets::Entry::touch(void) {
	if (ref == -1) // This asset's dependency was lost, need to resolve.
		return nullptr;

	Map::Tiles tiles;
	if (data->tiles(tiles) && tiles.texture)
		return tiles.texture;

	TilesAssets::Entry* ref_ = getTiles(ref);
	if (!ref_)
		return nullptr;
	Texture::Ptr &tex = ref_->touch(); // TODO: ASSET LOCAL PALETTE
	if (!tex)
		return nullptr;

	Math::Vec2i count(tex->width() / GBBASIC_TILE_SIZE, tex->height() / GBBASIC_TILE_SIZE);
	tiles = Map::Tiles(tex, count);
	data->tiles(&tiles);

	return tiles.texture;
}

void MapAssets::Entry::cleanup(void) {
	data->cleanup();
	Map::Tiles tiles;
	if (data->tiles(tiles)) {
		tiles.texture = nullptr;
		data->tiles(&tiles);
	}

	attributes->cleanup();

	TilesAssets::Entry* ref_ = getTiles(ref);
	if (ref_)
		ref_->cleanup();
}

void MapAssets::Entry::cleanup(const Math::Recti &area) {
	data->cleanup(area);
}

bool MapAssets::Entry::refresh(void) {
	// Determine whether can refresh.
	TilesAssets::Entry* ref_ = getTiles(ref);
	if (!ref_)
		return false;

	// Determine whether need to refresh.
	if (
		isRefRevisionChanged(ref_) || // There is unsynchronized modification in this asset's dependency.
		ref_->refresh() // Refresh the outdated editable resources.
	) {
		// Refresh this asset.
		cleanup(); // Clean up the outdated editable and runtime resources.

		// Mark this asset synchronized.
		synchronizeRefRevision(ref_); // Mark as synchronized for this asset's dependency.

		// Increase this asset's revision.
		increaseRevision();

		// Finish.
		return true;
	}

	// Finish.
	return false;
}

int MapAssets::Entry::estimateFinalByteCount(void) const {
	int result = data->width() * data->height();
	if (hasAttributes)
		result += attributes->width() * attributes->height();

	return result;
}

bool MapAssets::Entry::serializeBasic(std::string &val, int page) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	(void)useLocalPalette; // TODO: ASSET LOCAL PALETTE

	std::string asset;
	if (name.empty())
		asset = "#" + Text::toString(page);
	else
		asset = "\"" + name + "\"";

	const TilesAssets::Entry* tiles = getTiles(ref);
	const int w = data->width();
	const int h = data->height();
	if (hasAttributes) {
		val += "map on";
		val += "\n";
		if (tiles) {
			const Image::Ptr &tilesData = tiles->data;
			const int n = (tilesData->width() / GBBASIC_TILE_SIZE) * (tilesData->height() / GBBASIC_TILE_SIZE);
			val += "fill tile(";
			val += Text::toString(0);
			val += ", ";
			val += Text::toString(n);
			val += ") = ";
			val += asset;
			val += "\n";
		}
		val += "option VRAM_USAGE, VRAM_ATTRIBUTES";
		val += "\n";
		val += "def map(";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(w);
		val += ", ";
		val += Text::toString(h);
		val += ") = ";
		val += asset + ":1";
		val += "\n";
		val += "option VRAM_USAGE, VRAM_TILES";
		val += "\n";
		val += "def map(";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(w);
		val += ", ";
		val += Text::toString(h);
		val += ") = ";
		val += asset;
		val += "\n";
		val += "map 0, 0";
		val += "\n";
	} else {
		val += "map on";
		val += "\n";
		if (tiles) {
			const Image::Ptr &tilesData = tiles->data;
			const int n = (tilesData->width() / GBBASIC_TILE_SIZE) * (tilesData->height() / GBBASIC_TILE_SIZE);
			val += "fill tile(";
			val += Text::toString(0);
			val += ", ";
			val += Text::toString(n);
			val += ") = ";
			val += asset;
			val += "\n";
		}
		val += "def map(";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(0);
		val += ", ";
		val += Text::toString(w);
		val += ", ";
		val += Text::toString(h);
		val += ") = ";
		val += asset;
		val += "\n";
		val += "map 0, 0";
		val += "\n";
	}

	// Finish.
	return true;
}

bool MapAssets::Entry::serializeDataSequence(std::string &val, int base, int layer) const {
	// Prepare.
	val.clear();

	const Map::Ptr &lyr = layer == ASSETS_MAP_GRAPHICS_LAYER ? data : attributes;

	// Serialize the pixels.
	for (int j = 0; j < lyr->height(); ++j) {
		val += "data ";
		for (int i = 0; i < lyr->width(); ++i) {
			const int cel = lyr->get(i, j);
			if (base == 16) {
				val += "0x" + Text::toHex(cel, 2, '0', false);
			} else if (base == 10) {
				val += Text::toString(cel);
			} else if (base == 2) {
				val += "0b" + Text::toBin((Byte)cel);
			} else {
				GBBASIC_ASSERT(false && "Not supported.");
			}

			if (i != lyr->width() - 1)
				val += ", ";
		}
		val += "\n";
	}

	// Finish.
	return true;
}

bool MapAssets::Entry::parseDataSequence(Map::Ptr &map, const std::string &val, int layer) const {
	// Prepare.
	typedef std::vector<Int64> Data;

	// Parse the pixels.
	Data bytes;
	std::string val_ = val;
	val_ = Text::replace(val_, "\r\n", ",");
	val_ = Text::replace(val_, "\r", ",");
	val_ = Text::replace(val_, "\n", ",");
	val_ = Text::replace(val_, "\t", ",");
	val_ = Text::replace(val_, " ", ",");
	Text::Array tokens = Text::split(val_, ",");
	val_.clear();
	for (std::string tk : tokens) {
		tk = Text::trim(tk);
		if (tk.empty())
			continue;

		int cel = 0;
		if (!Text::fromString(tk, cel))
			continue;

		bytes.push_back(cel);
	}

	// Fill in a map.
	const Map::Ptr &lyr = layer == ASSETS_MAP_GRAPHICS_LAYER ? data : attributes;

	Map* ptr = nullptr;
	if (!lyr->clone(&ptr, false))
		return false;
	map = Map::Ptr(ptr);

	Map::Tiles tiles;
	if (!map->tiles(tiles))
		return false;
	const int n = tiles.count.x * tiles.count.y;

	int k = 0;
	for (int j = 0; j < map->height(); ++j) {
		for (int i = 0; i < map->width(); ++i) {
			Int64 num = bytes[k++];
			num = Math::clamp(num, (Int64)0, (Int64)n - 1);
			map->set(i, j, (int)num);

			if (k >= (int)bytes.size())
				break;
		}

		if (k >= (int)bytes.size())
			break;
	}

	// Finish.
	return true;
}

bool MapAssets::Entry::serializeJson(std::string &val, bool pretty) const {
	// Prepare.
	val.clear();

	// Serialize the tiles.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "tiles");
	rapidjson::Value* tiles = nullptr;
	Jpath::get(doc, tiles, "tiles");

	Jpath::set(doc, doc, hasAttributes, "has_attributes");

	Jpath::set(doc, doc, Jpath::ANY(), "attributes");
	rapidjson::Value* attributes_ = nullptr;
	Jpath::get(doc, attributes_, "attributes");

	rapidjson::Document doc0, doc1;
	if (!data->toJson(doc0) || !attributes->toJson(doc1))
		return false;
	tiles->Set(doc0.GetObject());
	attributes_->Set(doc1.GetObject());

	// Convert to string.
	std::string str;
	if (!Json::toString(doc, str, pretty))
		return false;

	val = str;

	// Finish.
	return true;
}

bool MapAssets::Entry::parseJson(Map::Ptr &map, bool &hasAttrib, Map::Ptr &attrib, const std::string &val, ParsingStatuses &status) const {
	// Prepare.
	map = nullptr;
	hasAttrib = hasAttributes;
	attrib = nullptr;
	status = ParsingStatuses::SUCCESS;

	// Convert from string.
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	if (!doc.IsObject())
		return false;

	// Parse the tiles.
	const rapidjson::Value* tiles = nullptr;
	const rapidjson::Value* attributes_ = nullptr;
	if (!Jpath::get(doc, tiles, "tiles") || !Jpath::get(doc, attributes_, "attributes"))
		return false;
	if (!tiles || !tiles->IsObject() || !attributes_ || !attributes_->IsObject())
		return false;

	int w = 0;
	int h = 0;
	Jpath::get(*tiles, w, "width");
	Jpath::get(*tiles, h, "height");
	if (w * h > GBBASIC_MAP_MAX_AREA_SIZE) {
		status = ParsingStatuses::OUT_OF_BOUNDS;

		return false;
	}

	Jpath::get(doc, hasAttrib, "has_attributes");

	// Fill in a map.
	Map* ptr = nullptr;
	if (!data->clone(&ptr, false))
		return false;
	map = Map::Ptr(ptr);

	ptr = nullptr;
	if (!attributes->clone(&ptr, false))
		return false;
	attrib = Map::Ptr(ptr);

	if (!map->fromJson(*tiles, nullptr))
		return false;
	if (!attrib->fromJson(*attributes_, nullptr))
		return false;

	if (data->width() != attributes->width() || data->height() != attributes->height())
		attributes->resize(data->width(), data->height());

	Map::Tiles attribtiles;
	attributes->tiles(attribtiles);
	Math::Vec2i attribcount(attribtiles.texture->width() / GBBASIC_TILE_SIZE, attribtiles.texture->height() / GBBASIC_TILE_SIZE);
	if (attribtiles.count != attribcount) {
		attribtiles.count = attribcount;
		attributes->tiles(&attribtiles);
	}

	// Finish.
	return true;
}

bool MapAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the ref and tiles.
	rapidjson::Document doc;

	Jpath::set(doc, doc, ref, "ref");

	Jpath::set(doc, doc, Jpath::ANY(), "tiles");
	rapidjson::Value* tiles = nullptr;
	Jpath::get(doc, tiles, "tiles");

	Jpath::set(doc, doc, hasAttributes, "has_attributes");

	Jpath::set(doc, doc, Jpath::ANY(), "attributes");
	rapidjson::Value* attributes_ = nullptr;
	Jpath::get(doc, attributes_, "attributes");

	Jpath::set(doc, doc, useLocalPalette, "use_local_palette");

	(void)localPalette; // TODO: ASSET LOCAL PALETTE

	Jpath::set(doc, doc, name, "name");

	Jpath::set(doc, doc, magnification, "magnification");

	Jpath::set(doc, doc, optimize, "optimize");

	rapidjson::Document doc0, doc1;
	if (!data->toJson(doc0) || !attributes->toJson(doc1)) {
		assetsRaiseWarningOrError("Cannot serialize map to JSON.", false, onWarningOrError);

		return false;
	}
	tiles->Set(doc0.GetObject());
	attributes_->Set(doc1.GetObject());

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert map to JSON.", false, onWarningOrError);

		return false;
	}
	std::string str;
	if (!json->toString(str, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	val = str;

	// Finish.
	return true;
}

bool MapAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert map from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the ref and tiles.
	if (!Jpath::get(doc, ref, "ref"))
		ref = 0;

	const rapidjson::Value* tiles = nullptr;
	if (!Jpath::get(doc, tiles, "tiles")) {
		assetsRaiseWarningOrError("Cannot find \"tiles\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!tiles || !tiles->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"tiles\" entry.", false, onWarningOrError);

		return false;
	}

	const rapidjson::Value* attributes_ = nullptr;
	if (!Jpath::get(doc, attributes_, "attributes")) {
		assetsRaiseWarningOrError("Cannot find \"attributes\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!attributes_ || !attributes_->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"attributes\" entry.", false, onWarningOrError);

		return false;
	}

	Jpath::get(doc, hasAttributes, "has_attributes");

	if (!Jpath::get(doc, useLocalPalette, "use_local_palette"))
		useLocalPalette = false;

	(void)localPalette; // TODO: ASSET LOCAL PALETTE

	if (!Jpath::get(doc, name, "name"))
		name.clear();

	if (!Jpath::get(doc, magnification, "magnification"))
		magnification = -1;

	if (!Jpath::get(doc, optimize, "optimize"))
		optimize = true;

	// Fill in with the tiles.
	TilesAssets::Entry* entry = getTiles(ref);
	if (!entry) {
		assetsRaiseWarningOrError("Invalid referenced tiles.", false, onWarningOrError);

		return false;
	}
	entry->refresh(); // Refresh the outdated editable resources.
	Texture::Ptr &tex = entry->touch(); // Ensure the runtime resources are ready.
	if (!tex) {
		assetsRaiseWarningOrError("Invalid texture of referenced tiles.", false, onWarningOrError);

		return false;
	}

	Map::Tiles attribtiles;
	attributes->tiles(attribtiles);
	if (!data->fromJson(*tiles, tex)) {
		assetsRaiseWarningOrError("Cannot convert map from JSON.", false, onWarningOrError);

		return false;
	}
	if (!attributes->fromJson(*attributes_, attribtiles.texture)) {
		attributes->resize(data->width(), data->height());
	}

	if (data->width() != attributes->width() || data->height() != attributes->height())
		attributes->resize(data->width(), data->height());

	Math::Vec2i attribcount(attribtiles.texture->width() / GBBASIC_TILE_SIZE, attribtiles.texture->height() / GBBASIC_TILE_SIZE);
	if (attribtiles.count != attribcount) {
		attribtiles.count = attribcount;
		attributes->tiles(&attribtiles);
	}

	// Finish.
	return true;
}

bool MapAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError) {
	const std::string val_(val, len);

	return fromString(val_, onWarningOrError);
}

MapAssets::Slice::Slice() {
}

MapAssets::Slice::Slice(Image::Ptr img) :
	image(img)
{
	GBBASIC_ASSERT(img && "Wrong data.");

	hash = image->hash();
}

bool MapAssets::Slice::operator == (const Slice &other) const {
	return equals(other);
}

bool MapAssets::Slice::operator != (const Slice &other) const {
	return !equals(other);
}

bool MapAssets::Slice::equals(const Slice &other) const {
	if (hash != other.hash)
		return false;

	return image->compare(other.image.get()) == 0;
}

bool MapAssets::empty(void) const {
	return entries.empty();
}

int MapAssets::count(void) const {
	return (int)entries.size();
}

bool MapAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool MapAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

const MapAssets::Entry* MapAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

MapAssets::Entry* MapAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const MapAssets::Entry* MapAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const MapAssets::Entry* MapAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			gotName = entry.name;

			return &entry;
		}

		if (entry.name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(entry.name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = entry.name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int MapAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name)
			return i;
	}

	return -1;
}

bool MapAssets::serializeImage(const TilesAssets::Entry &tiles_, const MapAssets::Entry &map_, Image* val) {
	// Prepare.
	if (!val)
		return false;

	if (!tiles_.data)
		return false;

	if (!map_.data)
		return false;

	const int tw = map_.data->width();
	const int th = map_.data->height();

	// Serialize.
	val->fromBlank(tw * GBBASIC_TILE_SIZE, th * GBBASIC_TILE_SIZE, 0);
	for (int j = 0; j < th; ++j) {
		for (int i = 0; i < tw; ++i) {
			const int cel = map_.data->get(i, j);
			const std::div_t div = std::div(cel, tiles_.data->width() / GBBASIC_TILE_SIZE);
			const int x = div.rem;
			const int y = div.quot;

			Image::Ptr tmp(Image::create());
			const Math::Recti area = Math::Recti::byXYWH(x * GBBASIC_TILE_SIZE, y * GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE);
			tiles_.serializeImage(tmp.get(), &area);
			bool hFlip = false;
			bool vFlip = false;
			if (map_.hasAttributes && map_.attributes) {
				const int attrs = map_.attributes->get(i, j);
				hFlip = !!((attrs >> GBBASIC_MAP_HFLIP_BIT) & 0x00000001);
				vFlip = !!((attrs >> GBBASIC_MAP_VFLIP_BIT) & 0x00000001);
			}
			tmp->blit(val, i * GBBASIC_TILE_SIZE, j * GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE, 0, 0, hFlip, vFlip);
		}
	}

	// Finish.
	return true;
}

bool MapAssets::parseImage(
	TilesAssets::Entry &tiles_, MapAssets::Entry &map_,
	const Image* val, bool allowFlip,
	bool allowReuse,
	PaletteAssets::Getter getplt,
	TilesAssets::Getter gettls, int tilesPageCount,
	PrintHandler onPrint, WarningOrErrorHandler onWarningOrError
) {
	// Prepare.
	struct Cel {
		typedef std::vector<Cel> Array;

		int index = -1;
		bool hFlip = false;
		bool vFlip = false;

		Cel() {
		}
		Cel(int idx, bool hFlip_, bool vFlip_) : index(idx), hFlip(hFlip_), vFlip(vFlip_) {
		}
	};

	auto indexOfSlice = [] (const Slice::Array &coll, const Slice &what, bool allowFlip, bool &hFlip, bool &vFlip) -> int {
		for (int i = 0; i < (int)coll.size(); ++i) {
			if (coll[i] == what) // Compare the slices' images.
				return i;

			if (!allowFlip)
				continue;

			Image* ptr = nullptr;
			what.image->clone(&ptr);
			Image::Ptr tmp(ptr);
			tmp->flip(true, false);
			if (coll[i] == Slice(tmp)) {
				hFlip = true;

				return i;
			}

			tmp->flip(false, true);
			if (coll[i] == Slice(tmp)) {
				hFlip = true;
				vFlip = true;

				return i;
			}

			tmp->flip(true, false);
			if (coll[i] == Slice(tmp)) {
				vFlip = true;

				return i;
			}
		}

		return -1;
	};

	// Convert the true-color image to paletted.
	Image::Ptr paletted(val->quantized2Bpp());

	// TODO: EDIT MAP AS IMAGE
	// CALCULATE PALETTE

	// Slice the image.
	Slice::Array slices;
	Cel::Array cels;
	int flipped = 0;

	const std::div_t dx = std::div(paletted->width(), GBBASIC_TILE_SIZE);
	const std::div_t dy = std::div(paletted->height(), GBBASIC_TILE_SIZE);
	if (dx.rem || dy.rem) {
		if (onWarningOrError)
			onWarningOrError("Image size is not a multiple of 8.", true);
	}
	const int mw = dx.quot + (dx.rem ? 1 : 0);
	const int mh = dy.quot + (dy.rem ? 1 : 0);
	const int area = mw * mh;
	if (mw > GBBASIC_MAP_MAX_WIDTH || mh > GBBASIC_MAP_MAX_HEIGHT || area > GBBASIC_MAP_MAX_AREA_SIZE) {
		if (onWarningOrError)
			onWarningOrError("Image size out of bounds for map.", false);

		return false;
	}

	cels.resize(area);
	for (int j = 0; j < mh; j += 2) {
		for (int i = 0; i < mw; ++i) {
			const int x = i;
			for (int h = 0; h < 2; ++h) { // Import as 8x16 if possible.
				const int y = j + h;
				if (y >= mh)
					continue;

				Image::Ptr s(Image::create(paletted->palette()));
				s->fromBlank(GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE, GBBASIC_PALETTE_DEPTH);
				paletted->blit(
					s.get(),
					0, 0,
					GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
					x * GBBASIC_TILE_SIZE, y * GBBASIC_TILE_SIZE
				);
				Slice slice(s);

				int index = -1;
				bool hFlip = false;
				bool vFlip = false;
				const int exists = indexOfSlice(slices, slice, allowFlip, hFlip, vFlip);
				if (exists == -1) {
					slices.push_back(slice);
					index = (int)slices.size() - 1;
				} else {
					index = exists;
				}
				cels[x + y * mw] = Cel(index, hFlip, vFlip);

				if (hFlip || vFlip)
					++flipped;
			}
		}
	}

	// Determine the tiles asset size.
	int tw = (int)std::ceil(std::sqrt(slices.size()));
	int th = (int)std::ceil((float)slices.size() / tw);
	const int min = tw / 3;
	int waste = std::numeric_limits<int>::max();
	for (int i = Math::max(min, 1); i < (int)slices.size() - min; ++i) {
		const int tw_ = i;
		const int th_ = (int)std::ceil((float)slices.size() / tw_);
		const int waste_ = tw_ * th_ - (int)slices.size();
		if (waste_ < waste) { // Less waste.
			waste = waste_;
			tw = tw_;
			th = th_;
		}
	}

	if (slices.size() > GBBASIC_TILES_MAX_AREA_SIZE || tw * th > GBBASIC_TILES_MAX_AREA_SIZE) {
		if (onWarningOrError)
			onWarningOrError("Image size out of bounds for tiles.", false);

		return false;
	}

	// Fill the tiles asset.
	tiles_.data->fromBlank(tw * GBBASIC_TILE_SIZE, th * GBBASIC_TILE_SIZE, GBBASIC_PALETTE_DEPTH);
	int k = 0;
	for (int j = 0; j < th; ++j) {
		for (int i = 0; i < tw; ++i) {
			if (k >= (int)slices.size())
				break;

			const Slice &slice = slices[k++];
			slice.image->blit(
				tiles_.data.get(),
				i * GBBASIC_TILE_SIZE, j * GBBASIC_TILE_SIZE,
				GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
				0, 0
			);
		}

		if (k >= (int)slices.size())
			break;
	}

	TilesAssets::Entry* ref__ = &tiles_;

	map_.ref = tilesPageCount; // Pre-assign with the new tiles page.
	if (allowReuse) {
		const size_t thash = tiles_.data->hash();
		for (int i = 0; i < tilesPageCount; ++i) {
			TilesAssets::Entry* tilesEntry = gettls(i);
			if (!tilesEntry || !tilesEntry->data || tilesEntry->data->hash() != thash)
				continue;

			if (tiles_.data->compare(tilesEntry->data.get()) != 0) // Compare the pixels only.
				continue;

			ref__ = tilesEntry; // Reuse the identical tiles asset.
			tiles_.data = nullptr; // No new tiles data has been generated.
			map_.ref = i; // Reference to the reused tiles page index.

			if (onPrint) {
				const std::string msg = Text::format("Reused tiles page {0}.", { Text::toString(i) });

				onPrint(msg.c_str());
			}

			break;
		}
	}

	ref__->refresh(); // Refresh the outdated editable resources.
	Texture::Ptr &tex = ref__->touch(); // Ensure the runtime resources are ready.

	// Fill the map asset.
	Math::Vec2i count(tw, th);
	Map::Tiles tiles(tex, count);
	map_.data = Map::Ptr(Map::create(&tiles, true));
	map_.data->resize(mw, mh);

	map_.hasAttributes = !!flipped;
	map_.attributes->resize(mw, mh);

	k = 0;
	for (int j = 0; j < mh; ++j) {
		for (int i = 0; i < mw; ++i) {
			const Cel &cel = cels[k++];
			map_.data->set(i, j, cel.index);

			int attrs = 0;
			if (cel.hFlip) {
				attrs |= (1 << GBBASIC_MAP_HFLIP_BIT);
			}
			if (cel.vFlip) {
				attrs |= (1 << GBBASIC_MAP_VFLIP_BIT);
			}
			map_.attributes->set(i, j, attrs);
		}
	}

	// Finish.
	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Audio assets
*/

Tracker::Location::Location() {
}

Tracker::Location::Location(int ch, int ln) : channel(ch), line(ln) {
}

Tracker::Location Tracker::Location::INVALID(void) {
	return Location(-1, -1);
}

Tracker::Cursor::Cursor() {
}

Tracker::Cursor::Cursor(int seq, int ch, int ln, DataTypes d) : sequence(seq), channel(ch), line(ln), data(d) {
}

Tracker::Cursor::Cursor(const Cursor &other) {
	sequence = other.sequence;
	channel = other.channel;
	line = other.line;
	data = other.data;
}

Tracker::Cursor &Tracker::Cursor::operator = (const Cursor &other) {
	sequence = other.sequence;
	channel = other.channel;
	line = other.line;
	data = other.data;

	return *this;
}

bool Tracker::Cursor::operator == (const Cursor &other) const {
	return equals(other);
}

bool Tracker::Cursor::operator != (const Cursor &other) const {
	return !equals(other);
}

bool Tracker::Cursor::operator < (const Cursor &other) const {
	return compare(other) < 0;
}

bool Tracker::Cursor::operator <= (const Cursor &other) const {
	return compare(other) <= 0;
}

bool Tracker::Cursor::operator > (const Cursor &other) const {
	return compare(other) > 0;
}

bool Tracker::Cursor::operator >= (const Cursor &other) const {
	return compare(other) >= 0;
}

int Tracker::Cursor::compare(const Cursor &other) const {
	if (sequence < other.sequence)
		return -1;
	else if (sequence > other.sequence)
		return 1;

	if (line < other.line)
		return -1;
	else if (line > other.line)
		return 1;

	if (channel < other.channel)
		return -1;
	else if (channel > other.channel)
		return 1;

	if (data < other.data)
		return -1;
	else if (data > other.data)
		return 1;

	return 0;
}

bool Tracker::Cursor::equals(const Cursor &other) const {
	return compare(other) == 0;
}

Tracker::Cursor Tracker::Cursor::nextData(void) const {
	Cursor result = *this;
	int data_ = (int)result.data;
	if (++data_ < (int)DataTypes::COUNT) {
		result.data = (DataTypes)data_;

		return result;
	}
	result.data = DataTypes::NOTE;

	if (++result.channel < GBBASIC_MUSIC_CHANNEL_COUNT) {
		return result;
	}
	result.channel = 0;

	if (++result.line < GBBASIC_MUSIC_PATTERN_COUNT) {
		return result;
	}
	result.line = 0;

	return INVALID();
}

Tracker::Cursor Tracker::Cursor::nextLine(void) const {
	Cursor result = *this;
	if (++result.line < GBBASIC_MUSIC_PATTERN_COUNT) {
		return result;
	}
	result.line = 0;

	return INVALID();
}

bool Tracker::Cursor::valid(void) const {
	if (sequence == -1)
		return false;
	if (channel == -1)
		return false;
	if (line == -1)
		return false;

	return true;
}

void Tracker::Cursor::set(int seq, int ch, int ln, DataTypes d) {
	sequence = seq;
	channel = ch;
	line = ln;
	data = d;
}

void Tracker::Cursor::set(int ch, int ln, DataTypes d) {
	channel = ch;
	line = ln;
	data = d;
}

Tracker::Cursor Tracker::Cursor::INVALID(void) {
	return Cursor(-1, -1, -1, DataTypes::NOTE);
}

Tracker::Range::Range() {
}

void Tracker::Range::start(int seq, int ch, int ln, DataTypes data) {
	first.set(seq, ch, ln, data);
}

void Tracker::Range::start(const Cursor &where) {
	first.set(where.sequence, where.channel, where.line, where.data);
}

void Tracker::Range::end(int seq, int ch, int ln, DataTypes data) {
	second.set(seq, ch, ln, data);
}

void Tracker::Range::end(const Cursor &where) {
	second.set(where.sequence, where.channel, where.line, where.data);
}

Tracker::Cursor Tracker::Range::min(void) const {
	if (first.sequence != second.sequence)
		return Cursor::INVALID();

	constexpr const int _ = 0;
	const Cursor left(_, first.channel, _, first.data);
	const Cursor right(_, second.channel, _, second.data);
	Cursor result = Math::min(left, right);
	result.sequence = first.sequence;
	result.line = Math::min(first.line, second.line);

	return result;
}

Tracker::Cursor Tracker::Range::max(void) const {
	if (first.sequence != second.sequence)
		return Cursor::INVALID();

	constexpr const int _ = 0;
	const Cursor left(_, first.channel, _, first.data);
	const Cursor right(_, second.channel, _, second.data);
	Cursor result = Math::max(left, right);
	result.sequence = first.sequence;
	result.line = Math::max(first.line, second.line);

	return result;
}

Math::Vec2i Tracker::Range::size(void) const {
	if (first == Cursor::INVALID() || second == Cursor::INVALID())
		return Math::Vec2i(0, 0);

	int a = 0, b = 0, c = 0;

	a = Math::max(max().channel - min().channel - 2 + 1, 0) * 4;
	if (max().channel == min().channel) {
		b = ((int)max().data - (int)min().data) + 1;
	} else {
		b = (int)DataTypes::COUNT - (int)min().data;
		c = (int)max().data + 1;
	}

	const Math::Vec2i result(a + b + c, max().line - min().line + 1);

	return result;
}

bool Tracker::Range::startsWith(int ch, int ln, DataTypes data) const {
	return
		first.channel == ch &&
		first.line == ln &&
		first.data == data;
}

bool Tracker::Range::startsWith(const Cursor &where) const {
	return
		first.sequence == where.sequence &&
		first.channel == where.channel &&
		first.line == where.line &&
		first.data == where.data;
}

bool Tracker::Range::endsWith(int ch, int ln, DataTypes data) const {
	return
		second.channel == ch &&
		second.line == ln &&
		second.data == data;
}

bool Tracker::Range::endsWith(const Cursor &where) const {
	return
		second.sequence == where.sequence &&
		second.channel == where.channel &&
		second.line == where.line &&
		second.data == where.data;
}

bool Tracker::Range::single(void) const {
	if (first == Cursor::INVALID())
		return false;

	return first == second;
}

bool Tracker::Range::invalid(void) const {
	return first == Cursor::INVALID() || second == Cursor::INVALID();
}

void Tracker::Range::clear(void) {
	first = Cursor::INVALID();
	second = Cursor::INVALID();
}

MusicAssets::Entry::Entry(void) {
	data = Music::Ptr(Music::create());

	Music::Song* song = data->pointer();
	Music::Channels &channels = song->channels;
	for (int i = 0; i < (int)channels.size(); ++i) {
		Music::Sequence &seq = channels[i];
		seq.push_back(0);
	}
}

MusicAssets::Entry::Entry(const std::string &val) {
	data = Music::Ptr(Music::create());

	Music::Song* song = data->pointer();
	Music::Channels &channels = song->channels;
	for (int i = 0; i < (int)channels.size(); ++i) {
		Music::Sequence &seq = channels[i];
		seq.push_back(0);
	}

	fromString(val, nullptr);
}

size_t MusicAssets::Entry::hash(void) const {
	size_t result = 0;

	if (data)
		result = Math::hash(result, data->hash());

	return result;
}

int MusicAssets::Entry::compare(const Entry &other) const {
	int ret = 0;

	if (data && other.data)
		ret = data->compare(other.data.get());
	if (ret != 0)
		return ret;

	return 0;
}

int MusicAssets::Entry::estimateFinalByteCount(void) const {
	// Prepare.
	typedef std::vector<Music::Pattern> FilledPatterns;

	FilledPatterns filledPatterns;

	int result = 0;
	const Music::Song* song = data->pointer();
	const Music::Orders &orders = song->orders;
	const Music::Song::OrderMatrix ordMat = song->orderMatrix();
	const Music::DutyInstrument::Array &dutyInsts = song->dutyInstruments;
	const Music::WaveInstrument::Array &waveInsts = song->waveInstruments;
	const Music::NoiseInstrument::Array &noiseInsts = song->noiseInstruments;
	const Music::Waves &waves = song->waves;

	// Order count.
	result += 1;

	// Patterns.
	for (int i = 0; i < (int)ordMat.size(); ++i) {
		const Music::Order &order = orders[i];
		const Music::Song::OrderSet &ordSet = ordMat[i];
		for (int ord : ordSet) {
			const Music::Pattern &pattern = order[ord];
			FilledPatterns::const_iterator pit = std::find(filledPatterns.begin(), filledPatterns.end(), pattern); // Ignore for duplicate pattern.
			if (pit == filledPatterns.end()) {
				result += MUSIC_PATTERN_SIZE;
				filledPatterns.push_back(pattern);
			} else {
				// Do nothing.
			}
		}
	}

	// Orders.
	for (int i = 0; i < (int)ordMat.size(); ++i) {
		const Music::Song::OrderSet &ordSet = ordMat[i];
		result += (int)ordSet.size() * 2;
	}

	// Instruments.
	result += (int)dutyInsts.size() * 4;
	result += (int)waveInsts.size() * 4;
	for (int i = 0; i < (int)noiseInsts.size(); ++i) {
		const Music::NoiseInstrument &inst = noiseInsts[i];
		const int n = Math::min((inst.bitCount == Music::NoiseInstrument::BitCount::_7 ? 7 : 15), (int)inst.noiseMacro.size());
		result += 2 + n;
	}

	// Waves.
	for (int i = 0; i < (int)waves.size(); ++i) {
		const Music::Wave &wave = waves[i];
		result += (int)wave.size() / 2;
	}

	// Song.
	result += 21;

	// Finish.
	return result;
}

bool MusicAssets::Entry::serializeBasic(std::string &val, int page) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	const Music::Ptr &ptr = data;
	const Music::Song* song = ptr->pointer();
	std::string asset;
	if (song->name.empty())
		asset = "#" + Text::toString(page);
	else
		asset = "\"" + song->name + "\"";

	val += "sound on";
	val += "\n";
	val += "play ";
	val += asset;
	val += "\n";

	// Finish.
	return true;
}

bool MusicAssets::Entry::serializeC(std::string &val) const {
	return data->toC(val);
}

bool MusicAssets::Entry::parseC(Music::Ptr &music, const char* filename, const std::string &val, Music::ErrorHandler onError) const {
	// Prepare.
	music = nullptr;

	// Fill in music.
	Music* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	music = Music::Ptr(ptr);

	if (!music->fromC(filename, val, onError))
		return false;

	// Finish.
	return true;
}

bool MusicAssets::Entry::serializeJson(std::string &val, bool pretty) const {
	// Prepare.
	val.clear();

	// Serialize the data.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "song");
	rapidjson::Value* song = nullptr;
	Jpath::get(doc, song, "song");

	rapidjson::Document doc_;
	if (!data->toJson(doc_))
		return false;
	song->Set(doc_.GetObject());

	// Convert to string.
	std::string str;
	if (!Json::toString(doc, str, pretty))
		return false;

	val = str;

	// Finish.
	return true;
}

bool MusicAssets::Entry::parseJson(Music::Ptr &music, const std::string &val) const {
	// Prepare.
	music = nullptr;

	// Convert from string.
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	if (!doc.IsObject())
		return false;

	// Parse the data.
	const rapidjson::Value* song = nullptr;
	if (!Jpath::get(doc, song, "song"))
		return false;
	if (!song || !song->IsObject())
		return false;

	// Fill in music.
	Music* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	music = Music::Ptr(ptr);

	if (!music->fromJson(*song))
		return false;

	// Finish.
	return true;
}

bool MusicAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the notes.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "song");
	rapidjson::Value* song = nullptr;
	Jpath::get(doc, song, "song");

	Jpath::set(doc, doc, magnification, "magnification");

	Jpath::set(doc, doc, (int)lastView, "last_view");

	Jpath::set(doc, doc, (int)lastNote, "last_note");

	Jpath::set(doc, doc, (int)lastOctave, "last_octave");

	rapidjson::Document doc_;
	if (!data->toJson(doc_)) {
		assetsRaiseWarningOrError("Cannot serialize music to JSON.", false, onWarningOrError);

		return false;
	}
	song->Set(doc_.GetObject());

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert music to JSON.", false, onWarningOrError);

		return false;
	}
	std::string str;
	if (!json->toString(str, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	val = str;

	// Finish.
	return true;
}

bool MusicAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert music from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the notes.
	const rapidjson::Value* song = nullptr;
	if (!Jpath::get(doc, song, "song")) {
		assetsRaiseWarningOrError("Cannot find \"song\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!song || !song->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"song\" entry.", false, onWarningOrError);

		return false;
	}

	if (!Jpath::get(doc, magnification, "magnification"))
		magnification = -1;

	int view = -1;
	if (!Jpath::get(doc, view, "last_view"))
		lastView = Tracker::ViewTypes::TRACKER;

	if (!Jpath::get(doc, lastNote, "last_note"))
		lastNote = 0;

	if (!Jpath::get(doc, lastOctave, "last_octave"))
		lastOctave = 0;

	// Fill in with the notes.
	if (!data->fromJson(*song)) {
		assetsRaiseWarningOrError("Cannot convert music from JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool MusicAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError) {
	const std::string val_(val, len);

	return fromString(val_, onWarningOrError);
}

bool MusicAssets::empty(void) const {
	return entries.empty();
}

int MusicAssets::count(void) const {
	return (int)entries.size();
}

bool MusicAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool MusicAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

const MusicAssets::Entry* MusicAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

MusicAssets::Entry* MusicAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const MusicAssets::Entry* MusicAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		const Music::Ptr &ptr = entry.data;
		const Music::Song* song = ptr->pointer();
		if (song->name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const MusicAssets::Entry* MusicAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		const Music::Ptr &ptr = entry.data;
		const Music::Song* song = ptr->pointer();
		if (song->name == name) {
			if (index)
				*index = i;

			gotName = song->name;

			return &entry;
		}

		if (song->name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(song->name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = song->name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int MusicAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		const Music::Ptr &ptr = entry.data;
		const Music::Song* song = ptr->pointer();
		if (song->name == name)
			return i;
	}

	return -1;
}

SfxAssets::Entry::Entry(void) {
	data = Sfx::Ptr(Sfx::create());
}

SfxAssets::Entry::Entry(const std::string &val) {
	data = Sfx::Ptr(Sfx::create());

	fromString(val, nullptr);
}

SfxAssets::Entry::Entry(const Sfx::Sound &val) {
	data = Sfx::Ptr(Sfx::create());

	*data->pointer() = val;
}

size_t SfxAssets::Entry::hash(void) const {
	size_t result = 0;

	if (data)
		result = Math::hash(result, data->hash());

	return result;
}

int SfxAssets::Entry::compare(const Entry &other) const {
	int ret = 0;

	if (data && other.data)
		ret = data->compare(other.data.get());
	if (ret != 0)
		return ret;

	return 0;
}

int SfxAssets::Entry::estimateFinalByteCount(void) const {
	int result = 0;
	const Sfx::Sound* sound = data->pointer();
	result += sizeof(Sfx::Mask);
	for (int j = 0; j < (int)sound->rows.size(); ++j) {
		const Sfx::Row &row = sound->rows[j];
		result += sizeof(UInt8); // Count.
		for (int i = 0; i < (int)row.cells.size(); ++i) {
			const Sfx::Cell &cell = row.cells[i];
			result += sizeof(Sfx::Command);
			result += sizeof(Sfx::Register) * (int)cell.registers.size();
		}
	}

	return result;
}

bool SfxAssets::Entry::serializeBasic(std::string &val, int page) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	const Sfx::Ptr &ptr = data;
	const Sfx::Sound* snd = ptr->pointer();
	std::string asset;
	if (snd->name.empty())
		asset = "#" + Text::toString(page);
	else
		asset = "\"" + snd->name + "\"";

	val += "sound on";
	val += "\n";
	val += "sound ";
	val += asset;
	val += "\n";

	// Finish.
	return true;
}

bool SfxAssets::Entry::serializeJson(std::string &val, bool pretty) const {
	// Prepare.
	val.clear();

	// Serialize the data.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "sound");
	rapidjson::Value* sound = nullptr;
	Jpath::get(doc, sound, "sound");

	rapidjson::Document doc_;
	if (!data->toJson(doc_))
		return false;
	sound->Set(doc_.GetObject());

	// Convert to string.
	std::string str;
	if (!Json::toString(doc, str, pretty))
		return false;

	val = str;

	// Finish.
	return true;
}

bool SfxAssets::Entry::parseJson(Sfx::Ptr &sfx, const std::string &val) const {
	// Prepare.
	sfx = nullptr;

	// Convert from string.
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	if (!doc.IsObject())
		return false;

	// Parse the data.
	const rapidjson::Value* sound = nullptr;
	if (!Jpath::get(doc, sound, "sound"))
		return false;
	if (!sound || !sound->IsObject())
		return false;

	// Fill in SFX.
	Sfx* ptr = nullptr;
	if (!data->clone(&ptr))
		return false;
	sfx = Sfx::Ptr(ptr);

	// Finish.
	return true;
}

bool SfxAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the notes.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "sound");
	rapidjson::Value* sound = nullptr;
	Jpath::get(doc, sound, "sound");

	rapidjson::Document doc_;
	if (!data->toJson(doc_)) {
		assetsRaiseWarningOrError("Cannot serialize SFX to JSON.", false, onWarningOrError);

		return false;
	}
	sound->Set(doc_.GetObject());

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert SFX to JSON.", false, onWarningOrError);

		return false;
	}
	std::string str;
	if (!json->toString(str, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	val = str;

	// Finish.
	return true;
}

bool SfxAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert SFX from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the notes.
	const rapidjson::Value* sound = nullptr;
	if (!Jpath::get(doc, sound, "sound")) {
		assetsRaiseWarningOrError("Cannot find \"sound\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!sound || !sound->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"sound\" entry.", false, onWarningOrError);

		return false;
	}

	// Fill in with the notes.
	if (!data->fromJson(*sound)) {
		assetsRaiseWarningOrError("Cannot convert SFX from JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool SfxAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError) {
	const std::string val_(val, len);

	return fromString(val_, onWarningOrError);
}

bool SfxAssets::empty(void) const {
	return entries.empty();
}

int SfxAssets::count(void) const {
	return (int)entries.size();
}

bool SfxAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool SfxAssets::insert(const Entry &entry, int index) {
	if (index < 0 || index > (int)entries.size())
		return false;

	if (index == (int)entries.size())
		entries.push_back(entry);
	else
		entries.insert(entries.begin() + index, entry);

	return true;
}

bool SfxAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

void SfxAssets::clear(void) {
	entries.clear();
}

const SfxAssets::Entry* SfxAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

SfxAssets::Entry* SfxAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const SfxAssets::Entry* SfxAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		const Sfx::Ptr &ptr = entry.data;
		const Sfx::Sound* snd = ptr->pointer();
		if (snd->name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const SfxAssets::Entry* SfxAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		const Sfx::Ptr &ptr = entry.data;
		const Sfx::Sound* snd = ptr->pointer();
		if (snd->name == name) {
			if (index)
				*index = i;

			gotName = snd->name;

			return &entry;
		}

		if (snd->name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(snd->name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = snd->name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int SfxAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		const Sfx::Ptr &ptr = entry.data;
		const Sfx::Sound* snd = ptr->pointer();
		if (snd->name == name)
			return i;
	}

	return -1;
}

bool SfxAssets::swap(int leftIndex, int rightIndex) {
	if (leftIndex < 0 || leftIndex >= (int)entries.size() || rightIndex < 0 || rightIndex >= (int)entries.size())
		return false;

	std::swap(entries[leftIndex], entries[rightIndex]);

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Actor assets
*/

ActorAssets::Entry::Entry(Renderer* rnd, bool is8x16, PaletteAssets::Getter getplt, active_t::BehaviourSerializer serializeBhvr, active_t::BehaviourParser parseBhvr) :
	renderer(rnd),
	getPalette(getplt),
	serializeBehaviour(serializeBhvr), parseBehaviour(parseBhvr)
{
	palette = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
	const PaletteAssets::Entry* ref_ = getPalette(GBBASIC_ACTOR_DEFAULT_PALETTE);
	if (ref_) {
		for (int i = 0; i < GBBASIC_PALETTE_PER_GROUP_COUNT; ++i) {
			Colour col;
			ref_->data->get(i, col);
			palette->set(i, &col);
		}
	}
	data = Actor::Ptr(Actor::create());
	data->load(is8x16);
	Image::Ptr img(Image::create(palette));
	img->fromBlank(GBBASIC_ACTOR_DEFAULT_WIDTH * GBBASIC_TILE_SIZE, GBBASIC_ACTOR_DEFAULT_HEIGHT * GBBASIC_TILE_SIZE, GBBASIC_PALETTE_DEPTH);
	Image::Ptr props(Image::create(Indexed::Ptr(Indexed::create((int)Math::pow(2, ACTOR_PALETTE_DEPTH)))));
	props->fromBlank(GBBASIC_ACTOR_DEFAULT_WIDTH, GBBASIC_ACTOR_DEFAULT_HEIGHT, ACTOR_PALETTE_DEPTH);
	const Math::Vec2i anchor = computeAnchor(GBBASIC_ACTOR_DEFAULT_WIDTH, GBBASIC_ACTOR_DEFAULT_HEIGHT);
	data->add(img, props, anchor);
	definition.bounds = boundingbox_t(
		(Int8)(-anchor.x), (Int8)(GBBASIC_ACTOR_DEFAULT_WIDTH * GBBASIC_TILE_SIZE - anchor.x - 1),
		(Int8)(-anchor.y), (Int8)(GBBASIC_ACTOR_DEFAULT_HEIGHT * GBBASIC_TILE_SIZE - anchor.x - 1)
	);
}

ActorAssets::Entry::Entry(Renderer* rnd, const std::string &val, PaletteAssets::Getter getplt, active_t::BehaviourSerializer serializeBhvr, active_t::BehaviourParser parseBhvr) :
	renderer(rnd),
	getPalette(getplt),
	serializeBehaviour(serializeBhvr), parseBehaviour(parseBhvr)
{
	palette = Indexed::Ptr(Indexed::create(GBBASIC_PALETTE_PER_GROUP_COUNT));
	const PaletteAssets::Entry* ref_ = getPalette(GBBASIC_ACTOR_DEFAULT_PALETTE);
	if (ref_) {
		for (int i = 0; i < GBBASIC_PALETTE_PER_GROUP_COUNT; ++i) {
			Colour col;
			ref_->data->get(i, col);
			palette->set(i, &col);
		}
	}
	data = Actor::Ptr(Actor::create());
	data->load(false);

	fromString(val, nullptr);
}

size_t ActorAssets::Entry::hash(void) const {
	size_t result = 0;

	if (data)
		result = Math::hash(result, data->hash());

	// `name` doesn't count.

	result = Math::hash(result, asActor);

	result = Math::hash(result, definition.hash());

	return result;
}

int ActorAssets::Entry::compare(const Entry &other) const {
	int ret = 0;

	if (data && other.data)
		ret = data->compare(other.data.get());
	if (ret != 0)
		return ret;

	// `name` doesn't count.

	if (!asActor && other.asActor)
		return -1;
	else if (asActor && !other.asActor)
		return 1;

	if (definition < other.definition)
		return -1;
	else if (definition > other.definition)
		return 1;

	return 0;
}

void ActorAssets::Entry::touch(void) {
	for (int i = 0; i < data->count(); ++i)
		data->touch(renderer, i);
}

void ActorAssets::Entry::touch(PaletteResolver resolve) {
	for (int i = 0; i < data->count(); ++i) {
		data->touch(
			renderer, i,
			[resolve, i] (int x, int y, Byte idx) -> Colour {
				return resolve(i, x, y, idx);
			}
		);
	}
}

void ActorAssets::Entry::cleanup(void) {
	data->cleanup();
}

void ActorAssets::Entry::cleanup(int frame) {
	data->cleanup(frame);
}

bool ActorAssets::Entry::refresh(void) {
	// Determine whether can refresh.
	const PaletteAssets::Entry* ref_ = getPalette(ref);
	if (!ref_)
		return false;

	// Determine whether need to refresh.
	if (!isRefRevisionChanged(ref_)) // There is unsynchronized modification in this asset's dependency.
		return false;

	// Refresh this asset.
	for (int i = 0; i < Math::min(ref_->data->count(), GBBASIC_PALETTE_PER_GROUP_COUNT); ++i) {
		Colour col;
		ref_->data->get(i, col);
		palette->set(i, &col);
	}
	cleanup(); // Clean up the outdated editable and runtime resources.

	// Mark this asset synchronized.
	synchronizeRefRevision(ref_); // Mark as synchronized for this asset's dependency.

	// Finish.
	return true;
}

Colour ActorAssets::Entry::colorAt(int frame_, int x, int y, Byte idx) const {
	const Colour transparent(255, 255, 255, 0);

	if (!getPalette)
		return transparent;

	if (frame_ < 0 || frame_ >= data->count())
		return transparent;

	Actor::Frame* frame = data->get(frame_);
	if (!frame)
		return transparent;

	const bool is8x16 = data->is8x16();
	const int tx = x / GBBASIC_TILE_SIZE;
	const int ty = is8x16 ? y / 16 : y / GBBASIC_TILE_SIZE;
	int props = 0;
	frame->getProperty(tx, ty, props);
	const int bits = props & ((0x00000001 << GBBASIC_ACTOR_PALETTE_BIT0) | (0x00000001 << GBBASIC_ACTOR_PALETTE_BIT1) | (0x00000001 << GBBASIC_ACTOR_PALETTE_BIT2));
	const int plt = bits + GBBASIC_ACTOR_DEFAULT_PALETTE;
	const PaletteAssets::Entry* entry = getPalette(plt);
	if (!entry)
		return transparent;

	Colour col;
	if (!entry->data->get(idx, col))
		return transparent;

	return col;
}

Texture::Ptr ActorAssets::Entry::figureTexture(Math::Vec2i* anchor, PaletteResolver resolve) const {
	if (anchor)
		*anchor = Math::Vec2i();

	if (!renderer)
		return nullptr;

	if (!data)
		return nullptr;

	Actor::Frame* frame = data->get(figure);
	if (!frame)
		return nullptr;

	if (resolve && getPalette) {
		frame->touch(
			renderer,
			[this, resolve] (int x, int y, Byte idx) -> Colour {
				return resolve(figure, x, y, idx);
			}
		);
	} else {
		frame->touch(renderer);
	}

	const Texture::Ptr &tex = frame->texture;
	if (!tex)
		return nullptr;

	if (anchor)
		*anchor = frame->anchor;

	return tex;
}

std::pair<int, int> ActorAssets::Entry::estimateFinalByteCount(void) const {
	std::pair<int, int> result = std::make_pair(0, 0);

	if (!slices.empty()) {
		if (data->is8x16())
			result.first = 8 * 16;
		else
			result.first = GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE;
		result.first *= (int)slices.size();
		result.first *= GBBASIC_PALETTE_DEPTH;
		result.first /= 8;
	}

	if (!shadow.empty() && !animation.empty()) {
		for (int a : animation) {
			const Actor::Shadow &s = shadow[a];
			// One frame.
			for (int i = 0; i < (int)s.refs.size(); ++i) {
				const int t = s.refs[i].index;
				const Actor::Slice &slice = slices[t];
				if (slice.image->blank())
					continue;

				result.second += 4; // Frame tiles.
			}
			result.second += 1; // End byte.
		}
		result.second += (int)shadow.size() + 1; // A group of frames.
	}

	return result;
}

bool ActorAssets::Entry::serializeBasic(std::string &val, int page, bool asActor_) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	std::string asset;
	if (name.empty())
		asset = "#" + Text::toString(page);
	else
		asset = "\"" + name + "\"";

	if (asActor_) {
		val += "sprite on";
		val += "\n";
		if (data->is8x16()) {
			val += "option SPRITE8x16_ENABLED, true";
			val += "\n";
		}
		val += "fill actor(";
		val += Text::toString(0);
		val += ", ";
		if (data->is8x16()) {
			const int n = (int)slices.size() * ((8 * 16) / (GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE));
			val += Text::toString(n);
		} else {
			val += Text::toString((UInt32)slices.size());
		}
		val += ") = ";
		val += asset;
		val += "\n";
		val += "let a = new actor()";
		val += "\n";
		val += "def actor(a, 0, 0, 0) = ";
		val += asset;
		val += "\n";
	} else {
		val += "sprite on";
		val += "\n";
		if (data->is8x16()) {
			val += "option SPRITE8x16_ENABLED, true";
			val += "\n";
		}
		val += "fill projectile(";
		val += Text::toString(0);
		val += ", ";
		if (data->is8x16()) {
			const int n = (int)slices.size() * ((8 * 16) / (GBBASIC_TILE_SIZE * GBBASIC_TILE_SIZE));
			val += Text::toString(n);
		} else {
			val += Text::toString((UInt32)slices.size());
		}
		val += ") = ";
		val += asset;
		val += "\n";
		val += "def projectile(0, 0) = ";
		val += asset;
		val += "\n";
		val += "let p = start projectile(0, 0, 0, 0, PROJECTILE_NONE_FLAG)";
		val += "\n";
	}

	// Finish.
	return true;
}

bool ActorAssets::Entry::serializeC(std::string &val) const {
	return data->toC(val, name.c_str());
}

bool ActorAssets::Entry::serializeDataSequence(int frame, std::string &val, int base) const {
	// Prepare.
	val.clear();

	const Actor::Frame* frame_ = data->get(frame);
	if (!frame_)
		return false;

	const Image::Ptr &pixels = frame_->pixels;
	if (!pixels)
		return false;

	// Serialize the pixels.
	if (!pixels->paletted())
		return false;

	int tw = 0;
	int th = 0;
	if (data->is8x16()) {
		tw = 8;
		th = 16;
	} else {
		tw = GBBASIC_TILE_SIZE;
		th = GBBASIC_TILE_SIZE;
	}
	pixels->serializeTiles(
		tw, th,
		[&] (int /* tx */, int /* ty */) -> void {
			val += "data ";
		},
		[&] (int /* tx */, int /* ty */) -> void {
			val += "\n";
		},
		[&] (int y, int /* lineno */, int /* tx */, int /* ty */, UInt8 ln0, UInt8 ln1) -> void {
			if (base == 16) {
				val += "0x" + Text::toHex(ln0, 2, '0', false);
				val += ", ";
				val += "0x" + Text::toHex(ln1, 2, '0', false);
			} else if (base == 10) {
				val += Text::toString(ln0);
				val += ", ";
				val += Text::toString(ln1);
			} else if (base == 2) {
				val += "0b" + Text::toBin(ln0);
				val += ", ";
				val += "0b" + Text::toHex(ln1);
			} else {
				GBBASIC_ASSERT(false && "Not supported.");
			}

			if (y != th - 1)
				val += ", ";
		}
	);

	// Finish.
	return true;
}

bool ActorAssets::Entry::parseDataSequence(int frame, Image::Ptr &img, const std::string &val, bool isCompressed) const {
	// Prepare.
	typedef std::vector<Int64> Data;

	img = nullptr;

	const Actor::Frame* frame_ = data->get(frame);
	if (!frame_)
		return false;

	const Image::Ptr &pixels = frame_->pixels;
	if (!pixels)
		return false;

	// Parse the pixels.
	Data bytes;
	std::string val_ = val;
	val_ = Text::replace(val_, "\r\n", ",");
	val_ = Text::replace(val_, "\r", ",");
	val_ = Text::replace(val_, "\n", ",");
	val_ = Text::replace(val_, "\t", ",");
	val_ = Text::replace(val_, " ", ",");
	Text::Array tokens = Text::split(val_, ",");
	val_.clear();
	for (std::string tk : tokens) {
		tk = Text::trim(tk);
		if (tk.empty())
			continue;

		if (pixels->paletted()) {
			int idx = 0;
			if (!Text::fromString(tk, idx))
				continue;

			bytes.push_back(idx);
			if (isCompressed)
				bytes.push_back(idx);
		} else {
			unsigned rgba = 0;
			if (!Text::fromString(tk, rgba))
				continue;

			bytes.push_back(rgba);
		}
	}

	// Fill in an image.
	Image* ptr = nullptr;
	if (!pixels->clone(&ptr))
		return false;
	img = Image::Ptr(ptr);

	if (!img->paletted())
		return false;

	int tw = 0;
	int th = 0;
	if (data->is8x16()) {
		tw = 8;
		th = 16;
	} else {
		tw = GBBASIC_TILE_SIZE;
		th = GBBASIC_TILE_SIZE;
	}
	img->parseTiles(
		tw, th,
		[&] (void) -> int {
			return (int)bytes.size();
		},
		[&] (int idx) -> Int64 {
			return bytes[idx];
		}
	);

	// Finish.
	return true;
}

bool ActorAssets::Entry::serializeJson(std::string &val, bool pretty) const {
	// Prepare.
	val.clear();

	// Serialize the data.
	rapidjson::Document doc;

	Jpath::set(doc, doc, figure, "figure");

	Jpath::set(doc, doc, asActor, "as_actor");

	Jpath::set(doc, doc, Jpath::ANY(), "definition");
	rapidjson::Value* jdef = nullptr;
	Jpath::get(doc, jdef, "definition");
	definition.toJson(*jdef, doc, serializeBehaviour);

	Jpath::set(doc, doc, Jpath::ANY(), "frames");
	rapidjson::Value* frames = nullptr;
	Jpath::get(doc, frames, "frames");

	rapidjson::Document doc_;
	if (!data->toJson(doc_))
		return false;
	frames->Set(doc_.GetObject());

	// Convert to string.
	std::string str;
	if (!Json::toString(doc, str, pretty))
		return false;

	val = str;

	// Finish.
	return true;
}

bool ActorAssets::Entry::parseJson(Actor::Ptr &actor, int &figure_, bool &asActor_, active_t &def, const std::string &val, ParsingStatuses &status) const {
	// Prepare.
	actor = nullptr;
	figure_ = 0;
	asActor_ = true;
	def = active_t();
	status = ParsingStatuses::SUCCESS;

	// Convert from string.
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	if (!doc.IsObject())
		return false;

	// Parse the data.
	if (!Jpath::get(doc, figure_, "figure"))
		figure_ = 0;

	if (!Jpath::get(doc, asActor_, "as_actor"))
		asActor_ = true;

	rapidjson::Value* jdef = nullptr;
	if (Jpath::get(doc, jdef, "definition") && jdef) {
		if (!def.fromJson(*jdef, parseBehaviour))
			def = active_t();
	}

	const rapidjson::Value* frames = nullptr;
	if (!Jpath::get(doc, frames, "frames"))
		return false;
	if (!frames || !frames->IsObject())
		return false;

	// Fill in an actor.
	actor = Actor::Ptr(Actor::create());
	if (!actor->fromJson(palette, *frames))
		return false;

	// Finish.
	return true;
}

bool ActorAssets::Entry::serializeSpriteSheet(Bytes* val, const char* type, int pitch) const {
	// Prepare.
	val->clear();

	if (pitch <= 0)
		return false;

	int w = 0;
	int h = 0;
	for (int i = 0; i < data->count(); ++i) {
		const Actor::Frame* frame = data->get(i);
		if (frame->width() > w)
			w = frame->width();
		if (frame->height() > h)
			h = frame->height();
	}

	// Serialize.
	Image::Ptr tmp(Image::create(palette));
	const std::div_t div = std::div(data->count(), pitch);
	const int v = div.quot + (div.rem ? 1 : 0);
	tmp->fromBlank(w * pitch, h * v, palette->depth());
	int k = 0;
	for (int j = 0; j < v; ++j) {
		for (int i = 0; i < pitch; ++i) {
			if (k >= data->count())
				break;

			const Actor::Frame* frame = data->get(k++);
			const int dx = (w - frame->width()) / 2;
			const int dy = (h - frame->height()) / 2;
			frame->pixels->blit(tmp.get(), i * w + dx, j * h + dy, frame->width(), frame->height(), 0, 0);
		}
	}
	if (!tmp->realize())
		return false;
	if (!tmp->toBytes(val, type))
		return false;

	// Finish.
	return true;
}

bool ActorAssets::Entry::parseSpriteSheet(Actor::Ptr &actor, const Bytes* val, const Math::Vec2i &n, ParsingStatuses &status) const {
	// Prepare.
	actor = nullptr;
	status = ParsingStatuses::SUCCESS;

	// Convert from bytes.
	Image::Ptr tmp(Image::create(palette));
	if (!tmp->fromBytes(val))
		return false;

	if (n.x <= 0 || n.y <= 0)
		return false;

	if (tmp->width() == 0 || tmp->height() == 0)
		return false;

	if ((tmp->width() % GBBASIC_TILE_SIZE) || (tmp->height() % GBBASIC_TILE_SIZE)) {
		status = ParsingStatuses::NOT_A_MULTIPLE_OF_8x8;

		return false;
	}

	const int tw = tmp->width() / GBBASIC_TILE_SIZE;
	const int th = tmp->height() / GBBASIC_ACTOR_MAX_HEIGHT;
	if (tw / n.x > GBBASIC_ACTOR_MAX_WIDTH || th / n.y> GBBASIC_ACTOR_MAX_HEIGHT) {
		status = ParsingStatuses::OUT_OF_BOUNDS;

		return false;
	}

	// Convert to a paletted image.
	Image::Ptr img(Image::create(palette));
	Indexed::Lookup lookup;
	if (!palette->match(tmp.get(), lookup))
		return false;

	img->fromBlank(tmp->width(), tmp->height(), GBBASIC_PALETTE_DEPTH);
	for (int j = 0; j < img->height(); ++j) {
		for (int i = 0; i < img->width(); ++i) {
			Colour col;
			tmp->get(i, j, col);
			const int idx = lookup[col];
			img->set(i, j, idx);
		}
	}

	// Fill in a sprite sheet.
	actor = Actor::Ptr(Actor::create());
	const int w = img->width() / n.x;
	const int h = img->height() / n.y;
	for (int j = 0; j < n.y; ++j) {
		for (int i = 0; i < n.x; ++i) {
			Image::Ptr frame(Image::create(palette));
			frame->fromBlank(w, h, palette->depth());
			img->blit(frame.get(), 0, 0, w, h, i * w, j * h);
			if (frame->blank())
				continue; // Ignore blank frames.

			Image::Ptr props(Image::create(Indexed::Ptr(Indexed::create((int)Math::pow(2, ACTOR_PALETTE_DEPTH)))));
			const int tw = w / GBBASIC_TILE_SIZE;
			const int th = h / GBBASIC_TILE_SIZE;
			props->fromBlank(tw, th, ACTOR_PALETTE_DEPTH);
			const Math::Vec2i anchor = computeAnchor(tw, th);
			actor->add(frame, props, anchor);
		}
	}

	// Finish.
	return true;
}

bool ActorAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the frames.
	rapidjson::Document doc;

	Jpath::set(doc, doc, figure, "figure");

	Jpath::set(doc, doc, name, "name");

	Jpath::set(doc, doc, magnification, "magnification");

	Jpath::set(doc, doc, asActor, "as_actor");

	Jpath::set(doc, doc, Jpath::ANY(), "definition");
	rapidjson::Value* jdef = nullptr;
	Jpath::get(doc, jdef, "definition");
	definition.toJson(*jdef, doc, serializeBehaviour);

	Jpath::set(doc, doc, Jpath::ANY(), "frames");
	rapidjson::Value* frames = nullptr;
	Jpath::get(doc, frames, "frames");

	rapidjson::Document doc_;
	if (!data->toJson(doc_)) {
		assetsRaiseWarningOrError("Cannot serialize actor to JSON.", false, onWarningOrError);

		return false;
	}
	frames->Set(doc_.GetObject());

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert actor to JSON.", false, onWarningOrError);

		return false;
	}
	std::string str;
	if (!json->toString(str, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	val = str;

	// Finish.
	return true;
}

bool ActorAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert actor from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the frames.
	if (!Jpath::get(doc, figure, "figure"))
		figure = 0;

	if (!Jpath::get(doc, name, "name"))
		name.clear();

	if (!Jpath::get(doc, magnification, "magnification"))
		magnification = -1;

	if (!Jpath::get(doc, asActor, "as_actor"))
		asActor = true;

	rapidjson::Value* jdef = nullptr;
	if (Jpath::get(doc, jdef, "definition") && jdef) {
		if (!definition.fromJson(*jdef, parseBehaviour))
			definition = active_t();
	}

	const rapidjson::Value* frames = nullptr;
	if (!Jpath::get(doc, frames, "frames")) {
		assetsRaiseWarningOrError("Cannot find \"frames\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!frames || !frames->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"frames\" entry.", false, onWarningOrError);

		return false;
	}

	// Fill in with the frames.
	if (!data->fromJson(palette, *frames)) {
		assetsRaiseWarningOrError("Cannot convert actor from JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool ActorAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError) {
	const std::string val_(val, len);

	return fromString(val_, onWarningOrError);
}

bool ActorAssets::empty(void) const {
	return entries.empty();
}

int ActorAssets::count(void) const {
	return (int)entries.size();
}

bool ActorAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool ActorAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

const ActorAssets::Entry* ActorAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

ActorAssets::Entry* ActorAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const ActorAssets::Entry* ActorAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const ActorAssets::Entry* ActorAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			gotName = entry.name;

			return &entry;
		}

		if (entry.name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(entry.name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = entry.name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int ActorAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name)
			return i;
	}

	return -1;
}

bool ActorAssets::computeSpriteSheetSlices(const Image::Ptr &img, Math::Vec2i &count) {
	constexpr const int MAX_ATTEMPT = 24;

	count = Math::Vec2i(1, 1);

	if (img->width() == 0 || img->height() == 0)
		return false;

	if ((img->width() % GBBASIC_TILE_SIZE) || (img->height() % GBBASIC_TILE_SIZE))
		return false;

	const int tw = img->width() / GBBASIC_TILE_SIZE;
	const int th = img->height() / GBBASIC_ACTOR_MAX_HEIGHT;

	const Colour baseCol = img->findLightest();

	for (int s = MAX_ATTEMPT; s >= 2; --s) {
		if (img->canSliceV(s, baseCol)) {
			count.x = s;

			break;
		}
	}
	for (int s = MAX_ATTEMPT; s >= 2; --s) {
		if (img->canSliceH(s, baseCol)) {
			count.y = s;

			break;
		}
	}

	constexpr const int _16 = 16;
	if (img->width() > _16 && count.x == 1)
		count.x = img->width() / _16;
	if (img->height() > _16 && count.y == 1)
		count.y = img->height() / _16;

	if (tw / count.x > GBBASIC_ACTOR_MAX_WIDTH || th / count.y > GBBASIC_ACTOR_MAX_HEIGHT)
		return false;

	return true;
}

Math::Vec2i ActorAssets::computeAnchor(int twidth, int theight) {
	Math::Vec2i result;
	const std::div_t div = std::div(twidth * GBBASIC_TILE_SIZE, 2);
	result.x = div.quot + (!!div.rem ? 1 : 0);
	result.y = theight * GBBASIC_TILE_SIZE - GBBASIC_TILE_SIZE;

	return result;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Scene assets
*/

ActorRoutines::ActorRoutines() {
}

ActorRoutines::ActorRoutines(const std::string &up, const std::string &hit) : update(up), onHits(hit) {
}

ActorRoutineOverriding* Routine::add(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos) {
	overridings.push_back(ActorRoutineOverriding(pos));
	sort(overridings);
	overridings.shrink_to_fit();

	return search(overridings, pos);
}

bool Routine::remove(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos) {
	ActorRoutineOverriding* existing = search(overridings, pos);
	if (!existing)
		return false;

	const int diff = (int)(existing - &overridings.front());
	overridings.erase(overridings.begin() + diff);

	return true;
}

ActorRoutineOverriding* Routine::move(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos, const Math::Vec2i &newPos) {
	ActorRoutineOverriding* existing = search(overridings, pos);
	if (!existing)
		return nullptr;

	existing->position = newPos;

	sort(overridings);

	return search(overridings, newPos);
}

void Routine::sort(ActorRoutineOverriding::Array &overridings) {
	std::sort(
		overridings.begin(), overridings.end(),
		[&] (const ActorRoutineOverriding &left, const ActorRoutineOverriding &right) -> bool {
			return left < right;
		}
	);
}

const ActorRoutineOverriding* Routine::search(const ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos) {
	ActorRoutineOverriding::Array &overridings_ = const_cast<ActorRoutineOverriding::Array &>(overridings);

	return search(overridings_, pos);
}

ActorRoutineOverriding* Routine::search(ActorRoutineOverriding::Array &overridings, const Math::Vec2i &pos) {
	if (overridings.empty())
		return nullptr;

	const ActorRoutineOverriding key(pos);
	void* ptr = std::bsearch(
		&key, &overridings.front(), overridings.size(), sizeof(ActorRoutineOverriding),
		[] (const void* l_, const void* r_) -> int {
			const ActorRoutineOverriding* l = (const ActorRoutineOverriding*)l_;
			const ActorRoutineOverriding* r = (const ActorRoutineOverriding*)r_;
			if (*l < *r)
				return -1;
			if (*l > *r)
				return 1;

			return 0;
		}
	);
	if (!ptr)
		return nullptr;

	return (ActorRoutineOverriding*)ptr;
}

bool Routine::toJson(const ActorRoutineOverriding &overriding, rapidjson::Value &val, rapidjson::Document &doc) {
	Jpath::set(doc, val, overriding.position.x, "position", "x");

	Jpath::set(doc, val, overriding.position.y, "position", "y");

	Jpath::set(doc, val, overriding.routines.update, "routines", "update");

	Jpath::set(doc, val, overriding.routines.onHits, "routines", "on_hits");

	return true;
}

bool Routine::fromJson(ActorRoutineOverriding &overriding, const rapidjson::Value &val) {
	Jpath::get(val, overriding.position.x, "position", "x");

	Jpath::get(val, overriding.position.y, "position", "y");

	Jpath::get(val, overriding.routines.update, "routines", "update");

	Jpath::get(val, overriding.routines.onHits, "routines", "on_hits");

	return true;
}

SceneAssets::Entry::Entry(int refMap_, MapAssets::Getter getmap, ActorAssets::Getter getact, Texture::Ptr propstex, Texture::Ptr actorstex) :
	refMap(refMap_),
	getMap(getmap), getActor(getact)
{
	MapAssets::Entry* refMap__ = getmap(refMap_);
	if (refMap__->refresh()) { // Refresh the outdated editable resources.
		refMap__->touch();
	}
	Map::Tiles tiles;
	if (!refMap__->data->tiles(tiles) || !tiles.texture)
		refMap__->touch(); // Ensure the runtime resources are ready.

	const Map::Ptr &mapPtr = refMap__->data;

	const Map::Ptr &attribPtr = refMap__->attributes;

	Math::Vec2i propscount(propstex->width() / GBBASIC_TILE_SIZE, propstex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles propstiles(propstex, propscount);
	Map::Ptr propsPtr = Map::Ptr(Map::create(&propstiles, true));
	propsPtr->resize(mapPtr->width(), mapPtr->height());

	Math::Vec2i actorscount(actorstex->width() / GBBASIC_TILE_SIZE, actorstex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles actorstiles(actorstex, actorscount);
	Map::Ptr actorsPtr = Map::Ptr(Map::create(&actorstiles, true));
	actorsPtr->resize(mapPtr->width(), mapPtr->height());
	actorsPtr->load(Scene::INVALID_ACTOR(), actorsPtr->width(), actorsPtr->height());

	const Trigger::Array triggers;

	data = Scene::Ptr(Scene::create());
	data->load(mapPtr->width(), mapPtr->height(), mapPtr, attribPtr, propsPtr, actorsPtr, triggers);
}

SceneAssets::Entry::Entry(const std::string &val, MapAssets::Getter getmap, ActorAssets::Getter getact, Texture::Ptr propstex, Texture::Ptr actorstex) :
	getMap(getmap), getActor(getact)
{
	int refMap_ = 0;
	int width = 0;
	int height = 0;
	do {
		rapidjson::Document doc;
		if (!Json::fromString(doc, val.c_str()))
			break;

		if (!Jpath::get(doc, refMap_, "ref_map"))
			refMap_ = 0;

		if (!Jpath::get(doc, width, "width"))
			width = 0;
		if (!Jpath::get(doc, height, "height"))
			height = 0;
	} while (false);

	MapAssets::Entry* refMap__ = getmap(refMap_);
	Map::Ptr mapPtr = nullptr;
	Map::Ptr attribPtr = nullptr;
	if (refMap__) {
		if (refMap__->refresh()) { // Refresh the outdated editable resources.
			refMap__->touch();
		}
		Map::Tiles tiles;
		if (!refMap__->data->tiles(tiles) || !tiles.texture)
			refMap__->touch(); // Ensure the runtime resources are ready.

		mapPtr = refMap__->data;

		attribPtr = refMap__->attributes;
	}

	Math::Vec2i propscount(propstex->width() / GBBASIC_TILE_SIZE, propstex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles propstiles(propstex, propscount);
	Map::Ptr propsPtr = Map::Ptr(Map::create(&propstiles, true));

	Math::Vec2i actorscount(actorstex->width() / GBBASIC_TILE_SIZE, actorstex->height() / GBBASIC_TILE_SIZE);
	Map::Tiles actorstiles(actorstex, actorscount);
	Map::Ptr actorsPtr = Map::Ptr(Map::create(&actorstiles, true));

	const Trigger::Array triggers;

	data = Scene::Ptr(Scene::create());
	data->load(width, height, mapPtr, attribPtr, propsPtr, actorsPtr, triggers);

	fromString(val, nullptr);
}

size_t SceneAssets::Entry::hash(void) const {
	size_t result = 0;

	if (data)
		result = Math::hash(result, data->hash());

	result = Math::hash(result, refMap);

	// `name` doesn't count.

	result = Math::hash(result, definition.hash());

	return result;
}

int SceneAssets::Entry::compare(const Entry &other) const {
	int ret = 0;

	if (data && other.data)
		ret = data->compare(other.data.get());
	if (ret != 0)
		return ret;

	if (refMap < other.refMap)
		return -1;
	else if (refMap > other.refMap)
		return 1;

	// `name` doesn't count.

	if (definition < other.definition)
		return -1;
	else if (definition > other.definition)
		return 1;

	return 0;
}

bool SceneAssets::Entry::has16x16PlayerActor(ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour) const {
	constexpr const int VISUAL_MIN_SIZE = 16;
	constexpr const int BOUNDS_MIN_SIZE = 12;

	if (!getActor || !isPlayerBehaviour)
		return false;

	SceneAssets::Entry::UniqueRef uref;
	const SceneAssets::Entry::Ref refActors_ = getRefActors(&uref);
	(void)refActors_;
	for (int refActor : uref) {
		ActorAssets::Entry* actorEntry = getActor(refActor);
		if (!actorEntry)
			continue;

		if (!isPlayerBehaviour(actorEntry->definition.behaviour))
			continue;

		const Actor::Ptr &actorPtr = actorEntry->data;
		if (!actorPtr)
			continue;

		const Actor::Frame* figureFrame = actorPtr->get(actorEntry->figure);
		if (!figureFrame)
			continue;

		if (figureFrame->width() < VISUAL_MIN_SIZE && figureFrame->height() < VISUAL_MIN_SIZE)
			continue;

		if (actorEntry->definition.bounds.width() >= BOUNDS_MIN_SIZE && actorEntry->definition.bounds.height() >= BOUNDS_MIN_SIZE)
			return true;
	}

	return false;
}

SceneAssets::Entry::Ref SceneAssets::Entry::getRefActors(UniqueRef* uref) const {
	Ref result;
	if (uref)
		uref->clear();

	Map::Ptr layer = data->actorLayer();
	if (!layer)
		return result;

	const int w = layer->width();
	const int h = layer->height();
	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			const UInt8 cel = (UInt8)layer->get(i, j);
			if (cel == Scene::INVALID_ACTOR())
				continue;

			result.push_back(cel);
			if (uref && uref->find(cel) == uref->end())
				uref->insert(cel);
		}
	}

	return result;
}

int SceneAssets::Entry::updateRefActors(int removedIndex) {
	int result = 0;

	Map::Ptr layer = data->actorLayer();
	if (!layer)
		return result;

	const int w = layer->width();
	const int h = layer->height();
	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			const UInt8 cel = (UInt8)layer->get(i, j);
			if (cel == Scene::INVALID_ACTOR())
				continue;

			if (cel == removedIndex) {
				// Do nothing.
			} else if (cel > removedIndex) {
				layer->set(i, j, cel - 1, false);
			}
		}
	}

	return result;
}

bool SceneAssets::Entry::isRefMapRevisionChanged(const BaseAssets::Versionable* versionable) const {
	return mapRevision != versionable->revision;
}

bool SceneAssets::Entry::isRefActorRevisionChanged(int ref, const BaseAssets::Versionable* versionable) const {
	Revisions::const_iterator it = actorRevisions.find(ref);
	if (it == actorRevisions.end())
		return true;

	return it->second != versionable->revision;
}

void SceneAssets::Entry::synchronizeRefMapRevision(BaseAssets::Versionable* versionable) {
	mapRevision = versionable->revision;
}

void SceneAssets::Entry::synchronizeRefActorRevision(int ref, BaseAssets::Versionable* versionable) {
	actorRevisions[ref] = versionable->revision;
}

void SceneAssets::Entry::touch(void) {
	if (refMap == -1) {
		// This asset's dependency was lost, need to resolve.
	} else {
		MapAssets::Entry* mapEntry = getMap(refMap);
		mapEntry->touch();
	}
}

void SceneAssets::Entry::cleanup(void) {
	MapAssets::Entry* mapEntry = getMap(refMap);
	if (mapEntry)
		mapEntry->cleanup();

	if (data->propertyLayer())
		data->propertyLayer()->cleanup();

	if (data->actorLayer())
		data->actorLayer()->cleanup();

	for (int refActor : uniqueRefActors) {
		ActorAssets::Entry* actorEntry = getActor(refActor);
		if (!actorEntry)
			continue;

		actorEntry->cleanup();
	}
}

void SceneAssets::Entry::cleanup(const Math::Recti &area) {
	MapAssets::Entry* mapEntry = getMap(refMap);
	if (mapEntry)
		mapEntry->cleanup(area);
}

void SceneAssets::Entry::reload(void) {
	MapAssets::Entry* refMap__ = getMap(refMap);
	Map::Ptr mapPtr = nullptr;
	Map::Ptr attribPtr = nullptr;
	if (refMap__) {
		if (refMap__->refresh()) { // Refresh the outdated editable resources.
			refMap__->touch();
		}
		Map::Tiles tiles;
		if (!refMap__->data->tiles(tiles) || !tiles.texture)
			refMap__->touch(); // Ensure the runtime resources are ready.

		mapPtr = refMap__->data;

		attribPtr = refMap__->attributes;
	}

	data->reload(mapPtr, attribPtr);
}

bool SceneAssets::Entry::refresh(void) {
	// Determine whether need to refresh with map.
	bool result = false;
	MapAssets::Entry* refMap_ = getMap(refMap);
	if (refMap_) {
		// Determine whether need to refresh.
		if (
			isRefMapRevisionChanged(refMap_) || // There is unsynchronized modification in this asset's dependency.
			refMap_->refresh() // Refresh the outdated editable resources.
		) {
			// Refresh this asset.
			cleanup(); // Clean up the outdated editable and runtime resources.

			// Mark this asset synchronized with the map.
			synchronizeRefMapRevision(refMap_);

			// Finish.
			result |= true;
		}
	}

	// Determine whether need to refresh with actors.
	for (int refActor : uniqueRefActors) {
		ActorAssets::Entry* refActor_ = getActor(refActor);
		if (refActor_) {
			// Determine whether need to refresh.
			if (
				isRefActorRevisionChanged(refActor, refActor_) // There is unsynchronized modification in this asset's dependency.
			) {
				// Refresh this asset.
				cleanup(); // Clean up the outdated editable and runtime resources.

				// Mark this asset synchronized with the actors.
				synchronizeRefActorRevision(refActor, refActor_);

				// Finish.
				result |= true;
			}
		}
	}

	// Finish.
	return result;
}

int SceneAssets::Entry::estimateFinalByteCount(void) const {
	int result = 0;
	const MapAssets::Entry* mapEntry = getMap(refMap);
	if (mapEntry && !mapEntry->hasAttributes) {
		if (data->hasAttributes())
			result += data->width() * data->height();
	}
	if (data->hasProperties())
		result += data->width() * data->height();
	if (data->hasActors()) {
		result += 1 /* actor count */ + 19 /* bytes per ref */ * (int)refActors.size() /* count */;
	}
	if (data->hasTriggers()) {
		result += 1 /* actor count */ + 8 /* bytes per object */ * (int)data->triggerLayer()->size() /* count */;
	}
	result += 12; // Definition.

	result += 1; // Layer mask.
	result += 1 + 2; // Map.
	result += 1 + 2; // Attributes.
	result += 1 + 2; // Properties.
	result += 1 + 2; // Actors.
	result += 1 + 2; // Triggers.
	result += 1 + 2; // Definition.

	return result;
}

bool SceneAssets::Entry::serializeBasic(std::string &val, int page, bool loading) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	// TODO: ASSET LOCAL PALETTE

	std::string asset;
	if (name.empty())
		asset = "#" + Text::toString(page);
	else
		asset = "\"" + name + "\"";

	if (loading) {
		val += "load scene(0, 0) = ";
		val += asset;
		val += "\n";
	} else {
		val += "def scene(";
		val += Text::toString(data->width());
		val += ", ";
		val += Text::toString(data->height());
		val += ", ";
		val += Text::toString(0);
		val += ") = ";
		val += asset;
		val += "\n";
	}

	// Finish.
	return true;
}

bool SceneAssets::Entry::serializeBasicForTriggerCallback(std::string &val, int index) const {
	// Prepare.
	val.clear();

	// Serialize the code.
	val += "on trigger(" + Text::toString(index) + ") ENTER bor LEAVE start trigger" + Text::toString(index) + "_hits\n";
	val += "trigger" + Text::toString(index) + "_hits:\n";
	val += "  begin def\n";
	val += "    def index = stack1\n";
	val += "    def event = stack0\n";
	val += "    if event = ENTER then\n";
	val += "      ' Your code here...\n";
	val += "    else ' if event = LEAVE then\n";
	val += "      ' Your code here...\n";
	val += "    end if\n";
	val += "    end\n";
	val += "  end def\n";

	// Finish.
	return true;
}

bool SceneAssets::Entry::serializeDataSequence(std::string &val, int base, int layer) const {
	// Prepare.
	val.clear();

	Map::Ptr lyr = nullptr;
	switch (layer) {
	case ASSETS_SCENE_GRAPHICS_LAYER:
		lyr = data->mapLayer();

		break;
	case ASSETS_SCENE_ATTRIBUTES_LAYER:
		lyr = data->attributeLayer();

		break;
	case ASSETS_SCENE_PROPERTIES_LAYER:
		lyr = data->propertyLayer();

		break;
	case ASSETS_SCENE_ACTORS_LAYER:
		lyr = data->actorLayer();

		break;
	default:
		GBBASIC_ASSERT(false && "Not supported.");

		break;
	}

	// Serialize the pixels.
	for (int j = 0; j < lyr->height(); ++j) {
		val += "data ";
		for (int i = 0; i < lyr->width(); ++i) {
			const int cel = lyr->get(i, j);
			if (base == 16) {
				val += "0x" + Text::toHex(cel, 2, '0', false);
			} else if (base == 10) {
				val += Text::toString(cel);
			} else if (base == 2) {
				val += "0b" + Text::toBin((Byte)cel);
			} else {
				GBBASIC_ASSERT(false && "Not supported.");
			}

			if (i != lyr->width() - 1)
				val += ", ";
		}
		val += "\n";
	}

	// Finish.
	return true;
}

bool SceneAssets::Entry::parseDataSequence(Map::Ptr &map, const std::string &val, int layer) const {
	// Prepare.
	typedef std::vector<Int64> Data;

	// Parse the pixels.
	Data bytes;
	std::string val_ = val;
	val_ = Text::replace(val_, "\r\n", ",");
	val_ = Text::replace(val_, "\r", ",");
	val_ = Text::replace(val_, "\n", ",");
	val_ = Text::replace(val_, "\t", ",");
	val_ = Text::replace(val_, " ", ",");
	Text::Array tokens = Text::split(val_, ",");
	val_.clear();
	for (std::string tk : tokens) {
		tk = Text::trim(tk);
		if (tk.empty())
			continue;

		int cel = 0;
		if (!Text::fromString(tk, cel))
			continue;

		bytes.push_back(cel);
	}

	// Fill in a map.
	Map::Ptr lyr = nullptr;
	switch (layer) {
	case ASSETS_SCENE_GRAPHICS_LAYER:
		lyr = data->mapLayer();

		break;
	case ASSETS_SCENE_ATTRIBUTES_LAYER:
		lyr = data->attributeLayer();

		break;
	case ASSETS_SCENE_PROPERTIES_LAYER:
		lyr = data->propertyLayer();

		break;
	case ASSETS_SCENE_ACTORS_LAYER:
		lyr = data->actorLayer();

		break;
	default:
		GBBASIC_ASSERT(false && "Not supported.");

		break;
	}

	Map* ptr = nullptr;
	if (!lyr->clone(&ptr, false))
		return false;
	map = Map::Ptr(ptr);

	Map::Tiles tiles;
	if (!map->tiles(tiles))
		return false;
	const int n = tiles.count.x * tiles.count.y;

	int k = 0;
	for (int j = 0; j < map->height(); ++j) {
		for (int i = 0; i < map->width(); ++i) {
			Int64 num = bytes[k++];
			num = Math::clamp(num, (Int64)0, (Int64)n - 1);
			map->set(i, j, (int)num);

			if (k >= (int)bytes.size())
				break;
		}

		if (k >= (int)bytes.size())
			break;
	}

	// Finish.
	return true;
}

bool SceneAssets::Entry::serializeJson(std::string &val, bool pretty) const {
	// Prepare.
	val.clear();

	// Serialize the layers.
	rapidjson::Document doc;

	Jpath::set(doc, doc, Jpath::ANY(), "layers");
	rapidjson::Value* layers = nullptr;
	Jpath::get(doc, layers, "layers");

	Jpath::set(doc, doc, Jpath::ANY(), "definition");
	rapidjson::Value* jdef = nullptr;
	Jpath::get(doc, jdef, "definition");
	definition.toJson(*jdef, doc);

	rapidjson::Document doc_;
	if (!data->toJson(doc_))
		return false;
	layers->Set(doc_.GetObject());

	if (actorRoutineOverridings.empty()) {
		Jpath::set(doc, doc, nullptr, "actor_routine_overridings");
	} else {
		for (int i = 0; i < (int)actorRoutineOverridings.size(); ++i) {
			const ActorRoutineOverriding &overriding = actorRoutineOverridings[i];
			Jpath::set(doc, doc, Jpath::ANY(), "actor_routine_overridings", i);
			rapidjson::Value* overriding_ = nullptr;
			Jpath::get(doc, overriding_, "actor_routine_overridings", i);
			Routine::toJson(overriding, *overriding_, doc);
		}
	}

	// Convert to string.
	std::string str;
	if (!Json::toString(doc, str, pretty))
		return false;

	val = str;

	// Finish.
	return true;
}

bool SceneAssets::Entry::parseJson(Scene::Ptr &scene, scene_t &def, ActorRoutineOverriding::Array &actorRoutines, const std::string &val, ParsingStatuses &status) const {
	// Prepare.
	scene = nullptr;
	status = ParsingStatuses::SUCCESS;

	// Convert from string.
	rapidjson::Document doc;
	if (!Json::fromString(doc, val.c_str()))
		return false;

	if (!doc.IsObject())
		return false;

	// Parse the layers.
	const rapidjson::Value* layers = nullptr;
	if (!Jpath::get(doc, layers, "layers"))
		return false;
	if (!layers || !layers->IsObject())
		return false;

	rapidjson::Value* jdef = nullptr;
	if (Jpath::get(doc, jdef, "definition") && jdef) {
		if (!def.fromJson(*jdef))
			def = scene_t();
	}

	rapidjson::Value* jactorRoutineOverridings = nullptr;
	if (Jpath::get(doc, jactorRoutineOverridings, "actor_routine_overridings") && jactorRoutineOverridings) {
		if (jactorRoutineOverridings->IsArray()) {
			actorRoutines.clear();
			const int n = Jpath::count(*jactorRoutineOverridings);
			for (int i = 0; i < n; ++i) {
				rapidjson::Value* overriding_ = nullptr;
				Jpath::get(doc, overriding_, "actor_routine_overridings", i);
				ActorRoutineOverriding overriding;
				if (Routine::fromJson(overriding, *overriding_))
					actorRoutines.push_back(overriding);
			}
			if (!actorRoutines.empty()) {
				actorRoutines.shrink_to_fit();
				Routine::sort(actorRoutines);
			}
		}
	}

	// Fill in with the layers.
	Scene* ptr = nullptr;
	if (!data->clone(&ptr, false))
		return false;
	scene = Scene::Ptr(ptr);

	Map::Tiles propsTiles;
	data->propertyLayer()->tiles(propsTiles);
	const Texture::Ptr &propstex = propsTiles.texture;

	Map::Tiles actorsTiles;
	data->actorLayer()->tiles(actorsTiles);
	const Texture::Ptr &actorstex = actorsTiles.texture;

	if (!scene->fromJson(*layers, propstex, actorstex))
		return false;

	// Finish.
	return true;
}

bool SceneAssets::Entry::toString(std::string &val, WarningOrErrorHandler onWarningOrError) const {
	// Prepare.
	val.clear();

	// Serialize the ref and layers.
	rapidjson::Document doc;

	Jpath::set(doc, doc, refMap, "ref_map");

	Jpath::set(doc, doc, Jpath::ANY(), "layers");
	rapidjson::Value* layers = nullptr;
	Jpath::get(doc, layers, "layers");

	Jpath::set(doc, doc, name, "name");

	Jpath::set(doc, doc, magnification, "magnification");

	Jpath::set(doc, doc, Jpath::ANY(), "definition");
	rapidjson::Value* jdef = nullptr;
	Jpath::get(doc, jdef, "definition");
	definition.toJson(*jdef, doc);

	if (actorRoutineOverridings.empty()) {
		Jpath::set(doc, doc, nullptr, "actor_routine_overridings");
	} else {
		for (int i = 0; i < (int)actorRoutineOverridings.size(); ++i) {
			const ActorRoutineOverriding &overriding = actorRoutineOverridings[i];
			Jpath::set(doc, doc, Jpath::ANY(), "actor_routine_overridings", i);
			rapidjson::Value* overriding_ = nullptr;
			Jpath::get(doc, overriding_, "actor_routine_overridings", i);
			Routine::toJson(overriding, *overriding_, doc);
		}
	}

	rapidjson::Document doc_;
	if (!data->toJson(doc_)) {
		assetsRaiseWarningOrError("Cannot serialize scene to JSON.", false, onWarningOrError);

		return false;
	}
	layers->Set(doc_.GetObject());

	// Convert to string.
	Json::Ptr json(Json::create());
	if (!json->fromJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert scene to JSON.", false, onWarningOrError);

		return false;
	}
	std::string str;
	if (!json->toString(str, ASSETS_PRETTY_JSON_ENABLED)) {
		assetsRaiseWarningOrError("Cannot serialize JSON.", false, onWarningOrError);

		return false;
	}

	val = str;

	// Finish.
	return true;
}

bool SceneAssets::Entry::fromString(const std::string &val, WarningOrErrorHandler onWarningOrError) {
	// Convert from string.
	Json::Ptr json(Json::create());
	if (!json->fromString(val)) {
		assetsRaiseWarningOrError("Cannot parse JSON.", false, onWarningOrError);

		return false;
	}
	rapidjson::Document doc;
	if (!json->toJson(doc)) {
		assetsRaiseWarningOrError("Cannot convert scene from JSON.", false, onWarningOrError);

		return false;
	}

	if (!doc.IsObject()) {
		assetsRaiseWarningOrError("Invalid JSON object.", false, onWarningOrError);

		return false;
	}

	// Parse the ref and layers.
	if (!Jpath::get(doc, refMap, "ref_map"))
		refMap = 0;

	const rapidjson::Value* layers = nullptr;
	if (!Jpath::get(doc, layers, "layers")) {
		assetsRaiseWarningOrError("Cannot find \"layers\" entry in JSON.", false, onWarningOrError);

		return false;
	}
	if (!layers || !layers->IsObject()) {
		assetsRaiseWarningOrError("Invalid \"layers\" entry.", false, onWarningOrError);

		return false;
	}

	if (!Jpath::get(doc, name, "name"))
		name.clear();

	if (!Jpath::get(doc, magnification, "magnification"))
		magnification = -1;

	rapidjson::Value* jdef = nullptr;
	if (Jpath::get(doc, jdef, "definition") && jdef) {
		if (!definition.fromJson(*jdef))
			definition = scene_t();
	}

	rapidjson::Value* jactorRoutineOverridings = nullptr;
	if (Jpath::get(doc, jactorRoutineOverridings, "actor_routine_overridings") && jactorRoutineOverridings) {
		if (jactorRoutineOverridings->IsArray()) {
			actorRoutineOverridings.clear();
			const int n = Jpath::count(*jactorRoutineOverridings);
			for (int i = 0; i < n; ++i) {
				rapidjson::Value* overriding_ = nullptr;
				Jpath::get(doc, overriding_, "actor_routine_overridings", i);
				ActorRoutineOverriding overriding;
				if (Routine::fromJson(overriding, *overriding_))
					actorRoutineOverridings.push_back(overriding);
			}
			if (!actorRoutineOverridings.empty()) {
				actorRoutineOverridings.shrink_to_fit();
				Routine::sort(actorRoutineOverridings);
			}
		}
	}

	// Fill in with the layers.
	Map::Tiles propsTiles;
	data->propertyLayer()->tiles(propsTiles);
	const Texture::Ptr &propstex = propsTiles.texture;

	Map::Tiles actorsTiles;
	data->actorLayer()->tiles(actorsTiles);
	const Texture::Ptr &actorstex = actorsTiles.texture;

	if (!data->fromJson(*layers, propstex, actorstex)) {
		assetsRaiseWarningOrError("Cannot convert scene from JSON.", false, onWarningOrError);

		return false;
	}

	// Finish.
	return true;
}

bool SceneAssets::Entry::fromString(const char* val, size_t len, WarningOrErrorHandler onWarningOrError) {
	const std::string val_(val, len);

	return fromString(val_, onWarningOrError);
}

bool SceneAssets::empty(void) const {
	return entries.empty();
}

int SceneAssets::count(void) const {
	return (int)entries.size();
}

bool SceneAssets::add(const Entry &entry) {
	entries.push_back(entry);

	return true;
}

bool SceneAssets::remove(int index) {
	if (index < 0 || index >= (int)entries.size())
		return false;

	entries.erase(entries.begin() + index);

	return true;
}

const SceneAssets::Entry* SceneAssets::get(int index) const {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

SceneAssets::Entry* SceneAssets::get(int index) {
	if (index < 0 || index >= (int)entries.size())
		return nullptr;

	return &entries[index];
}

const SceneAssets::Entry* SceneAssets::find(const std::string &name, int* index) const {
	if (index)
		*index = -1;

	if (entries.empty())
		return nullptr;

	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			return &entry;
		}
	}

	return nullptr;
}

const SceneAssets::Entry* SceneAssets::fuzzy(const std::string &name, int* index, std::string &gotName) const {
	if (index)
		*index = -1;
	gotName.clear();

	if (entries.empty())
		return nullptr;

	double fuzzyScore = 0.0;
	int fuzzyIdx = -1;
	std::string fuzzyName;
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name) {
			if (index)
				*index = i;

			gotName = entry.name;

			return &entry;
		}

		if (entry.name.empty())
			continue;

		const double score = rapidfuzz::fuzz::ratio(entry.name, name);
		if (score < ASSET_FUZZY_MATCHING_SCORE_THRESHOLD)
			continue;
		if (score > fuzzyScore) {
			fuzzyScore = score;
			fuzzyIdx = i;
			fuzzyName = entry.name;
		}
	}

	if (fuzzyIdx >= 0) {
		if (index)
			*index = fuzzyIdx;

		gotName = fuzzyName;

		const Entry &entry = entries[fuzzyIdx];

		return &entry;
	}

	return nullptr;
}

int SceneAssets::indexOf(const std::string &name) const {
	for (int i = 0; i < (int)entries.size(); ++i) {
		const Entry &entry = entries[i];
		if (entry.name == name)
			return i;
	}

	return -1;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Assets bundle
*/

AssetsBundle::AssetsBundle() {
}

AssetsBundle::~AssetsBundle() {
}

void AssetsBundle::clone(AssetsBundle* other, FontGetter getFont, CodeGetter getCode) const {
	if (!other)
		return;

	other->palette = palette;
	other->fonts = fonts;
	other->code = code;
	other->tiles = tiles;
	other->maps = maps;
	other->music = music;
	other->sfx = sfx;
	other->actors = actors;
	other->scenes = scenes;

	if (getFont) {
		FontAssets fonts_;
		for (int i = 0; i < other->fonts.count(); ++i) {
			const FontAssets::Entry* entry = other->fonts.get(i);
			const FontAssets::Entry newEntry = getFont(entry);
			fonts_.add(newEntry);
		}
		other->fonts = fonts_;
	}

	if (getCode) {
		CodeAssets code_;
		for (int i = 0; i < other->code.count(); ++i) {
			const CodeAssets::Entry* entry = other->code.get(i);
			const CodeAssets::Entry newEntry(getCode(entry));
			code_.add(newEntry);
		}
		other->code = code_;
	}
}

void AssetsBundle::clone(CodeAssets* other, CodeGetter getCode) const {
	if (!other)
		return;

	*other = code;

	if (getCode) {
		CodeAssets code_;
		for (int i = 0; i < other->count(); ++i) {
			const CodeAssets::Entry* entry = other->get(i);
			const CodeAssets::Entry newEntry(getCode(entry));
			code_.add(newEntry);
		}
		*other = code_;
	}
}

std::string AssetsBundle::nameOf(Categories category) {
	switch (category) {
	case Categories::PALETTE:
		return "palette";
	case Categories::FONT:
		return "font";
	case Categories::CODE:
		return "code";
	case Categories::TILES:
		return "tiles";
	case Categories::MAP:
		return "map";
	case Categories::MUSIC:
		return "music";
	case Categories::SFX:
		return "sfx";
	case Categories::ACTOR:
		return "actor";
	case Categories::PROJECTILE:
		return "projectile";
	case Categories::SCENE:
		return "scene";
	default:
		GBBASIC_ASSERT(false && "Impossible.");

		break;
	}

	return "";
}

/* ===========================================================================} */
