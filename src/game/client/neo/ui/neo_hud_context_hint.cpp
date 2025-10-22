#include "cbase.h"
#include "neo_hud_context_hint.h"

#include "iclientmode.h"
#include "c_neo_player.h"
#include "vgui/ISurface.h"
#include "igameresources.h"
#include "ienginevgui.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui_controls/AnimationController.h"
#include "neo_root_settings.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_hud_context_hint_enabled("cl_neo_hud_context_hint_enabled", "1",
	FCVAR_ARCHIVE, "Whether to show contextual hud hints. 0: Disabled, 1: Enabled.", false, 0, true, 1);

ConVar cl_neo_spec_takeover_player_hint_time_sec("cl_neo_spec_takeover_player_hint_time_sec", "3.0",
	FCVAR_ARCHIVE, "Duration in seconds to display the spectator bot takeover hint.", true, 0, true, 9999);

DECLARE_HUDELEMENT(CNEOHud_ContextHint);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(ContextHint, 0.00695);

CNEOHud_ContextHint::CNEOHud_ContextHint(const char* pElementName)
	: CNEOHud_ChildElement(), CHudElement(pElementName), vgui::EditablePanel(NULL, "neo_context_hint")
{
	SetParent(g_pClientMode->GetViewport());
	SetVisible(false);

	m_flDisplayTime = 0.0f;
	m_bHintShownForCurrentSpecTarget = false;
	m_hLastSpecTarget = nullptr;
}

CNEOHud_ContextHint::~CNEOHud_ContextHint()
{
}

void CNEOHud_ContextHint::Init()
{
}

void CNEOHud_ContextHint::VidInit()
{
}

void CNEOHud_ContextHint::Reset()
{
	m_flDisplayTime = 0.0f;
	m_bHintShownForCurrentSpecTarget = false;
	m_hLastSpecTarget = nullptr;
	SetVisible(false);
}

void CNEOHud_ContextHint::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);
	SetBounds(0, 0, iScrWide, iScrTall);

	LoadControlSettings("scripts/HudLayout.res");
}

bool CNEOHud_ContextHint::ShouldDraw()
{
	if (!cl_neo_hud_context_hint_enabled.GetBool())
	{
		return false;
	}

	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pLocalPlayer)
	{
		return false;
	}

	C_BaseEntity* pObserverTargetEntity = pLocalPlayer->GetObserverTarget();
	C_BasePlayer* pObserverTargetPlayer = (pObserverTargetEntity && pObserverTargetEntity->IsPlayer()) ? ToBasePlayer(pObserverTargetEntity) : nullptr;

	auto eObserverMode = pLocalPlayer->GetObserverMode();
	bool bIsSpectating = (eObserverMode == OBS_MODE_CHASE || eObserverMode == OBS_MODE_IN_EYE);

	if (pObserverTargetPlayer != m_hLastSpecTarget.Get())
	{
		m_bHintShownForCurrentSpecTarget = false;
		m_hLastSpecTarget = pObserverTargetPlayer;
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
		if (!m_bHintShownForCurrentSpecTarget)
		{
			m_flDisplayTime = gpGlobals->curtime + cl_neo_spec_takeover_player_hint_time_sec.GetFloat();
			m_bHintShownForCurrentSpecTarget = true;
		}

		// If the hint is displaying and the timer hasn't expired, keep displaying it.
		if (gpGlobals->curtime < m_flDisplayTime)
		{
			return true;
		}
	}

	// If conditions are not met, or timer has expired, hide the hint.
	return false;
}

void CNEOHud_ContextHint::UpdateStateForNeoHudElementDraw()
{
	const char* useKeyBinding = engine->Key_LookupBinding("+use");
	if (useKeyBinding && useKeyBinding[0] != '\0')
	{
		char szUppercaseKeyBinding[16]; // Assuming keybinds won't exceed 15 characters + null terminator
		V_strncpy(szUppercaseKeyBinding, useKeyBinding, sizeof(szUppercaseKeyBinding));
		V_strupr(szUppercaseKeyBinding);
		V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] Control Bot", szUppercaseKeyBinding);
	}
	else
	{
		V_wcsncpy(m_wszHintText, L"Press Use To Control Bot", sizeof(m_wszHintText));
	}
}

void CNEOHud_ContextHint::DrawNeoHudElement()
{
	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);

	int iTextWide, iTextTall;
	vgui::surface()->GetTextSize(m_hHintFont, m_wszHintText, iTextWide, iTextTall);

	int iBoxWide = iTextWide + m_iPaddingX * 2;
	int iBoxTall = iTextTall + m_iPaddingY * 2;

	int iBoxX = (iScrWide - iBoxWide) / 2;
	int iBoxY = iScrTall * m_flBoxYFactor - iBoxTall / 2;

	vgui::surface()->DrawSetColor(m_BoxColor);
	vgui::surface()->DrawFilledRect(iBoxX, iBoxY, iBoxX + iBoxWide, iBoxY + iBoxTall);

	vgui::surface()->DrawSetTextFont(m_hHintFont);
	vgui::surface()->DrawSetTextColor(m_TextColor);
	vgui::surface()->DrawSetTextPos(iBoxX + m_iPaddingX, iBoxY + m_iPaddingY);
	vgui::surface()->DrawPrintText(m_wszHintText, static_cast<int>(wcslen(m_wszHintText)));
}

void CNEOHud_ContextHint::Paint()
{
	PaintNeoElement();
	BaseClass::Paint();
}

void CNEOHud_ContextHint::FireGameEvent(IGameEvent* event)
{
}