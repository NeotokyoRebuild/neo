#include "cbase.h"
#include "neo_hud_context_hint.h"

#include "iclientmode.h"
#include "c_neo_player.h"
#include "neo_player_shared.h"
#include "vgui/ISurface.h"
#include "igameresources.h"
#include "ienginevgui.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui_controls/AnimationController.h"
#include "neo_root_settings.h"
#include "glow_outline_effect.h"
#include "smoke_fog_overlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_hud_context_hint_enabled("cl_neo_hud_context_hint_enabled", "1",
	FCVAR_ARCHIVE, "Whether to show contextual hud hints. 0: Disabled, 1: Enabled.", false, 0, true, 1);

ConVar cl_neo_spec_takeover_player_hint_time_sec("cl_neo_spec_takeover_player_hint_time_sec", "3.0",
	FCVAR_ARCHIVE, "Duration in seconds to display the spectator bot takeover hint.", true, 0, true, 9999);

DECLARE_HUDELEMENT(CNEOHud_ContextHint);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(ContextHint, 0.1);

CNEOHud_ContextHint::CNEOHud_ContextHint(const char* pElementName)
	: CNEOHud_ChildElement(), CHudElement(pElementName), vgui::EditablePanel(NULL, "neo_context_hint")
{
	SetParent(g_pClientMode->GetViewport());
	SetVisible(false);

	m_flDisplayEndTime = 0.0f;
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
	m_flDisplayEndTime = 0.0f;
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
		g_GlowObjectManager.ClearUseItemGlowObject();
		return false;
	}

	return true;
}

extern ConVar sv_neo_spec_replace_player_bot_enable;
extern ConVar sv_neo_spec_replace_player_min_exp;
extern ConVar sv_neo_bot_cmdr_enable;
ConVar cl_neo_hud_context_hint_show_player_takeover_hint("cl_neo_hud_context_hint_show_player_takeover_hint", "1", FCVAR_ARCHIVE, "Show player takeover hint", true, 0.f, true, 1.f);
ConVar cl_neo_hud_context_hint_show_object_interact_hint("cl_neo_hud_context_hint_show_object_interact_hint", "1", FCVAR_ARCHIVE, "Show object inteact hint", true, 0.f, true, 1.f);
ConVar cl_neo_hud_context_hint_show_bot_interact_hint("cl_neo_hud_context_hint_show_bot_interact_hint", "1", FCVAR_ARCHIVE, "Show bot command and weapon request hint", true, 0.f, true, 1.f);
void CNEOHud_ContextHint::UpdateStateForNeoHudElementDraw()
{
	g_GlowObjectManager.ClearUseItemGlowObject();

	C_NEO_Player* pLocalNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalNeoPlayer)
		return;

	char szUppercaseKeyBinding[16]; // Assuming keybinds won't exceed 15 characters + null terminator
	const char* useKeyBinding = engine->Key_LookupBinding("+use");
	if (useKeyBinding && useKeyBinding[0] != '\0')
	{
		V_strncpy(szUppercaseKeyBinding, useKeyBinding, sizeof(szUppercaseKeyBinding));
		V_strupr(szUppercaseKeyBinding);
	}
	else
	{
		const char notBoundText[] = "+use unbound\0";
		COMPILE_TIME_ASSERT(sizeof(notBoundText) <= sizeof(szUppercaseKeyBinding));
		V_strncpy(szUppercaseKeyBinding, notBoundText, sizeof(szUppercaseKeyBinding));
	}

	if (pLocalNeoPlayer->IsObserver())
	{
		if (!cl_neo_hud_context_hint_show_player_takeover_hint.GetBool())
			return;

		// Takeover hint
		{
			bool showTakeOverHint = false;
			if (auto eObserverMode = pLocalNeoPlayer->GetObserverMode();
				(eObserverMode == OBS_MODE_CHASE || eObserverMode == OBS_MODE_IN_EYE))
			{
				if (C_NEO_Player* pObserverTargetPlayer = ToNEOPlayer(pLocalNeoPlayer->GetObserverTarget());
					pObserverTargetPlayer && pObserverTargetPlayer->ValidTakeoverTargetFor(pLocalNeoPlayer))
				{
					// update hint duration
					if (pObserverTargetPlayer != m_hLastSpecTarget.Get())
					{
						m_flDisplayEndTime = gpGlobals->curtime + cl_neo_spec_takeover_player_hint_time_sec.GetFloat();
						m_hLastSpecTarget = pObserverTargetPlayer;

						// NEO NOTE (Adam) currently this is the only hint we can show when observing. If the hint text ever changes while observer target remains the same, will need to update this more often
						V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] Control Bot", szUppercaseKeyBinding);
					}
					
					showTakeOverHint = true;
				}
			}

			if (!showTakeOverHint)
			{
				m_flDisplayEndTime = gpGlobals->curtime;
				m_hLastSpecTarget = INVALID_EHANDLE;
			}
		}
	}
	else
	{
		constexpr float ITEM_DISCOVERY_SMOKE_THRESHOLD = 0.8f;
		if (CBaseEntity *pUseEntity = pLocalNeoPlayer->FindUseEntity();
			pUseEntity && (g_SmokeFogOverlayThermalOverride || g_SmokeFogOverlayAlpha < ITEM_DISCOVERY_SMOKE_THRESHOLD))
		{
			g_GlowObjectManager.SetUseItemGlowObject(pUseEntity, Vector( 1.0f, 1.0f, 1.0f ), g_SmokeFogOverlayThermalOverride ? 1.0f : Max( 0.0f, 1.0f - g_SmokeFogOverlayAlpha), true, false);
			
			if (!cl_neo_hud_context_hint_show_object_interact_hint.GetBool())
				return;
			
			// Weapon pickup hint
			if (pUseEntity->IsBaseCombatWeapon())
			{
				C_NEOBaseCombatWeapon* pNeoWeapon = static_cast<C_NEOBaseCombatWeapon*>(pUseEntity);
				
				// Ghost pickup hint
				if (pNeoWeapon->GetNeoWepBits() & NEO_WEP_GHOST)
				{
					V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] pickup the Ghost", szUppercaseKeyBinding);
					m_flDisplayEndTime = gpGlobals->curtime + 1.f;
				}
				// Weapon pickup hint
				else if (pNeoWeapon->CanBePickedUpByClass(pLocalNeoPlayer->GetClass()))
				{
					V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] pickup %hs", szUppercaseKeyBinding, pNeoWeapon->GetPrintName());
					m_flDisplayEndTime = gpGlobals->curtime + 1.f;
				}
				else if (pLocalNeoPlayer->m_nButtons & IN_USE)
				{
					V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"Cannot pickup weapon");
					m_flDisplayEndTime = gpGlobals->curtime + 1.f;
					// g_GlowObjectManager.ClearUseItemGlowObject(); // NEO TODO (Adam) Clear highlight? or show which weapon cannot be picked up? Change highlight colour to red?
				}
				else
				{
					g_GlowObjectManager.ClearUseItemGlowObject();
				}
			}
			else
			{
				// Juggernaut hint
				if (Q_strcmp(pUseEntity->GetClassname(), "class C_NEO_Juggernaut") == 0)
				{
					if (CNEO_Juggernaut* pJuggernaut = static_cast<CNEO_Juggernaut*>(pUseEntity);
						pJuggernaut)
					{
						m_flDisplayEndTime = gpGlobals->curtime + 1.f;
						if (pJuggernaut->m_bLocked)
						{
							V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"Juggernaut is locked");
						}
						else if (C_NEO_Player* pNeoJuggernautPlayer = pJuggernaut->m_hHoldingPlayer.Get();
							pNeoJuggernautPlayer && pJuggernaut->m_bIsHolding)
						{
							g_GlowObjectManager.ClearUseItemGlowObject();
							m_flDisplayEndTime = gpGlobals->curtime;
						}
						else
						{
							V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"Hold [%hs] take the Juggernaut", szUppercaseKeyBinding);
						}
					}
					else
					{
						m_flDisplayEndTime = gpGlobals->curtime;
					}
				}
				// Some other useable entity
				else
				{
					V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] use", szUppercaseKeyBinding);
					m_flDisplayEndTime = gpGlobals->curtime + 1.f;
				}
			}
		}
		else
		{
			m_flDisplayEndTime = gpGlobals->curtime;
			if (!cl_neo_hud_context_hint_show_bot_interact_hint.GetBool())
				return;

			// Bot command hint
			{
				if (C_NEO_Player* pTargetPlayer = pLocalNeoPlayer->PlayerUseTraceLine();
					pTargetPlayer
					&& GameResources()->IsFakePlayer(pTargetPlayer->entindex())
					&& NEORules()->IsTeamplay()	&& pTargetPlayer->GetTeamNumber() == pLocalNeoPlayer->GetTeamNumber())
				{
					Vector teamGlowColor { 1.f, 1.f, 1.f };
					NEORules()->GetTeamGlowColor(pTargetPlayer->GetTeamNumber(), teamGlowColor[0], teamGlowColor[1], teamGlowColor[2]);
					g_GlowObjectManager.SetUseItemGlowObject(pTargetPlayer, teamGlowColor, g_SmokeFogOverlayThermalOverride ? 1.0f : Max(0.0f, 1.0f - g_SmokeFogOverlayAlpha), false, true);
			
					m_flDisplayEndTime = gpGlobals->curtime + 1.f;
					if (sv_neo_bot_cmdr_enable.GetBool())
					{
						V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] %hs %hs", szUppercaseKeyBinding, pTargetPlayer->m_hCommandingPlayer.Get() == pLocalNeoPlayer ? "release" : "command", pTargetPlayer->GetNeoPlayerName());
					}
					else
					{
						V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"[%hs] request primary weapon", szUppercaseKeyBinding); // NEO TODO (Adam) network primary weapon so can print its name here?
					}
					// else if NEO TODO (Adam) check if bots are frozen because of no navigation mesh or nb_player_stop, show appropriate message here?
				}
			}
		}
	}
}

void CNEOHud_ContextHint::DrawNeoHudElement()
{
	if (m_wszHintText[0] == L'\0')
		return;

	if (m_flDisplayEndTime <= gpGlobals->curtime)
		return;

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

	// NEO TODO (Adam) different text colour for keybind, instruction and target name?
}

void CNEOHud_ContextHint::Paint()
{
	PaintNeoElement();
	BaseClass::Paint();
}

void CNEOHud_ContextHint::FireGameEvent(IGameEvent* event)
{
}