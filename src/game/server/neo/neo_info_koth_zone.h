#pragma once
#include "cbase.h"
#include "utlvector.h"
#include "Sprite.h"
#include "neo_gamerules.h"

class CNEO_Player;
class CNEO_TriggerKOTHZone;
class CNEO_KOTHBorder;

class CNEO_InfoKOTHZone : public CSprite
{
public:
	DECLARE_CLASS(CNEO_InfoKOTHZone, CSprite);
	DECLARE_DATADESC();

	virtual void Spawn() override;
	virtual void Precache() override;
	virtual void Activate() override;
	virtual void Think() override;
	void ScoreThink();

	// called by every neo_trigger_koth_zone referring to this zone
	void OnPlayerEnter(CNEO_Player *pPlayer);
	void OnPlayerLeave(CNEO_Player *pPlayer);

	// both are called by a neo_trigger_koth_zone once it has resolved us as parent
	void AddChildTrigger(CNEO_TriggerKOTHZone *pTrigger);
	void AddChildBorder(CNEO_KOTHBorder *pBorder);

	// both are called by neo_koth_master: show/hide ONLY the our sprite + child borders
	void SetVisible(bool bVisible);
	bool IsVisible() const { return m_bVisible; }

	// both are called by neo_koth_master: enable/disable ONLY the child triggers
	void SetCapturable(bool bCapturable);
	bool IsCapturable() const { return m_bCapturable; }

	// clears captors/score state (round restart) without touching visibility/capturable state
	void ResetCapture();

	KothControllingTeams GetState() const { return m_State; }

private:
	void UpdateState();
	// safety net: drops captors that died/disconnected without a matching EndTouch
	void PruneStaleCaptors();

	struct ZoneCaptor
	{
		EHANDLE hPlayer;
		int team{};        // team recorded at the moment they entered (survives team switch/death before leaving)
		int touchCount{};  // how many of this zone's triggers currently touch them
	};

	CUtlVector<ZoneCaptor> m_Captors;
	int m_iJinraiCount = 0;
	int m_iNSFCount = 0;
	KothControllingTeams m_State = KOTH_NONE;

	CUtlVector<CHandle<CNEO_TriggerKOTHZone>> m_ChildTriggers;
	CUtlVector<CHandle<CNEO_KOTHBorder>> m_ChildBorders;
	bool m_bVisible = false;
	bool m_bCapturable = false;

	// score accumulation - runs on its own think context (see ScoreThink), independent
	// of PruneStaleCaptors' slower one, since seconds-held needs to be tracked continuously
	float m_flAccumulatorNSF = 0.0f;
	float m_flAccumulatorJinrai = 0.0f;
};

LINK_ENTITY_TO_CLASS(neo_info_koth_zone, CNEO_InfoKOTHZone);
