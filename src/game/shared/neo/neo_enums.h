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

	// NOTENOTE: VIP *must* be last, because we are
	// using array offsets for recon/assault/support
	NEO_CLASS_VIP,

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

