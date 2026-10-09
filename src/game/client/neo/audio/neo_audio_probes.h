// NEO HRTF: probe placement for baked acoustics. Probes come from the spatializer's own floor
// generator, run once per BSP leaf, and are batched by the BSP area of the leaf that contains
// them: one batch per region sealed off by areaportals, usually one or two per map, which is also
// the scope pathing needs (paths only join probes in the same batch). Plain C++ with no Source SDK
// dependency (like the backend), so it can be exercised on map data offline.
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "neo_spatializer.h"

namespace NeoSpatial
{

// The BSP tree, in the same frame and units as positions (Source axes, metres).
struct BspTree
{
	struct Plane
	{
		Vec3 normal;
		float dist;
	};
	struct Node
	{
		int32_t plane;
		int32_t children[2]; // front, back; negative = -(leaf + 1)
	};
	struct Leaf
	{
		Vec3 mins;
		Vec3 maxs;
		int32_t area; // BSP area, 0 for solid leaves
		bool open; // a listener can be here: not solid, and inside the map (has a vis cluster)
	};

	std::vector<Plane> planes;
	std::vector<Node> nodes;
	std::vector<Leaf> leaves;

	// The leaf containing `point`, or -1 if the tree is malformed.
	int FindLeaf(const Vec3 &point) const;
};

struct ProbeSettings
{
	float spacing; // between neighbouring probes
	float height; // above the floor
};

// Steam Audio's own defaults, about a stride apart at roughly ear height (a standing player's eyes
// are ~1.6 m up). Shared by the game and the compile-time baker, so both place the same probes.
static constexpr ProbeSettings DEFAULT_PROBE_SETTINGS = { 2.0f, 1.5f };

struct ProbeSet
{
	std::vector<Vec3> centres; // grouped by batch
	std::vector<int32_t> batchStarts; // numBatches + 1
	std::vector<int32_t> batchAreas; // the BSP area each batch covers, used as its batch id
	float radius = 0.0f;

	int NumBatches() const { return static_cast<int>(batchAreas.size()); }
	ProbeLayout Layout() const;
};

// Which points lie inside some probe's sphere of influence. Steam Audio only recomputes a voice's
// paths when both the voice and the listener are covered; otherwise it silently returns the last
// ones, which would keep a sound audible at whatever level it had when last covered.
class ProbeCoverage
{
public:
	void Build(const ProbeSet &probes);
	bool Covers(const Vec3 &point) const;

	// `point` itself if covered; otherwise the point just inside the sphere of the nearest probe
	// within maxDistance, on the way from that probe towards `point`. False if no probe is that close.
	bool PullInside(const Vec3 &point, float maxDistance, Vec3 &out) const;

private:
	float m_radius = 0.0f;
	std::unordered_map<int64_t, std::vector<Vec3>> m_cells; // cells one radius wide
};

// Generates floor probes in every open leaf and batches each probe by the BSP area of the open
// leaf it ends up in.
// The generator centres its grid in each box, so neighbouring leaves can place probes almost on
// top of each other; any probe closer than 3/4 of the spacing to one already kept is dropped,
// which keeps the spacing close to even across leaf boundaries.
void BuildLeafProbes(ISpatializer &spatializer, const BspTree &tree, const ProbeSettings &settings, ProbeSet &out);

} // namespace NeoSpatial
