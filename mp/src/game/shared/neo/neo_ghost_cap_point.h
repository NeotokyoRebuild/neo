#ifndef NEO_GHOST_CAP_POINT_H
#define NEO_GHOST_CAP_POINT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity_shared.h"
#include "baseplayer_shared.h"
#include "neo_gamerules.h"

#ifdef GAME_DLL
#include "entityoutput.h"
#endif

#ifdef CLIENT_DLL
#include "iclientmode.h"
#include "hudelement.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>

#include <vgui/ILocalize.h>
#include "tier3/tier3.h"
#include "vphysics_interface.h"
#include "c_neo_player.h"
#include "ienginevgui.h"
#endif

#ifdef CLIENT_DLL
#define CNEOGhostCapturePoint C_NEOGhostCapturePoint
#endif

#ifdef CLIENT_DLL
// In seconds, how often should the client think to update capzone graphics info.
// This is *not* the framerate, only info like which team it belongs to, etc.
// Actual rendering happens in the element's Paint() loop.
#define NEO_GHOSTCAP_GRAPHICS_THINK_INTERVAL 1

ConVar neo_ghost_cap_point_hud_scale_factor("neo_ghost_cap_point_hud_scale_factor", "0.33", FCVAR_USERINFO,
	"Ghost cap HUD element scaling factor", true, 0.01, false, 0);

class CNEOHud_GhostCapPoint : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostCapPoint, Panel);

public:
	CNEOHud_GhostCapPoint(const char *pElementName, vgui::Panel *parent = NULL)
		: CHudElement(pElementName), Panel(parent, pElementName)
	{
		m_iPosX = m_iPosY = 0;
		m_iCapTexWidth = m_iCapTexHeight = 0;
		m_iMyTeam = TEAM_INVALID;

		m_flCapTexScale = 1.0f;
		m_flMyRadius = 0;

		memset(m_szMarkerText, 0, sizeof(m_szMarkerText));
		memset(m_wszMarkerTextUnicode, 0, sizeof(m_wszMarkerTextUnicode));

		m_vecMyPos = vec3_origin;

		SetAutoDelete(true);

#ifdef CLIENT_DLL
		vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
			enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme_Neo.res", "ClientScheme_Neo");
		SetScheme(neoscheme);
#endif

		if (parent)
		{
			SetParent(parent);
		}
		else
		{
			SetParent(g_pClientMode->GetViewport());
		}

		vgui::surface()->GetScreenSize(m_iPosX, m_iPosY);
		SetBounds(0, 0, m_iPosX, m_iPosY);

		// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
#ifdef CLIENT_DLL
		vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
		if (!scheme) {
			Assert(scheme);
			Error("CNEOHud_GhostCapPoint: Failed to load neoscheme\n");
		}
		m_hFont = scheme->GetFont("NHudOCRSmall");
#endif

		m_hCapTex = vgui::surface()->CreateNewTextureID();
		Assert(m_hCapTex > 0);
		vgui::surface()->DrawSetTextureFile(m_hCapTex, "vgui/hud/ctg/g_beacon_arrow_down", 1, false);
		vgui::surface()->DrawGetTextureSize(m_hCapTex, m_iCapTexWidth, m_iCapTexHeight);

		SetVisible(false);
	}

	virtual void Paint()
	{
		SetFgColor(COLOR_TRANSPARENT);
		SetBgColor(COLOR_TRANSPARENT);

		BaseClass::Paint();

		const Color jinColor = Color(38, 127, 0, 255),
			nsfColor = Color(0, 38, 127, 255),
			redColor = Color(255, 0, 0, 255);

		const Color *targetColor = (m_iMyTeam == TEAM_JINRAI) ? &jinColor : &nsfColor;

		C_NEO_Player *player = C_NEO_Player::GetLocalNEOPlayer();

		const bool playerIsPlaying = (player->GetTeamNumber() == TEAM_JINRAI || player->GetTeamNumber() == TEAM_NSF);
		if (playerIsPlaying)
		{
			if (player->GetTeamNumber() != m_iMyTeam)
			{
				targetColor = &redColor;
			}
		}

#if(0)
		const Vector dir = player->GetAbsOrigin() - m_vecMyPos;
		const float distScale = dir.Length2D();
#endif

		int x, y;
		GetVectorInScreenSpace(m_vecMyPos, x, y);

		const float scale = neo_ghost_cap_point_hud_scale_factor.GetFloat();

		const int offset_X = x - ((m_iCapTexWidth / 2) * scale);
		const int offset_Y = y - ((m_iCapTexHeight / 2) * scale);

#ifdef CLIENT_DLL
		if (playerIsPlaying)
		{
			const float distance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(m_vecMyPos);
			if (distance > 0.2)
			{
				// TODO (nullsystem): None of this is particularly efficient, but it works so
				V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "RETRIEVAL ZONE DISTANCE: %.0f m", distance);
				g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));

				int xWide = 0;
				int yTall = 0;
				vgui::surface()->GetTextSize(m_hFont, m_wszMarkerTextUnicode, xWide, yTall);
				vgui::surface()->DrawSetColor(COLOR_TRANSPARENT);
				vgui::surface()->DrawSetTextColor(COLOR_TINTGREY);
				vgui::surface()->DrawSetTextFont(m_hFont);
				vgui::surface()->DrawSetTextPos(x - (xWide / 2), offset_Y + (m_iCapTexHeight * scale) + (yTall / 2));
				vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, sizeof(m_szMarkerText));
			}
		}
#endif

		vgui::surface()->DrawSetColor(*targetColor);
		vgui::surface()->DrawSetTexture(m_hCapTex);
		vgui::surface()->DrawTexturedRect(
			offset_X,
			offset_Y,
			offset_X + (m_iCapTexWidth * scale),
			offset_Y + (m_iCapTexHeight * scale));
	}

	void SetTeam(int team) { m_iMyTeam = team; }
	void SetRadius(float radius) { m_flMyRadius = radius; }
	void SetPos(const Vector &pos) { m_vecMyPos = pos; }

private:
	int m_iPosX, m_iPosY;
	int m_iCapTexWidth, m_iCapTexHeight;

	float m_flCapTexScale;

	int m_iMyTeam;
	int m_flMyRadius;

	char m_szMarkerText[64 + 1];
	wchar_t m_wszMarkerTextUnicode[64 + 1];

	Vector m_vecMyPos;

	vgui::HFont m_hFont;

	vgui::HTexture m_hCapTex;

private:
	CNEOHud_GhostCapPoint(const CNEOHud_GhostCapPoint &other);
};
#endif

class CNEOGhostCapturePoint : public CBaseEntity
{
	DECLARE_CLASS(CNEOGhostCapturePoint, CBaseEntity);
	//DECLARE_NETWORKCLASS();

public:
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS();
	//DECLARE_PREDICTABLE();
#else
	DECLARE_SERVERCLASS();
#endif
	DECLARE_DATADESC();

	CNEOGhostCapturePoint();
	virtual ~CNEOGhostCapturePoint();
	
	virtual void Precache(void);
	virtual void Spawn(void);

#ifdef CLIENT_DLL
	virtual void ClientThink(void);
#endif

	void SetActive(bool isActive);
	void ResetCaptureState()
	{
		m_bGhostHasBeenCaptured = false;
		m_iSuccessfulCaptorClientIndex = 0;
	}

#ifdef GAME_DLL
	int ShouldTransmit(const CCheckTransmitInfo *pInfo) { return EF_BRIGHTLIGHT; }

	bool IsGhostCaptured(int &outTeamNumber, int &outCaptorClientIndex);
#endif

#ifdef GAME_DLL
	void Think_CheckMyRadius(void); // NEO FIXME (Rain): this should be private
#endif

private:
	inline void UpdateVisibility(void);

private:
#ifdef GAME_DLL
	CNetworkVar(int, m_iOwningTeam);
	CNetworkVar(int, m_iSuccessfulCaptorClientIndex);

	CNetworkVar(float, m_flCapzoneRadius);

	CNetworkVar(bool, m_bGhostHasBeenCaptured);
	CNetworkVar(bool, m_bIsActive);
#else
	int m_iOwningTeam;
	int m_iSuccessfulCaptorClientIndex;

	float m_flCapzoneRadius;

	bool m_bGhostHasBeenCaptured;
	bool m_bIsActive;
#endif

#ifdef CLIENT_DLL
	CNEOHud_GhostCapPoint *m_pHUDCapPoint;
#endif
};

#endif // NEO_GHOST_CAP_POINT_H
