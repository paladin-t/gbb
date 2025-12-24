/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "bytes.h"
#include "music.h"
#include "parsers.h"
#include "text.h"

/*
** {===========================================================================
** Utilities
*/

#if true
namespace Jpath {

static bool getValue(const rapidjson::Value &obj, Music::NullableInt &ret) {
	if (obj.IsNull()) {
		ret = Right<std::nullptr_t>(nullptr);

		return true;
	} else if (obj.IsNumber()) {
		ret = Left<int>(obj.GetInt());

		return true;
	}

	return false;
}
static void setValue(rapidjson::Value &obj, const Music::NullableInt &src, rapidjson::Document &) {
	if (src.isRight()) {
		obj.SetNull();
	} else /* if (src.isLeft()) */ {
		obj.SetInt(src.left().get());
	}
}

}

#include "../../lib/jpath/jpath.hpp"
#endif

/* ===========================================================================} */

/*
** {===========================================================================
** Music
*/

class MusicImpl : public Music {
private:
	Song _song;

public:
	MusicImpl() {
	}
	virtual ~MusicImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Music** ptr, bool /* represented */) const override {
		if (!ptr)
			return false;

		*ptr = nullptr;

		MusicImpl* result = static_cast<MusicImpl*>(Music::create());
		result->_song = _song;

		*ptr = result;

		return true;
	}
	virtual bool clone(Music** ptr) const override {
		return clone(ptr, true);
	}
	virtual bool clone(Object** ptr) const override {
		Music* obj = nullptr;
		if (!clone(&obj, true))
			return false;

		*ptr = obj;

		return true;
	}

	virtual size_t hash(void) const override {
		size_t result = 0;

		result = Math::hash(result, _song.hash(false));

		return result;
	}
	virtual int compare(const Music* other) const override {
		const MusicImpl* implOther = static_cast<const MusicImpl*>(other);

		if (this == other)
			return 0;

		if (!other)
			return 1;

		return _song.compare(implOther->_song, false);
	}

	virtual const Song* pointer(void) const override {
		return &_song;
	}
	virtual Song* pointer(void) override {
		return &_song;
	}

	virtual bool load(const Song &data) override {
		_song = data;

		return true;
	}
	virtual void unload(void) override {
		_song.~Song();
		new (&_song) Song(); // To avoid stack overflow.
	}

	virtual bool toC(std::string &val) const override {
		// Prepare.
		typedef std::map<Math::Vec2i, int> OrderIndices;
		typedef std::vector<Pattern> FilledPatterns;

		OrderIndices orderIndices;
		FilledPatterns filledPatterns;

		val.clear();

		const std::string name = _song.name.empty() ? "Song" : Text::toVariableName(_song.name);
		const std::string pascalName = Text::snakeToPascal(name);
		const std::string snakeName = Text::pascalToSnake(name);

		// Serialize the header.
		val += "#include \"hUGEDriver.h\"\n";
		val += "#include <stddef.h>\n";
		val += "\n";

		// Serialize the order count.
		val += Text::format("static const unsigned char order_cnt = {0};\n", { Text::toString((UInt32)_song.channels.front().size() * 2) });
		val += "\n";

		// Serialize the patterns.
		const Orders &orders = _song.orders;
		const Song::OrderMatrix ordMat = _song.orderMatrix();
		for (int i = 0; i < (int)ordMat.size(); ++i) {
			const Order &order = orders[i];
			const Song::OrderSet &ordSet = ordMat[i];
			for (int ord : ordSet) {
				const Pattern &pattern = order[ord];
				FilledPatterns::const_iterator it = std::find(filledPatterns.begin(), filledPatterns.end(), pattern); // Ignore for duplicate pattern.
				if (it == filledPatterns.end()) {
					val += Text::format("static const unsigned char P{0}[] = {\n", { Text::toString((UInt32)filledPatterns.size()) });
					for (int j = 0; j < (int)pattern.size(); ++j) {
						const Cell &cell = pattern[j];
						val += Text::format(
							"\tDN({0}, {1}, {2}),\n",
							{
								nameOf(cell.note),
								Text::toString(cell.instrument),
								"0x" + Text::toHex((cell.effectCode << 8) | cell.effectParameters, 3, '0', true)
							}
						);
					}
					orderIndices[Math::Vec2i(i, ord)] = (int)filledPatterns.size();
					filledPatterns.push_back(pattern);
					val += "};\n";
				} else {
					const int idx = (int)(it - filledPatterns.begin());
					orderIndices[Math::Vec2i(i, ord)] = idx;
				}
			}
		}
		val += "\n";

		// Serialize the orders.
		for (int i = 0; i < (int)_song.channels.size(); ++i) {
			val += Text::format("static const unsigned char* const order{0}[] = {\n", { Text::toString(i + 1) });
			val += "\t";
			const Sequence &seq = _song.channels[i];
			for (int j = 0; j < (int)seq.size(); ++j) {
				const int ord = seq[j];

				OrderIndices::const_iterator it = orderIndices.find(Math::Vec2i(i, ord));
				if (it == orderIndices.end()) {
					GBBASIC_ASSERT(false && "Impossible.");

					return false;
				}

				const int idx = it->second;
				val += Text::format("P{0}", { Text::toString(idx) });
				if (j != (int)seq.size() - 1)
					val += ", ";
			}
			val += "\n};\n";
		}
		val += "\n";

		// Serialize the instruments.
		val += "static const unsigned char duty_instruments[] = {\n";
		for (int i = 0; i < (int)_song.dutyInstruments.size(); ++i) {
			const DutyInstrument &inst = _song.dutyInstruments[i];

			Bytes::Ptr bytes(Bytes::create());
			if (!inst.serialize(bytes.get()))
				return false;

			val += "\t";
			for (int j = 0; j < (int)bytes->count(); ++j) {
				val += Text::toString(bytes->get(j));
				if (j != (int)bytes->count() - 1)
					val += ", ";
			}
			val += ",\n";
		}
		val += "};\n";
		val += "static const unsigned char wave_instruments[] = {\n";
		for (int i = 0; i < (int)_song.waveInstruments.size(); ++i) {
			const WaveInstrument &inst = _song.waveInstruments[i];

			Bytes::Ptr bytes(Bytes::create());
			if (!inst.serialize(bytes.get()))
				return false;

			val += "\t";
			for (int j = 0; j < (int)bytes->count(); ++j) {
				val += Text::toString(bytes->get(j));
				if (j != (int)bytes->count() - 1)
					val += ", ";
			}
			val += ",\n";
		}
		val += "};\n";
		val += "static const unsigned char noise_instruments[] = {\n";
		for (int i = 0; i < (int)_song.noiseInstruments.size(); ++i) {
			const NoiseInstrument &inst = _song.noiseInstruments[i];

			Bytes::Ptr bytes(Bytes::create());
			if (!inst.serialize(bytes.get()))
				return false;

			val += "\t";
			for (int j = 0; j < (int)bytes->count(); ++j) {
				val += Text::toString(bytes->get(j));
				if (j != (int)bytes->count() - 1)
					val += ", ";
			}
			val += ",\n";
		}
		val += "};\n";
		val += "\n";

		// Serialize the waves.
		val += "static const unsigned char waves[] = {\n";
		for (int i = 0; i < (int)_song.waves.size(); ++i) {
			const Wave &wave = _song.waves[i];

			Bytes::Ptr bytes(Bytes::create());
			if (!serialize(bytes.get(), wave))
				return false;

			val += "\t";
			for (int j = 0; j < (int)bytes->count(); ++j) {
				val += Text::toString(bytes->get(j));
				if (j != (int)bytes->count() - 1)
					val += ", ";
			}
			val += ",\n";
		}
		val += "};\n";
		val += "\n";

		// Serialize the song.
		val += "const hUGESong_t " + pascalName + " = {\n";
		val += Text::format("\t{0},\n", { Text::toString(_song.ticksPerRow) });
		val += "\t&order_cnt,\n";
		val += "\torder1, order2, order3, order4,\n";
		val += "\tduty_instruments, wave_instruments, noise_instruments,\n";
		val += "\tNULL,\n";
		val += "\twaves\n";
		val += "};\n";

		// Finish.
		return true;
	}
	virtual bool fromC(const char* filename, const std::string &val, ErrorHandler onError) override {
		// Prepare.
		typedef std::vector<Cell> Cells;
		typedef std::map<int /* pattern index */, int /* sorted pattern index */> PatternMapper;
		typedef std::map<int /* sorted pattern index */, Cells> Patterns;
		typedef std::vector<int> Values;

		const Parsers::CParser::Entry::List SIMPLE_ASSIGN_EXPR = {
			Parsers::CParser::Entry("typeident|>"),
			Parsers::CParser::Entry("char", "="),
			Parsers::CParser::Entry("lexp|term|factor|number|regex"),
			Parsers::CParser::Entry("char", ";")
		};
		const Parsers::CParser::Entry::List STRUCT_ASSIGN_EXPR = {
			Parsers::CParser::Entry("typeident|>"),
			Parsers::CParser::Entry("char", "="),
			Parsers::CParser::Entry("char", "{"),
			Parsers::CParser::Entry("list|>"),
			Parsers::CParser::Entry("char", "}"),
			Parsers::CParser::Entry("char", ";")
		};
		const Parsers::CParser::Entry::List ARRAY_ASSIGN_EXPR = {
			Parsers::CParser::Entry("typeident|>"),
			Parsers::CParser::Entry("char", "["),
			Parsers::CParser::Entry("char", "]"),
			Parsers::CParser::Entry("char", "="),
			Parsers::CParser::Entry("char", "{"),
			Parsers::CParser::Entry("list|>"),
			Parsers::CParser::Entry("char", "}"),
			Parsers::CParser::Entry("char", ";")
		};
		const Parsers::CParser::Entry::List CELL_STRUCT = {
			Parsers::CParser::Entry("ident|regex"),
			Parsers::CParser::Entry("char", "("),
			Parsers::CParser::Entry("lexp|term|factor|ident|regex"),
			Parsers::CParser::Entry("char", ","),
			Parsers::CParser::Entry("lexp|term|factor|number|regex"),
			Parsers::CParser::Entry("char", ","),
			Parsers::CParser::Entry("lexp|term|factor|number|regex"),
			Parsers::CParser::Entry("char", ")")
		};

		auto findDecls = [] (Parsers::CParser &parser, mpc_ast_t* ast, Parsers::CParser::Children &decls) -> bool {
			Parsers::CParser::Children children;
			if (!parser.is(ast, Parsers::CParser::Entry(">")))
				return false;
			if (!parser.children(ast, &children))
				return false;
			mpc_ast_t* decls_ = parser.find(&children, Parsers::CParser::Entry("decls|>"));
			if (!decls_)
				return false;
			if (!parser.children(decls_, &decls))
				return false;

			return true;
		};
		auto getId = [] (Parsers::CParser &parser, const Parsers::CParser::Children &seq, std::string &id) -> bool {
			Parsers::CParser::Children children;
			id.clear();
			if (seq.empty() || !parser.children(seq[0], &children) || children.size() != 2)
				return false;
			if (!parser.is(children[1], Parsers::CParser::Entry("ident|regex")))
				return false;

			if (!children[1]->contents)
				return false;
			id = children[1]->contents;

			return true;
		};
		auto getDecl = [] (Parsers::CParser &parser, mpc_ast_t* ast, Parsers::CParser::Children &decl) -> bool {
			decl.clear();
			if (!parser.is(ast, Parsers::CParser::Entry("typeident|>")))
				return false;
			if (!parser.children(ast, &decl))
				return false;

			return true;
		};
		auto getList = [] (Parsers::CParser &parser, mpc_ast_t* ast, Parsers::CParser::Children &list) -> bool {
			list.clear();
			if (!parser.is(ast, Parsers::CParser::Entry("list|>")))
				return false;
			if (!parser.children(ast, &list))
				return false;

			return true;
		};

		auto parsePIndex = [] (Parsers::CParser &/* parser */, const std::string &id, int &ord) -> bool {
			ord = -1;
			const std::string prefix = "P";
			if (!Text::startsWith(id, prefix, false))
				return false;
			const std::string num = id.substr(prefix.length());
			if (num.empty() || !Text::fromString(num, ord))
				return false;

			return true;
		};
		auto parseOrderIndex = [] (Parsers::CParser &/* parser */, const std::string &id, int &ord) -> bool {
			ord = -1;
			const std::string prefix = "order";
			if (!Text::startsWith(id, prefix, false))
				return false;
			const std::string num = id.substr(prefix.length());
			if (num.empty() || !Text::fromString(num, ord))
				return false;

			return true;
		};
		auto parseCell = [CELL_STRUCT] (Parsers::CParser &parser, mpc_ast_t* ast, Cell &cell) -> bool {
			cell = Cell();
			Parsers::CParser::Children children;
			if (!parser.children(ast, &children))
				return false;
			Parsers::CParser::Children seq;
			if (!parser.match(&children, CELL_STRUCT, &seq))
				return false;

			if (seq.size() != 8 || !seq[2]->contents || !seq[4]->contents || !seq[6]->contents)
				return false;
			int note = 0;
			int inst = 0;
			int fx = 0;
			note = noteOf(seq[2]->contents);
			if (!Text::fromString(seq[4]->contents, inst))
				return false;
			if (!Text::fromString(seq[6]->contents, fx))
				return false;

			cell.note             = note;
			cell.instrument       = inst;
			cell.effectCode       = fx >> 8;
			cell.effectParameters = fx & 0x0ff;

			return true;
		};

		// The error handler.
		auto onError_ = [onError] (const char* msg) -> void {
			if (onError)
				onError(msg);
		};

		// Parse the data.
		bool result = true;
		Parsers::CParser parser("\"hUGESong_t\"", onError);
		std::string val_ = val;
		size_t start = 0;
		for (; ; ) {
			const size_t pos = Text::indexOf(val_, "static const unsigned char* const order", start);
			if (pos == std::string::npos)
				break;
			const size_t end = Text::indexOf(val_, "};", pos);
			if (end == std::string::npos)
				break;
			val_.insert(val_.begin() + end, ',');
			start = end + 3;
		}
		if (parser.parse(filename, val_.c_str(), val_.length())) {
			// Parse the parsed AST.
			mpc_ast_t* ast = (mpc_ast_t*)parser.result.output;

			do {
				// Parse the declarations.
				Parsers::CParser::Children decls;
				if (!findDecls(parser, ast, decls))
					break;

				Song song;
				int orderCount = -1;
				PatternMapper patternMapper;
				Patterns patterns;
				Values dutyInstruments;
				Values waveInstruments;
				Values noiseInstruments;
				Values waves;
				Channels channels; Values filledChannels;
				std::string name;
				int ticksPerRow = 0;
				while (!decls.empty()) {
					Parsers::CParser::Children seq;
					if (parser.match(&decls, SIMPLE_ASSIGN_EXPR, &seq)) { // Parse order count.
						if (seq.size() != SIMPLE_ASSIGN_EXPR.size()) {
							result = false;

							break;
						}
						std::string id;
						if (!getId(parser, seq, id) || id != "order_cnt") {
							onError_("No order count.");

							result = false;

							break;
						}
						if (!seq[2]->contents || !Text::fromString(seq[2]->contents, orderCount)) {
							onError_("Invalid order count.");

							result = false;

							break;
						}
						if (orderCount / 2 > GBBASIC_MUSIC_SEQUENCE_MAX_COUNT) {
							onError_("Invalid order count.");
						}
					} else if (parser.match(&decls, ARRAY_ASSIGN_EXPR, &seq)) { // Parse patterns, orders, instruments or waves.
						if (seq.size() != ARRAY_ASSIGN_EXPR.size()) {
							result = false;

							break;
						}
						std::string id;
						if (!getId(parser, seq, id)) {
							const std::string msg = Text::format("Invalid ID \"{0}\".", { id });
							onError_(msg.c_str());

							result = false;

							break;
						}
						bool isDutyInst = false;
						bool isWaveInst = false;
						bool isNoiseInst = false;
						bool isWave = false;
						const char* type = nullptr;
						if (id == "duty_instruments") {
							isDutyInst = true;
							type = "duty instrument";
						} else if (id == "wave_instruments") {
							isWaveInst = true;
							type = "wave instrument";
						} else if (id == "noise_instruments") {
							isNoiseInst = true;
							type = "noise instrument";
						} else if (id == "waves") {
							isWave = true;
							type = "wave";
						}

						int index = -1;
						if (parsePIndex(parser, id, index)) { // Parse patterns.
							Parsers::CParser::Children list;
							if (!getList(parser, seq[5], list)) {
								onError_("Invalid pattern list.");

								result = false;

								break;
							}
							Cells cells;
							bool ok = true;
							int i = 0;
							while (i < (int)list.size()) {
								if (!parser.is(list[i], Parsers::CParser::Entry("lexp|term|factor|>"))) {
									onError_("Invalid pattern list.");

									ok = false;

									break;
								}
								Cell cell;
								if (!parseCell(parser, list[i], cell)) {
									onError_("Invalid pattern cell.");

									ok = false;

									break;
								}
								cells.push_back(cell);
								if (cells.size() > GBBASIC_MUSIC_PATTERN_COUNT) {
									onError_("Too many pattern cells.");

									ok = false;

									break;
								}
								++i;
								if (i < (int)list.size()) {
									if (!parser.is(list[i], Parsers::CParser::Entry("char", ","))) {
										onError_("Comma expected.");

										ok = false;

										break;
									}
									++i;
								}
							}
							if (!ok) {
								result = false;

								break;
							}
							PatternMapper::const_iterator mit = patternMapper.find(index);
							if (mit != patternMapper.end()) {
								const std::string msg = Text::format("Duplicate pattern index {0}.", { Text::toString(index) });
								onError_(msg.c_str());

								result = false;

								break;
							}
							const int index_ = (int)patterns.size();
							Patterns::const_iterator pit = patterns.find(index_);
							if (pit != patterns.end()) {
								const std::string msg = Text::format("Duplicate sorted pattern index {0}.", { Text::toString(index) });
								onError_(msg.c_str());

								result = false;

								break;
							}
							patternMapper[index] = index_;
							patterns[index_] = cells;
						} else if (parseOrderIndex(parser, id, index)) { // Parse orders.
							const int ordIdx = index - 1; // 1-based.
							if (ordIdx < 0 || ordIdx >= GBBASIC_MUSIC_CHANNEL_COUNT) {
								const std::string msg = Text::format("Order index out of bounds {0}.", { Text::toString(ordIdx) });
								onError_(msg.c_str());

								result = false;

								break;
							}
							Parsers::CParser::Children list;
							if (!getList(parser, seq[5], list)) {
								onError_("Invalid order list.");

								result = false;

								break;
							}
							Sequence sequence;
							bool ok = true;
							int i = 0;
							while (i < (int)list.size()) {
								if (!parser.is(list[i], Parsers::CParser::Entry("lexp|term|factor|ident|regex"))) {
									onError_("Invalid order list.");

									ok = false;

									break;
								}
								if (!list[i]->contents) {
									onError_("Invalid order pattern.");

									ok = false;

									break;
								}
								const std::string id_ = list[i]->contents;
								int idx = -1;
								if (!parsePIndex(parser, id_, idx)) {
									const std::string msg = Text::format("Invalid order pattern {0}.", { id_ });
									onError_(msg.c_str());

									ok = false;

									break;
								}
								if (sequence.size() + 1 > GBBASIC_MUSIC_ORDER_COUNT) {
									onError_("Too many order patterns.");

									ok = false;

									break;
								}
								PatternMapper::const_iterator mit = patternMapper.find(idx);
								if (mit == patternMapper.end()) {
									const std::string msg = Text::format("Cannot find pattern index {0}.", { Text::toString(idx) });
									onError_(msg.c_str());

									result = false;

									break;
								}
								const int idx_ = mit->second;
								sequence.push_back(idx_);
								++i;
								if (i < (int)list.size()) {
									if (!parser.is(list[i], Parsers::CParser::Entry("char", ","))) {
										onError_("Comma expected.");

										ok = false;

										break;
									}
									++i;
								}
							}
							if (!ok) {
								result = false;

								break;
							}
							Values::const_iterator it = std::find(filledChannels.begin(), filledChannels.end(), ordIdx);
							if (it != filledChannels.end()) {
								onError_("Duplicate order list.");

								result = false;

								break;
							}
							channels[ordIdx] = sequence;
						} else if (isDutyInst || isWaveInst || isNoiseInst || isWave) { // Parse instruments or waves.
							Parsers::CParser::Children list;
							if (!getList(parser, seq[5], list)) {
								const std::string msg = Text::format("Invalid {0} type.", { type });
								onError_(msg.c_str());

								result = false;

								break;
							}
							Values values;
							bool ok = true;
							int i = 0;
							while (i < (int)list.size()) {
								if (!parser.is(list[i], Parsers::CParser::Entry("lexp|term|factor|number|regex"))) {
									const std::string msg = Text::format("Invalid {0} list.", { type });
									onError_(msg.c_str());

									ok = false;

									break;
								}
								if (!list[i]->contents) {
									const std::string msg = Text::format("Invalid {0} list.", { type });
									onError_(msg.c_str());

									ok = false;

									break;
								}
								const std::string id_ = list[i]->contents;
								int idx = -1;
								if (!Text::fromString(id_, idx)) {
									const std::string msg = Text::format("Invalid {0} data {1}.", { type, id_ });
									onError_(msg.c_str());

									ok = false;

									break;
								}
								values.push_back(idx);
								++i;
								if (i < (int)list.size()) {
									if (!parser.is(list[i], Parsers::CParser::Entry("char", ","))) {
										onError_("Comma expected.");

										ok = false;

										break;
									}
									++i;
								}
							}
							if (!ok) {
								result = false;

								break;
							}
							if (isDutyInst)
								dutyInstruments = values;
							else if (isWaveInst)
								waveInstruments = values;
							else if (isNoiseInst)
								noiseInstruments = values;
							else if (isWave)
								waves = values;
						} else {
							onError_("Unknown list.");

							result = false;

							break;
						}
					} else if (parser.match(&decls, STRUCT_ASSIGN_EXPR, &seq)) { // Parse the song.
						if (seq.size() != STRUCT_ASSIGN_EXPR.size()) {
							result = false;

							break;
						}
						Parsers::CParser::Children decl;
						if (!getDecl(parser, seq[0], decl)) {
							onError_("Invalid song structure.");

							result = false;

							break;
						}
						if (decl.size() != 2) {
							onError_("Invalid song data.");

							result = false;

							break;
						}
						if (!decl[1]->contents) {
							onError_("Invalid song data.");

							result = false;

							break;
						}
						name = decl[1]->contents;
						name = Text::replace(name, "_", " ");
						Parsers::CParser::Children list;
						if (!getList(parser, seq[3], list)) {
							onError_("Invalid song structure.");

							result = false;

							break;
						}
						if (list.size() != 21) {
							onError_("Invalid song data.");

							result = false;

							break;
						}
						if (!list[0]->contents || !Text::fromString(list[0]->contents, ticksPerRow)) {
							onError_("Invalid song data.");

							result = false;

							break;
						}
					} else {
						onError_("Unknown data.");

						result = false;

						break;
					}
				}
				if (!result)
					break;

				// Parse the orders.
				typedef std::shared_ptr<Orders> OrdersPtr;
				OrdersPtr orders(new Orders());
				for (int i = 0; i < (int)channels.size(); ++i) {
					const Sequence &seq = channels[i];
					for (int j = 0; j < (int)seq.size(); ++j) {
						const int idx = seq[j];
						Patterns::const_iterator it = patterns.find(idx);
						if (it == patterns.end()) {
							const std::string msg = Text::format("Cannot find pattern {0}.", { Text::toString(idx) });
							onError_(msg.c_str());

							result = false;

							break;
						} else {
							if (idx < 0 || idx >= (int)(*orders)[i].size()) {
								const std::string msg = Text::format("Order index {0} out of bound.", { Text::toString(idx) });
								onError_(msg.c_str());

								result = false;

								break;
							}
							const Cells &cells = it->second;
							for (int k = 0; k < (int)(*orders)[i][idx].size() && k < (int)cells.size(); ++k) {
								(*orders)[i][idx][k] = cells[k];
							}
						}
					}
					if (!result)
						break;
				}
				if (!result)
					break;

				// Parse the instruments.
				typedef std::shared_ptr<DutyInstrument::Array> DutyInstrumentArrayPtr;
				DutyInstrumentArrayPtr dutyInstruments_(new DutyInstrument::Array());
				for (int i = 0; i < (int)dutyInstruments_->size(); ++i) {
					constexpr const int N = 4;
					Bytes::Ptr bytes(Bytes::create());
					for (int j = 0; j < N; ++j)
						bytes->writeUInt8((UInt8)dutyInstruments[i * N + j]);
					if (!(*dutyInstruments_)[i].parse(bytes.get())) {
						const std::string msg = Text::format("Invalid duty instrument data {0}.", { Text::toString(i) });
						onError_(msg.c_str());

						result = false;

						break;
					}
				}
				if (!result)
					break;

				typedef std::shared_ptr<WaveInstrument::Array> WaveInstrumentArrayPtr;
				WaveInstrumentArrayPtr waveInstruments_(new WaveInstrument::Array());
				for (int i = 0; i < (int)waveInstruments_->size(); ++i) {
					constexpr const int N = 4;
					Bytes::Ptr bytes(Bytes::create());
					for (int j = 0; j < N; ++j)
						bytes->writeUInt8((UInt8)waveInstruments[i * N + j]);
					if (!(*waveInstruments_)[i].parse(bytes.get())) {
						const std::string msg = Text::format("Invalid wave instrument data {0}.", { Text::toString(i) });
						onError_(msg.c_str());

						result = false;

						break;
					}
				}
				if (!result)
					break;

				typedef std::shared_ptr<NoiseInstrument::Array> NoiseInstrumentArrayPtr;
				NoiseInstrumentArrayPtr noiseInstruments_(new NoiseInstrument::Array());
				for (int i = 0; i < (int)noiseInstruments_->size(); ++i) {
					constexpr const int N = 8;
					Bytes::Ptr bytes(Bytes::create());
					for (int j = 0; j < N; ++j)
						bytes->writeUInt8((UInt8)noiseInstruments[i * N + j]);
					if (!(*noiseInstruments_)[i].parse(bytes.get())) {
						const std::string msg = Text::format("Invalid noise instrument data {0}.", { Text::toString(i) });
						onError_(msg.c_str());

						result = false;

						break;
					}
				}
				if (!result)
					break;

				// Parse the waves.
				typedef std::shared_ptr<Waves> WavesPtr;
				WavesPtr waves_(new Waves());
				for (int i = 0; i < (int)waves_->size(); ++i) {
					constexpr const int N = 16;
					Bytes::Ptr bytes(Bytes::create());
					for (int j = 0; j < N; ++j)
						bytes->writeUInt8((UInt8)waves[i * N + j]);
					if (!parse((*waves_)[i], bytes.get())) {
						const std::string msg = Text::format("Invalid wave data {0}.", { Text::toString(i) });
						onError_(msg.c_str());

						result = false;

						break;
					}
				}
				if (!result)
					break;

				// Construct song.
				song.name             = name.empty() ? _song.name : name;
				song.artist           = _song.artist;
				song.comment          = _song.comment;
				song.dutyInstruments  = *dutyInstruments_;
				song.waveInstruments  = *waveInstruments_;
				song.noiseInstruments = *noiseInstruments_;
				song.waves            = *waves_;
				song.ticksPerRow      = ticksPerRow;
				song.orders           = *orders;
				song.channels         = channels;
				_song                 = song;
			} while (false);

#if defined GBBASIC_DEBUG
			mpc_ast_print(ast);
#endif /* GBBASIC_DEBUG */
			mpc_ast_delete(ast);
		} else {
			// Handle error.
			mpc_err_t* err = parser.result.error;
			if (onError) {
				char* str = mpc_err_string(err);
				onError(str);
				free(str);
			} else {
				mpc_err_print(err);
			}
			mpc_err_delete(err);

			result = false;
		}

		// Finish.
		return result;
	}

	virtual bool toBytes(class Bytes* val) const override {
		rapidjson::Document doc;
		if (!toJson(doc, doc))
			return false;

		std::string txt;
		if (!Json::toString(doc, txt, false))
			return false;

		val->clear();
		val->writeString(txt);

		return true;
	}
	virtual bool fromBytes(const Byte* val, size_t size) override {
		unload();

		std::string txt;
		txt.assign((const char*)val, size);

		rapidjson::Document doc;

		if (!Json::fromString(doc, txt.c_str()))
			return false;

		return fromJson(doc);
	}
	virtual bool fromBytes(const class Bytes* val) override {
		return fromBytes(val->pointer(), val->count());
	}

	virtual bool toJson(rapidjson::Value &val, rapidjson::Document &doc) const override {
		return _song.toJson(val, doc);
	}
	virtual bool toJson(rapidjson::Document &val) const override {
		return toJson(val, val);
	}
	virtual bool fromJson(const rapidjson::Value &val) override {
		unload();

		return _song.fromJson(val);
	}
	virtual bool fromJson(const rapidjson::Document &val) override {
		const rapidjson::Value &jval = val;

		return fromJson(jval);
	}
};

Music::Cell::Cell() {
}

Music::Cell::Cell(int n, int in, int fx, int params) :
	note(n),
	instrument(in),
	effectCode(fx),
	effectParameters(params)
{
}

Music::Cell::~Cell() {
}

bool Music::Cell::operator == (const Cell &other) const {
	return compare(other) == 0;
}

bool Music::Cell::operator != (const Cell &other) const {
	return compare(other) != 0;
}

bool Music::Cell::operator < (const Cell &other) const {
	return compare(other) < 0;
}

bool Music::Cell::operator > (const Cell &other) const {
	return compare(other) > 0;
}

int Music::Cell::compare(const Cell &other) const {
	if (note < other.note)
		return -1;
	else if (note > other.note)
		return 1;

	if (instrument < other.instrument)
		return -1;
	else if (instrument > other.instrument)
		return 1;

	if (effectCode < other.effectCode)
		return -1;
	else if (effectCode > other.effectCode)
		return 1;

	if (effectParameters < other.effectParameters)
		return -1;
	else if (effectParameters > other.effectParameters)
		return 1;

	return 0;
}

bool Music::Cell::equals(const Cell &other) const {
	return compare(other) == 0;
}

bool Music::Cell::empty(void) const {
	return *this == Cell();
}

bool Music::Cell::serialize(Bytes* bytes) const {
	const Byte data[] = {
		MUSIC_ENCODE_NOTE(note, instrument, (effectCode << 8) | effectParameters)
	};
	static_assert(sizeof(data) == 3, "Wrong size.");

	for (int i = 0; i < GBBASIC_COUNTOF(data); ++i)
		bytes->writeUInt8(data[i]);

	return true;
}

bool Music::Cell::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!Jpath::set(doc, val, note, 0))
		return false;

	if (!Jpath::set(doc, val, instrument, 1))
		return false;

	if (!Jpath::set(doc, val, effectCode, 2))
		return false;

	if (!Jpath::set(doc, val, effectParameters, 3))
		return false;

	return true;
}

bool Music::Cell::fromJson(const rapidjson::Value &val) {
	if (!Jpath::get(val, note, 0))
		return false;

	if (!Jpath::get(val, instrument, 1))
		return false;

	if (!Jpath::get(val, effectCode, 2))
		return false;

	if (!Jpath::get(val, effectParameters, 3))
		return false;

	return true;
}

Music::Cell Music::Cell::BLANK(void) {
	return Cell();
}

Music::BaseInstrument::BaseInstrument() {
}

Music::BaseInstrument::~BaseInstrument() {
}

Music::DutyInstrument::DutyInstrument() {
}

Music::DutyInstrument::~DutyInstrument() {
}

bool Music::DutyInstrument::operator == (const DutyInstrument &other) const {
	return compare(other) == 0;
}

bool Music::DutyInstrument::operator != (const DutyInstrument &other) const {
	return compare(other) != 0;
}

bool Music::DutyInstrument::operator < (const DutyInstrument &other) const {
	return compare(other) < 0;
}

bool Music::DutyInstrument::operator > (const DutyInstrument &other) const {
	return compare(other) > 0;
}

int Music::DutyInstrument::compare(const DutyInstrument &other) const {
	if (name < other.name)
		return -1;
	else if (name > other.name)
		return 1;

	if (length < other.length)
		return -1;
	else if (length > other.length)
		return 1;

	if (initialVolume < other.initialVolume)
		return -1;
	else if (initialVolume > other.initialVolume)
		return 1;

	if (volumeSweepChange < other.volumeSweepChange)
		return -1;
	else if (volumeSweepChange > other.volumeSweepChange)
		return 1;

	if (frequencySweepTime < other.frequencySweepTime)
		return -1;
	else if (frequencySweepTime > other.frequencySweepTime)
		return 1;

	if (frequencySweepShift < other.frequencySweepShift)
		return -1;
	else if (frequencySweepShift > other.frequencySweepShift)
		return 1;

	if (dutyCycle < other.dutyCycle)
		return -1;
	else if (dutyCycle > other.dutyCycle)
		return 1;

	return 0;
}

bool Music::DutyInstrument::equals(const DutyInstrument &other) const {
	return compare(other) == 0;
}

bool Music::DutyInstrument::serialize(Bytes* bytes) const {
	const Byte nr10 = (Byte)(
		(frequencySweepTime << 4) |
		(frequencySweepShift < 0 ? 0x08 : 0x00) |
		std::abs(frequencySweepShift)
	);
	const Byte nr11 = (Byte)(
		(dutyCycle << 6) |
		((length.isLeft() ? 64 - length.left().get() : 0) & 0x3f)
	);
	Byte nr12 = (Byte)(
		(initialVolume << 4) |
		(volumeSweepChange > 0 ? 0x08 : 0x00)
	);
	if (volumeSweepChange != 0)
		nr12 |= 8 - std::abs(volumeSweepChange);
	const Byte nr14 = 0x80 | (length.isLeft() ? 0x40 : 0);

	if (!bytes->writeUInt8(nr10))
		return false;
	if (!bytes->writeUInt8(nr11))
		return false;
	if (!bytes->writeUInt8(nr12))
		return false;
	if (!bytes->writeUInt8(nr14))
		return false;

	return true;
}

bool Music::DutyInstrument::parse(const Bytes* bytes) {
	if (bytes->count() != 4)
		return false;

	const Byte b0 = bytes->get(0);
	const Byte b1 = bytes->get(1);
	const Byte b2 = bytes->get(2);
	const Byte b3 = bytes->get(3);

	DutyInstrument val;
	const Byte b31 = b3 & ~0x80;
	GBBASIC_ASSERT(b31 == 0x40 || b31 == 0 && "Wrong data.");
	const bool hasLength = b31 == 0x40;

	val.frequencySweepTime = b0 >> 4;
	const Byte b01 = b0 & 0x0f;
	if (b01 & 0x08)
		val.frequencySweepShift = -(b01 & ~0x08);
	else
		val.frequencySweepShift = b01;

	val.dutyCycle = b1 >> 6;
	const Byte b11 = b1 & 0x3f;
	if (hasLength)
		val.length = Left<int>(64 - b11);
	else
		val.length = Right<std::nullptr_t>(nullptr);

	val.initialVolume = b2 >> 4;
	const Byte b21 = b2 & 0x0f;
	if (b21 & 0x08)
		val.volumeSweepChange = 8 - (b21 & ~0x08);
	else
		val.volumeSweepChange = b21 == 0 ? 0 : -(8 - b21);

	*this = val;

	return true;
}

bool Music::DutyInstrument::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!Jpath::set(doc, val, name, "name"))
		return false;

	if (!Jpath::set(doc, val, length, "length"))
		return false;

	if (!Jpath::set(doc, val, initialVolume, "initial_volume"))
		return false;

	if (!Jpath::set(doc, val, volumeSweepChange, "volume_sweep_change"))
		return false;

	if (!Jpath::set(doc, val, frequencySweepTime, "frequency_sweep_time"))
		return false;

	if (!Jpath::set(doc, val, frequencySweepShift, "frequency_sweep_shift"))
		return false;

	if (!Jpath::set(doc, val, dutyCycle, "duty_cycle"))
		return false;

	return true;
}

bool Music::DutyInstrument::fromJson(const rapidjson::Value &val) {
	if (!Jpath::get(val, name, "name"))
		return false;

	if (!Jpath::get(val, length, "length"))
		return false;

	if (!Jpath::get(val, initialVolume, "initial_volume"))
		return false;

	if (!Jpath::get(val, volumeSweepChange, "volume_sweep_change"))
		return false;

	if (!Jpath::get(val, frequencySweepTime, "frequency_sweep_time"))
		return false;

	if (!Jpath::get(val, frequencySweepShift, "frequency_sweep_shift"))
		return false;

	if (!Jpath::get(val, dutyCycle, "duty_cycle"))
		return false;

	return true;
}

Music::WaveInstrument::WaveInstrument() {
}

Music::WaveInstrument::~WaveInstrument() {
}

bool Music::WaveInstrument::operator == (const WaveInstrument &other) const {
	return compare(other) == 0;
}

bool Music::WaveInstrument::operator != (const WaveInstrument &other) const {
	return compare(other) != 0;
}

bool Music::WaveInstrument::operator < (const WaveInstrument &other) const {
	return compare(other) < 0;
}

bool Music::WaveInstrument::operator > (const WaveInstrument &other) const {
	return compare(other) > 0;
}

int Music::WaveInstrument::compare(const WaveInstrument &other) const {
	if (name < other.name)
		return -1;
	else if (name > other.name)
		return 1;

	if (length < other.length)
		return -1;
	else if (length > other.length)
		return 1;

	if (volume < other.volume)
		return -1;
	else if (volume > other.volume)
		return 1;

	if (waveIndex < other.waveIndex)
		return -1;
	else if (waveIndex > other.waveIndex)
		return 1;

	return 0;
}

bool Music::WaveInstrument::equals(const WaveInstrument &other) const {
	return compare(other) == 0;
}

bool Music::WaveInstrument::serialize(Bytes* bytes) const {
	const Byte nr31 = (length.isLeft() ? length.left().get() : 0) & 0xff;
	const Byte nr32 = (Byte)(volume << 5);
	const Byte wave_nr = (Byte)waveIndex;
	const Byte nr34 = 0x80 | (length.isLeft() ? 0x40 : 0);

	if (!bytes->writeUInt8(nr31))
		return false;
	if (!bytes->writeUInt8(nr32))
		return false;
	if (!bytes->writeUInt8(wave_nr))
		return false;
	if (!bytes->writeUInt8(nr34))
		return false;

	return true;
}

bool Music::WaveInstrument::parse(const Bytes* bytes) {
	if (bytes->count() != 4)
		return false;

	const Byte b0 = bytes->get(0);
	const Byte b1 = bytes->get(1);
	const Byte b2 = bytes->get(2);
	const Byte b3 = bytes->get(3);

	WaveInstrument val;
	const Byte b31 = b3 & ~0x80;
	GBBASIC_ASSERT(b31 == 0x40 || b31 == 0 && "Wrong data.");
	const bool hasLength = b31 == 0x40;

	if (hasLength)
		val.length = Left<int>(b0);
	else
		val.length = Right<std::nullptr_t>(nullptr);

	val.volume = b1 >> 5;

	val.waveIndex = b2;

	*this = val;

	return true;
}

bool Music::WaveInstrument::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!Jpath::set(doc, val, name, "name"))
		return false;

	if (!Jpath::set(doc, val, length, "length"))
		return false;

	if (!Jpath::set(doc, val, volume, "volume"))
		return false;

	if (!Jpath::set(doc, val, waveIndex, "wave_index"))
		return false;

	return true;
}

bool Music::WaveInstrument::fromJson(const rapidjson::Value &val) {
	if (!Jpath::get(val, name, "name"))
		return false;

	if (!Jpath::get(val, length, "length"))
		return false;

	if (!Jpath::get(val, volume, "volume"))
		return false;

	if (!Jpath::get(val, waveIndex, "wave_index"))
		return false;

	return true;
}

Music::NoiseInstrument::NoiseInstrument() {
}

Music::NoiseInstrument::~NoiseInstrument() {
}

bool Music::NoiseInstrument::operator == (const NoiseInstrument &other) const {
	return compare(other) == 0;
}

bool Music::NoiseInstrument::operator != (const NoiseInstrument &other) const {
	return compare(other) != 0;
}

bool Music::NoiseInstrument::operator < (const NoiseInstrument &other) const {
	return compare(other) < 0;
}

bool Music::NoiseInstrument::operator > (const NoiseInstrument &other) const {
	return compare(other) > 0;
}

int Music::NoiseInstrument::compare(const NoiseInstrument &other) const {
	if (name < other.name)
		return -1;
	else if (name > other.name)
		return 1;

	if (length < other.length)
		return -1;
	else if (length > other.length)
		return 1;

	if (initialVolume < other.initialVolume)
		return -1;
	else if (initialVolume > other.initialVolume)
		return 1;

	if (volumeSweepChange < other.volumeSweepChange)
		return -1;
	else if (volumeSweepChange > other.volumeSweepChange)
		return 1;

	if (shiftClockMask < other.shiftClockMask)
		return -1;
	else if (shiftClockMask > other.shiftClockMask)
		return 1;

	if (dividingRatio < other.dividingRatio)
		return -1;
	else if (dividingRatio > other.dividingRatio)
		return 1;

	if (bitCount < other.bitCount)
		return -1;
	else if (bitCount > other.bitCount)
		return 1;

	if (noiseMacro.size() < other.noiseMacro.size())
		return -1;
	else if (noiseMacro.size() > other.noiseMacro.size())
		return 1;

	for (int i = 0; i < (int)noiseMacro.size(); ++i) {
		const int dl = noiseMacro[i];
		const int dr = other.noiseMacro[i];
		if (dl < dr)
			return -1;
		else if (dl > dr)
			return 1;
	}

	return 0;
}

bool Music::NoiseInstrument::equals(const NoiseInstrument &other) const {
	return compare(other) == 0;
}

bool Music::NoiseInstrument::serialize(Bytes* bytes) const {
	Byte nr42 = (Byte)(
		(initialVolume << 4) |
		(volumeSweepChange > 0 ? 0x08 : 0x00)
	);
	if (volumeSweepChange != 0)
		nr42 |= 8 - std::abs(volumeSweepChange);
	Byte param1 = (length.isLeft() ? 64 - length.left().get() : 0) & 0x3f;
	if (length.isLeft())
		param1 |= 0x40;
	if (bitCount == BitCount::_7)
		param1 |= 0x80;

	if (!bytes->writeUInt8(nr42))
		return false;
	if (!bytes->writeUInt8(param1))
		return false;
	if (noiseMacro.size() < 6)
		return false;
	for (int i = 0; i < 6; ++i) {
		if (!bytes->writeUInt8(noiseMacro[i] & 0xff))
			return false;
	}

	return true;
}

bool Music::NoiseInstrument::parse(const Bytes* bytes) {
	if (bytes->count() != 8)
		return false;

	const Byte b0 = bytes->get(0);
	const Byte b1 = bytes->get(1);

	NoiseInstrument val;
	val.initialVolume = b0 >> 4;
	const Byte b01 = b0 & 0x0f;
	if (b01 & 0x08)
		val.volumeSweepChange = 8 - (b01 & ~0x08);
	else
		val.volumeSweepChange = b01 == 0 ? 0 : -(8 - b01);

	val.bitCount = b1 & 0x80 ? BitCount::_7 : BitCount::_15;
	const Byte b11 = b1 & ~0x80;
	if (b11 & 0x40)
		val.length = Left<int>(64 - (b11 & 0x3f));
	else
		val.length = Right<std::nullptr_t>(nullptr);

	val.noiseMacro.resize(15);
	int k = 0;
	for (int i = 2; i < (int)bytes->count() && k < (int)val.noiseMacro.size(); ++i, ++k)
		val.noiseMacro[k] = bytes->get(i);

	*this = val;

	return true;
}

bool Music::NoiseInstrument::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!Jpath::set(doc, val, name, "name"))
		return false;

	if (!Jpath::set(doc, val, length, "length"))
		return false;

	if (!Jpath::set(doc, val, initialVolume, "initial_volume"))
		return false;

	if (!Jpath::set(doc, val, volumeSweepChange, "volume_sweep_change"))
		return false;

	if (!Jpath::set(doc, val, shiftClockMask, "shift_clock_mask"))
		return false;

	if (!Jpath::set(doc, val, dividingRatio, "dividing_ratio"))
		return false;

	if (!Jpath::set(doc, val, (int)bitCount, "bit_count"))
		return false;

	if (!Jpath::set(doc, val, noiseMacro, "noise_macro"))
		return false;

	return true;
}

bool Music::NoiseInstrument::fromJson(const rapidjson::Value &val) {
	if (!Jpath::get(val, name, "name"))
		return false;

	if (!Jpath::get(val, length, "length"))
		return false;

	if (!Jpath::get(val, initialVolume, "initial_volume"))
		return false;

	if (!Jpath::get(val, volumeSweepChange, "volume_sweep_change"))
		return false;

	if (!Jpath::get(val, shiftClockMask, "shift_clock_mask"))
		return false;

	if (!Jpath::get(val, dividingRatio, "dividing_ratio"))
		return false;

	int bitCount_ = (int)bitCount;
	if (Jpath::get(val, bitCount_, "bit_count"))
		bitCount = (BitCount)bitCount_;
	else
		return false;

	if (!Jpath::get(val, noiseMacro, "noise_macro"))
		return false;
	/*if (bitCount == BitCount::_7)
		noiseMacro.resize(7);
	else
		noiseMacro.resize(15);*/
	noiseMacro.resize(15);

	return true;
}

bool Music::serialize(Bytes* bytes, const Wave &wave) {
	for (int i = 0; i < (int)wave.size(); i += 2) {
		if (!bytes->writeUInt8((wave[i] << 4) | wave[i + 1]))
			return false;
	}

	return true;
}

bool Music::parse(Wave &wave, const Bytes* bytes) {
	Wave val;
	val.fill(0);
	for (int i = 0; i < (int)bytes->count(); ++i) {
		if (i * 2 + 1 >= (int)val.size())
			return false;

		const Byte byte = bytes->get(i);
		val[i * 2] = byte >> 4;
		val[i * 2 + 1] = byte & 0x0f;
	}

	wave = val;

	return true;
}

Music::Song::Song() {
	for (int i = 0; i < (int)waves.size(); ++i) {
		waves[i].fill(0);
	}
}

Music::Song::~Song() {
}

bool Music::Song::operator == (const Song &other) const {
	return compare(other, true) == 0;
}

bool Music::Song::operator != (const Song &other) const {
	return compare(other, true) != 0;
}

size_t Music::Song::hash(bool countName) const {
	size_t result = 0;

	if (countName)
		result = Math::hash(result, name);

	result = Math::hash(result, artist);

	result = Math::hash(result, comment);

	for (int i = 0; i < (int)dutyInstruments.size(); ++i) {
		const DutyInstrument &inst = dutyInstruments[i];
		result = Math::hash(
			result,
			inst.name,
			inst.length.isLeft() ? inst.length.left().get() : 0,
			inst.initialVolume,
			inst.volumeSweepChange,
			inst.frequencySweepTime,
			inst.frequencySweepShift,
			inst.dutyCycle
		);
	}

	for (int i = 0; i < (int)waveInstruments.size(); ++i) {
		const WaveInstrument &inst = waveInstruments[i];
		result = Math::hash(
			result,
			inst.name,
			inst.length.isLeft() ? inst.length.left().get() : 0,
			inst.volume,
			inst.waveIndex
		);
	}

	for (int i = 0; i < (int)noiseInstruments.size(); ++i) {
		const NoiseInstrument &inst = noiseInstruments[i];
		result = Math::hash(
			result,
			inst.name,
			inst.length.isLeft() ? inst.length.left().get() : 0,
			inst.initialVolume,
			inst.volumeSweepChange,
			inst.shiftClockMask,
			inst.dividingRatio,
			inst.bitCount
		);
		for (int j = 0; j < (int)inst.noiseMacro.size(); ++j) {
			const int d = inst.noiseMacro[j];
			result = Math::hash(result, d);
		}
	}

	for (int i = 0; i < (int)waves.size(); ++i) {
		const Wave &wave = waves[i];
		for (int j = 0; j < (int)wave.size(); ++j) {
			const UInt8 d = wave[j];
			result = Math::hash(result, d);
		}
	}

	result = Math::hash(result, ticksPerRow);

	for (int i = 0; i < (int)orders.size(); ++i) {
		const Order &order = orders[i];
		for (int j = 0; j < (int)order.size(); ++j) {
			const Pattern &pattern = order[j];
			for (int k = 0; k < (int)pattern.size(); ++k) {
				const Cell &cell = pattern[k];
				result = Math::hash(
					result,
					cell.note,
					cell.instrument,
					cell.effectCode,
					cell.effectParameters
				);
			}
		}
	}

	for (int i = 0; i < (int)channels.size(); ++i) {
		const Sequence &seq = channels[i];
		for (int j = 0; j < (int)seq.size(); ++j) {
			const int idx = seq[j];
			result = Math::hash(result, idx);
		}
	}

	return result;
}

int Music::Song::compare(const Song &other, bool countName) const {
	if (countName) {
		if (name < other.name)
			return -1;
		else if (name > other.name)
			return 1;
	}

	if (artist < other.artist)
		return -1;
	else if (artist > other.artist)
		return 1;

	if (comment < other.comment)
		return -1;
	else if (comment > other.comment)
		return 1;

	for (int i = 0; i < (int)dutyInstruments.size(); ++i) {
		const DutyInstrument &instl = dutyInstruments[i];
		const DutyInstrument &instr = other.dutyInstruments[i];
		if (instl < instr)
			return -1;
		else if (instl > instr)
			return 1;
	}

	for (int i = 0; i < (int)waveInstruments.size(); ++i) {
		const WaveInstrument &instl = waveInstruments[i];
		const WaveInstrument &instr = other.waveInstruments[i];
		if (instl < instr)
			return -1;
		else if (instl > instr)
			return 1;
	}

	for (int i = 0; i < (int)noiseInstruments.size(); ++i) {
		const NoiseInstrument &instl = noiseInstruments[i];
		const NoiseInstrument &instr = other.noiseInstruments[i];
		if (instl < instr)
			return -1;
		else if (instl > instr)
			return 1;
	}

	for (int i = 0; i < (int)waves.size(); ++i) {
		const Wave &wavel = waves[i];
		const Wave &waver = other.waves[i];
		for (int j = 0; j < (int)wavel.size(); ++j) {
			const UInt8 dl = wavel[j];
			const UInt8 dr = waver[j];
			if (dl < dr)
				return -1;
			else if (dl > dr)
				return 1;
		}
	}

	if (ticksPerRow < other.ticksPerRow)
		return -1;
	else if (ticksPerRow > other.ticksPerRow)
		return 1;

	for (int i = 0; i < (int)orders.size(); ++i) {
		const Order &orderl = orders[i];
		const Order &orderr = other.orders[i];
		for (int j = 0; j < (int)orderl.size(); ++j) {
			const Pattern &patternl = orderl[j];
			const Pattern &patternr = orderr[j];
			for (int k = 0; k < (int)patternl.size(); ++k) {
				const Cell &celll = patternl[k];
				const Cell &cellr = patternr[k];
				if (celll < cellr)
					return -1;
				else if (celll > cellr)
					return 1;
			}
		}
	}

	for (int i = 0; i < (int)channels.size(); ++i) {
		const Sequence &seql = channels[i];
		const Sequence &seqr = other.channels[i];
		if (seql.size() < seqr.size())
			return -1;
		else if (seql.size() > seqr.size())
			return 1;
		for (int j = 0; j < (int)seql.size(); ++j) {
			const int idxl = seql[i];
			const int idxr = seqr[i];
			if (idxl < idxr)
				return -1;
			else if (idxl > idxr)
				return 1;
		}
	}

	return 0;
}

bool Music::Song::equals(const Song &other) const {
	return compare(other, true) == 0;
}

int Music::Song::instrumentCount(void) const {
	const int n0 = (int)dutyInstruments.size();
	const int n1 = (int)waveInstruments.size();
	const int n2 = (int)noiseInstruments.size();

	const int result = n0 + n1 + n2;

	return result;
}

const Music::BaseInstrument* Music::Song::instrumentAt(int idx, Instruments* type) const {
	const int n0 = (int)dutyInstruments.size();
	const int n1 = (int)waveInstruments.size();
	const int n2 = (int)noiseInstruments.size();
	if (idx < 0 || idx >= n0 + n1 + n2)
		return nullptr;

	const BaseInstrument* result = nullptr;
	if (idx < n0) {
		result = &dutyInstruments[idx];
		if (type)
			*type = Instruments::SQUARE;
	} else if (idx < n0 + n1) {
		result = &waveInstruments[idx - n0];
		if (type)
			*type = Instruments::WAVE;
	} else /* if (idx < n0 + n1 + n2) */ {
		result = &noiseInstruments[idx - n0 - n1];
		if (type)
			*type = Instruments::NOISE;
	}

	return result;
}

Music::BaseInstrument* Music::Song::instrumentAt(int idx, Instruments* type) {
	const int n0 = (int)dutyInstruments.size();
	const int n1 = (int)waveInstruments.size();
	const int n2 = (int)noiseInstruments.size();
	if (idx < 0 || idx >= n0 + n1 + n2)
		return nullptr;

	BaseInstrument* result = nullptr;
	if (idx < n0) {
		result = &dutyInstruments[idx];
		if (type)
			*type = Instruments::SQUARE;
	} else if (idx < n0 + n1) {
		result = &waveInstruments[idx - n0];
		if (type)
			*type = Instruments::WAVE;
	} else /* if (idx < n0 + n1 + n2) */ {
		result = &noiseInstruments[idx - n0 - n1];
		if (type)
			*type = Instruments::NOISE;
	}

	return result;
}

Music::Song::OrderMatrix Music::Song::orderMatrix(void) const {
	OrderMatrix result;
	for (int i = 0; i < (int)channels.size(); ++i) {
		const Sequence &seq = channels[i];
		for (int j = 0; j < (int)seq.size(); ++j) {
			const int ord = seq[j];
			result[i].insert(ord);
		}
	}

	return result;
}

bool Music::Song::findStop(int &seq_, int &ln, int &ticks) const {
	seq_ = 0;
	ln = 0;
	ticks = 0;

	const int n = (int)channels.front().size();
	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < GBBASIC_MUSIC_PATTERN_COUNT; ++k) {
			for (int i = 0; i < (int)channels.size(); ++i) {
				const Sequence &seq = channels[i];
				if ((int)seq.size() != n) {
					GBBASIC_ASSERT(false && "Wrong data.");

					return false;
				}
				const int ord = seq[j];
				const Pattern &pattern = orders[i][ord];
				const Cell &cell = pattern[k];
				if (!cell.empty()) {
					seq_ = j;
					ln = k;
					ticks = Math::max(ticksPerRow - 1, 0);
				}
			}
		}
	}

	return true;
}

bool Music::Song::toJson(rapidjson::Value &val, rapidjson::Document &doc) const {
	if (!Jpath::set(doc, val, name, "name"))
		return false;

	if (!Jpath::set(doc, val, artist, "artist"))
		return false;

	if (!Jpath::set(doc, val, comment, "comment"))
		return false;

	for (int i = 0; i < (int)dutyInstruments.size(); ++i) {
		if (!Jpath::set(doc, val, Jpath::ANY(), "duty_instruments", i))
			return false;

		rapidjson::Value* jval = nullptr;
		if (!Jpath::get(val, jval, "duty_instruments", i))
			return false;

		dutyInstruments[i].toJson(*jval, doc);
	}

	for (int i = 0; i < (int)waveInstruments.size(); ++i) {
		if (!Jpath::set(doc, val, Jpath::ANY(), "wave_instruments", i))
			return false;

		rapidjson::Value* jval = nullptr;
		if (!Jpath::get(val, jval, "wave_instruments", i))
			return false;

		waveInstruments[i].toJson(*jval, doc);
	}

	for (int i = 0; i < (int)noiseInstruments.size(); ++i) {
		if (!Jpath::set(doc, val, Jpath::ANY(), "noise_instruments", i))
			return false;

		rapidjson::Value* jval = nullptr;
		if (!Jpath::get(val, jval, "noise_instruments", i))
			return false;

		noiseInstruments[i].toJson(*jval, doc);
	}

	for (int i = 0; i < (int)waves.size(); ++i) {
		const Wave &wave = waves[i];
		for (int j = 0; j < (int)wave.size(); ++j) {
			if (!Jpath::set(doc, val, wave[j], "waves", i, j))
				return false;
		}
	}

	if (!Jpath::set(doc, val, ticksPerRow, "ticks_per_row"))
		return false;

	for (int i = 0; i < (int)orders.size(); ++i) {
		const Order &order = orders[i];
		for (int j = 0; j < (int)order.size(); ++j) {
			const Pattern &pattern = order[j];
			if (empty(pattern)) {
				if (!Jpath::set(doc, val, nullptr, "orders", i, j))
					return false;
			} else {
				for (int k = 0; k < (int)pattern.size(); ++k) {
					const Cell &cel = pattern[k];
					if (!Jpath::set(doc, val, Jpath::ANY(), "orders", i, j, k))
						return false;

					rapidjson::Value* jval = nullptr;
					if (!Jpath::get(val, jval, "orders", i, j, k))
						return false;

					cel.toJson(*jval, doc);
				}
			}
		}
	}

	for (int i = 0; i < (int)channels.size(); ++i) {
		const Sequence &seq = channels[i];
		for (int j = 0; j < (int)seq.size(); ++j) {
			const int index = seq[j];
			if (!Jpath::set(doc, val, index, "channels", i, j))
				return false;
		}
	}

	return true;
}

bool Music::Song::fromJson(const rapidjson::Value &val) {
	if (!Jpath::get(val, name, "name"))
		return false;

	if (!Jpath::get(val, artist, "artist"))
		return false;

	if (!Jpath::get(val, comment, "comment"))
		return false;

	int n = Jpath::count(val, "duty_instruments");
	for (int i = 0; i < n && i < (int)dutyInstruments.size(); ++i) {
		rapidjson::Value* jval = nullptr;
		if (!Jpath::get(val, jval, "duty_instruments", i))
			return false;

		if (jval)
			dutyInstruments[i].fromJson(*jval);
	}

	n = Jpath::count(val, "wave_instruments");
	for (int i = 0; i < n && i < (int)waveInstruments.size(); ++i) {
		rapidjson::Value* jval = nullptr;
		if (!Jpath::get(val, jval, "wave_instruments", i))
			return false;

		if (jval)
			waveInstruments[i].fromJson(*jval);
	}

	n = Jpath::count(val, "noise_instruments");
	for (int i = 0; i < n && i < (int)noiseInstruments.size(); ++i) {
		rapidjson::Value* jval = nullptr;
		if (!Jpath::get(val, jval, "noise_instruments", i))
			return false;

		if (jval)
			noiseInstruments[i].fromJson(*jval);
	}

	for (int i = 0; i < (int)waves.size(); ++i) {
		Wave &wave = waves[i];
		for (int j = 0; j < (int)wave.size(); ++j) {
			int val_ = 0;
			if (!Jpath::get(val, val_, "waves", i, j))
				return false;

			wave[j] = (UInt8)val_;
		}
	}

	if (!Jpath::get(val, ticksPerRow, "ticks_per_row"))
		return false;

	for (int i = 0; i < (int)orders.size(); ++i) {
		Order &order = orders[i];
		n = Jpath::count(val, "orders", i);
		for (int j = 0; j < n && j < GBBASIC_MUSIC_ORDER_COUNT; ++j) {
			Pattern &pattern = order[j];
			rapidjson::Value* jval_ = nullptr;
			if (!Jpath::get(val, jval_, "orders", i, j))
				return false;

			if (jval_->IsNull()) {
				pattern = Pattern();
			} else {
				for (int k = 0; k < (int)pattern.size(); ++k) {
					Cell &cel = pattern[k];
					rapidjson::Value* jval = nullptr;
					if (!Jpath::get(val, jval, "orders", i, j, k))
						return false;

					if (jval)
						cel.fromJson(*jval);
				}
			}
		}
	}

	for (int i = 0; i < (int)channels.size(); ++i) {
		Sequence &seq = channels[i];
		n = Jpath::count(val, "channels", i);
		seq.resize(n);
		for (int j = 0; j < n; ++j) {
			if (!Jpath::get(val, seq[j], "channels", i, j))
				return false;
		}
	}

	return true;
}

bool Music::empty(const Pattern &pattern) {
	for (int i = 0; i < (int)pattern.size(); ++i) {
		if (pattern[i] != Cell::BLANK())
			return false;
	}

	return true;
}

bool Music::toBytes(class Bytes* val, const Orders &orders) {
	rapidjson::Document doc;
	for (int i = 0; i < (int)orders.size(); ++i) {
		const Order &order = orders[i];
		for (int j = 0; j < (int)order.size(); ++j) {
			const Pattern &pattern = order[j];
			if (empty(pattern)) {
				if (!Jpath::set(doc, doc, nullptr, "orders", i, j))
					return false;
			} else {
				for (int k = 0; k < (int)pattern.size(); ++k) {
					const Cell &cel = pattern[k];
					if (!Jpath::set(doc, doc, Jpath::ANY(), "orders", i, j, k))
						return false;

					rapidjson::Value* jval = nullptr;
					if (!Jpath::get(doc, jval, "orders", i, j, k))
						return false;

					cel.toJson(*jval, doc);
				}
			}
		}
	}

	std::string txt;
	if (!Json::toString(doc, txt, false))
		return true;

	val->clear();
	val->writeString(txt);

	return true;
}

bool Music::fromBytes(const Byte* val, size_t size, Orders &orders) {
	std::string txt;
	txt.assign((const char*)val, size);

	rapidjson::Document doc;
	if (!Json::fromString(doc, txt.c_str()))
		return false;

	for (int i = 0; i < (int)orders.size(); ++i) {
		Order &order = orders[i];
		const int n = Jpath::count(doc, "orders", i);
		for (int j = 0; j < n; ++j) {
			Pattern &pattern = order[j];
			rapidjson::Value* jval_ = nullptr;
			if (!Jpath::get(doc, jval_, "orders", i, j))
				return false;

			if (jval_->IsNull()) {
				pattern = Pattern();
			} else {
				for (int k = 0; k < (int)pattern.size(); ++k) {
					Cell &cel = pattern[k];
					rapidjson::Value* jval = nullptr;
					if (!Jpath::get(doc, jval, "orders", i, j, k))
						return false;

					if (jval)
						cel.fromJson(*jval);
				}
			}
		}
	}

	return true;
}

bool Music::fromBytes(const class Bytes* val, Orders &orders) {
	return fromBytes(val->pointer(), val->count(), orders);
}

bool Music::toBytes(class Bytes* val, const Channels &channels) {
	rapidjson::Document doc;
	for (int i = 0; i < (int)channels.size(); ++i) {
		const Sequence &seq = channels[i];
		for (int j = 0; j < (int)seq.size(); ++j) {
			const int index = seq[j];
			if (!Jpath::set(doc, doc, index, "channels", i, j))
				return false;
		}
	}

	std::string txt;
	if (!Json::toString(doc, txt, false))
		return false;

	val->clear();
	val->writeString(txt);

	return true;
}

bool Music::fromBytes(const Byte* val, size_t size, Channels &channels) {
	std::string txt;
	txt.assign((const char*)val, size);

	rapidjson::Document doc;
	if (!Json::fromString(doc, txt.c_str()))
		return false;

	for (int i = 0; i < (int)channels.size(); ++i) {
		Sequence &seq = channels[i];
		const int n = Jpath::count(doc, "channels", i);
		seq.resize(n);
		for (int j = 0; j < n; ++j) {
			if (!Jpath::get(doc, seq[j], "channels", i, j))
				return false;
		}
	}

	return true;
}

bool Music::fromBytes(const class Bytes* val, Channels &channels) {
	return fromBytes(val->pointer(), val->count(), channels);
}

int Music::noteOf(const std::string &name) {
	typedef std::map<std::string, int> Dict;

	const Dict dict = {
		{ "C_3", C_3 }, { "Cs3", Cs3 }, { "D_3", D_3 }, { "Ds3", Ds3 }, { "E_3", E_3 }, { "F_3", F_3 }, { "Fs3", Fs3 }, { "G_3", G_3 }, { "Gs3", Gs3 }, { "A_3", A_3 }, { "As3", As3 }, { "B_3", B_3 },
		{ "C_4", C_4 }, { "Cs4", Cs4 }, { "D_4", D_4 }, { "Ds4", Ds4 }, { "E_4", E_4 }, { "F_4", F_4 }, { "Fs4", Fs4 }, { "G_4", G_4 }, { "Gs4", Gs4 }, { "A_4", A_4 }, { "As4", As4 }, { "B_4", B_4 },
		{ "C_5", C_5 }, { "Cs5", Cs5 }, { "D_5", D_5 }, { "Ds5", Ds5 }, { "E_5", E_5 }, { "F_5", F_5 }, { "Fs5", Fs5 }, { "G_5", G_5 }, { "Gs5", Gs5 }, { "A_5", A_5 }, { "As5", As5 }, { "B_5", B_5 },
		{ "C_6", C_6 }, { "Cs6", Cs6 }, { "D_6", D_6 }, { "Ds6", Ds6 }, { "E_6", E_6 }, { "F_6", F_6 }, { "Fs6", Fs6 }, { "G_6", G_6 }, { "Gs6", Gs6 }, { "A_6", A_6 }, { "As6", As6 }, { "B_6", B_6 },
		{ "C_7", C_7 }, { "Cs7", Cs7 }, { "D_7", D_7 }, { "Ds7", Ds7 }, { "E_7", E_7 }, { "F_7", F_7 }, { "Fs7", Fs7 }, { "G_7", G_7 }, { "Gs7", Gs7 }, { "A_7", A_7 }, { "As7", As7 }, { "B_7", B_7 },
		{ "C_8", C_8 }, { "Cs8", Cs8 }, { "D_8", D_8 }, { "Ds8", Ds8 }, { "E_8", E_8 }, { "F_8", F_8 }, { "Fs8", Fs8 }, { "G_8", G_8 }, { "Gs8", Gs8 }, { "A_8", A_8 }, { "As8", As8 }, { "B_8", B_8 },
		{ "___", ___ }
	};
	Dict::const_iterator it = dict.find(name);
	if (it == dict.end())
		return ___;

	return it->second;
}

std::string Music::nameOf(int note) {
	typedef std::map<int, std::string> Dict;

	const Dict dict = {
		{ C_3, "C_3" }, { Cs3, "Cs3" }, { D_3, "D_3" }, { Ds3, "Ds3" }, { E_3, "E_3" }, { F_3, "F_3" }, { Fs3, "Fs3" }, { G_3, "G_3" }, { Gs3, "Gs3" }, { A_3, "A_3" }, { As3, "As3" }, { B_3, "B_3" },
		{ C_4, "C_4" }, { Cs4, "Cs4" }, { D_4, "D_4" }, { Ds4, "Ds4" }, { E_4, "E_4" }, { F_4, "F_4" }, { Fs4, "Fs4" }, { G_4, "G_4" }, { Gs4, "Gs4" }, { A_4, "A_4" }, { As4, "As4" }, { B_4, "B_4" },
		{ C_5, "C_5" }, { Cs5, "Cs5" }, { D_5, "D_5" }, { Ds5, "Ds5" }, { E_5, "E_5" }, { F_5, "F_5" }, { Fs5, "Fs5" }, { G_5, "G_5" }, { Gs5, "Gs5" }, { A_5, "A_5" }, { As5, "As5" }, { B_5, "B_5" },
		{ C_6, "C_6" }, { Cs6, "Cs6" }, { D_6, "D_6" }, { Ds6, "Ds6" }, { E_6, "E_6" }, { F_6, "F_6" }, { Fs6, "Fs6" }, { G_6, "G_6" }, { Gs6, "Gs6" }, { A_6, "A_6" }, { As6, "As6" }, { B_6, "B_6" },
		{ C_7, "C_7" }, { Cs7, "Cs7" }, { D_7, "D_7" }, { Ds7, "Ds7" }, { E_7, "E_7" }, { F_7, "F_7" }, { Fs7, "Fs7" }, { G_7, "G_7" }, { Gs7, "Gs7" }, { A_7, "A_7" }, { As7, "As7" }, { B_7, "B_7" },
		{ C_8, "C_8" }, { Cs8, "Cs8" }, { D_8, "D_8" }, { Ds8, "Ds8" }, { E_8, "E_8" }, { F_8, "F_8" }, { Fs8, "Fs8" }, { G_8, "G_8" }, { Gs8, "Gs8" }, { A_8, "A_8" }, { As8, "As8" }, { B_8, "B_8" },
		{ ___, "___" }
	};
	Dict::const_iterator it = dict.find(note);
	if (it == dict.end())
		return "___";

	return it->second;
}

Music* Music::create(void) {
	MusicImpl* result = new MusicImpl();

	return result;
}

void Music::destroy(Music* ptr) {
	MusicImpl* impl = static_cast<MusicImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
