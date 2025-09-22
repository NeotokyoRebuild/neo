#include "cbase.h"
#include "neo_spectator_takeover.h"

#include "iclientmode.h"
#include "c_neo_player.h"
#include "vgui/ISurface.h"
#include "neo_ui.h"
#include "igameresources.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_spec_replace_player_hint_time_sec("cl_neo_spec_replace_player_hint_time_sec", "3.0",
	FCVAR_ARCHIVE, "Duration in seconds to display the spectator bot takeover hint.");

DECLARE_HUDELEMENT(CNEOHudSpectatorTakeover);

CNEOHudSpectatorTakeover::CNEOHudSpectatorTakeover(const char* pElementName)
	: CHudElement(pElementName), vgui::Panel(NULL, "CNEOHudSpectatorTakeover")
{
	SetParent(g_pClientMode->GetViewport());
	SetVisible(false);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	m_flDisplayTime = 0.0f;
	m_bIsDisplayingHint = false;
	m_bHintShownForCurrentTarget = false;
	m_hLastSpectatedTarget = nullptr;
}

CNEOHudSpectatorTakeover::~CNEOHudSpectatorTakeover()
{
	NeoUI::FreeContext(&m_uiCtx);
}

void CNEOHudSpectatorTakeover::Init()
{
}

void CNEOHudSpectatorTakeover::VidInit()
{
}

void CNEOHudSpectatorTakeover::Reset()
{
	m_flDisplayTime = 0.0f;
	m_bIsDisplayingHint = false;
	m_bHintShownForCurrentTarget = false;
	m_hLastSpectatedTarget = nullptr;
	SetVisible(false);
}

void CNEOHudSpectatorTakeover::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);
	SetBounds(0, 0, iScrWide, iScrTall);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char* FONT_NAMES[NeoUI::FONT__TOTAL] = {
		"NeoUISmall", "NHudOCR", "NHudOCRSmallerNoAdditive", "ClientTitleFont", "ClientTitleFontSmall",
		"NeoUINormal"
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		m_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	// Calculate initial UI context dimensions for positioning
	// NEO JANK potential duplication: Using UI_SCALE from killer_damage_info as a reference
	static const constexpr float UI_SCALE = 0.66f;
	m_uiCtx.layout.iDefRowTall = (iScrTall / 27) * UI_SCALE;
	m_uiCtx.iMarginX = (iScrWide / 192) * UI_SCALE;
	m_uiCtx.iMarginY = (iScrTall / 108) * UI_SCALE;

	// Hint text buffer formatting
	V_wcsncpy(m_wszHintText, L"Press Use To Control Bot", sizeof(m_wszHintText));
}

bool CNEOHudSpectatorTakeover::ShouldDraw()
{
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pLocalPlayer)
	{
		return false;
	}

	C_BaseEntity* pObserverTargetEntity = pLocalPlayer->GetObserverTarget();
	C_BasePlayer* pObserverTargetPlayer = (pObserverTargetEntity && pObserverTargetEntity->IsPlayer()) ? ToBasePlayer(pObserverTargetEntity) : nullptr;

	auto eObserverMode = pLocalPlayer->GetObserverMode();
	bool bIsSpectating = (eObserverMode == OBS_MODE_CHASE || eObserverMode == OBS_MODE_IN_EYE);

	if (pObserverTargetPlayer != m_hLastSpectatedTarget.Get())
	{
		m_bHintShownForCurrentTarget = false;
		m_hLastSpectatedTarget = pObserverTargetPlayer;
	}

	bool bShouldDisplayBotTakeoverHint = false;
	if (bIsSpectating && pObserverTargetPlayer)
	{
		if (GameResources()->IsFakePlayer(pObserverTargetPlayer->entindex()))
		{
			if (pLocalPlayer->InSameTeam(pObserverTargetPlayer))
			{
				ConVar* pBotEnableCvar = g_pCVar->FindVar("sv_neo_spec_replace_player_bot_enable");
				bool bBotEnable = pBotEnableCvar ? pBotEnableCvar->GetBool() : false;

				if (bBotEnable)
				{
					// Check that spectator's XP is not at concerning griefing levels
					int localPlayerXP = GameResources()->GetXP(pLocalPlayer->entindex());
					ConVar* pMinExpCvar = g_pCVar->FindVar("sv_neo_spec_replace_player_min_exp");
					int minExp = pMinExpCvar ? pMinExpCvar->GetInt() : 0;

					bShouldDisplayBotTakeoverHint = (localPlayerXP >= minExp);
				}
			}
		}
	}

	if (bShouldDisplayBotTakeoverHint)
	{
		// If the hint has not been shown for the current target yet, start the timer.
		if (!m_bHintShownForCurrentTarget)
		{
			m_bIsDisplayingHint = true;
			m_flDisplayTime = gpGlobals->curtime + cl_neo_spec_replace_player_hint_time_sec.GetFloat();
			m_bHintShownForCurrentTarget = true;
		}

		// If the hint is displaying and the timer hasn't expired, keep displaying it.
		if (m_bIsDisplayingHint && gpGlobals->curtime < m_flDisplayTime)
		{
			return true;
		}
	}

	// If conditions are not met, or timer has expired, hide the hint.
	m_bIsDisplayingHint = false;
	return false;
}

void CNEOHudSpectatorTakeover::OnThink()
{
	SetVisible(ShouldDraw());
}

void CNEOHudSpectatorTakeover::Paint()
{
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);

	int iTextWide, iTextTall;
	vgui::surface()->GetTextSize(m_uiCtx.fonts[NeoUI::FONT_NTHORIZSIDES].hdl, m_wszHintText, iTextWide, iTextTall);

	int iPaddingX = m_uiCtx.iMarginX * 2;
	int iPaddingY = m_uiCtx.iMarginY * 2;

	int iBoxWide = iTextWide + iPaddingX;
	int iBoxTall = iTextTall + iPaddingY;

	int iBoxX = (iScrWide - iBoxWide) / 2;
	int iBoxY = (iScrTall * 3) / 4;

	m_uiCtx.dPanel.x = iBoxX;
	m_uiCtx.dPanel.y = iBoxY;
	m_uiCtx.dPanel.wide = iBoxWide;
	m_uiCtx.dPanel.tall = iBoxTall;

	NeoUI::BeginContext(&m_uiCtx, NeoUI::MODE_PAINT, nullptr, "CNEOHudSpectatorTakeover");
	NeoUI::BeginSection();
	{
		NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
		m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

		NeoUI::Label(m_wszHintText);
	}
	NeoUI::EndSection();
	NeoUI::EndContext();
}

void CNEOHudSpectatorTakeover::FireGameEvent(IGameEvent* event)
{
}

