#pragma once

#include "neo_crosshair.h"

namespace NeoSerial
{
	namespace V1
	{
		static constexpr char SEGEND = ';';
	}

	namespace V7
	{
		// The segment-end token was changed from ';' to ',' to avoid clashing with the statement-end token of consolecmds.
		static constexpr char SEGEND = ',';
	}
}

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
		const int iMin = 0, const int iMax = 0, NeoXHairSerial ver = NEOXHAIR_SERIAL_CURRENT);

[[nodiscard]] bool SerialBool(const bool bVal, const bool bCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx,
		NeoXHairSerial ver = NEOXHAIR_SERIAL_CURRENT);

[[nodiscard]] float SerialFloat(const float flVal, const float flCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx,
		const float flMin = 0.0f, const float flMax = 0.0f,
		NeoXHairSerial ver = NEOXHAIR_SERIAL_CURRENT);

// 2nd pass - Serialization run length encoding on empty
void SerialRLEncode(char (&szMutSeq)[NEO_XHAIR_SEQMAX], const ESerialMode eSerialMode,
	NeoXHairSerial ver = NEOXHAIR_SERIAL_CURRENT);

// Checks the contents of "pszSequence" to verify the segment-end delimiter character
// is compatible with the provided "ver" serial version.
// Returns false if the input sequence had any invalid delimiters for the version,
// and true otherwise.
// Will heap-allocate/free at most seqMax chars in the failure case for printing the message.
// Will return false if seqMax > NEO_XHAIR_SEQMAX or <= 0.
bool NagBadSegEnd(const char* pszContext, const char* pszSequence, int seqMax, NeoXHairSerial ver);
