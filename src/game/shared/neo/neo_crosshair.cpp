#include "neo_crosshair.h"

#ifdef UNIT_TEST_DLL
#include "strtools.h"
#include "tier0/dbg.h"

extern const char *g_testFnName;
extern int g_verbose;
#else
#include "cbase.h"
#include "shareddefs.h"
#endif // UNIT_TEST_DLL

#include "mathlib/mathlib.h"
#include "neo_serial.h"

const ENeoCrosshairWep MAP_WEAPON_TYPE_TO_XHAIR[WEP_TYPE__TOTAL] = {
	CROSSHAIR_WEP_NONE,			// WEP_TYPE_NIL
	CROSSHAIR_WEP_NONE,			// WEP_TYPE_THROWABLE
	CROSSHAIR_WEP_SECONDARY,	// WEP_TYPE_PISTOL
	CROSSHAIR_WEP_DEFAULT,		// WEP_TYPE_SMG
	CROSSHAIR_WEP_SHOTGUN,		// WEP_TYPE_SHOTGUN
	CROSSHAIR_WEP_DEFAULT,		// WEP_TYPE_RIFLE
	CROSSHAIR_WEP_DEFAULT,		// WEP_TYPE_MACHINEGUN
	CROSSHAIR_WEP_NONE,			// WEP_TYPE_SNIPER
};

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

void PaintCrosshair(const CrosshairWepInfo *crh, const int inaccuracy, const int x, const int y)
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	int iSize = crh->iSize;
	int iThick = crh->iThick;
	int iGap = crh->iGap;
	int iCircleRad = crh->iCircleRad;
	int iCircleSegments = crh->iCircleSegments;

	switch (crh->eDynamicType)
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
		if (crh->eSizeType == CROSSHAIR_SIZETYPE_SCREEN && crh->eDynamicType == CROSSHAIR_DYNAMICTYPE_NONE)
		{
			iSize = (crh->flScrSize * (Max(wide, tall) / 2));
		}

		const bool bOdd = ((crh->iThick % 2) == 1);
		const int iRBOffset = bOdd ? -1 : 0; // Right + bottom, odd-px offset
		const int iHalf = crh->iThick / 2;
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
		const int iRectSize = (crh->flags & CROSSHAIR_FLAG_NOTOPLINE) ? 3 : 4;
		if (crh->iOutline > 0)
		{
			vgui::IntRect iOutRects[4] = {};
			V_memcpy(iOutRects, iRects, sizeof(vgui::IntRect) * 4);
			for (vgui::IntRect &rect : iOutRects)
			{
				rect.x0 -= crh->iOutline;
				rect.y0 -= crh->iOutline;
				rect.x1 += crh->iOutline;
				rect.y1 += crh->iOutline;
			}
			vgui::surface()->DrawSetColor(crh->colorOutline);
			vgui::surface()->DrawFilledRectArray(iOutRects, iRectSize);
		}

		vgui::surface()->DrawSetColor(crh->color);
		vgui::surface()->DrawFilledRectArray(iRects, iRectSize);
	}

	if (crh->iCenterDot > 0)
	{
		const bool bOdd = ((crh->iCenterDot % 2) == 1);
		const int iHalf = crh->iCenterDot / 2;
		const int iStartCenter = bOdd ? iHalf + 1 : iHalf;
		const int iEndCenter = iHalf;
		const bool bSeparateColorDot = (crh->flags & CROSSHAIR_FLAG_SEPERATEDOTCOLOR);

		if (crh->iOutline > 0)
		{
			vgui::surface()->DrawSetColor(bSeparateColorDot ? crh->colorDotOutline : crh->colorOutline);
			vgui::surface()->DrawFilledRect(-iStartCenter + x -crh->iOutline, -iStartCenter + y -crh->iOutline,
											iEndCenter + x +crh->iOutline, iEndCenter + y +crh->iOutline);
		}

		vgui::surface()->DrawSetColor(bSeparateColorDot ? crh->colorDot : crh->color);
		vgui::surface()->DrawFilledRect(-iStartCenter + x, -iStartCenter + y, iEndCenter + x, iEndCenter + y);
	}

	if (iCircleRad > 0 && iCircleSegments > 0)
	{
		vgui::surface()->DrawSetColor(crh->color);
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

void ResetCrosshairToDefault(CrosshairInfo *xhairInfo,
		EHipfireOpt (*paeHipfireOpts)[CROSSHAIR_WEP__TOTAL])
{
	xhairInfo->wepFlags = CROSSHAIR_WEP_FLAG_DEFAULT;
	xhairInfo->hipfireFlags = CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL;
	CrosshairWepInfo *crh = &(xhairInfo->wep[CROSSHAIR_WEP_DEFAULT]);
	crh->iStyle = CROSSHAIR_STYLE_DEFAULT;
	crh->color = COLOR_WHITE;
	crh->flags = CROSSHAIR_FLAG_DEFAULT;
	crh->eSizeType = CROSSHAIR_SIZETYPE_ABSOLUTE;
	crh->iSize = 6;
	crh->flScrSize = 0.0f;
	crh->iThick = 2;
	crh->iGap = 4;
	crh->iOutline = 0;
	crh->iCenterDot = 0;
	crh->iCircleRad = 0;
	crh->iCircleSegments = 0;
	crh->eDynamicType = CROSSHAIR_DYNAMICTYPE_NONE;
	crh->colorDot = COLOR_WHITE;
	crh->colorDotOutline = COLOR_BLACK;
	crh->colorOutline = COLOR_BLACK;
	for (int i = 1; i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		V_memcpy(&(xhairInfo->wep[i]), crh, sizeof(CrosshairWepInfo));
	}
	if (paeHipfireOpts)
	{
		V_memset(*paeHipfireOpts, 0, sizeof(EHipfireOpt) * CROSSHAIR_WEP__TOTAL);
	}
}

void DefaultCrosshairSerial(char (&szSequence)[NEO_XHAIR_SEQMAX])
{
	CrosshairInfo xhairInfo = {};
	ResetCrosshairToDefault(&xhairInfo);
	ExportCrosshair(&xhairInfo, szSequence);
}

int UseCrosshairIndexFor(const CrosshairInfo *xhairInfo, const int iXHairWep, bool *pbHide)
{
	if (pbHide && iXHairWep == CROSSHAIR_WEP_NONE)
	{
		*pbHide = true;
	}

	if (iXHairWep > CROSSHAIR_WEP_DEFAULT)
	{
		if (xhairInfo->wepFlags & (1 << (iXHairWep - 1)))
		{
			if ((iXHairWep < CROSSHAIR_WEP_DEFAULT_HIPFIRE) ||
					(xhairInfo->hipfireFlags & (1 << (iXHairWep - CROSSHAIR_WEP_DEFAULT_HIPFIRE))))
			{
				return iXHairWep;
			}

			if (iXHairWep > CROSSHAIR_WEP_DEFAULT_HIPFIRE)
			{
				if (pbHide && !(xhairInfo->wepFlags & CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE))
				{
					*pbHide = true;
				}

				if (xhairInfo->hipfireFlags & CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT)
				{
					return CROSSHAIR_WEP_DEFAULT_HIPFIRE;
				}
			}
		}
		else if (pbHide && iXHairWep >= CROSSHAIR_WEP_DEFAULT_HIPFIRE)
		{
			*pbHide = true;
		}
	}
	return CROSSHAIR_WEP_DEFAULT;
}

static bool ImportOrExportCrosshair(const ESerialMode eSerialMode, CrosshairInfo *xhairInfo,
		char (&szMutSeq)[NEO_XHAIR_SEQMAX], const int iSeqSize, const int iExportSerialVersion)
{
	SerialContext ctx = {
		.eSerialMode = eSerialMode,
		.iSeqSize = iSeqSize,
	};
#ifdef UNIT_TEST_DLL
	if (g_verbose > 0) fprintf(stderr, "%s: ImportOrExportCrosshair: iSeqSize: %d\n", g_testFnName, iSeqSize);
#endif

	const int iSerialVersion = SerialInt(iExportSerialVersion, NEOXHAIR_SERIAL_CURRENT,
			COMPMODE_IGNORE, szMutSeq, &ctx);
	if (iSerialVersion <= NEOXHAIR_SERIAL_PREALPHA_V8_2 || iSerialVersion > NEOXHAIR_SERIAL_CURRENT)
	{
		// Unsupported serialization version or corrupted from first character
		return false;
	}

	// v28 onwards cuts out segments if unused
	const bool bNotCompact = (iSerialVersion < NEOXHAIR_SERIAL_ALPHA_V28);

	if (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V29)
	{
		xhairInfo->wepFlags = SerialInt(xhairInfo->wepFlags, xhairInfo->wepFlags,
				COMPMODE_IGNORE, szMutSeq, &ctx, 0, CROSSHAIR_WEP_FLAG__HIGHESTFLAG);
		xhairInfo->hipfireFlags = SerialInt(xhairInfo->hipfireFlags, xhairInfo->hipfireFlags,
				COMPMODE_IGNORE, szMutSeq, &ctx, 0, CROSSHAIR_HIPFIRECUSTOM_FLAG__HIGHESTFLAG);
	}

	for (int i = 0; i < CROSSHAIR_WEP__TOTAL; ++i)
	{
		const ECompMode eCompMode = (i == CROSSHAIR_WEP_DEFAULT) ? COMPMODE_IGNORE : COMPMODE_EQUALS;
		const int iCompIdx = (i > CROSSHAIR_WEP_DEFAULT_HIPFIRE)
				? CROSSHAIR_WEP_DEFAULT_HIPFIRE
				: CROSSHAIR_WEP_DEFAULT;

		CrosshairWepInfo *crh = &xhairInfo->wep[i];
		const CrosshairWepInfo *cmpCrh = &xhairInfo->wep[iCompIdx];

		const int iUseXHairIdx = UseCrosshairIndexFor(xhairInfo, i);
		if (iUseXHairIdx != i)
		{
			if (eSerialMode == SERIALMODE_DESERIALIZE)
			{
				V_memcpy(crh, &xhairInfo->wep[iUseXHairIdx], sizeof(CrosshairWepInfo));
			}
			continue;
		}

		if (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V29)
		{
			crh->flags = SerialInt(crh->flags, cmpCrh->flags, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_FLAG__HIGHESTFLAG);
		}
		crh->iStyle = SerialInt(crh->iStyle, cmpCrh->iStyle, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_STYLE__TOTAL - 1);
		crh->color.SetRawColor(SerialInt(crh->color.GetRawColor(), cmpCrh->color.GetRawColor(), eCompMode, szMutSeq, &ctx));

		if (bNotCompact || crh->iStyle == CROSSHAIR_STYLE_CUSTOM)
		{
			crh->eSizeType = static_cast<NeoHudCrosshairSizeType>(SerialInt(crh->eSizeType, cmpCrh->eSizeType, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_SIZETYPE__TOTAL - 1));
			if (bNotCompact || crh->eSizeType == CROSSHAIR_SIZETYPE_ABSOLUTE)
			{
				crh->iSize = SerialInt(crh->iSize, cmpCrh->iSize, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_SIZE);
			}
			if (bNotCompact || crh->eSizeType == CROSSHAIR_SIZETYPE_SCREEN)
			{
				crh->flScrSize = SerialFloat(crh->flScrSize, cmpCrh->flScrSize, eCompMode, szMutSeq, &ctx, 0.0f, 1.0f);
			}
			crh->iThick = SerialInt(crh->iThick, cmpCrh->iThick, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_THICKNESS);
			crh->iGap = SerialInt(crh->iGap, cmpCrh->iGap, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_GAP);
			crh->iOutline = SerialInt(crh->iOutline, cmpCrh->iOutline, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_OUTLINE);
			crh->iCenterDot = SerialInt(crh->iCenterDot, cmpCrh->iCenterDot, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_CENTER_DOT);
			if (iSerialVersion < NEOXHAIR_SERIAL_ALPHA_V29)
			{
				const bool bTopLine = SerialBool(!(crh->flags & CROSSHAIR_FLAG_NOTOPLINE), !(cmpCrh->flags & CROSSHAIR_FLAG_NOTOPLINE), eCompMode, szMutSeq, &ctx);
				if (!bTopLine)
				{
					crh->flags |= CROSSHAIR_FLAG_NOTOPLINE;
				}
			}
			crh->iCircleRad = SerialInt(crh->iCircleRad, cmpCrh->iCircleRad, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_CIRCLE_RAD);
			if (bNotCompact || crh->iCircleRad > 0)
			{
				crh->iCircleSegments = SerialInt(crh->iCircleSegments, cmpCrh->iCircleSegments, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_MAX_CIRCLE_SEGMENTS);
			}
			
			if (iSerialVersion < NEOXHAIR_SERIAL_ALPHA_V19)
			{
				continue;
			}
			// >= NEOXHAIR_SERIAL_ALPHA_V19 segments

			crh->eDynamicType = (iSerialVersion >= NEOXHAIR_SERIAL_ALPHA_V19)
					? static_cast<NeoHudCrosshairDynamicType>(SerialInt(crh->eDynamicType, cmpCrh->eDynamicType, eCompMode, szMutSeq, &ctx, 0, CROSSHAIR_DYNAMICTYPE__TOTAL - 1))
					: CROSSHAIR_DYNAMICTYPE_NONE;

			if (iSerialVersion < NEOXHAIR_SERIAL_ALPHA_V22)
			{
				continue;
			}
			// >= NEOXHAIR_SERIAL_ALPHA_V22 segments

			if (bNotCompact || crh->iCenterDot > 0)
			{
				if (iSerialVersion < NEOXHAIR_SERIAL_ALPHA_V29)
				{
					const bool bSeparateColorDot = SerialBool((crh->flags & CROSSHAIR_FLAG_SEPERATEDOTCOLOR), (cmpCrh->flags & CROSSHAIR_FLAG_SEPERATEDOTCOLOR), eCompMode, szMutSeq, &ctx);
					if (bSeparateColorDot)
					{
						crh->flags |= CROSSHAIR_FLAG_SEPERATEDOTCOLOR;
					}
				}
				if (bNotCompact || (crh->flags & CROSSHAIR_FLAG_SEPERATEDOTCOLOR))
				{
					crh->colorDot.SetRawColor(SerialInt(crh->colorDot.GetRawColor(), cmpCrh->colorDot.GetRawColor(), eCompMode, szMutSeq, &ctx));
					if (bNotCompact || crh->iOutline > 0)
					{
						crh->colorDotOutline.SetRawColor(SerialInt(crh->colorDotOutline.GetRawColor(), cmpCrh->colorDotOutline.GetRawColor(), eCompMode, szMutSeq, &ctx));
					}
				}
			}
			if (bNotCompact || crh->iOutline > 0)
			{
				crh->colorOutline.SetRawColor(SerialInt(crh->colorOutline.GetRawColor(), cmpCrh->colorOutline.GetRawColor(), eCompMode, szMutSeq, &ctx));
			}
		}
	}

	// Further compress with RLE
	SerialRLEncode(szMutSeq, ctx.eSerialMode);

	return true;
}

bool ImportCrosshair(CrosshairInfo *xhairInfo, const char *pszSequence,
		EHipfireOpt (*paeHipfireOpts)[CROSSHAIR_WEP__TOTAL])
{
	const int iSeqSize = V_strlen(pszSequence);
	if (iSeqSize <= 0 || iSeqSize > NEO_XHAIR_SEQMAX)
	{
		return false;
	}

	char szMutSeq[NEO_XHAIR_SEQMAX];
	V_memcpy(szMutSeq, pszSequence, sizeof(char) * iSeqSize);
	szMutSeq[iSeqSize] = '\0';

	ResetCrosshairToDefault(xhairInfo, paeHipfireOpts);
	const bool bValid = ImportOrExportCrosshair(SERIALMODE_DESERIALIZE, xhairInfo, szMutSeq, iSeqSize, NEOXHAIR_SERIAL_CURRENT);
	if (!bValid)
	{
		return false;
	}

	if (paeHipfireOpts)
	{
		for (int i = CROSSHAIR_WEP_DEFAULT_HIPFIRE; i < CROSSHAIR_WEP__TOTAL; ++i)
		{
			const NeoCrosshairWepFlags wepMask = (1 << (i - 1));
			const NeoCrosshairHipfireCustomFlags wepHipfireMask = (1 << (i - CROSSHAIR_WEP_DEFAULT_HIPFIRE));
			// NeoCrosshairHipfireCustomFlags flag equiv. only sets when NeoCrosshairWepFlags flag equiv. is set
			Assert(false == ((false == (xhairInfo->wepFlags & wepMask)) && (xhairInfo->hipfireFlags & wepHipfireMask)));
			(*paeHipfireOpts)[i] = static_cast<EHipfireOpt>(static_cast<bool>(xhairInfo->wepFlags & wepMask) + static_cast<bool>(xhairInfo->hipfireFlags & wepHipfireMask));
		}
	}

	return true;
}

void ExportCrosshair(CrosshairInfo *xhairInfo, char (&szSequence)[NEO_XHAIR_SEQMAX]
#ifdef UNIT_TEST_DLL
		, const int iExportSerialVersion
#endif
		)
{
#ifndef UNIT_TEST_DLL
	static constexpr int iExportSerialVersion = NEOXHAIR_SERIAL_CURRENT;
#endif
	szSequence[0] = '\0';
	ImportOrExportCrosshair(SERIALMODE_SERIALIZE, xhairInfo, szSequence, NEO_XHAIR_SEQMAX
			, iExportSerialVersion);
}

