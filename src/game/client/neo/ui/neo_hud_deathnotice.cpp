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

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

// Player entries in a death notice
struct DeathNoticePlayer
{
	wchar_t		szName[MAX_PLAYER_NAME_LENGTH];
	int			iNameLength = 0;
	int			iEntIndex = 0;
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
	DeathNoticePlayer	Assist;
	wchar_t				szDeathIcon[2];
	bool				bExplosive = false;
	bool				bSuicide = false;
	bool				bHeadshot = false;
	DeathNoticePlayer   Victim;
	bool				bWasCarryingGhost = false;

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

	void SetColorForNoticePlayer( int iTeamNumber );
	void RetireExpiredDeathNotices( void );
	
	virtual void FireGameEvent( IGameEvent * event );
	void AddPlayerDeath(IGameEvent* event);
	void AddPlayerRankChange(IGameEvent* event);
	void AddPlayerGhostCapture(IGameEvent* event);

	void SetDeathNoticeItemDimensions(DeathNoticeItem* deathNoticeItem);

	void DrawPlayerDeath(int index);
	void DrawPlayerRankChange(int index);
	void DrawPlayerGhostCapture(int index);

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:

	CPanelAnimationVarAliasType( int, m_iLineMargin, "LineMargin", "2", "proportional_ypos");
	CPanelAnimationVar( int, m_iMaxDeathNotices, "MaxDeathNotices", "8" );
	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );
	CPanelAnimationVar( int, m_iRed, "Red", "200");
	CPanelAnimationVar( int, m_iGreen, "Green", "200");
	CPanelAnimationVar( int, m_iBlue, "Blue", "200");
	CPanelAnimationVar( int, m_iAlpha, "Alpha", "40");

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

enum NeoHudDeathNoticeIcon
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
		if (m_DeathNotices[i].bGhostCap)
		{
			DrawPlayerGhostCapture(i);
		}
		else
		{
			DrawPlayerDeath(i);
		}
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

static constexpr wchar_t ASSIST_SEPARATOR[] = L" + ";
static constexpr wchar_t GHOST_CAPTURE_TEXT[] = L" has captured the ";
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
		/*surface()->GetTextSize(g_hFontKillfeedIcons, deathNoticeItem->szDeathIcon, width, height);
		totalWidth += width;
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_RANKLESS_DOG + deathNoticeItem->szOldRank);
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, deathNoticeItem->bRankUp ? NEO_HUD_DEATHNOTICEICON_RANKUP : NEO_HUD_DEATHNOTICEICON_RANKDOWN);
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_RANKLESS_DOG + deathNoticeItem->szNewRank);*/
	}
	else if (deathNoticeItem->bGhostCap)
	{	// Ghost capture message
		if (deathNoticeItem->Killer.iEntIndex != 0)
		{
			surface()->GetTextSize(g_hFontKillfeed, deathNoticeItem->Killer.szName, width, height);
			totalWidth += width;
		}
		surface()->GetTextSize(g_hFontKillfeed, GHOST_CAPTURE_TEXT, width, height);
		totalWidth += width;
		totalWidth += surface()->GetCharacterWidth(g_hFontKillfeedIcons, NEO_HUD_DEATHNOTICEICON_GHOST);
	}
	else
	{	// Player killed message
		if (deathNoticeItem->Killer.iEntIndex != 0)
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
		if (deathNoticeItem->Victim.iEntIndex != 0 && !deathNoticeItem->bSuicide)
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
	deathNoticeItem->iHeight = height;
}

void CNEOHud_DeathNotice::DrawPlayerDeath(int i)
{
	surface()->DrawSetTextFont(g_hFontKillfeed);

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
	DrawNeoHudRoundedBox(xStart - m_iLineMargin -2,
		yStart - halfMargin - 2,
		xStart + m_DeathNotices[i].iLength + m_iLineMargin + 2,
		yStart + m_DeathNotices[i].iHeight + halfMargin + 2,
		COLOR_BLACK,
		true, true, true, true);
	DrawNeoHudRoundedBox(xStart - m_iLineMargin,
		yStart - halfMargin, 
		xStart + m_DeathNotices[i].iLength + m_iLineMargin,
		yStart + m_DeathNotices[i].iHeight + halfMargin,
		Color(m_iRed, m_iGreen, m_iBlue, m_iAlpha),
		true, true, true, true);

	surface()->DrawSetTextPos(xStart, yStart);
	// Killer
	if (m_DeathNotices[i].Killer.iEntIndex != 0)
	{
		SetColorForNoticePlayer(GetPlayersTeam(m_DeathNotices[i].Killer.iEntIndex)->m_iTeamNum);
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(m_DeathNotices[i].Killer.szName, m_DeathNotices[i].Killer.iNameLength);
	}
	// Assister
	if (m_DeathNotices[i].Assist.iEntIndex != 0)
	{
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawPrintText(ASSIST_SEPARATOR, 3);

		SetColorForNoticePlayer(GetPlayersTeam(m_DeathNotices[i].Assist.iEntIndex)->m_iTeamNum);
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(m_DeathNotices[i].Assist.szName, m_DeathNotices[i].Assist.iNameLength);
	}

	// Icons
	if (m_DeathNotices[i].bSuicide)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(L" ", 1);
		surface()->DrawSetTextFont(g_hFontKillfeedIcons);
		surface()->DrawSetTextColor(COLOR_NEO_ORANGE);
		wchar_t icon = NEO_HUD_DEATHNOTICEICON_SHORTBUS;
		surface()->DrawPrintText(&icon, 1);
	}
	else
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
	if (m_DeathNotices[i].Victim.iEntIndex != 0 && !m_DeathNotices[i].bSuicide)
	{
		surface()->DrawSetTextFont(g_hFontKillfeed);
		surface()->DrawPrintText(L" ", 1);
		SetColorForNoticePlayer(GetPlayersTeam(m_DeathNotices[i].Victim.iEntIndex)->m_iTeamNum);
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
}

void CNEOHud_DeathNotice::DrawPlayerRankChange(int index)
{

}

void CNEOHud_DeathNotice::DrawPlayerGhostCapture(int index)
{

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
	else if (!Q_stricmp(eventName, "player_rankup"))
	{
		AddPlayerRankChange(event);
	}
	else //if (!Q_stricmp(eventName, "ghost_capture"))
	{
		AddPlayerGhostCapture(event);
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
	deathMsg.Killer.iNameLength = strlen(killer_name);
	deathMsg.Victim.iNameLength = strlen(victim_name);
	deathMsg.Assist.iNameLength = strlen(assists_name);
	g_pVGuiLocalize->ConvertANSIToUnicode(killer_name, deathMsg.Killer.szName, sizeof(deathMsg.Killer.szName));
	g_pVGuiLocalize->ConvertANSIToUnicode(victim_name, deathMsg.Victim.szName, sizeof(deathMsg.Victim.szName));
	g_pVGuiLocalize->ConvertANSIToUnicode(assists_name, deathMsg.Assist.szName, sizeof(deathMsg.Assist.szName));
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bExplosive = event->GetBool("explosive");
	deathMsg.bSuicide = event->GetBool("suicide");
	deathMsg.bHeadshot = event->GetBool("headshot");
	const char* deathIcon = event->GetString("deathIcon");
	g_pVGuiLocalize->ConvertANSIToUnicode(deathIcon, deathMsg.szDeathIcon, sizeof(deathMsg.szDeathIcon));

	SetDeathNoticeItemDimensions(&deathMsg);

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);

	// Record the death notice in the console
	char sDeathMsg[512];
	if (deathMsg.bSuicide)
	{
		if (!strcmp(fullkilledwith, "d_worldspawn"))
		{
			Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%s died.", deathMsg.Victim.szName);
		}
		else	//d_world
		{
			Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%s suicided.", deathMsg.Victim.szName);
		}
	}
	else
	{
		// These phrases are the same as in original NT to provide plugin/stats tracking website/etc compatibility.
#define HEADSHOT_LINGO "headshot'd"
#define NON_HEADSHOT_LINGO "killed"

		Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%s %s %s",
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
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bRankChange = true;
	const char* oldRankIcon = event->GetString("oldRank");
	g_pVGuiLocalize->ConvertANSIToUnicode(oldRankIcon, deathMsg.szOldRank, sizeof(deathMsg.szOldRank));
	const char* newRankIcon = event->GetString("newRank");
	g_pVGuiLocalize->ConvertANSIToUnicode(newRankIcon, deathMsg.szNewRank, sizeof(deathMsg.szNewRank));
	deathMsg.bRankUp = newRank > oldRank;
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
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bGhostCap = true;
}

void CNEOHud_DeathNotice::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}