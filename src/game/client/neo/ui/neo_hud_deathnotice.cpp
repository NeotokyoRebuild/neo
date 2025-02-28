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
#include "spectatorgui.h"
#include "takedamageinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

// Player entries in a death notice
struct DeathNoticePlayer
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	int			iEntIndex;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticePlayer	Killer;
	bool				bRankChange = false;
	int					oldRank = 0;
	int					newRank = 0;
	bool				bRankUp = false;
	bool				bGhostCap = false;
	DeathNoticePlayer	Assist;
	char				iDeathIcon = 0;
	bool				bExplosive = false;
	bool				bSuicide = false;
	bool				bHeadshot = false;
	DeathNoticePlayer   Victim;
	bool				bWasCarryingGhost = false;

	int					iLength = 0;
	float				flHideTime = 0.f;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDeathNotice, vgui::Panel );
public:
	CHudDeathNotice( const char *pElementName );

	void Init( void );
	void VidInit( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	void SetColorForNoticePlayer( int iTeamNumber );
	void RetireExpiredDeathNotices( void );
	
	virtual void FireGameEvent( IGameEvent * event );
	void AddPlayerDeath(IGameEvent* event);
	void AddPlayerRankChange(IGameEvent* event);
	void AddPlayerGhostCapture(IGameEvent* event);

	int GetDeathNoticeItemLength(IGameEvent* event);

	void DrawPlayerDeath(int index);
	void DrawPlayerRankChange(int index);
	void DrawPlayerGhostCapture(int index);

private:

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "10", "proportional_float" );

	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "8" );

	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudNumbersTimer" );

	CUtlVector<DeathNoticeItem> m_DeathNotices;
};

using namespace vgui;

DECLARE_HUDELEMENT( CHudDeathNotice );

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
CHudDeathNotice::CHudDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );
	
    SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit( void )
{
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::SetColorForNoticePlayer( int iTeamNumber )
{
	surface()->DrawSetTextColor( GameResources()->GetTeamColor( iTeamNumber ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	static constexpr wchar_t ASSIST_SEPARATOR[] = L" + ";
	if (g_hFontKillfeed != vgui::INVALID_FONT)
	{
		m_hTextFont = g_hFontKillfeed;
	}
	else
	{
		Assert(false);
	}

	int yStart = GetClientModeHL2MPNormal()->GetDeathMessageStartHeight();
	if (g_pSpectatorGUI->IsVisible())
	{
		yStart += g_pSpectatorGUI->GetTopBarHeight();
	}

	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( GameResources()->GetTeamColor( 0 ) );

	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		int yNeoIconOffset = 0;

		CHudTexture *icon = m_DeathNotices[i].iconDeath;
		if ( !icon )
			continue;

		wchar_t victim[ 256 ];
		wchar_t killer[ 256 ];

		// Get the team numbers for the players involved
		int iKillerTeam = 0;
		int iVictimTeam = 0;

		if( g_PR )
		{
			iKillerTeam = g_PR->GetTeam( m_DeathNotices[i].Killer.iEntIndex );
			iVictimTeam = g_PR->GetTeam( m_DeathNotices[i].Victim.iEntIndex );
		}

		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Killer.szName, killer, sizeof( killer ) );

		wchar_t assists[256];
		int iAssistsTeam = 0;
		const bool hasAssists = m_DeathNotices[i].Assists.iEntIndex > 0;
		if (g_PR)
		{
			iAssistsTeam = g_PR->GetTeam(m_DeathNotices[i].Assists.iEntIndex);
		}
		g_pVGuiLocalize->ConvertANSIToUnicode(m_DeathNotices[i].Assists.szName, assists, sizeof(assists));

		int nLinePadding = vgui::scheme()->GetProportionalScaledValue( 4 );

		// Get the local position for this notice
		int len = UTIL_ComputeStringWidth( m_hTextFont, victim );
		int y = yStart + ( ( m_flLineHeight + nLinePadding ) * i);

		int iconWide;
		int iconTall;

		if( icon->bRenderUsingFont )
		{
			iconWide = surface()->GetCharacterWidth( icon->hFont, icon->cCharacterInFont );
			iconTall = surface()->GetFontTall( icon->hFont );
		}
		else
		{
			surface()->DrawGetTextureSize(icon->textureId, iconWide, iconTall);

			const float scale = ((float)ScreenHeight() / 480.0f);	//scale based on 640x480
			float neoIconScale = (surface()->GetFontTall(m_hTextFont) / (float)iconTall);
			if (neoIconScale == 0)
			{
				neoIconScale = 1;
			}

			// Original neotokyo icons look a bit small, double their size
			if (m_DeathNotices[i].iSuicide || !V_strncmp("neo_boom", icon->szShortName, 8))
			{
				neoIconScale *= 2;
			}

			iconWide = (int)(iconWide * neoIconScale);
			iconTall = (int)(iconTall * neoIconScale);

			// Center the icon vertically in relation to the killfeed text.
			yNeoIconOffset = (int)((surface()->GetFontTall(m_hTextFont) - iconTall) * 0.5);
		}

		// misyl: Looks bad all crunched up in the corner.
		int nPadding = vgui::scheme()->GetProportionalScaledValue( 16 );

		int x;
		if ( m_bRightJustify )
		{
			x =	GetWide() - len - iconWide - nPadding;
			if (m_DeathNotices[i].bHeadshot)
			{
				x -= m_iHeadshotIconWidth;
			}
		}
		else
		{
			x = nPadding;
		}

		// Only draw killers name if it wasn't a suicide or has an assists in it
		if (!m_DeathNotices[i].iSuicide || hasAssists)
		{
			if ( m_bRightJustify )
			{
				x -= UTIL_ComputeStringWidth( m_hTextFont, killer );
				if (hasAssists)
				{
					x -= (UTIL_ComputeStringWidth(m_hTextFont, assists) + UTIL_ComputeStringWidth(m_hTextFont, ASSIST_SEPARATOR));
				}
			}

			SetColorForNoticePlayer( iKillerTeam );

			// Draw killer's name
			surface()->DrawSetTextPos( x, y );
			surface()->DrawSetTextFont(m_hTextFont);
			surface()->DrawUnicodeString( killer );
			surface()->DrawGetTextPos( x, y );

			if (hasAssists)
			{
				// Draw +
				surface()->DrawSetTextColor(COLOR_NEO_WHITE);
				surface()->DrawSetTextPos(x, y);
				surface()->DrawSetTextFont(m_hTextFont);
				surface()->DrawUnicodeString(ASSIST_SEPARATOR);
				surface()->DrawGetTextPos(x, y);

				// Draw assists's name
				SetColorForNoticePlayer(iAssistsTeam);
				surface()->DrawSetTextPos(x, y);
				surface()->DrawSetTextFont(m_hTextFont);
				surface()->DrawUnicodeString(assists);
				surface()->DrawGetTextPos(x, y);
			}
		}

		// Draw death weapon
		//If we're using a font char, this will ignore iconTall and iconWide
		icon->DrawSelf(x, y + yNeoIconOffset, iconWide, iconTall,
			(m_DeathNotices[i].bSuicide ? COLOR_NEO_ORANGE : COLOR_NEO_WHITE));
		x += iconWide;
		if (!m_DeathNotices[i].bSuicide && m_DeathNotices[i].bHeadshot && m_iconD_headshot)
		{
			m_iconD_headshot->DrawSelf(x, y - 0.25 * m_iHeadshotIconHeight, m_iHeadshotIconWidth, m_iHeadshotIconHeight,
				(m_DeathNotices[i].bSuicide ? COLOR_NEO_ORANGE : COLOR_NEO_WHITE));
			x += m_iHeadshotIconWidth;
		}

		SetColorForNoticePlayer( iVictimTeam );

		// Draw victims name
		surface()->DrawSetTextPos( x, y );
		// reset the font, draw icon can change it
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawUnicodeString( victim );
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

int CHudDeathNotice::GetDeathNoticeItemLength(IGameEvent* event)
{
	return 0;
}

void CHudDeathNotice::DrawPlayerDeath(int index)
{

}

void CHudDeathNotice::DrawPlayerRankChange(int index)
{

}

void CHudDeathNotice::DrawPlayerGhostCapture(int index)
{

}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::RetireExpiredDeathNotices( void )
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
void CHudDeathNotice::FireGameEvent(IGameEvent* event)
{
	if (!g_PR)
		return;

	if (hud_deathnotice_time.GetFloat() == 0)
		return;
	
	// Do we have too many death messages in the queue?
	if (m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() >= (int)m_flMaxDeathNotices)
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

void CHudDeathNotice::AddPlayerDeath(IGameEvent* event)
{
	// the event should be "player_death"
	const int killer = engine->GetPlayerForUserID(event->GetInt("attacker"));
	const int victim = engine->GetPlayerForUserID(event->GetInt("userid"));
	const int assistInt = event->GetInt("assists");
	const int assist = engine->GetPlayerForUserID(assistInt);
	const bool hasAssists = assist > 0;
	const char* killedwith = event->GetString("weapon");
	const int weaponIcon = event->GetInt("weaponIcon");

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
	Q_strncpy(deathMsg.Killer.szName, killer_name, MAX_PLAYER_NAME_LENGTH);
	Q_strncpy(deathMsg.Victim.szName, victim_name, MAX_PLAYER_NAME_LENGTH);
	Q_strncpy(deathMsg.Assist.szName, assists_name, MAX_PLAYER_NAME_LENGTH);
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bExplosive = event->GetBool("explosive");
	deathMsg.bSuicide = event->GetBool("suicide");
	deathMsg.bHeadshot = event->GetBool("headshot");
	deathMsg.iDeathIcon = event->GetInt("deathIcon");
	if (deathMsg.bSuicide)
	{
		deathMsg.iDeathIcon = NEO_HUD_DEATHNOTICEICON_SHORTBUS;
	}

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

void CHudDeathNotice::AddPlayerRankChange(IGameEvent* event)
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
	Q_strncpy(deathMsg.Killer.szName, playerRankChangeName, MAX_PLAYER_NAME_LENGTH);
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bRankChange = true;
	deathMsg.oldRank = event->GetInt("oldRank");
	deathMsg.newRank = event->GetInt("newRank");
	deathMsg.bRankUp = newRank > oldRank;
}

void CHudDeathNotice::AddPlayerGhostCapture(IGameEvent* event)
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
	Q_strncpy(deathMsg.Killer.szName, playerCapturedGhostName, MAX_PLAYER_NAME_LENGTH);
	deathMsg.flHideTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.bGhostCap = true;
}