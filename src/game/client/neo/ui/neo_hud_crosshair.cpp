#include "neo_hud_crosshair.h"
#include "neo_gamerules.h"

#include "filesystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "ui/neo_root.h"

ConVar cl_neo_crosshair("cl_neo_crosshair", CL_NEO_CROSSHAIR_DEFAULT, FCVAR_ARCHIVE | FCVAR_USERINFO, "Serialized crosshair setting");
ConVar cl_neo_crosshair_network("cl_neo_crosshair_network", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Network crosshair - 0 = disable, 1 = show other players' crosshairs", true, 0.0f, true, 1.0f);

static const char *INTERNAL_CROSSHAIR_FILES[CROSSHAIR_STYLE__TOTAL] = { "vgui/hud/crosshair", "vgui/hud/crosshair_b", "" };
const char **CROSSHAIR_FILES = INTERNAL_CROSSHAIR_FILES;

static const wchar_t *INTERNAL_CROSSHAIR_LABELS[CROSSHAIR_STYLE__TOTAL] = { L"Default", L"Alt", L"Custom" };
const wchar_t **CROSSHAIR_LABELS = INTERNAL_CROSSHAIR_LABELS;

static const wchar_t *INTERNAL_CROSSHAIR_SIZETYPE_LABELS[CROSSHAIR_SIZETYPE__TOTAL] = { L"Absolute", L"Screen halves" };
const wchar_t **CROSSHAIR_SIZETYPE_LABELS = INTERNAL_CROSSHAIR_SIZETYPE_LABELS;

void PaintCrosshair(const CrosshairInfo &crh, const int x, const int y)
{
	if (NEORules() && NEORules()->GetHiddenHudElements() & NEO_HUD_ELEMENT_CROSSHAIR)
	{
		return;
	}
	if (crh.iSize > 0)
	{
		int iSize = crh.iSize;
		if (crh.iESizeType == CROSSHAIR_SIZETYPE_SCREEN)
		{
			int wide, tall;
			vgui::surface()->GetScreenSize(wide, tall);
			iSize = (crh.flScrSize * (max(wide, tall) / 2));
		}

		const bool bOdd = ((crh.iThick % 2) == 1);
		const int iHalf = crh.iThick / 2;
		const int iStartThick = bOdd ? iHalf + 1 : iHalf;
		const int iEndThick = iHalf;
		vgui::IntRect iRects[4] = {
			{ -iSize - crh.iGap, -iStartThick, -crh.iGap, iEndThick },	// Left
			{ crh.iGap, -iStartThick, crh.iGap + iSize, iEndThick },	// Right
			{ -iStartThick, crh.iGap, iEndThick, crh.iGap + iSize },	// Bottom
			{ -iStartThick, -iSize - crh.iGap, iEndThick, -crh.iGap },	// Top (Must be last for bTopLine)
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
			vgui::surface()->DrawSetColor(COLOR_BLACK);
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
			vgui::surface()->DrawSetColor(COLOR_BLACK);
			vgui::surface()->DrawFilledRect(-iStartCenter + x -crh.iOutline, -iStartCenter + y -crh.iOutline,
											iEndCenter + x +crh.iOutline, iEndCenter + y +crh.iOutline);
		}

		vgui::surface()->DrawSetColor(crh.color);
		vgui::surface()->DrawFilledRect(-iStartCenter + x, -iStartCenter + y, iEndCenter + x, iEndCenter + y);
	}

	if (crh.iCircleRad > 0 && crh.iCircleSegments > 0)
	{
		vgui::surface()->DrawSetColor(crh.color);
		vgui::surface()->DrawOutlinedCircle(x, y, crh.iCircleRad, crh.iCircleSegments);
	}
}

// NEO WARNING (nullsystem): When adding new/removing items to serialize, only append enum one after another
// before the NEOXHAIR_SERIAL__LATESTPLUSONE section!
// When working on this put in the current in-dev/unreleased version for the enum name.
enum NeoXHairSerial
{
	NEOXHAIR_SERIAL_PREALPHA_V8_2 = 1,
	NEOXHAIR_SERIAL_ALPHA_V17,

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
};

bool ImportCrosshair(CrosshairInfo *crh, const char *pszSequence)
{
	int iPrevSegment = 0;
	int iSegmentIdx = 0;
	NeoXHairVariant vars[NEOXHAIR_SEGMENT__TOTAL] = {};

	const int iPszSequenceSize = V_strlen(pszSequence);
	if (iPszSequenceSize <= 0 || iPszSequenceSize > NEO_XHAIR_SEQMAX)
	{
		return false;
	}

	char szMutSequence[NEO_XHAIR_SEQMAX];
	V_memcpy(szMutSequence, pszSequence, sizeof(char) * iPszSequenceSize);
	for (int i = 0; i < iPszSequenceSize && iSegmentIdx < NEOXHAIR_SEGMENT__TOTAL; ++i)
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

	if (iSegmentIdx < NEOXHAIR_SEGMENT__TOTAL)
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

	return true;
}

void ExportCrosshair(const CrosshairInfo *crh, char (&szSequence)[NEO_XHAIR_SEQMAX])
{
	V_sprintf_safe(szSequence,
			"%d;%d;%d;%d;%d;%.3f;%d;%d;%d;%d;%d;%d;%d;",
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
			crh->iCircleSegments);
}
