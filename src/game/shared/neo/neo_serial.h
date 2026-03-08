#pragma once

// Requires neo_crosshair.h included before this for NEO_XHAIR_SEQMAX

enum ESerialMode
{
	SERIALMODE_DESERIALIZE = 0,
	SERIALMODE_SERIALIZE,
	SERIALMODE_CHECK, // SERIALMODE_DESERIALIZE but checks for bounds
};

// Value comparison mode
enum ECompMode
{
	COMPMODE_IGNORE = 0,

	// Serialize: Serial empty if it matches flCompVal
	// Deserialize: Deserialize take flCompVal if seen empty
	COMPMODE_EQUALS,
};

struct SerialContext
{
	ESerialMode eSerialMode;
	int iSeqSize;
	int idx;
	int iSkipIdx;
	bool bOutOfBound;
};

[[nodiscard]] int SerialInt(const int iVal, const int iCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx,
		const int iMin = 0, const int iMax = 0);

[[nodiscard]] bool SerialBool(const bool bVal, const bool bCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx);

[[nodiscard]] float SerialFloat(const float flVal, const float flCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx,
		const float flMin = 0.0f, const float flMax = 0.0f);

// 2nd pass - Serialization run length encoding on empty
void SerialRLEncode(char (&szMutSeq)[NEO_XHAIR_SEQMAX], const ESerialMode eSerialMode);

