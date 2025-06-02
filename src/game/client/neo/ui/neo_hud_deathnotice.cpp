//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"

#include "neo_hud_game_event.h"
#include "neo_player_shared.h"
#include "neo_hud_childelement.h"
#include "spectatorgui.h"
#include "takedamageinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "20", 0 );

// Player entries in a death notice
struct DeathNoticePlayer
{
	wchar_t		szName[MAX_PLAYER_NAME_LENGTH];
	int			iNameLength = 0;
	int			iEntIndex = 0;
	int			iTeam = 0;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticePlayer	Killer;
	bool				bRankChange = false;
	wchar_t				szOldRank[2];
	wchar_t				szNewRank[2];
	bool				bRankUp = false;
	bool				bGhostCap = false;
	bool				bVIPExtract = false;
	bool				bVIPDeath = false;
	DeathNoticePlayer	Assist;
	wchar_t				szDeathIcon[2];
	bool				bExplosive = false;
	bool				bSuicide = false;
	bool				bHeadshot = false;
	DeathNoticePlayer   Victim;
	bool				bWasCarryingGhost = false;
	bool				bInvolved = false;

	int					iLength = 0;
	int					iHeight = 0;
	float				flHideTime = 0.f;
};

using vgui::surface;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNEOHud_DeathNotice : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_DeathNotice, Panel );
public:
	CNEOHud_DeathNotice( const char *pElementName, vgui::Panel* parent = NULL);

	void Init( void );
	void VidInit( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void ) override;
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

protected:
	void SetColorForNoticePlayer( int iTeamNumber );
	void RetireExpiredDeathNotices( void );
	
	virtual void FireGameEvent( IGameEvent * event );
	void AddPlayerDeath(IGameEvent* event);
	void AddPlayerRankChange(IGameEvent* event);
	void AddPlayerGhostCapture(IGameEvent* event);
	void AddVIPExtract(IGameEvent* event);
	void AddVIPDeath(IGameEvent* event);

	void SetDeathNoticeItemDimensions(DeathNoticeItem* deathNoticeItem);

	void DrawCommon(int index);
	void DrawPlayerDeath(int index);
	void DrawPlayerRankChange(int index);
	void DrawPlayerGhostCapture(int index);
	void DrawVIPExtract(int index);
	void DrawVIPDeath(int index);

	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:

	CPanelAnimationVarAliasType( int, m_iLineMargin, "LineMargin", "2", "proportional_ypos");
	CPanelAnimationVar( int, m_iMaxDeathNotices, "MaxDeathNotices", "8" );
	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );
	CPanelAnimationVar( Color, m_BackgroundColour, "BackgroundColour", "200 200 200 40");
	CPanelAnimationVar( Color, m_BackgroundColourInvolved, "BackgroundColourInvolved", "50 50 50 200");

	CUtlVector<DeathNoticeItem> m_DeathNotices;
};

DECLARE_NAMED_HUDELEMENT( CNEOHud_DeathNotice, neo_death_notice);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR( DeathNotice, 0.1 )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNEOHud_DeathNotice::CNEOHud_DeathNotice( const char *pElementName, vgui::Panel* parent) :
	CHudElement( pElementName ), Panel(parent, pElementName)
{
	if (parent) {
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	SetHiddenBits( HIDEHUD_MISCSTATUS );
	SetSize( ScreenWidth(), ScreenHeight() );
}

void CNEOHud_DeathNotice::UpdateStateForNeoHudElementDraw()
{
	
}

enum NeoHudDeathNoticeIcon : wchar_t
{
	NEO_HUD_DEATHNOTICEICON_EXPLODE = '!',
	NEO_HUD_DEATHNOTICEICON_GUN,
	NEO_HUD_DEATHNOTICEICON_HEADSHOT,
	NEO_HUD_DEATHNOTICEICON_KILL,
	NEO_HUD_DEATHNOTICEICON_MELEE,
	NEO_HUD_DEATHNOTICEICON_SHORTBUS,

	NEO_HUD_DEATHNOTICEICON_RANKUP = '(',
	NEO_HUD_DEATHNOTICEICON_RANKDOWN,
	NEO_HUD_DEATHNOTICEICON_RANKLESS_DOG,
	NEO_HUD_DEATHNOTICEICON_PVT,
	NEO_HUD_DEATHNOTICEICON_CPL,
	NEO_HUD_DEATHNOTICEICON_SGT,
	NEO_HUD_DEATHNOTICEICON_LT,

	NEO_HUD_DEATHNOTICEICON_AA12 = '0',
	NEO_HUD_DEATHNOTICEICON_GHOST,
	NEO_HUD_DEATHNOTICEICON_GRENADE,
	NEO_HUD_DEATHNOTICEICON_JITTE,
	NEO_HUD_DEATHNOTICEICON_JITTESCOPED,
	NEO_HUD_DEATHNOTICEICON_KNIFE,
	NEO_HUD_DEATHNOTICEICON_KYLA,
	NEO_HUD_DEATHNOTICEICON_M41,
	NEO_HUD_DEATHNOTICEICON_M41L,
	NEO_HUD_DEATHNOTICEICON_M41S,
	NEO_HUD_DEATHNOTICEICON_MILSO,
	NEO_HUD_DEATHNOTICEICON_MPN,
	NEO_HUD_DEATHNOTICEICON_MPN_UNSUPRESSED,
	NEO_HUD_DEATHNOTICEICON_MX,
	NEO_HUD_DEATHNOTICEICON_MX_SILENCED,
	NEO_HUD_DEATHNOTICEICON_PBK56S,
	NEO_HUD_DEATHNOTICEICON_PZ,
	NEO_HUD_DEATHNOTICEICON_REMOTEDET,
	NEO_HUD_DEATHNOTICEICON_SMAC,
	NEO_HUD_DEATHNOTICEICON_SMOKEGRENADE,
	NEO_HUD_DEATHNOTICEICON_SRM,
	NEO_HUD_DEATHNOTICEICON_SRM_S,
	NEO_HUD_DEATHNOTICEICON_SRS,
	NEO_HUD_DEATHNOTICEICON_SUPA7,
	NEO_HUD_DEATHNOTICEICON_ZR68C,
	NEO_HUD_DEATHNOTICEICON_ZR68L,
	NEO_HUD_DEATHNOTICEICON_ZR68S,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_DeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );
	
    SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_DeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_rankchange" );
	ListenForGameEvent( "ghost_capture" );
	ListenForGameEvent( "vip_extract" );
	ListenForGameEvent( "vip_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_DeathNotice::VidInit( void )
{
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CNEOHud_DeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_DeathNotice::SetColorForNoticePlayer( int iTeamNumber )
{
	surface()->DrawSetTextColor( GameResources()->GetTeamColor( iTeamNumber ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_DeathNotice::DrawNeoHudElement()
{
	int iCount = m_DeathNotices.Count();
	for (int i = 0; i < iCount; i++)
	{
		if (m_DeathNotices[i].bRankChange)
		{
			DrawPlayerRankChange(i);
		}
		else if (m_DeathNotices[i].bGhostCap)
		{
			DrawPlayerGhostCapture(i);
		}
		else if (m_DeathNotices[i].bVIPExtract)
		{
			DrawVIPExtract(i);
		}
		else if (m_DeathNotices[i].bVIPDeath)
		{
			DrawVIPDeath(i);
		}
		else
		{
			DrawPlayerDeath(i);
		}
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

constexpr int ASSIST_SEPARATOR_LENGTH = 4;
constexpr wchar_t ASSIST_SEPARATOR[ASSIST_SEPARATOR_LENGTH] = L" + ";

constexpr int GHOST_CAPTURE_TEXT_LENGTH = 19;
constexpr wchar_t GHOST_CAPTURE_TEXT[GHOST_CAPTURE_TEXT_LENGTH] = L" has captured the ";

constexpr int GHOST_CAPTURE_TEXT_2_LENGTH = 19;
constexpr wchar_t GHOST_CAPTURE_TEXT_2[GHOST_CAPTURE_TEXT_LENGTH] = L" has been captured";

constexpr int VIP_TEXT_LENGTH = 9;
constexpr wchar_t VIP_TEXT[VIP_TEXT_LENGTH] = L"The VIP ";

constexpr int VIP_EXTRACT_TEXT_LENGTH = 14;
constexpr wchar_t VIP_EXTRACT_TEXT[VIP_EXTRACT_TEXT_LENGTH] = L"has extracted";

constexpr int VIP_DEATH_TEXT_LENGTH = 20;
constexpr wchar_t VIP_DEATH_TEXT[VIP_DEATH_TEXT_LENGTH] = L" has killed the VIP";

constexpr int VIP_DEATH_TEXT_2_LENGTH = 9;
constexpr wchar_t VIP_DEATH_TEXT_2[VIP_DEATH_TEXT_2_LENGTH] = L"has died";
void CNEOHud_DeathNotice::SetDeathNoticeItemDimensions(DeathNoticeItem* deathNoticeItem)
{
	int totalWidth = 0;
	int width, height = 0;

	const int spaceLength = surface()->GetCharacterWidth(g_hFontKillfeed, ' ');

	if (g_hFontKillfeed == vgui::INVALID_FONT)
	{
		surface()->GetTextSize(g_hFontTrebuchet24, L"Killfeed font missing", width, height);
		deathNoticeItem->iLength = width;
		deathNoticeItem->iHeight = height;
		return;
	}
	else if (g_hFontKillfeedIcons == vgui::INVALID_FONT)
	{
		surface()->GetTextSize(g_hFontTrebuchet24, L"KillfeedIcons font missing", width, height);
		deathNoticeItem->iLength = width;
		deathNoticeItem->iHeight = height;
		return;
	}

	if (deathNoticeItem->bRankChange)
	{	// Rank change message
		if (deathNoticeItem->Killer.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Killer.szName, width, height);
			totalWidth += width;
		}
		totalWidth += spaceLength;
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, deathNoticeItem->szOldRank[0]);
		totalWidth += spaceLength;
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, deathNoticeItem->bRankUp ? NEO_HUD_DEATHNOTICEICON_RANKUP : NEO_HUD_DEATHNOTICEICON_RANKDOWN);
		totalWidth += spaceLength;
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, deathNoticeItem->szNewRank[0]);
	}
	else if (deathNoticeItem->bGhostCap)
	{	// Ghost capture message
		if (deathNoticeItem->Killer.iEntIndex != 0)
		{	// *playername* has captured the *ghost*
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Killer.szName, width, height);
			totalWidth += width;

			surface()->GetTextSize(g_hFontKillfeed, GHOST_CAPTURE_TEXT, width, height);
			totalWidth += width;
			totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_GHOST);
		}
		else
		{	// *ghost* has been captured
			totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_GHOST);
			surface()->GetTextSize(g_hFontKillfeed, GHOST_CAPTURE_TEXT_2, width, height);
			totalWidth += width;
		}
	}
	else if (deathNoticeItem->bVIPExtract)
	{	// VIP Extract message
		surface()->GetTextSize(g_hFontKillfeed, VIP_TEXT, width, height);
		totalWidth += width;
		if (deathNoticeItem->Killer.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Killer.szName, width, height);
			totalWidth += width;
			totalWidth += spaceLength;
		}
		surface()->GetTextSize(g_hFontKillfeed, VIP_EXTRACT_TEXT, width, height);
		totalWidth += width;
	}
	else if (deathNoticeItem->bVIPDeath)
	{	// VIP Death Message
		if (deathNoticeItem->Victim.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Victim.szName, width, height);
			totalWidth += width;
			surface()->GetTextSize(g_hFontKillfeed, VIP_DEATH_TEXT, width, height);
			totalWidth += width;
		}
		else
		{
			surface()->GetTextSize(g_hFontKillfeed, VIP_TEXT, width, height);
			totalWidth += width;
			surface()->GetTextSize(g_hFontKillfeed, VIP_DEATH_TEXT_2, width, height);
			totalWidth += width;
		}
		if (deathNoticeItem->Killer.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Killer.szName, width, height);
			totalWidth += width;
			totalWidth += spaceLength;
		}
	}
	else
	{	// Player killed message
		if (deathNoticeItem->Killer.iEntIndex != 0 && !deathNoticeItem->bSuicide)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Killer.szName, width, height);
			totalWidth += width;
		}
		if (deathNoticeItem->Assist.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Assist.szName, width, height);
			totalWidth += width;
			surface()->GetTextSize(g_hFontKillfeed, ASSIST_SEPARATOR, width, height);
			totalWidth += width;
		}
		if (deathNoticeItem->Victim.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Victim.szName, width, height);
			totalWidth += width + spaceLength;
		}
		if (deathNoticeItem->bSuicide)
		{
			totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_SHORTBUS) + spaceLength;
		}
		else
		{
			surface()->GetTextSize(g_hFontKillfeedIcons, deathNoticeItem->szDeathIcon, width, height);
			totalWidth += width + spaceLength;
			if (deathNoticeItem->bExplosive)
			{
				totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_EXPLODE) + spaceLength;
			}
			if (deathNoticeItem->bHeadshot)
			{
				totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_HEADSHOT) + spaceLength;
			}
		}
		if (deathNoticeItem->bWasCarryingGhost)
		{
			totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_GHOST) + spaceLength + spaceLength;
		}
	}
	deathNoticeItem->iLength = totalWidth;
	surface()->GetTextSize(g_hFontKillfeed, ASSIST_SEPARATOR, width, height);
	deathNoticeItem->iHeight = height;
}

void CNEOHud_DeathNotice::DrawCommon(int i)
{
	int width, height;
	GetSize(width, height);
	int yStart = GetClientModeHL2MPNormal()->GetDeathMessageStartHeight();
	if (g_pSpectatorGUI->IsVisible())
	{
		yStart += g_pSpectatorGUI->GetTopBarHeight();
	}
	yStart += m_iLineMargin + (((m_iLineMargin * 2) + m_DeathNotices[i].iHeight) * i);
	int xStart = m_bRightJustify ? width - m_DeathNotices[i].iLength - (m_iLineMargin * 2) : (m_iLineMargin * 2);

	// Background
	int halfMargin = m_iLineMargin / 2;
	DrawNeoHudRoundedBox(xStart - m_iLineMargin,
		yStart - halfMargin,
		xStart + m_DeathNotices[i].iLength + m_iLineMargin,
		yStart + m_DeathNotices[i].iHeight + halfMargin,
		m_DeathNotices[i].bInvolved ? m_BackgroundColourInvolved : m_BackgroundColour,
		true, true, true, true);
	surface()->DrawSetTextPos(xStart, yStart);
}

void CNEOHud_DeathNotice::DrawPlayerDeath(int i)
{
	DrawCommon(i);

	surface()->DrawSetTextFont(g_hFontKillfeed);
	// Killer
	if (m_DeathNotices[i].Killer.iEntIndex != 0 && !m_DeathNotices[i].bSuicide)
	{
		SetColorForNoticePlayer(m_DeathNotices[i].Killer.iTeam);
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(m_DeathNotices[i].Killer.szName, m_DeathNotices[i].Killer.iNameLength);
	}

	// Assister
	if (m_DeathNotices[i].Assist.iEntIndex != 0)
	{
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(ASSIST_SEPARATOR, ASSIST_SEPARATOR_LENGTH - 1);

		SetColorForNoticePlayer(m_DeathNotices[i].Assist.iTeam);
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(m_DeathNotices[i].Assist.szName, m_DeathNotices[i].Assist.iNameLength);
	}

	// Icons
	if (!m_DeathNotices[i].bSuicide)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(L" ", 1);
		surface()->DrawSetTextFont(g_hFontKillfeedIcons);
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(m_DeathNotices[i].szDeathIcon, 1);
		if (m_DeathNotices[i].bExplosive)
		{
			surface()->DrawSetTextFont(g_hFontKillfeed);
			surface()->DrawPrintText(L" ", 1);
			surface()->DrawSetTextFont(g_hFontKillfeedIcons);
			wchar_t icon = NEO_HUD_DEATHNOTICEICON_EXPLODE;
			surface()->DrawPrintText(&icon, 1);
		}
		if (m_DeathNotices[i].bHeadshot)
		{
			surface()->DrawSetTextFont(g_hFontKillfeed);
			surface()->DrawPrintText(L" ", 1);
			surface()->DrawSetTextFont(g_hFontKillfeedIcons);
			wchar_t icon = NEO_HUD_DEATHNOTICEICON_HEADSHOT;
			surface()->DrawPrintText(&icon, 1);
		}
	}

	// Victim
	if (m_DeathNotices[i].Victim.iEntIndex != 0)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(L" ", 1);
		SetColorForNoticePlayer(m_DeathNotices[i].Victim.iTeam);
		surface()->DrawPrintText(m_DeathNotices[i].Victim.szName, m_DeathNotices[i].Victim.iNameLength);
	}

	// Victim Ghoster Icon
	if (m_DeathNotices[i].bWasCarryingGhost)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(L" ", 1);
		surface()->DrawSetTextFont(g_hFontKillfeedIcons);
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		wchar_t icon = NEO_HUD_DEATHNOTICEICON_GHOST;
		surface()->DrawPrintText(&icon, 1);
	}

	// Suicide Icon
	if (m_DeathNotices[i].bSuicide)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(L" ", 1);
		surface()->DrawSetTextFont(g_hFontKillfeedIcons);
		surface()->DrawSetTextColor(COLOR_NEO_ORANGE);
		wchar_t icon = NEO_HUD_DEATHNOTICEICON_SHORTBUS;
		surface()->DrawPrintText(&icon, 1);
	}
}

void CNEOHud_DeathNotice::DrawPlayerRankChange(int i)
{
	DrawCommon(i);

	// *Player name*
	if (m_DeathNotices[i].Killer.iEntIndex != 0)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		SetColorForNoticePlayer(m_DeathNotices[i].Killer.iTeam);
		surface()->DrawPrintText(m_DeathNotices[i].Killer.szName, m_DeathNotices[i].Killer.iNameLength);
		surface()->DrawPrintText(L" ", 1);
	}

	// *old rank*
	surface()->DrawSetTextFont(g_hFontKillfeedIcons);
	surface()->DrawSetTextColor(COLOR_NEO_WHITE);
	surface()->DrawPrintText(&m_DeathNotices[i].szOldRank[0], 1);
	surface()->DrawSetTextFont(g_hFontKillfeed);
	surface()->DrawPrintText(L" ", 1);

	// *rank change arrow*
	if (!m_DeathNotices[i].bRankUp)
	{
		surface()->DrawSetTextColor(COLOR_RED);
	}
	else
	{
		if (m_DeathNotices[i].Killer.iEntIndex != 0)
		{
			SetColorForNoticePlayer(m_DeathNotices[i].Killer.iTeam); // NEO TODO (Adam) Different colours here for deathmatch?
		}
	}
	wchar_t icon = m_DeathNotices[i].bRankUp ? NEO_HUD_DEATHNOTICEICON_RANKUP : NEO_HUD_DEATHNOTICEICON_RANKDOWN;
	surface()->DrawSetTextFont(g_hFontKillfeedIcons);
	surface()->DrawPrintText(&icon, 1);
	surface()->DrawSetTextFont(g_hFontKillfeed);
	surface()->DrawPrintText(L" ", 1);

	// *new rank*
	surface()->DrawSetTextFont(g_hFontKillfeedIcons);
	surface()->DrawSetTextColor(COLOR_NEO_WHITE);
	surface()->DrawPrintText(&m_DeathNotices[i].szNewRank[0], 1);
}

void CNEOHud_DeathNotice::DrawPlayerGhostCapture(int i)
{
	DrawCommon(i);

	surface()->DrawSetTextFont(g_hFontKillfeed);
	// *ghost capturer name*
	if (m_DeathNotices[i].Killer.iEntIndex != 0)
	{
		SetColorForNoticePlayer(m_DeathNotices[i].Killer.iTeam);
		surface()->DrawPrintText(m_DeathNotices[i].Killer.szName, m_DeathNotices[i].Killer.iNameLength);

		// has captured the
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(GHOST_CAPTURE_TEXT, GHOST_CAPTURE_TEXT_LENGTH - 1);

		// *ghost icon*
		surface()->DrawSetTextFont(g_hFontKillfeedIcons);
		wchar_t icon = NEO_HUD_DEATHNOTICEICON_GHOST;
		surface()->DrawPrintText(&icon, 1);
	}
	else
	{
		// *ghost icon*
		surface()->DrawSetTextFont(g_hFontKillfeedIcons);
		wchar_t icon = NEO_HUD_DEATHNOTICEICON_GHOST;
		surface()->DrawPrintText(&icon, 1);

		// has been captured
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(GHOST_CAPTURE_TEXT_2, GHOST_CAPTURE_TEXT_2_LENGTH - 1);
	}
}

void CNEOHud_DeathNotice::DrawVIPExtract(int i)
{
	DrawCommon(i);

	// The VIP 
	surface()->DrawSetTextColor(COLOR_NEO_WHITE);
	surface()->DrawSetTextFont(g_hFontKillfeed);
	surface()->DrawPrintText(VIP_TEXT, VIP_TEXT_LENGTH - 1);

	// *VIP name*
	if (m_DeathNotices[i].Killer.iEntIndex != 0)
	{
		SetColorForNoticePlayer(m_DeathNotices[i].Killer.iTeam);
		surface()->DrawPrintText(m_DeathNotices[i].Killer.szName, m_DeathNotices[i].Killer.iNameLength);
		surface()->DrawPrintText(L" ", 1);
	}

	// has extracted
	surface()->DrawSetTextColor(COLOR_NEO_WHITE);
	surface()->DrawPrintText(VIP_EXTRACT_TEXT, VIP_EXTRACT_TEXT_LENGTH - 1);
}

void CNEOHud_DeathNotice::DrawVIPDeath(int i)
{
	DrawCommon(i);

	surface()->DrawSetTextFont(g_hFontKillfeed);
	const bool hasKiller = m_DeathNotices[i].Killer.iEntIndex != 0;
	if (hasKiller)
	{
		// *killer name*
		SetColorForNoticePlayer(m_DeathNotices[i].Killer.iTeam);
		surface()->DrawPrintText(m_DeathNotices[i].Killer.szName, m_DeathNotices[i].Killer.iNameLength);

		// has killed the VIP
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(VIP_DEATH_TEXT, VIP_DEATH_TEXT_LENGTH - 1);
		
		// *VIP name*
		if (m_DeathNotices[i].Victim.iEntIndex != 0)
		{
			surface()->DrawPrintText(L" ", 1);
			SetColorForNoticePlayer(m_DeathNotices[i].Victim.iTeam);
			surface()->DrawPrintText(m_DeathNotices[i].Victim.szName, m_DeathNotices[i].Victim.iNameLength);
		}
	}
	else
	{
		// The VIP
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(VIP_TEXT, VIP_TEXT_LENGTH - 1);

		// *VIP name*
		if (m_DeathNotices[i].Victim.iEntIndex != 0)
		{
			SetColorForNoticePlayer(m_DeathNotices[i].Victim.iTeam);
			surface()->DrawPrintText(m_DeathNotices[i].Victim.szName, m_DeathNotices[i].Victim.iNameLength);
			surface()->DrawPrintText(L" ", 1);
		}

		// has died
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(VIP_DEATH_TEXT_2, VIP_DEATH_TEXT_2_LENGTH - 1);
	}
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CNEOHud_DeathNotice::RetireExpiredDeathNotices( void )
{
	// Loop backwards because we might remove one
	int iSize = m_DeathNotices.Size();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		if ( m_DeathNotices[i].flHideTime < gpGlobals->curtime )
		{
			m_DeathNotices.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
ConVar cl_neo_hud_extended_killfeed("cl_neo_hud_extended_killfeed", "1", FCVAR_ARCHIVE, "Show extra events in killfeed", true, 0, true, 1);
void CNEOHud_DeathNotice::FireGameEvent(IGameEvent* event)
{
	if (!g_PR)
		return;

	if (hud_deathnotice_time.GetFloat() == 0)
		return;
	
	// Do we have too many death messages in the queue?
	if (m_DeathNotices.Count() >= m_iMaxDeathNotices)
	{
		// Remove the oldest one in the queue, which will always be the first
		m_DeathNotices.Remove(0);
	}

	auto eventName = event->GetName();
	if (!Q_stricmp(eventName, "player_death"))
	{
		AddPlayerDeath(event);
	}
	else if (!cl_neo_hud_extended_killfeed.GetBool())
	{
		return;
	}
	else if (!Q_stricmp(eventName, "player_rankchange"))
	{
		AddPlayerRankChange(event);
	}
	else if (!Q_stricmp(eventName, "ghost_capture"))
	{
		AddPlayerGhostCapture(event);
	}
	else if (!Q_stricmp(eventName, "vip_extract"))
	{
		AddVIPExtract(event);
	}
	else if (!Q_stricmp(eventName, "vip_death"))
	{
		AddVIPDeath(event);
	}

}

void CNEOHud_DeathNotice::AddPlayerDeath(IGameEvent* event)
{
	// the event should be "player_death"
	const int killer = engine->GetPlayerForUserID(event->GetInt("attacker"));
	const int victim = engine->GetPlayerForUserID(event->GetInt("userid"));
	const int assistInt = event->GetInt("assists");
	const int assist = engine->GetPlayerForUserID(assistInt);
	const bool hasAssists = assist > 0;
	const char* killedwith = event->GetString("weapon");

	char fullkilledwith[128];
	if (killedwith && *killedwith)
	{
		Q_snprintf(fullkilledwith, sizeof(fullkilledwith), "death_%s", killedwith);
	}
	else
	{
		fullkilledwith[0] = 0;
	}

	// Get the names of the players
	const char* killer_name = g_PR->GetPlayerName(killer);
	const char* victim_name = g_PR->GetPlayerName(victim);
	const char* assists_name = g_PR->GetPlayerName(assist);
	if (!assists_name)
		assists_name = "";
	if (!killer_name)
		killer_name = "";
	if (!victim_name)
		victim_name = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = killer;
	deathMsg.Victim.iEntIndex = victim;
	deathMsg.Assist.iEntIndex = assist;
	g_pVGuiLocalize->ConvertANSIToUnicode(killer_name, deathMsg.Killer.szName, sizeof(deathMsg.Killer.szName));
	g_pVGuiLocalize->ConvertANSIToUnicode(victim_name, deathMsg.Victim.szName, sizeof(deathMsg.Victim.szName));
	g_pVGuiLocalize->ConvertANSIToUnicode(assists_name, deathMsg.Assist.szName, sizeof(deathMsg.Assist.szName));
	deathMsg.Killer.iNameLength = wcslen(deathMsg.Killer.szName);
	deathMsg.Victim.iNameLength = wcslen(deathMsg.Victim.szName);
	deathMsg.Assist.iNameLength = wcslen(deathMsg.Assist.szName);
	if (const auto killerTeam = GetPlayersTeam(killer))
	{
		deathMsg.Killer.iTeam = killerTeam->GetTeamNumber();
	}
	if (const auto victimTeam = GetPlayersTeam(victim))
	{
		deathMsg.Victim.iTeam = victimTeam->GetTeamNumber();
	}
	if (const auto assistTeam = GetPlayersTeam(assist))
	{
		deathMsg.Assist.iTeam = assistTeam->GetTeamNumber();
	}
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bExplosive = event->GetBool("explosive");
	deathMsg.bSuicide = event->GetBool("suicide");
	deathMsg.bHeadshot = event->GetBool("headshot");
	deathMsg.bWasCarryingGhost = event->GetBool("ghoster");
	const char* deathIcon = event->GetString("deathIcon");
	g_pVGuiLocalize->ConvertANSIToUnicode(deathIcon, deathMsg.szDeathIcon, sizeof(deathMsg.szDeathIcon));
	deathMsg.bInvolved = killer == GetLocalPlayerIndex() || victim == GetLocalPlayerIndex() || assist == GetLocalPlayerIndex();

	SetDeathNoticeItemDimensions(&deathMsg);

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);

	// Record the death notice in the console
	char sDeathMsg[512];
	if (deathMsg.bSuicide)
	{
		if (!strcmp(fullkilledwith, "d_worldspawn"))
		{
			Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%ls died.", deathMsg.Victim.szName);
		}
		else	//d_world
		{
			Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%ls suicided.", deathMsg.Victim.szName);
		}
	}
	else
	{
		// These phrases are the same as in original NT to provide plugin/stats tracking website/etc compatibility.
#define HEADSHOT_LINGO "headshot'd"
#define NON_HEADSHOT_LINGO "killed"

		Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%ls %s %ls",
			deathMsg.Killer.szName,
			(deathMsg.bHeadshot ? HEADSHOT_LINGO : NON_HEADSHOT_LINGO),
			deathMsg.Victim.szName);

		if ((*fullkilledwith != '\0') && (*fullkilledwith > 13))
		{
			Q_strncat(sDeathMsg, VarArgs(" with %s.", fullkilledwith + 6), sizeof(sDeathMsg), COPY_ALL_CHARACTERS);
		}
	}

	if (hasAssists)
	{
		Q_strncat(sDeathMsg, VarArgs(" Assists: %s.", assists_name), sizeof(sDeathMsg), COPY_ALL_CHARACTERS);
	}
	Msg("%s\n", sDeathMsg);
}

void CNEOHud_DeathNotice::AddPlayerRankChange(IGameEvent* event)
{
	// the event should be "player_rank_change"
	const int playerRankChange = engine->GetPlayerForUserID(event->GetInt("userid"));
	const int oldRank = event->GetInt("oldRank");
	const int newRank = event->GetInt("newRank");

	// Get the name of the player
	const char* playerRankChangeName = g_PR->GetPlayerName(playerRankChange);
	if (!playerRankChangeName)
		playerRankChangeName = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = playerRankChange;
	g_pVGuiLocalize->ConvertANSIToUnicode(playerRankChangeName, deathMsg.Killer.szName, sizeof(deathMsg.Killer.szName));
	deathMsg.Killer.iNameLength = wcslen(deathMsg.Killer.szName);
	if (const auto playerRankChangeTeam = GetPlayersTeam(playerRankChange))
	{
		deathMsg.Killer.iTeam = playerRankChangeTeam->GetTeamNumber();
	}
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bRankChange = true;

	deathMsg.szOldRank[0] = NEO_HUD_DEATHNOTICEICON_RANKLESS_DOG + oldRank;
	deathMsg.szNewRank[0] = NEO_HUD_DEATHNOTICEICON_RANKLESS_DOG + newRank;
	deathMsg.bRankUp = newRank > oldRank;
	deathMsg.bInvolved = deathMsg.Killer.iEntIndex == GetLocalPlayerIndex();

	SetDeathNoticeItemDimensions(&deathMsg);

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);
}

void CNEOHud_DeathNotice::AddPlayerGhostCapture(IGameEvent* event)
{
	// the event should be "player_ghost_capture"
	const int playerCapturedGhost = engine->GetPlayerForUserID(event->GetInt("userid"));

	// Get the name of the player
	const char* playerCapturedGhostName = g_PR->GetPlayerName(playerCapturedGhost);
	if (!playerCapturedGhostName)
		playerCapturedGhostName = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = playerCapturedGhost;
	g_pVGuiLocalize->ConvertANSIToUnicode(playerCapturedGhostName, deathMsg.Killer.szName, sizeof(deathMsg.Killer.szName));
	deathMsg.Killer.iNameLength = wcslen(deathMsg.Killer.szName);
	if (const auto playerCapturedGhostTeam = GetPlayersTeam(playerCapturedGhost))
	{
		deathMsg.Killer.iTeam = playerCapturedGhostTeam->GetTeamNumber();
	}
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bGhostCap = true;
	deathMsg.bInvolved = deathMsg.Killer.iEntIndex == GetLocalPlayerIndex();

	SetDeathNoticeItemDimensions(&deathMsg);

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);
}

void CNEOHud_DeathNotice::AddVIPExtract(IGameEvent* event)
{
	// the event should be "vip_extract"
	const int playerExtracted = engine->GetPlayerForUserID(event->GetInt("userid"));

	// Name of VIP
	const char* playerExtractedName = g_PR->GetPlayerName(playerExtracted);
	if (!playerExtractedName)
		playerExtractedName = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = playerExtracted;
	g_pVGuiLocalize->ConvertANSIToUnicode(playerExtractedName, deathMsg.Killer.szName, sizeof(deathMsg.Killer.szName));
	deathMsg.Killer.iNameLength = wcslen(deathMsg.Killer.szName);
	if (const auto playerExtractedTeam = GetPlayersTeam(playerExtracted))
	{
		deathMsg.Killer.iTeam = playerExtractedTeam->GetTeamNumber();
	}
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bVIPExtract = true;
	deathMsg.bInvolved = deathMsg.Killer.iEntIndex == GetLocalPlayerIndex();

	SetDeathNoticeItemDimensions(&deathMsg);

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);
}

void CNEOHud_DeathNotice::AddVIPDeath(IGameEvent* event)
{
	// the event should be "vip_death"
	const int playerKilled = engine->GetPlayerForUserID(event->GetInt("userid"));
	const int VIPKiller = engine->GetPlayerForUserID(event->GetInt("attacker"));

	// Names of vip and killer
	const char* playerKilledName = g_PR->GetPlayerName(playerKilled);
	if (!playerKilledName)
		playerKilledName = "";
	const char* VIPKillerName = g_PR->GetPlayerName(VIPKiller);
	if (!VIPKillerName)
		VIPKillerName = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Victim.iEntIndex = playerKilled;
	g_pVGuiLocalize->ConvertANSIToUnicode(playerKilledName, deathMsg.Victim.szName, sizeof(deathMsg.Victim.szName));
	deathMsg.Victim.iNameLength = wcslen(deathMsg.Victim.szName);
	if (const auto playerKilledTeam = GetPlayersTeam(playerKilled))
	{
		deathMsg.Victim.iTeam = playerKilledTeam->GetTeamNumber();
	}
	deathMsg.Killer.iEntIndex = VIPKiller;
	g_pVGuiLocalize->ConvertANSIToUnicode(VIPKillerName, deathMsg.Killer.szName, sizeof(deathMsg.Killer.szName));
	deathMsg.Killer.iNameLength = wcslen(deathMsg.Killer.szName);
	if (const auto VIPKillerTeam = GetPlayersTeam(VIPKiller))
	{
		deathMsg.Killer.iTeam = VIPKillerTeam->GetTeamNumber();
	}
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bVIPDeath = true;
	deathMsg.bInvolved = deathMsg.Killer.iEntIndex == GetLocalPlayerIndex();

	SetDeathNoticeItemDimensions(&deathMsg);

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);
}

void CNEOHud_DeathNotice::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}