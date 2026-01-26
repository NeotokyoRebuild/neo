#include "neo_hud_worldpos_marker_generic.h"
#include "neo_worldpos_marker.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>

#include "c_neo_player.h"
#include "neo_gamerules.h"

extern ConVar neo_ghost_cap_point_hud_scale_factor;
extern ConVar cl_neo_hud_center_ghost_cap_size;

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR( WorldPosMarker_Generic, 0 )

CNEOHud_WorldPosMarker_Generic::CNEOHud_WorldPosMarker_Generic( const char *pElementName, C_NEOWorldPosMarkerEnt *src, vgui::Panel *parent )
	: CNEOHud_WorldPosMarker (pElementName, parent )
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_WORLDPOS_MARKER_ENT;

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

	m_pSource = src;

	for ( int i = 0; i < MAX_SCREEN_OVERLAYS; i++ )
	{
		if ( m_pSource->m_iszSpriteNames[i][0] )
		{
			m_hSprite[i] = vgui::surface()->CreateNewTextureID();
			Assert(m_hSprite[i] > 0);
			vgui::surface()->DrawSetTextureFile( m_hSprite[i], m_pSource->m_iszSpriteNames[i], 1, false );
		}
	}

	SetVisible(false);
}

void CNEOHud_WorldPosMarker_Generic::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall");

	vgui::surface()->GetScreenSize(m_iPosX, m_iPosY);
	SetBounds(0, 0, m_iPosX, m_iPosY);

	const int widerAxis = max(m_viewWidth, m_viewHeight);
	m_viewCentreSize = widerAxis * (cl_neo_hud_center_ghost_cap_size.GetFloat() / 100.0f);
}

void CNEOHud_WorldPosMarker_Generic::UpdateStateForNeoHudElementDraw()
{
	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	if ( !player || !m_pSource )
	{
		return;
	}
	
	m_vecMyPos = m_pSource->GetAbsOrigin();
	m_flDistance = METERS_PER_INCH * player->GetAbsOrigin().DistTo( m_vecMyPos );

	g_pVGuiLocalize->ConvertANSIToUnicode( m_pSource->m_szText, m_wszMarkerTextUnicode, sizeof( m_wszMarkerTextUnicode ) );
	if ( m_wszMarkerTextUnicode[0] != L'\0' && m_pSource->m_bShowDistance )
	{
		wchar_t wszTemp[ARRAYSIZE(m_wszMarkerTextUnicode)];
		V_wcsncpy( wszTemp, m_wszMarkerTextUnicode, sizeof( wszTemp ) );

		V_snwprintf( m_wszMarkerTextUnicode, ARRAYSIZE( m_wszMarkerTextUnicode ), L"%ls %.0f m", wszTemp, m_flDistance );
	}
}

void CNEOHud_WorldPosMarker_Generic::DrawNeoHudElement()
{
	if ( !ShouldDraw() )
	{
		return;
	}

	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	const int playerTeam = player->GetTeamNumber();

	const color32 rCol = m_pSource->GetRenderColor();
	Color targetColor( rCol.r, rCol.g, rCol.b, rCol.a );
	
	const bool playerIsPlaying = ( playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF );

	int x, y;
	GetVectorInScreenSpace(m_vecMyPos, x, y);

	const float scale = neo_ghost_cap_point_hud_scale_factor.GetFloat() * ( m_pSource->m_flScale / 0.5f );

	constexpr float HALF_BASE_TEX_LENGTH = 64;
	if ( playerIsPlaying && m_wszMarkerTextUnicode[0] != L'\0' )
	{
		const float fadeTextMultiplier = GetFadeValueTowardsScreenCentre(x, y);
		if (m_flDistance > 0.2f && fadeTextMultiplier > 0.001f)
		{
			int xWide = 0;
			int yTall = 0;
			vgui::surface()->GetTextSize(m_hFont, m_wszMarkerTextUnicode, xWide, yTall);
			vgui::surface()->DrawSetColor(COLOR_TRANSPARENT);
			vgui::surface()->DrawSetTextColor(FadeColour(COLOR_TINTGREY, fadeTextMultiplier));
			vgui::surface()->DrawSetTextFont(m_hFont);
			vgui::surface()->DrawSetTextPos(x - (xWide / 2), y + (HALF_BASE_TEX_LENGTH * scale));
			vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, V_wcslen(m_wszMarkerTextUnicode));
		}
	}

	vgui::surface()->DrawSetTexture( m_hSprite[m_pSource->m_iCurrentSprite] );
	if ( m_pSource->m_bCapzoneEffect )
	{
		for (int i = 0; i < 4; i++) {
			m_fMarkerScalesCurrent[i] = (remainder(gpGlobals->curtime, 2) / 2) + 0.5 + m_fMarkerScalesStart[i];
			if (m_fMarkerScalesCurrent[i] > 1)
				m_fMarkerScalesCurrent[i] -= 1;

			int alpha = 32;
			if (m_fMarkerScalesCurrent[i] > 0.5)
				alpha *= ((0.5 - (m_fMarkerScalesCurrent[i] - 0.5)) * 2);

			targetColor[3] = alpha;
			vgui::surface()->DrawSetColor(targetColor);

			const float halfArrowLength = HALF_BASE_TEX_LENGTH * m_fMarkerScalesCurrent[i] * scale;
			vgui::surface()->DrawTexturedRect(
				x - halfArrowLength,
				y - halfArrowLength,
				x + halfArrowLength,
				y + halfArrowLength);
		}

		float alpha6 = remainder(gpGlobals->curtime, 6) + 3;
		alpha6 = alpha6 / 6;
		if (alpha6 > 0.5)
			alpha6 = 1 - alpha6;
		alpha6 = 255 * alpha6;

		targetColor[3] = alpha6;
	}

	const float halfArrowLength = HALF_BASE_TEX_LENGTH * scale;
	vgui::surface()->DrawSetColor( targetColor );
	vgui::surface()->DrawTexturedRect(
		x - halfArrowLength,
		y - halfArrowLength,
		x + halfArrowLength,
		y + halfArrowLength);
}

void CNEOHud_WorldPosMarker_Generic::Paint()
{
	SetFgColor( COLOR_TRANSPARENT );
	SetBgColor( COLOR_TRANSPARENT );
	BaseClass::Paint();
	PaintNeoElement();
}

bool CNEOHud_WorldPosMarker_Generic::ShouldDraw()
{
	if ( !BaseClass::ShouldDraw() || NEORules()->IsRoundOver() || !m_pSource || !m_pSource->m_bEnabled )
	{
		return false;
	}
	return true;
}
