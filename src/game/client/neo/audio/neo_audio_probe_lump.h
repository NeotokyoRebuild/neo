// NEO HRTF: the game lump a map's baked acoustic probes are stored in. The compile-time baker
// (utils/neo_soundbake) writes it into the BSP's game lump directory (lump 35); the client loads it
// instead of baking at runtime. Plain C++ with no Source SDK dependency, so both sides share it.
//
// Layout, little endian, before compression:
//   ProbeLumpHeader
//   Vec3    centres[numProbes]          probe layout, so the client never has to regenerate it
//   int32_t batchStarts[numBatches + 1]
//   int32_t batchAreas[numBatches]
//   uint8_t batches[batchesSize]        ISpatializer::SerializeProbeBatches
// The baker LZMA-compresses the whole payload in Valve's lzma_header_t format (the client's
// CLZMA reads it); ReadProbeLump expects it already decompressed.
#pragma once

#include <cstdint>
#include <vector>

#include "neo_audio_probes.h"

namespace NeoSpatial
{

// Game lump ids are four-character codes, as Valve's 'sprp' (static props).
static constexpr int32_t PROBE_GAME_LUMP_ID = ('n' << 24) | ('s' << 16) | ('a' << 8) | 'p';
static constexpr uint16_t PROBE_GAME_LUMP_VERSION = 1;

struct ProbeLumpHeader
{
	uint32_t magic;
	uint32_t formatVersion;
	uint32_t shapeCrc; // CNeoAudioGeometry::GetShapeCrc of the map that was baked
	uint32_t libraryVersion; // ISpatializer::GetLibraryVersion that baked it, for diagnostics
	float radius;
	int32_t numProbes;
	int32_t numBatches;
	int32_t reserved;
	int64_t batchesSize;
};

static constexpr uint32_t PROBE_LUMP_MAGIC = 0x4c50534e; // "NSPL"
static constexpr uint32_t PROBE_LUMP_FORMAT_VERSION = 1;

// Appends the uncompressed payload for `probes` and their serialised batches to `out`.
void WriteProbeLump(const ProbeSet &probes, const uint8_t *batches, int64_t batchesSize, uint32_t shapeCrc,
					uint32_t libraryVersion, std::vector<uint8_t> &out);

// Parses an uncompressed payload. On success `probes` holds the layout and `batches` points into
// `data` (valid as long as it is). False, with the reason in errorOut, for anything malformed.
bool ReadProbeLump(const uint8_t *data, int64_t size, ProbeLumpHeader &header, ProbeSet &probes,
				   const uint8_t *&batches, char *errorOut, int errorLen);

} // namespace NeoSpatial
