#include "neo_crosshair.h"

#include "cbase.h"
#include "shareddefs.h"

#ifdef CLIENT_DLL
#include "neo_gamerules.h"
#include "weapon_neobasecombatweapon.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "ui/neo_root.h"
#include "view.h"

#include "c_neo_player.h"

static char static_szCrhSerialDefault[NEO_XHAIR_SEQMAX];
ConVar cl_neo_crosshair("cl_neo_crosshair", "", FCVAR_ARCHIVE | FCVAR_USERINFO, "Serialized crosshair setting");
ConVar cl_neo_crosshair_network("cl_neo_crosshair_network", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Network crosshair - 0 = disable, 1 = show other players' crosshairs", true, 0.0f, true, 1.0f);

static const char *INTERNAL_CROSSHAIR_FILES[CROSSHAIR_STYLE__TOTAL] = { "vgui/hud/crosshair", "vgui/hud/crosshair_b", "" };
const char **CROSSHAIR_FILES = INTERNAL_CROSSHAIR_FILES;

static const wchar_t *INTERNAL_CROSSHAIR_LABELS[CROSSHAIR_STYLE__TOTAL] = { L"Default", L"Alt", L"Custom" };
const wchar_t **CROSSHAIR_LABELS = INTERNAL_CROSSHAIR_LABELS;

static const wchar_t *INTERNAL_CROSSHAIR_DYNAMICTYPE_LABELS[CROSSHAIR_DYNAMICTYPE__TOTAL] = { L"Disabled", L"Gap", L"Circle", L"Size" };
const wchar_t **CROSSHAIR_DYNAMICTYPE_LABELS = INTERNAL_CROSSHAIR_DYNAMICTYPE_LABELS;

static const wchar_t *INTERNAL_CROSSHAIR_SIZETYPE_LABELS[CROSSHAIR_SIZETYPE__TOTAL] = { L"Absolute", L"Screen halves" };
const wchar_t **CROSSHAIR_SIZETYPE_LABELS = INTERNAL_CROSSHAIR_SIZETYPE_LABELS;

void PaintCrosshair(const CrosshairInfo &crh, int inaccuracy, const int x, const int y)
{
	if (NEORules() && NEORules()->GetHiddenHudElements() & NEO_HUD_ELEMENT_CROSSHAIR)
	{
		return;
	}

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	int iSize = crh.iSize;
	int iThick = crh.iThick;
	int iGap = crh.iGap;
	int iCircleRad = crh.iCircleRad;
	int iCircleSegments = crh.iCircleSegments;

	switch (crh.eDynamicType)
	{ // Do we want to add the inaccuracy or replace the inaccuracy, or set the size to whatever is larger? should the operation be another option for the player?
	case CROSSHAIR_DYNAMICTYPE_SIZE:
		iSize += inaccuracy;
		if (!iThick)
		{
			iThick = 2;
		}
		break;
	case CROSSHAIR_DYNAMICTYPE_GAP:
		iGap += inaccuracy;
		if (!iThick)
		{
			iThick = 2;
		}
		if (!iSize)
		{
			iSize = 6;
		}
		break;
	case CROSSHAIR_DYNAMICTYPE_CIRCLE:
		iCircleRad += inaccuracy;
		if (!iCircleSegments)
		{
			iCircleSegments = 20;
		}
		break;
	default:
		break;
	}

	if (iSize > 0)
	{
		if (crh.eSizeType == CROSSHAIR_SIZETYPE_SCREEN && crh.eDynamicType == CROSSHAIR_DYNAMICTYPE_NONE)
		{
			iSize = (crh.flScrSize * (Max(wide, tall) / 2));
		}

		const bool bOdd = ((crh.iThick % 2) == 1);
		const int iRBOffset = bOdd ? -1 : 0; // Right + bottom, odd-px offset
		const int iHalf = crh.iThick / 2;
		const int iStartThick = bOdd ? iHalf + 1 : iHalf;
		const int iEndThick = iHalf;
		vgui::IntRect iRects[4] = {
			{ -iSize - iGap, -iStartThick, -iGap, iEndThick },	// Left
			{ iGap + iRBOffset, -iStartThick, iGap + iSize + iRBOffset, iEndThick },	// Right
			{ -iStartThick, iGap + iRBOffset, iEndThick, iGap + iSize + iRBOffset },	// Bottom
			{ -iStartThick, -iSize - iGap, iEndThick, -iGap },	// Top (Must be last for bTopLine)
		};
		for (vgui::IntRect &rect : iRects)
		{
			rect.x0 += x;
			rect.y0 += y;
			rect.x1 += x;
			rect.y1 += y;
		}
		const int iRectSize = (crh.bTopLine) ? 4 : 3;
		if (crh.iOutline > 0)
		{
			vgui::IntRect iOutRects[4] = {};
			V_memcpy(iOutRects, iRects, sizeof(vgui::IntRect) * 4);
			for (vgui::IntRect &rect : iOutRects)
			{
				rect.x0 -= crh.iOutline;
				rect.y0 -= crh.iOutline;
				rect.x1 += crh.iOutline;
				rect.y1 += crh.iOutline;
			}
			vgui::surface()->DrawSetColor(crh.colorOutline);
			vgui::surface()->DrawFilledRectArray(iOutRects, iRectSize);
		}

		vgui::surface()->DrawSetColor(crh.color);
		vgui::surface()->DrawFilledRectArray(iRects, iRectSize);
	}

	if (crh.iCenterDot > 0)
	{
		const bool bOdd = ((crh.iCenterDot % 2) == 1);
		const int iHalf = crh.iCenterDot / 2;
		const int iStartCenter = bOdd ? iHalf + 1 : iHalf;
		const int iEndCenter = iHalf;

		if (crh.iOutline > 0)
		{
			vgui::surface()->DrawSetColor(crh.bSeparateColorDot ? crh.colorDotOutline : crh.colorOutline);
			vgui::surface()->DrawFilledRect(-iStartCenter + x -crh.iOutline, -iStartCenter + y -crh.iOutline,
											iEndCenter + x +crh.iOutline, iEndCenter + y +crh.iOutline);
		}

		vgui::surface()->DrawSetColor(crh.bSeparateColorDot ? crh.colorDot : crh.color);
		vgui::surface()->DrawFilledRect(-iStartCenter + x, -iStartCenter + y, iEndCenter + x, iEndCenter + y);
	}

	if (iCircleRad > 0 && iCircleSegments > 0)
	{
		vgui::surface()->DrawSetColor(crh.color);
		vgui::surface()->DrawOutlinedCircle(x, y, iCircleRad, iCircleSegments);
	}
}

int HalfInaccuracyConeInScreenPixels(C_NEOBaseCombatWeapon* pWeapon, int halfScreenWidth)
{
	float spread = pWeapon && pWeapon->GetNeoWepBits() & NEO_WEP_FIREARM ? pWeapon->GetBulletSpread().x : 0.f;
	if (!spread)
	{
		return 0;
	}
	float scaledFov = DEG2RAD(ScaleFOVByWidthRatio(
		C_BasePlayer::GetLocalPlayer()->GetFOV(),
		engine->GetScreenAspectRatio() * 0.75f )) / 2;
	return halfScreenWidth * spread / tan(scaledFov);
}

void InitializeClNeoCrosshair()
{
	DefaultCrosshairSerial(static_szCrhSerialDefault);
	cl_neo_crosshair.SetDefault(static_szCrhSerialDefault);
}

#endif // CLIENT_DLL

// NEO WARNING (nullsystem): When adding new/removing items to serialize, only append enum one after another
// before the NEOXHAIR_SERIAL__LATESTPLUSONE section!
// When working on this put in the current in-dev/unreleased version for the enum name.
// Bump NEO_XHAIR_SEQMAX if it's going to go over the string length
enum NeoXHairSerial
{
	NEOXHAIR_SERIAL_PREALPHA_V8_2 = 1,
	NEOXHAIR_SERIAL_ALPHA_V17,
	NEOXHAIR_SERIAL_ALPHA_V19,
	NEOXHAIR_SERIAL_ALPHA_V22,
	NEOXHAIR_SERIAL_ALPHA_V28,

	NEOXHAIR_SERIAL__LATESTPLUSONE,
	NEOXHAIR_SERIAL_CURRENT = NEOXHAIR_SERIAL__LATESTPLUSONE - 1,
};

void ResetCrosshairToDefault(CrosshairInfo *crh)
{
	crh->iStyle = CROSSHAIR_STYLE_DEFAULT;
	crh->color = COLOR_WHITE;
	crh->eSizeType = CROSSHAIR_SIZETYPE_ABSOLUTE;
	crh->iSize = 6;
	crh->flScrSize = 0.0f;
	crh->iThick = 2;
	crh->iGap = 4;
	crh->iOutline = 0;
	crh->iCenterDot = 0;
	crh->bTopLine = true;
	crh->iCircleRad = 0;
	crh->iCircleSegments = 0;
	crh->eDynamicType = CROSSHAIR_DYNAMICTYPE_NONE;
	crh->bSeparateColorDot = false;
	crh->colorDot = COLOR_WHITE;
	crh->colorDotOutline = COLOR_WHITE;
	crh->colorOutline = COLOR_WHITE;
}

void DefaultCrosshairSerial(char (&szSequence)[NEO_XHAIR_SEQMAX])
{
	CrosshairInfo crh = {};
	ResetCrosshairToDefault(&crh);
	ExportCrosshair(&crh, szSequence);
}

union NeoXHairVariant
{
	int iVal;
	float flVal;
	bool bVal;
};

enum NeoXHairVariantType
{
	NEOXHAIRVARTYPE_INT = 0,
	NEOXHAIRVARTYPE_FLOAT,
	NEOXHAIRVARTYPE_BOOL,
};

static NeoXHairVariant DeserialVariant(char (&szMutStr)[NEO_XHAIR_SEQMAX], const int iSzMutStrSize,
		int *idx, const NeoXHairVariantType eType, const NeoXHairVariant varDefault)
{
	NeoXHairVariant var;
	V_memcpy(&var, &varDefault, sizeof(NeoXHairVariant));

	const int iPrevSegment = *idx;
	Assert(iSzMutStrSize <= NEO_XHAIR_SEQMAX);

	for (; *idx < iSzMutStrSize; ++(*idx))
	{
		if (szMutStr[*idx] == ';')
		{
			szMutStr[*idx] = '\0';
			const char *pszCurSegment = szMutStr + iPrevSegment;
			if (pszCurSegment[0] != '\0')
			{
				switch (eType)
				{
				case NEOXHAIRVARTYPE_INT:
					var.iVal = atoi(pszCurSegment);
					break;
				case NEOXHAIRVARTYPE_BOOL:
					var.bVal = (atoi(pszCurSegment) != 0);
					break;
				case NEOXHAIRVARTYPE_FLOAT:
					var.flVal = static_cast<float>(atof(pszCurSegment));
					break;
				}
			}
			++(*idx);
			break;
		}
	}

	return var;
}

enum ESerialMode
{
	SERIALMODE_DESERIALIZE = 0,
	SERIALMODE_SERIALIZE,
};

[[nodiscard]] static inline int SerialInt(const int iVal, const ESerialMode eSerialMode,
		char (&szMutStr)[NEO_XHAIR_SEQMAX], const int iSzMutStrSize, int *idx)
{
	if (eSerialMode == SERIALMODE_SERIALIZE)
	{
		char szTmp[NEO_XHAIR_SEQMAX];
		V_sprintf_safe(szTmp, "%d;", iVal);
		V_strcat_safe(szMutStr, szTmp);
		return iVal;
	}
	else
	{
		return DeserialVariant(szMutStr, iSzMutStrSize, idx, NEOXHAIRVARTYPE_INT, { .iVal = iVal }).iVal;
	}
}

[[nodiscard]] static inline bool SerialBool(const bool bVal, const ESerialMode eSerialMode,
		char (&szMutStr)[NEO_XHAIR_SEQMAX], const int iSzMutStrSize, int *idx)
{
	if (eSerialMode == SERIALMODE_SERIALIZE)
	{
		char szTmp[NEO_XHAIR_SEQMAX];
		V_sprintf_safe(szTmp, "%d;", static_cast<int>(bVal));
		V_strcat_safe(szMutStr, szTmp);
		return bVal;
	}
	else
	{
		return DeserialVariant(szMutStr, iSzMutStrSize, idx, NEOXHAIRVARTYPE_BOOL, { .bVal = bVal } ).bVal;
	}
}

[[nodiscard]] static inline float SerialFloat(const float flVal, const ESerialMode eSerialMode,
			char (&szMutStr)[NEO_XHAIR_SEQMAX], const int iSzMutStrSize, int *idx)
{
	if (eSerialMode == SERIALMODE_SERIALIZE)
	{
		char szTmp[NEO_XHAIR_SEQMAX];
		V_sprintf_safe(szTmp, "%.3f;", flVal);
		V_strcat_safe(szMutStr, szTmp);
		return flVal;
	}
	else
	{
		return DeserialVariant(szMutStr, iSzMutStrSize, idx, NEOXHAIRVARTYPE_FLOAT, { .flVal = flVal } ).flVal;
	}
}

static void ImportOrExportCrosshair(const ESerialMode eSerialMode, CrosshairInfo *crh,
		char (&szMutSeq)[NEO_XHAIR_SEQMAX], const int iSeqSize)
{
	int idx = 0;

	const int iSerialVersion = SerialInt(NEOXHAIR_SERIAL_CURRENT, eSerialMode,
			szMutSeq, iSeqSize, &idx);

	// v28 onwards cuts out segments if unused
	const bool bNotCompact = (iSerialVersion < NEOXHAIR_SERIAL_ALPHA_V28);

	crh->iStyle = SerialInt(crh->iStyle, eSerialMode, szMutSeq, iSeqSize, &idx);
	crh->color.SetRawColor(SerialInt(crh->color.GetRawColor(), eSerialMode, szMutSeq, iSeqSize, &idx));

	if (bNotCompact || crh->iStyle == CROSSHAIR_STYLE_CUSTOM)
	{
		crh->eSizeType = static_cast<NeoHudCrosshairSizeType>(SerialInt(crh->eSizeType, eSerialMode, szMutSeq, iSeqSize, &idx));
		if (bNotCompact || crh->eSizeType == CROSSHAIR_SIZETYPE_ABSOLUTE)
		{
			crh->iSize = SerialInt(crh->iSize, eSerialMode, szMutSeq, iSeqSize, &idx);
		}
		if (bNotCompact || crh->eSizeType == CROSSHAIR_SIZETYPE_SCREEN)
		{
			crh->flScrSize = SerialFloat(crh->flScrSize, eSerialMode, szMutSeq, iSeqSize, &idx);
		}
		crh->iThick = SerialInt(crh->iThick, eSerialMode, szMutSeq, iSeqSize, &idx);
		crh->iGap = SerialInt(crh->iGap, eSerialMode, szMutSeq, iSeqSize, &idx);
		crh->iOutline = SerialInt(crh->iOutline, eSerialMode, szMutSeq, iSeqSize, &idx);
		crh->iCenterDot = SerialInt(crh->iCenterDot, eSerialMode, szMutSeq, iSeqSize, &idx);
		crh->bTopLine = SerialBool(crh->bTopLine, eSerialMode, szMutSeq, iSeqSize, &idx);
		crh->iCircleRad = SerialInt(crh->iCircleRad, eSerialMode, szMutSeq, iSeqSize, &idx);
		if (bNotCompact || crh->iCircleRad > 0)
		{
			crh->iCircleSegments = SerialInt(crh->iCircleSegments, eSerialMode, szMutSeq, iSeqSize, &idx);
		}
		
		crh->eDynamicType = (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V19)
				? static_cast<NeoHudCrosshairDynamicType>(SerialInt(crh->eDynamicType, eSerialMode, szMutSeq, iSeqSize, &idx))
				: CROSSHAIR_DYNAMICTYPE_NONE;

		if (eSerialMode == SERIALMODE_DESERIALIZE)
		{
			crh->bSeparateColorDot = false;
			crh->colorDot = COLOR_BLACK;
			crh->colorDotOutline = COLOR_BLACK;
			crh->colorOutline = COLOR_BLACK;
		}
		if (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V22 && (bNotCompact || crh->iCenterDot > 0))
		{
			crh->bSeparateColorDot = SerialBool(crh->bSeparateColorDot, eSerialMode, szMutSeq, iSeqSize, &idx);
			if (bNotCompact || crh->bSeparateColorDot)
			{
				crh->colorDot.SetRawColor(SerialInt(crh->colorDot.GetRawColor(), eSerialMode, szMutSeq, iSeqSize, &idx));
				crh->colorDotOutline.SetRawColor(SerialInt(crh->colorDotOutline.GetRawColor(), eSerialMode, szMutSeq, iSeqSize, &idx));
				crh->colorOutline.SetRawColor(SerialInt(crh->colorOutline.GetRawColor(), eSerialMode, szMutSeq, iSeqSize, &idx));
			}
		}
	}
}

bool ImportCrosshair(CrosshairInfo *crh, const char *pszSequence)
{
	const int iSeqSize = V_strlen(pszSequence);
	if (iSeqSize <= 0 || iSeqSize > NEO_XHAIR_SEQMAX)
	{
		return false;
	}

	char szMutSeq[NEO_XHAIR_SEQMAX];
	V_memcpy(szMutSeq, pszSequence, sizeof(char) * iSeqSize);
	szMutSeq[iSeqSize] = '\0';

	ImportOrExportCrosshair(SERIALMODE_DESERIALIZE, crh, szMutSeq, iSeqSize);
	return true;
}

void ExportCrosshair(CrosshairInfo *crh, char (&szSequence)[NEO_XHAIR_SEQMAX])
{
	szSequence[0] = '\0';
	ImportOrExportCrosshair(SERIALMODE_SERIALIZE, crh, szSequence, NEO_XHAIR_SEQMAX);
}

