/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "activities.h"
#include "../utils/file_handle.h"
#include "../utils/json.h"
#include "../utils/text.h"
#if defined GBBASIC_OS_WIN
#   ifndef and
#       define and &&
#   endif /* and */
#   ifndef or
#       define or ||
#   endif /* or */
#endif /* GBBASIC_OS_WIN */
#if defined GBBASIC_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4267)
#endif /* GBBASIC_CP_VC */
#include "../../lib/bigint/include/constructors/constructors.hpp"
#include "../../lib/bigint/include/functions/conversion.hpp"
#include "../../lib/bigint/include/functions/math.hpp"
#include "../../lib/bigint/include/functions/random.hpp"
#include "../../lib/bigint/include/functions/utility.hpp"
#include "../../lib/bigint/include/operators/arithmetic_assignment.hpp"
#include "../../lib/bigint/include/operators/assignment.hpp"
#include "../../lib/bigint/include/operators/binary_arithmetic.hpp"
#include "../../lib/bigint/include/operators/increment_decrement.hpp"
#include "../../lib/bigint/include/operators/io_stream.hpp"
#include "../../lib/bigint/include/operators/relational.hpp"
#include "../../lib/bigint/include/operators/unary_arithmetic.hpp"
#include "../../lib/bigint/include/third_party/catch.hpp"
#if defined GBBASIC_CP_VC
#	pragma warning(pop)
#endif /* GBBASIC_CP_VC */
#include "../../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Activities
*/

Activities::Activities() {
}

Activities::~Activities() {
}

bool Activities::fromFile(const char* path) {
	File::Ptr file(File::create());
	if (!file->open(path, Stream::READ))
		return false;

	rapidjson::Document doc;
	doc.SetNull();

	std::string buf;
	if (!file->readString(buf)) {
		file->close();

		return false;
	}
	if (!Json::fromString(doc, buf.c_str())) {
		file->close();

		return false;
	}

	file->close();

	int errors = 0;
	std::string opened_;
	if (!Jpath::get(doc, opened_, "opened", "total")) {
		opened_ = "0";
		++errors;
	}
	std::string openedBasic_;
	if (!Jpath::get(doc, openedBasic_, "opened", "basic")) {
		openedBasic_ = "0";
		++errors;
	}
	std::string openedRom_;
	if (!Jpath::get(doc, openedRom_, "opened", "rom")) {
		openedRom_ = "0";
		++errors;
	}

	std::string created_;
	if (!Jpath::get(doc, created_, "created", "total")) {
		created_ = "0";
		++errors;
	}
	std::string createdBasic_;
	if (!Jpath::get(doc, createdBasic_, "created", "basic")) {
		createdBasic_ = "0";
		++errors;
	}

	std::string played_;
	if (!Jpath::get(doc, played_, "played", "total")) {
		played_ = "0";
		++errors;
	}
	std::string playedBasic_;
	if (!Jpath::get(doc, playedBasic_, "played", "basic")) {
		playedBasic_ = "0";
		++errors;
	}
	std::string playedRom_;
	if (!Jpath::get(doc, playedRom_, "played", "rom")) {
		playedRom_ = "0";
		++errors;
	}

	std::string playedTime_;
	if (!Jpath::get(doc, playedTime_, "played_time", "total")) {
		playedTime_ = "0";
		++errors;
	}
	std::string playedTimeBasic_;
	if (!Jpath::get(doc, playedTimeBasic_, "played_time", "basic")) {
		playedTimeBasic_ = "0";
		++errors;
	}
	std::string playedTimeRom_;
	if (!Jpath::get(doc, playedTimeRom_, "played_time", "rom")) {
		playedTimeRom_ = "0";
		++errors;
	}

	std::string wroteCodeLines_;
	if (!Jpath::get(doc, wroteCodeLines_, "wrote_code_lines", "total")) {
		wroteCodeLines_ = "0";
		++errors;
	}

	opened = opened_;
		openedBasic = openedBasic_;
		openedRom = openedRom_;
	created = created_;
		createdBasic = createdBasic_;
	played = played_;
		playedBasic = playedBasic_;
		playedRom = playedRom_;
	playedTime = playedTime_;
		playedTimeBasic = playedTimeBasic_;
		playedTimeRom = playedTimeRom_;
	wroteCodeLines = wroteCodeLines_;

	return true;
}

bool Activities::toFile(const char* path) const {
	File::Ptr file(File::create());
	if (!file->open(path, Stream::WRITE))
		return false;

	rapidjson::Document doc;
	doc.SetNull();

	int errors = 0;
	if (!Jpath::set(doc, doc, opened.to_string(), "opened", "total")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, openedBasic.to_string(), "opened", "basic")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, openedRom.to_string(), "opened", "rom")) {
		++errors;
	}

	if (!Jpath::set(doc, doc, created.to_string(), "created", "total")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, createdBasic.to_string(), "created", "basic")) {
		++errors;
	}

	if (!Jpath::set(doc, doc, played.to_string(), "played", "total")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, playedBasic.to_string(), "played", "basic")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, playedRom.to_string(), "played", "rom")) {
		++errors;
	}

	if (!Jpath::set(doc, doc, playedTime.to_string(), "played_time", "total")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, playedTimeBasic.to_string(), "played_time", "basic")) {
		++errors;
	}
	if (!Jpath::set(doc, doc, playedTimeRom.to_string(), "played_time", "rom")) {
		++errors;
	}

	if (!Jpath::set(doc, doc, wroteCodeLines.to_string(), "wrote_code_lines", "total")) {
		++errors;
	}

	std::string buf;
	if (!Json::toString(doc, buf)) {
		file->close();

		return false;
	}
	if (!file->writeString(buf)) {
		file->close();

		return false;
	}

	file->close();

	return true;
}

std::string Activities::formatTime(const BigInt &bigInt) {
	if (bigInt <= BigInt(0))
		return "0:00:00";

	const BigInt hr = bigInt / 3600;
	const BigInt tmp = bigInt % 3600;
	int rem = 0;
	Text::fromString(tmp.to_string(), rem);
	const int min = rem / 60;
	const int sec = rem % 60;

	std::string ret;
	ret += hr.to_string();
	ret += ":";
	ret += Text::toString(min, 2, '0');
	ret += ":";
	ret += Text::toString(sec, 2, '0');

	return ret;
}

/* ===========================================================================} */
