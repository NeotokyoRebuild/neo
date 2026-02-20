#include "neo_hud_crosshair.h"
#include "neo_gamerules.h"
#include "weapon_neobasecombatweapon.h"

#include "filesystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "ui/neo_root.h"
#include "view.h"

ConVar cl_neo_crosshair("cl_neo_crosshair", NEO_CROSSHAIR_DEFAULT, FCVAR_ARCHIVE | FCVAR_USERINFO, "Serialized crosshair setting");
ConVar cl_neo_crosshair_network("cl_neo_crosshair_network", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Network crosshair - 0 = disable, 1 = show other players' crosshairs", true, 0.0f, true, 1.0f);

static const char *INTERNAL_CROSSHAIR_FILES[CROSSHAIR_STYLE__TOTAL] = { "vgui/hud/crosshair", "vgui/hud/crosshair_b", "" };
const char **CROSSHAIR_FILES = INTERNAL_CROSSHAIR_FILES;

static const wchar_t *INTERNAL_CROSSHAIR_LABELS[CROSSHAIR_STYLE__TOTAL] = { L"Default", L"Alt", L"Custom" };
const wchar_t **CROSSHAIR_LABELS = INTERNAL_CROSSHAIR_LABELS;

static const wchar_t *INTERNAL_CROSSHAIR_DYNAMICTYPE_LABELS[CROSSHAIR_DYNAMICTYPE_TOTAL] = { L"Disabled", L"Gap", L"Circle", L"Size" };
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

	switch (crh.iEDynamicType)
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
		if (crh.iESizeType == CROSSHAIR_SIZETYPE_SCREEN && crh.iEDynamicType == CROSSHAIR_DYNAMICTYPE_NONE)
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

// NEO WARNING (nullsystem): When adding new/removing items to serialize, only append enum one after another
// before the NEOXHAIR_SERIAL__LATESTPLUSONE section!
// When working on this put in the current in-dev/unreleased version for the enum name.
enum NeoXHairSerial
{
	NEOXHAIR_SERIAL_PREALPHA_V8_2 = 1,
	NEOXHAIR_SERIAL_ALPHA_V17,
	NEOXHAIR_SERIAL_ALPHA_V19,
	NEOXHAIR_SERIAL_ALPHA_V22,

	NEOXHAIR_SERIAL__LATESTPLUSONE,
	NEOXHAIR_SERIAL_CURRENT = NEOXHAIR_SERIAL__LATESTPLUSONE - 1,
};

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

static constexpr const NeoXHairVariantType NEOXHAIR_SEGMENT_VARTYPES[NEOXHAIR_SEGMENT__TOTAL] = {
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_VERSION
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_STYLE
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_COLOR
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_SIZETYPE
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_SIZE
	NEOXHAIRVARTYPE_FLOAT, // NEOXHAIR_SEGMENT_FL_SCRSIZE
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_THICK
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_GAP
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_OUTLINE
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_CENTERDOT
	NEOXHAIRVARTYPE_BOOL,  // NEOXHAIR_SEGMENT_B_TOPLINE
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_CIRCLERAD
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_CIRCLESEGMENTS
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_DYNAMICTYPE
	NEOXHAIRVARTYPE_BOOL,  // NEOXHAIR_SEGMENT_B_SEPARATECOLORDOT,
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_COLORDOT,
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_COLORDOTOUTLINE,
	NEOXHAIRVARTYPE_INT,   // NEOXHAIR_SEGMENT_I_COLOROUTLINE,
};

bool ImportCrosshair(CrosshairInfo *crh, const char *pszSequence)
{
	int iPrevSegment = 0;
	int iSegmentIdx = 0;
	NeoXHairVariant vars[NEOXHAIR_SEGMENT__TOTAL] = {};

	const int iPszSequenceLength = V_strlen(pszSequence);
	if (iPszSequenceLength <= 0 || iPszSequenceLength > NEO_XHAIR_SEQMAX)
	{
		return false;
	}

	char szMutSequence[NEO_XHAIR_SEQMAX];
	V_memcpy(szMutSequence, pszSequence, sizeof(char) * iPszSequenceLength);
	for (int i = 0; i < iPszSequenceLength && iSegmentIdx < NEOXHAIR_SEGMENT__TOTAL; ++i)
	{
		const char ch = szMutSequence[i];
		if (ch == ';')
		{
			szMutSequence[i] = '\0';
			const char *pszCurSegment = szMutSequence + iPrevSegment;
			const int iPszCurSegmentSize = i - iPrevSegment;
			if (iPszCurSegmentSize > 0)
			{
				switch (NEOXHAIR_SEGMENT_VARTYPES[iSegmentIdx])
				{
				case NEOXHAIRVARTYPE_INT:
					vars[iSegmentIdx].iVal = atoi(pszCurSegment);
					break;
				case NEOXHAIRVARTYPE_BOOL:
					vars[iSegmentIdx].bVal = (atoi(pszCurSegment) != 0);
					break;
				case NEOXHAIRVARTYPE_FLOAT:
					vars[iSegmentIdx].flVal = static_cast<float>(atof(pszCurSegment));
					break;
				}
			}
			iPrevSegment = i + 1;
			++iSegmentIdx;
		}
	}

	static constexpr const int SERIAL_TO_TOTAL_MAP[NEOXHAIR_SERIAL__LATESTPLUSONE] = {
		0, // not used
		0, // NEOXHAIR_SERIAL_PREALPHA_V8_2 (pre-string serial, not used)
		NEOXHAIR_SEGMENT__TOTAL_SERIAL_ALPHA_V17, // NEOXHAIR_SERIAL_ALPHA_V17
		NEOXHAIR_SEGMENT__TOTAL_SERIAL_ALPHA_V19, // NEOXHAIR_SERIAL_ALPHA_V19
		NEOXHAIR_SEGMENT__TOTAL_SERIAL_ALPHA_V22, // NEOXHAIR_SERIAL_ALPHA_V22
	};

	const int iSerialVersion = vars[NEOXHAIR_SEGMENT_I_VERSION].iVal;
	const int iTotalForSerial = (IN_BETWEEN_AR(0, iSerialVersion, NEOXHAIR_SERIAL__LATESTPLUSONE)) ?
			SERIAL_TO_TOTAL_MAP[iSerialVersion] : 0;
	if ((iTotalForSerial <= 0) || (iSegmentIdx < iTotalForSerial))
	{
		return false;
	}

	crh->iStyle = vars[NEOXHAIR_SEGMENT_I_STYLE].iVal;
	crh->color.SetRawColor(vars[NEOXHAIR_SEGMENT_I_COLOR].iVal);
	crh->iESizeType = vars[NEOXHAIR_SEGMENT_I_SIZETYPE].iVal;
	crh->iSize = vars[NEOXHAIR_SEGMENT_I_SIZE].iVal;
	crh->flScrSize = vars[NEOXHAIR_SEGMENT_FL_SCRSIZE].flVal;
	crh->iThick = vars[NEOXHAIR_SEGMENT_I_THICK].iVal;
	crh->iGap = vars[NEOXHAIR_SEGMENT_I_GAP].iVal;
	crh->iOutline = vars[NEOXHAIR_SEGMENT_I_OUTLINE].iVal;
	crh->iCenterDot = vars[NEOXHAIR_SEGMENT_I_CENTERDOT].iVal;
	crh->bTopLine = vars[NEOXHAIR_SEGMENT_B_TOPLINE].bVal;
	crh->iCircleRad = vars[NEOXHAIR_SEGMENT_I_CIRCLERAD].iVal;
	crh->iCircleSegments = vars[NEOXHAIR_SEGMENT_I_CIRCLESEGMENTS].iVal;
	
	crh->iEDynamicType = (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V19) ?
			vars[NEOXHAIR_SEGMENT_I_DYNAMICTYPE].iVal : 0;

	if (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V22)
	{
		crh->bSeparateColorDot = vars[NEOXHAIR_SEGMENT_B_SEPARATECOLORDOT].bVal;
		crh->colorDot.SetRawColor(vars[NEOXHAIR_SEGMENT_I_COLORDOT].iVal);
		crh->colorDotOutline.SetRawColor(vars[NEOXHAIR_SEGMENT_I_COLORDOTOUTLINE].iVal);
		crh->colorOutline.SetRawColor(vars[NEOXHAIR_SEGMENT_I_COLOROUTLINE].iVal);
	}
	else
	{
		crh->bSeparateColorDot = false;
		crh->colorDot = COLOR_BLACK;
		crh->colorDotOutline = COLOR_BLACK;
		crh->colorOutline = COLOR_BLACK;
	}

	return true;
}

void ExportCrosshair(const CrosshairInfo *crh, char (&szSequence)[NEO_XHAIR_SEQMAX])
{
	V_sprintf_safe(szSequence,
			"%d;%d;%d;%d;%d;%.3f;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;",
			NEOXHAIR_SERIAL_CURRENT,
			crh->iStyle,
			crh->color.GetRawColor(),
			crh->iESizeType,
			crh->iSize,
			crh->flScrSize,
			crh->iThick,
			crh->iGap,
			crh->iOutline,
			crh->iCenterDot,
			static_cast<int>(crh->bTopLine),
			crh->iCircleRad,
			crh->iCircleSegments,
			crh->iEDynamicType,
			static_cast<int>(crh->bSeparateColorDot),
			crh->colorDot.GetRawColor(),
			crh->colorDotOutline.GetRawColor(),
			crh->colorOutline.GetRawColor());
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
};
