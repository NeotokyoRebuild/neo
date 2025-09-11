#ifndef NEO_SCOREBOARD_H
#define NEO_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "GameEventListener.h"

#define TYPE_NOTEAM			0	// NOTEAM must be zero :)
#define TYPE_TEAM			1	// a section for a single team
#define TYPE_PLAYERS		2
#define TYPE_SPECTATORS		3	// a section for a spectator group
#define TYPE_BLANK			4


//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CNEOScoreBoard : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CNEOScoreBoard, vgui::EditablePanel );

public:
	CNEOScoreBoard( IViewPort *pViewPort );
	~CNEOScoreBoard();

	virtual const char *GetName( void ) { return PANEL_SCOREBOARD; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	virtual bool ShowAvatars();
	virtual bool UpdateAvatars();

	GameActionSet_t GetPreferredActionSet() override { return GAME_ACTION_SET_IN_GAME_HUD; }

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event);

	virtual void UpdatePlayerAvatar(int playerIndex, KeyValues* kv);

	vgui::ImageList				*m_pImageList;
	CUtlMap<CSteamID,int>		m_mapAvatarsToImageList;

protected:
	MESSAGE_FUNC_INT( OnPollHideCode, "PollHideCode", code );

	// functions to override
	virtual void GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo);
	virtual void InitScoreboardSections();
	virtual void UpdateTeamInfo();
	virtual void UpdatePlayerInfo();
	virtual void OnThink();
	virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team
	virtual int GetAdditionalHeight() { return 12; }

	// sorts players within a section
	static bool StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual void PostApplySchemeSettings( vgui::IScheme *pScheme );

	// finds the player in the scoreboard
	int FindItemIDForPlayerIndex(vgui::SectionedListPanel *pPlayerList, int playerIndex);

	int m_iNumTeams;

	vgui::SectionedListPanel *m_pJinraiPlayerList;
	vgui::SectionedListPanel *m_pNSFPlayerList;
	vgui::SectionedListPanel *m_pSpectatorsPlayerList;

	int m_iLeftTeamXPos = 0;
	int m_iLeftTeamYPos = 0;
	int m_iRightTeamXPos = 0;
	int m_iRightTeamYPos = 0;

	int s_VoiceImage[5];
	int TrackerImage;
	int	m_HLTVSpectators;
	int m_ReplaySpectators;
	int	m_iDesiredHeight;
	float m_fNextUpdateTime;

	void MoveLabelToFront(const char *textEntryName);
	void MoveToCenterOfScreen();

	CPanelAnimationVarAliasType( int, m_iPingWidth, "ping_width", "25", "proportional_int" );
	CPanelAnimationVar( int, m_iAvatarWidth, "avatar_width", "40" );		// Avatar width doesn't scale with resolution
	CPanelAnimationVarAliasType( int, m_iNameWidth, "name_width", "110", "proportional_int" );
	CPanelAnimationVar(int, m_iStatusWidth, "status_width", "40");
	CPanelAnimationVarAliasType( int, m_iClassWidth, "class_width", "40", "proportional_int" );
	CPanelAnimationVarAliasType(int, m_iRankWidth, "rank_width", "60", "proportional_int");
	CPanelAnimationVarAliasType( int, m_iScoreWidth, "score_width", "25", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iDeathWidth, "death_width", "30", "proportional_int" );

private:
	int			m_iPlayerIndexSymbol;
	IViewPort	*m_pViewPort;
	ButtonCode_t m_nCloseKey;
	int 		m_iDeadIcon;


	// methods
	void FillScoreBoard();
	int GetSectionFromTeamNumber( int teamNumber );
	vgui::SectionedListPanel *GetPanelForTeam(int team);
	void RemoveItemForPlayerIndex(int index);
	void UpdateTeamColumnsPosition(int team);
};

extern CNEOScoreBoard* g_pNeoScoreBoard;

#endif // CLIENTSCOREBOARDDIALOG_H
