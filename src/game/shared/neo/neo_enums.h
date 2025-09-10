#pragma once

enum NeoSkin {
	NEO_SKIN_FIRST = 0,
	NEO_SKIN_SECOND,
	NEO_SKIN_THIRD,

	NEO_SKIN__ENUM_COUNT
};
static constexpr int NEO_SKIN_ENUM_COUNT = NEO_SKIN__ENUM_COUNT;

enum NeoClass {
	NEO_CLASS_RECON = 0,
	NEO_CLASS_ASSAULT,
	NEO_CLASS_SUPPORT,

	// NOTENOTE: VIP and Juggernaut *must* be last, because we are
	// using array offsets for recon/assault/support
	NEO_CLASS_VIP,
	NEO_CLASS_JUGGERNAUT,

	NEO_CLASS__ENUM_COUNT
};
static constexpr int NEO_CLASS_ENUM_COUNT = NEO_CLASS__ENUM_COUNT;

enum NeoStar {
	STAR_NONE = 0,
	STAR_ALPHA,
	STAR_BRAVO,
	STAR_CHARLIE,
	STAR_DELTA,
	STAR_ECHO,
	STAR_FOXTROT,

	STAR__TOTAL
};
#define NEO_DEFAULT_STAR STAR_ALPHA

enum ENeoRank
{
	NEO_RANK_RANKLESS_DOG = -1,
	NEO_RANK_PRIVATE = 0,
	NEO_RANK_CORPORAL,
	NEO_RANK_SERGEANT,
	NEO_RANK_LIEUTENANT,

	NEO_RANK__TOTAL, // rankless dog doesn't count within the arrays
};
static const constexpr int NEO_RANK_TOTAL = NEO_RANK__TOTAL;

static const constexpr short NEO_ENVIRON_KILLED = -1;

