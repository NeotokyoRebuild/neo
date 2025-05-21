#include "neo_hud_crosshair.h"
#include "neo_gamerules.h"

#include "filesystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "ui/neo_root.h"

ConVar cl_neo_crosshair_style("cl_neo_crosshair_style", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the crosshair style. 0 = default, 0 to 1 = textured, 2 = custom", true, 0.0f, true, CROSSHAIR_STYLE__TOTAL);
ConVar cl_neo_crosshair_color_r("cl_neo_crosshair_color_r", "255", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the red value of the crosshair color", true, 0.0f, true, UCHAR_MAX);
ConVar cl_neo_crosshair_color_g("cl_neo_crosshair_color_g", "255", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the green value of the crosshair color", true, 0.0f, true, UCHAR_MAX);
ConVar cl_neo_crosshair_color_b("cl_neo_crosshair_color_b", "255", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the blue value of the crosshair color", true, 0.0f, true, UCHAR_MAX);
ConVar cl_neo_crosshair_color_a("cl_neo_crosshair_color_a", "255", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the alpha value of the crosshair color", true, 0.0f, true, UCHAR_MAX);
ConVar cl_neo_crosshair_size("cl_neo_crosshair_size", "15", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the size of the crosshair (custom only)", true, 0.0f, true, CROSSHAIR_MAX_SIZE);
ConVar cl_neo_crosshair_size_screen("cl_neo_crosshair_size_screen", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the size of the crosshair by half-screen scale (custom only)", true, 0.0f, true, 1.0f);
ConVar cl_neo_crosshair_size_type("cl_neo_crosshair_size_type", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the size unit used for the crosshair. 0 = cl_neo_crosshair_size, 1 = cl_neo_crosshair_size_screen (custom only)", true, 0.0f, true, (CROSSHAIR_SIZETYPE__TOTAL - 1));
ConVar cl_neo_crosshair_thickness("cl_neo_crosshair_thickness", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the thickness of the crosshair (custom only)", true, 0.0f, true, CROSSHAIR_MAX_THICKNESS);
ConVar cl_neo_crosshair_gap("cl_neo_crosshair_gap", "5", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the gap of the crosshair (custom only)", true, 0.0f, true, CROSSHAIR_MAX_GAP);
ConVar cl_neo_crosshair_outline("cl_neo_crosshair_outline", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the crosshair outline (custom only)", true, 0.0f, true, CROSSHAIR_MAX_OUTLINE);
ConVar cl_neo_crosshair_center_dot("cl_neo_crosshair_center_dot", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the size of the center dot (custom only)", true, 0.0f, true, CROSSHAIR_MAX_CENTER_DOT);
ConVar cl_neo_crosshair_top_line("cl_neo_crosshair_top_line", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set if the top line will be shown (custom only)", true, 0.0f, true, 1.0f);
ConVar cl_neo_crosshair_circle_radius("cl_neo_crosshair_circle_radius", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the circle radius of the crosshair (custom only)", true, 0.0f, true, CROSSHAIR_MAX_CIRCLE_RAD);
ConVar cl_neo_crosshair_circle_segments("cl_neo_crosshair_circle_segments", "30", FCVAR_ARCHIVE | FCVAR_USERINFO, "Set the segments of the circle (custom only)", true, 0.0f, true, CROSSHAIR_MAX_CIRCLE_SEGMENTS);

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

	NEOXHAIR_SERIAL__LATESTPLUSONE,
	NEOXHAIR_SERIAL_CURRENT = NEOXHAIR_SERIAL__LATESTPLUSONE - 1,
};
static constexpr unsigned int MAGIC_NUMBER = 0x15AF104B;

void ImportCrosshair(CrosshairInfo *crh, const char *szFullpath)
{
	if (!szFullpath)
	{
		return;
	}

	CUtlBuffer buf(0, 0, CUtlBuffer::READ_ONLY);

	if (!filesystem->ReadFile(szFullpath, nullptr, buf))
	{
		Msg("[CROSSHAIR]: Failed to read file: %s", szFullpath);
		return;
	}

	if (buf.GetUnsignedInt() != MAGIC_NUMBER)
	{
		return;
	}

	const int version = buf.GetInt();
	if (version < 1 || version > NEOXHAIR_SERIAL_CURRENT)
	{
		Msg("[CROSSHAIR]: File corrupted, invalid version %d", version);
		return;
	}

	crh->color.SetRawColor(buf.GetInt());
	crh->iESizeType = buf.GetInt();
	crh->iSize = buf.GetInt();
	crh->flScrSize = buf.GetFloat();
	crh->iThick = buf.GetInt();
	crh->iGap = buf.GetInt();
	crh->iOutline = buf.GetInt();
	crh->iCenterDot = buf.GetInt();
	crh->bTopLine = buf.GetChar();
	crh->iCircleRad = buf.GetInt();
	crh->iCircleSegments = buf.GetInt();

	g_pNeoRoot->m_ns.bModified = true;
}

void ExportCrosshair(CrosshairInfo *crh, const char *szFullpath)
{
	if (!szFullpath)
	{
		return;
	}

	CUtlBuffer buf;
	buf.PutUnsignedInt(MAGIC_NUMBER);
	buf.PutInt(NEOXHAIR_SERIAL_CURRENT);

	buf.PutInt(crh->color.GetRawColor());
	buf.PutInt(crh->iESizeType);
	buf.PutInt(crh->iSize);
	buf.PutFloat(crh->flScrSize);
	buf.PutInt(crh->iThick);
	buf.PutInt(crh->iGap);
	buf.PutInt(crh->iOutline);
	buf.PutInt(crh->iCenterDot);
	buf.PutChar(crh->bTopLine);
	buf.PutInt(crh->iCircleRad);
	buf.PutInt(crh->iCircleSegments);

	if (!filesystem->WriteFile(szFullpath, nullptr, buf))
	{
		Msg("[CROSSHAIR]: Failed to write file: %s", szFullpath);
	}
}
