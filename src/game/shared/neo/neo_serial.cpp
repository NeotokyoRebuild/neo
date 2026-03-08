#include "neo_crosshair.h"
#include "neo_serial.h"

#include "strtools.h"
#include "mathlib/mathlib.h"

static constexpr char CH_XH_SEGEND = ';';
static constexpr char CH_XH_SEGSKIP = '^';

union SerialVariant
{
	int iVal;
	float flVal;
	bool bVal;
};

enum ESerialVariantType
{
	SERIALVARIANTTYPE_INT = 0,
	SERIALVARIANTTYPE_FLOAT,
	SERIALVARIANTTYPE_BOOL,
};

static void StrCatCh(char (&szMutStr)[NEO_XHAIR_SEQMAX], const char ch)
{
	const int iSize = V_strlen(szMutStr);
	if ((iSize + 1) < NEO_XHAIR_SEQMAX)
	{
		szMutStr[iSize] = ch;
		szMutStr[iSize + 1] = '\0';
	}
}

static SerialVariant DeserialVariant(char (&szMutStr)[NEO_XHAIR_SEQMAX],
		const ESerialVariantType eType, const SerialVariant varDefault,
		const SerialVariant varMin, const SerialVariant varMax, SerialContext *ctx)
{
	SerialVariant var = varDefault;

	if (ctx->iSkipIdx > 0)
	{
		--(ctx->iSkipIdx);
		return var;
	}

	int *idx = &ctx->idx;
	const int iPrevSegment = *idx;
	Assert(ctx->iSeqSize <= NEO_XHAIR_SEQMAX);

	for (; *idx < ctx->iSeqSize; ++(*idx))
	{
		const char endCh = szMutStr[*idx];
		if (endCh == CH_XH_SEGEND || endCh == CH_XH_SEGSKIP)
		{
			szMutStr[*idx] = '\0';
			const char *pszCurSegment = szMutStr + iPrevSegment;
			if (pszCurSegment[0] != '\0')
			{
				if (endCh == CH_XH_SEGEND) // Value
				{
					switch (eType)
					{
					case SERIALVARIANTTYPE_INT:
					{
						const int iInitVal = V_atoi(pszCurSegment);
						if (SERIALMODE_CHECK == ctx->eSerialMode && varMin.iVal < varMax.iVal)
						{
							ctx->bOutOfBound = (iInitVal > varMax.iVal || iInitVal < varMin.iVal);
						}
						var.iVal = (varMin.iVal < varMax.iVal) ? clamp(iInitVal, varMin.iVal, varMax.iVal) : iInitVal;
					} break;
					case SERIALVARIANTTYPE_BOOL:
					{
						var.bVal = (V_atoi(pszCurSegment) != 0);
					} break;
					case SERIALVARIANTTYPE_FLOAT:
					{
						const float flInitVal = static_cast<float>(V_atof(pszCurSegment));
						if (SERIALMODE_CHECK == ctx->eSerialMode && varMin.flVal < varMax.flVal)
						{
							ctx->bOutOfBound = (flInitVal > varMax.flVal || flInitVal < varMin.flVal);
						}
						var.flVal = (varMin.flVal < varMax.flVal) ? clamp(flInitVal, varMin.flVal, varMax.flVal) : flInitVal;
					} break;
					}
				}
				else if (endCh == CH_XH_SEGSKIP) // Run-length encoding skip
				{
					const int iSkipLen = V_atoi(pszCurSegment);
					if (iSkipLen > 1)
					{
						ctx->iSkipIdx = iSkipLen - 1;
					}
				}
			}
			++(*idx);
			break;
		}
	}

	return var;
}

[[nodiscard]] int SerialInt(const int iVal, const int iCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx,
		const int iMin, const int iMax)
{
	if (ctx->eSerialMode == SERIALMODE_SERIALIZE)
	{
		if (eCompMode == COMPMODE_EQUALS && iVal == iCompVal)
		{
			StrCatCh(szMutStr, CH_XH_SEGEND);
		}
		else
		{
			char szTmp[NEO_XHAIR_SEQMAX];
			V_sprintf_safe(szTmp, "%d%c", (iMin < iMax) ? clamp(iVal, iMin, iMax) : iVal, CH_XH_SEGEND);
			V_strcat_safe(szMutStr, szTmp);
		}
		return iVal;
	}
	else
	{
		return DeserialVariant(szMutStr, SERIALVARIANTTYPE_INT,
				{ .iVal = (eCompMode == COMPMODE_EQUALS) ? iCompVal : iVal },
				{ .iVal = iMin }, { .iVal = iMax }, ctx).iVal;
	}
}

[[nodiscard]] bool SerialBool(const bool bVal, const bool bCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx)
{
	if (ctx->eSerialMode == SERIALMODE_SERIALIZE)
	{
		if (eCompMode == COMPMODE_EQUALS && bVal == bCompVal)
		{
			StrCatCh(szMutStr, CH_XH_SEGEND);
		}
		else
		{
			char szTmp[NEO_XHAIR_SEQMAX];
			V_sprintf_safe(szTmp, "%d%c", static_cast<int>(bVal), CH_XH_SEGEND);
			V_strcat_safe(szMutStr, szTmp);
		}
		return bVal;
	}
	else
	{
		return DeserialVariant(szMutStr, SERIALVARIANTTYPE_BOOL,
				{ .bVal = (eCompMode == COMPMODE_EQUALS) ? bCompVal : bVal },
				{}, {}, ctx).bVal;
	}
}

[[nodiscard]] float SerialFloat(const float flVal, const float flCompVal,
		const ECompMode eCompMode, char (&szMutStr)[NEO_XHAIR_SEQMAX], SerialContext *ctx,
		const float flMin, const float flMax)
{
	if (ctx->eSerialMode == SERIALMODE_SERIALIZE)
	{
		if (eCompMode == COMPMODE_EQUALS && flVal == flCompVal)
		{
			StrCatCh(szMutStr, CH_XH_SEGEND);
		}
		else
		{
			char szTmp[NEO_XHAIR_SEQMAX];
			V_sprintf_safe(szTmp, "%.3f%c", (flMin < flMax) ? clamp(flVal, flMin, flMax) : flVal, CH_XH_SEGEND);
			V_strcat_safe(szMutStr, szTmp);
		}
		return flVal;
	}
	else
	{
		return DeserialVariant(szMutStr, SERIALVARIANTTYPE_FLOAT,
				{ .flVal = (eCompMode == COMPMODE_EQUALS) ? flCompVal : flVal },
				{ .flVal = flMin }, { .flVal = flMax }, ctx).flVal;
	}
}

void SerialRLEncode(char (&szMutSeq)[NEO_XHAIR_SEQMAX], const ESerialMode eSerialMode)
{
	// NEO NOTE (nullsystem): This RLE doesn't "properly" decode/encode right from the
	// first character. But in the general case it'll never really hit that scenario
	// as typically you'll need some properly values to be serialize before the whole
	// empty segments + run-length encoding becomes useful anyway.
	static constexpr char SZ_XH_MINSEGCOMPRESS[] = ";;;;";

	if (eSerialMode != SERIALMODE_SERIALIZE)
	{
		return;
	}

	char szFinalSeq[NEO_XHAIR_SEQMAX] = {};
	char *pszToRLE = nullptr;
	int iOffset = 0;
	const int iSzMutSeqSize = V_strlen(szMutSeq);

	while ((pszToRLE = V_strstr(szMutSeq + iOffset, SZ_XH_MINSEGCOMPRESS)))
	{
		// NEO NOTE (nullsystem): ";;;;" is already 3 segments (after the first),
		// so start counting from the character after that (in index 4)
		// It's also the minimal to take use of RLE as ";;;;" -> ";3^" saving 1 byte,
		// greater saving the greater the repeats.
		int iLen = 3;
		while (((iOffset + iLen + 1) < iSzMutSeqSize) && (pszToRLE[iLen + 1] == CH_XH_SEGEND))
		{
			++iLen;
		}
		const int iPszToRLEPos = pszToRLE - szMutSeq;
		if (iPszToRLEPos > iOffset)
		{
			V_strncat(szFinalSeq, szMutSeq + iOffset, NEO_XHAIR_SEQMAX, iPszToRLEPos - iOffset + 1);
		}
		{
			// iPszToRLEPos == 0 never really going to happen for crosshair, but deal
			// with the edge case anyway
			char szTmp[NEO_XHAIR_SEQMAX];
			V_sprintf_safe(szTmp, "%s%d%c", (iPszToRLEPos == 0) ? ";" : "", iLen, CH_XH_SEGSKIP);
			V_strcat_safe(szFinalSeq, szTmp);
		}
		iOffset = iPszToRLEPos + iLen + 1;
	}
	if (iOffset < iSzMutSeqSize)
	{
		V_strcat_safe(szFinalSeq, szMutSeq + iOffset);
	}

	V_strcpy_safe(szMutSeq, szFinalSeq);
}

