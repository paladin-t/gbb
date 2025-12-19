/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#include "compiler.h"
#include "pipeline.h"
#include "../utils/datetime.h"
#include "../utils/platform.h"
#include "../utils/text.h"
#include "../utils/work_queue.h"

/*
** {===========================================================================
** Utilities
*/

namespace GBBASIC {

/**< Asset organizers. */

template<typename T> struct Ordered {
	typedef std::vector<Ordered> Array;
	typedef std::vector<Array> Pages;

	typedef T ValueType;

	Math::Vec2i position;
	ValueType value;

	Ordered() {
	}
	Ordered(const Math::Vec2i &pos, const ValueType &val) : position(pos), value(val) {
	}
};

/**< Allocation. */

struct Source {
	AssetsBundle::Categories category = AssetsBundle::Categories::NONE;
	int page = -1;

	Source() {
	}
	Source(AssetsBundle::Categories cat, int pg) : category(cat), page(pg) {
	}

	bool operator < (const Source &other) const {
		return compare(other) < 0;
	}

	int compare(const Source &other) const {
		// Ordered by category firstly.
		if (category < other.category)
			return -1;
		else if (category > other.category)
			return 1;

		// Ordered by page secondly.
		if (page < other.page)
			return -1;
		else if (page > other.page)
			return 1;

		return 0;
	}
};

struct Destination {
	struct Chunk {
		typedef std::vector<Chunk> Array;

		struct Scheduled {
			typedef std::vector<Scheduled> Array;

			int argsOffset = 0;
			int index = 0;

			Scheduled() {
			}

			Byte* args(const Byte* begin) const {
				return (Byte*)((intptr_t)begin + argsOffset);
			}
		};

		int beginOffset = -1;
		int endOffset = -1;
		int bank = 0;
		int address = 0;
		Bytes::Ptr bytes = nullptr;
		Scheduled::Array scheduled;
		int layer = -1;

		Chunk() {
		}
		Chunk(Bytes::Ptr bytes_) : bytes(bytes_) {
		}
		Chunk(Bytes::Ptr bytes_, int layer_) : bytes(bytes_), layer(layer_) {
		}
		Chunk(Bytes::Ptr bytes_, const Scheduled::Array &scheduled_) :
			bytes(bytes_), scheduled(scheduled_)
		{
		}
		Chunk(Bytes::Ptr bytes_, const Scheduled::Array &scheduled_, int layer_) :
			bytes(bytes_), scheduled(scheduled_), layer(layer_)
		{
		}
	};

	Chunk::Array chunks;
	int refCount = 0;

	Destination() {
	}
	Destination(const Chunk::Array &lyrs) : chunks(lyrs) {
	}
};

typedef std::map<Source, Destination> Table;

struct FreeSpace {
	struct Cluster {
		int bank = 0;
		int addressCursor = 0;
		int size = 0;

		Cluster() {
		}
		Cluster(int b, int a, int s) : bank(b), addressCursor(a), size(s) {
		}
	};
	typedef std::vector<Cluster> Array;

	Array entries;

	FreeSpace() {
	}

	void add(const Cluster &fs) {
		entries.push_back(fs);
	}
	void clear(void) {
		entries.clear();
	}
	int find(int size) const {
		for (int i = 0; i < (int)entries.size(); ++i) {
			const Cluster &cluster = entries[i];
			if (cluster.size >= size)
				return i;
		}

		return -1;
	}
	bool allocate(int clusterIndex, int size, int* bank, int* addressCursor) {
		*bank = 0;
		*addressCursor = 0;
		Cluster &cluster = entries[clusterIndex];
		if (cluster.size >= size) {
			*bank = cluster.bank;
			*addressCursor = cluster.addressCursor;
			cluster.addressCursor += size;
			cluster.size -= size;
			if (cluster.size == 0)
				entries.erase(entries.begin() + clusterIndex);

			return true;
		}

		return false;
	}
	bool allocate(int size, int* bank, int* addressCursor) {
		*bank = 0;
		*addressCursor = 0;
		for (int i = 0; i < (int)entries.size(); ++i) {
			Cluster &cluster = entries[i];
			if (cluster.size >= size) {
				*bank = cluster.bank;
				*addressCursor = cluster.addressCursor;
				cluster.addressCursor += size;
				cluster.size -= size;
				if (cluster.size == 0)
					entries.erase(entries.begin() + i);

				return true;
			}
		}

		return false;
	}
};

/**< Endians. */

enum class Endians {
	LITTLE,
	BIG
};

static bool isDifferentEndian(Endians endian) {
	return
		(!Platform::isLittleEndian() && endian == Endians::LITTLE) ||
		(Platform::isLittleEndian() && endian == Endians::BIG);
}

/**< Writers. */

static void write(Byte* ptr, UInt8 val) {
	*((Byte*)ptr++) = val;
}

static void write(Byte* ptr, UInt16 val, Endians endian) {
	union {
		UInt16 data;
		UInt8 bytes[2];
	} u;
	u.data = val;
	if (isDifferentEndian(endian))
		std::swap(u.bytes[0], u.bytes[1]);
	*((Byte*)ptr++) = u.bytes[0];
	*((Byte*)ptr++) = u.bytes[1];
}

static void write(Bytes* bytes, UInt16 val, Endians endian) {
	union {
		UInt16 data;
		UInt8 bytes[2];
	} u;
	u.data = val;
	if (isDifferentEndian(endian))
		std::swap(u.bytes[0], u.bytes[1]);
	bytes->writeUInt8(u.bytes[0]);
	bytes->writeUInt8(u.bytes[1]);
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Serializers
*/

namespace GBBASIC {

static bool generate_toBytes(const TilesAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, bool includeUnused) {
	// Prepare.
	const Image::Ptr &data = entry->data;
	if (!data)
		return false;

	// Check whether the asset is in use.
	bool inuse = true;
	const Source src(AssetsBundle::Categories::TILES, page);
	Table::iterator it = table.find(src);
	if (it != table.end()) {
		Destination &dst = it->second;
		if (!includeUnused && dst.refCount == 0) // Not referenced.
			inuse = false;
	}

	// Generate the bytes.
	Bytes::Ptr bytes(Bytes::create());
	if (inuse) {
		data->serializeTiles(
			GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
			nullptr,
			nullptr,
			[&] (int /* y */, int /* lineno */, int /* tx */, int /* ty */, UInt8 ln0, UInt8 ln1) -> void {
				bytes->writeUInt8(ln0); // The low bits of a line.
				bytes->writeUInt8(ln1); // The high bits of a line.
			}
		);
	}

	// Fill in with the destination data.
	const Destination::Chunk chunk(bytes);
	if (it == table.end()) {
		const Destination::Chunk::Array dst({ chunk });
		const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
		if (!ret.second)
			return false;

		it = ret.first;
	} else {
		it->second.chunks.push_back(chunk);
	}

	// Ignore unused.
	if (!includeUnused && it->second.refCount == 0)
		return true;

	// Finish.
	pipeline->effectiveSize().addTiles((int)bytes->count());

	return true;
}
static bool post_toBytes(const TilesAssets::Entry*, Pipeline*, Table &, int) {
	// Do nothing.

	return true;
}

static bool generate_toBytes(const MapAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, bool includeUnused) {
	auto serializePlaneLayer = [] (
		const Map::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused, int layer
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MAP, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		if (inuse) {
			const int w = data->width();
			const int h = data->height();
			for (int j = 0; j < h; ++j) {
				for (int i = 0; i < w; ++i) {
					const UInt8 cel = (UInt8)data->get(i, j);
					bytes->writeUInt8(cel); // The cel.
				}
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, layer);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMap((int)bytes->count());

		return true;
	};

	if (!serializePlaneLayer(entry->data, pipeline, table, page, includeUnused, ASSETS_MAP_GRAPHICS_LAYER))
		return false;
	if (entry->hasAttributes) {
		if (!serializePlaneLayer(entry->attributes, pipeline, table, page, includeUnused, ASSETS_MAP_ATTRIBUTES_LAYER))
			return false;
	}

	return true;
}
static bool post_toBytes(const MapAssets::Entry*, Pipeline*, Table &, int) {
	// Do nothing.

	return true;
}

static bool generate_toBytes(const MusicAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, bool includeUnused) {
	typedef std::map<Math::Vec2i, int> OrderIndices;
	typedef std::vector<Music::Pattern> FilledPatterns;

	OrderIndices orderIndices;
	FilledPatterns filledPatterns;

	auto serializeOrderCount = [] (
		const Music::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		const Music::Song* song = data->pointer();
		const Music::Channels &channels = song->channels;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			const int offset = (int)bytes->peek();
			Destination::Chunk::Scheduled s_;
			s_.index = offset;
			scheduled.push_back(s_);

			bytes->writeUInt8((UInt8)(channels.front().size() * 2)); // Order count.
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMusic((int)bytes->count());

		return true;
	};
	auto serializePatterns = [&orderIndices, &filledPatterns] (
		const Music::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		const Music::Song* song = data->pointer();
		const Music::Orders &orders = song->orders;
		const Music::Song::OrderMatrix ordMat = song->orderMatrix();

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			for (int i = 0; i < (int)ordMat.size(); ++i) {
				const Music::Order &order = orders[i];
				const Music::Song::OrderSet &ordSet = ordMat[i];
				for (int ord : ordSet) {
					const int offset = (int)bytes->peek();
					Destination::Chunk::Scheduled s_;
					s_.index = offset;
					scheduled.push_back(s_);

					const Music::Pattern &pattern = order[ord];
					FilledPatterns::const_iterator pit = std::find(filledPatterns.begin(), filledPatterns.end(), pattern); // Ignore for duplicate pattern.
					if (pit == filledPatterns.end()) {
						for (int j = 0; j < (int)pattern.size(); ++j) {
							Music::Cell cell = pattern[j];
							switch (cell.effectCode) {
							case 0x06:
								// The "Call routine" effect is unused.
								cell.effectCode = 0x00;

								break;
							case 0x0b:
								// Clamp the range of position jump.
								cell.effectParameters = Math::clamp(cell.effectParameters, 1, (int)ordSet.size());

								break;
							}
							cell.serialize(bytes.get());
						}
						orderIndices[Math::Vec2i(i, ord)] = (int)filledPatterns.size();
						filledPatterns.push_back(pattern);
					} else {
						const int idx = (int)(pit - filledPatterns.begin());
						orderIndices[Math::Vec2i(i, ord)] = idx;
					}
				}
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMusic((int)bytes->count());

		return true;
	};
	auto serializeOrders = [&orderIndices] (
		const Music::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		const Music::Song* song = data->pointer();
		const Music::Song::OrderMatrix ordMat = song->orderMatrix();

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			for (int i = 0; i < (int)song->channels.size(); ++i) {
				const Music::Sequence &seq = song->channels[i];
				for (int j = 0; j < (int)seq.size(); ++j) {
					const int ord = seq[j];

					const int offset = (int)bytes->peek();
					Destination::Chunk::Scheduled s_;
					s_.index = offset;
					scheduled.push_back(s_);

					OrderIndices::const_iterator it = orderIndices.find(Math::Vec2i(i, ord));
					if (it == orderIndices.end()) {
						GBBASIC_ASSERT(false && "Impossible.");

						return false;
					}
					const int idx = it->second;
					bytes->writeUInt16((UInt16)idx); // To be determined during posting.
				}
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMusic((int)bytes->count());

		return true;
	};
	auto serializeInstruments = [] (
		const Music::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		const Music::Song* song = data->pointer();
		const Music::DutyInstrument::Array &dutyInsts = song->dutyInstruments;
		const Music::WaveInstrument::Array &waveInsts = song->waveInstruments;
		const Music::NoiseInstrument::Array &noiseInsts = song->noiseInstruments;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			{
				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.index = offset;
				scheduled.push_back(s_);

				for (int i = 0; i < (int)dutyInsts.size(); ++i) {
					const Music::DutyInstrument &inst = dutyInsts[i];
					inst.serialize(bytes.get());
				}
			}

			{
				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.index = offset;
				scheduled.push_back(s_);

				for (int i = 0; i < (int)waveInsts.size(); ++i) {
					const Music::WaveInstrument &inst = waveInsts[i];
					inst.serialize(bytes.get());
				}
			}

			{
				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.index = offset;
				scheduled.push_back(s_);

				for (int i = 0; i < (int)noiseInsts.size(); ++i) {
					const Music::NoiseInstrument &inst = noiseInsts[i];
					inst.serialize(bytes.get());
				}
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMusic((int)bytes->count());

		return true;
	};
	auto serializeWaves = [] (
		const Music::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		const Music::Song* song = data->pointer();
		const Music::Waves &waves = song->waves;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			const int offset = (int)bytes->peek();
			Destination::Chunk::Scheduled s_;
			s_.index = offset;
			scheduled.push_back(s_);

			for (int i = 0; i < (int)waves.size(); ++i) {
				const Music::Wave &wave = waves[i];
				Music::serialize(bytes.get(), wave);
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMusic((int)bytes->count());

		return true;
	};
	auto serializeSong = [] (
		const Music::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		const Music::Song* song = data->pointer();

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			const int offset = (int)bytes->peek();
			Destination::Chunk::Scheduled s_;
			s_.argsOffset = offset;
			scheduled.push_back(s_);

			bytes->writeUInt8((UInt8)song->ticksPerRow);      // Ticks per row (Tempo).
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Order count.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Order 1.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Order 2.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Order 3.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Order 4.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Duty instruments.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Wave instruments.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Noise instruments.
			bytes->writeUInt16((UInt16)NULL);                 // Routines.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // Waves.
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addMusic((int)bytes->count());

		return true;
	};

	if (!serializeOrderCount(entry->data, pipeline, table, page, includeUnused))
		return false;
	if (!serializePatterns(entry->data, pipeline, table, page, includeUnused))
		return false;
	if (!serializeOrders(entry->data, pipeline, table, page, includeUnused))
		return false;
	if (!serializeInstruments(entry->data, pipeline, table, page, includeUnused))
		return false;
	if (!serializeWaves(entry->data, pipeline, table, page, includeUnused))
		return false;
	if (!serializeSong(entry->data, pipeline, table, page, includeUnused))
		return false;

	return true;
}
static bool post_toBytes(const MusicAssets::Entry* entry, Pipeline* pipeline, Table &table, int page) {
	auto serializeOrders = [] (
		const Music::Ptr &data,
		Pipeline*, Table &table, int page
	) -> bool {
		if (!data)
			return false;

		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it == table.end())
			return true;
		if (it->second.chunks.size() != 6)
			return true;

		Destination::Chunk &chunkPatterns = it->second.chunks[1];
		const Destination::Chunk::Scheduled::Array &scheduledPatterns = chunkPatterns.scheduled;
		const int address1 = chunkPatterns.address;

		Destination::Chunk &chunk = it->second.chunks[2];
		Bytes::Ptr &bytes = chunk.bytes;
		const Destination::Chunk::Scheduled::Array &scheduled = chunk.scheduled;
		for (int i = 0; i < (int)scheduled.size(); ++i) {
			const Destination::Chunk::Scheduled &s = scheduled[i];
			const Byte* ptr = bytes->pointer() + s.index;
			const UInt16 num = *(UInt16*)ptr;
			const Destination::Chunk::Scheduled &sPatterns = scheduledPatterns[num];
			const UInt16 addr = (UInt16)(address1 + sPatterns.index);
			write(s.args(ptr), addr, Endians::LITTLE); ptr += sizeof(UInt16); // The pattern address.
		}

		return true;
	};
	auto serializeSong = [] (const Music::Ptr &data,
		Pipeline*, Table &table, int page
	) -> bool {
		if (!data)
			return false;

		const Source src(AssetsBundle::Categories::MUSIC, page);
		Table::iterator it = table.find(src);
		if (it == table.end())
			return true;
		if (it->second.chunks.size() != 6)
			return true;

		Destination::Chunk &chunkOrderCount = it->second.chunks[0];
		const Destination::Chunk::Scheduled::Array &scheduledOrderCount = chunkOrderCount.scheduled;
		const int address1 = chunkOrderCount.address;
		Destination::Chunk &chunkOrders = it->second.chunks[2];
		const Destination::Chunk::Scheduled::Array &scheduledOrders = chunkOrders.scheduled;
		const int address2 = chunkOrders.address;
		Destination::Chunk &chunkInstruments = it->second.chunks[3];
		const Destination::Chunk::Scheduled::Array &scheduledInstruments = chunkInstruments.scheduled;
		const int address3 = chunkInstruments.address;
		Destination::Chunk &chunkWaves = it->second.chunks[4];
		const Destination::Chunk::Scheduled::Array &scheduledWaves = chunkWaves.scheduled;
		const int address4 = chunkWaves.address;

		Destination::Chunk &chunk = it->second.chunks[5];
		Bytes::Ptr &bytes = chunk.bytes;
		const int address = chunk.address;
		(void)address;
		const Destination::Chunk::Scheduled::Array &scheduled = chunk.scheduled;
		GBBASIC_ASSERT(scheduled.size() <= 1 && "Wrong data.");
		if (!scheduled.empty()) {
			const int lines = (int)scheduledOrders.size() / GBBASIC_MUSIC_CHANNEL_COUNT;

			const Destination::Chunk::Scheduled &s = scheduled.front();
			const UInt16 addr1   = (UInt16)(address1 + scheduledOrderCount.front().index);
			const UInt16 addr2_1 = (UInt16)(address2 + scheduledOrders[0 * lines].index);
			const UInt16 addr2_2 = (UInt16)(address2 + scheduledOrders[1 * lines].index);
			const UInt16 addr2_3 = (UInt16)(address2 + scheduledOrders[2 * lines].index);
			const UInt16 addr2_4 = (UInt16)(address2 + scheduledOrders[3 * lines].index);
			const UInt16 addr3_1 = (UInt16)(address3 + scheduledInstruments[0].index);
			const UInt16 addr3_2 = (UInt16)(address3 + scheduledInstruments[1].index);
			const UInt16 addr3_3 = (UInt16)(address3 + scheduledInstruments[2].index);
			const UInt16 addr4   = (UInt16)(address4 + scheduledWaves.front().index);
			const Byte* ptr = bytes->pointer();
			ptr += sizeof(UInt8);                                                // The ticks per row.
			write(s.args(ptr), addr1,   Endians::LITTLE); ptr += sizeof(UInt16); // The order count address.
			write(s.args(ptr), addr2_1, Endians::LITTLE); ptr += sizeof(UInt16); // The order 1 address.
			write(s.args(ptr), addr2_2, Endians::LITTLE); ptr += sizeof(UInt16); // The order 2 address.
			write(s.args(ptr), addr2_3, Endians::LITTLE); ptr += sizeof(UInt16); // The order 3 address.
			write(s.args(ptr), addr2_4, Endians::LITTLE); ptr += sizeof(UInt16); // The order 4 address.
			write(s.args(ptr), addr3_1, Endians::LITTLE); ptr += sizeof(UInt16); // The duty instruments address.
			write(s.args(ptr), addr3_2, Endians::LITTLE); ptr += sizeof(UInt16); // The wave instruments address.
			write(s.args(ptr), addr3_3, Endians::LITTLE); ptr += sizeof(UInt16); // The noise instruments address.
			ptr += sizeof(UInt16);                                               // The routines.
			write(s.args(ptr), addr4,   Endians::LITTLE); ptr += sizeof(UInt16); // The waves address.
		}

		return true;
	};

	if (!serializeOrders(entry->data, pipeline, table, page))
		return false;
	if (!serializeSong(entry->data, pipeline, table, page))
		return false;

	return true;
}

static bool generate_toBytes(const SfxAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, bool includeUnused) {
	// Prepare.
	const Sfx::Ptr &data = entry->data;
	if (!data)
		return false;

	// Check whether the asset is in use.
	bool inuse = true;
	const Source src(AssetsBundle::Categories::SFX, page);
	Table::iterator it = table.find(src);
	if (it != table.end()) {
		Destination &dst = it->second;
		if (!includeUnused && dst.refCount == 0) // Not referenced.
			inuse = false;
	}

	// Generate the bytes.
	Bytes::Ptr bytes(Bytes::create());
	if (inuse) {
		const Sfx::Sound* sound = data->pointer();
		bytes->writeUInt8((UInt8)sound->mask);
		for (int j = 0; j < (int)sound->rows.size(); ++j) {
			const Sfx::Row &row = sound->rows[j];
			const UInt8 dm = ((row.delay & 0x0f) << 4) | ((UInt8)row.cells.size() & 0x0f);
			bytes->writeUInt8((UInt8)dm);
			for (int i = 0; i < (int)row.cells.size(); ++i) {
				const Sfx::Cell &cell = row.cells[i];
				bytes->writeUInt8((UInt8)cell.command);
				for (int k = 0; k < (int)cell.registers.size(); ++k)
					bytes->writeUInt8((UInt8)cell.registers[k]);
			}
		}
	}

	// Fill in with the destination data.
	const Destination::Chunk chunk(bytes);
	if (it == table.end()) {
		const Destination::Chunk::Array dst({ chunk });
		const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
		if (!ret.second)
			return false;

		it = ret.first;
	} else {
		it->second.chunks.push_back(chunk);
	}

	// Ignore unused.
	if (!includeUnused && it->second.refCount == 0)
		return true;

	// Finish.
	pipeline->effectiveSize().addSfx((int)bytes->count());

	return true;
}
static bool post_toBytes(const SfxAssets::Entry*, Pipeline*, Table &, int) {
	// Do nothing.

	return true;
}

static bool generate_toBytes(const ActorAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, bool includeUnused) {
	auto serializeTiles = [entry] (
		const Actor::Ptr &data,
		const Actor::Slice::Array &slices,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::ACTOR, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		bool inuseAsProjectile = true;
		const Source srcAsProjectile(AssetsBundle::Categories::PROJECTILE, page);
		Table::iterator itAsProjectile = table.find(srcAsProjectile);
		if (itAsProjectile != table.end()) {
			Destination &dst = itAsProjectile->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuseAsProjectile = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		if (inuse || inuseAsProjectile) {
			for (int k = 0; k < (int)slices.size(); ++k) {
				const Actor::Slice &slice = slices[k];
				const Image::Ptr &img = slice.image;
				img->serializeTiles(
					GBBASIC_TILE_SIZE, GBBASIC_TILE_SIZE,
					nullptr,
					nullptr,
					[&] (int /* y */, int /* lineno */, int /* tx */, int /* ty */, UInt8 ln0, UInt8 ln1) -> void {
						bytes->writeUInt8(ln0); // The low bits of a line.
						bytes->writeUInt8(ln1); // The high bits of a line.
					}
				);
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes);
		if (entry->asActor) {
			if (it == table.end()) {
				const Destination::Chunk::Array dst({ chunk });
				const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
				if (!ret.second)
					return false;

				it = ret.first;
			} else {
				it->second.chunks.push_back(chunk);
			}
		} else /* if (!entry->asActor) */ {
			if (itAsProjectile == table.end()) {
				const Destination::Chunk::Array dst({ chunk });
				const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(srcAsProjectile, dst));
				if (!ret.second)
					return false;

				itAsProjectile = ret.first;
			} else {
				itAsProjectile->second.chunks.push_back(chunk);
			}
		}

		// Ignore unused.
		const int refCount = (it != table.end() ? it->second.refCount : 0) + (itAsProjectile != table.end() ? itAsProjectile->second.refCount : 0);
		if (!includeUnused && refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addActor((int)bytes->count());

		return true;
	};
	auto serializeFrames = [entry] (
		const Actor::Ptr &data,
		const Actor::Animation &animation, const Actor::Shadow::Array &shadow,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		constexpr const Int8 END_OF_FRAME = -128;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::ACTOR, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		bool inuseAsProjectile = true;
		const Source srcAsProjectile(AssetsBundle::Categories::PROJECTILE, page);
		Table::iterator itAsProjectile = table.find(srcAsProjectile);
		if (itAsProjectile != table.end()) {
			Destination &dst = itAsProjectile->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuseAsProjectile = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse || inuseAsProjectile) {
			for (int i = 0; i < (int)animation.size(); ++i) {
				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.argsOffset = offset;
				scheduled.push_back(s_);
				bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The frame address.
			}
			bytes->writeUInt16((UInt16)NULL);                     // The end of this animation.
			std::map<int, int> filled_;
			for (int i = 0; i < (int)animation.size(); ++i) {
				Destination::Chunk::Scheduled &s_ = scheduled[i];
				const int a = animation[i];
				std::map<int, int>::const_iterator it = filled_.find(a);
				if (it == filled_.end()) {
					const int offset = (int)bytes->peek();
					s_.index = offset;

					filled_.insert(std::make_pair(a, offset));
				} else {
					const int offset = it->second;
					s_.index = offset;

					continue; // Ignore if the frame has already been filled.
				}

				const Actor::Shadow &s = shadow[a];
				for (int i = 0; i < (int)s.refs.size(); ++i) {
					const int t = s.refs[i].index * (data->is8x16() ? 2 : 1);
					const Math::Vec2i &step = s.refs[i].step;
					const int p = s.refs[i].properties;
					bytes->writeInt8((Int8)step.y);               // The Y step.
					bytes->writeInt8((Int8)step.x);               // The X step.
					bytes->writeUInt8((UInt8)t);                  // The tile index.
					bytes->writeUInt8((UInt8)p);                  // The properties.
				}
				bytes->writeInt8((Int8)END_OF_FRAME);             // The end of this frame.
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (entry->asActor) {
			if (it == table.end()) {
				const Destination::Chunk::Array dst({ chunk });
				const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
				if (!ret.second)
					return false;

				it = ret.first;
			} else {
				it->second.chunks.push_back(chunk);
			}
		} else /* if (!entry->asActor) */ {
			if (itAsProjectile == table.end()) {
				const Destination::Chunk::Array dst({ chunk });
				const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(srcAsProjectile, dst));
				if (!ret.second)
					return false;

				itAsProjectile = ret.first;
			} else {
				itAsProjectile->second.chunks.push_back(chunk);
			}
		}

		// Ignore unused.
		const int refCount = (it != table.end() ? it->second.refCount : 0) + (itAsProjectile != table.end() ? itAsProjectile->second.refCount : 0);
		if (!includeUnused && refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addActor((int)bytes->count());

		return true;
	};
	auto serializeDefinitionAsActor = [] (
		const Actor::Ptr &data,
		const Actor::Animation &animation, const active_t &def,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::ACTOR, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			bytes->writeUInt8((UInt8)def.enabled);
			bytes->writeUInt8((UInt8)def.hidden);
			bytes->writeUInt8((UInt8)def.pinned);
			bytes->writeUInt8((UInt8)def.persistent);
			bytes->writeUInt8((UInt8)def.following);
			bytes->writeUInt8((UInt8)def.direction);
			bytes->writeUInt8((Int8)def.bounds.left);
			bytes->writeUInt8((Int8)def.bounds.right);
			bytes->writeUInt8((Int8)def.bounds.top);
			bytes->writeUInt8((Int8)def.bounds.bottom);
			{
				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.argsOffset = offset;
				scheduled.push_back(s_);
				bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The sprite bank.
				bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The sprite address.
			}
			bytes->writeUInt8((UInt8)def.animation_interval);
			for (int i = 0; i < ASSETS_ACTOR_MAX_ANIMATIONS; ++i) {
				const int begin = Math::clamp((int)def.animations[i].begin, 0, (int)animation.size() - 1);
				const int end = Math::clamp((int)def.animations[i].end, 0, (int)animation.size() - 1);
				bytes->writeUInt8((UInt8)begin);
				bytes->writeUInt8((UInt8)end);
			}
			bytes->writeUInt8((UInt8)def.move_speed);
			bytes->writeUInt8((UInt8)def.behaviour);
			bytes->writeUInt8((UInt8)def.collision_group);
			// No routine is written here.
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addActor((int)bytes->count());

		return true;
	};
	auto serializeDefinitionAsProjectile = [] (
		const Actor::Ptr &data,
		const Actor::Animation &animation, const active_t &def,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuseAsProjectile = true;
		const Source srcAsProjectile(AssetsBundle::Categories::PROJECTILE, page);
		Table::iterator itAsProjectile = table.find(srcAsProjectile);
		if (itAsProjectile != table.end()) {
			Destination &dst = itAsProjectile->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuseAsProjectile = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuseAsProjectile) {
			bytes->writeUInt8((Int8)def.bounds.left);
			bytes->writeUInt8((Int8)def.bounds.right);
			bytes->writeUInt8((Int8)def.bounds.top);
			bytes->writeUInt8((Int8)def.bounds.bottom);
			{
				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.argsOffset = offset;
				scheduled.push_back(s_);
				bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The sprite bank.
				bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The sprite address.
			}
			bytes->writeUInt8((UInt8)def.animation_interval);
			for (int i = 0; i < ASSETS_PROJECTILE_MAX_ANIMATIONS; ++i) {
				const int begin = Math::clamp((int)def.animations[i].begin, 0, (int)animation.size() - 1);
				const int end = Math::clamp((int)def.animations[i].end, 0, (int)animation.size() - 1);
				bytes->writeUInt8((UInt8)begin);
				bytes->writeUInt8((UInt8)end);
			}
			bytes->writeUInt8((UInt8)def.life_time);
			bytes->writeUInt8((UInt8)def.move_speed);
			bytes->writeUInt16((UInt16)def.initial_offset);
			bytes->writeUInt8((UInt8)def.collision_group);
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (itAsProjectile == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(srcAsProjectile, dst));
			if (!ret.second)
				return false;

			itAsProjectile = ret.first;
		} else {
			itAsProjectile->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && itAsProjectile->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addActor((int)bytes->count());

		return true;
	};

	if (!serializeTiles(entry->data, entry->slices, pipeline, table, page, includeUnused))
		return false;
	if (!serializeFrames(entry->data, entry->animation, entry->shadow, pipeline, table, page, includeUnused))
		return false;
	if (entry->asActor) {
		if (!serializeDefinitionAsActor(entry->data, entry->animation, entry->definition, pipeline, table, page, includeUnused))
			return false;
	} else /* if (!entry->asActor) */ {
		if (!serializeDefinitionAsProjectile(entry->data, entry->animation, entry->definition, pipeline, table, page, includeUnused))
			return false;
	}

	return true;
}
static bool post_toBytes(const ActorAssets::Entry* entry, Pipeline* pipeline, Table &table, int page) {
	auto serializeFrames = [] (
		const Actor::Ptr &data,
		Pipeline*, Table &table, int page
	) -> bool {
		if (!data)
			return false;

		const Source src(AssetsBundle::Categories::ACTOR, page);
		Table::iterator it = table.find(src);

		const Source srcAsProjectile(AssetsBundle::Categories::PROJECTILE, page);
		Table::iterator itAsProjectile = table.find(srcAsProjectile);

		if (it == table.end() && itAsProjectile == table.end())
			return true;

		Destination::Chunk* chunkptr = nullptr;
		if (it != table.end()) {
			if (it->second.chunks.size() != 3)
				return true;

			chunkptr = &it->second.chunks[1];
		} else /* if (itAsProjectile != table.end()) */ {
			if (itAsProjectile->second.chunks.size() != 3)
				return true;

			chunkptr = &itAsProjectile->second.chunks[1];
		}

		Destination::Chunk &chunk = *chunkptr;
		Bytes::Ptr &bytes = chunk.bytes;
		const int address = chunk.address;
		const Destination::Chunk::Scheduled::Array &scheduled = chunk.scheduled;
		for (int i = 0; i < (int)scheduled.size(); ++i) {
			const Destination::Chunk::Scheduled &s = scheduled[i];
			const UInt16 addr = (UInt16)(address + s.index);
			const Byte* ptr = bytes->pointer();
			write(s.args(ptr), addr, Endians::LITTLE); ptr += sizeof(UInt16); // The frame address.
		}

		return true;
	};
	auto serializeDefinitionAsActor = [] (
		const Actor::Ptr &data,
		Pipeline*, Table &table, int page
	) -> bool {
		if (!data)
			return false;

		const Source src(AssetsBundle::Categories::ACTOR, page);
		Table::iterator it = table.find(src);
		if (it == table.end())
			return true;
		if (it->second.chunks.size() != 3)
			return true;

		const Destination::Chunk &chunk1 = it->second.chunks[1];
		const int bank1 = chunk1.bank;
		const int address1 = chunk1.address;

		Destination::Chunk &chunk = it->second.chunks[2];
		Bytes::Ptr &bytes = chunk.bytes;
		const Destination::Chunk::Scheduled::Array &scheduled = chunk.scheduled;
		GBBASIC_ASSERT(scheduled.size() <= 1 && "Wrong data.");
		if (!scheduled.empty()) {
			const Destination::Chunk::Scheduled &s = scheduled.front();
			const Byte* ptr = bytes->pointer();
			write(s.args(ptr), (UInt8)bank1);                      ptr += sizeof(UInt8);  // The sprite bank.
			write(s.args(ptr), (UInt16)address1, Endians::LITTLE); ptr += sizeof(UInt16); // The sprite address.
		}

		return true;
	};
	auto serializeDefinitionAsProjectile = [] (
		const Actor::Ptr &data,
		Pipeline*, Table &table, int page
	) -> bool {
		if (!data)
			return false;

		const Source srcAsProjectile(AssetsBundle::Categories::PROJECTILE, page);
		Table::iterator itAsProjectile = table.find(srcAsProjectile);
		if (itAsProjectile == table.end())
			return true;
		if (itAsProjectile->second.chunks.size() != 3)
			return true;

		const Destination::Chunk &chunk1 = itAsProjectile->second.chunks[1];
		const int bank1 = chunk1.bank;
		const int address1 = chunk1.address;

		Destination::Chunk &chunk = itAsProjectile->second.chunks[2];
		Bytes::Ptr &bytes = chunk.bytes;
		const Destination::Chunk::Scheduled::Array &scheduled = chunk.scheduled;
		GBBASIC_ASSERT(scheduled.size() <= 1 && "Wrong data.");
		if (!scheduled.empty()) {
			const Destination::Chunk::Scheduled &s = scheduled.front();
			const Byte* ptr = bytes->pointer();
			write(s.args(ptr), (UInt8)bank1);                      ptr += sizeof(UInt8);  // The sprite bank.
			write(s.args(ptr), (UInt16)address1, Endians::LITTLE); ptr += sizeof(UInt16); // The sprite address.
		}

		return true;
	};

	if (!serializeFrames(entry->data, pipeline, table, page))
		return false;
	if (entry->asActor) {
		if (!serializeDefinitionAsActor(entry->data, pipeline, table, page))
			return false;
	} else /* if (!entry->asActor) */ {
		if (!serializeDefinitionAsProjectile(entry->data, pipeline, table, page))
			return false;
	}

	return true;
}

static bool generate_toBytes(const SceneAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, bool includeUnused, AssetsBundle::ConstPtr assets, const Ordered<int>::Pages &orderedActorsInScene, ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour, Pipeline::ErrorHandler onError) {
	auto serializePlaneLayer = [] (
		const Map::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused, int layer
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		if (inuse) {
			const int w = data->width();
			const int h = data->height();
			for (int j = 0; j < h; ++j) {
				for (int i = 0; i < w; ++i) {
					const UInt8 cel = (UInt8)data->get(i, j);
					bytes->writeUInt8(cel);
				}
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, layer);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addScene((int)bytes->count());

		return true;
	};
	auto serializeActorLayer = [entry, &orderedActorsInScene] (
		const Map::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused, int layer,
		AssetsBundle::ConstPtr assets,
		Pipeline::ErrorHandler onError
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			const size_t cur = bytes->peek();
			bytes->writeUInt8((UInt8)0);                              // The count. Filled at a).
			int n = 0;
			GBBASIC_ASSERT(page <= (int)orderedActorsInScene.size() && "Impossible.");
			const Ordered<int>::Array &orderedActors = orderedActorsInScene[page];
			for (const Ordered<int> &orderedActor : orderedActors) {
				const int i = orderedActor.position.x;
				const int j = orderedActor.position.y;
				const UInt8 cel = (UInt8)orderedActor.value;
				GBBASIC_ASSERT(cel != Scene::INVALID_ACTOR() && "Impossible.");

				const ActorAssets &actors = assets->actors;
				const ActorAssets::Entry* actorEntry = actors.get(cel);
				int updateRoutineBank = 0;
				int updateRoutineAddress = 0;
				int hitsRoutineBank = 0;
				int hitsRoutineAddress = 0;
				if (actorEntry && actorEntry->data) {
					Actor::Ptr actor = actorEntry->data;
					if (!actor->updateRoutine().empty() && !pipeline->codeLocationHandler()(actor->updateRoutine(), &updateRoutineBank, &updateRoutineAddress, nullptr)) {
						updateRoutineBank = 0;
						updateRoutineAddress = 0;

						const std::string msg = Text::format(
							"Invalid code binding (UPDATE) of actor {0}.",
							{
								Text::toString(cel)
							}
						);
						onError(msg, false, AssetsBundle::Categories::SCENE, page);
					}
					if (!actor->onHitsRoutine().empty() && !pipeline->codeLocationHandler()(actor->onHitsRoutine(), &hitsRoutineBank, &hitsRoutineAddress, nullptr)) {
						hitsRoutineBank = 0;
						hitsRoutineAddress = 0;

						const std::string msg = Text::format(
							"Invalid code binding (HITS) of actor {0}.",
							{
								Text::toString(cel)
							}
						);
						onError(msg, false, AssetsBundle::Categories::SCENE, page);
					}
					const ActorRoutineOverriding::Array &actorRoutines = entry->actorRoutineOverridings;
					const ActorRoutineOverriding* actorRoutine = Routine::search(actorRoutines, Math::Vec2i(i, j));
					if (actorRoutine) {
						if (!actorRoutine->routines.update.empty() && !pipeline->codeLocationHandler()(actorRoutine->routines.update, &updateRoutineBank, &updateRoutineAddress, nullptr)) {
							updateRoutineBank = 0;
							updateRoutineAddress = 0;

							const std::string msg = Text::format(
								"Invalid code binding (UPDATE) of actor {0} at scene page {1}.",
								{
									Text::toString(cel),
									Text::toString(page)
								}
							);
							onError(msg, false, AssetsBundle::Categories::SCENE, page);
						}
						if (!actorRoutine->routines.onHits.empty() && !pipeline->codeLocationHandler()(actorRoutine->routines.onHits, &hitsRoutineBank, &hitsRoutineAddress, nullptr)) {
							hitsRoutineBank = 0;
							hitsRoutineAddress = 0;

							const std::string msg = Text::format(
								"Invalid code binding (HITS) of actor {0} at scene page {1}.",
								{
									Text::toString(cel),
									Text::toString(page)
								}
							);
							onError(msg, false, AssetsBundle::Categories::SCENE, page);
						}
					}
				}

				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.argsOffset = offset;
				scheduled.push_back(s_);
				bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);       // The tiles count.
				bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);       // The tiles bank.
				bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER);     // The tiles address.
				bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);       // The sprite count.
				bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);       // The ref bank.
				bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER);     // The ref address.
				bytes->writeUInt8((UInt8)cel);                        // The cel.
				const int x = i * GBBASIC_TILE_SIZE;
				const int y = j * GBBASIC_TILE_SIZE;
				write(bytes.get(), (UInt16)x, Endians::LITTLE);       // The X position.
				write(bytes.get(), (UInt16)y, Endians::LITTLE);       // The Y position.
				bytes->writeUInt8((UInt8)updateRoutineBank);          // The behave routine bank.
				bytes->writeUInt16((UInt16)updateRoutineAddress);     // The behave routine address.
				bytes->writeUInt8((UInt8)hitsRoutineBank);            // The hits routine bank.
				bytes->writeUInt16((UInt16)hitsRoutineAddress);       // The hits routine address.

				++n;
			}
			bytes->poke(cur);
			bytes->writeUInt8((UInt8)n);                              // a).
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled, layer);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addScene((int)bytes->count());

		return true;
	};
	auto serializeTriggerLayer = [] (
		const Trigger::Array* data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused, int layer,
		Pipeline::ErrorHandler onError
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			const int n = (int)data->size();
			bytes->writeUInt8((UInt8)n);
			for (int i = 0; i < n; ++i) {
				const Trigger &trigger = (*data)[i];

				const int offset = (int)bytes->peek();
				Destination::Chunk::Scheduled s_;
				s_.argsOffset = offset;
				scheduled.push_back(s_);
				const int x = trigger.xMin();
				const int y = trigger.yMin();
				const int width = trigger.width();
				const int height = trigger.height();
				const int hitsRoutineType = trigger.eventType;
				int hitsRoutineBank = 0;
				int hitsRoutineAddress = 0;
				if (hitsRoutineType != TRIGGER_HAS_NONE) {
					if (!trigger.eventRoutine.empty() && !pipeline->codeLocationHandler()(trigger.eventRoutine, &hitsRoutineBank, &hitsRoutineAddress, nullptr)) {
						hitsRoutineBank = 0;
						hitsRoutineAddress = 0;

						const std::string msg = Text::format(
							"Invalid code binding (HITS) of trigger {0} at scene page {1}.",
							{
								Text::toString(i),
								Text::toString(page)
							}
						);
						onError(msg, false, AssetsBundle::Categories::SCENE, page);
					}
				}
				bytes->writeUInt8((UInt8)x);
				bytes->writeUInt8((UInt8)y);
				bytes->writeUInt8((UInt8)width);
				bytes->writeUInt8((UInt8)height);
				bytes->writeUInt8((UInt8)hitsRoutineType);      // The hits routine type.
				bytes->writeUInt8((UInt8)hitsRoutineBank);      // The hits routine bank.
				bytes->writeUInt16((UInt16)hitsRoutineAddress); // The hits routine address.
			}
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled, layer);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addScene((int)bytes->count());

		return true;
	};
	auto serializeDefinition = [entry, isPlayerBehaviour] (
		const Scene::Ptr &data,
		const scene_t &def,
		Pipeline* pipeline, Table &table, int page, bool includeUnused, int layer
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		if (inuse) {
			const bool is16x16Player = entry->has16x16PlayerActor(isPlayerBehaviour);
			bytes->writeUInt8((UInt8)def.is_16x16_grid);
			bytes->writeUInt8((UInt8)is16x16Player);
			bytes->writeUInt8((UInt8)def.gravity);
			bytes->writeUInt8((UInt8)def.jump_gravity);
			bytes->writeUInt8((UInt8)def.jump_max_count);
			bytes->writeUInt8((UInt8)def.jump_max_ticks);
			bytes->writeUInt8((UInt8)def.climb_velocity);
			write(bytes.get(), (UInt16)def.camera_position.x, Endians::LITTLE);
			write(bytes.get(), (UInt16)def.camera_position.y, Endians::LITTLE);
			bytes->writeUInt8((UInt8)def.camera_deadzone.x);
			bytes->writeUInt8((UInt8)def.camera_deadzone.y);
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, layer);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addScene((int)bytes->count());

		return true;
	};
	auto serializeLayers = [] (
		const Scene::Ptr &data,
		Pipeline* pipeline, Table &table, int page, bool includeUnused
	) -> bool {
		// Prepare.
		if (!data)
			return false;

		// Check whether the asset is in use.
		bool inuse = true;
		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it != table.end()) {
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				inuse = false;
		}

		// Generate the bytes.
		Bytes::Ptr bytes(Bytes::create());
		Destination::Chunk::Scheduled::Array scheduled;
		if (inuse) {
			const int offset = (int)bytes->peek();
			Destination::Chunk::Scheduled s_;
			s_.argsOffset = offset;
			scheduled.push_back(s_);
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The layer mask.
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The map bank.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The map address.
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The attributes bank.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The attributes address.
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The properties bank.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The properties address.
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The actor bank.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The actor address.
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The trigger bank.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The trigger address.
			bytes->writeUInt8((UInt8)COMPILER_PLACEHOLDER);   // The definition bank.
			bytes->writeUInt16((UInt16)COMPILER_PLACEHOLDER); // The definition address.
		}

		// Fill in with the destination data.
		const Destination::Chunk chunk(bytes, scheduled);
		if (it == table.end()) {
			const Destination::Chunk::Array dst({ chunk });
			const std::pair<Table::iterator, bool> ret = table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		} else {
			it->second.chunks.push_back(chunk);
		}

		// Ignore unused.
		if (!includeUnused && it->second.refCount == 0)
			return true;

		// Finish.
		pipeline->effectiveSize().addScene((int)bytes->count());

		return true;
	};

	const MapAssets::Entry* mapEntry = entry->getMap(entry->refMap);
	if (mapEntry && !mapEntry->hasAttributes) {
		if (entry->data->hasAttributes()) {
			if (!serializePlaneLayer(entry->data->attributeLayer(), pipeline, table, page, includeUnused, ASSETS_SCENE_ATTRIBUTES_LAYER))
				return false;
		}
	}
	if (entry->data->hasProperties()) {
		if (!serializePlaneLayer(entry->data->propertyLayer(), pipeline, table, page, includeUnused, ASSETS_SCENE_PROPERTIES_LAYER))
			return false;
	}
	if (entry->data->hasActors()) {
		if (!serializeActorLayer(entry->data->actorLayer(), pipeline, table, page, includeUnused, ASSETS_SCENE_ACTORS_LAYER, assets, onError))
			return false;
	}
	if (entry->data->hasTriggers()) {
		if (!serializeTriggerLayer(entry->data->triggerLayer(), pipeline, table, page, includeUnused, ASSETS_SCENE_TRIGGERS_LAYER, onError))
			return false;
	}
	if (!serializeDefinition(entry->data, entry->definition, pipeline, table, page, includeUnused, ASSETS_SCENE_DEF_LAYER))
		return false;
	if (!serializeLayers(entry->data, pipeline, table, page, includeUnused))
		return false;

	return true;
}
static bool post_toBytes(const SceneAssets::Entry* entry, Pipeline* pipeline, Table &table, int page, int refMap, const ActorAssets &actors, const Ordered<int>::Pages &orderedActorsInScene) {
	auto serializeActorLayer = [&orderedActorsInScene] (
		const Map::Ptr &data,
		const ActorAssets &actors,
		Pipeline*, Table &table, int page
	) -> bool {
		if (!data)
			return false;

		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it == table.end())
			return true;
		Destination::Chunk::Array::iterator itChunk = std::find_if(
			it->second.chunks.begin(), it->second.chunks.end(),
			[] (const Destination::Chunk &chunk) -> bool {
				return chunk.layer == ASSETS_SCENE_ACTORS_LAYER;
			}
		);
		if (itChunk == it->second.chunks.end())
			return true;

		Destination::Chunk &chunkActors = *itChunk;
		const Destination::Chunk::Scheduled::Array &scheduledActors = chunkActors.scheduled;
		if (scheduledActors.empty())
			return true;

		int n = 0;
		GBBASIC_ASSERT(page <= (int)orderedActorsInScene.size() && "Impossible.");
		const Ordered<int>::Array &orderedActors = orderedActorsInScene[page];
		int currentActorIndex = -1;
		for (const Ordered<int> &orderedActor : orderedActors) {
			const UInt8 cel = (UInt8)orderedActor.value;
			GBBASIC_ASSERT(cel != Scene::INVALID_ACTOR() && "Impossible.");
			const bool isNewActorIndex = currentActorIndex != (int)cel;
			if (isNewActorIndex)
				currentActorIndex = (int)cel;

			if (n >= (int)scheduledActors.size())
				return false;

			const Source srcActor(AssetsBundle::Categories::ACTOR, cel);
			Table::iterator itActor = table.find(srcActor);
			if (itActor == table.end())
				return false;
			if (itActor->second.chunks.size() != 3)
				return false;

			const Destination::Chunk &chunkTiles = itActor->second.chunks.front();
			const int tilesCount = (int)chunkTiles.bytes->count() / 16;
			const int tilesBank = chunkTiles.bank;
			const int tilesAddress = chunkTiles.address;

			const ActorAssets::Entry* actorEntry = actors.get(cel);
			int spriteCount = 0;
			for (const Actor::Shadow &shadow : actorEntry->shadow)
				spriteCount += (int)shadow.refs.size();

			const Destination::Chunk &chunkActor = itActor->second.chunks.back();
			const int bankActor = chunkActor.bank;
			const int addressActor = chunkActor.address;

			Bytes::Ptr &bytesDef = chunkActors.bytes;
			const Destination::Chunk::Scheduled &s = scheduledActors[n];
			const Byte* ptr = bytesDef->pointer();
			if (isNewActorIndex) {
				// When it is a new actor index, tell the tiles count, then it will fill the tiles.
				*((UInt8*)s.args(ptr)) = (UInt8)tilesCount;    ptr += sizeof(UInt8);  // The tiles count.
			} else {
				// Otherwise will not re-fill or grow tiles.
				*((UInt8*)s.args(ptr)) = (UInt8)0;             ptr += sizeof(UInt8);  // The tiles count.
			}
			*((UInt8*)s.args(ptr))     = (UInt8)tilesBank;     ptr += sizeof(UInt8);  // The tiles bank.
			*((UInt16*)s.args(ptr))    = (UInt16)tilesAddress; ptr += sizeof(UInt16); // The tiles address.
			*((UInt8*)s.args(ptr))     = (UInt8)spriteCount;   ptr += sizeof(UInt8);  // The sprite count.
			*((UInt8*)s.args(ptr))     = (UInt8)bankActor;     ptr += sizeof(UInt8);  // The ref bank.
			*((UInt16*)s.args(ptr))    = (UInt16)addressActor; ptr += sizeof(UInt16); // The ref address.

			++n;
		}

		return true;
	};
	auto serializeLayers = [] (
		const Scene::Ptr &data,
		Pipeline*, Table &table, int page,
		int refMap
	) -> bool {
		if (!data)
			return false;

		const Source src(AssetsBundle::Categories::SCENE, page);
		Table::iterator it = table.find(src);
		if (it == table.end())
			return true;
		if (it->second.chunks.size() < 1)
			return true;

		Destination::Chunk &chunkLayers = it->second.chunks.back();
		const Destination::Chunk::Scheduled::Array &scheduledDef = chunkLayers.scheduled;
		if (scheduledDef.empty())
			return true;

		const Source srcMap(AssetsBundle::Categories::MAP, refMap);
		Table::iterator itMap = table.find(srcMap);
		if (itMap == table.end())
			return false;
		if (itMap->second.chunks.size() < 1)
			return false;

		UInt8 layers = 0;
		int bankMap = 0;
		int addressMap = 0;
		int bankAttrib = 0;
		int addressAttrib = 0;
		int bankProp = 0;
		int addressProp = 0;
		int bankActor = 0;
		int addressActor = 0;
		int bankTrigger = 0;
		int addressTrigger = 0;
		int bankDef = 0;
		int addressDef = 0;

		const bool useMapAttr = itMap->second.chunks.size() >= 2;
		int n = 0;
		layers |= (UInt8)Layers::SCENE_LAYER_MAP; // Must have.
		const Destination::Chunk &chunkMap = itMap->second.chunks[0];
		bankMap = chunkMap.bank;
		addressMap = chunkMap.address;
		if (data->hasAttributes()) {
			layers |= (UInt8)Layers::SCENE_LAYER_ATTR;
			const Destination::Chunk &chunkAttrib = useMapAttr ? itMap->second.chunks[1] : it->second.chunks[n++];
			bankAttrib = chunkAttrib.bank;
			addressAttrib = chunkAttrib.address;
		}
		if (data->hasProperties()) {
			layers |= (UInt8)Layers::SCENE_LAYER_PROP;
			const Destination::Chunk &chunkProp = it->second.chunks[n++];
			bankProp = chunkProp.bank;
			addressProp = chunkProp.address;
		}
		if (data->hasActors()) {
			layers |= (UInt8)Layers::SCENE_LAYER_ACTOR;
			const Destination::Chunk &chunkActor = it->second.chunks[n++];
			bankActor = chunkActor.bank;
			addressActor = chunkActor.address;
		}
		if (data->hasTriggers()) {
			layers |= (UInt8)Layers::SCENE_LAYER_TRIGGER;
			const Destination::Chunk &chunkTrigger = it->second.chunks[n++];
			bankTrigger = chunkTrigger.bank;
			addressTrigger = chunkTrigger.address;
		}
		layers |= (UInt8)Layers::SCENE_LAYER_DEF; // Must have.
		const Destination::Chunk &chunkDef = it->second.chunks[n++];
		bankDef = chunkDef.bank;
		addressDef = chunkDef.address;

		Bytes::Ptr &bytesDef = chunkLayers.bytes;
		const Destination::Chunk::Scheduled &s = scheduledDef.front();
		const Byte* ptr = bytesDef->pointer();
		*((UInt8*)s.args(ptr))  = (UInt8)layers;          ptr += sizeof(UInt8);  // The layer mask.
		*((UInt8*)s.args(ptr))  = (UInt8)bankMap;         ptr += sizeof(UInt8);  // The map bank.
		*((UInt16*)s.args(ptr)) = (UInt16)addressMap;     ptr += sizeof(UInt16); // The map address.
		*((UInt8*)s.args(ptr))  = (UInt8)bankAttrib;      ptr += sizeof(UInt8);  // The attributes bank.
		*((UInt16*)s.args(ptr)) = (UInt16)addressAttrib;  ptr += sizeof(UInt16); // The attributes address.
		*((UInt8*)s.args(ptr))  = (UInt8)bankProp;        ptr += sizeof(UInt8);  // The properties bank.
		*((UInt16*)s.args(ptr)) = (UInt16)addressProp;    ptr += sizeof(UInt16); // The properties address.
		*((UInt8*)s.args(ptr))  = (UInt8)bankActor;       ptr += sizeof(UInt8);  // The actor bank.
		*((UInt16*)s.args(ptr)) = (UInt16)addressActor;   ptr += sizeof(UInt16); // The actor address.
		*((UInt8*)s.args(ptr))  = (UInt8)bankTrigger;     ptr += sizeof(UInt8);  // The trigger bank.
		*((UInt16*)s.args(ptr)) = (UInt16)addressTrigger; ptr += sizeof(UInt16); // The trigger address.
		*((UInt8*)s.args(ptr))  = (UInt8)bankDef;         ptr += sizeof(UInt8);  // The definition bank.
		*((UInt16*)s.args(ptr)) = (UInt16)addressDef;     ptr += sizeof(UInt16); // The definition address.

		return true;
	};

	if (entry->data->hasActors()) {
		if (!serializeActorLayer(entry->data->actorLayer(), actors, pipeline, table, page))
			return false;
	}
	if (!serializeLayers(entry->data, pipeline, table, page, refMap))
		return false;

	return true;
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Pipeline
*/

namespace GBBASIC {

class PipelineImpl : public Pipeline {
private:
	typedef std::vector<size_t> HashArray;

private:
	/**
	 * @brief The cartridge compatibility.
	 */
	GBBASIC::Options::Strategies::Compatibilities _compatibility = GBBASIC::Options::Strategies::Compatibilities::CLASSIC | GBBASIC::Options::Strategies::Compatibilities::COLORED;
	/**
	 * @brief Const. The maximum ROM size.
	 */
	int _romMaxSize = 0;
	/**
	 * @brief Const. The index of the start ROM bank.
	 */
	int _startBank = 0;
	/**
	 * @brief Const. The size of a ROM bank.
	 */
	int _bankSize = 0;
	/**
	 * @brief Const. The start address of a ROM bank.
	 */
	int _startAddress = 0;
	/**
	 * @brief Non-const. The index of the ROM bank.
	 */
	int* _bank = nullptr; // Foreign.
	/**
	 * @brief Non-const. The address cursor in the ROM bank.
	 */
	int* _addressCursor = nullptr; // Foreign.
	/**
	 * @brief The shared compiled bytes.
	 */
	Bytes::Ptr _bytes = nullptr; // Foreign.
	/**
	 * @brief The source code location getter.
	 */
	CodeLocationHandler _codeLocationHandler = nullptr; // Foreign.
	/**
	 * @brief The effective data size.
	 */
	Size _effectiveSize;

	/**
	 * @brief The ROM allocator.
	 */
	FreeSpace _allocator;
	/**
	 * @brief The lookup table.
	 */
	Table _table;

	/**
	 * @brief Get whether the specific behaviour is for player.
	 */
	ActorAssets::Entry::PlayerBehaviourCheckingHandler _isPlayerBehaviour = nullptr;
	/**
	 * @brief The message output handler.
	 */
	PrintHandler _onPrint = nullptr;
	/**
	 * @brief The error output handler.
	 */
	ErrorHandler _onError = nullptr;

	/**
	 * @brief Whether to use work queue.
	 */
	bool _useWorkQueue = true;
	/**
	 * @brief Whether to output less to the console.
	 */
	bool _lessConsoleOutput = false;
	/**
	 * @brief Asynchronized work queue.
	 */
	WorkQueue* _workQueue = nullptr;

public:
	PipelineImpl(bool useWorkQueue, bool lessConsoleOutput) : _useWorkQueue(useWorkQueue), _lessConsoleOutput(lessConsoleOutput) {
		if (_useWorkQueue) {
			_workQueue = WorkQueue::create();
		}
	}
	virtual ~PipelineImpl() override {
		if (_workQueue) {
			WorkQueue::destroy(_workQueue);
			_workQueue = nullptr;
		}
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual unsigned compatibility(void) const override {
		return (unsigned)_compatibility;
	}
	virtual void compatibility(unsigned val) override {
		_compatibility = (GBBASIC::Options::Strategies::Compatibilities)val;
	}

	virtual int romMaxSize(void) const override {
		return _romMaxSize;
	}
	virtual void romMaxSize(int val) override {
		_romMaxSize = val;
	}

	virtual int startBank(void) const override {
		return _startBank;
	}
	virtual void startBank(int val) override {
		_startBank = val;
	}

	virtual int bankSize(void) const override {
		return _bankSize;
	}
	virtual void bankSize(int val) override {
		_bankSize = val;
	}

	virtual int startAddress(void) const override {
		return _startAddress;
	}
	virtual void startAddress(int val) override {
		_startAddress = val;
	}

	virtual int* bank(void) const override {
		return _bank;

	}
	virtual void bank(int* val) override {
		_bank = val;
	}

	virtual int* addressCursor(void) const override {
		return _addressCursor;
	}
	virtual void addressCursor(int* val) override {
		_addressCursor = val;
	}

	virtual Bytes::Ptr bytes(void) const override {
		return _bytes;
	}
	virtual void bytes(Bytes::Ptr val) override {
		_bytes = val;
	}

	virtual CodeLocationHandler codeLocationHandler(void) const override {
		return _codeLocationHandler;
	}
	virtual void codeLocationHandler(CodeLocationHandler val) override {
		_codeLocationHandler = val;
	}

	virtual const Size &effectiveSize(void) const override {
		return _effectiveSize;
	}
	virtual Size &effectiveSize(void) override {
		return _effectiveSize;
	}

	virtual void open(void) override {
		if (_workQueue) {
			_workQueue->startup("PIPELINE", 4);
		}
	}
	virtual void close(void) override {
		if (_workQueue) {
			_workQueue->shutdown();
		}
	}

	virtual ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour(void) const override {
		return _isPlayerBehaviour;
	}
	virtual void isPlayerBehaviour(ActorAssets::Entry::PlayerBehaviourCheckingHandler val) override {
		_isPlayerBehaviour = val;
	}

	virtual PrintHandler onPrint(void) const override {
		return _onPrint;
	}
	virtual void onPrint(PrintHandler val) override {
		_onPrint = val;
	}

	virtual ErrorHandler onError(void) const override {
		return _onError;
	}
	virtual void onError(ErrorHandler val) override {
		_onError = val;
	}

	virtual bool pipe(AssetsBundle::ConstPtr assets, bool includeUnused, bool optimizeAssets) override {
		const long long start = DateTime::ticks();

		Ordered<int>::Pages orderedActorsInScene;
		const bool ok =
			generate(assets, includeUnused, orderedActorsInScene) &&           // Generate assets into pieces of bytes.
			post(assets, includeUnused, optimizeAssets, orderedActorsInScene); // Fill in the final bytes with the generated ones.

		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const int assetsSize = _effectiveSize.total() - _effectiveSize.sgbResources();
		const double secs = assetsSize != 0 ? DateTime::toSeconds(diff) : 0;

		if (ok) {
			if (!_lessConsoleOutput) {
				const std::string dstn = Text::toScaledBytes(assetsSize);
				const std::string time = Text::toString(secs, 6, 0, ' ', std::ios::fixed);
				const std::string msg = Text::format("Succeeded to process the assets to {0} in {1}s.", { dstn, time });
				_onPrint(msg, AssetsBundle::Categories::NONE);
			}
		} else {
			_onError("Failed to process the assets.", false, AssetsBundle::Categories::NONE, -1);
		}

		return ok;
	}

	virtual bool touch(AssetsBundle::ConstPtr assets, AssetsBundle::Categories category, int page, bool incRef) override {
		const Source src(category, page);
		Table::iterator it = _table.find(src);
		if (it == _table.end()) {
			const Destination dst;
			const std::pair<Table::iterator, bool> ret = _table.insert(std::make_pair(src, dst));
			if (!ret.second)
				return false;

			it = ret.first;
		}

		if (incRef) {
			Destination &dst = it->second;
			++dst.refCount; // Increase reference count by source code.

			if (category == AssetsBundle::Categories::SCENE) { // Process this scene's referenced assets.
				const SceneAssets &scenes = assets->scenes;
				const SceneAssets::Entry* entry = scenes.get(src.page);

				touch(assets, AssetsBundle::Categories::MAP, entry->refMap, incRef);

				SceneAssets::Entry::UniqueRef uref;
				const SceneAssets::Entry::Ref refActors = entry->getRefActors(&uref);
				(void)refActors;
				for (int refActor : uref)
					touch(assets, AssetsBundle::Categories::ACTOR, refActor, incRef);
			}
		}

		return true;
	}
	virtual bool lookup(AssetsBundle::Categories category, int page, Resource::Array &locations) const override {
		locations.clear();

		const Source src(category, page);
		Table::const_iterator it = _table.find(src);
		if (it == _table.end())
			return false;

		const Destination &dst = it->second;
		for (const Destination::Chunk &chunk : dst.chunks) {
			const Resource inRom(chunk.bank, chunk.address, (int)chunk.bytes->count());
			locations.push_back(inRom);
		}

		return true;
	}
	virtual bool lookup(AssetsBundle::Categories category, int page, int &refCount) const override {
		refCount = 0;

		const Source src(category, page);
		Table::const_iterator it = _table.find(src);
		if (it == _table.end())
			return false;

		const Destination &dst = it->second;
		refCount = dst.refCount;

		return true;
	}

	virtual void prompt(AssetsBundle::ConstPtr assets) override {
		// Calculate hash for each asset for further duplication detection.
		HashArray hashedFonts;
		HashArray hashedCode;
		HashArray hashedTiles;
		HashArray hashedMaps;
		HashArray hashedMusic;
		HashArray hashedSfx;
		HashArray hashedActors;
		HashArray hashedScenes;

		calculateHash <FontAssets>(hashedFonts,  assets->fonts );
		calculateHash <CodeAssets>(hashedCode,   assets->code  );
		calculateHash<TilesAssets>(hashedTiles,  assets->tiles );
		calculateHash  <MapAssets>(hashedMaps,   assets->maps  );
		calculateHash<MusicAssets>(hashedMusic,  assets->music );
		calculateHash  <SfxAssets>(hashedSfx,    assets->sfx   );
		calculateHash<ActorAssets>(hashedActors, assets->actors);
		calculateHash<SceneAssets>(hashedScenes, assets->scenes);

		// Output the duplicate assets.
		checkDuplication <FontAssets>(hashedFonts,  AssetsBundle::Categories::FONT,  "Font",  assets->fonts );
		checkDuplication <CodeAssets>(hashedCode,   AssetsBundle::Categories::CODE,  "Code",  assets->code  );
		checkDuplication<TilesAssets>(hashedTiles,  AssetsBundle::Categories::TILES, "Tiles", assets->tiles );
		checkDuplication  <MapAssets>(hashedMaps,   AssetsBundle::Categories::MAP,   "Map",   assets->maps  );
		checkDuplication<MusicAssets>(hashedMusic,  AssetsBundle::Categories::MUSIC, "Music", assets->music );
		checkDuplication  <SfxAssets>(hashedSfx,    AssetsBundle::Categories::SFX,   "Sfx",   assets->sfx   );
		checkDuplication<ActorAssets>(hashedActors, AssetsBundle::Categories::ACTOR, "Actor", assets->actors);
		checkDuplication<SceneAssets>(hashedScenes, AssetsBundle::Categories::SCENE, "Scene", assets->scenes);

		// Check references from asset to another (reference in source code is increased via the `touch` method).
		for (Table::reverse_iterator it = _table.rbegin(); it != _table.rend(); ++it) {
			const Source &src = it->first;
			const Destination &dst = it->second;
			switch (src.category) {
			case AssetsBundle::Categories::FONT:
				// Do nothing.

				break;
			case AssetsBundle::Categories::CODE:
				// Do nothing.

				break;
			case AssetsBundle::Categories::TILES:// Do nothing.

				break;
			case AssetsBundle::Categories::MAP: {
					const MapAssets &maps = assets->maps;
					const MapAssets::Entry* entry = maps.get(src.page);

					if (dst.refCount == 0) // Check whether this asset has been referenced.
						break;

					if (entry) {
						const Source ref(AssetsBundle::Categories::TILES, entry->ref);
						Table::iterator rit = _table.find(ref);
						if (rit != _table.end()) {
							Destination &rdst = rit->second;
							++rdst.refCount; // Increase reference count by asset.
						}
					}
				}

				break;
			case AssetsBundle::Categories::MUSIC:
				// Do nothing.

				break;
			case AssetsBundle::Categories::SFX:
				// Do nothing.

				break;
			case AssetsBundle::Categories::ACTOR: // Fall through.
			case AssetsBundle::Categories::PROJECTILE:
				// Do nothing.

				break;
			case AssetsBundle::Categories::SCENE: {
					const SceneAssets &scenes = assets->scenes;
					const SceneAssets::Entry* entry = scenes.get(src.page);

					if (dst.refCount == 0) // Check whether this asset has been referenced.
						break;

					if (entry) {
						const Source refMap(AssetsBundle::Categories::MAP, entry->refMap);
						Table::iterator rmit = _table.find(refMap);
						if (rmit != _table.end()) {
							Destination &rdst = rmit->second;
							++rdst.refCount; // Increase reference count by asset.
						}
						SceneAssets::Entry::UniqueRef uref;
						const SceneAssets::Entry::Ref refActors = entry->getRefActors(&uref);
						(void)refActors;
						for (int refActor : uref) {
							const Source refAct(AssetsBundle::Categories::ACTOR, refActor);
							Table::iterator rait = _table.find(refAct);
							if (rait != _table.end()) {
								Destination &rdst = rait->second;
								++rdst.refCount; // Increase reference count by asset.
							}
						}
					}
				}

				break;
			default:
				GBBASIC_ASSERT(false && "Impossible.");

				break;
			}
		}

		// Output the unused assets.
		std::string msg;
		int unusedCount = 0;
		for (Table::const_iterator it = _table.begin(); it != _table.end(); ++it) {
			// Prepare.
			const Source &src = it->first;
			const Destination &dst = it->second;

			// Ignore referenced.
			if (dst.refCount > 0)
				continue;

			if (src.category == AssetsBundle::Categories::ACTOR) {
				const Source alias(AssetsBundle::Categories::PROJECTILE, src.page);
				const Table::const_iterator ait = _table.find(alias);
				if (ait != _table.end())
					continue;
			} else if (src.category == AssetsBundle::Categories::PROJECTILE) {
				const Source alias(AssetsBundle::Categories::ACTOR, src.page);
				const Table::const_iterator ait = _table.find(alias);
				if (ait != _table.end())
					continue;
			}

			// Count referenced.
			const std::string cat = AssetsBundle::nameOf(src.category);
			const std::string page = Text::toPageNumber(src.page);
			if (unusedCount > 0)
				msg += ",\n";
			msg += "  " + Text::format("{0} at page {1}", { cat, page });
			++unusedCount;
		}
		if (unusedCount > 0) {
			if (unusedCount == 1)
				msg = "Ignored unused asset:\n" + msg + ".";
			else
				msg = "Ignored unused assets:\n" + msg + ".";
			_onPrint(msg, AssetsBundle::Categories::NONE);
		}
	}

private:
	bool generate(AssetsBundle::ConstPtr assets, bool includeUnused, Ordered<int>::Pages &orderedActorsInScene) {
		// Prepare.
		bool result = true;

		// Compact the actor assets.
		asyncCompactActors(assets);

		// Sort the actors in scene assets.
		asyncSortActorsInScene(assets, orderedActorsInScene);

		// Generate the tiles assets.
		const TilesAssets &tiles = assets->tiles;
		for (int i = 0; i < tiles.count(); ++i) {
			const TilesAssets::Entry* entry = tiles.get(i);
			if (!generate_toBytes(entry, this, _table, i, includeUnused))
				result = false;
		}

		// Generate the map assets.
		const MapAssets &maps = assets->maps;
		const bool colored = (_compatibility & GBBASIC::Options::Strategies::Compatibilities::COLORED) != GBBASIC::Options::Strategies::Compatibilities::NONE;
		bool usedColoredFeatureFlip = false;
		for (int i = 0; i < maps.count(); ++i) {
			const MapAssets::Entry* entry = maps.get(i);
			if (!generate_toBytes(entry, this, _table, i, includeUnused))
				result = false;

			bool inuse = true;
			const Source src(AssetsBundle::Categories::MAP, i);
			Table::iterator it = _table.find(src);
			if (it == _table.end()) {
				inuse = false;
			} else {
				Destination &dst = it->second;
				if (!includeUnused && dst.refCount == 0) // Not referenced.
					inuse = false;
			}

			if (inuse) {
				if (entry->ref < 0 || entry->ref >= tiles.count()) {
					const std::string msg = Text::format(
						entry->ref == -1 ?
							"Map page {0} is referencing an invalid tiles asset {1}." :
							"Map page {0} is referencing an invalid tiles asset.",
						{
							Text::toString(i),
							Text::toString(entry->ref)
						}
					);
					_onError(msg, true, AssetsBundle::Categories::MAP, i);
				}

				if (entry->hasAttributes) {
					for (int j = 0; j < entry->attributes->height(); ++j) {
						for (int i = 0; i < entry->attributes->width(); ++i) {
							const UInt8 attrs = (UInt8)entry->attributes->get(i, j);
							// These bits are for horizontal flip and vertical flip respectively.
							if (!!(attrs & (1 << GBBASIC_MAP_HFLIP_BIT)) || !!(attrs & (1 << GBBASIC_MAP_VFLIP_BIT))) {
								usedColoredFeatureFlip |= true;

								break;
							}
						}

						if (usedColoredFeatureFlip)
							break;
					}
				}
			}
		}

		if (!colored) {
			if (usedColoredFeatureFlip)
				_onError("Tile flip is available with colored-compatible cartridge only.", true, AssetsBundle::Categories::MAP, -1);
		}

		// Generate the music assets.
		const MusicAssets &music = assets->music;
		for (int i = 0; i < music.count(); ++i) {
			const MusicAssets::Entry* entry = music.get(i);
			if (!generate_toBytes(entry, this, _table, i, includeUnused))
				result = false;
		}

		// Generate the SFX assets.
		const SfxAssets &sfx = assets->sfx;
		for (int i = 0; i < sfx.count(); ++i) {
			const SfxAssets::Entry* entry = sfx.get(i);
			if (!generate_toBytes(entry, this, _table, i, includeUnused))
				result = false;
		}

		// Ensure the async procedures are completed.
		await();

		// Generate the actor assets.
		const ActorAssets &actors = assets->actors;
		int _8x8Count = 0;
		int _8x16Count = 0;
		for (int i = 0; i < actors.count(); ++i) {
			const ActorAssets::Entry* entry = actors.get(i);
			if (!generate_toBytes(entry, this, _table, i, includeUnused))
				result = false;

			bool inuse = true;
			const Source src(AssetsBundle::Categories::ACTOR, i);
			Table::iterator it = _table.find(src);
			if (it == _table.end()) {
				inuse = false;
			} else {
				Destination &dst = it->second;
				if (!includeUnused && dst.refCount == 0) // Not referenced.
					inuse = false;
			}

			if (inuse) {
				if (entry->data->is8x16())
					++_8x16Count;
				else
					++_8x8Count;
			}
		}

		if (_8x8Count > 0 && _8x16Count > 0 && _8x8Count != _8x16Count) {
			_onError("Mixed 8x8 and 8x16 actors.", true, AssetsBundle::Categories::ACTOR, -1);
		}

		// Generate the scene assets.
		const SceneAssets &scenes = assets->scenes;
		for (int i = 0; i < scenes.count(); ++i) {
			const SceneAssets::Entry* entry = scenes.get(i);
			if (!generate_toBytes(entry, this, _table, i, includeUnused, assets, orderedActorsInScene, _isPlayerBehaviour, _onError))
				result = false;

			bool inuse = true;
			const Source src(AssetsBundle::Categories::SCENE, i);
			Table::iterator it = _table.find(src);
			if (it == _table.end()) {
				inuse = false;
			} else {
				Destination &dst = it->second;
				if (!includeUnused && dst.refCount == 0) // Not referenced.
					inuse = false;
			}

			if (inuse) {
				if (entry->refMap < 0 || entry->refMap >= maps.count()) {
					const std::string msg = Text::format(
						entry->refMap == -1 ?
							"Scene page {0} is referencing an invalid map asset {1}." :
							"Scene page {0} is referencing an invalid map asset.",
						{
							Text::toString(i),
							Text::toString(entry->refMap)
						}
					);
					_onError(msg, true, AssetsBundle::Categories::MAP, i);
				}

				int playerCount = 0;
				int followingCount = 0;
				const SceneAssets::Entry::Ref refActors = entry->getRefActors(nullptr);
				for (int actor : refActors) {
					if (actor < 0 || actor >= actors.count()) {
						const std::string msg = Text::format(
							actor == -1 ?
								"Scene page {0} is referencing an invalid actor asset {1}." :
								"Scene page {0} is referencing an invalid actor asset.",
							{
								Text::toString(i),
								Text::toString(actor)
							}
						);
						_onError(msg, true, AssetsBundle::Categories::MAP, i);
					}

					const ActorAssets::Entry* entry = actors.get(actor);
					if (_isPlayerBehaviour && _isPlayerBehaviour(entry->definition.behaviour))
						++playerCount;
					if (entry && entry->definition.following)
						++followingCount;
				}
				if (playerCount > 1) {
					const std::string msg = Text::format("There are {0} actors have assigned with player controllers in scene {1}.", { Text::toString(playerCount), Text::toString(i) });
					_onError(msg, true, AssetsBundle::Categories::SCENE, i);
				}
				if (followingCount > 1) {
					const std::string msg = Text::format("There are {0} actors have been marked as \"following\" in scene {1}.", { Text::toString(followingCount), Text::toString(i) });
					_onError(msg, true, AssetsBundle::Categories::SCENE, i);
				}

				std::string overlappedTriggers;
				if (entry->data && entry->data->hasTriggers() && entry->data->triggerLayer()) {
					const Trigger::Array* triggers = entry->data->triggerLayer();
					for (int i = 0; i < (int)triggers->size(); ++i) {
						const Trigger &trigger0 = (*triggers)[i];
						const Math::Recti rect0 = trigger0.toRect();
						for (int j = i + 1; j < (int)triggers->size(); ++j) {
							if (j == i)
								continue; // Skip self.

							const Trigger &trigger1 = (*triggers)[j];
							const Math::Recti rect1 = trigger1.toRect();
							if (Math::intersects(rect0, rect1, false))
								overlappedTriggers += "  " + Text::toString(i) + " and " + Text::toString(j) + "\n";
						}
					}
				}
				if (!overlappedTriggers.empty()) {
					overlappedTriggers.erase(overlappedTriggers.end() - 1);
					const std::string msg = Text::format("Trigger overlap in scene {0}:\n{1}", { Text::toString(i), overlappedTriggers });
					_onError(msg, true, AssetsBundle::Categories::SCENE, i);
				}
			}
		}

		// Finish.
		return result;
	}
	bool post(AssetsBundle::ConstPtr assets, bool includeUnused, bool optimizeAssets, const Ordered<int>::Pages &orderedActorsInScene) {
		// Prepare.
		struct OrderedAssets {
			typedef std::list<OrderedAssets> List;

			Table::iterator iterator;
			int size = 0;

			OrderedAssets() {
			}
			OrderedAssets(Table::iterator it, int sz) : iterator(it), size(sz) {
			}
		};

		if (_effectiveSize.total() == 0)
			return true;

		if (_table.empty())
			return true;

		// Get the free space.
		_allocator.clear();
		const std::div_t div = std::div((int)_bytes->count(), _bankSize);
		const int startBank = _startBank + div.quot;
		const int offset = div.rem;
		GBBASIC_ASSERT(offset == *_addressCursor && "Wrong data.");
		const int rest = _bankSize - offset;
		const FreeSpace::Cluster fs(startBank, offset, rest);
		_allocator.add(fs);
		const int maxBanks = _romMaxSize / _bankSize;
		for (int i = startBank + 1; i < maxBanks; ++i) {
			const FreeSpace::Cluster fs(i, 0, _bankSize);
			_allocator.add(fs);
		}

		// Allocate ROM space for the resources.
		auto shouldAllocateContiguously = [] (const Source &src) -> bool {
			return src.category == AssetsBundle::Categories::MUSIC;
		};
		auto canAllocate = [] (const Source &src) -> bool {
			switch (src.category) {
			case AssetsBundle::Categories::FONT:       // Fall through.
			case AssetsBundle::Categories::TILES:      // Fall through.
			case AssetsBundle::Categories::MAP:        // Fall through.
			case AssetsBundle::Categories::MUSIC:      // Fall through.
			case AssetsBundle::Categories::SFX:        // Fall through.
			case AssetsBundle::Categories::ACTOR:      // Fall through.
			case AssetsBundle::Categories::PROJECTILE: // Fall through.
			case AssetsBundle::Categories::SCENE:
				return true;
			default:
				return false;
			}
		};
		auto allocateOne = [&] (Table::iterator it) -> bool {
			// Prepare.
			const Source &src = it->first;
			Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				return true; // Skip, continue outer loop.

			// Check whether is allocatable.
			if (!canAllocate(src)) {
				const std::string cat = AssetsBundle::nameOf(src.category);
				const std::string page = Text::toPageNumber(src.page);
				const std::string msg = Text::format("Unknown allocator for {0} at page {1}.", { cat, page });
				_onError(msg, false, src.category, src.page);

				GBBASIC_ASSERT(false && "Wrong data.");

				return false;
			}

			// Find a contiguously piece of space if needed.
			int clusterIndex = -1;
			if (shouldAllocateContiguously(src)) {
				int totalSize = 0;
				for (int i = 0; i < (int)dst.chunks.size(); ++i) {
					const Destination::Chunk &chunk = dst.chunks[i];
					totalSize += (int)chunk.bytes->count();
				}

				clusterIndex = _allocator.find(totalSize);
				if (clusterIndex == -1) {
					const std::string cat = AssetsBundle::nameOf(src.category);
					const std::string page = Text::toPageNumber(src.page);
					const std::string msg = Text::format("Cannot allocate ROM space for {0} at page {1}.", { cat, page });
					_onError(msg, false, src.category, src.page);

					return false;
				}
			}

			// Allocate and assign the space.
			for (int i = 0; i < (int)dst.chunks.size(); ++i) {
				Destination::Chunk &chunk = dst.chunks[i];
				int bank = 0;
				int addressCursor = 0;
				const bool ok = clusterIndex != -1 ?                                                      // With prefered cluster?
					_allocator.allocate(clusterIndex, (int)chunk.bytes->count(), &bank, &addressCursor) : // Allocate from the prefered cluster, or
					_allocator.allocate((int)chunk.bytes->count(), &bank, &addressCursor);                // allocate from available space.
				if (ok) { // Allocated.
					// Determine the bank and address of the chunk.
					chunk.bank = bank;
					chunk.address = _startAddress + addressCursor;

					// Determine the space of the final ROM.
					const int begin = (bank - _startBank) * _bankSize + addressCursor;
					const int end = begin + (int)chunk.bytes->count();
					_bytes->resize(Math::max(end, (int)_bytes->count()));
					chunk.beginOffset = begin;
					chunk.endOffset = end;
					if (chunk.bank >= *_bank) {
						*_bank = chunk.bank;
						if (end > *_addressCursor)
							*_addressCursor = end;
					}
				} else { // ROM overflow.
					// Report the overflow.
					const std::string cat = AssetsBundle::nameOf(src.category);
					const std::string page = Text::toPageNumber(src.page);
					const std::string msg = Text::format("Cannot allocate ROM space for {0} at page {1}.", { cat, page });
					_onError(msg, false, src.category, src.page);

					return false;
				}
			}

			// Finish.
			return true; // Ok, continue outer loop.
		};

		OrderedAssets::List orderedAssetList;
		if (optimizeAssets) { // Sort the resources.
			for (Table::iterator it = _table.begin(); it != _table.end(); ++it) {
				const Destination &dst = it->second;
				if (!includeUnused && dst.refCount == 0) { // Not referenced.
					orderedAssetList.push_back(OrderedAssets(it, 0));

					continue;
				}

				int totalSize = 0;
				for (int i = 0; i < (int)dst.chunks.size(); ++i) {
					const Destination::Chunk &chunk = dst.chunks[i];
					totalSize += (int)chunk.bytes->count();
				}
				orderedAssetList.push_back(OrderedAssets(it, totalSize));
			}

			orderedAssetList.sort(
				[] (const OrderedAssets &l, const OrderedAssets &r) -> bool {
					if (l.size > r.size) // Larger ahead.
						return true;

					return false;
				}
			);
		}

		if (!optimizeAssets || orderedAssetList.empty()) {
			for (Table::iterator it = _table.begin(); it != _table.end(); ++it) { // Iterate the resources.
				if (!allocateOne(it))
					return false;
			}
		} else {
			for (OrderedAssets &ordered : orderedAssetList) { // Iterate the ordered resources.
				Table::iterator it = ordered.iterator;
				if (!allocateOne(it))
					return false;
			}
		}

		// Post the tiles assets.
		const TilesAssets &tiles = assets->tiles;
		for (int i = 0; i < tiles.count(); ++i) {
			const TilesAssets::Entry* entry = tiles.get(i);
			post_toBytes(entry, this, _table, i);
		}

		// Post the map assets.
		const MapAssets &maps = assets->maps;
		for (int i = 0; i < maps.count(); ++i) {
			const MapAssets::Entry* entry = maps.get(i);
			post_toBytes(entry, this, _table, i);
		}

		// Post the music assets.
		const MusicAssets &music = assets->music;
		for (int i = 0; i < music.count(); ++i) {
			const MusicAssets::Entry* entry = music.get(i);
			post_toBytes(entry, this, _table, i);
		}

		// Post the SFX assets.
		const SfxAssets &sfx = assets->sfx;
		for (int i = 0; i < sfx.count(); ++i) {
			const SfxAssets::Entry* entry = sfx.get(i);
			post_toBytes(entry, this, _table, i);
		}

		// Post the actor assets.
		const ActorAssets &actors = assets->actors;
		for (int i = 0; i < actors.count(); ++i) {
			const ActorAssets::Entry* entry = actors.get(i);
			post_toBytes(entry, this, _table, i);
		}

		// Post the scene assets.
		const SceneAssets &scenes = assets->scenes;
		for (int i = 0; i < scenes.count(); ++i) {
			const SceneAssets::Entry* entry = scenes.get(i);
			const int refMap = entry->refMap;
			post_toBytes(entry, this, _table, i, refMap, actors, orderedActorsInScene);
		}

		// Emit the resources.
		for (Table::const_iterator it = _table.begin(); it != _table.end(); ++it) {
			const Destination &dst = it->second;
			if (!includeUnused && dst.refCount == 0) // Not referenced.
				continue;

			for (int i = 0; i < (int)dst.chunks.size(); ++i) {
				const Destination::Chunk &chunk = dst.chunks[i];
				if (chunk.beginOffset >= 0 && chunk.endOffset > chunk.beginOffset) {
					_bytes->poke(chunk.beginOffset);
					_bytes->writeBytes(chunk.bytes.get());
				}
			}
		}

		// Finish.
		return true;
	}

	void asyncCompactActors(AssetsBundle::ConstPtr assets) const {
		auto compactActor = [] (const ActorAssets::Entry* entry) -> void {
			Actor::Animation &animation = entry->animation;
			Actor::Shadow::Array &shadow = entry->shadow;
			Actor::Slice::Array &slices = entry->slices;
			Actor::Ptr obj = entry->data;
			for (int i = 0; i < obj->count(); ++i) {
				const Actor::Frame* frame = obj->get(i);
				if (frame->slices.empty())
					obj->slice(i);
			}
			obj->compact(animation, shadow, slices, nullptr);
		};

		const ActorAssets &actors = assets->actors;
		for (int i = 0; i < actors.count(); ++i) {
			const ActorAssets::Entry* entry = actors.get(i);

			if (_workQueue) {
				_workQueue->push(
					WorkTaskFunction::create(
						std::bind(
							[compactActor] (WorkTask* /* task */, const ActorAssets::Entry* entry) -> uintptr_t { // On work thread.
								compactActor(entry);

								return 0;
							},
							std::placeholders::_1, entry
						),
						[] (WorkTask* /* task */, uintptr_t) -> void { // On main thread.
							// Do nothing.
						},
						[] (WorkTask* task, uintptr_t) -> void { // On main thread.
							task->disassociated(true);
						}
					)
				);
			} else {
				compactActor(entry);
			}
		}
	}
	void asyncSortActorsInScene(AssetsBundle::ConstPtr assets, Ordered<int>::Pages &orderedActorsInScene) const {
		auto sortActorsInScene = [this] (const Map::Ptr &layer, const ActorAssets &actors, Ordered<int>::Array &orderedActors) -> void {
			const int w = layer->width();
			const int h = layer->height();
			for (int j = 0; j < h; ++j) {
				for (int i = 0; i < w; ++i) {
					const UInt8 cel = (UInt8)layer->get(i, j);
					if (cel == Scene::INVALID_ACTOR())
						continue;

					orderedActors.push_back(Ordered<int>(Math::Vec2i(i, j), cel));
				}
			}
			orderedActors.shrink_to_fit();
			std::sort(
				orderedActors.begin(), orderedActors.end(),
				[this, &actors] (const Ordered<int> &l, const Ordered<int> &r) -> bool {
					const int lv = l.value;
					const int rv = r.value;

					if (lv < rv) // Ordered by asset indices.
						return true;
					else if (lv > rv)
						return false;

					do {
						if (lv == rv)
							break;

						if (!_isPlayerBehaviour)
							break;

						const ActorAssets::Entry* lactor = actors.get(lv);
						const ActorAssets::Entry* ractor = actors.get(rv);
						const UInt8 lbhvr = lactor->definition.behaviour;
						const UInt8 rbhvr = ractor->definition.behaviour;
						const bool lisplayer = _isPlayerBehaviour(lbhvr);
						const bool risplayer = _isPlayerBehaviour(rbhvr);

						if (lisplayer && !risplayer)
							return true;
						else if (!lisplayer && risplayer)
							return false;
					} while (false);

					if (l.position.y < r.position.y)
						return true;
					else if (l.position.y > r.position.y)
						return false;

					if (l.position.x < r.position.x)
						return true;
					else if (l.position.x > r.position.x)
						return false;

					return false;
				}
			);
		};

		const SceneAssets &scenes = assets->scenes;
		orderedActorsInScene.resize(scenes.count() + 1);
		orderedActorsInScene.shrink_to_fit();
		for (int i = 0; i < scenes.count(); ++i) {
			const SceneAssets::Entry* entry = scenes.get(i);
			Ordered<int>::Array &orderedActors = orderedActorsInScene[i];

			if (_workQueue) {
				_workQueue->push(
					WorkTaskFunction::create(
						std::bind(
							[sortActorsInScene] (WorkTask* /* task */, const SceneAssets::Entry* entry, AssetsBundle::ConstPtr assets, Ordered<int>::Array* orderedActors) -> uintptr_t { // On work thread.
								sortActorsInScene(entry->data->actorLayer(), assets->actors, *orderedActors);

								return 0;
							},
							std::placeholders::_1, entry, assets, &orderedActors
						),
						[] (WorkTask* /* task */, uintptr_t) -> void { // On main thread.
							// Do nothing.
						},
						[] (WorkTask* task, uintptr_t) -> void { // On main thread.
							task->disassociated(true);
						}
					)
				);
			} else {
				sortActorsInScene(entry->data->actorLayer(), assets->actors, orderedActors);
			}
		}
	}
	void await(void) const {
		if (!_workQueue)
			return;

		constexpr const int STEP = 10;
		while (!_workQueue->allProcessed()) {
			_workQueue->update();
			DateTime::sleep(STEP);
		}
	}

	template<typename T> void calculateHash(HashArray &hashed, const T &entries) const {
		for (int i = 0; i < entries.count(); ++i) {
			const int page = i;
			const typename T::Entry* entry = entries.get(page);

			if (page + 1 > (int)hashed.size())
				hashed.resize(page + 1);
			if (entry)
				hashed[page] = entry->hash();
		}
	}
	template<typename T> void checkDuplication(const HashArray &hashed, AssetsBundle::Categories category, const std::string &typeName, const T &entries) const {
		typedef std::vector<int> Indices;

		Indices duplicated;
		for (int i = 0; i < (int)hashed.size(); ++i) {
			const size_t hashl = hashed[i];

			for (int j = i + 1; j < (int)hashed.size(); ++j) {
				Indices::const_iterator it = std::find(duplicated.begin(), duplicated.end(), j);
				if (it != duplicated.end())
					continue;

				const size_t hashr = hashed[j];
				if (hashl != hashr) // The asset does not equal to another.
					continue;

				const typename T::Entry* entryl = entries.get(i);
				const typename T::Entry* entryr = entries.get(j);
				if (entryl->compare(*entryr) != 0) // The asset does not equal to another.
					continue;

				// The asset is a duplicate of another.
				const std::string msg = Text::format("{0} asset at page {1} is a duplicate of {2}.", { typeName, Text::toString(j), Text::toString(i) });
				_onError(msg, true, category, -1);

				duplicated.push_back(j);

				break;
			}
		}
	}
};

Pipeline::Resource::Resource() {
}

Pipeline::Resource::Resource(int b, int a, int s) : bank(b), address(a), size(s) {
}

Pipeline::Size::Size() {
}

Pipeline::Size &Pipeline::Size::operator += (const Size &other) {
	_font         += other._font;
	_code         += other._code;
	_tiles        += other._tiles;
	_map          += other._map;
	_music        += other._music;
	_sfx          += other._sfx;
	_actor        += other._actor;
	_scene        += other._scene;
	_sgbResources += other._sgbResources;

	return *this;
}

int Pipeline::Size::font(void) const {
	return _font;
}

void Pipeline::Size::addFont(int val) {
	_font += val;
}

int Pipeline::Size::code(void) const {
	return _code;
}

void Pipeline::Size::addCode(int val) {
	_code += val;
}

int Pipeline::Size::tiles(void) const {
	return _tiles;
}

void Pipeline::Size::addTiles(int val) {
	_tiles += val;
}

int Pipeline::Size::map(void) const {
	return _map;
}

void Pipeline::Size::addMap(int val) {
	_map += val;
}

int Pipeline::Size::music(void) const {
	return _music;
}

void Pipeline::Size::addMusic(int val) {
	_music += val;
}

int Pipeline::Size::sfx(void) const {
	return _sfx;
}

void Pipeline::Size::addSfx(int val) {
	_sfx += val;
}

int Pipeline::Size::actor(void) const {
	return _actor;
}

void Pipeline::Size::addActor(int val) {
	_actor += val;
}

int Pipeline::Size::scene(void) const {
	return _scene;
}

void Pipeline::Size::addScene(int val) {
	_scene += val;
}

int Pipeline::Size::sgbResources(void) const {
	return _sgbResources;
}

void Pipeline::Size::addSgbResources(int val) {
	_sgbResources += val;
}

int Pipeline::Size::total(void) const {
	return
		_font +
		_code +
		_tiles +
		_map +
		_music +
		_sfx +
		_actor +
		_scene +
		_sgbResources;
}

std::string Pipeline::Size::toString(int indent) const {
	std::string ret;
	const std::string indent_ = Text::repeat(" ", indent);

	const bool hasSgbResources = sgbResources() > 0;

	const std::string sfont  = Text::toScaledBytes(font());
	const std::string scode  = Text::toScaledBytes(code());
	const std::string stiles = Text::toScaledBytes(tiles());
	const std::string smap   = Text::toScaledBytes(map());
	const std::string smusic = Text::toScaledBytes(music());
	const std::string ssfx   = Text::toScaledBytes(sfx());
	const std::string sactor = Text::toScaledBytes(actor());
	const std::string sscene = Text::toScaledBytes(scene());
	const std::string rsgb   = hasSgbResources ? Text::toScaledBytes(sgbResources()) : "";
	int maxs = (int)Math::max(
		sfont.length(),
		scode.length(),
		stiles.length(),
		smap.length(),
		smusic.length(),
		ssfx.length(),
		sactor.length(),
		sscene.length()
	);
	if (hasSgbResources) {
		maxs = (int)Math::max((size_t)maxs, rsgb.length());
	}

	int pfont  = 0;
	int pcode  = 0;
	int ptiles = 0;
	int pmap   = 0;
	int pmusic = 0;
	int psfx   = 0;
	int pactor = 0;
	int pscene = 0;
	int psgb   = 0;
	if (total() > 0) {
		 pfont   =         font() * 100 / total();
		 pcode   =         code() * 100 / total();
		ptiles   =        tiles() * 100 / total();
		  pmap   =          map() * 100 / total();
		pmusic   =        music() * 100 / total();
		  psfx   =          sfx() * 100 / total();
		pactor   =        actor() * 100 / total();
		pscene   =        scene() * 100 / total();
		if (hasSgbResources)
			psgb = sgbResources() * 100 / total();
	}
	const std::string spfont  = Text::toString(pfont);
	const std::string spcode  = Text::toString(pcode);
	const std::string sptiles = Text::toString(ptiles);
	const std::string spmap   = Text::toString(pmap);
	const std::string spmusic = Text::toString(pmusic);
	const std::string spsfx   = Text::toString(psfx);
	const std::string spactor = Text::toString(pactor);
	const std::string spscene = Text::toString(pscene);
	const std::string spsgb   = Text::toString(psgb);
	const int maxp = (int)Math::max(spfont.length(), spcode.length(), sptiles.length(), spmap.length(), spmusic.length(), spsfx.length(), spactor.length(), spscene.length());

	if (hasSgbResources) {
		ret += indent_ + "        Code size: " + scode  + ", " + Text::repeat(" ", maxs - (int)scode.length())  + "~" + Text::repeat(" ", maxp - (int)spcode.length())  + Text::toString(pcode)  + "%\n";
		ret += indent_ + "       Tiles size: " + stiles + ", " + Text::repeat(" ", maxs - (int)stiles.length()) + "~" + Text::repeat(" ", maxp - (int)sptiles.length()) + Text::toString(ptiles) + "%\n";
		ret += indent_ + "         Map size: " + smap   + ", " + Text::repeat(" ", maxs - (int)smap.length())   + "~" + Text::repeat(" ", maxp - (int)spmap.length())   + Text::toString(pmap)   + "%\n";
		ret += indent_ + "       Scene size: " + sscene + ", " + Text::repeat(" ", maxs - (int)sscene.length()) + "~" + Text::repeat(" ", maxp - (int)spscene.length()) + Text::toString(pscene) + "%\n";
		ret += indent_ + "       Actor size: " + sactor + ", " + Text::repeat(" ", maxs - (int)sactor.length()) + "~" + Text::repeat(" ", maxp - (int)spactor.length()) + Text::toString(pactor) + "%\n";
		ret += indent_ + "        Font size: " + sfont  + ", " + Text::repeat(" ", maxs - (int)sfont.length())  + "~" + Text::repeat(" ", maxp - (int)spfont.length())  + Text::toString(pfont)  + "%\n";
		ret += indent_ + "       Music size: " + smusic + ", " + Text::repeat(" ", maxs - (int)smusic.length()) + "~" + Text::repeat(" ", maxp - (int)spmusic.length()) + Text::toString(pmusic) + "%\n";
		ret += indent_ + "         SFX size: " + ssfx   + ", " + Text::repeat(" ", maxs - (int)ssfx.length())   + "~" + Text::repeat(" ", maxp - (int)spsfx.length())   + Text::toString(psfx)   + "%\n";
		ret += indent_ + "SGB resource size: " + rsgb   + ", " + Text::repeat(" ", maxs - (int)rsgb.length())   + "~" + Text::repeat(" ", maxp - (int)spsgb.length())   + Text::toString(psgb)   + "%";
	} else {
		ret += indent_ + " Code size: " + scode  + ", " + Text::repeat(" ", maxs - (int)scode.length())  + "~" + Text::repeat(" ", maxp - (int)spcode.length())  + Text::toString(pcode)  + "%\n";
		ret += indent_ + "Tiles size: " + stiles + ", " + Text::repeat(" ", maxs - (int)stiles.length()) + "~" + Text::repeat(" ", maxp - (int)sptiles.length()) + Text::toString(ptiles) + "%\n";
		ret += indent_ + "  Map size: " + smap   + ", " + Text::repeat(" ", maxs - (int)smap.length())   + "~" + Text::repeat(" ", maxp - (int)spmap.length())   + Text::toString(pmap)   + "%\n";
		ret += indent_ + "Scene size: " + sscene + ", " + Text::repeat(" ", maxs - (int)sscene.length()) + "~" + Text::repeat(" ", maxp - (int)spscene.length()) + Text::toString(pscene) + "%\n";
		ret += indent_ + "Actor size: " + sactor + ", " + Text::repeat(" ", maxs - (int)sactor.length()) + "~" + Text::repeat(" ", maxp - (int)spactor.length()) + Text::toString(pactor) + "%\n";
		ret += indent_ + " Font size: " + sfont  + ", " + Text::repeat(" ", maxs - (int)sfont.length())  + "~" + Text::repeat(" ", maxp - (int)spfont.length())  + Text::toString(pfont)  + "%\n";
		ret += indent_ + "Music size: " + smusic + ", " + Text::repeat(" ", maxs - (int)smusic.length()) + "~" + Text::repeat(" ", maxp - (int)spmusic.length()) + Text::toString(pmusic) + "%\n";
		ret += indent_ + "  SFX size: " + ssfx   + ", " + Text::repeat(" ", maxs - (int)ssfx.length())   + "~" + Text::repeat(" ", maxp - (int)spsfx.length())   + Text::toString(psfx)   + "%";
	}

	return ret;
}

Pipeline* Pipeline::create(bool useWorkQueue, bool lessConsoleOutput) {
	PipelineImpl* result = new PipelineImpl(useWorkQueue, lessConsoleOutput);

	return result;
}

void Pipeline::destroy(Pipeline* ptr) {
	PipelineImpl* impl = static_cast<PipelineImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
