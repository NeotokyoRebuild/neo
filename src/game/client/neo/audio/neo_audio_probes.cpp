#include "neo_audio_probes.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <tuple>
#include <unordered_map>

namespace NeoSpatial
{

namespace
{

// Probes closer than this (a crest probe and the floor probe on top of a narrow wall, or two crest
// probes at a corner) mostly duplicate each other.
constexpr float PROBE_MIN_SEPARATION = 0.5f; // of the spacing

// Floors: surfaces at most ~45 degrees from flat, like Source's walkable slope.
constexpr float FLOOR_MIN_NORMAL_Z = 0.7f;
// Less room than this above a floor (e.g. under a detail brush or prop resting on it) gets no probe.
constexpr float FLOOR_MIN_HEADROOM = 0.5f;
// Ray hits closer than this are the same surface (shared edges, an object resting on a floor).
constexpr float COINCIDENT_METRES = 0.01f;

// A top edge is where a steep face hangs down from an edge (its in-face direction away from the
// edge points at least this far down) and nothing sharing the edge carries on upwards.
constexpr float CREST_FACE_MAX_DOWN_Z = -0.7f;
constexpr float CREST_NEIGHBOUR_MAX_UP_Z = 0.3f;
// Only crests this far above the floor in front of them; lower walls are covered by floor probes.
constexpr float CREST_MIN_DROP_METRES = 2.0f;
// Only crests at least this long: sound goes around anything narrower (posts, signs, pillars).
constexpr float CREST_MIN_LENGTH_METRES = 2.0f;
// Crest probes go this far apart along an edge, in probe spacings: a probe covers 2 spacings of
// edge, which is all paths over it need.
constexpr float CREST_SPACING = 2.0f;
// Crest probes sit this far out from the face and above the edge.
constexpr float CREST_OFFSET_METRES = 0.4f;
// Probing just behind the face, above the edge, tells a real top edge (open space above the wall)
// from a seam where the wall carries on upwards as another, unconnected face or hull.
constexpr float CREST_BEHIND_METRES = 0.05f;
constexpr float CREST_ABOVE_METRES = 0.1f;

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

Vec3 Sub(const Vec3 &a, const Vec3 &b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 Cross(const Vec3 &a, const Vec3 &b)
{
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

float Dot(const Vec3 &a, const Vec3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct ColumnHit
{
	float z;
	float normalZ; // of the surface's unit normal, which points to its open side
};

// The mesh's triangles bucketed by the grid cells their footprints overlap, so a vertical ray only
// tests the few triangles over its own cell.
class CMeshColumns
{
public:
	struct TopEdge
	{
		Vec3 a, b;
		Vec3 out; // horizontal unit vector out of the face below the edge
	};

	CMeshColumns(const SceneGeometry &geometry, float cellSize)
		: m_geometry(geometry)
		, m_cellSize(cellSize)
	{
		for (int t = 0; t < geometry.numTriangles; ++t)
		{
			// Edge-on to a vertical ray: a wall never bounds a column from above or below.
			if (fabsf(Normal(t).z) < 1e-6f)
			{
				continue;
			}
			const Vec3 &a = Vertex(t, 0), &b = Vertex(t, 1), &c = Vertex(t, 2);
			const int64_t x0 = Cell(std::min({ a.x, b.x, c.x })), x1 = Cell(std::max({ a.x, b.x, c.x }));
			const int64_t y0 = Cell(std::min({ a.y, b.y, c.y })), y1 = Cell(std::max({ a.y, b.y, c.y }));
			for (int64_t y = y0; y <= y1; ++y)
			{
				for (int64_t x = x0; x <= x1; ++x)
				{
					m_cells[CellKey(x, y, 0)].push_back(t);
				}
			}
			m_min[0] = std::min(m_min[0], x0);
			m_max[0] = std::max(m_max[0], x1);
			m_min[1] = std::min(m_min[1], y0);
			m_max[1] = std::max(m_max[1], y1);
		}
	}

	int64_t MinCell(int axis) const { return m_min[axis]; }
	int64_t MaxCell(int axis) const { return m_max[axis]; }

	// Every surface a vertical line through (x, y) crosses, bottom to top.
	void Cast(float x, float y, std::vector<ColumnHit> &hits) const
	{
		hits.clear();
		const auto it = m_cells.find(CellKey(Cell(x), Cell(y), 0));
		if (it == m_cells.end())
		{
			return;
		}
		for (const int32_t t : it->second)
		{
			const Vec3 &a = Vertex(t, 0), &b = Vertex(t, 1), &c = Vertex(t, 2);
			// Barycentric weights in the xy plane; a line through a shared edge hits both sides.
			const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
			const float wa = ((b.x - x) * (c.y - y) - (b.y - y) * (c.x - x)) / area;
			const float wb = ((c.x - x) * (a.y - y) - (c.y - y) * (a.x - x)) / area;
			const float wc = 1.0f - wa - wb;
			constexpr float kOnEdge = -1e-5f;
			if (wa < kOnEdge || wb < kOnEdge || wc < kOnEdge)
			{
				continue;
			}
			const Vec3 normal = Normal(t);
			hits.push_back({ wa * a.z + wb * b.z + wc * c.z, normal.z / sqrtf(Dot(normal, normal)) });
		}
		std::sort(hits.begin(), hits.end(), [](const ColumnHit &l, const ColumnHit &r) { return l.z < r.z; });
	}

	// Whether the nearest surface above `point` faces up: `point` is then inside something closed,
	// such as a prop's hull.
	bool UnderUpwardFace(const Vec3 &point, std::vector<ColumnHit> &hits) const
	{
		Cast(point.x, point.y, hits);
		for (const ColumnHit &hit : hits)
		{
			if (hit.z > point.z)
			{
				return hit.normalZ > 0.0f;
			}
		}
		return false;
	}

	// Top edges of steep faces, as straight runs: the face hangs down from the edge, and whatever
	// shares the edge (a wall top, the far face of a thin wall, or nothing at all for a nodraw top)
	// does not carry on upwards. The diagonals shared by a face's own triangles fail the second test.
	std::vector<TopEdge> FindTopEdges() const
	{
		struct Side
		{
			int32_t triangle;
			int edge;
			float inwardZ;
		};
		std::unordered_map<EdgeKey, std::vector<Side>, EdgeKeyHash> edges;
		for (int t = 0; t < m_geometry.numTriangles; ++t)
		{
			for (int e = 0; e < 3; ++e)
			{
				edges[MakeEdgeKey(Vertex(t, e), Vertex(t, (e + 1) % 3))].push_back({ t, e, InwardZ(t, e) });
			}
		}

		std::vector<TopEdge> top;
		for (const auto &entry : edges)
		{
			bool bCarriesOn = false;
			for (const Side &side : entry.second)
			{
				bCarriesOn |= (side.inwardZ > CREST_NEIGHBOUR_MAX_UP_Z);
			}
			if (bCarriesOn)
			{
				continue;
			}
			for (const Side &side : entry.second)
			{
				const Vec3 normal = Normal(side.triangle);
				const float horizontal = sqrtf(normal.x * normal.x + normal.y * normal.y);
				if (side.inwardZ > CREST_FACE_MAX_DOWN_Z || horizontal <= 0.0f)
				{
					continue;
				}
				top.push_back({ Vertex(side.triangle, side.edge), Vertex(side.triangle, (side.edge + 1) % 3),
								{ normal.x / horizontal, normal.y / horizontal, 0.0f } });
			}
		}
		// Hash map order differs between standard libraries; sort so the game and the baker always
		// place the same probes.
		std::sort(top.begin(), top.end(), [](const TopEdge &l, const TopEdge &r) {
			return std::tie(l.a.x, l.a.y, l.a.z, l.b.x, l.b.y, l.b.z, l.out.x, l.out.y)
				   < std::tie(r.a.x, r.a.y, r.a.z, r.b.x, r.b.y, r.b.z, r.out.x, r.out.y);
		});
		return MergeRuns(top);
	}

private:
	// Joins top edges that carry on from each other in a straight line, facing the same way: brush
	// faces are cut at brush and leaf boundaries, so one wall top usually arrives in pieces.
	static std::vector<TopEdge> MergeRuns(const std::vector<TopEdge> &edges)
	{
		std::vector<size_t> parent(edges.size());
		for (size_t i = 0; i < parent.size(); ++i)
		{
			parent[i] = i;
		}
		const auto root = [&parent](size_t i) {
			while (parent[i] != i)
			{
				i = parent[i] = parent[parent[i]];
			}
			return i;
		};
		const auto direction = [](const TopEdge &edge) {
			const Vec3 along = Sub(edge.b, edge.a);
			const float length = sqrtf(Dot(along, along));
			return (length > 0.0f) ? Vec3{ along.x / length, along.y / length, along.z / length } : Vec3{ 0, 0, 0 };
		};
		const auto pointKey = [](const Vec3 &p) { return CellKey(Millimetres(p.x), Millimetres(p.y), Millimetres(p.z)); };

		std::unordered_map<int64_t, std::vector<size_t>> byEndpoint;
		for (size_t i = 0; i < edges.size(); ++i)
		{
			const Vec3 dir = direction(edges[i]);
			for (const Vec3 *pEnd : { &edges[i].a, &edges[i].b })
			{
				std::vector<size_t> &touching = byEndpoint[pointKey(*pEnd)];
				for (const size_t j : touching)
				{
					if (Dot(edges[i].out, edges[j].out) > 0.99f && fabsf(Dot(dir, direction(edges[j]))) > 0.999f)
					{
						parent[root(i)] = root(j);
					}
				}
				touching.push_back(i);
			}
		}

		// Each run spans its pieces' extent along the first piece's line.
		std::vector<TopEdge> runs;
		std::vector<float> extent; // min, max along the line per run
		std::unordered_map<size_t, size_t> runOfRoot;
		for (size_t i = 0; i < edges.size(); ++i)
		{
			const auto inserted = runOfRoot.emplace(root(i), runs.size());
			if (inserted.second)
			{
				runs.push_back(edges[i]);
				extent.insert(extent.end(), { FLT_MAX, -FLT_MAX });
			}
			const size_t run = inserted.first->second;
			const Vec3 dir = direction(runs[run]);
			for (const Vec3 *pEnd : { &edges[i].a, &edges[i].b })
			{
				const float t = Dot(Sub(*pEnd, runs[run].a), dir);
				extent[run * 2] = std::min(extent[run * 2], t);
				extent[run * 2 + 1] = std::max(extent[run * 2 + 1], t);
			}
		}
		for (size_t run = 0; run < runs.size(); ++run)
		{
			const Vec3 origin = runs[run].a, dir = direction(runs[run]);
			const float t0 = extent[run * 2], t1 = extent[run * 2 + 1];
			runs[run].a = { origin.x + dir.x * t0, origin.y + dir.y * t0, origin.z + dir.z * t0 };
			runs[run].b = { origin.x + dir.x * t1, origin.y + dir.y * t1, origin.z + dir.z * t1 };
		}
		return runs;
	}

	struct EdgeKey
	{
		int32_t v[6]; // both endpoints in millimetres, the lower one first
		bool operator==(const EdgeKey &other) const { return std::equal(v, v + 6, other.v); }
	};
	struct EdgeKeyHash
	{
		size_t operator()(const EdgeKey &key) const
		{
			uint64_t hash = 1469598103934665603ull; // FNV-1a
			for (const int32_t value : key.v)
			{
				hash = (hash ^ static_cast<uint32_t>(value)) * 1099511628211ull;
			}
			return static_cast<size_t>(hash);
		}
	};

	static int32_t Millimetres(float metres) { return static_cast<int32_t>(lroundf(metres * 1000.0f)); }
	static EdgeKey MakeEdgeKey(const Vec3 &a, const Vec3 &b)
	{
		const int32_t qa[3] = { Millimetres(a.x), Millimetres(a.y), Millimetres(a.z) };
		const int32_t qb[3] = { Millimetres(b.x), Millimetres(b.y), Millimetres(b.z) };
		const bool bSwap = std::lexicographical_compare(qb, qb + 3, qa, qa + 3);
		const int32_t *first = bSwap ? qb : qa, *second = bSwap ? qa : qb;
		return { { first[0], first[1], first[2], second[0], second[1], second[2] } };
	}

	int64_t Cell(float coordinate) const { return static_cast<int64_t>(floorf(coordinate / m_cellSize)); }
	const Vec3 &Vertex(int t, int corner) const { return m_geometry.vertices[m_geometry.triangles[t * 3 + corner]]; }
	Vec3 Normal(int t) const { return Cross(Sub(Vertex(t, 1), Vertex(t, 0)), Sub(Vertex(t, 2), Vertex(t, 0))); }

	// The z of the unit direction, in the triangle's plane, from edge e towards the opposite corner.
	float InwardZ(int t, int e) const
	{
		const Vec3 &a = Vertex(t, e), &b = Vertex(t, (e + 1) % 3), &c = Vertex(t, (e + 2) % 3);
		const Vec3 along = Sub(b, a);
		const float lengthSqr = Dot(along, along);
		if (lengthSqr <= 0.0f)
		{
			return 0.0f;
		}
		const Vec3 toC = Sub(c, a);
		const float k = Dot(toC, along) / lengthSqr;
		const Vec3 inward = { toC.x - along.x * k, toC.y - along.y * k, toC.z - along.z * k };
		const float inwardLength = sqrtf(Dot(inward, inward));
		return (inwardLength > 0.0f) ? inward.z / inwardLength : 0.0f;
	}

	const SceneGeometry &m_geometry;
	float m_cellSize;
	std::unordered_map<int64_t, std::vector<int32_t>> m_cells;
	int64_t m_min[2] = { INT64_MAX, INT64_MAX };
	int64_t m_max[2] = { INT64_MIN, INT64_MIN };
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

void BuildProbes(const SceneGeometry &geometry, const BspTree &tree, const ProbeSettings &settings, ProbeSet &out)
{
	out = ProbeSet();
	out.radius = settings.spacing;
	if (geometry.numTriangles <= 0 || settings.spacing <= 0.0f)
	{
		out.batchStarts.push_back(0);
		return;
	}

	const CMeshColumns columns(geometry, settings.spacing);

	struct Placed
	{
		int32_t area;
		Vec3 centre;
	};
	std::vector<Placed> placed;
	CProbeSpacing spacing(settings.spacing * PROBE_MIN_SEPARATION);
	const auto openLeaf = [&](const Vec3 &point) {
		const int leaf = tree.FindLeaf(point);
		return (leaf >= 0 && tree.leaves[leaf].open) ? leaf : -1;
	};
	const auto tryPlace = [&](const Vec3 &centre) {
		const int leaf = openLeaf(centre);
		if (leaf >= 0 && spacing.TryAdd(centre))
		{
			placed.push_back({ tree.leaves[leaf].area, centre });
		}
	};

	// Crest probes first, so floor probes give way to them rather than the other way round.
	std::vector<ColumnHit> hits;
	for (const CMeshColumns::TopEdge &edge : columns.FindTopEdges())
	{
		const Vec3 along = Sub(edge.b, edge.a);
		const float length = sqrtf(Dot(along, along));
		if (length < CREST_MIN_LENGTH_METRES)
		{
			continue;
		}
		const int samples = std::max(1, static_cast<int>(lroundf(length / (settings.spacing * CREST_SPACING))));
		for (int i = 0; i < samples; ++i)
		{
			const float t = (i + 0.5f) / samples;
			const Vec3 point = { edge.a.x + along.x * t, edge.a.y + along.y * t, edge.a.z + along.z * t };
			const Vec3 behind = { point.x - edge.out.x * CREST_BEHIND_METRES, point.y - edge.out.y * CREST_BEHIND_METRES,
								  point.z + CREST_ABOVE_METRES };
			if (openLeaf(behind) < 0 || columns.UnderUpwardFace(behind, hits))
			{
				continue;
			}
			const Vec3 centre = { point.x + edge.out.x * CREST_OFFSET_METRES, point.y + edge.out.y * CREST_OFFSET_METRES,
								  point.z + CREST_OFFSET_METRES };
			columns.Cast(centre.x, centre.y, hits);
			float floor = -FLT_MAX;
			for (const ColumnHit &hit : hits)
			{
				if (hit.normalZ > 0.0f && hit.z < point.z - COINCIDENT_METRES)
				{
					floor = std::max(floor, hit.z);
				}
			}
			if (point.z - floor >= CREST_MIN_DROP_METRES)
			{
				tryPlace(centre);
			}
		}
	}

	// Floor probes, one per floor in every column.
	for (int64_t cy = columns.MinCell(1); cy <= columns.MaxCell(1); ++cy)
	{
		for (int64_t cx = columns.MinCell(0); cx <= columns.MaxCell(0); ++cx)
		{
			const float x = (cx + 0.5f) * settings.spacing;
			const float y = (cy + 0.5f) * settings.spacing;
			columns.Cast(x, y, hits);
			for (const ColumnHit &floor : hits)
			{
				if (floor.normalZ < FLOOR_MIN_NORMAL_Z)
				{
					continue;
				}
				// The nearest surface above bounds the room over the floor; a surface facing down at
				// the floor's own height is something resting on it, leaving no room at all.
				bool bCovered = false;
				float ceiling = FLT_MAX;
				for (const ColumnHit &other : hits)
				{
					if (fabsf(other.z - floor.z) < COINCIDENT_METRES)
					{
						bCovered |= (other.normalZ < 0.0f);
					}
					else if (other.z > floor.z)
					{
						ceiling = std::min(ceiling, other.z);
					}
				}
				const float headroom = ceiling - floor.z;
				if (!bCovered && headroom >= FLOOR_MIN_HEADROOM)
				{
					tryPlace({ x, y, floor.z + std::min(settings.height, headroom * 0.5f) });
				}
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
