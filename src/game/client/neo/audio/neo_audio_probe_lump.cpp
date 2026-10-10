#include "neo_audio_probe_lump.h"

#include <cstdio>
#include <cstring>

namespace NeoSpatial
{

namespace
{

template <typename T>
void AppendArray(std::vector<uint8_t> &out, const T *values, size_t count)
{
	const uint8_t *bytes = reinterpret_cast<const uint8_t *>(values);
	out.insert(out.end(), bytes, bytes + count * sizeof(T));
}

template <typename T>
bool ReadArray(const uint8_t *data, int64_t size, int64_t &offset, std::vector<T> &out, int64_t count)
{
	if (count < 0 || count > (size - offset) / static_cast<int64_t>(sizeof(T)))
	{
		return false;
	}
	out.resize(static_cast<size_t>(count));
	memcpy(out.data(), data + offset, static_cast<size_t>(count) * sizeof(T));
	offset += count * static_cast<int64_t>(sizeof(T));
	return true;
}

} // namespace

void WriteProbeLump(const ProbeSet &probes, const uint8_t *batches, int64_t batchesSize, uint32_t shapeCrc,
					uint32_t libraryVersion, std::vector<uint8_t> &out)
{
	ProbeLumpHeader header = {};
	header.magic = PROBE_LUMP_MAGIC;
	header.formatVersion = PROBE_LUMP_FORMAT_VERSION;
	header.shapeCrc = shapeCrc;
	header.libraryVersion = libraryVersion;
	header.radius = probes.radius;
	header.numProbes = static_cast<int32_t>(probes.centres.size());
	header.numBatches = probes.NumBatches();
	header.batchesSize = batchesSize;

	AppendArray(out, &header, 1);
	AppendArray(out, probes.centres.data(), probes.centres.size());
	AppendArray(out, probes.batchStarts.data(), probes.batchStarts.size());
	AppendArray(out, probes.batchAreas.data(), probes.batchAreas.size());
	AppendArray(out, batches, static_cast<size_t>(batchesSize));
}

bool ReadProbeLump(const uint8_t *data, int64_t size, ProbeLumpHeader &header, ProbeSet &probes,
				   const uint8_t *&batches, char *errorOut, int errorLen)
{
	if (size < static_cast<int64_t>(sizeof(header)))
	{
		snprintf(errorOut, errorLen, "probe lump is truncated");
		return false;
	}
	memcpy(&header, data, sizeof(header));
	if (header.magic != PROBE_LUMP_MAGIC || header.formatVersion != PROBE_LUMP_FORMAT_VERSION)
	{
		snprintf(errorOut, errorLen, "probe lump has an unknown format (version %u)", header.formatVersion);
		return false;
	}

	probes = ProbeSet();
	probes.radius = header.radius;
	int64_t offset = sizeof(header);
	if (header.numProbes < 0 || header.numBatches < 0
		|| !ReadArray(data, size, offset, probes.centres, header.numProbes)
		|| !ReadArray(data, size, offset, probes.batchStarts, static_cast<int64_t>(header.numBatches) + 1)
		|| !ReadArray(data, size, offset, probes.batchAreas, header.numBatches)
		|| header.batchesSize < 0 || header.batchesSize != size - offset)
	{
		snprintf(errorOut, errorLen, "probe lump is malformed");
		return false;
	}

	// Batches must tile the probes in order, or a batch would index outside them.
	for (int32_t b = 0; b < header.numBatches; ++b)
	{
		if (probes.batchStarts[b] < 0 || probes.batchStarts[b] > probes.batchStarts[b + 1])
		{
			snprintf(errorOut, errorLen, "probe lump has malformed batches");
			return false;
		}
	}
	if (probes.batchStarts.front() != 0 || probes.batchStarts.back() != header.numProbes)
	{
		snprintf(errorOut, errorLen, "probe lump has malformed batches");
		return false;
	}

	batches = data + offset;
	return true;
}

} // namespace NeoSpatial
