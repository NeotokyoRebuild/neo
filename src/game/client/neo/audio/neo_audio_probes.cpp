#include "neo_audio_probes.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace NeoSpatial
{

namespace
{

// Leaf boxes are bounded by walls, and the generator puts its outermost columns on the box edges,
// so boxes are pulled in from the walls before generating.
constexpr float PROBE_WALL_INSET_METRES = 0.25f;
constexpr float PROBE_MIN_SEPARATION = 0.75f; // of the spacing

int64_t CellKey(int64_t x, int64_t y, int64_t z)
{
	constexpr int64_t bias = 1 << 20;
	constexpr int64_t mask = (1 << 21) - 1;
	return ((x + bias) & mask) | (((y + bias) & mask) << 21) | (((z + bias) & mask) << 42);
}

float DistanceSqr(const Vec3 &a, const Vec3 &b)
{
	const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
	return dx * dx + dy * dy + dz * dz;
}

// Probes already kept, bucketed in cells one minimum separation wide so only the 27 cells around
// a candidate need checking.
class CProbeSpacing
{
public:
	explicit CProbeSpacing(float minSeparation)
		: m_cellSize(minSeparation)
		, m_minSeparationSqr(minSeparation * minSeparation)
	{
	}

	bool TryAdd(const Vec3 &centre)
	{
		const int64_t cx = static_cast<int64_t>(floorf(centre.x / m_cellSize));
		const int64_t cy = static_cast<int64_t>(floorf(centre.y / m_cellSize));
		const int64_t cz = static_cast<int64_t>(floorf(centre.z / m_cellSize));
		for (int64_t x = cx - 1; x <= cx + 1; ++x)
		{
			for (int64_t y = cy - 1; y <= cy + 1; ++y)
			{
				for (int64_t z = cz - 1; z <= cz + 1; ++z)
				{
					const auto it = m_cells.find(CellKey(x, y, z));
					if (it == m_cells.end())
					{
						continue;
					}
					for (const Vec3 &kept : it->second)
					{
						if (DistanceSqr(kept, centre) < m_minSeparationSqr)
						{
							return false;
						}
					}
				}
			}
		}
		m_cells[CellKey(cx, cy, cz)].push_back(centre);
		return true;
	}

private:
	float m_cellSize;
	float m_minSeparationSqr;
	std::unordered_map<int64_t, std::vector<Vec3>> m_cells;
};

} // namespace

int BspTree::FindLeaf(const Vec3 &point) const
{
	int node = 0;
	// A well-formed tree reaches a leaf in fewer steps than it has nodes; this also stops a loop.
	for (size_t steps = 0; steps <= nodes.size(); ++steps)
	{
		if (node < 0)
		{
			const int leaf = -1 - node;
			return (leaf < static_cast<int>(leaves.size())) ? leaf : -1;
		}
		if (node >= static_cast<int>(nodes.size()) || nodes[node].plane < 0
			|| nodes[node].plane >= static_cast<int>(planes.size()))
		{
			return -1;
		}
		const Plane &plane = planes[nodes[node].plane];
		const float side = plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z - plane.dist;
		node = nodes[node].children[(side >= 0.0f) ? 0 : 1];
	}
	return -1;
}

void ProbeCoverage::Build(const ProbeSet &probes)
{
	m_radius = probes.radius;
	m_cells.clear();
	if (m_radius <= 0.0f)
	{
		return;
	}
	for (const Vec3 &centre : probes.centres)
	{
		m_cells[CellKey(static_cast<int64_t>(floorf(centre.x / m_radius)), static_cast<int64_t>(floorf(centre.y / m_radius)),
						static_cast<int64_t>(floorf(centre.z / m_radius)))].push_back(centre);
	}
}

bool ProbeCoverage::Covers(const Vec3 &point) const
{
	if (m_cells.empty())
	{
		return false;
	}
	const int64_t cx = static_cast<int64_t>(floorf(point.x / m_radius));
	const int64_t cy = static_cast<int64_t>(floorf(point.y / m_radius));
	const int64_t cz = static_cast<int64_t>(floorf(point.z / m_radius));
	const float radiusSqr = m_radius * m_radius;
	for (int64_t x = cx - 1; x <= cx + 1; ++x)
	{
		for (int64_t y = cy - 1; y <= cy + 1; ++y)
		{
			for (int64_t z = cz - 1; z <= cz + 1; ++z)
			{
				const auto it = m_cells.find(CellKey(x, y, z));
				if (it == m_cells.end())
				{
					continue;
				}
				for (const Vec3 &centre : it->second)
				{
					if (DistanceSqr(centre, point) <= radiusSqr)
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool ProbeCoverage::PullInside(const Vec3 &point, float maxDistance, Vec3 &out) const
{
	if (Covers(point))
	{
		out = point;
		return true;
	}
	if (m_cells.empty())
	{
		return false;
	}

	const int64_t reach = static_cast<int64_t>(ceilf(maxDistance / m_radius));
	const int64_t cx = static_cast<int64_t>(floorf(point.x / m_radius));
	const int64_t cy = static_cast<int64_t>(floorf(point.y / m_radius));
	const int64_t cz = static_cast<int64_t>(floorf(point.z / m_radius));
	float bestSqr = maxDistance * maxDistance;
	const Vec3 *pBest = nullptr;
	for (int64_t x = cx - reach; x <= cx + reach; ++x)
	{
		for (int64_t y = cy - reach; y <= cy + reach; ++y)
		{
			for (int64_t z = cz - reach; z <= cz + reach; ++z)
			{
				const auto it = m_cells.find(CellKey(x, y, z));
				if (it == m_cells.end())
				{
					continue;
				}
				for (const Vec3 &centre : it->second)
				{
					const float distSqr = DistanceSqr(centre, point);
					if (distSqr < bestSqr)
					{
						bestSqr = distSqr;
						pBest = &centre;
					}
				}
			}
		}
	}
	if (!pBest)
	{
		return false;
	}

	// Just inside the sphere, so float error cannot put it back outside.
	constexpr float kInside = 0.9f;
	const float scale = kInside * m_radius / sqrtf(bestSqr);
	out = { pBest->x + (point.x - pBest->x) * scale, pBest->y + (point.y - pBest->y) * scale,
			pBest->z + (point.z - pBest->z) * scale };
	return true;
}

ProbeLayout ProbeSet::Layout() const
{
	ProbeLayout layout;
	layout.centres = centres.data();
	layout.numProbes = static_cast<int>(centres.size());
	layout.radius = radius;
	layout.batchStarts = batchStarts.data();
	layout.batchIds = batchAreas.data();
	layout.numBatches = NumBatches();
	return layout;
}

void BuildLeafProbes(ISpatializer &spatializer, const BspTree &tree, const ProbeSettings &settings, ProbeSet &out)
{
	out = ProbeSet();
	out.radius = settings.spacing; // what the generator itself gives each probe

	struct Placed
	{
		int32_t area;
		Vec3 centre;
	};
	std::vector<Placed> placed;
	std::vector<Vec3> generated(64);
	CProbeSpacing spacing(settings.spacing * PROBE_MIN_SEPARATION);

	for (const BspTree::Leaf &leaf : tree.leaves)
	{
		if (!leaf.open)
		{
			continue;
		}
		Vec3 lo = leaf.mins;
		Vec3 hi = leaf.maxs;
		const float insetX = std::min(PROBE_WALL_INSET_METRES, (hi.x - lo.x) * 0.25f);
		const float insetY = std::min(PROBE_WALL_INSET_METRES, (hi.y - lo.y) * 0.25f);
		lo.x += insetX;
		hi.x -= insetX;
		lo.y += insetY;
		hi.y -= insetY;
		if (hi.x <= lo.x || hi.y <= lo.y || hi.z <= lo.z)
		{
			continue;
		}

		int count = spatializer.GenerateFloorProbes(lo, hi, settings.spacing, settings.height, generated.data(),
													static_cast<int>(generated.size()));
		if (count > static_cast<int>(generated.size()))
		{
			generated.resize(count);
			count = spatializer.GenerateFloorProbes(lo, hi, settings.spacing, settings.height, generated.data(), count);
		}

		// The generator looks for floors up to `height` below the box, so a probe can belong to the
		// leaf above or beside this one (possibly in another area); it is batched where it actually
		// is, and only in open space.
		for (int i = 0; i < count; ++i)
		{
			const int owner = tree.FindLeaf(generated[i]);
			if (owner >= 0 && tree.leaves[owner].open && spacing.TryAdd(generated[i]))
			{
				placed.push_back({ tree.leaves[owner].area, generated[i] });
			}
		}
	}

	std::stable_sort(placed.begin(), placed.end(), [](const Placed &a, const Placed &b) { return a.area < b.area; });
	out.centres.reserve(placed.size());
	for (size_t i = 0; i < placed.size(); ++i)
	{
		if (i == 0 || placed[i].area != placed[i - 1].area)
		{
			out.batchStarts.push_back(static_cast<int32_t>(i));
			out.batchAreas.push_back(placed[i].area);
		}
		out.centres.push_back(placed[i].centre);
	}
	out.batchStarts.push_back(static_cast<int32_t>(placed.size()));
}

} // namespace NeoSpatial
