#include "cbase.h"
#include "neo_hud_spectator_takeover.h"

#include "iclientmode.h"
#include "c_neo_player.h"
#include "vgui/ISurface.h"
#include "neo_ui.h"
#include "igameresources.h"
#include "ienginevgui.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_spec_replace_player_hint_time_sec("cl_neo_spec_replace_player_hint_time_sec", "3.0",
	FCVAR_ARCHIVE, "Duration in seconds to display the spectator bot takeover hint.");

DECLARE_HUDELEMENT(CNEOHud_SpectatorTakeover);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(SpectatorTakeover, 0.00695);

CNEOHud_SpectatorTakeover::CNEOHud_SpectatorTakeover(const char* pElementName)
	: CNEOHud_ChildElement(), CHudElement(pElementName), vgui::Panel(NULL, "neo_spectator_takeover")
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

CNEOHud_SpectatorTakeover::~CNEOHud_SpectatorTakeover()
{
	NeoUI::FreeContext(&m_uiCtx);
}

void CNEOHud_SpectatorTakeover::Init()
{
}

void CNEOHud_SpectatorTakeover::VidInit()
{
}

void CNEOHud_SpectatorTakeover::Reset()
{
	m_flDisplayTime = 0.0f;
	m_bIsDisplayingHint = false;
	m_bHintShownForCurrentTarget = false;
	m_hLastSpectatedTarget = nullptr;
	SetVisible(false);
}

void CNEOHud_SpectatorTakeover::ApplySchemeSettings(vgui::IScheme* pScheme)
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

	KeyValues* pKV = new KeyValues("HudLayout");
	if (pKV->LoadFromFile(reinterpret_cast<IBaseFileSystem*>(g_pFullFileSystem), "scripts/HudLayout.res"))
	{
		KeyValues* pSpectatorTakeoverKV = pKV->FindKey("neo_spectator_takeover");
		if (pSpectatorTakeoverKV)
		{
			float flUIScale = pSpectatorTakeoverKV->GetFloat("ui_scale", 0.66f);
			m_uiCtx.layout.iDefRowTall = (iScrTall / pSpectatorTakeoverKV->GetInt("def_row_tall_divisor", 27)) * flUIScale;
			m_uiCtx.iMarginX = (iScrWide / pSpectatorTakeoverKV->GetInt("margin_x_divisor", 192)) * flUIScale;
			m_uiCtx.iMarginY = (iScrTall / pSpectatorTakeoverKV->GetInt("margin_y_divisor", 108)) * flUIScale;
		}
		else
		{
			// Fallback to default values if the key is not found
			float flUIScale = 0.66f;
			m_uiCtx.layout.iDefRowTall = (iScrTall / 27) * flUIScale;
			m_uiCtx.iMarginX = (iScrWide / 192) * flUIScale;
			m_uiCtx.iMarginY = (iScrTall / 108) * flUIScale;
		}
	}
	else
	{
		// Fallback to default values if the file cannot be loaded
		float flUIScale = 0.66f;
		m_uiCtx.layout.iDefRowTall = (iScrTall / 27) * flUIScale;
		m_uiCtx.iMarginX = (iScrWide / 192) * flUIScale;
		m_uiCtx.iMarginY = (iScrTall / 108) * flUIScale;
	}
	pKV->deleteThis();
}

bool CNEOHud_SpectatorTakeover::ShouldDraw()
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

void CNEOHud_SpectatorTakeover::UpdateStateForNeoHudElementDraw()
{
	SetVisible(ShouldDraw());

	if (IsVisible())
	{
		const char* useKeyBinding = engine->Key_LookupBinding("+use");
		if (useKeyBinding && useKeyBinding[0] != '\0')
		{
			char szUppercaseKeyBinding[16]; // Assuming keybinds won't exceed 15 characters + null terminator
			V_strncpy(szUppercaseKeyBinding, useKeyBinding, sizeof(szUppercaseKeyBinding));
			V_strupr(szUppercaseKeyBinding);
			V_snwprintf(m_wszHintText, ARRAYSIZE(m_wszHintText), L"Control Bot [%hs]", szUppercaseKeyBinding);
		}
		else
		{
			V_wcsncpy(m_wszHintText, L"Press Use To Control Bot", sizeof(m_wszHintText));
		}
	}
}

void CNEOHud_SpectatorTakeover::DrawNeoHudElement()
{
	if (!IsVisible())
		return;

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);

	int iTextWide, iTextTall;
	vgui::surface()->GetTextSize(m_uiCtx.fonts[NeoUI::FONT_NTHORIZSIDES].hdl, m_wszHintText, iTextWide, iTextTall);

	KeyValues* pKV = new KeyValues("HudLayout");
	int iPaddingX = m_uiCtx.iMarginX * 2; // Default values
	int iPaddingY = m_uiCtx.iMarginY * 2; // Default values
	int iBoxY = (iScrTall * 3) / 4; // Default values

	if (pKV->LoadFromFile(reinterpret_cast<IBaseFileSystem*>(g_pFullFileSystem), "scripts/HudLayout.res"))
	{
		KeyValues* pSpectatorTakeoverKV = pKV->FindKey("neo_spectator_takeover");
		if (pSpectatorTakeoverKV)
		{
			iPaddingX = m_uiCtx.iMarginX * pSpectatorTakeoverKV->GetInt("padding_x_multiplier", 2);
			iPaddingY = m_uiCtx.iMarginY * pSpectatorTakeoverKV->GetInt("padding_y_multiplier", 2);
			iBoxY = (iScrTall * pSpectatorTakeoverKV->GetInt("box_y_numerator", 3)) / pSpectatorTakeoverKV->GetInt("box_y_divisor", 4);
		}
	}
	pKV->deleteThis();

	int iBoxWide = iTextWide + iPaddingX;
	int iBoxTall = iTextTall + iPaddingY;

	int iBoxX = (iScrWide - iBoxWide) / 2;

	m_uiCtx.dPanel.x = iBoxX;
	m_uiCtx.dPanel.y = iBoxY;
	m_uiCtx.dPanel.wide = iBoxWide;
	m_uiCtx.dPanel.tall = iBoxTall;

	NeoUI::BeginContext(&m_uiCtx, NeoUI::MODE_PAINT, nullptr, "CNEOHud_SpectatorTakeover");
	NeoUI::BeginSection();
	{
		NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
		m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

		NeoUI::Label(m_wszHintText);
	}
	NeoUI::EndSection();
	NeoUI::EndContext();
}

void CNEOHud_SpectatorTakeover::Paint()
{
	PaintNeoElement();
	BaseClass::Paint();
}

void CNEOHud_SpectatorTakeover::FireGameEvent(IGameEvent* event)
{
}

