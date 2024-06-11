//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>
#include <igameresources.h>
#include "IGameUIFuncs.h" // for key bindings
#include "inputsystem/iinputsystem.h"
#include "neo_scoreboard.h"
#include "c_team.h"
#include "c_playerresource.h"
#include <voice_status.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vstdlib/IKeyValuesSystem.h>

#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>

#include <game/client/iviewport.h>
#include <igameresources.h>

#include "vgui_avatarimage.h"

#include "neo_player_shared.h"
#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define TEAM_MAXCOUNT 5

#define SHOW_ENEMY_STATUS true

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CNEOScoreBoard::CNEOScoreBoard(IViewPort *pViewPort) : EditablePanel( NULL, PANEL_SCOREBOARD )
{
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");
	m_nCloseKey = BUTTON_CODE_INVALID;

	//memset(s_VoiceImage, 0x0, sizeof( s_VoiceImage ));
	TrackerImage = 0;
	m_pViewPort = pViewPort;

	// initialize dialog
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	// set the scheme before any child control is created
	SetScheme("ClientScheme");

	m_pJinraiPlayerList = new SectionedListPanel(this, "JinraiPlayerList");
	m_pJinraiPlayerList->SetVerticalScrollbar(false);
	m_pJinraiPlayerList->SetProportional(true);
	m_pNSFPlayerList = new SectionedListPanel(this, "NSFPlayerList");
	m_pNSFPlayerList->SetVerticalScrollbar(false);
	m_pNSFPlayerList->SetProportional(true);
	m_pSpectatorsPlayerList = new SectionedListPanel(this, "SpectatorsPlayerList");
	m_pSpectatorsPlayerList->SetVerticalScrollbar(false);
	m_pSpectatorsPlayerList->SetProportional(true);

	LoadControlSettings("Resource/UI/ScoreBoard.res");
	m_iDesiredHeight = GetTall();
	m_pJinraiPlayerList->SetVisible( false ); // hide this until we load the images in applyschemesettings
	m_pNSFPlayerList->SetVisible( false );
	m_pSpectatorsPlayerList->SetVisible( false );

	m_HLTVSpectators = 0;
	m_ReplaySpectators = 0;

	// update scoreboard instantly if on of these events occure
	ListenForGameEvent( "hltv_status" );
	ListenForGameEvent( "server_spawn" );

	m_pImageList = NULL;

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapAvatarsToImageList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CNEOScoreBoard::~CNEOScoreBoard()
{
	if ( NULL != m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

//-----------------------------------------------------------------------------
// Call every frame
//-----------------------------------------------------------------------------
void CNEOScoreBoard::OnThink()
{
	BaseClass::OnThink();

	// NOTE: this is necessary because of the way input works.
	// If a key down message is sent to vgui, then it will get the key up message
	// Sometimes the scoreboard is activated by other vgui menus,
	// sometimes by console commands. In the case where it's activated by
	// other vgui menus, we lose the key up message because this panel
	// doesn't accept keyboard input. It *can't* accept keyboard input
	// because another feature of the dialog is that if it's triggered
	// from within the game, you should be able to still run around while
	// the scoreboard is up. That feature is impossible if this panel accepts input.
	// because if a vgui panel is up that accepts input, it prevents the engine from
	// receiving that input. So, I'm stuck with a polling solution.
	//
	// Close key is set to non-invalid when something other than a keybind
	// brings the scoreboard up, and it's set to invalid as soon as the
	// dialog becomes hidden.
	if ( m_nCloseKey != BUTTON_CODE_INVALID )
	{
		if ( !g_pInputSystem->IsButtonDown( m_nCloseKey ) )
		{
			m_nCloseKey = BUTTON_CODE_INVALID;
			gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
			GetClientVoiceMgr()->StopSquelchMode();
		}
	}
}

//-----------------------------------------------------------------------------
// Called by vgui panels that activate the client scoreboard
//-----------------------------------------------------------------------------
void CNEOScoreBoard::OnPollHideCode( int code )
{
	m_nCloseKey = (ButtonCode_t)code;
}

//-----------------------------------------------------------------------------
// Purpose: clears everything in the scoreboard and all it's state
//-----------------------------------------------------------------------------
void CNEOScoreBoard::Reset()
{
	// clear
	m_pJinraiPlayerList->DeleteAllItems();
	m_pJinraiPlayerList->RemoveAllSections();
	m_pNSFPlayerList->DeleteAllItems();
	m_pNSFPlayerList->RemoveAllSections();
	m_pSpectatorsPlayerList->DeleteAllItems();
	m_pSpectatorsPlayerList->RemoveAllSections();

	m_fNextUpdateTime = 0;
	// add all the sections
	InitScoreboardSections();
}

//-----------------------------------------------------------------------------
// Purpose: adds all the team sections to the scoreboard
//-----------------------------------------------------------------------------
void CNEOScoreBoard::InitScoreboardSections()
{
	// add the team sections
	AddSection( TYPE_TEAM, TEAM_JINRAI );
	AddSection( TYPE_TEAM, TEAM_NSF );

	AddSection( TYPE_SPECTATORS, TEAM_UNASSIGNED );
	AddSection( TYPE_SPECTATORS, TEAM_SPECTATOR );
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CNEOScoreBoard::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pImageList )
		delete m_pImageList;
	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();

	auto deadIcon = vgui::scheme()->GetImage("hud/kill_kill", true);
	deadIcon->SetSize(32, 32);
	m_iDeadIcon = m_pImageList->AddImage(deadIcon);

	PostApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: Does dialog-specific customization after applying scheme settings.
//-----------------------------------------------------------------------------
void CNEOScoreBoard::PostApplySchemeSettings( vgui::IScheme *pScheme )
{
	// resize the images to our resolution
	// for (int i = 0; i < m_pImageList->GetImageCount(); i++ )
	// {
	// 	int wide, tall;
	// 	m_pImageList->GetImage(i)->GetSize(wide, tall);
	// 	m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(),wide), scheme()->GetProportionalScaledValueEx( GetScheme(),tall));
	// }

	m_pJinraiPlayerList->SetImageList( m_pImageList, false );
	m_pJinraiPlayerList->SetVisible( true );
	m_pNSFPlayerList->SetImageList( m_pImageList, false );
	m_pNSFPlayerList->SetVisible( true );
	m_pSpectatorsPlayerList->SetImageList( m_pImageList, false );
	m_pSpectatorsPlayerList->SetVisible( true );

	// light up scoreboard a bit
	SetBgColor( Color( 0,0,0,0) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNEOScoreBoard::ShowPanel(bool bShow)
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if ( m_pImageList == NULL )
	{
		InvalidateLayout( true, true );
	}

	if ( !bShow )
	{
		m_nCloseKey = BUTTON_CODE_INVALID;
	}

	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Reset();
		SetKeyBoardInputEnabled(true);
		Update();
		SetVisible( true );
		MoveToFront();
	}
	else
	{
		BaseClass::SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}
}

void CNEOScoreBoard::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "hltv_status") == 0 )
	{
		// spectators = clients - proxies
		m_HLTVSpectators = event->GetInt( "clients" );
		m_HLTVSpectators -= event->GetInt( "proxies" );
	}
	else if ( Q_strcmp(type, "server_spawn") == 0 )
	{
		// We'll post the message ourselves instead of using SetControlString()
		// so we don't try to translate the hostname.
		const char *hostname = event->GetString( "hostname" );
		Panel *control = FindChildByName( "ServerName" );
		if ( control )
		{
			PostMessage( control, new KeyValues( "SetText", "text", hostname ) );
			control->MoveToFront();
		}
	}

	if( IsVisible() )
		Update();

}

bool CNEOScoreBoard::NeedsUpdate( void )
{
	return (m_fNextUpdateTime < gpGlobals->curtime);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void CNEOScoreBoard::Update( void )
{
	// Set the title

	// Reset();
	m_pJinraiPlayerList->DeleteAllItems();
	m_pNSFPlayerList->DeleteAllItems();
	m_pSpectatorsPlayerList->DeleteAllItems();

	FillScoreBoard();

	// grow the scoreboard to fit all the players
	int jinraiWide, jinraiTall;
	m_pJinraiPlayerList->GetContentSize(jinraiWide, jinraiTall);
	int nsfWide, nsfTall;
	m_pNSFPlayerList->GetContentSize(nsfWide, nsfTall);
	int spectatorsWide, spectatorsTall;
	m_pSpectatorsPlayerList->GetContentSize(spectatorsWide, spectatorsTall);

	int tall, wide;
	int tallest = MAX(jinraiTall, nsfTall);
	tall = tallest + spectatorsTall;
	int addHeight = scheme()->GetProportionalScaledValueEx( GetScheme(), GetAdditionalHeight());
	tall += addHeight;
	wide = GetWide();
	m_pJinraiPlayerList->SetSize(wide / 2, tallest);
	m_pNSFPlayerList->SetSize(wide / 2, tallest);
	if (m_iDesiredHeight < tall)
	{
		SetSize(wide, tall);
		m_pSpectatorsPlayerList->SetSize(wide, spectatorsTall);
	}
	else
	{
		SetSize(wide, m_iDesiredHeight);
		m_pSpectatorsPlayerList->SetSize(wide, m_iDesiredHeight - tallest);
	}

	m_pSpectatorsPlayerList->SetPos(0, tallest + addHeight);

	MoveToCenterOfScreen();

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void CNEOScoreBoard::UpdateTeamInfo()
{
	if ( !g_PR )
		return;

	int iNumPlayersInGame = 0;

	for ( int j = 1; j <= gpGlobals->maxClients; j++ )
	{
		if ( g_PR->IsConnected( j ) )
		{
			iNumPlayersInGame++;
		}
	}

	// update the team sections in the scoreboard
	for ( int i = TEAM_SPECTATOR; i < TEAM_MAXCOUNT; i++ )
	{
		wchar_t *teamName = NULL;
		int sectionID = 0;
		C_Team *team = GetGlobalTeam(i);

		if ( team )
		{
			auto pPlayerList = GetPanelForTeam( i );
			sectionID = GetSectionFromTeamNumber( i );

			// update team name
			wchar_t name[64];
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];

			if ( HL2MPRules()->IsTeamplay() == false )
			{
				V_snwprintf( wNumPlayers, ARRAYSIZE(wNumPlayers), L"%i", iNumPlayersInGame );
				V_snwprintf( name, ARRAYSIZE(name), L"%ls", g_pVGuiLocalize->Find("#ScoreBoard_Deathmatch") );

				teamName = name;

				if ( iNumPlayersInGame == 1)
				{
					g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_Player"), 2, teamName, wNumPlayers );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_Players"), 2, teamName, wNumPlayers );
				}
			}
			else
			{
				V_snwprintf(wNumPlayers, ARRAYSIZE(wNumPlayers), L"%i", team->Get_Number_Players());

				if (!teamName && team)
				{
					g_pVGuiLocalize->ConvertANSIToUnicode(team->Get_Name(), name, sizeof(name));
					teamName = name;
				}

				// update stats
				wchar_t val[12];

				if (i != TEAM_SPECTATOR)
				{
					V_snwprintf(val, ARRAYSIZE(val), L"%ls: %d", teamName, team->Get_Score());
					pPlayerList->ModifyColumn(sectionID, "ping", val);
					V_snwprintf(string1, ARRAYSIZE(string1), L"Players: %ls", wNumPlayers);
					pPlayerList->ModifyColumn(sectionID, "name", string1);
				}

				// if (team->Get_Ping() < 1)
				// {
				// 	pPlayerList->ModifyColumn(sectionID, "ping", L"Ping");
				// }
				// else
				// {
				// 	V_snwprintf(val, ARRAYSIZE(val), L"%d", team->Get_Ping());
				// 	pPlayerList->ModifyColumn(sectionID, "ping", val);
				// }
			}
		}
	}
}

int CNEOScoreBoard::GetSectionFromTeamNumber(int team)
{
	int sectionID = team;
	// Jinrai/NSF panels have only one section
	if (sectionID >= TEAM_JINRAI) {
		sectionID = 0;
	}
	return sectionID;
}

vgui::SectionedListPanel *CNEOScoreBoard::GetPanelForTeam(int team)
{
	if (team == TEAM_JINRAI)
	{
		return m_pJinraiPlayerList;
	}
	else if (team == TEAM_NSF)
	{
		return m_pNSFPlayerList;
	}
	return m_pSpectatorsPlayerList;
}

void CNEOScoreBoard::RemoveItemForPlayerIndex(int index)
{
	for (int i = TEAM_SPECTATOR; i < TEAM_MAXCOUNT; i++)
	{
		auto pPlayerList = GetPanelForTeam(i);
		int itemID = FindItemIDForPlayerIndex(pPlayerList, index);
		if (itemID != -1)
		{
			pPlayerList->RemoveItem(itemID);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNEOScoreBoard::UpdatePlayerInfo()
{
	IGameResources *gr = GameResources();
	if ( !gr )
		return;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		if ( gr->IsConnected( i ) )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );
			UpdatePlayerAvatar( i, playerData );

			const char *oldName = playerData->GetString("name","");
			char newName[MAX_PLAYER_NAME_LENGTH];

			UTIL_MakeSafeName( oldName, newName, MAX_PLAYER_NAME_LENGTH );

			playerData->SetString("name", newName);

  			int team = gr->GetTeam( i );
			int sectionID = GetSectionFromTeamNumber( team );

			auto pPlayerList = GetPanelForTeam(team);
			int itemID = FindItemIDForPlayerIndex( pPlayerList, i );

			if (itemID == -1)
			{
				// Player might have switched team
				RemoveItemForPlayerIndex(i);
				// add a new row
				itemID = pPlayerList->AddItem( sectionID, playerData );
				// set the row color based on the players team
				pPlayerList->SetItemFgColor( itemID, gr->GetTeamColor( team ) );
			}
			else
			{
				// modify the current row
				pPlayerList->ModifyItem( itemID, sectionID, playerData );
			}

			if ( gr->IsLocalPlayer( i ) )
			{
				Color color = gr->GetTeamColor(team);
				color.SetColor(color.r(), color.g(), color.b(), 16);
				pPlayerList->SetItemBgColor(itemID, color);
			}
			else
			{
				if (!SHOW_ENEMY_STATUS)
				{
					CBasePlayer* player = C_BasePlayer::GetLocalPlayer();
					const int playerTeam = player->GetTeamNumber();
					const bool oppositeTeam = playerTeam >= TEAM_JINRAI && team != playerTeam;
					pPlayerList->SetItemBgColor( itemID, !oppositeTeam && gr->IsAlive(i) ? Color(255,255,255,4) : Color(0,0,0,0));
				}
				else
				{
					pPlayerList->SetItemBgColor( itemID, gr->IsAlive(i) ? Color(255,255,255,4) : Color(0,0,0,0));
				}
			}

			playerData->deleteThis();
		}
		else
		{
			// remove the player
			RemoveItemForPlayerIndex(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CNEOScoreBoard::AddSection(int teamType, int teamNumber)
{
	IGameResources *gr = GameResources();
	if ( !gr )
		return;

	auto pPlayerList = GetPanelForTeam(teamNumber);
	int sectionID = GetSectionFromTeamNumber(teamNumber);
	if ( teamType == TYPE_TEAM )
	{
		pPlayerList->AddSection(sectionID, "", StaticPlayerSortFunc);
		pPlayerList->SetSectionAlwaysVisible(sectionID);

		pPlayerList->AddColumnToSection(sectionID, "ping", "", 0, m_iPingWidth );
		if ( ShowAvatars() )
		{
			pPlayerList->AddColumnToSection( sectionID, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );
		}
		pPlayerList->AddColumnToSection(sectionID, "name", "", 0, m_iNameWidth - m_iAvatarWidth );
		pPlayerList->AddColumnToSection(sectionID, "status", "", SectionedListPanel::COLUMN_IMAGE, m_iStatusWidth);
		pPlayerList->AddColumnToSection(sectionID, "class", "Class", 0, m_iClassWidth);
		pPlayerList->AddColumnToSection(sectionID, "rank", "Rank", 0, m_iRankWidth);
		pPlayerList->AddColumnToSection(sectionID, "xp", "XP", 0, m_iScoreWidth);
		pPlayerList->AddColumnToSection(sectionID, "deaths", "#PlayerDeath", 0, m_iDeathWidth );
	}
	else if ( teamType == TYPE_SPECTATORS )
	{
		pPlayerList->AddSection(sectionID, "");

		// Avatars are always displayed at 32x32 regardless of resolution
		if ( ShowAvatars() )
		{
			pPlayerList->AddColumnToSection( sectionID, "avatar", teamNumber == TEAM_UNASSIGNED ? "Unassigned" : "Spectators", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );
		}
		pPlayerList->AddColumnToSection(sectionID, "name", "", 0, m_iNameWidth - m_iAvatarWidth );
	}

	// set the section to have the team color
	pPlayerList->SetSectionFgColor(sectionID, (teamNumber == TEAM_SPECTATOR) ? COLOR_NEO_WHITE : GameResources()->GetTeamColor(teamNumber));
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CNEOScoreBoard::StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	// first compare xp
	int v1 = it1->GetInt("xp");
	int v2 = it2->GetInt("xp");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// next compare deaths
	v1 = it1->GetInt("deaths");
	v2 = it2->GetInt("deaths");
	if (v1 > v2)
		return false;
	else if (v1 < v2)
		return true;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return itemID1 < itemID2;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
void CNEOScoreBoard::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	if ( !g_PR )
		return;

	kv->SetInt("playerIndex", playerIndex);
	const int neoTeam = g_PR->GetTeam(playerIndex);
	kv->SetInt("team", neoTeam);
	kv->SetString("name", g_PR->GetPlayerName(playerIndex) );
	kv->SetInt("deaths", g_PR->GetDeaths( playerIndex ));

	const int xp = g_PR->GetXP(playerIndex);
	const int neoClassIdx = g_PR->GetClass(playerIndex);
	kv->SetString("rank", GetRankName(xp));
	kv->SetInt("xp", xp);

	CBasePlayer* player = C_BasePlayer::GetLocalPlayer();
	const int playerNeoTeam = player->GetTeamNumber();
	const bool oppositeTeam = (playerNeoTeam == TEAM_JINRAI || playerNeoTeam == TEAM_NSF) && (neoTeam != playerNeoTeam);

	int statusIcon = -1;
	if (neoTeam == TEAM_JINRAI || neoTeam == TEAM_NSF)
	{
		statusIcon = (!SHOW_ENEMY_STATUS && oppositeTeam || g_PR->IsAlive(playerIndex)) ? -1 : m_iDeadIcon;
	}
	kv->SetInt("status", statusIcon);
	kv->SetString("class", oppositeTeam ? "" : GetNeoClassName(neoClassIdx));

	if ( g_PR->IsFakePlayer( playerIndex ) )
	{
		kv->SetString("ping", "BOT");
	}
	else
	{
		kv->SetInt("ping", g_PR->GetPing( playerIndex ));
	}

	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNEOScoreBoard::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	// Update their avatar
	if ( kv && ShowAvatars() && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		player_info_t pi;
		if ( engine->GetPlayerInfo( playerIndex, &pi ) )
		{
			if ( pi.friendsID )
			{
				CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

				// See if we already have that avatar in our list
				int iMapIndex = m_mapAvatarsToImageList.Find( steamIDForPlayer );
				int iImageIndex;
				if ( iMapIndex == m_mapAvatarsToImageList.InvalidIndex() )
				{
					CAvatarImage *pImage = new CAvatarImage();
					pImage->SetAvatarSteamID( steamIDForPlayer );
					pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
					iImageIndex = m_pImageList->AddImage( pImage );

					m_mapAvatarsToImageList.Insert( steamIDForPlayer, iImageIndex );
				}
				else
				{
					iImageIndex = m_mapAvatarsToImageList[ iMapIndex ];
				}

				kv->SetInt( "avatar", iImageIndex );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: reload the player list on the scoreboard
//-----------------------------------------------------------------------------
void CNEOScoreBoard::FillScoreBoard()
{
	// update totals information
	UpdateTeamInfo();

	// update player info
	UpdatePlayerInfo();
}

//-----------------------------------------------------------------------------
// Purpose: searches for the player in the scoreboard
//-----------------------------------------------------------------------------
int CNEOScoreBoard::FindItemIDForPlayerIndex(vgui::SectionedListPanel *pPlayerList, int playerIndex)
{
	for (int i = 0; i <= pPlayerList->GetHighestItemID(); i++)
	{
		if (pPlayerList->IsItemIDValid(i))
		{
			KeyValues *kv = pPlayerList->GetItemData(i);
			kv = kv->FindKey(m_iPlayerIndexSymbol);
			if (kv && kv->GetInt() == playerIndex)
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CNEOScoreBoard::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Center the dialog on the screen.  (vgui has this method on
//			Frame, but we're an EditablePanel, need to roll our own.)
//-----------------------------------------------------------------------------
void CNEOScoreBoard::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
}
